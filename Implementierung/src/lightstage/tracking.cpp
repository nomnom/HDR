// camera tracking with ARToolKit

#include "tracking.h" 
 
using namespace std;
using namespace cv;

// webcam frame size
#ifdef __APPLE__
  #define VIDEO_WIDTH  1280
  #define VIDEO_HEIGHT  720
#else
  #define VIDEO_WIDTH  640
  #define VIDEO_HEIGHT 480
#endif

 
// constructor
Tracking::Tracking (VideoCapture& camCapture, 
                    string camParamsFile, 
                    string markerConfigFile,
                    int threshold, 
                    bool greyscale,
                    bool inverted)
 :capt(camCapture),
  lock(false),
  readerLock(false),
  useInverted(inverted),
  useGreyscale(greyscale), 
  thresh(threshold), 
  err(0), 
  hasNew(false), 
  debug(true)//,
  //stageTransMat(stageTransMat)
{
    cout << "initializing trackin class" << endl;
    cout << "using " << (useInverted?"inverted ":"") << (useGreyscale?"greyscale":"color") << " image";
    // read first frame for size
    capt >> frame;
    clock(tlast);
    if (not (frame.size().width == VIDEO_WIDTH && frame.size().height == VIDEO_HEIGHT)) {
        cout << "error: video frame size of " << VIDEO_WIDTH << " x " << VIDEO_HEIGHT << " does not match the video stream of " << frame.size().width << " x " << frame.size().height << endl;
        exit(-1);
    }
    
    //
    // OpenCV camera stuff
    //
    
    // load camera parameters
    cout << "loading camera parameters " << camParamsFile << endl;
    FileStorage fs(camParamsFile, FileStorage::READ);
    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    fs.release();
    
    // calculate undistort maps
    initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, frame.size(), CV_16SC2, map1, map2);

    cout << "compilation time video frame size is " << VIDEO_WIDTH << " " << VIDEO_HEIGHT << endl;
    
    // calculate FOV
    double fov_theta_x = 2 * atan2( (VIDEO_WIDTH / 2),  cameraMatrix.at<double>(0,0)) / M_PI * 180.0;
    double fov_theta_y = 2 * atan2( (VIDEO_HEIGHT / 2), cameraMatrix.at<double>(1,1)) / M_PI * 180.0;
    cout << "tracking cam FOV is " << fov_theta_x << " x " << fov_theta_y << " degree" << endl;
    
    //
    // set up ARToolKit camera parameters
    //
    
    cameraParam.xsize=VIDEO_WIDTH;
    cameraParam.ysize=VIDEO_HEIGHT;
    
    
    // convert OpenCV camera matrix to artoolkit array
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            cameraParam.mat[i][j] = cameraMatrix.at<double>(i,j); 
        }
        // is 3x4 array: 0,0,0 tralation
        cameraParam.mat[i][3] = 0;
    }
    
    // 'identity' distortion factor (has no effect)
    cameraParam.dist_factor[0] = 0;
    cameraParam.dist_factor[1] = 0;
    cameraParam.dist_factor[2] = 0;
    cameraParam.dist_factor[3] = 1;
 
    arInitCparam(&cameraParam);
        
    cout << "ARToolKit image size is " << arImXsize << " " << arImYsize << endl;
    //
    // load multi AR marker file
    //
    if( (config = arMultiReadConfigFile(markerConfigFile.c_str())) == NULL  ) {
        cout << "Error while loading multi AR marker config " << markerConfigFile  << endl;
        capt.release();
    }
 
 }

// destructor
Tracking::~Tracking ()
{
    stop();
}

// find best threshold
// return true if a new best threshold was found
bool Tracking::runAutoThreshold()
{
    double minErr = 1E10;
    int bestThresh = -1;
    
    readerLock = true;
    
    int numMarkersUsed;
    for (int t=0; t<256; t++) {
        arDetectMarker((ARUint8 *)frame.data, t, &markerInfo, &markerNum);
        double err = arMultiGetTransMat(markerInfo, markerNum, config);
        
        numMarkersUsed = 0;
        for (int i=0; i<config->marker_num; i++ ) {
            if (config->marker[i].visible != -1) numMarkersUsed++;
        }
        
        if (numMarkersUsed < 4 ) continue;
        
        err /= (float)numMarkersUsed;
        
        if (err >= 0 && err < minErr ) { 
            minErr = err;
            bestThresh = t;
            cout << " thresh=" << thresh << "  err = " << err << endl;
        }
        
    }
    readerLock = false;
    if (bestThresh == -1) { 
        cout << "Autothreshold found nothing" << endl; 
        return false;
    } else {
        thresh = bestThresh;
        cout << "new threshold: " << thresh << endl;
        return true;
    }
    
        
}


// start/stop tracking thread
void Tracking::start()
{
    running = true;
    lock = false;
    pthread_t thread;
    pthread_create (&thread, NULL, trackingLoop, this);
        
}
void Tracking::stop()
{
    running = false;
}

// infinite loop: grab frame, detect marker, calculate position
// only limited by camera input framerate (waits for next frame delivered by opencv)
void* Tracking::trackingLoop(void *ptr)
{
    cout << "tracking loop started" << endl;
    Tracking* t = reinterpret_cast<Tracking*>(ptr);
    while (t->running) {
    
        // grab next frame ( waits until new frame is available, sets lock=true after image was aquired )
        t->grab();
        
        // wait until shared ressources are no longer accessed
        while (t->readerLock) {} 
        
        // detect marker and calculate position
        t->hasNew = t->detect();
        
        // create tracking debug image
        if (t->debug) t->createDebugImage();
    
        t->lock = false;
        
    }
    
    cout << "tracking loop ended" << endl;
    return NULL; // supress warning
}

// grab next frame, undistort and apply greyscale and value inversion
bool Tracking::grab() 
{
    if (!capt.grab()) {
        cout << "Error capturing video frame" << endl;
        return false;
    }
    lock = true;
    
    capt.retrieve(frame);
    
    
            
    // undistort image
    remap(frame,frame, map1, map2, INTER_LINEAR);

    // POSSIBLE SPEEDUP: use single-channel images with artoolkit
    // convert to grey scale
    if (useGreyscale) {
        Mat grey; cvtColor(frame,grey,CV_BGR2GRAY);
        Mat gchannels[3] = {grey,grey,grey};
        merge(gchannels, 3, frame);
    }

    // POSSIBLE SPEEDUP: inplace inversion
    if (useInverted) {
        frame = Mat(frame.size(), CV_8UC3, CV_RGB(255,255,255)) - frame;
    }    
    
    return true;
}

double Tracking::lastTime() 
{
    timespec tnow;
    clock(tnow);
    return elapsed_ms(tlast, tnow);
}


bool Tracking::hasNewData()
{
   // while (lock) {} ; 
    if (hasNew) { 
        hasNew = false; 
        return true;
    }
    return false;
} 

bool Tracking::hasStablePosition (int n, double maxDist)
{
    if (n > (int)lastPositions.size()) return false; // not enough last positions
    
    // naive: check distance between all pairs of the n last positions
    for (int i=0; i<n; i++) {
        for (int j=0; j<n; j++) {
            if (i == j) continue;
            double dist = norm(lastPositions[i], lastPositions[j], NORM_L2);
            if (dist > maxDist) return false;

        }
    }
    
    return true;
}


    
// main marker detection
bool Tracking::detect()
{
    

    // for timing things
    timespec tstart, tend;  

    clock(tstart);
    markerInfo = NULL;
    markerNum = -1;
    if ( arDetectMarker((ARUint8 *)frame.data, (useInverted?(255-thresh):thresh), &markerInfo, &markerNum) < 0 ) {
        cout << "Error while detecting marker" << endl;
        markerDetected = false;
        return false;
    }
    if (debug)  clock(tend);
    if (debug) cout << "marker detection took " << elapsed_ms (tstart, tend)  << " ms" << endl;


    // count number of visible markers
    numMarkersUsed = 0;
    for (int i=0; i<config->marker_num; i++) {
        if (config->marker[i].visible != -1) numMarkersUsed++;
    }

    //
    // get camera position and orientation
    //
    
    if (debug) clock(tstart);
    
    // get 3x4 transformation matrix
    err = arMultiGetTransMat(markerInfo, markerNum, config);
    err /= (float)numMarkersUsed;

    // auto threshold: find threshold that produces the smallest error
    if (err < 0 && useAutoThreshold) {    
        runAutoThreshold();
    }
     
    
    if (err < 0 ) {
        markerDetected = false;
        return false;
        
    } else {

        // transformation matrix world coordinates -> camera coordinates
        for (int j=0; j<3; j++) {
            for (int i=0; i<4; i++ ) {
                transMat(j,i) = config->trans[j][i];
            }
        }
        transMat(4,0) = transMat(4,1) = transMat(4,2) = 0;
        transMat(4,3) = 1;

        // apply stage transf. matrix (shift origin, rotate stage etc)
        //transMat =  transMat * stageTransMat;
        
        // get camera rotation 
        for (int j=0; j<3; j++) {
            for (int i=0; i<4; i++ ) {
                rotMat(j,i) = transMat(j,i);
            }
        }
        rotMat = rotMat.inv();
        
        // get camera position (translation vector)
        camPos = -rotMat * Matx31d(transMat(0,3), transMat(1,3), transMat(2,3));
        
        
        // inefficient, but quickly implemented:
        lastPositions.insert(lastPositions.begin(), camPos);
        lastPositions.resize(maxLastPositions);
    }
    
    if (debug) clock(tend);
    if (debug) cout << "position calculation detection took " << elapsed_ms (tstart, tend)  << " ms" << endl;
    
    clock(tlast);
        
    markerDetected = true;
    return true;
}

// draw the debug image
void Tracking::createDebugImage()
{

    // POSSIBLE SPEEDUP
    frame.copyTo(debugFrame);

     for( int k=0; k<markerNum; k++ ) {
        for (int m=0; m<config->marker_num; m++ ) {
            if ( (config->marker[m].patt_id == markerInfo[k].id) && (config->marker[m].visible != -1) ) { 
                Point2i corners[4];
                for (int i=0; i<4; i++)
                corners[i] = Point2i (markerInfo[k].vertex[i][0], markerInfo[k].vertex[i][1]);

                // draw border
                for (int i=0; i<4; i++) {
                    line(debugFrame, corners[i], corners [(i+1)%4],CV_RGB(255,0,0),1);
                }

                // print ID
                stringstream ss;    
                ss << config->marker[m].patt_id << " (" << markerInfo[k].cf << ") ";
                putText(debugFrame, ss.str().c_str(),  Point(markerInfo[k].pos[0], markerInfo[k].pos[1]), FONT_HERSHEY_PLAIN, 1, CV_RGB(0,255,0), 1.5);
            }
        }
    }


    // revert previous invertion for correct display
//    if (useInverted) {
//        debugFrame = Mat(debugFrame.size(), CV_8UC3, CV_RGB(255,255,255)) - debugFrame;
//    }
    
    threshold(debugFrame, debugFrame, (useInverted?(255-thresh):thresh), 255, THRESH_BINARY);

    // draw unit vectors at origin of world
    double len = 30; // in mm
    
    Vec3d O (0,0,0); // origin
    Vec3d xaxis (len,0,0); 
    Vec3d yaxis (0,len,0);
    Vec3d zaxis (0,0,len);

    line(debugFrame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, xaxis), CV_RGB(0,0,255),3);
    line(debugFrame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, yaxis), CV_RGB(255,0,0),3);
    line(debugFrame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, zaxis), CV_RGB(0,255,0),3);
 
    /*
    // print position, rotation, error, number of markers
    stringstream ss;
    ss << "pos_cart = " << camPos;
    putText(debugFrame, ss.str().c_str(), Point(20,440), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);

    ss.str(string());
    ss << "pos_sphere = " << cart2spher(camPos);
    putText(debugFrame, ss.str().c_str(), Point(20,460), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);

    ss.str(string());
    ss << "err = " << err;
    putText(debugFrame, ss.str().c_str(), Point(20,420), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);

    // print threshold 
    ss.str(string());
    ss << thresh;
    putText(debugFrame, ss.str().c_str(), Point(20,20), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);
    */
}

        
