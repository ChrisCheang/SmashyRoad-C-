#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/imgcodecs.hpp>

#include <iostream>

#include <vector>
#include <Windows.h>
#include <tuple>
#include <cmath>
#include <list>
#include <algorithm>

#define WINVER 0x0500

using namespace std;
using namespace cv;

// test git.

/* opencv setup :

	https://subwaymatch.medium.com/opencv-410-with-vs-2019-3d0bc0c81d96
	https://www.tutorialspoint.com/how-to-install-opencv-for-cplusplus-in-windows

*/

//Comment off so some stuff isn't defined twice

// boxCentre of phone screen (172, 130),   boxSize = 42
Point boxCentre = Point(172, 130);
int boxSize = 42;
// Phone size = (2400, 1080)
// Weird error (26-7-2023): screen size changed somehow to (1940, 873), then boxCentre = (139, 105), boxSize = 34

// Data Setups //

Point screenSize = Point(2400, 1080);

int xa = boxCentre.x - boxSize / 2;
int xb = boxCentre.x + boxSize / 2;
int ya = boxCentre.y - boxSize / 2;
int yb = boxCentre.y + boxSize / 2;

vector<double> anglesVar{ -CV_PI * 0.2, 0, CV_PI * 0.2 }, laneAngles{ -CV_PI * 0.75, -CV_PI * 0.25, CV_PI * 0.25, CV_PI * 0.75 };

// End of Setups //

Mat getMat(HWND hWND) {

	HDC deviceContext = GetDC(hWND);
	HDC memoryDeviceContext = CreateCompatibleDC(deviceContext);

	RECT windowRect;
	GetClientRect(hWND, &windowRect);

	int height = screenSize.y;    // original code: windowRect.bottom
	int width = screenSize.x;		// original code: windowRect.right

	HBITMAP bitmap = CreateCompatibleBitmap(deviceContext, width, height);

	SelectObject(memoryDeviceContext, bitmap);

	//copy data into bitmap
	BitBlt(memoryDeviceContext, 0, 0, width, height, deviceContext, 0, 0, SRCCOPY);


	//specify format by using bitmapinfoheader!
	BITMAPINFOHEADER bi{};
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0; //because no compression
	bi.biXPelsPerMeter = 1; //we
	bi.biYPelsPerMeter = 2; //we
	bi.biClrUsed = 3; //we ^^
	bi.biClrImportant = 4; //still we

	cv::Mat mat = cv::Mat(height, width, CV_8UC4); // 8 bit unsigned ints 4 Channels -> RGBA

	//transform data and store into mat.data
	GetDIBits(memoryDeviceContext, bitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	//clean up!
	DeleteObject(bitmap);
	DeleteDC(memoryDeviceContext); //delete not release!
	ReleaseDC(hWND, deviceContext);

	return mat;
}

int main() {



	LPCWSTR window_title = L"2107113SG";
	HWND hWND = FindWindow(NULL, window_title);


	bool setup = false;  // gives a few seconds to setup the screen after initiation
	bool reversed = false; // prevents reverse lock cycle; initiates as false
	bool driving = true; // boolean to identify driving or running states
	bool damaged = false; // boolean to notify when to change vehicles
	int redCounter = 1, smokeCounter = 1; // cumulative counters for damage and running states to notify the above

	while (true) {

		// line detection of top left little box for orientation + colour detection of middle bit for state (in vehicle or not)

		Mat imgRaw = getMat(hWND);
		Mat imgBox = imgRaw(Range(ya, yb), Range(xa, xb));

		Mat imgBoxMid = imgRaw(Range(250, 290), Range(60, 100));  // (Range(802, 810), Range(601, 612));
		Mat imgBoxRedHeart, imgBoxSmoke;
		inRange(imgBoxMid, Scalar(52, 54, 240), Scalar(67, 63, 253), imgBoxRedHeart);
		imgBoxMid = imgRaw(Range(screenSize.y / 2 - 100, screenSize.y / 2 + 100), Range(screenSize.x / 2 - 100, screenSize.x / 2 + 100)); // gives a slightly bigger detect area for smoke
		inRange(imgBoxMid, Scalar(55, 76, 86), Scalar(60, 83, 97), imgBoxSmoke);
		int redCount = countNonZero(imgBoxRedHeart);
		int smokeCount = countNonZero(imgBoxSmoke);


		if (redCount > 200) {
			if (redCounter <= 3) { ++redCounter; }
		}
		else {
			if (redCounter >= 1) { --redCounter; }
		}
		if (smokeCount > 10) {
			if (smokeCounter <= 20) { ++smokeCounter; }
		}
		else {
			if (smokeCounter >= 1) { --smokeCounter; }
		}

		if (redCounter > 0) { driving = false, cout << "Running... "; }
		else { driving = true; }
		if (smokeCounter > 10) { damaged = true, cout << "Smoking... "; }
		else { damaged = false; }

		// cout << "redCount = " << redCount << " smokeCount = " << smokeCount << " ";


		Mat imgGrayBox, imgBlurBox, edges;
		imshow("ha", imgBox);
		Mat testing;
		inRange(imgBox, Scalar(150, 70, 45), Scalar(255, 170, 100), testing);
		imshow("after inRange", testing);
		cvtColor(imgBox, imgGrayBox, cv::COLOR_BGR2GRAY); // it seems for c++ the target image also needs to be specified) // inRange(imgBox, Scalar(45, 100, 200), Scalar(75, 130, 200), imgBox);
		GaussianBlur(imgGrayBox, imgBlurBox, Size(3, 3), 0);
		Canny(imgBlurBox, edges, 100, 200);    // original thresholds 100, 200
		vector<Vec4i> lines;
		HoughLinesP(edges, lines, 1, CV_PI / 180, 1, 0, 0);
		imshow("Canny of small box", edges);

		//

		vector<Point> points;
		vector<double> pointsDistance;


		vector<Vec4i> datalineSetup;
		vector<double> datalineDistanceLanes, datalineDistanceEdges;
		vector<Point> endPoints;     // end points for the setup datalines
		double minDistLanes, minDistEdges, angleDelta;
		Point intersect, point1, point2;



		if (lines.size() == 0) {
			for (int i = 7; i > 0; i--) {
				cout << "Game Restart, T - " << i << " seconds" << endl;
				bool reversed = false;
				if (datalineDistanceEdges.size() != 0) { datalineDistanceEdges[1] = 1000; }
				Sleep(1000);
			}
		}
		else {


			for (size_t i = 0; i < lines.size(); i++) {
				points.push_back(Point(lines[i][0], lines[i][1]));
				points.push_back(Point(lines[i][2], lines[i][3]));
				pointsDistance.push_back(sqrt(pow((lines[i][0] - boxSize / 2), 2) + pow((lines[i][1] - boxSize / 2), 2)));
				pointsDistance.push_back(sqrt(pow((lines[i][2] - boxSize / 2), 2) + pow((lines[i][3] - boxSize / 2), 2)));   // this a[3] is a[1] instead in the python version, weirdly still works 
				line(imgRaw, Point(lines[i][0] + xa, lines[i][1] + ya), Point(lines[i][2] + xa, lines[i][3] + ya), Scalar(0, 255, 0), 2);
			}

			line(imgRaw, Point(xa, ya), Point(xb, yb), Scalar(255, 255, 255), 4);

			Point dirPoint1 = points[distance(pointsDistance.begin(), max_element(pointsDistance.begin(), pointsDistance.end()))]; // index of largest distance];
			Point dirPoint = Point(dirPoint1.x + xa - boxCentre.x, -(dirPoint1.y + ya - boxCentre.y));
			double angle = atan2(dirPoint.y, dirPoint.x);
			// cout << "Angle = " << angle << " radians" << endl;

			// line detection of resized whole screen for lanes and boundaries

			Mat imgRawResize, imgGray, imgBlur, edgesLanes;
			resize(imgRaw, imgRawResize, Size(screenSize.x / 4, screenSize.y / 4), INTER_LINEAR);
			cvtColor(imgRawResize, imgGray, cv::COLOR_BGR2GRAY);
			GaussianBlur(imgGray, imgBlur, Size(3, 3), 0);
			Canny(imgBlur, edgesLanes, 100, 200);
			vector<Vec4i> boundaries;
			HoughLinesP(edgesLanes, boundaries, 1, CV_PI / 180, 80 / 4, 300 / 4, 8 / 4);   // divide used to make it easier to change detection resolution

			//

			for (size_t j = 0; j < boundaries.size(); j++) {
				line(imgRaw, Point(boundaries[j][0] * 4, boundaries[j][1] * 4), Point(boundaries[j][2] * 4, boundaries[j][3] * 4), Scalar(0, 255, 0), 2);
			}


			Mat img;
			//Mat justDatalinesResize;
			resize(imgRaw, img, Size(screenSize.x / 6, screenSize.y / 6), INTER_LINEAR);
			imshow("Output", img);
			moveWindow("Output", 0, 0);
			waitKey(1);



			if (not setup) {
				for (int i = 3; i > 0; i--) {
					datalineDistanceEdges[1] = 1000;
					cout << "Ready, T - " << i << " seconds" << endl;
					Sleep(1000);
				}
				setup = true;
			}






			




		}



	}

}
