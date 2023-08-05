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
Point screenSize = Point(2400, 1080);

int xa = boxCentre.x - boxSize / 2;
int xb = boxCentre.x + boxSize / 2;
int ya = boxCentre.y - boxSize / 2;
int yb = boxCentre.y + boxSize / 2;

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

bool onSegment(Point p, Point q, Point r) {
	// https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/ - line intersect checks
	if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
		q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
		return true;

	return false;
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are collinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int orientation(Point p, Point q, Point r)
{
	// See https://www.geeksforgeeks.org/orientation-3-ordered-points/
	// for details of below formula.
	int val = (q.y - p.y) * (r.x - q.x) -
		(q.x - p.x) * (r.y - q.y);

	if (val == 0) return 0;  // collinear

	return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool doIntersect(Point p1, Point q1, Point p2, Point q2)
{
	// Find the four orientations needed for general and
	// special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases
	// p1, q1 and p2 are collinear and p2 lies on segment p1q1
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and q2 are collinear and q2 lies on segment p1q1
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are collinear and p1 lies on segment p2q2
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are collinear and q1 lies on segment p2q2
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases
}

Point intersection(Vec4i a, Vec4i b) {
	double d = (a[0] - a[2]) * (b[1] - b[3]) - (a[1] - a[3]) * (b[0] - b[2]); // determinant?
	//if (d == 0) return ;

	double xi = ((b[0] - b[2]) * (a[0] * a[3] - a[1] * a[2]) - (a[0] - a[2]) * (b[0] * b[3] - b[1] * b[2])) / d;  // found on stack, probably cramer's rule
	double yi = ((b[1] - b[3]) * (a[0] * a[3] - a[1] * a[2]) - (a[1] - a[3]) * (b[0] * b[3] - b[1] * b[2])) / d;

	return Point(xi, yi);
}

double pointDistanceToCentre(Point a) {
	return sqrt(pow(a.x - screenSize.x / 2, 2) + pow(a.y - screenSize.y / 2, 2));
}



int main() {



	LPCWSTR window_title = L"2107113SG";
	HWND hWND = FindWindow(NULL, window_title);


	bool setup = false;  // gives a few seconds to setup the screen after initiation
	bool reversed = false; // prevents reverse lock cycle; initiates as false

	while (true) {

		Mat imgRaw = getMat(hWND);
		Mat imgBox = imgRaw(Range(ya, yb), Range(xa, xb));
		Mat imgGrayBox, imgBlurBox, edges;
		cvtColor(imgBox, imgGrayBox, cv::COLOR_BGR2GRAY); // it seems for c++ the target image also needs to be specified)
		GaussianBlur(imgGrayBox, imgBlurBox, Size(3, 3), 0);
		Canny(imgBlurBox, edges, 100, 200);    // original thresholds 100, 200
		vector<Vec4i> lines;
		HoughLinesP(edges, lines, 1, CV_PI / 180, 1, 0, 0);

		if (lines.size() == 0) {
			for (int i = 7; i > 0; i--) {
				cout << "Game Restart, T - " << i << " seconds" << endl;
				Sleep(1000);
			}
		}
		else {
			vector<Point> points;
			vector<double> pointsDistance;

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

			Mat imgRawResize, imgGray, imgBlur, edgesLanes;
			resize(imgRaw, imgRawResize, Size(screenSize.x / 4, screenSize.y / 4), INTER_LINEAR);
			cvtColor(imgRawResize, imgGray, cv::COLOR_BGR2GRAY);
			GaussianBlur(imgGray, imgBlur, Size(3, 3), 0);
			Canny(imgBlur, edgesLanes, 100, 200);
			vector<Vec4i> boundaries;
			HoughLinesP(edgesLanes, boundaries, 1, CV_PI / 180, 80 / 4, 300 / 4, 8 / 4);   // divide used to make it easier to change detection resolution

			for (size_t j = 0; j < boundaries.size(); j++) {
				line(imgRaw, Point(boundaries[j][0] * 4, boundaries[j][1] * 4), Point(boundaries[j][2] * 4, boundaries[j][3] * 4), Scalar(0, 255, 0), 2);
			}

			/*
			vector<double> anglesVar{-CV_PI * 0.3, 0, CV_PI * 0.3};
			vector<Vec4i> datalineSetup;
			vector<double> datalineDistance;
			vector<Point> endPoints;
			double minDist, angleDelta;
			Point intersect, point1, point2;
			*/

			vector<double> anglesVar{-CV_PI * 0.3, 0, CV_PI * 0.3};
			vector<Vec4i> datalineSetup;
			vector<double> datalineDistanceLanes, datalineDistanceEdges;
			vector<Point> endPoints;     // end points for the setup datalines
			double minDistLanes, minDistEdges, angleDelta;
			Point intersect, point1, point2;

			for (int i = 0; i < 3; i++) {
				minDistLanes = 1000, minDistEdges = 1000;    // also doubles as the length of the setup datalines
				endPoints.push_back(Point(screenSize.x / 2 + int(1000 * cos(angle + anglesVar[i])), screenSize.y / 2 - int(1000 * sin(angle + anglesVar[i]))));
				datalineSetup.push_back(Vec4i(screenSize.x / 2, screenSize.y / 2, endPoints[i].x, endPoints[i].y));

				for (int n = 0; n < boundaries.size(); n++) {
					point1 = Point(boundaries[n][0] * 4, boundaries[n][1] * 4);
					point2 = Point(boundaries[n][2] * 4, boundaries[n][3] * 4);
					if (doIntersect(Point(screenSize.x / 2, screenSize.y / 2), endPoints[i], point1, point2)) {
						intersect = intersection(datalineSetup[i], Vec4i(boundaries[n][0] * 4, boundaries[n][1] * 4, boundaries[n][2] * 4, boundaries[n][3] * 4));
						angleDelta = abs(angle - atan2(point2.y - point1.y, point2.x - point1.x));
						
						if (pointDistanceToCentre(intersect) < minDistEdges && angleDelta < 1.0 || angleDelta > 3.0) {    // edges, this ignores the edge case where the angle and line angle cross the atan2 - to + crossover, but this shouldnt be encountered
							minDistEdges = pointDistanceToCentre(intersect);     // Red
						}
						else if (pointDistanceToCentre(intersect) < minDistLanes && 1.0 > angleDelta > CV_PI * 0.5 || 3.0 < angleDelta < CV_PI ) {
							minDistLanes = pointDistanceToCentre(intersect);   // Blue
						}
					}
				}
				datalineDistanceLanes.push_back(minDistLanes);
				datalineDistanceEdges.push_back(minDistEdges);
				line(imgRaw, Point(screenSize.x / 2 + 10, screenSize.y / 2 + 10), Point(screenSize.x / 2 + int(minDistLanes * cos(angle + anglesVar[i])), screenSize.y / 2 - int(minDistLanes * sin(angle + anglesVar[i]))), Scalar(255, 0, 0), 3);
				line(imgRaw, Point(screenSize.x / 2, screenSize.y / 2), Point(screenSize.x / 2 + int(minDistEdges * cos(angle + anglesVar[i])), screenSize.y / 2 - int(minDistEdges * sin(angle + anglesVar[i]))), Scalar(0, 0, 255), 3);
			
			
			}

			




			Mat img;
			//Mat justDatalinesResize;
			resize(imgRaw, img, Size(screenSize.x / 6, screenSize.y / 6), INTER_LINEAR);
			imshow("Output", img);
			moveWindow("Output", 0, 0);
			waitKey(1);

			if (not setup) {
				for (int i = 5; i > 0; i--) {
					cout << "Ready, T - " << i << " seconds" << endl;
					Sleep(1000);
				}
				setup = true;
			}






			// Input Section



			// dx and dy values are a bit weird but these work ish (laptop screen size = 3456 x 2160)

			INPUT ip = {};   // mouse clicks
			ip.type = 0;
			ip.mi.dy = 2160 / 2 * 20;

			INPUT key = {};  // press down key for reverse
			key.type = INPUT_KEYBOARD;
			key.ki.wVk = 0x28;



			if (datalineDistanceEdges[1] < 500) {    // turn left/right if an edge is in front
				if (datalineDistanceLanes[2] < datalineDistanceLanes[0]) {
					cout << "Right (wall) - 0.7 s" << endl;
					ip.mi.dx = 3456 / 2 * 20; // turn right for more
					ip.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN);
					SendInput(1, &ip, sizeof(INPUT));
					Sleep(700);
				}
				else {
					cout << "Left (wall) - 0.7 s" << endl;
					ip.mi.dx = 3456 / 2 * 17; // turn left for more
					ip.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN);
					SendInput(1, &ip, sizeof(INPUT));
					Sleep(700);
				}
			}
			else if (datalineDistanceLanes[2] < 300) {     // lanes on the right
				ip.mi.dx = 3456 / 2 * 20; // turn right
				ip.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN);
				SendInput(1, &ip, sizeof(INPUT));
				Sleep((300 - datalineDistanceLanes[2]) * 0.5);
				ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &ip, sizeof(INPUT));
				cout << "Right" << endl;
			}
			else if (datalineDistanceLanes[0] < 300) {    // lanes on the left
				ip.mi.dx = 3456 / 2 * 17; // turn left
				ip.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN);
				SendInput(1, &ip, sizeof(INPUT));
				Sleep((300 - datalineDistanceLanes[0]) * 0.5);
				ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &ip, sizeof(INPUT));
				cout << "Left" << endl;
			}

			else {
				ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &ip, sizeof(INPUT));
			}

			// Vehicle Crash postprocess: take another pic and if its too similar (imgRawNorm and imgRetakeNorm) reverse the vehicle backwards

			Mat imgRetake = getMat(hWND);
			Mat imgRetakeResize, imgRetakeGray, imgRawNorm, imgRetakeNorm, dif;
			resize(imgRetake, imgRetakeResize, Size(screenSize.x / 4, screenSize.y / 4), INTER_LINEAR);
			cvtColor(imgRetakeResize, imgRetakeGray, cv::COLOR_BGR2GRAY);
			normalize(imgGray, imgRawNorm, 0, 1, NORM_MINMAX, -1);
			normalize(imgRetakeGray, imgRetakeNorm, 0, 1, NORM_MINMAX, -1);

			absdiff(imgRawNorm, imgRetakeNorm, dif);

			// cout << countNonZero(dif) << endl;




			if (countNonZero(dif) < 5000) {    // 5000 threshold is arbitrary; seems to work at least in the early stages
				if (not reversed) {
					cout << "Start reversing..." << endl;
					SendInput(1, &key, sizeof(key));
					Sleep(2000);
					key.ki.dwFlags = KEYEVENTF_KEYUP;
					SendInput(1, &key, sizeof(key));
					reversed = true;
					cout << "Finished reversing" << endl;
				}
				else if (reversed) {
					cout << "Start reversing..." << endl;
					SendInput(1, &key, sizeof(key));
					Sleep(2000);
					key.ki.dwFlags = KEYEVENTF_KEYUP;
					SendInput(1, &key, sizeof(key));
					reversed = true;
					cout << "Finished reversing" << endl;
					/*
					cout << "Right (after reversing) - 1 s" << endl;
					SendInput(1, &key, sizeof(key));   // added a round of reverse to cover "reversed" boolean switch inaccuracies
					Sleep(1500);
					key.ki.dwFlags = KEYEVENTF_KEYUP;
					SendInput(1, &key, sizeof(key));
					ip.mi.dx = 3456 / 2 * 20; // turn right for more
					ip.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN);
					SendInput(1, &ip, sizeof(INPUT));
					Sleep(1000);
					ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(1, &ip, sizeof(INPUT));
					reversed = false;
					cout << "Continue!" << endl;
					*/
				}
			}
			else { reversed = false; }


			cout << endl;




		}



	}

}
