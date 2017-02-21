// camera tracking with ARToolKit

#ifndef TRACKING_H
#define TRACKING_H

// OpenCV
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window and video I/O

// ARToolKit
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/arMulti.h>


#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

using namespace std;
using namespace cv;

class Tracking
{
  
  public: 
    Tracking (VideoCapture& camCapture, 
              string camParamsFile, 
              string markerConfigFile,
              int threshold = 50, 
              bool greyscale = false,
              bool inverted = false);
    ~Tracking ();
    
    // treshold can be changed at runtime
    void setThreshold (int threshold) { thresh = threshold; }
    int getThreshold () { return thresh; }
    bool runAutoThreshold();
    
    // start/stop tracking thread
    void start();
    void stop();
    double lastTime (); // time in ms since last valid tracking position
    void lockThread() { readerLock = true; }
    void unlockThread(){ readerLock = false; }
    
    // if new position is available; resets hasNew to false
    bool hasNewData();
    
    // if last frame had visible marker
    bool hasVisibleMarker() { return markerDetected; }
    
    // if position was stable in the last n detections
    bool hasStablePosition (int n, double maxDist);
    
    int getNumMarker() { return numMarkersUsed; }
    
    // position error (from artoolkit, normalized with number of visible pattern)
    double getError() { return err; }
    
    
    
    // get latest position information
    Matx44d getTransform() { return transMat; }
    Matx33d getRotation() { return rotMat; }
    Matx31d getPosition() { return camPos; }
    
    // debug image for display
    void setDebug( bool val ) { debug = val; }
    Mat& getDebugImage() { return debugFrame; }
    
    
  private: 
    
    // camera stuff
    VideoCapture& capt;
    Mat frame;
    Mat cameraMatrix;
    Mat distCoeffs;
    Mat map1, map2;
    ARParam cameraParam;
    
    
    // thread stuff
    bool grab();
    static void* trackingLoop(void *ptr);
    bool running;
    bool lock;          // write-lock if a detection is currently in progress (shared ressources are written)
    bool readerLock;    // read-lock if a shared ressources are accessed from outside
    timespec tlast;     // last time a position was calculated  
    
    
    
    // thresholding
    bool useInverted;
    bool useGreyscale;
    const bool useAutoThreshold = false;
    int thresh;
    
    // marker config and detection
    ARMultiMarkerInfoT  *config;
    ARMarkerInfo* markerInfo;
    int markerNum;      // all detected possible marker
    int numMarkersUsed; // actually used number of marker

    bool detect();      // run marker detection and pos calculation
    bool markerDetected; // if last frame had visible markers
    
    // tracking information
    Matx44d transMat; // transformation matrix from world coordinates -> camera coordinates
    Matx33d rotMat; 
    Matx31d camPos;
    double err;
    bool hasNew;    // if new data is available
    
    #define maxLastPositions 20
    
    vector<Matx31d> lastPositions;

    // debug stuff
    bool debug;
    Mat debugFrame;
    void createDebugImage();
    
    

};

#endif // TRACKING_H
