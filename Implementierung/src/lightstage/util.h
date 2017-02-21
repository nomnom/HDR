// Utility functions and types
#ifndef UTIL_H
#define UTIL_H

// OpenCV
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O
#include "opencv2/gpu/gpu.hpp"

#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <time.h>



#ifdef __APPLE__
 #include <sys/time.h>
#endif



using namespace std;
using namespace cv;



// use OpenCV GPU functions instead of the CPU ones
//#define USE_GPU

#ifdef USE_GPU
 #define MAT gpu::GpuMat
#else
 #define MAT Mat
#endif

//
// timing
//


// aquire timestamp
// for OSX use gettimeofday(), on linux use clock_gettime() 
inline void clock(timespec& t)
{ 
  #ifdef __APPLE__
    struct timeval now;
    gettimeofday(&now, NULL);
    t.tv_sec  = now.tv_sec;
    t.tv_nsec = now.tv_usec * 1000;
  #else
    clock_gettime(CLOCK_REALTIME, &t);
  #endif
}

// simple stopwatch (only for use in one thread!)
inline double elapsed_ms (timespec& ts, timespec& te) 
{ 
    return  (double)te.tv_nsec/1e6 + (double)te.tv_sec*1e3 - ((double)ts.tv_nsec/1e6 + (double)ts.tv_sec*1e3 );
}
extern timespec tstart, tend;
inline void sw_start() { clock(tstart); }
inline void sw_stop()   { clock(tend); }
inline double sw_elapsed_ms () { return elapsed_ms(tstart, tend); }


inline void sleep_nano (unsigned long nano)
{
    this_thread::sleep_for(chrono::nanoseconds(nano));
}

// precision sleep
inline void sleep (double seconds)
{
    if (seconds > 0) sleep_nano ((unsigned long)(seconds*1e9));
}

//
// coordinate systems
//


// spherical: (r,phi, theta)
// cartesian: (x,y,z)
//    phi: polar, against Z
//    theta: azimuth, against X (CCW)
Matx31d spher2cart (Matx31d in);
Matx31d cart2spher (Matx31d in);


//
// general image / vector stuff
//


// convert 3d world coordinates into screen position
Point  space2screen (Matx33d camMat, Matx44d transMat, Matx31d space);

//  clamp float value;
inline double clamp (double val, double lower, double upper)
{
    if (val < lower) return lower;
    if (val > upper ) return upper;
    return val;
}
    
    
// clamp int value, lower bound included, upper bound excluded
inline int clamp (int val, int lower, int upper)
{ 
    if (val < lower) return lower;
    if (val >= upper) return (upper-1);
    return val;
}
//clamp image pixel values
Mat& clamp (Mat& A, float lower, float upper);

Mat& clamp (Mat& A, float lower);

Mat clamp(Mat& in, Mat& low, Mat& high);
// minmax for images with three channels (bgr order)
// 4-value array: r g b min(r,g,b)
void  min_max (Mat img, double (&minVals)[4], double (&maxVals)[4]);

//
// interpolation
//
    
// bilinear interpolation    samplept, upper left, bot. right, vals{ ul,  ur,  bl,  br }
//                    wiki:         P     (x1,y2)     (x2,y1)       Q12, Q22, Q11, Q21                   
Vec3f interpolate_bilinear (Point2f p, Point2f ul, Point2f br, vector<Vec3f> vals);


// bilinear interpolation  (float coordinates)             
Vec3f interpolate_bilinear (float x, float y, float x1, float y1, float x2, float y2, vector<Vec3f>& vals);

// bilinear interpolation  (float coordinates and single channel)
float interpolate_bilinear (float x, float y, float x1, float y1, float x2, float y2, float* vals);



// linear interpolate value for point p, where p1 and p2 ar the neighboring value (p1<p<p2)
Vec3f interpolate_linear (float p, float p1, float p2, Vec3f val1, Vec3f val2);


// linear interpolate value, for single float value
float interpolate_linear (float p, float p1, float p2, float val1, float val2);


//
// (response) curve stuff
//

typedef Size_<double> Size2d;

// bundles all SV response curve - related information (some are redundant)
class SVRInfo {
  public:
    int size;                           // number of patches
    vector<vector<Vec3f> > response;    // response curve of all patches
    vector<Vec3f> vMin;                 // minimal rel. radiance of the patches
    vector<Vec3f> vMax;                 // maximum rel. radiance of the patches
    double exposure;                    // exposure time used for calibration (for relating the relative radiance values)
    Size2d screenSize;                  // native screen size
    Size2d borderSize;                  // border size in pixels (vertical, horizontal)
    Size patchLayout;                   // number of patches in each direction
    double patchSize;                   // edge length of the square patches in pixels
    Mat colorTransMat;                  // color transformation matrix (cam_channels -> disp_channels)
    
    // check patchconfig for consistency
    // NOTE: run on original values before rescaling
    bool checkValues()
    {
        if ( ((int)(screenSize.width - 2*(int)borderSize.width) % (int)patchSize) != 0 ||
             ((int)(screenSize.height - 2*(int)borderSize.height) % (int)patchSize) != 0  ||
             (((int)screenSize.width - 2*(int)borderSize.width) / (int)patchSize) != patchLayout.width ||
             (((int)screenSize.height - 2*(int)borderSize.height) / (int)patchSize) != patchLayout.height) {
            cout << "Error: screen size, border size, patch size and patch config are not consistent!" << endl;
            return false;
        }
        if ((int)patchSize % 2 != 0) {
            cout << "Error: patchsize is not divisible by 2!" << endl;
            return false;
        }
        return true;
    }    
    
    // rescale so we can reduce the virtual display size; crop border
    void rescale(double scale)
    {
        patchSize *= scale;
        borderSize.width *= scale;
        borderSize.height *= scale;
        screenSize.width *= scale;
        screenSize.height *= scale;
    }
};


// linear scale value val, so that minVal is mapped to 0.0 and maxVal is mapped to 1.0
float fit_in_range (float val, float minVal, float maxVal);

// rescales every elemen to that the value minVal is mapped to (0,0,0) and maxVal to (1,1,1)
vector<Vec3f>& fit_in_range ( vector<Vec3f>& curve, Vec3f minVal, Vec3f maxVal);


// apply response to one pixel
Vec3f lookup_response (Vec3f val, SVRInfo& svr, int idx);

// apply response to one sub-pixel
float lookup_response_subpixel (float& val, SVRInfo& svr, int idx, int channel);


// spatially varying response with bilinar interpolation
// apply resopnse to whole display (faster interpolation because we can iterate over the patches)
void apply_response_svr( Mat& img, SVRInfo& svr );

// apply SVR response to one channel of a specific pixel
// adapted from apply_response_svr:
float apply_response_svr_subpixel ( float val, SVRInfo& svr, double posX, double posY, int channel );


// get minimum or maximum possible screen radiance
float get_min_max_subpixel (SVRInfo& svr, Point2d pos, int channel, bool useMin );
void get_min_max_screen ( Mat& img, SVRInfo& svr, bool useMin );

// calculate dynamic range via min/max screen radiance
double screen_dynamic_range ( Mat& minRadiance, Mat& maxRadiance );

//
// remote camera control
//

// set canon parameters
int remote_setup ();

// capture an image with specific exposure time, aperture and the filename (if image should be downloaded)
int remote_capture(float exposure, float aperture, string filename);

// start capture in bulb mode; user can specify aperture and filename (if image should be downloaded)
int remote_start_exposure (float aperture, string filename);
int remote_end_exposure ();

//
// backlight control
//

int set_backlight (double value);

//
// sound notifications
//

enum SOUNDS { ERROR,    // critical error: no getting away from this one
              WARNING,  // a warning: something is not perfect
              START,    // process has started
              SEARCH,   // waiting for device position
              CAPTURE_START, // image exposure has begun
              CAPTURE_END,   // image exposure is finished
              FINISH,   // process has finished
              CLOSER,   // distance is too large
              AWAY,     // distance is too close
              ANGLE,    // light angle is to steep
              PROC_START, // process started
              PROC_END, // process ended
              POSITION, // bad position
              OVERLAP, MORE, LESS,   // not enough / too much overlap
              ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, // degree tilt angle
              LEFT, RIGHT, TOP
              };
              

// play back the specified sound notification
int play_sound ( SOUNDS sound );

#endif //  UTIL_H
