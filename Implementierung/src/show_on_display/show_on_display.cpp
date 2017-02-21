/**
  show_on_display - calculates and shows a HDR-sequence from a hdr input image. Same as the lightstage program, but without tracking and environment-mapping
  
  @author Manuel Jerger <nom@nomnom.de>
*/

#include "show_on_display.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "show_on_display";

/** 
 Prints usage.
*/
void help() 
{
    cout << "Shows images and environment maps on a display. Utilizes calibration data to achieve linear light output.\n" <<
        "Usage: \n" <<
         
        " " << PROGNAME << " --hdr_sequence <size> <fps> <in_image> <disp_params> [ outimg ] [ <ss> <ap> ] " << endl <<
        "     <size>               Number of HDR frame (range multiplier)." <<  endl <<
        "     <fps>                Frames per second." <<  endl <<
        "     <in_image>           The radiance map that should be shown; OpenCV compatible format (e.g. .exr) with values between 0 and 1" <<  endl <<
        "     <disp_params>        Display parameters, including response curve and patchconfig." <<  endl << 
        "     [out_image]           Dump equalized image istead of displaying it" << endl << endl;
}



bool captureThreadRunning = false;

/**
  remote DSLR control: start control thread
*/
void start_capture_thread(captureData data)
{
    captureThreadRunning = true;
    pthread_t thread;
    //cout << " {{{{{{ " << data.filename << endl;
    pthread_create (&thread, NULL, capture_thread, (void*) &data);
}

/**
  remote DSLR control: thread
*/
void* capture_thread (void *ptr)
{
    captureData* d = (captureData*) ptr;
    //play_sound(CAPTURE_START);
    
    cout << " >>> " << "CAPTURE BEGIN" << endl;
    // capture image
    remote_capture(d->exposure, d->aperture, d->filename);
    
    // exposure is complete if remote capture returns
    //play_sound(CAPTURE_END);
    cout << " >>> " << "END" << endl;
    captureThreadRunning = false;
    
    return NULL; // supresses warning.. 
}


/**
  calculate and show the hdr sequence
*/
int run_hdr_sequence (int argc, char *argv[]) 
{
 
    set_backlight (0.0);
    int size = atoi(argv[2]);
    double fps = atof(argv[3]);
    double factor = 1.0;
    if (size == 1) {
        factor = atof(argv[3]);
    }


    
    string infile = argv[4];
    
    string outfile;
    if (argc >= 7) {
       outfile = argv[6];
    }
 
    double ss=0;
    double ap=0;
    if (argc >= 9) {
        ss=atof(argv[7]);
        ap=atof(argv[8]);
        cout << "using shutterspeed " << ss << " and aperture " << ap << endl;
    }
    
    cout << "loading display configuration " << argv[5] << flush;
    FileStorage fs (argv[5], FileStorage::READ);
   
    // device screen size in pixels
    Size2d screenSize;          fs["screenSize"] >> screenSize;
    
    // width/height of vertical/horizontal border
    Size2d borderSize;          fs["borderSize"] >> borderSize;  
    
    // load SVR data
    SVRInfo svr; 
    
    // assert that response type is SVR
    string type;
    fs["type"] >> type;
    assert (strcasecmp(type.c_str(), "svr") == 0);
                     
    fs["borderSize"] >> svr.borderSize;         // width/height of vertical/horizontal border
    fs["screenSize"] >> svr.screenSize;         // screen size in pixels
    //fs["colorTransform"] >> svr.colorTransMat;  // color transformation matrix
    fs["patchLayout"] >> svr.patchLayout;       // number of patches in x / y direction
    fs["patchSize"] >> svr.patchSize;           // size of one square patch
    
    // check if patch layout and definitions are consistent
    if (not svr.checkValues()) {
        fs.release();
        return -1;
    }

    // load response curves
    svr.size = svr.patchLayout.width * svr.patchLayout.height;
    svr.vMin.resize(svr.size);
    svr.vMax.resize(svr.size);
    svr.response.resize(svr.size);
    
    for (int n=0; n<svr.size; n++) {
        stringstream ss;
        ss << "min_" << n;
        fs[ss.str().c_str()] >> svr.vMin[n];
        
        ss.str(string());
        ss << "max_" << n;
        fs[ss.str().c_str()] >> svr.vMax[n];
        
        ss.str(string());
        ss << "response_" << n;
        fs[ss.str().c_str()] >> svr.response[n];
        
        // stdout progress dots
        if (n % (svr.size/10) == 0) cout << "." << flush;
    }
    
    fs.release();
    cout << " done!" << endl;
    
    // screen size in pixel excluding border
    Size screenSizeNoBorder;
    screenSizeNoBorder.width = screenSize.width - 2*svr.borderSize.width;
    screenSizeNoBorder.height = screenSize.height - 2*svr.borderSize.height;
    
    // actually used screen region excluding border
    Rect screenRegion (borderSize.width, borderSize.height, screenSizeNoBorder.width, screenSizeNoBorder.height);
       
    Mat maxLight, minLight;
    
    // calculate maximum radiance we can produce at each pixel;
    cout << "calculating maximum and minimum radiance per display pixel ..." << flush;
    maxLight = Mat(screenSizeNoBorder, CV_32FC3, CV_RGB(1,1,1));
    get_min_max_screen(maxLight, svr, false);
    //imwrite("tmp/maxLight.exr", maxLight);

    minLight = Mat(screenSizeNoBorder, CV_32FC3, CV_RGB(0,0,0));
    get_min_max_screen(minLight, svr, true);
   // imwrite("tmp/minLight.exr", minLight);

    Mat maxScreenRadiance = maxLight - minLight;

   // imwrite("tmp/maxRange.exr", maxScreenRadiance);
    cout << " done!" << endl; 
    
    Mat screenRequired = Mat::zeros(screenSizeNoBorder, CV_32FC3);
    Mat tmp = imread (infile, -1);
    if (tmp.data== NULL) { 
       cout << " could not load file " << infile << endl;
       exit(-1);
    }
    tmp(screenRegion).copyTo (screenRequired);

    set_backlight(0); 
    // open window
    namedWindow("main",  CV_WINDOW_OPENGL);
    cvSetWindowProperty("main", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    Mat blackScreen = Mat::zeros(screenSize,CV_32FC3);
    // clear screen
    imshow("main", blackScreen); ;
    waitKey(100); // fullscreen window takes a little to show up
    sleep(1.0);
    
    
    // 3.2) calculate exposure multiplier, so that the required radiance completely fits inside the hdr sequence 
    //      and is thus displayed with the maxmimum possible dynamic range
    
    if (size == 0) {

        Mat tmp = Mat::zeros(screenSize, CV_32FC3);
        screenRequired.convertTo(screenRequired, -1, factor, 0.0);
        screenRequired += minLight;
        apply_response_svr (screenRequired, svr);
        //screenRequired(Rect(0,0,screenSizeNoBorder.width, screenSizeNoBorder.height)).copyTo(tmp(screenRegion));
        screenRequired.copyTo(tmp(screenRegion));
        set_backlight (1.0);
        imshow("main",tmp); 
        waitKey(0.0); 
        
        if (not outfile.empty()) {
           cout << "writing first frame to " << outfile << endl;
           imwrite (outfile,tmp);
        }
        
    } else {   
       
        vector<Mat> frames;
        for (int f=0; f<size; f++) {
            frames.push_back(Mat::zeros(screenSize, CV_32FC3));
        }
 
        // maximum required radiance
        double vmin[4], vmax[4];
        min_max(screenRequired, vmin, vmax);
        double maxRequired = vmax[3];
        cout << "maxRequired =" << maxRequired << endl;
        
        // maximum possible radiance we can produce that holds for all pixels
        
        tmp =  screenRequired/maxScreenRadiance;
        min_max(tmp, vmin, vmax);
        double scale =  size / vmax[3];
        cout << " scale is " << scale << endl;
        
        screenRequired.convertTo (screenRequired, -1, scale, 0.0);
        
        cout << " exposure multiplier is " << scale << endl;                         
        
    
    
        // apply response curve to pixels between min.. max; set everything else to 0 or 1
        Vec3f *reqPtr;
        Vec3f *maxPtr;
        Vec3f *minPtr;
        
        int idx=0;
        float val;
        for (int y=0; y<screenSizeNoBorder.height; y++) {
            
            reqPtr = screenRequired.ptr<Vec3f>(y);
            maxPtr = maxScreenRadiance.ptr<Vec3f>(y);
            minPtr = minLight.ptr<Vec3f>(y);
            
            for (int x=0; x<screenSizeNoBorder.width; x++) {
                for (int c=0; c<3; c++) {
                    val = reqPtr[x][c];
                    idx=0;
                    while (val > 0 && idx < size) {
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
        
        cout << "done " << endl;
        
        // pasting
        for (int f=0; f<size; f++) {
            Mat tmp = Mat::zeros(screenSize, CV_32FC3);
            
            frames[f](Rect(0,0,screenSizeNoBorder.width, screenSizeNoBorder.height)).copyTo(tmp(screenRegion));
            
            tmp.copyTo(frames[f]);
        } 
       
    
        
        set_backlight (1.0);
        
        // 3) show frames 
        
        if (ss > 0) { 
            start_capture_thread(captureData(outfile, ss, ap));
            sleep(1.0);
        }
        
        timespec tnow, tlast;
        timespec tfnow, tflast;
        clock(tlast);
        for (int i=0; i<size; i++) {
            clock(tflast);
            imshow("main", frames[i]);
            waitKey(10);
            clock(tfnow);
            sleep(1.0/fps-elapsed_ms(tflast,tfnow)/1000);
            cout << elapsed_ms(tflast,tfnow) << endl;
         }
        clock(tnow); 
        double elapsed = elapsed_ms(tlast,tnow);;
        cout << " took a total of " << elapsed << " ms" << endl;
        if (elapsed > 1.01 * (size / fps * 1000) ) {
            cout << " FAILED due to lag in HDR displaying routine (took " << elapsed << " ms instead of " <<  (size / fps * 1000)  << " ms " << endl;
        }
        
        imshow ("main", blackScreen);
        waitKey(10);
        if (ss > 0) { 
            while (captureThreadRunning) { sleep(0.1); }
        }
        
        set_backlight (0.0);
	    waitKey(100);

        if (ss == 0 && not outfile.empty()) {
           cout << "writing frames frame to " << outfile << endl;
           for (int i=0; i<frames.size(); i++) {
               stringstream ss; ss << outfile << "_" << i << ".png";
               frames[i].convertTo(frames[i], CV_8UC3, 255.0, 0);
	       imwrite (ss.str(),frames[i]);
           }
        }
      }        
    sleep(1.0);
    destroyWindow("main");
    cout << "done" << endl;

    set_backlight (1.0);
}


/**
  Main: calls run_hdr_sequence
*/
int main(int argc, char *argv[])
{
    cout << PROGNAME << " started" << endl;
    
    int ret = 0;
    
    if (argc < 2) {
        help();
        ret = -1;
    } else {
        if ( (strcmp( argv[1], "--hdr_sequence" ) == 0) && (argc >= 5)) {
            ret = run_hdr_sequence( argc, argv );
        } else {
            help();
            ret=-1;
        }
    }

    cout << PROGNAME << " finished" << endl;
    return ret;
}

