/**
   lightstage: util.cpp
 
   Contains all utility functions and datastructures used in the lighstage program.
   
   @author Manuel Jerger <nom@nomnom.de>
*/

#include "util.h"

using namespace std;
using namespace cv;


//
// timing stuf
//

timespec tstart, tend;

//
// coordinate systems
//



Matx31d spher2cart (Matx31d in)
{
    Matx31d c;
    // x = r * sin(t) * cos(p)
    c(0) = in(0) * sin(in(2)) * cos(in(1));
    // y = r * sin(t) * sin(p)
    c(1) = in(0) * sin(in(2)) * sin(in(1));
    // z = r * cos(t)
    c(2) = in(0) * cos(in(2));
    return c;
}

Matx31d cart2spher (Matx31d in)
{
    Matx31d s;
    // r = sqrt(x*x + y*y + z*z)
    s(0) = sqrt(in(0)*in(0) + in(1)*in(1) + in(2)*in(2));
    // phi = arccos(z/r)
    s(1) = acos(in(2)/s(0));
    // theta = arctan(y/x)
    s(2) = atan2(in(1),in(0));
    return s;
}


//
// general image / vector stuff
//



/** 
   Convert 3d world coordinates into screen position.
*/
Point  space2screen (Matx33d camMat, Matx44d transMat, Matx31d space)
{
    Matx41d sp;
    sp(0) = space(0);
    sp(1) = space(1);
    sp(2) = space(2);
    sp(3) = 1;
    Matx44d camMat44 = Matx44d::eye();
    for (int j=0; j<3; j++) {
        for (int i=0; i<3; i++ ) {
            camMat44(j,i) = camMat(j,i);
        }
    }
    Matx41d pt = camMat44 * (transMat * sp);
    return Point (pt(0)/pt(2), pt(1)/pt(2));
}



/** 
   clamp pixel values
*/
Mat& clamp (Mat& A, float lower, float upper)
{    
    for (uint t=0; t<A.total(); t++)
        for (uint c=0; c<3; c++ ) {
            if (A.at<Vec3f>(t)[c] < lower) A.at<Vec3f>(t)[c] = lower;
            if (A.at<Vec3f>(t)[c] > upper) A.at<Vec3f>(t)[c] = upper;
        }
    return A;
}


/**
  clamp pixel values (lower range only)
*/
Mat& clamp (Mat& A, float lower)
{
    for (uint t=0; t<A.total(); t++) {
        for (uint c=0; c<3; c++) {
            if (A.at<Vec3f>(t)[c] < lower) A.at<Vec3f>(t)[c] = lower;
        }
    }
    return A;
}

/**
  clamp image between two other images (element-wise clamp)
*/
Mat clamp(Mat& in, Mat& low, Mat& high) 
{
    Mat res (in.size(),CV_32FC3);

    for (uint16_t i=0; i<in.total(); i++) {
        for (uint8_t c=0; c<3; c++) {
            if ( in.at<Vec3f>(i)[c]  < low.at<Vec3f>(i)[c] ) res.at<Vec3f>(i)[c] = low.at<Vec3f>(i)[c];
            else if ( in.at<Vec3f>(i)[c]  > high.at<Vec3f>(i)[c] ) res.at<Vec3f>(i)[c] = high.at<Vec3f>(i)[c];
            else res.at<Vec3f>(i)[c] = in.at<Vec3f>(i)[c];
        }
    }    
    return res;
}


// minmax for images with three channels (bgr order)
// 4-value array: r g b min(r,g,b)
void  min_max (Mat img, double (&minVals)[4], double (&maxVals)[4])
{
    // split into channels, find min,max on each channel
    vector<Mat> channels;
    split(img, channels);
    minVals[3] = 1e10;
    maxVals[3] = 0.0;
    for (int i=0; i<3; i++) {
        minMaxLoc(channels[i], &(minVals[i]), &(maxVals[i]), 0, 0);
        minVals[3] = (minVals[i]<minVals[3])?minVals[i]:minVals[3];
        maxVals[3] = (maxVals[i]>maxVals[3])?maxVals[i]:maxVals[3];
    }
    return;
}


//
// interpolation
//
    
/**
 bilinear interpolation    samplept, upper left, bot. right, vals{ ul,  ur,  bl,  br }
*/
//  bilinear interpolation    samplept, upper left, bot. right, vals{ ul,  ur,  bl,  br }
//                    wiki:         P     (x1,y2)     (x2,y1)       Q12, Q22, Q11, Q21                   
Vec3f interpolate_bilinear (Point2f p, Point2f ul, Point2f br, vector<Vec3f> vals)
{
    Vec3f resx = (br.x-p.x)/(br.x-ul.x) * vals[2] + (p.x-ul.x)/(br.x-ul.x) * vals[3];
    Vec3f resy = (br.x-p.x)/(br.x-ul.x) * vals[0] + (p.x-ul.x)/(br.x-ul.x) * vals[1];
    Vec3f res = (ul.y-p.y)/(ul.y-br.y) * resx + (p.y-br.y)/(ul.y-br.y) * resy;
    return res;
}


/**
   bilinear interpolation  (float coordinates)
*/
Vec3f interpolate_bilinear (float x, float y, float x1, float y1, float x2, float y2, vector<Vec3f>& vals)
{
    Vec3f resx = (x2-x)/(x2-x1) * vals[2] + (x-x1)/(x2-x1) * vals[3];
    Vec3f resy = (x2-x)/(x2-x1) * vals[0] + (x-x1)/(x2-x1) * vals[1];
    Vec3f res = (y1-y)/(y1-y2) * resx + (y-y2)/(y1-y2) * resy;
    return res;
}


/**
 bilinear interpolation  (float coordinates and single channel)
*/
float interpolate_bilinear (float x, float y, float x1, float y1, float x2, float y2, float* vals)
{
    float resx = (x2-x)/(x2-x1) * vals[2] + (x-x1)/(x2-x1) * vals[3];
    float resy = (x2-x)/(x2-x1) * vals[0] + (x-x1)/(x2-x1) * vals[1];
    return (y1-y)/(y1-y2) * resx + (y-y2)/(y1-y2) * resy;
}


/**
 linear interpolate value for point p, where p1 and p2 ar the neighboring value (p1<p<p2)
*/
Vec3f interpolate_linear (float p, float p1, float p2, Vec3f val1, Vec3f val2)
{
   return val1 + (val2-val1) * (p-p1)/(p2-p1);
}

/**
 linear interpolate value, for single float value
*/
float interpolate_linear (float p, float p1, float p2, float val1, float val2)
{
   return val1 + (val2-val1) * (p-p1)/(p2-p1);
}


//
// (response) curve stuff
//

/**
 linear scale value val, so that minVal is mapped to 0.0 and maxVal is mapped to 1.0
*/
float fit_in_range (float val, float minVal, float maxVal)
{
    return 1.0/(maxVal-minVal) * val + minVal/(minVal-maxVal);
}


/**
 rescales every elemen to that the value minVal is mapped to (0,0,0) and maxVal to (1,1,1)
*/
vector<Vec3f>& fit_in_range ( vector<Vec3f>& curve, Vec3f minVal, Vec3f maxVal)
{
    for (uint8_t c=0; c<3; c++)
        for (uint16_t i=0; i<curve.size(); i++)
            curve[i][c] = fit_in_range (curve[i][c], minVal[c], maxVal[c]);
            
    return curve;
}


/**
  apply svr response to single pixel value using the patch number idx.
*/
Vec3f lookup_response (Vec3f val, SVRInfo& svr, int idx)
{
    Vec3f res;
    res[0] = svr.response[idx][(int)(fit_in_range(val[0],svr.vMin[idx][0],svr.vMax[idx][0]) * (svr.response[idx].size()-1) + 0.5)][0];
    res[1] = svr.response[idx][(int)(fit_in_range(val[1],svr.vMin[idx][1],svr.vMax[idx][1]) * (svr.response[idx].size()-1) + 0.5)][1];
    res[2] = svr.response[idx][(int)(fit_in_range(val[2],svr.vMin[idx][2],svr.vMax[idx][2]) * (svr.response[idx].size()-1) + 0.5)][2];
    return res;
}




/**
  apply svr response to single subpixel value using the patch number idx.
*/
float lookup_response_subpixel (float& val, SVRInfo& svr, int idx, int channel)
{
    if (val < svr.vMin[idx][channel]) return 0; // minimum light output reached
    if (val > svr.vMax[idx][channel]) return 1; // maximum light output reached
    
    float res = svr.response[idx][(int)(fit_in_range(val,svr.vMin[idx][channel],svr.vMax[idx][channel]) * (svr.response[idx].size()-1) + 0.5)][channel];
   
    return res;
}




/**
  apply svr response to all pixels of an image 
*/
void apply_response_svr( Mat& img, SVRInfo& svr ) 
{

    Vec3f *ps;  // pointer to a data rows
    for (int y=0; y<img.size().height; y++) {
    
        ps=img.ptr<Vec3f>(y);
        for (int x=0; x<img.size().width; x++) {
             ps[x][0] = apply_response_svr_subpixel( ps[x][0], svr, (double)x, (double)y, 0);
             ps[x][1] = apply_response_svr_subpixel( ps[x][1], svr, (double)x, (double)y, 1);
             ps[x][2] = apply_response_svr_subpixel( ps[x][2], svr, (double)x, (double)y, 2);

        }
    }

}


/**
  apply svr response to all pixels of an image (main bilineare interpolation code)
*/
float apply_response_svr_subpixel ( float val, SVRInfo& svr, double posX, double posY, int channel ) 
{
    
    // adapted variable names
    double ix = posX;// - svr.borderSize.width;
    double iy = posY;// - svr.borderSize.height;
    double w = svr.screenSize.width - 2*svr.borderSize.width;
    double h = svr.screenSize.height - 2*svr.borderSize.height;
    int c  = channel;
    
    // number of patches in each direction
    int numx = svr.patchLayout.width;
    //int numy = svr.patchLayout.height;
    
    // calculate the patch index (where the pixel lies within)
    int x = floor ( (double)ix / (double)svr.patchSize) ;    
    int y = floor ( (double)iy / (double)svr.patchSize);    
    
    //assert (x >= 0 && x < numx);
    //assert (y >= 0 && y < numy);
    // pixel position within patch
    double px = fmod(ix, svr.patchSize);
    double py = fmod (iy, svr.patchSize);
    //assert (px < svr.patchSize && px >= 0);
    //assert (py < svr.patchSize && py >= 0);
    
    // result
    double res;
    
    // check if and how we have to interpolate (4 cases)
    // 0) no interpolation (corners)
    if ( !(  ix >= svr.patchSize/2.0 && ix <= w-1 - svr.patchSize/2 ) && !(iy >= svr.patchSize/2 && iy <= h-1 - svr.patchSize/2 ) ) {                    
        res = lookup_response_subpixel(val, svr, y*numx + x, c);
    
    // 1) only vertical interpolation (vertical edges)
    } else if ( (ix < svr.patchSize/2 || ix > w-1 - svr.patchSize/2) && iy >= svr.patchSize/2 && iy <= h-1 - svr.patchSize/2 ) {
        int a = (py < svr.patchSize / 2) ? y-1 : y;  // above neighbor y position
        float val0 = lookup_response_subpixel(val, svr, a*numx + x, c );       // above
        float val1 = lookup_response_subpixel(val, svr, (a+1)*numx + x, c);   // below
        res = interpolate_linear(iy,    (double)a*svr.patchSize + svr.patchSize/2.0, 
                                    ((double)a+1.0)*svr.patchSize + svr.patchSize/2.0, val0, val1);
    
    // 2) only horizontal interpolation required (horizontal edges)
    } else if ( (iy < svr.patchSize/2 || iy > h-1 - svr.patchSize/2) && ix >= svr.patchSize/2 && ix <= w-1 - svr.patchSize/2 ) {
        int l = (px < svr.patchSize / 2) ? x-1 : x;  // left neighbor x position
        float val0 = lookup_response_subpixel(val, svr, y*numx + l, c);     // left
        float val1 = lookup_response_subpixel(val, svr, y*numx + l+1, c);   // right
        res = interpolate_linear(ix,    (double)l*svr.patchSize + svr.patchSize/2.0, 
                                    ((double)l+1.0)*svr.patchSize + svr.patchSize/2.0, val0, val1);
    // 3) bilinear interpolation (most of the pixels)
    } else  {
        // map over the four neightboring svr.response curves
        int l = (px < svr.patchSize / 2) ? x-1 : x;  // left neighbor x position
        int a = (py < svr.patchSize / 2) ? y-1 : y;  // above neighbor y position
        
        float vals[4];
        vals[0] = lookup_response_subpixel(val, svr, a*numx + l,c);      // top left
        vals[1] = lookup_response_subpixel(val, svr, a*numx + l+1, c);    // topright
        vals[2] = lookup_response_subpixel(val, svr, (a+1)*numx + l, c);  // bottom left
        vals[3] = lookup_response_subpixel(val, svr, (a+1)*numx + l+1, c);// bottom right
       
        res = interpolate_bilinear(ix, iy, (double)l*svr.patchSize + svr.patchSize/2.0, 
                                           (double)a*svr.patchSize + svr.patchSize/2.0, 
                                       ((double)l+1.0)*svr.patchSize + svr.patchSize/2.0, 
                                       ((double)a+1.0)*svr.patchSize + svr.patchSize/2.0, vals); 
    }
        
    return res;
}



/**
  Calculate interpolated image from the paches min/max values
*/
void get_min_max_screen (Mat& img, SVRInfo& svr, bool useMin)
{
    Vec3f *ps;  // pointer to a data rows
    for (int y=0; y<img.size().height; y++) {
    
        ps=img.ptr<Vec3f>(y);
        for (int x=0; x<img.size().width; x++) {
            ps[x][0] = get_min_max_subpixel(svr, Point(x,y), 0, useMin);
            ps[x][1] = get_min_max_subpixel(svr, Point(x,y), 1, useMin);
            ps[x][2] = get_min_max_subpixel(svr, Point(x,y), 2, useMin);
        }
    }
}

/**
  Calculate interpolated image from the paches min/max values (core interpolation code)
*/
float get_min_max_subpixel( SVRInfo& svr, Point2d pos, int channel, bool useMin ) 
{
    
    // adapted variable names
    double ix = pos.x;// - svr.borderSize.width;
    double iy = pos.y;// - svr.borderSize.height;
    double w = svr.screenSize.width - 2*svr.borderSize.width;
    double h = svr.screenSize.height - 2*svr.borderSize.height;
    int c  = channel;
    
    // number of patches in each direction
    int numx = svr.patchLayout.width;
    //int numy = svr.patchLayout.height;
    
    // calculate the patch index (where the pixel lies within)
    int x = floor ( (double)ix / (double)svr.patchSize) ;    
    int y = floor ( (double)iy / (double)svr.patchSize);    
    
    //assert (x >= 0 && x < numx);
    //assert (y >= 0 && y < numy);
    // pixel position within patch
    double px = fmod(ix, svr.patchSize);
    double py = fmod (iy, svr.patchSize);
    //assert (px < svr.patchSize && px >= 0);
    //assert (py < svr.patchSize && py >= 0);
    
    // result
    double res;
    
    // check if and how we have to interpolate (4 cases)
    // 0) no interpolation (corners)
    if ( !(  ix >= svr.patchSize/2.0 && ix <= w-1 - svr.patchSize/2 ) && !(iy >= svr.patchSize/2 && iy <= h-1 - svr.patchSize/2 ) ) {                    
        res = (useMin ? svr.vMin : svr.vMax)[y*numx + x][c];
    
    // 1) only vertical interpolation (vertical edges)
    } else if ( (ix < svr.patchSize/2 || ix > w-1 - svr.patchSize/2) && iy >= svr.patchSize/2 && iy <= h-1 - svr.patchSize/2 ) {
        int a = (py < svr.patchSize / 2) ? y-1 : y;  // above neighbor y position
        float val0 = (useMin ? svr.vMin : svr.vMax)[a*numx + x][c];       // above
        float val1 = (useMin ? svr.vMin : svr.vMax)[(a+1)*numx + x][c];   // below
        res = interpolate_linear(iy, (double)a*svr.patchSize + svr.patchSize/2.0, 
                                    ((double)a+1.0)*svr.patchSize + svr.patchSize/2.0, val0, val1);
    
    // 2) only horizontal interpolation required (horizontal edges)
    } else if ( (iy < svr.patchSize/2 || iy > h-1 - svr.patchSize/2) && ix >= svr.patchSize/2 && ix <= w-1 - svr.patchSize/2 ) {
        int l = (px < svr.patchSize / 2) ? x-1 : x;  // left neighbor x position
        float val0 = (useMin ? svr.vMin : svr.vMax)[y*numx + l][c];     // left
        float val1 = (useMin ? svr.vMin : svr.vMax)[y*numx + l+1][c];   // right
        res = interpolate_linear(ix, (double)l*svr.patchSize + svr.patchSize/2.0, 
                                    ((double)l+1.0)*svr.patchSize + svr.patchSize/2.0, val0, val1);
    // 3) bilinear interpolation (most of the pixels)
    } else  {
        // map over the four neightboring svr.response curves
        int l = (px < svr.patchSize / 2) ? x-1 : x;  // left neighbor x position
        int a = (py < svr.patchSize / 2) ? y-1 : y;  // above neighbor y position
        
        float vals[4];
        vals[0] = (useMin ? svr.vMin : svr.vMax)[a*numx + l][c];      // top left
        vals[1] = (useMin ? svr.vMin : svr.vMax)[a*numx + l+1][c];    // topright
        vals[2] = (useMin ? svr.vMin : svr.vMax)[(a+1)*numx + l][c];  // bottom left
        vals[3] = (useMin ? svr.vMin : svr.vMax)[(a+1)*numx + l+1][c];// bottom right
       
        res = interpolate_bilinear(ix, iy,  (double)l*svr.patchSize + svr.patchSize/2.0, 
                                            (double)a*svr.patchSize + svr.patchSize/2.0, 
                                           ((double)l+1.0)*svr.patchSize + svr.patchSize/2.0, 
                                           ((double)a+1.0)*svr.patchSize + svr.patchSize/2.0, vals); 
    }
        
    return res;
}


/**
   calculate dynamic range via min/max screen radiance
*/
double screen_dynamic_range ( Mat& minRadiance, Mat& maxRadiance)
{
    double range=10e10;
    
    // find maxmimum dynamic range that holds for all pixels
    
    Vec3f *pmin;  // pointer to a data rows
    Vec3f *pmax;  // pointer to a data rows
    for (int y=0; y<minRadiance.size().height; y++) {
    
        pmin=minRadiance.ptr<Vec3f>(y);
        pmax=maxRadiance.ptr<Vec3f>(y);
        for (int x=0; x<minRadiance.size().width; x++) {
            
            for (int c=0; c<3; c++) {
                double thisRange =  pmax[x](c) / pmin[x](c);
                if (thisRange < range) range = thisRange;
            }
        }
    }
    
    
    return range;
}

//
// remote camera control
//

#define useDownload true

string remoteCaptureCommand = "sh remote_canon.sh";

/** 
   remote camera control:  set camera parameters
*/
int remote_setup()
{
    cout << "initializing remote camera" << endl;
    stringstream cmd;
    cmd << remoteCaptureCommand << " setup";
    int ret = system(cmd.str().c_str());
    if (ret != 0) cout << "Error " << ret << " while executing \"" << cmd.str() << "\" on remote camera" << endl;
    return ret;
}

/** 
   remote camera control:  capture an image with specific exposure time, aperture and the filename (if image should be downloaded)
*/
int remote_capture(float exposure, float aperture, string filename)
{
    cout << " DSLR capture ss=" << exposure << "  ap=" << aperture << " to " << filename << (useDownload ? " download" : "") << endl;
    stringstream cmd;
    cmd << remoteCaptureCommand << " capture " << exposure << " " << aperture << " " << filename;
    int ret = system(cmd.str().c_str());
    if (ret != 0) cout << "Error " << ret << " while executing \"" << cmd.str() << "\" on remote camera" << endl;
    return ret;
}

/** 
   remote camera control: start capture in bulb mode; user can specify aperture and filename (if image should be downloaded)  (not used in final implementation)
*/
int remote_start_exposure (float aperture, string filename)
{
    cout << " DSLR capture bulb ap=" << aperture << " to " << filename << endl;
    stringstream cmd;
    cmd << remoteCaptureCommand << " bulb start " << " " << aperture << " " << filename;
    int ret = system(cmd.str().c_str());
    if (ret != 0) cout << "Error " << ret << " while executing \"" << cmd.str() << "\" on remote camera" << endl;
    return ret;
}
/** 
   remote camera control: stop bulb mode exposure (not used in final implementation)
*/
int remote_stop_exposure ()
{
    stringstream cmd;
    cout << " DSLR capture bulb stop" << endl;
    cmd << remoteCaptureCommand << " bulb stop";
    int ret = system(cmd.str().c_str());
    if (ret != 0) cout << "Error " << ret << " while executing \"" << cmd.str() << "\" on remote camera" << endl;
    return ret;
}


/**
  control the display backlight
*/
string backlightControlCommand = "sh set_backlight.sh";

int set_backlight (double value)
{
    stringstream cmd;
    cmd << backlightControlCommand << " " << (int)(100.0*value+0.5);
    int ret =  system(cmd.str().c_str());
    if (ret != 0) cout << "Error " << ret << " while executing \"" << cmd.str() << endl;
    return ret;
}


//
// sound notifications
//

string soundNotificationCommand =  "sh sound_notification.sh";

/**
  play back the specified sound notification
*/
int play_sound ( SOUNDS sound ) 
{
   
    stringstream cmd;
    cmd << soundNotificationCommand << " " << sound;
    int ret = system(cmd.str().c_str());
    return ret;
}
