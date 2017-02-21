// spherical environment map sampling

#ifndef CUBE_H
#define CUBE_H


#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window and video I/O

#include <time.h>
#include <iostream>
#include <string.h>

#include "util.h"


using namespace std;
using namespace cv;



class CubeMap {

  public:
    CubeMap (Mat& _envMap, SVRInfo _svr, Size _screenSizePixel, Size _screenSizeMm, Size2i borderRampSize);
    ~CubeMap ();
    
    // produce a series of hdr frames for illumination; uses range-maximization technique
    double calc_hdr_frames (vector<Mat>& frames, Matx31d& screenCenter, Matx31d& down, Matx31d& right,  Size2i screenSizeNoBorder, Size2i borderSize, int numFrames, double scale, bool applyCosFactor, double hdrSequenceMapBlurSize);
    
    // perspective projection matrix from one cube side onto screen 
    Mat get_perspective_transform (int cubeSide, Matx31d& screenCenter, Matx31d& down, Matx31d& right);
    
    // check if a cube side has to be projected (= is visible on the screen)
    bool is_side_visible (int side, Matx31d& screenCenter, Matx31d& down, Matx31d& right);
    
    // finds and returns the cube sides that have to be projected onto the screen (assumes screen distance to screen size ratio is large enough)
    vector<int> get_sides_to_project(Matx31d& screenCenter, Matx31d& down, Matx31d& right);

    // project cube map onto screen, with supersampling and cosine factor
    MAT& project_forward (MAT& screen, MAT& env, Matx31d& screenCenter, Matx31d& down, Matx31d& right, vector<int> sides = vector<int>(0));
    
    // project screen onto cube map, with supersampling and cosine factor 
    MAT& project_backward (MAT& env, MAT& screen, Matx31d& screenCenter, Matx31d& down, Matx31d& right, vector<int> sides = vector<int>(0));
    
    // shows the environment map on the screen (no HDR routine and envMap subtraction)
    Mat& show_environment (Matx31d& screenCenter, Matx31d& down, Matx31d& right);
    
    // if all pixel values are smaller than an epsilon)
    bool is_below_epsilon (MAT& img, double epsilon);
    
    // if illumination is completed (= envmap is zero)
    bool is_complete ();
    
    // calculates the maximum angle of the light rays exiting the display and hitting the scene with radius s (simple upper bound)
    double get_max_angle(Matx31d& screenCenter, Matx31d& down, Matx31d& right);

    
    // display response curve
    SVRInfo svr; 
    Size screenSizePixel;   // pixel
    Size screenSizeMm;      // mm
    
    // required screen radiance for last screen position    
    Mat screenRequired;
    // used screen radiance during last illumination
    Mat screenUsed;
    // alpha values vor border fading
    Mat borderRampMask;
    
    
    
    
    #ifdef USE_GPU 
        gpu::GpuMat screenUsedGPU; 
        gpu::GpuMat screenRequiredGPU;
        gpu::GpuMat borderRampMaskGPU;
    #endif
    
    // maximum and minimum possible radiance of all screen pixel;
    Mat maxLight;
    Mat maxScreenRadiance;  // (maxlight-minlight) * borderRampMask
    Mat minLight;
    
    // maximum radiance so that all pixels are capable of producing it
    //double maxRadiance;
    //double maxRadianceDF;  // minus minlight
    
    
    //
    // cube maps
    //
    // linear horizontal format (6 images concatenated) :
    //  +------------------------------+
    //  |  L | Ba |  R |  F |  T |  Bo |
    //  | -X | +Y | +X | -Y | +Z | -Z  |
    //  +------------------------------+
    
    // edge length of cube sides in pixel
    int cubeSize;
    
    // creates cube map this.envMap from a spherical environment map in lat/long format
    Mat& create_cubemap(Mat& cube, Mat& spherical);
    
    // get env map pixel for a specified light direction (as cartesian coordinates)
    Point2i get_cube_position ( Matx31d&  dir);
    
    // cube side normal for the light direction dir
    Matx31d get_cube_normal (Matx31d& dir);
    
    // light direction for the specified cube map pixel position
    Matx31d get_light_direction (Point2d& envMapPos);
    
    // original illumination (unmodified)
    MAT envMapOriginal;
    
    // remaining illumination (will be decreased in each illumination step)
    MAT envMapRemaining;
    
    // actual produced radiance in last step (will be subtracted from remaining light)
    MAT envMapUsed;
    
    // completed regions of environment map
    MAT envMapCompleted;
    
    #ifdef USE_GPU
        // cube side buffer for warping
       gpu::GpuMat cubeSideBufferGPU;
       gpu::GpuMat screenBufferGPU;
    #endif
    

    
    // area of screen pixel
    double delX, delY;
    
    

};

#endif // SPHERICAL_H
 
