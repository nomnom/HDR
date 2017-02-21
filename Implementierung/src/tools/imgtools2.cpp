/*
 * simple image tools 2 (opencv-based)
 *
 * @author Manuel Jerger
 *
 */

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O and image loading/writing
#include <string>
#include <getopt.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace cv;

// for old response curve code
struct rgb { rgb() : r(0), g(0), b(0) {}; float r; float g; float b; };



// response curve (gloal_
int responseSize;
float* responseCurve [3];
vector<float> exposureTimes;



// apply response curve to value
float linearize(float value, int channel) {
    assert (value >= 0.0);
    assert (value <= 1.0);

    float result;
    int idx=int((value*float(responseSize-1)+0.5));
    assert (idx>=0);
    assert (idx < responseSize);
    result = responseCurve[channel][ idx ];

    return result;
}

//map with response curve
Mat& apply_response (Mat& A)
{
    
    for (int t=0; t<A.total(); t++)
        for (int c=0; c<A.channels(); c++ )
                A.at<Vec3f>(t)[c] = linearize(A.at<Vec3f>(t)[c],c);
    return A;
}


//map with response curve
#define MAX_HDR_THRESHOLD  0.99
#define MIN_HDR_THRESHOLD  0.01
Mat& hdr_add(vector<Mat>& imgs, Mat& C)
{
    assert (exposureTimes.size() == imgs.size());
    
    C = Mat::zeros(imgs[0].size(), CV_32FC3);
    Mat cnts = Mat::zeros(imgs[0].size(), CV_32FC3);
        
    for (int i=0; i<imgs.size(); i++) {
        
        Mat A = imgs[i];
        
        assert (A.channels() == C.channels());
        assert (A.size().width == C.size().width);
        assert (A.size().height == C.size().height);
        
        for (int t=0; t<A.total(); t++) 
            for (int c=0; c<A.channels(); c++ ) 
                    if ( (A.at<Vec3f>(t)[c] <= MAX_HDR_THRESHOLD) && 
                         (A.at<Vec3f>(t)[c] >= MIN_HDR_THRESHOLD) )
                    {
                        // simple hut function for weighting
                        double weight = 1.0-abs(A.at<Vec3f>(t)[c]*2.0-1.0);
                        //cout << A.at<Vec3f>(t)[c] << " -> " << weight << endl;
                        C.at<Vec3f>(t)[c] += linearize(A.at<Vec3f>(t)[c], c) / exposureTimes[i] * weight;
                        cnts.at<Vec3f>(t)[c] += weight;
                    }
    
    }
  
    for (int t=0; t<C.total(); t++)
        for (int c=0; c<C.channels(); c++ )
                if (cnts.at<Vec3f>(t)[c] > 0)
                    C.at<Vec3f>(t)[c] /= (float)cnts.at<Vec3f>(t)[c];
    
    return C;
}



// clamp to 0 .. 1
float clamp (float val)
{
    if (val < 0.0) return 0.0;
    if (val > 1.0) return 1.0;
    return val;
}


//clamp pixel values
Mat& clamp (Mat& A, float lower, float upper)
{
    
    for (int t=0; t<A.total(); t++)
        for (int c=0; c<A.channels(); c++ ) {
            if (A.at<Vec3f>(t)[c] < lower) A.at<Vec3f>(t)[c] = lower;
            if (A.at<Vec3f>(t)[c] > upper) A.at<Vec3f>(t)[c] = upper;
        }
    return A;
}

Mat& gamma (Mat& A, float g)
{

    for (int t=0; t<A.total(); t++)
        for (int c=0; c<A.channels(); c++ ) {
            A.at<Vec3f>(t)[c] = pow(A.at<Vec3f>(t)[c],g);
        }
    return A;
}



// linear scale value v, so that whitepoint wp is mapped to 1.0 and bp is mapped to 0.0
float mapLinear (float val, float wp, float bp) {
    return 1/(wp-bp) * val + bp/(bp-wp);
}

Mat& mapImgLinear (Mat& A, float from, float to)
{

    for (int t=0; t<A.total(); t++)
        for (int c=0; c<A.channels(); c++ ) {
            A.at<Vec3f>(t)[c] = mapLinear(A.at<Vec3f>(t)[c], from, to);
        }
    return A;
}

rgb mapLinear (rgb val, rgb wp, rgb bp) {
    rgb result;
    result.r = mapLinear(val.r, wp.r, bp.r);
    result.g = mapLinear(val.g, wp.g, bp.g);
    result.b = mapLinear(val.b, wp.b, bp.b);
    return result;
}

Mat toLuminance (Mat& in) 
{
   Mat out = Mat (in.size(), CV_32FC1);

   Matx13f lumVec (0.072169, 0.71516, 0.212671);

   for (int i=0; i<in.size().height; i++) {
      for (int j=0; j<in.size().width; j++) {
          out.at<float>(i,j) = (lumVec * Matx31f(in.at<Vec3f>(i,j)))(0);
      }
   }
   
  return out;
}

// loads rgb response curve (either .m format created with hdrcalibrate, or a three-column list w/o indices)
void loadResponseCurve (string filename)
{
    ifstream in;
    in.open(filename.c_str(), ios::in);
    vector<rgb> values;

    // fileextension determines format
    string suffix = ".m";
    if (std::equal(filename.begin() + filename.size() - 2, filename.end(), suffix.begin() ) ) {

        // load .m format
        int color=-1; // current color
        int numRows=0;
        string tmp;
        while (!in.eof()) {
            getline (in,tmp);
            if (tmp.c_str()[0] == '#') {
                if (tmp.length() == 35)  {
                    if (tmp.substr(25,10) == "channel IR") { color = 0; }
                    else if (tmp.substr(25,10) == "channel IG") { color = 1; }
                    else if (tmp.substr(25,10) == "channel IB") { color = 2; }
                } else if (tmp.length() > 7) {

                    if (tmp.substr(0,7) == "# rows:") {
                        stringstream ss; ss << tmp.substr(8); ss >> numRows;

                        if (values.size() < numRows) {
                            for (int i=values.size(); i<numRows; i++) {
                                values.push_back(rgb());
                            }
                        }
                    }
                }
            } else if (color >= 0 && numRows > 0) {
                float logVal, val;
                int index;
                for (int i=0; i<numRows; i++) {

                    stringstream ss (tmp);

                    ss >> logVal; ss >> index; ss >> val;

                    switch (color) {
                        case 0: values[index].r = val; break;
                        case 1: values[index].g = val; break;
                        case 2: values[index].b = val; break;
                        default: break;
                    }

                    getline(in, tmp);
                }
                if (color == 2) break;
            }
        }
        responseSize = numRows;
    } else {
        // assume values in three columns
        rgb tmp;
        while (in >> tmp.r && in >> tmp.g && in >> tmp.b) {
            values.push_back (tmp);
        }
        responseSize = values.size();
    }

    in.close();
    responseCurve[0] = new float [responseSize];
    responseCurve[1] = new float [responseSize];
    responseCurve[2] = new float [responseSize];

    for (int i=0; i<responseSize; i++) {
        responseCurve[0][i] = values[i].r;
        responseCurve[1][i] = values[i].g;
        responseCurve[2][i] = values[i].b;
        //cout << ">>>>" << i << " : " << values[i].r << " " <<  values[i].g << " " <<  values[i].b << endl;
    }

    // fits image range of response curve to  (0:1);
	float maxVal = max(responseCurve[0][responseSize-1], max(responseCurve[1][responseSize-1], responseCurve[2][responseSize-1]));
	float minVal = 0;//max(max(responseCurve[0][0], responseCurve[1][0]), responseCurve[2][0]);
	for (int c=0; c<3; c++) {
		for (int i=0; i<responseSize; i++) {
			responseCurve[c][i] = clamp(mapLinear(responseCurve[c][i], maxVal, minVal));
		}
	}

   cout << "loaded response curve of size " << responseSize  << " from " << filename << endl;
}




void dumpPixelLine (Mat& A, int pos, char dir)
{
  // each channel separate
  for (int c=0; c<A.channels(); c++) {
    cout << "channel " << c << endl;

    // horizontal
    if (dir == 0) {
        assert ( (pos >= 0) && (pos < A.rows) );
        for (int x=0; x<A.cols; x++) {
            cout << (A.channels()==1 ? A.at<float>(pos,x) : A.at<Vec3f>(pos,x)[2-c]) << " "; // RGB order
        }
        cout << endl;

    // vertical
    } else {

        assert ( (pos >= 0) && (pos < A.cols) );
        for (int y=0; y<A.rows; y++) {
            cout << (A.channels()==1 ? A.at<float>(y,pos) : A.at<Vec3f>(y,pos)[2-c]) << " "; // RGB order
        }

        cout << endl;
    }
  }
}



void dumpPixels(Mat& A, int c, int patchsize, string outfile)
{
  ofstream of;
  of.open(outfile.c_str());
  int sizex = A.cols/patchsize;
  int sizey = A.rows/patchsize;

  float patchval; int numsamples;
  for (int x=0; x<sizex; x++) {
     for (int y=0; y<sizey; y++) {
      if (patchsize>1) {
          patchval=0; numsamples=0; // note: patches on the borders may be smaller than patchsize
          for (int px=0; px<patchsize && x*patchsize+px < A.cols; px++) {
             for (int py=0; py<patchsize && y*patchsize+py < A.rows; py++) {
                if (A.channels() == 1) {
                  patchval += A.at<float>(y*patchsize+py,x*patchsize+px);
                } else {
                  patchval += A.at<Vec3f>(y*patchsize+py,x*patchsize+px)[2-c];
                }
                numsamples++;
             }
          }
          patchval /= (float)numsamples;
          of << x << " " << y << " " << patchval << endl;
      } else {
          of << x << " " << y << " " << (A.channels()==1?A.at<float>(y,x):A.at<Vec3f>(y,x)[2-c]) << endl;
      }
     }
     of << endl; // gnuplot pm3d spacer
  }


  of.close();
}


// plot a slice through multiple image
void dump_3d_slice(vector<Mat>& images, int dir, int pos, string outfile) 
{


  ofstream of;
  of.open(outfile.c_str());
  
  // each channel separate
  for (int c=0; c<images[0].channels(); c++) {
    of << "#channel " << c << endl;

    // horizontal
    if (dir == 0) {
        for (int i=0; i<images.size(); i++) {
            Mat A = images[i];
            for (int x=0; x<A.cols; x++) {
                of << i << " " << x << " " << A.at<Vec3f>(pos,x)[2-c] << endl; // RGB order
            }
            of << endl;
        }
        
    // vertical
    } else {

        for (int i=0; i<images.size(); i++) {
            Mat A = images[i];
            for (int y=0; y<A.rows; y++) {
                of << i << " "  << y << " " << A.at<Vec3f>(y,pos)[2-c] << endl; // RGB order
            }
            of << endl;
        }
    }
  }
  of.close();

}

void printStats (Mat& A)
{
    double vmin[4], vmax[4], avg[4];

    for (int c=0; c<3; c++) {
        vmin[c]=1e20;
        vmax[c]=0;
        avg[c]=0;
        int avgnum=0;
        for (int x=0; x<A.cols; x++) {
            for (int y=0; y<A.rows; y++) {
                vmin[c] = A.at<Vec3f>(y,x)[2-c] < vmin[c] ? A.at<Vec3f>(y,x)[2-c]: vmin[c];
                vmax[c] = A.at<Vec3f>(y,x)[2-c] > vmax[c] ? A.at<Vec3f>(y,x)[2-c]: vmax[c];
                avg[c] += A.at<Vec3f>(y,x)[2-c];
                avgnum++;
            }
        }
        avg[c] /= (double)avgnum;
    }
    vmin[3] = min(vmin[0], min(vmin[1],vmin[2]));
    vmax[3] = max(vmax[0], max(vmax[1],vmax[2]));
    avg[3] = (avg[0]+avg[1]+avg[2]) / 3.0;

    cout << "size (WxH): " << A.cols << " x " << A.rows << endl;
    cout << "channels: " << A.channels() << endl;
    cout << "depth: " << A.depth() << endl;
    cout << "OpenCV type: " << A.type() << endl;
    cout << endl;
    cout << " minrgb: " << vmin[0] << " " << vmin[1] << " " << vmin[2] << endl;
    cout << " min: " << vmin[3] << endl;
    cout << " maxrgb: " << vmax[0] << " " << vmax[1] << " " << vmax[2] << endl;
    cout << " max: " << vmax[3] << endl;
    cout << endl;
    cout << " avgrgb: " << avg[0] << " " << avg[1] << " " << avg[2] << endl;
    cout << " avg: " << avg[3] << endl;
    cout << endl;
    cout << " R: " << vmax[3] / vmin[3] << endl;
}


Mat& apply_transform (Mat& img, Mat& trans) 
{
    assert ( (trans.size().height == 3) && (trans.size().width == 3 ) );
    for (int i=0; i<img.total(); i++) {
        Mat val (img.at<Vec3f>(i));
        val = trans * val;
        img.at<Vec3f>(i) = Vec3f(val);
    }
    return img;
}


Mat& logarithm (Mat& img) 
{
 if (img.channels() == 1) { 
    for (int i=0; i<img.total(); i++) {
        if (img.at<float>(i) == 0) {
           img.at<float>(i) = -10e10;
        } else {
           img.at<float>(i) = log10(img.at<float>(i));
        }
 
    }
 } else {  
  for (int i=0; i<img.total(); i++) {
     for (int c=0; c<3; c++) {
        if (img.at<float>(i) == 0) {
           img.at<float>(i) = -10e10;
        } else {
           img.at<Vec3f>(i)[c] = log10(img.at<Vec3f>(i)[c]);
        }
      }
    }
}
    return img;
}



////////// MAIN  ///////////////////////////////////////////////////////

int main(int argc, char *argv[])
{

    // input/output images (cache)
    vector<Mat> imgs;

    // iterate over arguments, perform actions
    int option_index = 0, c=0;
    while ( ( c = getopt (argc, argv, "i:r:o:ASMDva:V:m:sH:X:f:g:h:I:c:d:C:t:lLF:P:p:")) != -1)
    {
        string tmp;
        stringstream arg;
        if (c) arg << (optarg);
        switch (c) {
        
            // load images
            case 'i':
            {
                arg >> tmp;
                cout << "loading " << tmp << endl;
                Mat inimg = imread(tmp, CV_LOAD_IMAGE_UNCHANGED);
                if (inimg.data == NULL ) {
                    cout << "Error while loading the image!";
                } else {
                    if (inimg.type() != CV_32FC3) {
                        cout << "Warning: input image is not 32 bit float!" << endl;
                        inimg.convertTo(inimg, CV_32FC3, 1.0/256.0, 0.0);   // just assume its 8 bit int
                    }
                    imgs.push_back(inimg);
                }
                break;
            }
            
            // write out result 
            case 'o':
            {   
                string outfile; arg >> outfile;
                cout << "writing result to " << outfile << endl;
                if (imgs.size() > 1) cout << "Warning: more than one image in queue ! Dumping only the first one!" << endl;
                imwrite(outfile, imgs[0]);
                break;
            }
           
            // Sum (ADD) images 
            case 'A':
            {
                cout << "adding " << imgs.size() << " images" << endl;
                for (int i=1; i<imgs.size(); ++i) imgs[0] += imgs[i];
                imgs.resize(1);
                break;
            } 
            
            // subtract the other images from the first
            case 'S':
            {
                cout << "subtracting " << imgs.size() - 1 << " images from the first " << endl;
                for (int i=1; i<imgs.size(); ++i) imgs[0] -= imgs[i];
                imgs.resize(1);
                break;
            }
            
            // multiply images
            case 'M':
            {
                cout << "multiplying " << imgs.size() << " images" << endl;
                for (int i=1; i<imgs.size(); ++i) imgs[0] = imgs[0].mul(imgs[i]);
                imgs.resize(1);
                break;
            }
            
            // divide first image by the others
            case 'D':
            {
                cout << "dividing the first image by the other " << imgs.size() - 1 << " images" << endl;
                for (int i=1; i<imgs.size(); ++i) imgs[0] /= imgs[i];
                imgs.resize(1);
                break;
            }
            
            // average images
            case 'v':
            {
                cout << "calculating average of " << imgs.size() << " images" << endl;
                for (int i=1; i<imgs.size(); ++i) imgs[0] += imgs[i];
                imgs[0].convertTo(imgs[0], -1, 1.0/(float)imgs.size(), 0.0);
                imgs.resize(1);
                break;
            }
            
            // add a scalar to the images
            case 'a':
            {
                float val[3] = {-1, -1, -1};
                arg >> val[0]; arg >> val[1]; arg >> val[2];
                if (val[1] == -1) val[1] = val[0]; // if only one arg was given, use it for all color channels
                if (val[2] == -1) val[2] = val[0];

                cout << "adding the scalar rgb(" << val[2] << "," << val[1] << "," << val[0] << ") to " << imgs.size() << "  image" << endl;
                for (int i=0; i<imgs.size(); ++i) imgs[i] += CV_RGB(val[2],val[1],val[0]);
                break;
            }
            
            // multiply images by a scalar
            case 'm': 
            {
                float val;
                arg >> val;
                cout << "multiplying " << imgs.size() << " images with the scalar " << val << endl;
                for (int i=0; i<imgs.size(); ++i) imgs[i] *= val;
                break;
            }

             // apply logarithm (base 10)
            case 'L': 
            {
                cout << "applying base-10 logarithm to " << imgs.size() << " images" << endl;
                for (int i=0; i<imgs.size(); ++i) imgs[i] = logarithm(imgs[i]);
                break;
            }
            
            // clamp values
            case 'c':
            {
                double lower,upper; arg >> lower; arg >> upper;
                cout << "clamping " << imgs.size() << " images between " << lower << " and " << upper << endl;
                for (int i=0; i<imgs.size(); ++i) clamp(imgs[i], lower, upper);
                break;
            }
            
            // merge channels: three images r/g/b to one image
            case 'C':
            {
                if (imgs.size() != 3) { cout << "warning: not exactly three images given!" << endl; }
                cout << "merging channels: taking red from first image, green from the second and blue from the third" << endl;
                Mat result (imgs[0].size(), CV_32FC3);
                for (int c=0; c<3; c++) {
                    for (int i=0; i<result.total(); i++) {
                        result.at<Vec3f>(i)[c] = imgs[2-c].at<Vec3f>(i)[c];
                    }
                } 
                imgs.resize(1);
                imgs[0] = result; 
                break;
            }
                         
            // diff (prints several difference meassures and creates the absolute difference image)
            case 'd':
            {
                
                if (imgs.size() > 2) cout << "Warning: more than two image in queue!" << endl;
                cout << "calculating difference of first two images";
                char mode = 'a';
                arg >> mode;
                Mat result = Mat::zeros(imgs[0].size(), CV_32FC3);
                switch (mode) {
                    
                    // absolute difference (L1)
                    case 'a':   
                        cout << " using L1 metric" << endl;
                        cout << "diff: " << norm(imgs[0], imgs[1], CV_L1) / imgs[0].size().area() << endl;
                        absdiff(imgs[0], imgs[1], result);
                        break;
                        
                    // L2 norm
                    case  'l':
                        cout << " using L2 metric" << endl;
                        cout << "diff: " << norm(imgs[0], imgs[1], CV_L2)  / imgs[0].size().area() << endl;
                        imgs[0] -= imgs[1];
                        sqrt(imgs[0].mul(imgs[0]), result);
                        break;
                }
                imgs.resize(1);
                imgs[0] = result; 
                break;
            }
            
            // print images statistics
            case 's': 
            {
                for (int i=0; i<imgs.size(); ++i) {
                    cout << "image statistics for image " << i << ":" << endl;
                    printStats(imgs[i]);
                }
            
                break;
            }
            
            // 2d Plot of one slice (horizontal or vertical) of first image
            case 'H':
            case 'V':
            {   
                int plotdir = (c=='H') ? 0 : 1;
                int pos; arg >> pos;
                cout << "printing pixel values of the " << pos << "'th " << ( (plotdir==0)?"row":"column") << endl;
                for (int i=0; i<imgs.size(); ++i) {
                   cout << "image " << i << ":" << endl;
                   dumpPixelLine (imgs[i], pos, plotdir);
                }
                
                break;
            }
            
            // dump 3d plot: slice through all images
            case 'I': 
            {
                int plotdir, pos;
                string pfile; 
                arg >> plotdir;
                arg >> pos;
                arg >> pfile;
                cout << "dumping 3d plot data: slice through the " << pos << "'th " << ( (plotdir==0)?"row":"column") << " of " << imgs.size() << " images to " << pfile << endl;
                dump_3d_slice(imgs, plotdir, pos, pfile);
                break;
            }
            
            // dump 3D plot data:
            case 'X': 
            {
                int chan; arg >> chan;
                int psize; arg >> psize;
                string pfile; arg >> pfile;
                cout << "dumping pixel values of the first image (channel " << chan << ", patchsize " << psize << ") to " << pfile << endl;
                dumpPixels(imgs[0], chan, psize, pfile);
            
                break;
            }
            
            // apply median filter to all images
            case 'f': 
            {
                int k; arg >> k;
                cout << "applying " << k << "x" << k << " median filter to " << imgs.size() << " images" << endl;
                for (int i=0; i<imgs.size(); i++) medianBlur(imgs[i], imgs[i], k);
                break;
            }
            
            // apply gauss filter to all images
            case 'g': 
            {
                int k; arg >> k;
                cout << "applying " << k << "x" << k << " Gauss filter with sigma=" << k << " to " << imgs.size() << " images" << endl;
                for (int i=0; i<imgs.size(); i++)  GaussianBlur(imgs[i], imgs[i], Size(k,k), k,k);
                break;
            }
            case 'l':
            {
               cout << "converting image to luminance (assumed srgb primaries)" << endl;
               for (int i=0; i<imgs.size(); i++) imgs[i]=toLuminance(imgs[i]);
               break; 
            }
            
            // map images over a given response curve or apply gamma
            case 'r': 
            {
                 string response;
                 arg >> response;
                 double g=0;
                 if ( strcmp(response.c_str(), "g" ) == 0 ) { 
                   arg >> g; 
                   cout << "applying a gamma of " << g<< endl;
                    for (int i=0; i<imgs.size(); i++) {
                       imgs[i] = gamma(imgs[i],g);
                   }
     
                  
                 } else {                  
                   loadResponseCurve(response);
                   cout << "applying response curve " << response << " to " << imgs.size() << " images" << endl;
                   for (int i=0; i<imgs.size(); i++) {
                       imgs[i] = clamp(imgs[i],0.0, 1.0);
                       imgs[i] = apply_response(imgs[i]);
                   }
                 }
                 break;
            }
            
            // fit image values into specified range, so 0 maps to 'from' and 1 maps to 'to'
            case 'F':
            { 
                //float from, to;
                float  to;
                //arg >> from;
                arg >> to;
                //if (from == to) {
                  cout << "scaling " << imgs.size() << " images, so their max value is " << to << endl;
                  for (int i=0; i<imgs.size(); i++) { 
                     Mat A = imgs[i];
                     double vmax[4];   
                     for (int c=0; c<3; c++) {
                        vmax[c]=0;
                        for (int x=0; x<A.cols; x++) {
                            for (int y=0; y<A.rows; y++) {
                                vmax[c] = A.at<Vec3f>(y,x)[2-c] > vmax[c] ? A.at<Vec3f>(y,x)[2-c]: vmax[c];
                            }
                        }
                     }
                     double maxval = max(vmax[0], max(vmax[1],vmax[2]));
                     cout << "image " << i << " max value is " << maxval << endl;
                     imgs[i] *=  (1.0/maxval);
                  }
               // } else {
               //    cout << "scale and shift values of " << imgs.size() << " images so each one fits in the range " << from << " .. " << to << endl;
               //    for (int i=0; i<imgs.size(); i++) { 
               //      imgs[i] = mapImgLinear(imgs[i], from, to);
               //    }
               // }
                
               break;   
            }
            
            // perform a simple HDR add
            case 'h': 
            {
                 string response;
                 arg >> response;
                 loadResponseCurve(response); 
                 
                 stringstream exps;
                 for (int i=0; i<imgs.size(); i++) { 
                     arg >> tmp; exposureTimes.push_back(atof(tmp.c_str()));
                     exps << " " << tmp;
                 }   
                 cout << "performing HDR add of " << imgs.size() << " images (exposures:" << exps.str() << "; response: " << response << ")" << endl;
                 Mat result;
                 hdr_add(imgs, result); 
                 imgs.resize(1);
                 imgs[0] = result;
                 break;
            }
            
            // apply color transformation matrix
            case 't':
            {
                string field; arg >> field;
                string file; arg >> file;
                Mat trans;
                FileStorage fs(file, FileStorage::READ);
                fs[field.c_str()] >> trans;
                trans.convertTo(trans, CV_32FC3);
                cout << "loaded color transform from " << file << ":" << endl << trans << endl;
                cout << "applying transform to " << imgs.size() << " images" << endl;
                for (int i=0; i<imgs.size(); i++) {
                    imgs[i] = apply_transform(imgs[i], trans);
                }
                fs.release();
                break;
            }
            
            // remove dead pixel (1x1; for applying before bayering)
            case 'P':
            {
                int x; arg >> x;
                int y; arg >> y;
                
                cout << "removing dead pixel at " << x << "," <<y << endl;
                for (int m=0; m<imgs.size(); m++) {
                    Vec3f sum(0,0,0);
                    for (int i=-1; i<2; i++) {
                        for (int j=-1; j<2; j++) {
                           if (i==j==0) continue;
                           sum += imgs[m].at<Vec3f>(y+j,x+i);
                        }
                    }
                    imgs[m].at<Vec3f>(y,x) = sum / 8;
                }
                break;
            }
            
            // remove dead pixel (3x3; for applying after bayering)
            case 'p':
            {
                int x; arg >> x;
                int y; arg >> y;
                cout << "removing dead 3x3 pixel at " << x << "," <<y << endl;
                for (int m=0; m<imgs.size(); m++) {
                    
                    // calc average of sourrounding pixels
                    Vec3f sum(0,0,0);
                    for (int i=-2; i<=2; i++) {
                        for (int j=-2; j<=2; j++) {
                         if (abs(i)<2 && abs(j)<2 ) continue;
                         sum += imgs[m].at<Vec3f>(y+j,x+i);
                        }
                    }
                    sum /= 16 ;
                
                    // fill 3x3 region
                    for (int xf=-1; xf<=1; xf++) {
                        for (int yf=-1; yf<=1; yf++) {
                            imgs[m].at<Vec3f>(y-yf,x+xf) =   sum;
                        }
                    }
                }
                break;
            }
            
        }
    }
    
    return 0;
}





