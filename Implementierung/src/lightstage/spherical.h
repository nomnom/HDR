// spherical environment map sampling

#ifndef SPHERICAL_H
#define SPHERICAL_H


#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window and video I/O

#include <time.h>
#include <iostream>
#include <string.h>

#include "util.h"


class SphericalEnvMap {

  public:
    SphericalEnvMap (Mat _envMap, SVRInfo _svr, Size _screenSizePixel, Size _screenSizeMm);
    ~SphericalEnvMap ();
    
    // recalculate orientation-dependend values
    //void recalculate (Matx31d screenNormal, Matx31d down, Matx31d right);
    
    // perform one illumination step; return screen image
    Mat& illuminate (Matx31d& screenCenter, Matx31d& down, Matx31d& right);
    
    // shows the environment map on the screen (no HDR routine and envMap subtraction)
    Mat& showEnvironment (Matx31d& screenCenter, Matx31d& down, Matx31d& right);
    
    // if illumination is completed (= envmap is zero)
    bool isComplete ();
    
    // get remaining light
    Mat& getRemaining() { return envMap; }
    
    

  private:
    
    // display response curve
    SVRInfo svr; 
    Size screenSizePixel;   // pixel
    Size screenSizeMm;      // mm
    
  
    // screen pixel values of last illumination
    Mat screen;
    
    // maximum possible light output of all screen pixel
    Mat maxLight;
    
    // original illumination
    Mat envMapOriginal;
    
    // remaining illumination (will be decreased in each illumination step)
    Mat envMap;
  
    // area of environment map element
    double delPhi, delTheta;
    
    // area of screen pixel
    double delX, delY;
    

};

#endif // SPHERICAL_H
