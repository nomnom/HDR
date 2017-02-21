#ifndef DISPLAY_CONTROL_H
#define DISPLAY_CONTROL_H

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <chrono>
#include <time.h>

#include "util.h"



/**
   Datastructure for passing capture configuration to the capture thread
*/
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


/**
  remote DSLR control: start control thread
*/
void start_capture_thread(captureData data);


/**
  remote DSLR control: thread
*/
static void* capture_thread (void* ptr);


/**
  calculate and show the hdr sequence
*/
int run_hdr_sequence (int argc, char *argv[]);

#endif //  DISPLAY_CONTROL_H
