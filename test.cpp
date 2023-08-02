#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace std;
using namespace cv;

int main5() {

    VideoCapture cap(0);
    Mat img;

    while (true) {

        cap.read(img);
        imshow("Image", img);
        waitKey(1);
    }
}