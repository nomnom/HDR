/**
 control_display : control the framebuffer using OpenCV fullscreen OpenGL window 
  
 This program can control the framebuffer in various ways:
   - show an image
   - set uniform color
   - show checkerboard
   - show circles 
   - vertical and horizontal linear multi-ramps with border
   - exponential ramps
   - 3x3x3 Bit color map

 @author Manuel Jerger <nom@nomnom.de>
 
*/

#include "control_display.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "control_display";


/**
 Prints usage.
*/
void help()
{
    cout << "Shows a fullscreen window and can display patterns." << endl <<
        "Usage: " << PROGNAME << "  <w> <h> <duration> --<mode> [-o out_image]" << endl <<
        "     <w> <h>                        Display resolution" << endl <<
        "     <duration>                     time in seconds before closing the window (0 means infinite duration)" << endl <<
        "     [-o out_image]                 Write image to file instead of displaying it in fullscreen" <<
        "  following <mode> are available:" << endl <<
        "     --image <file>                 Show a fullscreen image (values must be in range 0..1)" << endl <<
        "     --rgb <r> <g> <b>              Uniform color, float values between 0.0 .. 1.0" << endl <<
        "     --checker <w> <h>              Checkerboard the size of w x h" << endl <<
        "     --circles                      four circles " << endl <<
        "     --vramp <num> <border> [r g b] num vertical ramps (linear, from 0 (left) to 1 (right)); negative num means inverted ramp; bordersize in pixels" << endl <<
        "     --hramp <num> <border> [r g b] num horizontal ramps (linear, from 0 (top) to 1 (bottom)); negative num means inverted ramp; bordersize in pixels" << endl <<
        "     --expramp <size> <border>      Exponential ramp; from 10^(-size) on the left, to 10^0=1 on the right" << endl << 
        "     --colors                      color patches (512 different colors)" << endl << endl;
}


/**
 Generates a checkerboard image for use with the undistort_display program
 Produces a three-channel image of size width x height, containing a checkerboard with w x h panels and a white border of borderfactor*screenDimensions
 @param width Image width in pixels
 @param height Image height in pixels
 @param w Checker size (vertical panels)
 @param h Checker size (horizontal panels)
 @return OpenCV Image containing the checker board
*/

Mat makeChecker (int width, int height, int w, int h, float borderFactor)
{
    Mat img(height, width, CV_32FC3);

    // size of white border as a factor of screen dimensions
    int wb = (int)((float)width * borderFactor);
    int hb = (int)((float)height * borderFactor);

    float value;
    float* p;
    for (int y=0; y<height; y++) {
        int yi = (int)( (float)(y-hb)/(float)(height-2*hb)*(float)(h) );
        p = img.ptr<float>(y);
        for (int x=0; x<width; x++) {
            int xi = (int)( (float)(x-wb)/(float)(width-2*wb)*(float)(w) );
            if (y >= hb && y < height-hb && x >= wb && x < width-wb) {
                value = (((xi%2) ^ (yi%2)) == 1) ? 1.0 : 0;
            } else {
                value = 1.0;
            }

            p[3*x] = value;
            p[3*x+1] = value;
            p[3*x+2] = value;
        }
    }

    return img;
}


/**
  Generates a horizontal, linear ramp with values from 0 .. 1 in all channels. Leaves a border of 'border' pixels on both sides, each with value 0 and 1 respectively.
  @param width Ramp width in pixels
  @param height Ramp height in pixels
  @param border Size of border in pixels. 
  @return OpenCV three channel Image containing the ramp
*/
Mat makeRamp (int width, int height, int border)
{
    Mat img(height, width, CV_32FC3, CV_RGB(1,1,1));

    float val;
    for (int x=0; x<width; x++) {
        if (x< border) {
            val = 0.0;
        } else if ( x>width-border ) {
            val = 1.0;
        } else {
            val = (float)(x-border) / (float) (width-2*border-1);
        }
        img.col(x) *= val;// CV_RGB(val,val,val);
    }

    return img;
}

/**
  Generates a horizontal, exponential amp with values from 10^(-size) .. 1 in all channels. Leaves a border of 'border' pixels on both sides.
  @param width Ramp width in pixels
  @param height Ramp height in pixels
  @param border Size of border in pixels. 
  @return OpenCV three channel Image containing the exponential ramp
*/

Mat exponentialRamp (int width, int height, int size, int border)
{
    Mat img(height, width, CV_32FC3, CV_RGB(1,1,1));

    
    double val;
    for (int x=0; x<width; x++) {
      if (x<border || x > width-border) { 
         img.col(x) *= 0; 
      } else {  
        val=(x-border-1)/(double)(width-border);
        img.col(x) *= pow(10.0,(val-1)*size);
      }
    }
    
    return img;
}

/**
  Generates a color map: Image contains 512 colored rectangles showing a 3x3x3 Bit subspace of the full 24bit color space
  @param width Image width in pixels
  @param height Image height in pixels
  @return OpenCV three channel Image
*/
Mat makeColors512 (int width, int height)
{
    Mat img(height, width, CV_32FC3);

    // 1) color array
    vector<Vec3f> colors;
    float stepsize = (1.0 / 7.0);
    for (int r=0; r<8; r++) {
        for (int g=0; g<8; g++) {
            for (int b=0; b<8; b++) {
                colors.push_back(Vec3f( (float)r * stepsize,
                                        (float)g * stepsize,
                                        (float)b * stepsize ) );
            }
        }
    }

    // 2) display colored checker
    int w = 32;
    int h = 16;

    float* p;
    for (int y=0; y<height; y++) {
        int yi = (int)( (float)y/(float)height*(float)(h) );
        p = img.ptr<float>(y);
        for (int x=0; x<width; x++) {
            int xi = (int)( (float)x/(float)width*(float)(w) );
            int index = yi * w + xi;
            p[3*x] = colors[index][2];
            p[3*x+1] = colors[index][1];
            p[3*x+2] = colors[index][0];
        }
    }
    return img;
}


/**
 Main code is here
*/
int main(int argc, char *argv[])
{
    cout << PROGNAME << " started" << endl;

    enum MODE {RGB, IMAGE, CHECKER, CIRCLES, VRAMP, HRAMP, EXPRAMP, COLORS, CENTER };
    MODE mode = RGB;

    if (argc < 5) {
        help();
        return -1;
    }

    // display size in pixels
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    float duration = atof(argv[3]);

    string outfile = "";
    if ( strcmp(argv[argc-2], "-o") == 0 ) {
        outfile = argv[argc-1];
    }




    // set uniform color
    if ( (strcmp( argv[4], "--rgb" ) == 0) && (argc >= 8)) {
        mode = RGB;
    // show image file
    } else if ( (strcmp( argv[4], "--image" ) == 0) && (argc >= 6) ) {
        mode = IMAGE;
    // show checkerboard
    } else if ( (strcmp( argv[4], "--checker" ) == 0) && (argc >= 7) ) {
        mode = CHECKER;
    // show circle calibration image
    } else if ( (strcmp( argv[4], "--circles" ) == 0) && (argc >= 5) ) {
        mode = CIRCLES;
    // vertical ramp
    } else if ( (strcmp( argv[4], "--vramp" ) == 0) && (argc >= 5) ) {
         mode = VRAMP;
    // vertical ramp
    } else if ( (strcmp( argv[4], "--hramp" ) == 0) && (argc >= 5) ) {
         mode = HRAMP;
    // exponential ramp
    } else if ( (strcmp( argv[4], "--expramp" ) == 0) && (argc >= 5) ) {
         mode = EXPRAMP;
    } else if ( (strcmp( argv[4], "--colors" ) == 0) && (argc >= 5) ) {
        mode = COLORS;
    } else if ( (strcmp( argv[4], "--center" ) == 0) && (argc >= 5) ) {
        mode = CENTER;
    } else {
        help();
        return -1;
    }

    // fullscreen color image
    Mat img;

    switch (mode) {
        case RGB:
        {
            float r =  atof(argv[5]);
            float g =  atof(argv[6]);
            float b =  atof(argv[7]);
            img = Mat(height, width, CV_32FC3, CV_RGB(r,g,b));
            cout << "r = " << r << " g = " << g << " b = " << b << endl;
            break;
        }
        case IMAGE:
        {
            img = imread (argv[5], CV_LOAD_IMAGE_UNCHANGED);
            break;
        }
        case CHECKER:
        {
            int w = atoi(argv[5])+1;
            int h = atoi(argv[6])+1;
            img = makeChecker(width, height, w, h, 0.1);
            break;
        }

        case CIRCLES: // unused for now..
        {
            // NOTE: antialiasing seems not to be working with 32 bit float images
            img = Mat(height, width, CV_8UC3 );

            // fill image with white
            unsigned char* p;
            for (int y=0; y<height; y++) {
                p = img.ptr<unsigned char>(y);
                for (int x=0; x<3*width; x++) {
                    p[x] = 255;
                }
            }
            float radius = (float)max(width, height) / 20.0;
            float distx = (float)max(width, height) / 10.0;
            float disty = distx;
            Point2f pos[] = {   Point2f(distx, disty),
                                Point2f((float)width - distx, disty),
                                Point2f(distx, (float)height - disty),
                                Point2f((float)width - distx, (float)height - disty)};
            for (int i=0; i<4; i++) {
                circle (img, pos[i], radius, cvScalar(0,0,0), -1, CV_AA);
            }
            break;
        }

        case VRAMP:
        {

            int numRamps = 1;
            bool inverted = false;
            int borderSize = 0;

            if (argc >= 6) numRamps = atoi(argv[5]);
            if (argc >= 7) borderSize = atoi(argv[6]);

            // negative number of ramps means inverted ramp
            if (numRamps < 1) {
                numRamps *= -1;
                inverted = true;

                // we will substract instead of add while composing the image, thus initialize it with 1.0 instead of 0.0
                img = Mat(height, width, CV_32FC3, CV_RGB(1,1,1));

            } else if (numRamps == 0) {
                cout << "Error: zero is not a valid number of ramps!" <<  endl;

            } else {
                img = Mat(height, width, CV_32FC3, CV_RGB(0,0,0));
            }

            cout << "displaying " << numRamps << (inverted?" inverted":"") << " vertical ramps" << endl;

            int size = height/numRamps;
            if ( height - numRamps*size > 0) cout << "Note: leftover pixels on the bottom of size " << height - numRamps*size << endl;

            // a single ramp
            Mat ramp = makeRamp(size, width, borderSize); // note: switched axes; image will be transposed in the next step

            // rotate 90 deg clockwise ( = transpose )
            ramp = ramp.t();

            // compose ramps
            for (int i=0; i<numRamps; i++) {
                if (inverted) {
                    img (Rect(0, i * size, width, size)) -= ramp;
                } else {
                    img (Rect(0, i * size, width, size)) += ramp;
                }
            }

            // colorize if wanted
            if (argc >= 10) {
                float r=atof(argv[7]), g=atof(argv[8]), b=atof(argv[9]);
                img = img.mul(Mat(height, width, CV_32FC3, CV_RGB(r,g,b)), 1.0 );
            }
            break;
        }

        case HRAMP:
        {


            int numRamps = 1;
            bool inverted = false;
            int borderSize = 0;

            if (argc >= 6) numRamps = atoi(argv[5]);
            if (argc >= 7) borderSize = atoi(argv[6]);

            // negative number of ramps means inverted ramp
            if (numRamps < 1) {
                numRamps *= -1;
                inverted = true;

                // we will substract instead of add while composing the image, thus initialize it with 1.0 instead of 0.0
                img = Mat(height, width, CV_32FC3, CV_RGB(1,1,1));

            } else if (numRamps == 0) {
                cout << "Error: zero is not a valid number of ramps!" <<  endl;

            } else {
                img = Mat(height, width, CV_32FC3, CV_RGB(0,0,0));
            }

            cout << "displaying " << numRamps << (inverted?" inverted":"") << " horizontal ramps" << endl;

            int size = width/numRamps;
            if ( width - numRamps*size > 0) cout << "Note: leftover pixel on the right of size " << width - numRamps*size << endl;

            // a single ramp
            Mat ramp = makeRamp(size, height, borderSize);

            // compose ramps
            for (int i=0; i<numRamps; i++) {
                if (inverted) {
                    img (Rect(i * size, 0, size, height)) -= ramp;
                } else {
                    img (Rect(i * size, 0, size, height)) += ramp;
                }
            }

            // colorize if wanted
            if (argc >= 10) {
                float r=atof(argv[7]), g=atof(argv[8]), b=atof(argv[9]);
                img = img.mul(Mat(height, width, CV_32FC3, CV_RGB(r,g,b)), 1.0 );
            }

            break;
        }

        case EXPRAMP:
        {
            int size=4; // 10^-4 - 10^0
            if (argc >= 6) size = atoi(argv[5]); 
            
            int border=0; 
            if (argc >= 7) border  = atoi(argv[6]); 
            cout << "generating exponential ramp with border "<<border<<"from 10^-" << size << " to 10^0" << endl;
            
            img = exponentialRamp(width, height, size, border);
            break;
        }
        
        case COLORS:
        {

            img = makeColors512(width, height);

            break;


        case CENTER:
        {

            img = Mat(Size(width, height), CV_32FC3, CV_RGB(1,1,1));
            const int s=50;
            Point2i center (width/2, height/2);
	    for (int x=center.x-s; x<center.x+s; x++) img.at<Vec3f>(center.y,x) = Vec3f(0,0,0);
	    for (int y=center.y-s; y<center.y+s; y++) img.at<Vec3f>(y,center.x) = Vec3f(0,0,0);
	

            break;


        }
        }
     
    }


    if (!outfile.empty()) {
        cout << "writing image to " << outfile << endl;

        const char* ext =  outfile.substr(outfile.length() - 4).c_str();

        // change data range for 8 bit integer files
        if ( strcmp(ext, ".exr") != 0 ) {
            img.convertTo(img, CV_8UC3, 256.0, 0);
        }

        imwrite(outfile, img);

    } else {

        // create fullscreen window
        namedWindow("main",  CV_WINDOW_OPENGL);
        cvSetWindowProperty("main", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
        imshow("main", img);

        // wait (approx.) for the desired duration
        if (duration) {
            int ms_runtime=0;
            for(;;) {
                if (waitKey(30) >= 0) break;
                ms_runtime += 30;
                if (ms_runtime >= (int)(1000.0 * duration)) break;
            }
        } else {
            waitKey();
        }

        destroyWindow("main");
    }

    // finished
    cout << PROGNAME << " finished" << endl;
    return 0;
}

