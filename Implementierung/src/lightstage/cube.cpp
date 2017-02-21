/**
   lighstage : cube environment-map class
   
   This class contains everything related to cube maps: The perspective projection code, HDR-sequence code etc.
   
  @author Manuel Jerger <nom@nomnom.de>
*/

#include "cube.h"

// constructor
CubeMap::CubeMap (Mat& _envMap, 
                  SVRInfo _svr, 
                  Size _screenSizePixel, 
                  Size _screenSizeMm,
                  Size2i borderRampSize=Size(0,0))
 :svr(_svr), 
  screenSizePixel(_screenSizePixel), 
  screenSizeMm(_screenSizeMm)
{
    
    
    // autodetect: cube map
    if (_envMap.size().width > 5*_envMap.size().height) {
        cubeSize = _envMap.size().height;
        #ifdef USE_GPU
            envMapOriginal = gpu::GpuMat(_envMap);
        #else
            _envMap.copyTo(envMapOriginal);
        #endif
        
    // assume spherical environment map and convert
    } else {
        cubeSize = 1000;
        Mat tmpEnvMapOriginal;
        create_cubemap(tmpEnvMapOriginal, _envMap);
        #ifdef USE_GPU
            envMapOriginal = gpu::GpuMat(tmpEnvMapOriginal);
        #else
            tmpEnvMapOriginal.copyTo(envMapOriginal);
        #endif
    }
    
    assert (envMapOriginal.size().width == cubeSize * 6);
    
    cout << "have a cube map of size " << cubeSize << " x " << cubeSize << " pixel" << endl;
    
    #ifdef USE_GPU
        envMapRemaining = gpu::GpuMat(envMapOriginal);
        envMapUsed = gpu::GpuMat(Mat::zeros(envMapOriginal.size(), CV_32FC3));
        envMapCompleted = gpu::GpuMat(Mat::zeros(envMapOriginal.size(), CV_32FC3));
    #else
        envMapOriginal.copyTo(envMapRemaining);
        envMapUsed = Mat::zeros(envMapOriginal.size(), CV_32FC3);
        envMapCompleted = Mat::zeros(envMapOriginal.size(), CV_32FC3);
    #endif
    
    cout << "screenSizePixel = " << screenSizePixel << endl;
    
    // size of pixels (mm per pixel)
    delX = (double)screenSizeMm.width/(double)(screenSizePixel.width);
    delY = (double)screenSizeMm.height/(double)(screenSizePixel.height);

    // calculate maximum radiance we can produce at each pixel;
    cout << "calculating maximum and minimum radiance per display pixel ..." << flush;
    maxLight = Mat(screenSizePixel, CV_32FC3, CV_RGB(1,1,1));
    get_min_max_screen(maxLight, svr, false);
   // imwrite("tmp/maxLight.exr", maxLight);

    minLight = Mat(screenSizePixel, CV_32FC3, CV_RGB(0,0,0));
    get_min_max_screen(minLight, svr, true);
    cout << " done!" << endl;
   // imwrite("tmp/minLight.exr", minLight);
    
    
    screenRequired = Mat::zeros(screenSizePixel, CV_32FC3);
    screenUsed = Mat::zeros(screenSizePixel, CV_32FC3);
    
    //
    // border ramp alpha mask
    //
    
    borderRampMask = Mat(screenSizePixel, CV_32FC3, CV_RGB(1,1,1));
    
    // calculate border ramp alpha mask
    if (borderRampSize.width != 0 || borderRampSize.height != 0) {
        cout << "calculating border ramp alpha mask of size " << borderRampSize << endl;
        
        for (int y=0; y<borderRampSize.height; y++) {
            double val = ((double)(y+1)/(double)(borderRampSize.height+1));
            for (int x=0; x<screenSizePixel.width; x++) {
                for (int c=0; c<3; c++) {
                    borderRampMask.ptr<Vec3f>(y)[x][c] *= val;
                    borderRampMask.ptr<Vec3f>(screenSizePixel.height-1-y)[x][c] *= val;
                }
            }
        }
        for (int x=0; x<borderRampSize.width; x++) {
            double val = ((double)(x+1)/(double)(borderRampSize.width+1));
            for (int y=0; y<screenSizePixel.height; y++) {
                for (int c=0; c<3; c++) {
                    borderRampMask.ptr<Vec3f>(y)[x][c] *= val;
                    borderRampMask.ptr<Vec3f>(y)[screenSizePixel.width-1-x][c] *= val;
                }
            }
        }
    }
    
    // max screen radiance for one frame with applied border fading
    maxScreenRadiance = maxLight - minLight;
   
    #ifdef USE_GPU
        borderRampMaskGPU = gpu::GpuMat(borderRampMask);
        cubeSideBufferGPU = gpu::GpuMat(Mat::zeros(Size(cubeSize, cubeSize), CV_32FC3));
        screenBufferGPU = gpu::GpuMat(Mat::zeros(screenSizePixel, CV_32FC3));
    #endif
}

// destructor
CubeMap::~CubeMap () {}

/**
   Create cube-map from spherical environment-map (debevec lightprobe images, converted from sphere to polar representation)
   Performs a 25x Oversampling (regular 5x5 grid)
*/
Mat& CubeMap::create_cubemap (Mat& cube, Mat& spherical)
{
    cubeSize = 1000;
    // supersampling grid size (must be uneven)
    const int ss_grid_size = 5; 
    
    // shortcuts         
    const Vec3d  X = Vec3d(1,0,0);
    const Vec3d mX = Vec3d(-1,0,0);
    const Vec3d  Y = Vec3d(0,1,0);
    const Vec3d mY = Vec3d(0,-1,0);
    const Vec3d  Z = Vec3d(0,0,1);
    const Vec3d mZ = Vec3d(0,0,-1);
    
    //     0    1    2    3    4    5
    //  +------------------------------+
    //  |  L | Ba |  R |  F |  T |  Bo |
    //  | -X | +Y | +X | -Y | +Z | -Z  | fw-vec
    //  |  Y |  X | -Y | -X |  X |  X  | right-vec
    //  | -Z | -Z | -Z | -Z |  Y | -Y  | down-vec
    //  +------------------------------+
        
    // array of forwardm right and down vector for each cube side
    Vec3d forward[6] = { mX, Y, X, mY, Z, mZ };
    Vec3d right[6] = {  Y, X, mY, mX, X, X };
    Vec3d down[6]  = { mZ, mZ, mZ, mZ, Y, mY };
    
    cout << "calculating cube map from spherical environment map " << flush;
    
    // initialize 6 images
    Mat sides[6];
    for (int i=0; i<6; i++) {
        sides[i] = Mat(Size(cubeSize, cubeSize), CV_32FC3);
    }
    
    // for each cube sides do:
    //   set up image plane vectors (forward, up, right)
    //     for each pixel on image plane do:
    //        for each supersampling point in pixel do:
    //           calculate spherical coordinate
    //           lookup value in spherical envmap
    //        calculate average and set pixel value
    
    double p_delta = 2.0 / cubeSize; // 2x pixel size
    Matx31d ycomp;       // y-coordinate component of 3D pixel position
    
    for (int i=0; i<6; i++) {
        for (int y=0; y<cubeSize; y++) {
            for (int x=0; x<cubeSize; x++) {
                    
                // pixel value for supersampling averaging
                Vec3f pixel(0,0,0); 
                
                // supersampling grid
                int ss_side = (ss_grid_size-1)/2;           // we iterate from -ss_side  (inclusive) to +ss_side (inclusive)
                double ss_delta = 1.0 / (double)(ss_grid_size-1);  // distance between two supersampling points
                for (int sy=-ss_side; sy<=ss_side; sy++) {
                    ycomp = down[i] * p_delta * ((double)y-(double)(cubeSize-1)/2.0 + (double)sy*ss_delta);
                    for (int sx=-ss_side; sx<=ss_side; sx++) {
                        
                        // 3d position in space
                        Matx31d posCart = forward[i] + ycomp + right[i] * p_delta * ((double)x-(double)(cubeSize-1)/2.0 + (double)sx*ss_delta);
                        
                        // get spherical coordinates
                        Matx31d posSpher = cart2spher(posCart);
                        // sample spherical envmap
                        Point2i envMapPos; 
                        envMapPos.x =  (int)(0.5 + ( 1-(posSpher(2)/M_PI+1) / 2.0 * (float)spherical.size().width-1) );
                        envMapPos.y =  (int)(0.5 + (  posSpher(1)/M_PI            * (float)spherical.size().height-1));
                        pixel += spherical.ptr<Vec3f>(envMapPos.y)[envMapPos.x];
                        //cout << i << " " << x << " " << y << " " << envMapPos << endl;
                        
                    }
                }
                
                // average supersampled values
                pixel[0] /= ss_grid_size * ss_grid_size;
                pixel[1] /= ss_grid_size * ss_grid_size;
                pixel[2] /= ss_grid_size * ss_grid_size;
                
                // set pixel value
                sides[i].ptr<Vec3f>(y)[x] = pixel;
            }
        }
        cout << "." << flush;
    }
    cout << " done!" << endl;
    
    // concatenate images horizontally
    cube = Mat(Size(6*cubeSize, cubeSize), CV_32FC3);
    for (int i=0; i<6; i++) {
        Rect region (i*cubeSize, 0, cubeSize, cubeSize);
        sides[i].copyTo (cube (region));
    }
    
    return cube;
}

/*
// cube map: get env-map image position from 3d coordinate
Point2i CubeMap::get_cube_position ( Matx31d&  pos)
{

    //
    // LUT for selecting the cubemap side via cartesian coordinates
    //                       X  Y  Z -X -Y -Z
    const int sideLUT[6] = { 2, 1, 4, 0, 3, 5 };  
    
    // check which cube side we have to sample
    int maxCoord = 0;                                           // start with x coord
    if (abs (pos (1)) > abs(pos(maxCoord))) maxCoord = 1;       // if abs value of  y coord is larger, choose that one
    if (abs (pos (2)) > abs(pos(maxCoord))) maxCoord = 2;       // if abs value of  z coord is larger, choose that one
    int side = (pos(maxCoord) > 0) ? sideLUT[maxCoord] : sideLUT[maxCoord+3];            // get side via LUT; if value is negative, use second half of sideLUT
    Point2d envMapPos;
                                   //     0    1    2    3    4    5
    double X  =  pos(0);             //  +------------------------------+
    double mX = (-pos(0));            //  |  L | Ba |  R |  F |  T |  Bo |
    double Y  =   pos(1);             //  | -X | +Y | +X | -Y | +Z | -Z  | fw-vec
    double mY = (-pos(1));            //  |  Y |  X | -Y | -X |  X |  X  | right-vec
    double Z  =   pos(2);             //  | -Z | -Z | -Z | -Z |  Y | -Y  | down-vec
    double mZ = (-pos(2));            //  +------------------------------+            
    
    switch (side) {
        case 0:         
            envMapPos.x = (  Y / mX + 1.0 ) / 2.0 * (cubeSize-1);      // x = ( right / forward + 1 ) / 2 * w
            envMapPos.y = ( mZ / mX + 1.0 ) / 2.0 * (cubeSize-1);      // y = ( down / forward + 1 ) / 2 * h
            break;
        case 1: 
            envMapPos.x = ( X / Y + 1.0 ) / 2.0 * (cubeSize-1);
            envMapPos.y = ( mZ / Y + 1.0 ) / 2.0 * (cubeSize-1);
            break;
        case 2: 
            envMapPos.x = ( mY / X + 1.0 ) / 2.0 * (cubeSize-1);
            envMapPos.y = ( mZ / X + 1.0 ) / 2.0 * (cubeSize-1);
            break;
        case 3: 
            envMapPos.x = ( mX  / mY + 1.0 ) / 2.0 * (cubeSize-1);
            envMapPos.y = ( mZ / mY + 1.0 ) / 2.0 * (cubeSize-1);
            break;
        case 4: 
            envMapPos.x = ( X / Z + 1.0 ) / 2.0 * (cubeSize-1);
            envMapPos.y = ( Y / Z + 1.0 ) / 2.0 * (cubeSize-1);
            break;
        case 5: 
            envMapPos.x = (  X / mZ + 1.0 ) / 2.0 * (cubeSize-1);
            envMapPos.y = ( mY / mZ + 1.0 ) / 2.0 * (cubeSize-1);
            break;
    }
    
    
    int px = (int)(envMapPos.x+0.5 + side*cubeSize);
    int py = (int)(envMapPos.y+0.5);
    return Point2i (px,py);
}
*/
/*
// get envmap normal for a specific envmap element determined by the 3d point position as angle
Matx31d CubeMap::get_cube_normal (Matx31d& pos) 
{

    // shortcuts         
     Vec3d  x = Vec3d(1,0,0);
     Vec3d mx = Vec3d(-1,0,0);
     Vec3d  y = Vec3d(0,1,0);
     Vec3d my = Vec3d(0,-1,0);
     Vec3d  z = Vec3d(0,0,1);
     Vec3d mz = Vec3d(0,0,-1);
    
    //     0    1    2    3    4    5
    //  +------------------------------+
    //  |  L | Ba |  R |  F |  T |  Bo |
    //  | -x | +y | +x | -y | +z | -z  | fw-vec
    //  |  y |  x | -y | -x |  x |  x  | right-vec
    //  | -z | -z | -z | -z |  y | -y  | down-vec
    //  +------------------------------+
    
    // array of forwardm right and down vector for each cube side
    Vec3d forward[6] = { mx, y, x, my, z, mz };
    
    const int sideLUT[6] = { 2, 1, 4, 0, 3, 5 };  
    
    // check which cube side we have to sample
    int maxCoord = 0;                                           // start with x coord
    if (abs (pos (1)) > abs(pos(maxCoord))) maxCoord = 1;       // if abs value of  y coord is larger, choose that one
    if (abs (pos (2)) > abs(pos(maxCoord))) maxCoord = 2;       // if abs value of  z coord is larger, choose that one
    int i = (pos(maxCoord) > 0) ? sideLUT[maxCoord] : sideLUT[maxCoord+3];            // get side via LUT; if value is negative, use second half of sideLUT                        
    return -forward[i];
}

*/

/**
   The HDR algorithm
*/
double CubeMap::calc_hdr_frames (vector<Mat>& frames, Matx31d& screenCenter, Matx31d& down, Matx31d& right, Size2i screenSizeNoBorder, Size2i borderSize, int numFrames, double scale=0.0, bool applyCosFactor=false, double hdrSequenceBlurSize=0)
{
    
    cout << "performing forward projection ... " << flush;
    sw_start();
    
    // 1) forward projection : get required radiance
    
    #ifdef USE_GPU
        screenRequiredGPU = gpu::GpuMat(Mat::zeros(screenRequired.size(), CV_32FC3));
        screenRequiredGPU = project_forward(screenRequiredGPU, envMapRemaining, screenCenter, down, right);
        sw_stop();
        cout << " took " << sw_elapsed_ms() << " ms" << endl;
        
        // apply border ramp alhpa values 
        gpu::multiply(screenRequiredGPU, borderRampMaskGPU, screenRequiredGPU);
        
        cout << "performing backward projection ... " << flush;
        sw_start();
        // 2) backward projection from screen onto cube map to get the used radiance
        envMapUsed = gpu::GpuMat(Mat::zeros(envMapUsed.size(), CV_32FC3));
        envMapUsed = project_backward(envMapUsed, borderRampMaskGPU, screenCenter, down, right);
        
        screenRequiredGPU.download(screenRequired);
        
        sw_stop();
        cout << " took " << sw_elapsed_ms() << " ms" << endl;
    
    #else
        screenRequired = Mat::zeros(screenRequired.size(), CV_32FC3);
        screenRequired = project_forward(screenRequired, envMapRemaining, screenCenter, down, right);
        sw_stop();
        cout << " took " << sw_elapsed_ms() << " ms" << endl;
        //imwrite ("tmp/required_before.exr",screenRequired);
        
        // apply border ramp alhpa values 
        //screenRequired = screenRequired.mul(borderRampMask);
        multiply(screenRequired, borderRampMask, screenRequired);
        
        cout << "performing backward projection ... " << flush;
        sw_start();
        // 2) backward projection from screen onto cube map to get the used radiance
        envMapUsed = Mat::zeros(envMapUsed.size(), CV_32FC3);
        envMapUsed = project_backward(envMapUsed, borderRampMask, screenCenter, down, right);
        
        sw_stop();
        cout << " took " << sw_elapsed_ms() << " ms" << endl;
        
    
    #endif

    cout << "calculating required display radiance ... " << flush;
    sw_start();
    
    // 3.1) apply per-pixel cos phi factor to correct for the light angle
    if (applyCosFactor) {
        Vec3f *ps; 
        Matx31d pos;
        Matx31d screenNormal = Mat(down).cross(right);  // TODO check direction
        for (int y=0; y<screenSizePixel.height; y++) {
            ps=screenRequired.ptr<Vec3f>(y);
            for (int x=0; x<screenSizePixel.width; x++) {
                // calculate center pixel position
                pos = screenCenter +  down * ((y+0.5) * delY - (screenSizeMm.height-delY)/2.0)
                                   + right * ((x+0.5) * delX - (screenSizeMm.width-delX)/2.0);  
                
                // pixel cos phi factor : angle between light ray from pixel to origin and the pixels surface normal
                double cosPhi = screenNormal.dot (-pos) / ( norm (screenNormal) * norm(-pos) );
                ps[x][0] = ps[x][0] / cosPhi;
                ps[x][1] = ps[x][1] / cosPhi;
                ps[x][2] = ps[x][2] / cosPhi;
            }
        }
    }
    
    
    // 3.2) calculate exposure multiplier, so that the required radiance completely fits inside the hdr sequence 
    //      and is thus displayed with the maxmimum possible dynamic range
    
    // maximum required radiance
    double vmin[4], vmax[4];
    min_max(screenRequired, vmin, vmax);
    double maxRequired = vmax[3];
    cout << "maxRequired =" << maxRequired << endl;
    
    bool useAutoScale = false;
    // automaticly chose the best scale factor
    if (scale <= 0) {
        // maximum required radiance
        double vmin[4], vmax[4];
        Mat  tmp =  screenRequired/maxScreenRadiance;
        min_max(tmp, vmin, vmax);
        scale =  numFrames / vmax[3];
        cout << " scale is " << scale << endl;
        //cout << "tmp vmin= " << vmin[3] <<  " vmax= " << vmax[3];
        //min_max(maxScreenRadiance, vmin, vmax);
        //cout << "maxScreenRadiance vmin= " << vmin[3] <<  " vmax= " << vmax[3];
        useAutoScale = true;
         
    }    
    
    screenRequired.convertTo (screenRequired, -1, scale, 0.0);
    
    
    sw_stop();
    cout << " took " << sw_elapsed_ms() << " ms" << endl;
    
    if (useAutoScale) {
        cout << " exposure multiplier is " << scale << endl;                         
    } else {
        cout << " exposure multiplier is 1.0" << endl;                         
    }

    sw_start();
    
   
    // apply response curve to pixels between min.. max; set everything else to 0 or 1
    Vec3f *reqPtr;
    Vec3f *maxPtr;
    Vec3f *minPtr;
    

    int idx=0;
    float val;
    for (int y=0; y<screenSizePixel.height; y++) {
        
        reqPtr = screenRequired.ptr<Vec3f>(y);
        maxPtr = maxScreenRadiance.ptr<Vec3f>(y);
        minPtr = minLight.ptr<Vec3f>(y);
        
        for (int x=0; x<screenSizePixel.width; x++) {
            for (int c=0; c<3; c++) {
                val = reqPtr[x][c];
                idx=0;
                while (val > 0 && idx < numFrames) {
                    if (val >= maxPtr[x][c]) {
                        frames[idx].ptr<Vec3f>(y)[x][c] = 1.0;
                        val -= maxPtr[x][c];
                    } else {                                                     
                        frames[idx].ptr<Vec3f>(y)[x][c] = apply_response_svr_subpixel( val + minPtr[x][c], svr, x, y, c );
                        val=0;
                    }
                    idx++;
                }
            }
        }
    }

    sw_stop();
    cout << " took " << sw_elapsed_ms() << " ms" << endl;
    
    
    // upscaling and pasting
    // scale hdr frames to screen size and paste onto screenbuffer
    if (screenSizePixel.width < screenSizePixel.width || screenSizePixel.height < screenSizePixel.height) {
        cout << " scaling up .." << endl;
        
        sw_start();
        for (uint f=0; f<frames.size(); f++) {
            Mat tmp (screenSizeNoBorder, CV_32FC3);
            resize(frames[f](Rect(0,0,screenSizePixel.width, screenSizePixel.height)), tmp, screenSizeNoBorder, 0,0, INTER_CUBIC);
            
            tmp.copyTo(frames[f](Rect(borderSize.width, borderSize.height, screenSizeNoBorder.width+2*borderSize.width, screenSizeNoBorder.height+2*borderSize.width)));
        } 
        sw_stop();
        cout <<  " upscaling took " <<  sw_elapsed_ms () << " ms" << endl; 
    }
    
    
    // blur mode: blur first;
    if (hdrSequenceBlurSize > 0) {
        int envMapBlurSize = (int)(hdrSequenceBlurSize * frames[0].size().height/2.0) * 2 + 1;
        for (uint f=0; f<frames.size(); f++) {
            GaussianBlur(frames[f], frames[f], Size2d(envMapBlurSize,envMapBlurSize),envMapBlurSize);
        }

    }
    
    
    

        
        
    if (useAutoScale) {
        return scale;
    } else {
        return 1.0;
    }

}
/*

// project the cube map onto the screen
// remembers the used cube map pixels in the classes' Vector<Point2i> envMapUsed
Mat& CubeMap::project_forward (Mat& screen, Mat& env, Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{

    // screen normal    // vvv matx misses some operations?!
    Matx31d screenNormal = Mat(down).cross(right); 
    
    Vec3f *ps;  // pointer to a row in screen 
    
    Matx31d pos, ycomp;     // position in cartesian coordinates; separate up-vector component for efficiency
    Point2i envMapPos;      // point on envmap

    
    // supersampling grid size (must be uneven)
    int ss_grid_size = supersamplingSize;
    int ss_side = (ss_grid_size-1)/2;           // we iterate from -ss_side  (inclusive) to +ss_side (inclusive)
    double ss_delta = (ss_grid_size==1?0.0:1.0 / (double)(ss_grid_size-1));  // distance between two supersampling points
           
   
    // iterate over display pixels
    for (int y=0; y<screenSizePixel.height; y++) {
        ps=screen.ptr<Vec3f>(y);
        for (int x=0; x<screenSizePixel.width; x++) {
            
            // calculate center pixel position
            ycomp = down * ((double)(y-screenSizePixel.height/2) + 0.5) * delY * 2;
            pos = screenCenter +  ycomp + right * ((double)(x-screenSizePixel.width/2) + 0.5) * delX * 2; 
            
            // pixel cos phi factor
            double cosPhi = screenNormal.dot (get_cube_normal (pos)) / (norm (screenNormal) * norm(get_cube_normal(pos) ));
            
            // required light output at this pixel
            Vec3d lReq(0,0,0); 
            
            // supersampling grid
            for (int sy=-ss_side; sy<=ss_side; sy++) {
                ycomp = down * ((double)(y-screenSizePixel.height/2) + 0.5 + (double)sy*ss_delta) * delY * 2;
                for (int sx=-ss_side; sx<=ss_side; sx++) {
                
                    // calculate pixel world coordinate
                    pos = screenCenter +  ycomp + right * ((double)(x-screenSizePixel.width/2) + 0.5 +  (double)sx*ss_delta) * delX * 2; 
                    
                    // pixel on cube
                    envMapPos = get_cube_position(pos);
                    
                    // increase required radiance at display pixel
                    lReq += env.ptr<Vec3f>(envMapPos.y)[envMapPos.x];
                    
                }
            } // end supersampling loop
           
            // average supersampled pixel
            lReq(0) /= (double)(ss_grid_size*ss_grid_size) / cosPhi;        // forward projection: divide by cos phi factor
            lReq(1) /= (double)(ss_grid_size*ss_grid_size) / cosPhi;
            lReq(2) /= (double)(ss_grid_size*ss_grid_size) / cosPhi;
            
            ps[x] = lReq;
            
        }
    }

    return screen;

}*/

/**
   Calculates a forward perspective transform : from one cube side onto the screen plane.
   The code calculates the position of the screen corners and the intersection points on the specified cube-map plane. 
   The perspective transformation matrix is calculated with OpenCVs getPerspectiveTransform(), and returned.
   @param cubeSide Index of the cube map side
   @param screenCenter Screen center in world coordinates
   @param down Screen plane vector (vertical)
   @param right Screen plane vector (horizontal)
   @return Perspective Transformation matrix 
*/
Mat CubeMap::get_perspective_transform (int cubeSide, Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{
    
    // get the four screen corners
    //            0,0 1,0 0,1 1,1
    double x[4] = {-1,  1,-1, 1};
    double y[4] = {-1, -1, 1, 1};
    
    // in screen coordinates
    vector<Point2f> screenPoints(4);
    screenPoints[0] = Point2f(0.5f,0.5f);
    screenPoints[1] = Point2f(screenSizePixel.width-0.5f,0.5f);
    screenPoints[2] = Point2f(0.5,screenSizePixel.height-0.5f);
    screenPoints[3] = Point2f(screenSizePixel.width-0.5f,screenSizePixel.height-0.5f);
    
    // position of the center of the corner pixels in space: 
    Matx31d corners[4];
    for (int i=0; i<4; i++) {
        corners[i] = screenCenter +  down * y[i] * ( (screenSizeMm.height-delY)/2.0 ) 
                                  + right * x[i] * ( (screenSizeMm.width-delX)/2.0) ;

    }


    vector<Point2f> cubeMapPoints(4);
    
    // get projected screen corners for this cube side (seen as infinite plane)
    for (int i=0; i<4; i++) {
        cubeMapPoints[i] = Point2f();
        double X = corners[i](0);
        double mX = -X;
        double Y = corners[i](1);
        double mY = -Y;
        double Z = corners[i](2);
        double mZ = -Z;
        
        { 
            //const float cubeSize = 251;
            const float a = -0.5;
            
        switch (cubeSide) {
            case 0: //-X
                cubeMapPoints[i].x = (  Y / mX  + 1.0 ) / 2.0 * (double)(cubeSize) + a;      // x = ( right / forward + 1 ) / 2 * w
                cubeMapPoints[i].y = ( mZ / mX  + 1.0 ) / 2.0 * (double)(cubeSize)+ a;      // y = ( down / forward + 1 ) / 2 * h
                break;
            case 1: //+Y
                cubeMapPoints[i].x = (  X / Y + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                cubeMapPoints[i].y = ( mZ / Y + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                break;
            case 2: //+X
                cubeMapPoints[i].x = ( mY / X + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                cubeMapPoints[i].y = ( mZ / X + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                break;
            case 3: //-Y
                cubeMapPoints[i].x = ( mX / mY + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                cubeMapPoints[i].y = ( mZ / mY + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                break;
            case 4: //+Z
                cubeMapPoints[i].x = (  X / Z + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                cubeMapPoints[i].y = (  Y / Z + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                break;
            case 5: //-Z
                cubeMapPoints[i].x = (  X / mZ + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                cubeMapPoints[i].y = ( mY / mZ + 1.0 ) / 2.0 * (double)(cubeSize)+ a;
                break;
        }
        }
    }
    
    Mat pmat = getPerspectiveTransform (cubeMapPoints, screenPoints);
 /*   
    Matx31d coord (0,0,1.0);
    Matx31d res = Matx33d(pmat) * coord;
    cout << "0: " << res(0)/res(2) << " " << res(1) / res(2) << endl;
    
    coord = Matx31d (cubeSize-1,0,1.0);
    res = Matx33d(pmat) * coord;
    cout << "1: " << res(0)/res(2) << " " << res(1) / res(2) << endl;
    
    coord = Matx31d(0,cubeSize-1,1.0);
    res = Matx33d(pmat) * coord;
    cout << "2: " << res(0)/res(2) << " " << res(1) / res(2) << endl;
    
    
    coord = Matx31d (cubeSize-1,cubeSize-1,1.0);
    res = Matx33d(pmat) * coord;
    cout << "3: " << res(0)/res(2) << " " << res(1) / res(2) << endl;*/
return pmat;
}

/**
   Checks which cube-map sides are facing the screen and returns them in a vector.
*/
vector<int> CubeMap::get_sides_to_project(Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{
    
    
    // LUT for selecting the cubemap side via cartesian coordinates
    //                       X  Y  Z -X -Y -Z
    const int sideLUT[6] = { 2, 1, 4, 0, 3, 5 };  
    
    bool visible[6] = { false, false, false, false, false, false };
    
    // iterate over screen edge pixels, calculate the vector from origin to the point on 
    // the real screen, lookup the corresponding cube side and mark it as visible in the bool array
    
    // two horizontal edges
    for (int y=0; y<screenSizePixel.height; y+=screenSizePixel.height-1) {  // two iterations: 0 and height-1
        for (int x=0; x<screenSizePixel.width; x++) {
            
            Matx31d pos = screenCenter +  down * ((y+0.5) * delY - (screenSizeMm.height-delY)/2.0)
                                       + right * ((x+0.5) * delX - (screenSizeMm.width-delX)/2.0);  
            int maxCoord = 0;                                           // start with x coord
            if (abs (pos (1)) > abs(pos(maxCoord))) maxCoord = 1;       // if abs value of  y coord is larger, choose that one
            if (abs (pos (2)) > abs(pos(maxCoord))) maxCoord = 2;       // if abs value of  z coord is larger, choose that one
            int s = (pos(maxCoord) > 0) ? sideLUT[maxCoord] : sideLUT[maxCoord+3];            // get side via LUT; if value is negative, use second half of sideLUT
            visible[s] = true;                            
        }
    }
    
    // two vertical edges
    for (int x=0; x<screenSizePixel.width; x+=screenSizePixel.width-1) { // two iterations: 0 and width-1
        for (int y=0; y<screenSizePixel.height; y++) {
            
            Matx31d pos = screenCenter +  down * ((y+0.5) * delY - (screenSizeMm.height-delY)/2.0)
                                       + right * ((x+0.5) * delX - (screenSizeMm.width-delX)/2.0);  
            int maxCoord = 0;                                           // start with x coord
            if (abs (pos (1)) > abs(pos(maxCoord))) maxCoord = 1;       // if abs value of  y coord is larger, choose that one
            if (abs (pos (2)) > abs(pos(maxCoord))) maxCoord = 2;       // if abs value of  z coord is larger, choose that one
            int s = (pos(maxCoord) > 0) ? sideLUT[maxCoord] : sideLUT[maxCoord+3];            // get side via LUT; if value is negative, use second half of sideLUT
            visible[s] = true;
        }
    }
    
    

    vector<int> visibleList;
    for (int i=0; i<6; i++) { 
        if (visible[i]) {
            visibleList.push_back(i);
        }
    }
    
    return visibleList;
}

/**
   Checks if a cube side has to be projected (= is visible on the screen)
*/
bool CubeMap::is_side_visible (int side, Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{
    
    // LUT for selecting the cubemap side via cartesian coordinates
    //                       X  Y  Z -X -Y -Z
    const int sideLUT[6] = { 2, 1, 4, 0, 3, 5 };  
    
    const double x[4] = {-1,  1,-1, 1};
    const double y[4] = {-1, -1, 1, 1};
    
    for (int i=0; i<4; i++) {
        Matx31d pos = screenCenter +  down * y[i] * ( (screenSizeMm.height-delY)/2.0) 
                                   + right * x[i] * ( (screenSizeMm.width -delX)/2.0) ;
        int maxCoord = 0;                                           // start with x coord
        if (abs (pos (1)) > abs(pos(maxCoord))) maxCoord = 1;       // if abs value of  y coord is larger, choose that one
        if (abs (pos (2)) > abs(pos(maxCoord))) maxCoord = 2;       // if abs value of  z coord is larger, choose that one
        int s = (pos(maxCoord) > 0) ? sideLUT[maxCoord] : sideLUT[maxCoord+3];            // get side via LUT; if value is negative, use second half of sideLUT
                                    
        if (s == side) return true;
    }
    return false;
    
}

/**  
  Perform the forward projection : project from cubemap onto screen using multiple perspective transformations
*/
MAT& CubeMap::project_forward (MAT& screen, MAT& env, Matx31d& screenCenter, Matx31d& down, Matx31d& right, vector<int> sides)
{

    // get visible cube sides that have to be projected
    if (sides.size() == 0) { 
        sides = get_sides_to_project(screenCenter, down, right);
    }
    
    for (int s : sides) {
    
        // do projection
        Mat pmat = get_perspective_transform (s, screenCenter, down, right);
        Rect region (s*cubeSize, 0, cubeSize, cubeSize);
       // cout << region << endl;
        #ifdef USE_GPU 
           env(region).copyTo(cubeSideBufferGPU);
           gpu::warpPerspective(cubeSideBufferGPU, screenBufferGPU, pmat, screenSizePixel, INTER_LINEAR);
           gpu::add(screenBufferGPU, screen, screen);
        #else
            Mat tmp (screenSizePixel, screen.type());
            //stringstream ss; ss << "tmp/env" << s << ".exr";
            //imwrite(ss.str(), env(region));
            warpPerspective(env(region), tmp, pmat, screenSizePixel, INTER_LINEAR );
            screen += tmp;    
        #endif
    }
        
    return screen;

}


/**
  Perform the backward projection project from screen onto a cubemap using multiple perspective transformations
*/
MAT& CubeMap::project_backward (MAT& env, MAT& screen, Matx31d& screenCenter, Matx31d& down, Matx31d& right, vector<int> sides)
{

    sides = get_sides_to_project(screenCenter, down, right);
   
    for (int s : sides) {
    
        Mat pmat = get_perspective_transform(s,screenCenter, down, right);
        //if (pmat.data == NULL) continue;
        Rect region (s*cubeSize, 0, cubeSize, cubeSize);
        // warp onto envmap
        #ifdef USE_GPU 
            gpu::warpPerspective(screen, cubeSideBufferGPU, pmat, Size(cubeSize, cubeSize), INTER_LINEAR);
            gpu::copyMakeBorder(cubeSideBufferGPU, env, 0, 0, s*cubeSize, (5-s)*cubeSize, BORDER_CONSTANT, CV_RGB(0,0,0));
        #else
            Mat tmp(Size(cubeSize, cubeSize), CV_32FC3);
            warpPerspective(screen, tmp, pmat, Size(cubeSize, cubeSize), INTER_LINEAR | WARP_INVERSE_MAP);
            tmp.copyTo(env(region));
        #endif
        
    }
    return env;

}

    
/**
  Just show the environment map on the screen using forward projections.
 note: envmap dynamic range has to fit into display range!
*/
Mat& CubeMap::show_environment (Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{
    #ifdef USE_GPU
        screenUsedGPU = gpu::GpuMat(Mat::zeros(screenUsed.size(), CV_32FC3));
        screenUsedGPU = project_forward(screenUsedGPU, envMapOriginal, screenCenter, down, right);
        gpu::multiply(screenUsedGPU, borderRampMaskGPU, screenUsedGPU);
        screenUsedGPU.download (screenUsed);
        return screenUsed;
    #else    
        screenUsed = Mat::zeros(screenUsed.size(), CV_32FC3);
        screenUsed = project_forward(screenUsed, envMapOriginal, screenCenter, down, right);
        screenUsed = screenUsed.mul(borderRampMask);
        return screenUsed;
    #endif
}


/**
   Check if all pixel values are smaller/equal than an epsilon
*/
bool CubeMap::is_below_epsilon (MAT& img, double epsilon)
{
    Vec3f *pe; // pointer to one row
    for (int r=0; r<img.rows; r++ ) {
        pe =img.ptr<Vec3f>(r);
        for (int c=0; c<img.cols; r++ ) {
            for (int chan = 0; chan <3; chan++) {
                if (pe[c][chan] > epsilon) return false;
            }
        }
    }
    return true;
}

    
    
/**
  Check if illumination is completed.
*/
bool CubeMap::is_complete () {
    return is_below_epsilon(envMapRemaining, 1e-5);
}

//
// position-related tools
//


/**
   Calculates the maximum angle in degree (against screen surface normal) of the light rays
*/
double CubeMap::get_max_angle(Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{
    Matx31d screenNormal = Mat(down).cross(right); 
    
    // simple estimation: angle between screen normal and forward-vector (light ray to origin)
    double angleRad =  acos( screenNormal.dot(-screenCenter) / (norm(screenCenter) * norm(-screenNormal)));
    return angleRad / M_PI * 180.0;
}

