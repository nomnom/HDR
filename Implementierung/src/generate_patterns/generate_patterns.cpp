/**
 generate_patterns : Generates ARToolKit fiducials as images and definition files
 
 @author Manuel Jerger <nom@nomnom.de>
*/


#include "generate_patterns.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "generate_patterns";


/**
  Prints usage.
*/
void help()
{
    cout << "Generate unique patterns for use with ARToolKit" << endl << endl << 
        "Display marker:" << endl << endl << 
        " " << PROGNAME << " --display <num>" << endl << endl << 
        "      <num>            Number of patterns to generate and display." << endl << endl << endl << 
        "Dump marker images and artoolkit training data to a directory:" << endl << endl << 
        " " << PROGNAME << " --dump <num> <out_dir>" << endl << endl << 
        "      <num>            Number of patterns to generate and display." << endl <<
        "      <out_dir>        Output directory for images and training data." << endl << endl << endl << 
        "Create marker panels (regular grid) and dump images and multi marker data to a directory:" << endl << endl << 
        " " << PROGNAME << " --panel <start> <x> <y> <size_mm> <size_pixel> <spacing_pixel> <out_dir>  [t <x> <y> <z>] [x <deg> y <deg>  ... ]" << endl << endl << 
        "      <start>          Index of the first marker in the unique marker sequence" << endl <<
        "      <x> <y>          Panel layout." << endl <<
        "      <size_mm>        Marker size in mm (depends on how large you print the image)" << endl <<
        "      <size_pixel>     Marker size in pixel." << endl <<
        "      <spacing_pixel>  Spacing between marker in pixel." << endl <<
        "      <out_dir>        Output directory for panel image and multi marker data." << endl << 
        "      t <tx> <ty> <tz> Translate panel center, in mm." << endl <<
        "      <axis> <deg>     Rotate panel by deg degree around an axis (x/y/z) (origin is at center of panel, x=right y=up  z=normal" << endl << endl;
        
        
}


typedef unsigned int uint;


/**
  Create the fiducials image.
  @param marker Image will be modified
  @param b The 16 bit number defining the fiducial pattern
  @param size Fiducial size in pixels
*/
Mat& createMarker(Mat& marker, uint b, int size)
{
    marker = Mat::zeros(size,size, CV_8UC3);
    CvScalar color;
    
    int border = (size/4);
    int square = (size/8);
    
    for (int y=0; y<4; y++) {
        for (int x=0; x<4; x++) {
            if (b & (1 << (y*4+x) ) ) {
                color = CV_RGB(1,1,1);
            } else {
                color = CV_RGB(0,0,0);
            }
            rectangle(marker, Point2i(border+x*square,     border+y*square), 
                              Point2i(border+(x+1)*square, border+(y+1)*square), color,  CV_FILLED);
        }
    }
    
    return marker;
}


/**
  Hamming weight: count number of bits
*/
uint hammingWeight (uint b)
{
    uint sum = 0;
    for (int i=0; i<32; i++) {
        if (b & (1 << i)) sum++;
    }
    return sum;
}


/**
  Hamming distance: number of bits that differ between two numbers
*/
uint hammingDistance (uint a, uint b)
{
    a^=b;
    return hammingWeight (a);
}

/**
   'rotate' a pattern by 90 degrees cw
*/
uint rot (uint b)
{
    b &= 0xFFFF;
    uint res = 0;
    bool square[4][4];
    for (int y=0; y<4; y++)
        for (int x=0; x<4; x++)
             square[y][x] = (b & (1 << (y*4+x))) ? true : false;
    
    for (int y=0; y<4; y++)
        for (int x=0; x<4; x++)
            if (square[3-y][x]) res |= (1 << (x*4+y) );
            
                
    return res;
}

/**
   generate fixed number of patterns as bitstrings
*/
vector<uint> generatePatterns (int num)
{
    
    // fixed seed
    RNG rng(23);
    
    // patterns have to be different in at least minDist bits
    uint minDist = 4;
    
    // at least minSet bits must be 0 / 1 (also dismisses empty or full pattern)
    uint minSet = 4;    
    
    // generated patterns
    vector<uint> patts;    
    
    // count unused patterns
    int wasted = 0;
    
    
    cout << "generating " << num << " patterns ..." << flush;
    // find suitable pattern (naive method)    
    while (patts.size() < num) {
        
        // random bitstring 
        uint b = rng.next() &  0xFFFF;
        wasted++;
        
        // dismiss pattern if less than 4 squares sets black or white
        if (hammingWeight(b) < minSet || hammingWeight(b) > 16-minSet ) continue;
        
        
        
        // dismiss if pattern itself is rotational symmetric 
        bool dismiss=false;
        int rotated = b;
        for (int r=0; r<3; r++) {
            rotated = rot (rotated);
            if ( b == rotated) { 
                dismiss=true; 
                break;
            }
        }
        if (dismiss) continue;
        
        // check against previous patterns:
        for (int i=0; i<patts.size() && !dismiss; i++) {           
        
            // dismiss if pattern was already generated or distance is too small
            if (hammingDistance(patts[i],b) < minDist) { 
                dismiss = true;
                break;
            }
            
            // check rotated versions against previous pattern (rotate 3 times by 90 degree)
            int rotated = b;
            for (int r=0; r<3; r++) {
                rotated = rot (rotated);
                // dismiss if distance of rotated version to previous pattern is too small
                if (hammingDistance (patts[i], rotated) < minDist ) {
                    dismiss = true;
                    break;
                }
            }
            
        }
        if (dismiss) continue;
        
        // we have found a valid pattern
        patts.push_back(b);
        wasted--;    
    }
    
    cout << "done!" << endl;
    cout << "wasted " << wasted << " patterns" << endl;
    
    return patts;
}

/**
  Main: check first argument and execute the codepaths in a giant switch condition.
*/
int main (int argc, char* argv[])
{

    if (argc < 2) {
        help();
        return -1;
    }
    
    enum MODE {NONE, DISPLAY, DUMP, PANEL} mode = NONE;
    if (strcmp(argv[1], "--display") == 0) {
        mode = DISPLAY;
    } else if (strcmp(argv[1], "--dump") == 0) {
        mode = DUMP;
    } else if (strcmp(argv[1], "--panel") == 0) {
        mode = PANEL;
    }
    
    
        
    switch (mode) {
    
       case DISPLAY:
       {
            if (argc < 3) { 
                help();
                return -1;
            }
            int num = atoi(argv[2]);
            
            vector<uint> patts = generatePatterns(num);
            
            namedWindow("main");
            for (int i=0; i<patts.size(); i++) {
                Mat marker;
                createMarker(marker, patts[i], 400);
                marker.convertTo(marker, -1, 255, 0);
                imshow("main", marker);
                int key = waitKey(0) & 0xff;
                if (key == 27) break;
            }
            destroyWindow("main");
            
       } break;
       
       // Mode 1: dump marker images
       case DUMP:
       {     
        
            if (argc < 4) { 
                help();
                return -1;
            }
            int num = atoi(argv[2]);
            string outDir = argv[3];
            
            vector<uint> patts = generatePatterns(num);
       
            cout << "dumping marker images to " << outDir << " ..." << flush;
            for (int p=0; p<patts.size(); p++) {
                
                stringstream file;
                file << outDir << "/" << "marker_" << p << ".png";
                Mat marker;
                marker = createMarker(marker, patts[p], 200);
                marker.convertTo(marker, -1, 255, 0);  
                imwrite(file.str(), marker);
            }
            cout << "done!" << endl;


            //
            // ARToolKit training (just like lib/ARToolKit/util/mk_patt/mk_patt.c)
            //
            
            // virtual screen for artoolkit marker training
            // NOTE: screensize is fixed to 640x480 (can be changed in artoolkits config.h)
            int markerSize = 200;     
            Mat screen(480, 640,  CV_8UC3, CV_RGB(1,1,1));
            Rect region ( (screen.size().width-markerSize) / 2, (screen.size().height-markerSize) / 2, markerSize, markerSize); 
           
             
            // load default camera params (from ARToolkit example)
            ARParam cameraParam;
            if (arParamLoad("data/camera/artk_default.dat", 1, &cameraParam) < 0) {
                cout << "Error while loading default camera calibration data" << endl;
                return -1;
            }
            arInitCparam(&cameraParam);
            arParamChangeSize(&cameraParam, screen.rows, screen.cols, &cameraParam);


            cout << "training ARToolKit.. " << flush;
            for (int p=0; p<patts.size(); p++) {
                
                //
                // create virtual screen image showing the marker on a white background
                //
                Mat marker;
                marker = createMarker(marker, patts[p], markerSize);  
                marker.copyTo(screen(region));
         
                // (0,1) -> (0,255)
                screen.convertTo(screen, -1, 255, 0);  
                
                // detect marker
                ARMarkerInfo *marker_info = NULL;	// Pointer to array holding the details of detected markers.
                int markerNum = -1;					// Count of number of markers detected.
                
                if (arDetectMarker((ARUint8 *)screen.data, 100, &marker_info, &markerNum) < 0) {
                    cout << "Error while running arDetectMarker!" << endl;
                    return -1;
                }
                
                // find the marker with the largest area
                int maxArea = 0;
                int largestMarker = -1;
                for (int i = 0; i < markerNum; i++) {
                    if (marker_info[i].area > maxArea) {
                        maxArea = marker_info[i].area;
                        largestMarker = i;
                    }
                }
                if (largestMarker == -1) { 
                    cout << "Error: Marker not detected" << endl; 
                    return -1;
                }
                
                // save training data
                stringstream file;
                file << outDir << "/" << "marker_" << p << ".dat";
                char name[255];
                strcpy(name, file.str().c_str());
                if (arSavePatt((ARUint8 *)screen.data, &marker_info[largestMarker], name) < 0) {
                    cout << "Error while dumping training data!" << endl;
                }
            }
        }
        break;
        
        // mode 2: dump regular grid of patterns as image and a multimarker data for artoolkit
        case PANEL: 
        {
            if (argc < 9) { 
                help();
                return -1;
            }
            
            int startPatternIndex = atoi(argv[2]);   // index in sequence
            int numX = atoi(argv[3]);                // num markers in each direction
            int numY = atoi(argv[4]);
        
            double markerSize = atof(argv[5]);       // in mm 
            int markerSizePixel = atoi(argv[6]);     // in pixel
            int markerSpacingPixel  = atoi(argv[7]); // also in pixel
            string outDir = argv[8];
            
            double markerSpacing = markerSpacingPixel * markerSize / markerSizePixel;
            
            // patterns can only be generated in continous order, we have to generate them all
            int num = startPatternIndex + numX*numY ;
            vector<uint> patts = generatePatterns(num);
            
            // panel uses fixed basename: panel_layout_startindex_spacingfactor
            stringstream basename;
            basename << outDir << "/panel_" << numX << "x" << numY << "_" << startPatternIndex << "_" << round( (double) markerSizePixel / (double) markerSpacingPixel);
            
            
            // multi marker file
            stringstream ss;
            ss << basename.str() << ".dat";
            ofstream of;
            of.open(ss.str().c_str());
            of << "# layout: " << numX << "x" << numY << endl;
            of << "# size: " << markerSize << endl;
            of << "# spacing: " << markerSpacing << endl << endl << endl;
            of << numX*numY << endl << endl;
            
            // parsing remaining args
            int aidx = 9;
            
            // panel translation
            Matx44d panelTrans = Matx44d::eye();
            if (argc > 11 && argv[aidx][0] == 't') {
                panelTrans(0,3) = atof(argv[aidx+1]);
                panelTrans(1,3) = atof(argv[aidx+2]);
                panelTrans(2,3) = atof(argv[aidx+3]);
                aidx += 4;
            }
            
            // create rotation matrix from arguments
            Matx44d rotMat = Matx44d::eye();            
            for (int i=aidx; i+1<argc; i+=2) {
                char axis = argv[i][0];
                double a = atof(argv[i+1]) / 180.0 * M_PI;
                cout << "rotating panel by " << a << " rad around " << axis << "-axis" << endl;
                Matx44d rotation;
                switch (axis) {
                    case 'x': rotation = Matx44d(1,0,0,0,
                                                 0,cos(a),sin(a),0,
                                                 0,-sin(a),cos(a),0,
                                                 0,0,0,1); 
                        break;
                    case 'y': rotation = Matx44d(cos(a),0,-sin(a),0,
                                                 0,1,0,0,
                                                 sin(a),0,cos(a),0,
                                                 0,0,0,1);
                        break;
                    case 'z': rotation = Matx44d(cos(a),sin(a),0,0,
                                                 -sin(a),cos(a),0,0,
                                                 0,0,1,0,
                                                 0,0,0,1);
                        break;
                    default : cout << "unknown axis: " << axis << endl; rotMat = Matx44d::eye();
                }
                rotMat = rotMat * rotation;
            }
            
            // panel translation happens before rotation
            //panelTrans = rotMat.inv() * panelTrans;
            
            // fix ugly floating point rounding at 0 crossings
            for (int y=0; y<3; y++)
                for (int x=0; x<3; x++)
                    if (abs(rotMat(y,x)) < 1e-7) rotMat(y,x) = 0.0;
            
            
            // create blank image and paste marker onto it 
            int sizeX = numX*markerSizePixel + (numX-1)*markerSpacingPixel;
            int sizeY = numY*markerSizePixel + (numY-1)*markerSpacingPixel;
            Mat panel = Mat(Size(sizeX, sizeY), CV_8UC3, CV_RGB(1,1,1));
            
            double sizeXmm = sizeX *  markerSize / markerSizePixel;
            double sizeYmm = sizeY *  markerSize / markerSizePixel;
            cout << "real panel size is " << sizeXmm << " x " << sizeYmm << " mm" << endl;
           
            
            int index = startPatternIndex;
            Mat marker;
            for (int y=0; y<numY; y++) {
                for (int x=0; x<numX; x++) {
                
                    // paste marker onto panel
                    Rect roi (x*(markerSizePixel+markerSpacingPixel), 
                              y*(markerSizePixel+markerSpacingPixel),
                              markerSizePixel, markerSizePixel);
                
                    marker = createMarker(marker, patts[index], markerSizePixel);
                    marker.copyTo(panel(roi));
                    
                    // calculate 4x4 marker transformation
                    //  panel-position transformation 
                    Matx44d subPanelTrans (1,0,0, ((double)x * (markerSize+markerSpacing) + markerSize/2.0 - (double)sizeXmm/2.0),
                                   0,1,0, -((double)y * (markerSize+markerSpacing) + markerSize/2.0 - (double)sizeYmm/2.0), 
                                   0,0,1,0,
                                   0,0,0,1);
                                   
                    Matx44d markerTrans =  panelTrans * rotMat * subPanelTrans;
                    
                    // write multi marker file line
                    of << outDir << "/marker_" << index << ".dat" << endl;
                    of << markerSize << endl;
                    of << "0 0" << endl;
                    of << markerTrans(0,0) << " " << markerTrans(0,1) << " " << markerTrans(0,2) << " " << markerTrans(0,3) << endl;
                    of << markerTrans(1,0) << " " << markerTrans(1,1) << " " << markerTrans(1,2) << " " << markerTrans(1,3) << endl;
                    of << markerTrans(2,0) << " " << markerTrans(2,1) << " " << markerTrans(2,2) << " " << markerTrans(2,3) << endl << endl;
                    
                    index++;
                }
                
            }
            cout << "dumping multi marker info to " << basename.str() << ".dat" << endl;
            of.close();
            
            // dump panel image
            cout << "dumping panel image to " << basename.str() << ".png" << endl;
            ss.str(string());
            ss << basename.str() << ".png";
            panel.convertTo(panel, -1, 255, 0);  
            imwrite(ss.str(), panel);
            
            
        }
        break;
    
    }
    
    
    cout << "done!" << endl;
    
    return 0;
}

