// HDR display tracking lightstage

#ifndef LIGHSTAGE_H
#define LIGHSTAGE_H

// OpenCV
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window and video I/O

// C
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <time.h>


#include "util.h"
#include "tracking.h"
#include "spherical.h"
#include "cube.h"


using namespace std;
using namespace cv;


// print commandline usage
void help();

// corrects the relative radiance (cos(phi) in debevec environment maps)
//Mat& correct_envmap_cosphi (Mat& env);

// first experimental run mode 
int run (int argc, char* argv[]);

Point2i get_shakeshift (Matx31d& screenCenter, Matx31d& newScreenCenter, Matx31d& newDown, Matx31d& newRight, Size2d& screenSizeMm, Size2i& screenSizeNoBorder);
/*
// for communication with display thread
class displayData {
  public:
    Mat& screenBuff;
    Mat& blackFrame;
    double duration;
    displayData (Mat& screenBuff, Mat& blackFrame, double duration) : screenBuff(screenBuff), blackFrame(blackFrame), duration(duration) {}
};

// display the screenbuffer for the precise duration in a separate thread
void start_display_thread(displayData data);
static void* display_thread (void* ptr);
*/


// for communication with capture thread
struct captureData {
    char* filename;
    double exposure;
    double aperture;
    captureData (string file, double exposure, double aperture) : exposure(exposure), aperture(aperture) 
    {
       filename = (char*)malloc(256);
       strcpy(filename, file.c_str());
    }
};

// remote DSLR control: separate control thread
void start_capture_thread(captureData data);
static void* capture_thread (void* ptr);



int main (int argc, char* argv[]);

#endif //  LIGHSTAGE_H
