/**
 evaluate_display : calculate display response curve
 
 @author Manuel Jerger <nom@nomnom.de>
*/


#include "evaluate_display.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "evaluate_display";


/**
 Prints usage.
*/
void help()
{
    cout << "Evaluates white/black screen image as well as grey ramp images and recovers the background illumination variation and the response curve respectively.\n" <<
        "Usage: \n" <<
        //" " << PROGNAME << " --bglight <in_whiteimage> " << endl <<
        //"     <in_whiteimage>      The recorded white screen image; OpenCV compatible format, e.g. exr." <<  endl << endl <<

        " " << PROGNAME << " --response <in_rampimage> <in_whiteimage> <in_blackimage> <avgsize> <bordersize> <numramps> <mode> <out_response>" << endl <<
        "     <in_rampimage>       The recorded ramp screen image: linear horizontal grey ramp from low (left) to high (right); OpenCV compatible format, e.g. exr." <<  endl <<
        "     <in_whiteimage>      The recorded white screen image; OpenCV compatible format, e.g. exr." <<  endl <<
        "     <in_blackimage>      The recorded black screen image; OpenCV compatible format, e.g. exr." <<  endl <<
        "     <avgsize>            Averaging size: number of lines that should be averaged, as a fraction (of image height)." <<  endl <<
        "     <bordersize>         Ramp bordersize in pixels" << endl << 
        "     <numramps>           Number of ramps shown in one image" << endl << 
        "     <mode>               Response curve mode: 1 = 8 bit, 2 = 8 bit inverted." <<  endl <<
        "     <out_response>       File for dumping the response curve (4 columns float, \"index R G B\")" <<  endl <<
        endl <<
        " " << PROGNAME << " --svr <w> <h> <size> <mode> <out_response> <cs_matrix> <in1> <in2> ... <inN> " << endl <<
        "     <w> <h>              Size of screen border in pixels. Can be used to achieve equal sized patches for averaging." << endl << 
        "     <size>               Size of square patches to average over; in pixels" <<  endl <<
        "     <mode>               Program mode" <<  endl <<
        "     <out_response>       File for dumping the response curve." <<  endl << 
        "     <cs_matrix>          Colorspace transformation matrices (created with --color_patches)." <<  endl << 
        "     <in1> <in2> .. <inN> Input Images: screen showing uniform grey of different values; assumes equal spaced values ranging from 0 .. 1" <<  endl <<
        endl <<
        " " << PROGNAME << " --average <in_response1> <in_response2> ... <in_responsen> <out_response>" << endl <<
        "     <in_response#>       Response curves (4-column) that should be averaged." <<  endl <<
        "     <out_response>       file to dump averaged response curve to." <<  endl <<
        endl <<
        " " << PROGNAME << " --color <in_redimage> <in_greenimage> <in_blueimage> <out_matrix>" << endl <<
        "     <in_redimage>      Recorded screen image showing only red pixels." <<  endl <<
        "     <in_greenimage>    Recorded screen image showing only red pixels." <<  endl <<
        "     <in_blueimage>     ecorded screen image showing only red pixels." <<  endl <<
        "     <out_matrix>       File to dump the color convertsion matrix to." <<  endl <<
        endl << 
        " " << PROGNAME << " --color_patches <w> <h> <size> <out_matrix> <in_redimage> <in_greenimage> <in_blueimage>  " << endl <<
        "     <w> <h>            Size of screen border in pixels. Can be used to achieve equal sized patches for averaging." << endl << 
        "     <size>             Size of square patches to average over; in pixels" <<  endl <<
        "     <out_matrix>       File to dump the color convertsion matrix to." <<  endl <<
        "     <in_redimage>      Recorded screen image showing only red pixels." <<  endl <<
        "     <in_greenimage>    Recorded screen image showing only red pixels." <<  endl <<
        "     <in_blueimage>     ecorded screen image showing only red pixels." <<  endl;
        
         
        
}

/**
  linear scale and shift the value val, so that minVal is mapped to 0.0 and maxVal is mapped to 1.0
  @param val Value to remap
  @param minVal Value that will be mapped to 0
  @param maxVal Value will be mapped to 1
  @return The remapped value
*/
inline double fit_in_range (double val, double minVal, double maxVal) {
    return 1.0/(maxVal-minVal) * val + minVal/(minVal-maxVal);
}


/**
 rescales every elements so the value minVal is mapped to (0,0,0) and maxVal to (1,1,1)
  @param curve Array of 3-valued vectors
  @param minVal Value that will be mapped to 0
  @param maxVal Value will be mapped to 1
  @return The remapped values
*/

vector<Vec3f>& fit_in_range ( vector<Vec3f>& curve, Vec3f minVal, Vec3f maxVal)
{
    for (int i=0; i<curve.size(); i++)
        for (int c=0; c<3; c++)
            curve[i][c] = fit_in_range (curve[i][c], minVal[c], maxVal[c]);
            
    return curve;
}



/**
  linear interpolate value for point p, where p1 and p2 ar the neighboring value (p1<p<p2)
*/
inline double interpolate_linear (double p, double p1, double p2, double val1, double val2)
{
   return val1 + (val2-val1) * (p-p1)/(p2-p1);
}


/**
 linear interpolate value for point p, where p1 and p2 ar the neighboring value (p1<p<p2)
*/
inline Vec3f interpolate_linear (double p, double p1, double p2, Vec3f val1, Vec3f val2)
{
   return val1 + (val2-val1) * (p-p1)/(p2-p1);
}

/** 
  Calculate inverted, linear interpolated curve. Supports only upsampling! inverted.size() >> response.size(); inverted must be initialized.
*/
vector<Vec3f> invert_response (vector<Vec3f>& response, vector<Vec3f>& inverted)
{

    // 1) foreach interval r1..r2 in response:
    //      find index i1 of inverted[] that is closest to response(b) -> use round(response)
    //      calculate inv. interpolated values from (a .. b]
    //      a=b; b++
    int size_in = response.size();
    int size_out = inverted.size();


    // note: we scale response to [0 .. 1] for sampling
    
    Vec3f minVal = response[0];
    Vec3f maxVal = response[size_in-1];
    //cout << minVal << endl;
    //cout << maxVal << endl;
    fit_in_range (response, minVal, maxVal);
    
    for (int c=0; c<3; c++) {
    
        int i1 = 0;
        
        for (int r=0; r<size_in-1; r++) {
            int r1 = r;
            int r2 = r+1;
            //if ( response[r2][c] < 0 ) { cout << r2 << " = " << response[r2][c]; }
            // index of inverted[] that is closest to response(b)
            int i2 = (int)(response[r2][c] * (float)(size_out-1)+0.5);
            assert (i2 >= 0 && i2 < size_out);
            // interpolate
            //cout << "interpolating: " << i1<< " " <<  i2<< " " <<  response[r1][c]<< " " <<  response[r2][c] << endl;
            for (int i=i1; i<i2; i++) {
                inverted[i][c] = interpolate_linear(i, i1, i2, r1 / (float)(size_in - 1), r2 / (float)(size_in - 1));
            }
            i1=i2;
        }
        
        
        // force first and last element
        inverted[0][c] = 0.0;
        inverted[size_out-1][c] = 1.0;
        
    }
    return inverted;
}



/**
 iterate in reverse, check for monotony and fix it response is not monotonous
*/
vector<Vec3f>& check_monotonicity (vector<Vec3f>& response)
{
    bool fixed;
    for (int i=response.size()-2;i>=0;i--) {
        fixed=false;
        for (int c=0; c<3; c++)
            if (response[i][c] > response[i+1][c]) { 
                fixed = true; 
                
                cout << response[i][c] << " > " << response[i+1][c] << endl;
                cout << response[i+1][c] - response[i][c] << endl;
                response[i][c] = response[i+1][c];
            }
        if (fixed) cout << "Warning: response curve is not monotonous (on index " << i << "). Fixing it by copying higher value " <<  response[i+1] << endl;
    }
    return response;
}


/**
 clamp pixel values
*/
Mat& clamp (Mat& A, float lower, float upper)
{
    for (int t=0; t<A.total(); t++)
        for (int c=0; c<A.channels(); c++ ) {
            if (A.at<Vec3f>(t)[c] < lower) A.at<Vec3f>(t)[c] = lower;
            if (A.at<Vec3f>(t)[c] > upper) A.at<Vec3f>(t)[c] = upper;
        }
    return A;
}

/**
  calculate response curve from ramp image. Assumes every pixel has the same response and the backlight is inhomogenous.
*/
int run_response(int argc, char *argv[])
{
    // load images
    Mat ramp = imread(argv[2], CV_LOAD_IMAGE_UNCHANGED);
    assert (ramp.type() == CV_32FC3);

    Mat wimg = Mat::ones(ramp.size(),CV_32FC3);
    if (strcmp(argv[3],"--") != 0) {
        wimg = imread(argv[3],  -1);
        assert (wimg.type() == CV_32FC3);
    }

    Mat bimg = Mat::zeros(ramp.size(),CV_32FC3);
    if (strcmp(argv[4],"--") != 0) {
        bimg = imread(argv[4],  -1);
        assert (bimg.type() == CV_32FC3);
    }


    // remove black offset
    ramp = ramp-bimg;
    ramp = clamp(ramp, 0, 1);
    wimg = wimg-bimg;
    wimg = clamp(wimg, 0, 1);
    
    // P = r^-1 ( (ramp - B) / H )
    ramp /= wimg;

    // lines to evaluate, as a fraction of image height
    float avgfrac = atof(argv[5]);
    
    int borderSize = atoi(argv[6]);
    
    int numRamps = atoi(argv[7]);
    bool inverted = false;
    if (numRamps < 0) { 
        numRamps *= -1;
        inverted = true;
    }

    // response output file
    string outfile = "";
    int mode=0;
    if (argc>=10) {
        mode=atoi(argv[8]);
        outfile=argv[9];
    }

    
    // 1) average lines
    
    int numRows = (int)(avgfrac*(float)ramp.rows);  // number of rows to average
    int width_b = ramp.cols / numRamps;                // width of one ramp including border
    int width = width_b-2*borderSize;                  // width of one ramp
    Mat avg = Mat (1, width_b-2*borderSize, CV_32FC3, CV_RGB(0,0,0));
   
    for (int i=0; i<numRamps; i++) {
        for (int r=0; r<numRows; r++) {
            for (int x=0; x<width; x++) {
                avg.at<Vec3f>(x) += ramp.at<Vec3f>(r+(int)(1.0-avgfrac)*(float)ramp.rows*0.5,i*width_b+x+borderSize);
            }
        }
    }
    float mult = 1.0 / ( (float)numRows * (float)numRamps );
    avg.convertTo(avg, -1, mult, 0.0);


    int response_size =  width;
    
    // calculate 8 bit response curve points
    vector<Vec3f> response;
    response.resize(response_size);

    // average between samples
    if (response_size < width) {
        vector<int> responseCounts;
        responseCounts.resize(response_size);
        //for (int i=0; i<response_size; i++) { responseCounts[i] = 0; }
        for (int x=0; x<width; x++) {
            int rx;
            if (inverted) {
                rx=round((float)(width-1-x)/(float)(width-1)*(float)(response_size-1));        // correct way of rounding!
            } else {
                rx=round((float)x/(float)(width-1)*(float)(response_size-1));        // correct way of rounding!
            }
            response[rx] += avg.at<Vec3f>(x); 
            responseCounts[rx]++;
        }
        
        for (int c=0; c<3; c++) {
            for (int i=0; i<response_size;i++) {
                response[i][c] /= (float)responseCounts[i];
            }
        }
        
    // use all samples
    } else {
        for (int x=0; x<width; x++) {
            if (inverted) {
                response[width-1-x] = avg.at<Vec3f>(x);
            } else {
                response[x] = avg.at<Vec3f>(x);
            }
         //   cout <<  response[x] << endl;
        }
    }
    

    response = check_monotonicity(response);
    
    //
    // dump results
    //

    // dump response curve r
    if (mode == 1) {
        if (!outfile.empty()) {
            cout << "dumping response curve to " << outfile << endl;
            ofstream of;
            of.open(outfile.c_str());
            for (int i=0;i<response_size;i++){
                of << i << " " << response[i][2] << " " << response[i][1] << " " << response[i][0] << endl; // R-G-B order
            }
            of.close();

        // print to stdout
        } else {
            cout << "response curve:" << endl;
            for (int i=0;i<response_size;i++) {
                cout << i << " " << response[i][2] << " " << response[i][1] << " " << response[i][0] << endl; // R-G-B order
            }
            cout << endl;
        }

    // dump 8 bit inverse response curve (r^-1), one file per channel
    } else if (mode == 2 && !outfile.empty()) {

        ofstream of;
        for (int c=0; c<3; c++) {
            stringstream filename; filename << outfile.c_str() << "." << c;
            of.open(filename.str().c_str());
            for (int i=0;i<response_size;i++) {
                of << response[i][c] << " " << i << endl;  // BGR order
            }
            of.close();
        }
    
    // dump inverse discrete response curve (r^-1), as yml array
    // Note: curve has to be monotonous!
    } else if ((mode == 3  || mode == 4 )&& !outfile.empty()) {
        int num = (int)pow(2,16); 
        
        vector<Vec3f> result(num);
        result = invert_response(response, result);
        
        // simple linear interpolation
        //result = interpolate_response(result);
        
        // dump 4-column textfile
        if (mode == 3) {
            cout << "dumping inverse response curve LUT to " << outfile << endl;
            
            ofstream of;
            of.open(outfile.c_str());
            for (int i=0;i<num;i++) {
                of << i << " " << result[i][2]  << " " << result[i][1]  << " " << result[i][0] << endl;  //RGB order
            }
            of.close();
            
        // dump vector in yml format
        } else if (mode == 4) {
            cout << "dumping inverse response curve LUT to " << outfile << endl;
            FileStorage fs(outfile.c_str(), FileStorage::WRITE);
            String type = "lut";
            fs << "type" << type;
            fs << "min" << response[0];
            fs << "max" << response[response.size()-1];
            fs << "lut" << result;
            
            fs.release();
        
        
        }
    }
    return 0;
}

/**
  Revocer a spatially varying response curve  (every pixel has a different response).
  Uses a sequence of grey images with values ranging from 0 to 1.
  The display will be divided in square patches. For every patch we calculate a response curve.
*/
int run_svr(int argc, char* argv[])
{
    // 0) parse command line args
    int bw = atoi(argv[2]);
    int bh = atoi(argv[3]);
    int patchSize = atoi(argv[4]);
    int mode = atoi(argv[5]);
    string outfile = argv[6];
   

    Matx33d csTrans;
    if ( strcmp(argv[7], "-") != 0) {
      // color space transform camera -> screen
      Mat tmp;   
      FileStorage fs(argv[7], FileStorage::READ);
      fs["colorTransform"] >> tmp;
      fs.release();
      csTrans = Matx33d(tmp);
    } else {
	csTrans = Matx33d::eye();
    }

  
    
    vector<Mat> imgs;
    
    cout << "loading " << argc-8 << " images  " << flush;
    for (int i=8; i<argc; i++) {
        imgs.push_back(imread(argv[i], -1));
        cout << "." << flush;
        if (imgs.size() >= 2) { 
            assert (imgs[imgs.size()-1].size().width == imgs[imgs.size()-2].size().width);
            assert (imgs[imgs.size()-1].size().height == imgs[imgs.size()-2].size().height);
        }
    }
    cout << " done." << endl;

    
    int numSamples = imgs.size();
    
    // 1) calculate patch values
    
    if ( ((imgs[0].size().width - 2*bw) % patchSize) != 0 ||
         ((imgs[0].size().height - 2*bh) % patchSize) != 0 )
    {
        cout << "Error: screen size (height and width) must be a multiple of the patch size!" << endl;
        return -1;
    }
    
    // number of patches
    int numx = (imgs[0].size().width - 2*bw) / patchSize;
    int numy = (imgs[0].size().height - 2*bh) / patchSize;
    vector<Mat> patches(imgs.size());   // averaged patch values
    
    cout << "averaging over " << numx << " x " << numy << " patches of size " << patchSize << endl;
    for (int i=0; i<imgs.size(); i++) {
        patches[i] = Mat::zeros (Size(numx, numy), CV_32FC3); 
        
        for (int y=0; y<numy; y++) {
            for (int x=0; x<numx; x++) {
                
                // iterate over patch pixels and calculate average
                for (int py=0; py < patchSize; py++) {
                    for (int px=0; px < patchSize; px++) {
                        patches[i].at<Vec3f>(y,x) += imgs[i].at<Vec3f>(bh + y*patchSize + py, bw + x*patchSize + px);
                    }
                }
                patches[i].at<Vec3f>(y,x) /= (float)(patchSize*patchSize);
                
                // convert from camera to screen color space
                patches[i].at<Vec3f>(y,x) = csTrans * Vec3d(patches[i].at<Vec3f>(y,x));
            }
        }
    }
    
    // convert sampled patchvalues into required response curve form
    vector<vector<Vec3f> > svr(numx*numy);
    for (int y=0; y<numy; y++) {
        for (int x=0; x<numx; x++) {
        
            vector<Vec3f> response(numSamples);
            for (int s=0; s<numSamples; s++) {
                response[s] = patches[s].at<Vec3f>(y,x);
            }
            
            check_monotonicity(response);
            svr[y*numx+x] = response;
        }
    }
   
    int num = pow(2,12);
    vector<Vec3f> inverted(num);
    
    // dump inverted response to yml file
    if (mode == 0) {
        cout << "dumping inverted response curve to " << outfile << endl;
        FileStorage fs(outfile.c_str(), FileStorage::WRITE);
        String type = "svr";
        fs << "type" << type;
        fs << "borderSize" << Size(bw, bh);
        fs << "screenSize" << imgs[0].size();
        fs << "patchLayout" << Size(numx,numy);
        fs << "patchSize" << patchSize;
        
        int numPatches = numx * numy;
        for (int n=0;n<numPatches;n++){

            Vec3f minVals = svr[n][0];
            Vec3f maxVals = svr[n][numSamples-1];
            
            // invert 
            inverted = invert_response(svr[n], inverted);

            stringstream ss;
            ss << "min_" << n;
            fs << ss.str().c_str() << minVals;
            
            ss.str(string()); // clearing a stringstream
            ss << "max_" << n;
            fs << ss.str().c_str() << maxVals;
            
            ss.str(string());
            ss << "response_" << n;
            fs << ss.str().c_str() << inverted;
            
        }
        fs.release();
    
    // dump the raw response curve for a single patch (index specified by cl argument <mode>)
    } else {
        // negative mode means invert curve
        if (mode < 0) {
            cout << "inverting response curve" << endl;
            mode *= -1;
            for (int n=0; n<numx * numy; n++) {
                inverted = invert_response(svr[n], inverted);
                svr[n] = inverted;
            }
        } 
        cout << "dumping response curve of patch number " << mode << " to " << outfile << endl;
        // magic number default to curve number 0
        if ( mode == 13371337 ) mode=0;
        ofstream of;
        of.open(outfile.c_str());
        for (int i=0;i<svr[mode].size();i++){
            of << i << " " << svr[mode][i][2] << " " << svr[mode][i][1] << " " << svr[mode][i][0] << endl; // R-G-B order
        }
        of.close();
    }

    return 0;
}

/**
 Calculate average of response curves (not used in final implementation).
*/
int run_average(int argc, char* argv[]) 
{

    // load response curves in matrix format
    string outfile;
    
    vector<vector<Vec3f> > invals;
    vector<Vec3f> outval;
    
    int narg=2;
    while(narg < argc-2-1-1) {
        FileStorage fs(argv[narg], FileStorage::READ);
        vector<Vec3f> in;
        fs["lut16"] >> in;
        invals.push_back(in);
        fs.release();
    }
    
    outval.resize(invals[0].size());
    for (int j=0; j<invals[0].size(); j++)  outval[j] = Vec3f(0,0,0);
    
    for (int i=0; i<invals.size(); i++) {
        if (i>1) assert(invals[i].size() == invals[i-1].size());
        for (int j=0; j<invals[i].size(); j++) {
            outval[j] += invals[i][j] / (float)invals.size();;
        }
    }
    
    // average values
        
    narg++;
    FileStorage fs (argv[narg], FileStorage::WRITE);
    fs << "lut16" << outval;
    fs.release();
    
    return 0;
}

/**
  Clamp all elements of A to 0 .. 1 
*/
Mat clamp (Mat& A)
{
    const double lower = 0.0;
    const double upper = 1.0;
    
    Mat res (A.size(), A.type());
    for (int i=0; i<A.total(); i++) {
        if (A.at<double>(i) < lower) res.at<double>(i) = lower;
        else if (A.at<double>(i) > upper ) res.at<double>(i) = upper;
        else res.at<double>(i) = A.at<double>(i);
    }
    return res;
}

const Matx<double, 3, 3> BGR2XYZ (0.1805, 0.3576, 0.4124,
                                0.0722, 0.7152, 0.2126,
                                0.9502, 0.1192, 0.0193);
   
/** 
   sRGB to xyY colorspace
   @param primary sRGB data in OpenCV's BGR order
   @return xyY
*/
Vec3d bgr2xyY (Vec3d primary) 
{
    Vec3d xyz = BGR2XYZ * primary;
    // convert to xyY and return
    double sum = xyz[0]+xyz[1]+xyz[2];
    return Vec3d(xyz[0]/sum, xyz[1]/sum, xyz[1]); // x y Y
}

/**
   Calculate color conversion matrix from red/green/blue image triplet (not used in final implementation)
*/
int run_color(int argc, char* argv[]) 
{
    Mat red   = imread(argv[2], CV_LOAD_IMAGE_UNCHANGED);
    Mat green = imread(argv[3], CV_LOAD_IMAGE_UNCHANGED);
    Mat blue  = imread(argv[4], CV_LOAD_IMAGE_UNCHANGED);
    string outfile = argv[5];
    
    if (red.data == NULL || blue.data == NULL ||  green.data == NULL) {
        cout << "Error loading input images" << endl;
        return -1;
    }
    
    // calculate average camera RGB intensities for each screen color
    Vec3d R(0,0,0), G(0,0,0), B(0,0,0);
    int i;
    for (i=0; i<red.total(); i++) R += red.at<Vec3f>(i);
    R /= i;
    for (i=0; i<green.total(); i++) G += green.at<Vec3f>(i);
    G /= i;
    for (i=0; i<blue.total(); i++) B += blue.at<Vec3f>(i);
    B /= i;
    
   /* R /= R[2];
    G /= G[1];
    B /= B[0];*/
    
    cout << endl;
    cout << "input primaries (screen primary -> camera ) BGR order" << endl;
    cout << " R: " << R << endl;
    cout << " G: " << G << endl;
    cout << " B: " << B << endl;
    
    
    // color transform matrix screen colors -> camera colors
    // Note: uses openCV B-G-R order
    Mat conv (3,3, CV_64F);
    for (int j=0; j<3; j++) {
       conv.at<double>(j,0) = B[j];
       conv.at<double>(j,1) = G[j];
       conv.at<double>(j,2) = R[j];
    }
    cout << endl;
    cout << "inverse color transform (screen -> camera), BGR order" << endl;
    cout << conv << endl;
    
    // calculate inverse transformation matrix
    Mat inv = conv.inv();
    
    // double maxVal = *std::max_element(inv.begin<double>(),inv.end<double>());
    
    // set negative values to 0
    //for (int i=0; i<inv.total(); i++) inv.at<double>(i) = (inv.at<double>(i) < 0) ? 0 : inv.at<double>(i);
    
    // scale so conv * (1,1,1) =<  (1,1,1)
    double maxSum = max ( inv.at<double>(0,0) + inv.at<double>(0,1) + inv.at<double>(0,2),
             max ( inv.at<double>(1,0) + inv.at<double>(1,1) + inv.at<double>(1,2),  inv.at<double>(2,0) + inv.at<double>(2,1) + inv.at<double>(2,2) ) );
    cout << "maxSum: " << maxSum << endl;
    //inv /= maxSum;
    
    cout << endl;
    cout << "forward color transform (camera -> screen), BGR order" << endl;
    cout << inv << endl << endl;
    
    //cout << Matx33d(inv) * Matx31d(0.2751, 0.2769, 0.3177) << endl;
    cout << Matx33d(inv) * Matx31d(0.1782,0.1816,0.2108) << endl;
    
    FileStorage fs(outfile.c_str(), FileStorage::WRITE);
    fs << "inverseColorTransform" << conv;
    fs << "colorTransform"<< inv;
    fs.release();
    
   /*cout << endl;
   
    Mat a = Mat::zeros(3,1, CV_64F);
    a.at<double>(0) = 0;
    a.at<double>(1) = 0;
    a.at<double>(2) = 1;
    a = inv * a;
    cout << endl << clamp(a) << endl;
    
    a.at<double>(0) = 0;
    a.at<double>(1) = 1;
    a.at<double>(2) = 0;
    a = inv * a;
    cout << endl << clamp(a) << endl;
    
    a.at<double>(0) = 1;
    a.at<double>(1) = 0;
    a.at<double>(2) = 0;
    a = inv * a;
    cout << endl << clamp(a) << endl;
    */
    
    // location of display primary colors in CIE 1931 color space
    cout << "location of screen primaries in CIE1931 color space (xyY):" << endl;
    cout << "R: " << bgr2xyY(R) << endl;
    cout << "G: " << bgr2xyY(G) << endl;
    cout << "B: " << bgr2xyY(B) << endl;
    
    return 0;
}

/**
  Patchwise color matrix calculation (not used in final implementation)
*/
int run_color_patches (int argc, char* argv[])
{
    // 0) parse command line args
    int bw = atoi(argv[2]);
    int bh = atoi(argv[3]);
    int patchSize = atoi(argv[4]);
    string outfile = argv[5];
  
    Mat red   = imread(argv[6], CV_LOAD_IMAGE_UNCHANGED);
    Mat green = imread(argv[7], CV_LOAD_IMAGE_UNCHANGED);
    Mat blue  = imread(argv[8], CV_LOAD_IMAGE_UNCHANGED);
    
    if (red.data == NULL || blue.data == NULL ||  green.data == NULL) {
        cout << "Error loading input images" << endl;
        return -1;
    }
    
    vector<Mat> imgs;
    imgs.push_back(red);
    imgs.push_back(green);
    imgs.push_back(blue);
    
    
    // 1) calculate patch values
    
    if ( ((imgs[0].size().width - 2*bw) % patchSize) != 0 ||
         ((imgs[0].size().height - 2*bh) % patchSize) != 0 )
    {
        cout << "Error: screen size (height and width) must be a multiple of the patch size!" << endl;
        return -1;
    }
    
    // number of patches
    int numx = (imgs[0].size().width - 2*bw) / patchSize;
    int numy = (imgs[0].size().height - 2*bh) / patchSize;
    
    vector<Mat> patches(imgs.size());   // averaged patch values
    
    cout << "averaging over " << numx << " x " << numy << " patches of size " << patchSize << endl;
    for (int i=0; i<imgs.size(); i++) {
        patches[i] = Mat::zeros (Size(numx, numy), CV_32FC3); 
        
        for (int y=0; y<numy; y++) {
            for (int x=0; x<numx; x++) {
                
                // iterate over patch pixels and calculate average
                for (int py=0; py < patchSize; py++) {
                    for (int px=0; px < patchSize; px++) {
                        patches[i].at<Vec3f>(y,x) += imgs[i].at<Vec3f>(bh + y*patchSize + py, bw + x*patchSize + px);
                    }
                }
                patches[i].at<Vec3f>(y,x) /= (float)(patchSize*patchSize);
                
            }
        }
    }
    
    cout << "dumping color matrices to " << outfile << endl;
    FileStorage fs(outfile.c_str(), FileStorage::WRITE);

    int num = 0;
    for (int y=0; y<numy; y++) {
        for (int x=0; x<numx; x++) {
       
            Vec3d R = patches[0].at<Vec3f>(y,x);
            Vec3d G = patches[1].at<Vec3f>(y,x);
            Vec3d B = patches[2].at<Vec3f>(y,x);
            
            
            
            R /= max(R[0],max(R[1],R[2]));
            G /= max(G[0],max(G[1],G[2]));
            B /= max(B[0],max(B[1],B[2]));
       
             // Note: uses openCV B-G-R order
            Mat conv (3,3, CV_64F);
            for (int j=0; j<3; j++) {
               conv.at<double>(j,0) = B[j];
               conv.at<double>(j,1) = G[j];
               conv.at<double>(j,2) = R[j];
            }

            
            stringstream ss;
            ss << "cstrans_" << num;
            fs << ss.str().c_str() << conv.inv();
            num++;
        }
    }
    fs.release();
    
    
    
}

/**
  Main: evaluate first argument and call required method with the arguments
*/
int main(int argc, char *argv[])
{
    cout << PROGNAME << " started" << endl;

    enum MODE {NONE, BGLIGHT, RESPONSE, SVR, AVERAGE, COLOR, COLORPATCHES};
    MODE mode = NONE;

    if (argc < 2) {
        help();
        return -1;
    }

    if ( (strcmp( argv[1], "--response" ) == 0) && (argc >= 8)) {
        mode = RESPONSE;
    } else if ( (strcmp( argv[1], "--svr" ) == 0) && (argc >= 9)) {
        mode = SVR;
    } else if ( (strcmp( argv[1], "--average" ) == 0) && (argc >= 5)) {
        mode = AVERAGE;
    } else if ( (strcmp( argv[1], "--color" ) == 0) && (argc >= 6)) {
        mode = COLOR;
    } else if ( (strcmp( argv[1], "--color_patches" ) == 0) && (argc >= 6)) {
        mode = COLORPATCHES;
    } else {
        help();
        return -1;
    }

    int result = -1;
    switch (mode) {
    
        case RESPONSE:
            result = run_response(argc, argv);
            break;
            
        case SVR:
            result = run_svr(argc, argv);
            break;
            
        case AVERAGE:
            result =  run_average(argc, argv);
            break;
            
        case COLOR:
            result =  run_color(argc, argv);
            break;
            
        case COLORPATCHES:
            result =  run_color_patches(argc, argv);
            break;
        
    }

    cout << PROGNAME << " finished" << endl;
    return result;
}

