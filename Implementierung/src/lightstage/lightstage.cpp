/**
    lightstage : The core program. Hi-Res HDR lightstage using a portable display.
    
    @author Manuel Jerger <nom@nomnom.de>
*/

#include "lightstage.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "lightstage";

/**
  Prints usage.
*/
void help()
{
    cout << "Interactively reproduces a HDR environment map with a LDR display by tracking its position in relation to the object stage. " << endl << 
            "Image data is aquired by controlling a DSLR in time with the illumination. " << endl <<
        "Usage: \n" <<
        " " << PROGNAME << " <cam_device> <cam_params.yml> <disp_params.yml> <lightstage_params.yml> <output/path>" << endl <<
        "      <cam_device>              An integer to denote a /dev/video# device or a path to a video file." << endl <<
        "      <cam_params.yml>          Camera Matrix and Distortion coefficients (from calibrate_camera)" << endl <<
        "      <disp_params.yml>         Display parameters (from evaluate_display --svr)" << endl <<
        "      <lightstage_params.yml>   Light stage configuration file (most important parameters are here)" << endl <<
        "      <outpu/path>              Output directory for all runtime data including DSLR images." << endl << 
        "      [stagemode]               Program mode as string: show, single or hold." << endl << 
        "      <continueindex>           Continue previous unfinished illumination process from the specified index." << endl << endl;
        
}


extern string remoteCaptureCommand, soundNotificationCommand, backlightControlCommand;

//bool displayThreadRunning = false;
bool captureThreadRunning = false;

// remote DSLR control: separate control thread
void start_capture_thread(captureData data)
{
    captureThreadRunning = true;
    pthread_t thread;
    pthread_create (&thread, NULL, capture_thread, (void*) &data);
}

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
  main code : do the thing
*/
int run (int argc, char* argv[]) 
{
    // misc init code
    setNumThreads(4);
    
    // time init
    sw_start();

    // parse args
    if (argc < 6) {
        help();
        return -1;
    }
    
    // videodevice camconfig dispconfig stageconfig outputdir [mode] [continueindex]
    int camDeviceID = atoi(argv[1]);
    string camParamsFile   = argv[2];
    string dispParamsFile  = argv[3];
    string lightstageParamsFile  = argv[4];
    string outDir  = argv[5];
    enum STAGE_MODE { show, single, hold } stageMode;
    
    if (argc > 6) { 
        if (strcasecmp(argv[6],"single") == 0 ) {
            stageMode = single;
        } else if (strcasecmp(argv[6],"hold") == 0 ) {
            stageMode = hold;
        } else { 
            stageMode = show; 
        }
    } else {
        stageMode = show;
    }
    
    int continueIndex = -1;
    if (argc > 7) continueIndex = atoi(argv[7]);
    
    
    
    cout << PROGNAME  << " started with the following arguments:" << endl;
    stringstream ss; 
    for (int i=0; i<argc; i++) ss << " " << argv[i];
    cout << ss.str() << endl << endl;
    
    cout << "GPU support is " << flush;
    #ifdef USE_GPU
       cout << "enabled" << endl;
    #else
       cout << "disabled" << endl;
    #endif
    
    // open video device or file with OpenCV
    VideoCapture capt;
    
    bool haveLiveStream = false;

    capt.open(camDeviceID);
    haveLiveStream = true;
    
    if ( ! capt.isOpened() ) {
        cout << " could not open video device " << camDeviceID <<endl;
        return -1;
    }
    
    // input framebuffer
    Mat frame;
    capt >> frame;
    
    cout << "successfully openend video " << (haveLiveStream?"device":"file") << " " << camDeviceID << endl;
    
    cout << "video frame size is " << frame.size() << endl; 

    
    //
    // load display params
    //
    
    cout << "loading display configuration " << dispParamsFile << flush;
    FileStorage fs (dispParamsFile, FileStorage::READ);
    
    // exposure time used while calibrating
    double calibrationExposure; fs["exposureTime"] >> calibrationExposure; 
    
    // aperture used while calibrating
    double calibrationAperture; fs["aperture"] >> calibrationAperture; 
    
   
    // device screen size in pixels
    Size2d screenSize;          fs["screenSize"] >> screenSize;
    
    // device screen size in mm
    Size2d screenSizeMm;        fs["screenSizeReal"] >> screenSizeMm;
    
    // width/height of vertical/horizontal border
    Size2d borderSize;          fs["borderSize"] >> borderSize;  
    
    // translation vector: screen center in relation to camera, unit is mm
    Vec3d screenPosition;       fs["screenPosition"] >> screenPosition; 
    
    // load SVR data
    SVRInfo svr; 
    
    // assert that response type is SVR
    string type;
    fs["type"] >> type;
    assert (strcasecmp(type.c_str(), "svr") == 0);
                     
    fs["borderSize"] >> svr.borderSize;         // width/height of vertical/horizontal border
    fs["screenSize"] >> svr.screenSize;         // screen size in pixels
    fs["colorTransform"] >> svr.colorTransMat;  // color transformation matrix
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
    
    
    //
    // load lightstage configuration file
    //
    
    cout << "loading lightstage configuration " << lightstageParamsFile << " ..." << flush;
    fs = FileStorage (lightstageParamsFile, FileStorage::READ);
    

    string markerConfigFile;           fs["markerConfigFile"] >> markerConfigFile;
    int numMarkerRequired;             fs["numMarkerRequired"] >> numMarkerRequired;
    double stageRadius;                fs["stageRadius"] >> stageRadius;
    double stageRadiusTolerance;       fs["stageRadiusTolerance"] >> stageRadiusTolerance;
    double stageAngleTolerance;        fs["stageAngleTolerance"] >> stageAngleTolerance;
    double allowedDrift=10;            fs["allowedDrift"] >> allowedDrift;
    double stabilityTolerance=10;      fs["stabilityTolerance"] >> stabilityTolerance;
    Vec3d stageOrigin(0,0,0);          fs["stageOrigin"] >> stageOrigin;
    bool fixedCamera=false;            fs["fixedCamera"] >> fixedCamera;
    Mat cameraRotation;                fs["cameraRotation"] >> cameraRotation;
    Vec3d cameraPosition;              fs["cameraPosition"] >> cameraPosition;
    
    Size2i virtScreenSize;             fs["virtScreenSize"] >> virtScreenSize;
    Size2i borderRampSize;             fs["borderRampSize"] >> borderRampSize;
    int hdrSequenceSize;               fs["hdrSequenceSize"] >> hdrSequenceSize;
    double hdrSequenceFPS;             fs["hdrSequenceFPS"] >> hdrSequenceFPS;
    double hdrSequenceBlurSize=0;      fs["hdrSequenceBlurSize"] >> hdrSequenceBlurSize;
    double captureWaitTime;            fs["captureWaitTime"] >> captureWaitTime;
    double dslrExposure;               fs["dslrExposure"] >> dslrExposure; 
    double dslrAperture;               fs["dslrAperture"] >> dslrAperture; 
    bool useBlackframe;                fs["useBlackframe"] >> useBlackframe; 
    double blackframeExposure;         fs["blackframeExposure"] >> blackframeExposure; 
    bool useOverlapCheck = false;      fs["useOverlapCheck"] >> useOverlapCheck; 

    
    int trackingThreshold;             fs["trackingThreshold"] >> trackingThreshold;
    bool trackingUseInverted;          fs["trackingUseInverted"] >> trackingUseInverted;
    bool trackingUseColor;             fs["trackingUseColor"] >> trackingUseColor;
    
    string envMapFile;                 fs["envMapFile"] >> envMapFile;
    double envMapExposure;             fs["envMapExposure"] >> envMapExposure;
    double envMapBlurSize=0;           fs["envMapBlurSize"] >> envMapBlurSize;
    double envMapResize=1.0;           fs["envMapResize"] >> envMapResize;
    double radianceMultiplier;         fs["radianceMultiplier"] >> radianceMultiplier;
    bool useCosFactor=false;           fs["useCosFactor"] >> useCosFactor;
    bool useColorSpaceTransform;       fs["useColorSpaceTransform"] >> useColorSpaceTransform;
    bool useAntiShake=false;           fs["useAntiShake"] >> useAntiShake; 
     
    bool dumpTrackingImage=false;      fs["dumpTrackingImage"] >> dumpTrackingImage; 
    bool dumpTrackingLog=false;        fs["dumpTrackingLog"] >> dumpTrackingLog; 
    bool dumpScreen=false;             fs["dumpScreen"] >> dumpScreen; 
    bool dumpHDRFrames=false;          fs["dumpHDRFrames"] >> dumpHDRFrames; 
    bool dumpEnvMapUsed=false;         fs["dumpEnvMapUsed"] >> dumpEnvMapUsed; 
    bool dumpEnvMapRemaining=false;    fs["dumpEnvMapRemaining"] >> dumpEnvMapRemaining; 
    bool dumpEnvMapCompleted=false;    fs["dumpEnvMapCompleted"] >> dumpEnvMapCompleted; 
    
    bool isFirstRow=false;             fs["isFirstRow"] >> isFirstRow;
    bool useBottomLine=false;          fs["useBottomLine"] >> useBottomLine;
    
    
    fs["remoteCaptureCommand"] >> remoteCaptureCommand;
    fs["soundNotificationCommand"] >> soundNotificationCommand;
    fs["backlightControlCommand"] >> backlightControlCommand;
    
    
    fs.release();
    cout << " done!" << endl;
    
    //
    // setup remote DSLR camera connection
    //
    if (remote_setup() != 0) {
        cout << "Error: remote camera not responding" << endl;
        return -1;
    }
    
    const char* stageModeStr[3] = { "show", "single", "hold" };
    cout << "starting up lightstage in " << stageModeStr[stageMode] << " mode" << endl;
    
    //
    // scale svr config to new screen dimensions
    //
    
    cout << "borderSize is " << svr.borderSize << endl;
    
    Size2d displayScale(1.0, 1.0);
    if (svr.screenSize.width != virtScreenSize.width) {
        cout << "resizing display calibration data to virtual screen size ..." << flush;
        Size2d scale;
        scale.width = virtScreenSize.width / svr.screenSize.width;
        scale.height = virtScreenSize.height / svr.screenSize.height;
        if (scale.width != scale.height) {
            if (scale.width != scale.height) cout << "Warning: resolution ratio changes!" << endl;
        }   
        cout << "scale is " << scale << " " << flush;
        svr.rescale(scale.width);
        displayScale = scale;
    }
    
    
    // remove border from virtual screen size
    virtScreenSize.width -= 2*svr.borderSize.width;
    virtScreenSize.height -= 2*svr.borderSize.height;
    
    
    // remove border from realworld screen size
    screenSizeMm.width  -= screenSizeMm.width / svr.screenSize.width * 2*svr.borderSize.width;
    screenSizeMm.height -= screenSizeMm.height / svr.screenSize.height * 2*svr.borderSize.height;
    
    
    cout << " done!" << endl;
    
    cout << "active display area: " << endl;
    cout << "virtualScreenSize is " << virtScreenSize << endl;
    cout << "screenSizeMm is " << screenSizeMm << endl;
    
    // anti shake max shift
    Point2i allowedShift;
    // allow percentage of the ramp to be cropped by antishake shifting
    const double rampPercentage = 0.25; 
    allowedShift.x = borderSize.width  + rampPercentage / displayScale.width  * borderRampSize.width;
    allowedShift.y = borderSize.height + rampPercentage / displayScale.height * borderRampSize.height;
    cout << "allowed antiShake shift in pixels is " << allowedShift << endl;  

    //
    // load and preprocess environment map
    //
    
    Mat envMap = imread (envMapFile, CV_LOAD_IMAGE_UNCHANGED);
    if (envMap.data == NULL ) {
        cout << "Error: cannot load " << envMapFile << endl;
        return -1;
    }
    
    if (not ( (envMap.type() == CV_32FC3) || (envMap.type() == CV_64FC3)) ) {
        cout << "Warning: environment map " << envMapFile << " is not in 32 bit HDR format!" << endl;
        envMap.convertTo(envMap, CV_32FC3, 1.0/255.0, 0);
    }
    
    if (useColorSpaceTransform && svr.colorTransMat.data != NULL) {
        cout << "applying color transform to environment map ..." << flush;
        for (uint i=0; i<envMap.total(); i++) {
            Mat val(envMap.at<Vec3f>(i));
            val = Mat_<float>(svr.colorTransMat) * val;
            envMap.at<Vec3f>(i) = Vec3f(val);
        }
        cout << " done!" << endl;
    }
    
    
    // experiment: simulate different aperture (manual calculation if kernel sized required)
    // simple envmap blur   
    // TODO: the math, requires DLSR extrinsics
    if (envMapBlurSize > 0) {
        envMapBlurSize = (int)(envMapBlurSize * envMap.size().height/2.0) * 2 + 1;
        cout << "filtering input envmap with a gaussian of size " << envMapBlurSize << " ..." << flush;
        GaussianBlur(envMap, envMap, Size2d(envMapBlurSize,envMapBlurSize),envMapBlurSize);
        cout << " done" << endl;
    }
    
    // experiment: resize envmap
    if (envMapResize != 1.0) {
        cout << "resizing input envmap with scale of " << envMapResize << " ..." <<flush;
        Mat tmp;
        resize(envMap, tmp, Size2d(), envMapResize, envMapResize, (envMapResize>0)?INTER_LANCZOS4:INTER_CUBIC);
        tmp.copyTo(envMap);
        cout << " done" << endl;
    }
    
    //
    // init environment map object
    //
    CubeMap environment (envMap, svr, virtScreenSize, screenSizeMm, borderRampSize);

    // show mode: scale envmap so its displayable

    if (stageMode == show) {
        double vmin[4], vmax[4];
        
        min_max(environment.maxLight, vmin, vmax);
        double Dmax = vmax[3];
        
        min_max(environment.minLight, vmin, vmax);
        double Dmin = vmin[3];
        
        min_max(environment.envMapOriginal(Rect(0,0,5000,1000)), vmin, vmax);
        double Emax = vmax[3];
        double Emin = vmin[3];
        cout << Dmax << " " << Dmin << endl;
        cout << Emax << " "  << Emin << endl;
        environment.envMapOriginal.convertTo(environment.envMapOriginal, CV_32FC3, Dmax/Emax,Dmin-Emin);
    }
    //
    // try to continue from last run if requested
    // 
    if (continueIndex > -1) {
        stringstream ss; ss << outDir << "/envmap_completed/" << continueIndex << ".exr";
        Mat lastEnvMapCompleted = imread(ss.str(), CV_LOAD_IMAGE_UNCHANGED);
        if (lastEnvMapCompleted.data == NULL) {
            cout << "Error: could not continue from index " << continueIndex << " because " << ss.str() << " was not readable" << endl;
        } else {
            cout << "continuing from last state " << continueIndex << endl;
        
            // recreate last state
            
            #ifdef USE_GPU
                environment.envMapCompleted = gpu::GpuMat(lastEnvMapCompleted);
                gpu::GpuMat tmp;
                gpu::min (environment.envMapCompleted, 1.0, environment.envMapCompleted);
                gpu::multiply(environment.envMapOriginal, environment.envMapCompleted, tmp);
                gpu::subtract(environment.envMapRemaining, tmp, environment.envMapRemaining);
            #else
                lastEnvMapCompleted.copyTo(environment.envMapCompleted);    
                environment.envMapCompleted = min(environment.envMapCompleted, 1.0);
                environment.envMapCompleted = max(environment.envMapCompleted, 0.0);
                environment.envMapRemaining -= environment.envMapOriginal.mul(environment.envMapCompleted);
            #endif
        }
    }
    //
    // init ARToolKit Tracking
    //
    cout << "initializing ARToolKit tracking ..." << flush;
    Tracking tracking (capt, camParamsFile, markerConfigFile, trackingThreshold, !trackingUseColor, trackingUseInverted);
    tracking.setDebug(dumpTrackingImage);
    tracking.start();
    cout << " done!" << endl;
    

    //
    // text and image (video) logging stuff
    //
    
    ofstream logTracking;
    if (dumpTrackingLog) {
        stringstream ss; 
        ss << outDir << "/tracking.log";
        logTracking.open(ss.str().c_str(), std::fstream::out | std::fstream::app);
    }
    
    ofstream logExposures;
    {
        stringstream ss; 
        ss << outDir << "/exposures.log";
        logExposures.open(ss.str().c_str(), std::fstream::out | std::fstream::app);
    }
    
    sw_stop();
    cout << "init took " << sw_elapsed_ms() << " ms" << endl;
    

    
    //
    // main loop
    //

    ///////////////////////////////////////////////////////////////
    //
    //  ILLUMINATION PROCEDURE
    //
    //  while envmap > 0
    //    while no marker detected: show envmap on screen (uncompleted parts)
    //    while position not OK: ( idle sound )
    //    ( pos found sound )
    //    1. calculate required radiance:
    //       - do forward projection from cube envmap onto screen
    //       - remember which pixels of cube map were used while doing this
    //    2. calculate HDR frames:
    //        while required radiance of cube map > 0
    //          - do backward projection:
    //              - supersampling on remembered cube pixels
    //              - subtract per pixel: projected pixel radiance * display time 
    //          - store calculated HDR frame 
    //    2. in parallel: capture a dark frame (multiple second exposure, start/exposure end sound)
    //    3. illuminate
    //       - calculate time needed to show the frames
    //        ( start sound )
    //       - start exposure for required duration
    //       - show HDR slices with 1/(display time) FPS
    //        ( exposure end sound )
    //    4. dump various env map and screen images (used/remaining/required illumination)
    //    5. mark illuminated pixel for overlap checks 
    //  end while   
    //   ( finish sound )
    //    
    ///////////////////////////////////////////////////////////////
                

    // virtual screen buffer and output screen buffer
    Mat screen = Mat::zeros(virtScreenSize, CV_32FC3);
    Mat screenBuff = Mat::zeros(screenSize, CV_32FC3);
    Mat blackFrame = Mat::zeros(screenSize, CV_32FC3);
    Mat debugFrame;
    vector<Mat> hdrFrames;
    
    // for timing whole loop
    timespec tlast, tnow;   
    timespec tlast_hdr, tnow_hdr;   
    
    //displayThreadRunning = false;
    captureThreadRunning = false;
    
    // persistent camera orientation
    Matx34d transMat;       
    Matx33d rotMat;
    Matx31d camPos;
    
    // true if we currently have a valid position
    bool havePosition = false; 
    
    // debug envmap for showing the screen position
    Mat envMapScreenPos;
    
    // for OpenCV key processing
    int key;
    
    // backlight on
    set_backlight (1.0);
    
    // open window
    namedWindow("main",  CV_WINDOW_OPENGL);
    cvSetWindowProperty("main", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    
    // clear screen
    imshow("main", blackFrame);
    waitKey(500); // fullscreen window takes a little to show up
    
    
    
    // start notification
    play_sound (START);
    
    // display frame index
    long expcounter = (continueIndex >= 0 ? continueIndex+1 : 0);       
    
    // loop iteration index
    long loopidx = 0;
    bool running = true;
    bool debug = false;
    
    // loop-di-loop
    cout << "starting main loop" << endl;
    while (running) {
        
        
       // cout << "begin loop iteration " << loopidx << endl;
            
        // process keys
        key = waitKey(1) & 0xFF;
        running = (key != 27);
        //if (key == 82) { trackingThreshold = min(trackingThreshold+5,255); tracking.setThreshold(trackingThreshold); }  // up-arrow
        //if (key == 84) { trackingThreshold = max(trackingThreshold-5,0); tracking.setThreshold(trackingThreshold); }    // down-arrow
        //if (key == 't') { tracking.runAutoThreshold();}        
        //if (key == 'd') { debug = not debug; tracking.setDebug(debug || dumpTrackingImage);}        

        
        // for log
        double screenAngle;
        double trackingError;
        int trackingNumMarker=0;
        
        //
        //  aquire position      
        //
        
        // single mode with fixed position
        if (stageMode == single && fixedCamera) {
            cout << "using fixed camera position" << endl;
            rotMat = cameraRotation;
            camPos = cameraPosition;
            havePosition = true;
        
        // position from tracking (every cycle in sweep mode but onlye at start of exposure in single and hold mode)
        } else {
            
            // get tracking position
            if (tracking.hasNewData()) {
                tracking.lockThread();
                rotMat = tracking.getRotation();
                camPos = tracking.getPosition() - stageOrigin;
                trackingError = tracking.getError();
                trackingNumMarker = tracking.getNumMarker();
                if (dumpTrackingImage || debug ) debugFrame = tracking.getDebugImage();
                tracking.unlockThread();
                
                // only proceed if enough marker are visible
                if (tracking.getNumMarker() < numMarkerRequired) {
                    cout << expcounter << " not enough marker visible (" << tracking.getNumMarker() << ")" << endl;
                } else {
                    havePosition = true;
                    cout << expcounter << " new tracking pos thresh= "<< trackingThreshold <<" #marker= " << tracking.getNumMarker() << " err= " << tracking.getError() << " " << endl;
                }
                
            
            } else {
                // invalidate position if last position update happened too long ago
                if (tracking.lastTime() > 2000.0) { // two seconds
                    if (havePosition == true) cout << expcounter << " lost position due to timeout after " << tracking.lastTime() << " ms" << endl;
                    havePosition = false;
                }
            }
        }
        
        if (havePosition) {
            
            // right/down/forward vector
            Matx31d down = (rotMat.col(1));     // Y = down
            Matx31d right = (-rotMat.col(0));   // X = left
            
            // position of screen center in world coordinates
            Matx31d screenCenter = camPos + rotMat * Matx31d(screenPosition);
        
            // check if the current position is OK and start new exposure
            bool positionOK = true;
            if (stageMode != show) {
            
                // check quality of the position
                cout << expcounter << " checking quality of new position ... " << flush;
                    
             
                //
                // 0) require stable position
                //
                 
                // the 10 last positions  have to be within a 10 mm radius
                bool isStable = tracking.hasStablePosition(10, stabilityTolerance);
                if (not isStable) {
                    positionOK = false;
                    cout << "position is unstable!" << endl;
                 //   play_sound(WARNING);
                 //   sleep(0.5);
                }

                
                //
                // 1) check distance of screen center
                //
                
                if (positionOK) {
                    // too far away
                    double screenCenterDistance = norm (screenCenter, NORM_L2);
                    if (screenCenterDistance > stageRadius + stageRadiusTolerance) {
                        positionOK = false;
                        cout << " too far " << endl;
                        play_sound(CLOSER);
                        sleep(1.0);
                        
                    // too close
                    } else if (screenCenterDistance < stageRadius - stageRadiusTolerance) {
                        positionOK = false;
                        cout << " too close " << endl;
                        play_sound(AWAY);
                        sleep(1.0);
                    
                    }

                }

                //
                // 2) check maximum light angle with respect to screen normal
                //
                                                                               
                if (positionOK) {                                       
                    screenAngle = environment.get_max_angle(screenCenter, down, right); 
                    if (screenAngle > stageAngleTolerance) {
                        cout << " angle too steep (" << screenAngle << " degree)" << endl;
                        if (screenAngle < 2 ) {
                            play_sound(ONE);
                        } else if (screenAngle < 3) {
                            play_sound(TWO);
                        } else if (screenAngle < 4) {
                            play_sound(THREE);
                        } else if (screenAngle < 5) {
                            play_sound(FOUR);
                        } else if (screenAngle < 6) {
                            play_sound(FIVE);
                        } else if (screenAngle < 7) {
                            play_sound(SIX);
                        } else if (screenAngle < 8) {
                            play_sound(SEVEN);
                        } else if (screenAngle < 9) {
                            play_sound(EIGHT);
                        } else if (screenAngle < 10) {
                            play_sound(NINE);
                        } else if (screenAngle < 11) {
                            play_sound(TEN);
                        } else {
                            play_sound(POSITION);
                        }
                        sleep(1.0);
                        positionOK = false;
                    }
                }

                
                
                //
                // 3) check overlap
                //
                //const double minCovered = 0.1;
                const double maxCovered = 0.50;
                 
                const double borderLength = 0.5; // factor
                
                // exclude step in the first frame, because there is no overlap to check
                if (useOverlapCheck && positionOK && expcounter > 0) {
                
                   // project  overlap envmap onto screen
                   Mat tmpScreen = Mat::zeros (virtScreenSize, CV_32FC3);
                   environment.project_forward(tmpScreen, environment.envMapCompleted, screenCenter, down, right);
                   //imwrite("tmp/mask_projected.exr",tmpScreen);
                   
                   
                   int topLine=0;
                   if (useBottomLine) topLine=virtScreenSize.height-1;
                   
                   // check top row of pixels
                   double sumTop = 0;
                   for (int i=0; i<virtScreenSize.width; i++) {
                      sumTop += tmpScreen.at<Vec3f>(topLine, i)[0];
                   }
                   bool hasTop = (sumTop >= virtScreenSize.width - 10);  // subtract 10, just to be sure (its late...)
                  cout << "hasTop = " << hasTop << endl;
                  cout << "isFirstRow = " << isFirstRow << endl;
                   
                   //left/right : check only fixed percentage
                   double sumLeft = 0, sumRight = 0;
                   int offset = virtScreenSize.height* (1.0-borderLength)/2;
                   for (int i=offset; i<virtScreenSize.height-offset; i++) {
                      sumLeft += tmpScreen.at<Vec3f>(i, 0)[0];
                      sumRight += tmpScreen.at<Vec3f>(i, virtScreenSize.width-1)[0];
                   }
                   bool hasRight = (sumRight >= virtScreenSize.height * borderLength - 10);  // subtract 10, just to be sure (its late...)
                   bool hasLeft = (sumLeft >= virtScreenSize.height * borderLength - 10);    // subtract 10, just to be sure (its late...)
                    
                    
                    // if this is not the first illumination row (set in lighstage config), top MUST overlap, to avoid horizontal gaps 
                    if (!isFirstRow && !hasTop) {
                       play_sound(TOP);
                       positionOK = false;
                       sleep(1.0);
                    // either left or right MUST overlap, to avoid vertical gaps
                    } else  if (!hasRight && !hasLeft) {                         
                       play_sound(MORE);
                       positionOK = false;
                       sleep(1.0);
                    }
                    
                   // aditionally, check if number of overlapping pixels (approx.) is large enough 
                   if (positionOK) { 
                   
                       Scalar elemSum = sum(tmpScreen); 
                       double covered = (elemSum[0] +  elemSum[1] + elemSum[2] ) / 3.0 / (virtScreenSize.width * virtScreenSize.height);
                   
                   
                   cout << expcounter << " approx. percentage of pixels is " << 1.0 - covered << "  ( sum is " << elemSum << " ) " << endl;
                      //if (covered < minCovered) { 
                      //     cout << "not enough overlap" << endl;
                      //     positionOK = false;
                      //     play_sound(MORE);
                      //     sleep (0.5);
                      //
                      if (covered > maxCovered) {
                           cout << "too much overlap" << endl;
                           positionOK = false;
                           play_sound(LESS);
                           sleep (0.5);
                      
                      }
                  }
                   
                   // pressing the enterkey overrides position check
                   if (not positionOK) {
                       if ( (waitKey(1) & 0xFF) == '\n' )  {
                           positionOK=true;
			  sleep (0.5);
                       }
                   }
                   
                }
                
                
                
                //project current screen position onto debug envmap
                envMapScreenPos = Mat::zeros (envMap.size(), CV_32FC3); 
                environment.project_backward(envMapScreenPos, environment.borderRampMask,screenCenter, down, right);
                    
            }       
                
        
            
            if (positionOK) {
            
                cout << " is OK!" << endl;
                    
                // right/down/forward vector
                Matx31d down = (rotMat.col(1));     // Y = down
                Matx31d right = (-rotMat.col(0));   // X = left
                
                // position of screen center in world coordinates
                Matx31d screenCenter = camPos + rotMat * Matx31d(screenPosition);
                           
                //             
                // SHOW mode: simply show environment map
                //
                if (stageMode == show) {
                    cout << expcounter << " displaying env map" << endl;
                
                    sw_start();
                    screen = environment.show_environment (screenCenter, down, right);
                    apply_response_svr(screen, svr);
                    sw_stop();
                    cout <<  expcounter << " sampling took " <<  sw_elapsed_ms () << " ms" << endl; 
                    
                    // show on screen
                    resize(screen, screen, screenSizeNoBorder);
                    screen.copyTo(screenBuff(screenRegion));
                    
                    imshow ("main", screenBuff);
                    waitKey(1);
                    
                    
                
                // 
                // HDR mode
                //
                } else {
                
                    //set_backlight(1.0);
                    imshow("main", blackFrame);
                    waitKey(1);
                
                    //
                    // start darkframe exposure in concurrent thread
                    //
                
                    if (useBlackframe) {
                    
                        // delay to assure blackscreen is showing before the shutter opens
                        sleep(captureWaitTime);
                    
                        // start capture 
                        stringstream ssDf; ssDf << outDir << "/result/" << expcounter << "_df.cr2";
                        start_capture_thread(captureData(ssDf.str(), blackframeExposure, dslrAperture));
                    }
                    
                    //
                    // calculate HDR frames
                    //
                
                    cout << expcounter << " calculating " << hdrSequenceSize << " HDR frames " << endl;
                    play_sound(PROC_START);
                    
                    clock(tlast);
                    
                    sw_start();
                                 
                    // allocate HDR frame storage
                    hdrFrames.clear();
                    for (int f=0; f<hdrSequenceSize; f++) {
                        hdrFrames.push_back(Mat::zeros(screenSize, CV_32FC3));
                    }
                    sw_stop();
                    cout <<  expcounter << " frame zeroing took " <<  sw_elapsed_ms () << " ms" << endl; 
              
                    
                    // required factor for relating env map to one frame of display light
                    
                    double expFactor = environment.calc_hdr_frames(hdrFrames, screenCenter, down, right, screenSizeNoBorder, borderSize, hdrSequenceSize, radianceMultiplier, useCosFactor, hdrSequenceBlurSize);                

                    clock(tnow);
                    cout <<  expcounter << " frame calculation took " <<  elapsed_ms (tlast, tnow) << " ms" << endl; 
              
                    // enable upscaling here (otherwise the opencv opengl window does it for us)
                    // NOT_IMPLEMENTED
                    
              
                    // wait for darkframe exposure to end
                    while (captureThreadRunning) sleep_nano(1000);
                    

                    //
                    // start exposure
                    //
                     
                    // wait  time at start and end of sequence display ("center" the hdr slices in the middle of the exposure)
                    //double captureWaitTimeAfter = (dslrExposure - (double)hdrSequenceSize / (double)hdrSequenceFPS) / 2.0;
                    
                    { 
                      stringstream ssRes;
                      ssRes << outDir << "/result/" << expcounter << ".cr2";
                      start_capture_thread(captureData(ssRes.str(), dslrExposure, dslrAperture));
                    } 
                    
                    
                    // delay to assure shutter is open
                    sleep(captureWaitTime);
                    
                    
                    //
                    // display hdr frames
                    //
                    
                    // if the illumination has failed
                    bool failure = false;
                    
                    
                    // for anti-shake: image shift in pixels
                    Point2i shakeShift(borderSize);
                    
                    Matx33d newRotMat;
                    Matx31d newCamPos;
                    double newTrackingError;
                    int newTrackingNumMarker;
                    Matx31d newScreenCenter, newDown, newRight;
                    
                    
                    
                    // first frame is pasted onto screen buffer here; all others are processed while displaying the previous frame
                    blackFrame.copyTo(screenBuff);
                    if (tracking.hasNewData()) {
                    
                        //
                        // CODE COPIED FROM INNER LOOP
                        //
                        // get current screen position
                        tracking.lockThread();
                        newRotMat = tracking.getRotation();
                        newCamPos = tracking.getPosition() - stageOrigin;;
                        newTrackingError = tracking.getError();
                        newTrackingNumMarker = tracking.getNumMarker();
                        tracking.unlockThread();
                        
                        newScreenCenter = newCamPos + newRotMat * Matx31d(screenPosition); 
                        newDown = (newRotMat.col(1));     // Y = down
                        newRight = (-newRotMat.col(0));   // X = left               
                        if (useAntiShake) {  
                           shakeShift = get_shakeshift(screenCenter, newScreenCenter, newDown, newRight, screenSizeMm, screenSizeNoBorder) + Point2i(borderSize);
                         // too large: position error;
                          if (abs(shakeShift.x) > allowedShift.x || abs(shakeShift.y) > allowedShift.y ) {
                              cout << expcounter << " Error: shift too large for antiShake ("<< shakeShift<<"); aborting." << endl;
                              failure = true;
                          }
                          
                        // no antishake: check drift
                        } else {
                            // check position again: L2 distance between screenCenter position at beginning, and at end
                            if ( norm(screenCenter, newScreenCenter) > allowedDrift) {  // 10 mm (may be too little)
                                failure=true;
                                play_sound(ERROR);
                                cout << expcounter << " FAILED due to movement of user (distance of screencenter has moved " << norm(screenCenter, newScreenCenter) << " mm)!" << endl;
                            } 
                        }
 
                        int f=0;
                        double newScreenAngle = environment.get_max_angle(newScreenCenter, newDown, newRight);
                        logTracking << expcounter << " AntiShake frame " << f << " shift ( " << shakeShift.x << " " << shakeShift.y << " ) " 
                            << "err = " << newTrackingError << " m = " << newTrackingNumMarker << " "
                            << "pos_pher ( " << cart2spher(newCamPos)(0) << " " << cart2spher(newCamPos)(1) << " " << cart2spher(newCamPos)(2) << " ) "
                            << "pos_cart ( " << newCamPos(0) << " " << newCamPos(1) << " " << newCamPos(2) << " ) " 
                            << "fw ( " << newScreenCenter(0) << " " << newScreenCenter(1) << " " << newScreenCenter(2) << " ) "
                            << "down ( " << newDown(0) << " " << newDown(1) << " " << newDown(2) << " ) "
                            << "right ( " << newRight(0) << " " << newRight(1) << " " << newRight(2) << " ) " 
                            << "angle = " << newScreenAngle << " " << endl;
                            
                       
                    }
                    cout << hdrFrames[0].size() << endl;
                    
                    // NOTE: copied from loop
                    
                    Rect availableRegion (0,0,screenBuff.size().width, screenBuff.size().height);
                    Rect targetRegion = availableRegion+shakeShift;
                    Rect intersection = targetRegion & availableRegion;
                    // copy to framebuffer 
                    hdrFrames[0](intersection - shakeShift ).copyTo( screenBuff( intersection ) );
                        
                    
                    play_sound(PROC_START);
                    double tookAvg = 0;
                    cout << expcounter << " displaying " << hdrFrames.size() << " HDR frames ... " << endl;
                    
                        clock(tlast_hdr);
                    
                    for (uint f=0; f<hdrFrames.size(); f++) {
                    
                        sw_start();
                        
                        // 1) display frame on screen
                        imshow("main", screenBuff);
                        waitKey(1);
                        
                        // for all frames except the last one: calculate next frame
                        if (f != hdrFrames.size()-1) {
                        
                            // 2) get new tracking position, process next frame 
                            if (tracking.hasNewData()) { 
                            
                                // get current screen position
                                tracking.lockThread();
                                newRotMat = tracking.getRotation();
                                newCamPos = tracking.getPosition() - stageOrigin;;
                                newTrackingError = tracking.getError();
                                newTrackingNumMarker = tracking.getNumMarker();
                                tracking.unlockThread();
                                
                                newScreenCenter = newCamPos + newRotMat * Matx31d(screenPosition); 
                                newDown = (newRotMat.col(1));     // Y = down
                                newRight = (-newRotMat.col(0));   // X = left               
                                
                                
                                // log tracking stuff
                                if (dumpTrackingLog) {
                                    
                                    double newScreenAngle = environment.get_max_angle(newScreenCenter, newDown, newRight);
                                    logTracking << expcounter << " Frame " << f << " shift ( " << shakeShift.x << " " << shakeShift.y << " ) " 
                                        << "err = " << newTrackingError << " m = " << newTrackingNumMarker << " "
                                        << "pos_pher ( " << cart2spher(newCamPos)(0) << " " << cart2spher(newCamPos)(1) << " " << cart2spher(newCamPos)(2) << " ) "
                                        << "pos_cart ( " << newCamPos(0) << " " << newCamPos(1) << " " << newCamPos(2) << " ) " 
                                        << "fw ( " << newScreenCenter(0) << " " << newScreenCenter(1) << " " << newScreenCenter(2) << " ) "
                                        << "down ( " << newDown(0) << " " << newDown(1) << " " << newDown(2) << " ) "
                                        << "right ( " << newRight(0) << " " << newRight(1) << " " << newRight(2) << " ) " 
                                        << "angle = " << newScreenAngle << " " << endl;
                               }
                                
                                // calc shake shift if enabled
                                if (useAntiShake) {          
                                    // calculate shake shift
                                    shakeShift = get_shakeshift(screenCenter, newScreenCenter, newDown, newRight, screenSizeMm, screenSizeNoBorder) + Point2i(borderSize);
                              
                                    // too large: position error;
                                    if (abs(shakeShift.x) > allowedShift.x || abs(shakeShift.y) > allowedShift.y ) {
                                        cout << " Error: shift too large for antiShake ("<< shakeShift<<"); aborting." << endl;
                                        failure = true;
                                        break;
                                    }
                                }
                                
                                
                            }
                            
                        }
                        
                        
                        // clear frame
                        blackFrame.copyTo(screenBuff);
                        
                        //calculate required image position and crop rectangle;
                        Rect availableRegion (0,0,screenBuff.size().width, screenBuff.size().height);
                        Rect targetRegion = availableRegion + shakeShift ;
                        Rect intersection = targetRegion & availableRegion;
                        
                        
                        // copy to framebuffer 
                        hdrFrames[f](intersection - shakeShift ).copyTo( screenBuff( intersection ) );
                        
                        sw_stop();
                        
                        // 3) wait the rest of the required time to achieve the desired FPS
                        double took = sw_elapsed_ms();
                        cout << " took = " << took << endl;
                        tookAvg += took;
                        sleep(1.0/(double)hdrSequenceFPS - took/1000.0);
                    }
                    clock(tnow_hdr);
                    
                    imshow("main", blackFrame);
                    waitKey(1);
                    
                    cout << "took at average " << tookAvg / hdrFrames.size() << " ms (" << 1.0/(tookAvg/1000.0 / hdrFrames.size()) << " max FPS)"<< endl;
                    double elapsed=elapsed_ms (tlast_hdr, tnow_hdr);
                    cout << "took a total of " << elapsed << " ms"<< endl;
                    if (elapsed > 1.05 * (hdrSequenceSize / hdrSequenceFPS * 1000) ) {
                        failure=true;
                        cout << expcounter << " FAILED due to lag in HDR displaying routine (took " << elapsed << " ms instead of " <<  (hdrSequenceSize / hdrSequenceFPS * 1000)  << " ms " << endl;
                    }
                    
                    
                    // check position again: L2 distance between screenCenter position at beginning, and at end
                    if ( norm(screenCenter, newScreenCenter) > allowedDrift) {  // 10 mm (may be too little)
                        failure=true;
                        cout << expcounter << " FAILED due to movement of user (distance of screencenter has moved " << norm(screenCenter, newScreenCenter) << " mm)!" << endl;
                    }
                    
                    
                    //
                    // wait for gphoto2 call to end (includes file transfer via usb)
                    //
                    
                    while (captureThreadRunning) sleep_nano(1000);
                   
 
		            // ESC key aborts current illumination
	                if (!failure && ( (waitKey(1) & 0xFF) == 27) )  {
                       cout << expcounter << " USER ABORTED " << endl;
                       failure=true;   
                    }
                
                    if (failure) {
                        play_sound(ERROR);
                        cout << expcounter << " ERROR: illumination failed" << endl;
                        sleep(1.5);
                        continue;
                    } else {
                        play_sound(PROC_END);
                    }
                    
                    
                    //
                    // we were sucessfull: subtract illumination from remaining env map
                    //
                    
                    sw_start();

                    #ifdef USE_GPU
                      gpu::GpuMat tmp;
                      gpu::multiply (environment.envMapUsed, environment.envMapRemaining, tmp);
                      gpu::subtract(environment.envMapRemaining, tmp, environment.envMapRemaining);
                      gpu::add(environment.envMapUsed, environment.envMapCompleted, environment.envMapCompleted);
                      gpu::min (environment.envMapCompleted, 1.0, environment.envMapCompleted);
                      //gpu::max (environment.envMapCompleted, 0.0, environment.envMapCompleted);
                    #else
                      environment.envMapRemaining -= environment.envMapUsed.mul(environment.envMapRemaining);
                      
                      // dump projected mask
                      Mat tmp (virtScreenSize, CV_32FC3);
                      environment.envMapCompleted += environment.envMapUsed;
                      environment.envMapCompleted = min(environment.envMapCompleted, 1.0);
                      
                    #endif

                    //
                    // log / dump and debug stuff
                    //
                    if (dumpTrackingLog) {
                        logTracking << expcounter << " err = " << trackingError << " m = " << trackingNumMarker << " "
                                    << "pos_pher ( " << cart2spher(camPos)(0) << " " << cart2spher(camPos)(1) << " " << cart2spher(camPos)(2) << " ) "
                                    << "pos_cart ( " << camPos(0) << " " << camPos(1) << " " << camPos(2) << " ) " 
                                    << "fw ( " << screenCenter(0) << " " << screenCenter(1) << " " << screenCenter(2) << " ) "
                                    << "down ( " << down(0) << " " << down(1) << " " << down(2) << " ) "
                                    << "right ( " << right(0) << " " << right(1) << " " << right(2) << " ) " 
                                    << "angle = " << screenAngle << " " << endl;
                    }
                    
                    logExposures << expcounter << " " << expFactor << endl;
                
                    
                    if (dumpScreen || dumpTrackingImage || dumpEnvMapRemaining || dumpEnvMapUsed || dumpEnvMapCompleted || dumpHDRFrames ) {
                        sw_start();
                        cout << expcounter << " dumping runtime image data ... " << flush;
                        
                        // write out debug images
                        
                        if (dumpScreen) {
                            stringstream ss; ss << outDir << "/screen/" << expcounter << ".exr";
                            imwrite(ss.str(), environment.screenRequired);
                        }
                        
                        if (dumpHDRFrames) {
                            for (int i=0; i<hdrSequenceSize; i++) {
                                hdrFrames[i].convertTo(hdrFrames[i], CV_8UC3, 255.0, 0);
                                stringstream ss; ss << outDir << "/screen/frame_" << i << ".bmp";
                                imwrite (ss.str(),hdrFrames[i]);
                            }
                        }
                            
                        if (dumpTrackingImage) {
                            stringstream ss; ss << outDir << "/tracking/" << expcounter << ".jpg";
                            imwrite(ss.str(), debugFrame);
                        }    
                        
                        if (dumpEnvMapRemaining) {
                            stringstream ss; ss << outDir << "/envmap_remaining/" << expcounter << ".exr";
                            imwrite(ss.str(), environment.envMapRemaining);
                        }
                        
                        if (dumpEnvMapUsed) {
                            stringstream ss; ss << outDir << "/envmap_used/" << expcounter << ".exr";
                            imwrite(ss.str(), environment.envMapUsed);
                        }
                        
                        if (dumpEnvMapCompleted) {
                            stringstream ss; ss << outDir << "/envmap_completed/" << expcounter << ".exr";
                            imwrite(ss.str(), environment.envMapCompleted);
                        }
                        
                        
                        cout << " done!" << endl;
                    
                        sw_stop();
                        cout << expcounter << " dumping took " << sw_elapsed_ms() << " ms" << endl;
                    }
                    
                    
                    // in single mode we exit after the first frame
                    if (stageMode == single) break;
                    
                    sw_stop();
                    double took = sw_elapsed_ms();
                    cout << expcounter << " postprocessing took " << took << " ms" << endl ;
                    sleep(2-took/1000.0);
                    
                    // running frame index (for logs and such)
                    expcounter++;
                
                
                }
                    
                
                
            } 
     
        
        } // end havePosition
       
        else { 
            // we have no position: output debug image or envmaps
            if (stageMode != show) {
                Mat tmp;
                int height = screenBuff.size().height / 3.0;
                // original
                if (environment.envMapOriginal.data != NULL ) {
                    resize(environment.envMapOriginal + envMapScreenPos, tmp, Size(screenBuff.size().width, height));
                    tmp.copyTo(screenBuff(Rect(0,0,screenBuff.size().width, height)));
                }
                // remaining
                if (environment.envMapRemaining.data != NULL ) {
                    resize(environment.envMapRemaining, tmp, Size(screenBuff.size().width, height));
                    tmp.copyTo(screenBuff(Rect(0,height,screenBuff.size().width, height)));
                }
                // completed regions
                if (environment.envMapCompleted.data != NULL ) {
                   envMapScreenPos.convertTo(envMapScreenPos, -1, 0.5, 0);
                   resize(environment.envMapCompleted+envMapScreenPos, tmp, Size(screenBuff.size().width, height));
                   tmp.copyTo(screenBuff(Rect(0,height*2,screenBuff.size().width, height)));
                }
                imshow("main", screenBuff);
                waitKey(1);
                // play idle sound (roughly evey 5 idle loops)
                if (loopidx % 5 == 0) play_sound(SEARCH);
                
                
                // 5 updates / sec in idle
                sleep(0.2);
            }
         }
        
        loopidx++;
      
    } // end main loop

    cout << "illumination finished with " << expcounter << " exposures and " << loopidx << " loop iterations" << endl;
    play_sound(FINISH);
    //set_backlight(0.5);
    
    
    //
    // cleanup
    //
    destroyWindow("main");
    tracking.stop();
    sleep (0.5);
    capt.release();
    if (dumpTrackingLog) logTracking.close();
    logExposures.close();
    
    
    return 0;    
}


/**
    calculate shift vector for shake compensation experiment.
    Does not fit anywhere but is needed multiple times for 'unrolling' the rendering loop.
*/
Point2i get_shakeshift (Matx31d& screenCenter, Matx31d& newScreenCenter, Matx31d& newDown, Matx31d& newRight, Size2d& screenSizeMm, Size2i& screenSizeNoBorder)
{
    // get new image translation
    // 1) intersect old forward vector (= requiredScreenCenter) with new image plane
    Matx33d A;
    for (int i=0; i<3; i++ ) {
        A(i,0) = newScreenCenter(i);
        A(i,1) = newDown(i);
        A(i,2) = newRight(i);
    } 
    
    // 2) solve for down and right component
    Matx31d res = A.inv() * screenCenter;
    
    // 3) convert mm into pixels
    Point2i shakeShift;
    shakeShift.x = ( res(2) / screenSizeMm.width / 2 ) * screenSizeNoBorder.width;
    shakeShift.y = ( res(1) / screenSizeMm.height/ 2) * screenSizeNoBorder.height;
    
    return shakeShift;
}


int main (int argc, char* argv[])
{

    return run (argc, argv);

}
 


