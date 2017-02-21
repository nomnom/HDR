/**
   artoolkit_test : combining ARToolKit with OpenCV
    
   It started as an experiment and became a tool for debugging and evaluating the tracking portion of the lightstage.
   There are two codepaths: the run_single method uses the single marker tracking functions of ARToolKit.
   The run_multi codepath is the final implementation that is used in the lightstage code.  
   It uses the multi marker functions and works with the tracking-stage definitions. 
   The program uses OpenCV to load a videofile, a single image, or a live camera stream, performs lense correction, and passes the image to ARToolKit.
   The calculated extrinsics are shown in a window, besides the augmented tracking images, and can also be dumped to a logfile.
   
  @author Manuel Jerger <nom@nomnom.de>
*/

#include "artoolkit_test.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "artoolkit_test";


/**
 Prints usage.
*/
void help()
{
    cout << "Use ARtoolkit to track camera position in 3d space; uses OpenCV for camera input and transformation" << endl <<
        "Usage: \n" <<
        " " << PROGNAME << " <camera_parameters.yml> <size> <marker1.dat> <marker2.dat> ..." << endl <<
        "      <camera_parameters.yml>        Camera Matrix and Distortion coefficients." << endl <<
        "      <size>                         Edgelength of the square markers in mm." << endl <<
        "      <marker0.dat> ...              ARToolKit marker data file list" << endl << endl <<
        " " << PROGNAME << " <camera_parameters.yml> --multi <multimarker.dat> [in_video.avi] [out.log]" << endl <<
        "      <camera_parameters.yml>        Camera Matrix and Distortion coefficients." << endl <<
        "      <multimarker.dat>              ARToolKit multimarker data file." << endl << 
        "      <threshold>                    Threshold to use (0-255)" << endl << 
        "      <use_color>                    If 0/false: use greyscale" << endl << 
        "      <use_inverted>                 If 1/true: Invert image data" << endl << 
        "      [in_video.avi]                 Use video as input; Can also be used to specify the video device id (0 = /dev/video0) " << endl  << 
        "      [out.log]                      Logfile for logging tracking data. Format: 'frameID err #used_markers spherical_coords cartesian_coords'" << endl << endl; 
        
}


/**
   Convert 3d world coordinates into screen position
*/
Point  space2screen (Mat camMat, Mat transMat, Vec3f space)
{
    Mat sp (4,1,CV_64F);
    sp.at<double>(0) = space[0];
    sp.at<double>(1) = space[1];
    sp.at<double>(2) = space[2];
    sp.at<double>(3) = 1;
    Mat pt = camMat * (transMat * sp);
    return Point (pt.at<double>(0)/pt.at<double>(2), pt.at<double>(1)/pt.at<double>(2));;
}


/**
   @param in spherical: (r,theta, phi)
   @return cartesian: (x,y,z)
      phi: polar, against Z
//    theta: azimuth, against X (CCW)
*/
inline Vec3f spher2cart (Vec3f in)
{
    Vec3f c;
    // x = r * sin(t) * cos(p)
    c[0] = in[0] * sin(in[1]) * cos(in[2]);
    // y = r * sin(t) * sin(p)
    c[1] = in[0] * sin(in[1]) * sin(in[2]);
    // z = r * cos(t)
    c[2] = in[0] * cos(in[1]);
    return c;
}

/**
   @param in cartesian: (x,y,z)
   @return spherical: (r,theta, phi)
*/
inline Vec3f cart2spher (Vec3f in)
{
    Vec3f s;
    // r = sqrt(x*x + y*y + z*z)
    s[0] = sqrt(in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
    // theta = arccos(z/r)
    s[1] = acos(in[2]/s[0]);
    // phi = arctan(y/x)
    s[2] = atan2(in[1],in[0]);
    return s;
}

// webcam frame size
#ifdef __APPLE__
  #define VIDEO_WIDTH  1280
  #define VIDEO_HEIGHT  720
#else
  #define VIDEO_WIDTH  640
  #define VIDEO_HEIGHT 480
#endif


/**
  ARToolkit and OpenCV: single marker codepath
*/
int run_single (int argc, char* argv[]) 
{
    //
    // OpenCV
    //

    Mat frame;
    VideoCapture capt;
    capt.open(0);
    if ( ! capt.isOpened() ) {
        cout << " could not open video device" << endl;
        return -1;
    }
    capt >> frame;
     
    // check frame size
//    if (not (frame.size().width == VIDEO_WIDTH && frame.size().height == VIDEO_HEIGHT)) {
//        cout << "error: video frame size is not " << VIDEO_WIDTH << " x " << VIDEO_HEIGHT << " but " << frame.size().width << " x " << frame.size().height << endl;
//        exit (-1);
//    }
    

    // load camera parameters
    Mat cameraMatrix, distCoeffs;
    Mat map1, map2;
    cout << "loading camera parameters " << argv[1] << endl;
    FileStorage fs(argv[1], FileStorage::READ);
    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    fs.release();
    // calculate undistort maps
    initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, frame.size(), CV_16SC2, map1, map2);

    // calculate FOV
    double fov_theta_x = 180 * atan2( (frame.size().width / 2),  cameraMatrix.at<double>(0,0));
    double fov_theta_y = 180 * atan2( (frame.size().height / 2), cameraMatrix.at<double>(1,1)); 
    cout << "FOV is " << fov_theta_x << " x " << fov_theta_y << " degree " << endl;
   
    // ARToolkit
    //

    // set up camera parameters for artoolkit
    ARParam cameraParam;

    // fixed frame size
    cameraParam.xsize=frame.size().width;
    cameraParam.ysize=frame.size().height;
    
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
        
                   
        
    // third arg is markersize in mm
    double markerSize = atof(argv[2]);
        
    // load single AR marker
    vector<int> patternIDs;
    for (int i=3; i<argc; i++) {
        int id = -1;
        cout << "loading pattern " << argv[i] << endl;
        if ( (id = arLoadPatt(argv[i])) < 0) {
            cout << "Error while setting up AR marker." << endl;
            capt.release();
            return -1;
        } else {
            patternIDs.push_back(id);
        }
    }

    namedWindow("main");

    ARMarkerInfo* markerInfo;
    int markerNum = -1;

    bool useHistory = false;
    double para[3][4]; // for history function: reuse transformation matrix of previous step
    
    int thresh = 70;
    bool showThreshold = false;
    bool useUndistort = true; 
    bool useColor = true;
    bool useInverted = false;

    int key;
    while ( (key = waitKey(30) & 0xFF ) != 27 ) {
        //cout << key << endl;
        if (key == 82) { thresh = min(thresh+5,255); }  // up-arrow
        if (key == 84) { thresh = max(thresh-5,0); }    // down-arrow
        if (key == 't') { showThreshold = not showThreshold; }
        if (key == 'c') { useColor = not useColor; }
        if (key == 'i') { useInverted = not useInverted; }
        if (key == 'd') { useUndistort = not useUndistort; }
        if (key == 32) { useHistory = not useHistory;}     // spacebar
        
        
        capt >> frame;
        
        // undistort image
        if (useUndistort && !map1.empty())
           remap(frame,frame, map1, map2, INTER_LINEAR);

        if (not useColor) {
            Mat grey; cvtColor(frame,grey,CV_BGR2GRAY);
            Mat gchannels[3] = {grey,grey,grey};
            merge(gchannels, 3, frame);
        }
        
        if (useInverted) {
            frame = Mat(frame.size(), CV_8UC3, CV_RGB(255,255,255)) - frame;
        }
        // detect marker
        markerInfo = NULL;
        markerNum = -1;
        if ( arDetectMarker((ARUint8 *)frame.data, (useInverted?(255-thresh):thresh), &markerInfo, &markerNum) < 0 ) {
            cout << "Error while detecting marker" << endl;
            return 1;
        }
        /*
        if (useInverted) {
            frame = Mat(frame.size(), CV_8UC3, CV_RGB(255,255,255)) - frame;
        }*/

        // check if our marker is present
        int k = -1;
        for (int i = 0; i < markerNum; i++) {
            for (int p=0; p<patternIDs.size(); p++) {
                if (patternIDs[p] == markerInfo[i].id) {

                    // get corner coordinates
                    k = i;
                    Point2i corners[4];
                    for (int i=0; i<4; i++)
                        corners[i] = Point2i (markerInfo[k].vertex[i][0], markerInfo[k].vertex[i][1]);

                    // draw lines
                    for (int i=0; i<4; i++) {
                        line(frame, corners[i], corners [(i+1)%4],CV_RGB(255,0,0),1);
                    }

                    // mark first corner (rotation invariant)
                    // Not sure how markerInfo.dir is supposed to be used..
                    int first = 0;
                    switch (markerInfo[k].dir) {
                        case 0: first = 1; break;
                        case 1: first = 0; break;
                        case 2: first = 3; break;
                        case 3: first = 2; break;
                    }
                    circle(frame, corners[first], 5, CV_RGB(0,0,255));

                    // ID
                    stringstream ss;
                    ss << p << " (" << markerInfo[k].cf << ") " << markerInfo[k].dir;
                    putText(frame, ss.str().c_str(),  Point(markerInfo[k].pos[0], markerInfo[k].pos[1]), FONT_HERSHEY_PLAIN, 1.5, CV_RGB(0,255,0), 2);

                    // get 3x4 transformation matrix
                    double center[2] = {0,0};
                    if (useHistory) {
                        arGetTransMatCont(&(markerInfo[k]), para, center, markerSize, para);
                    } else {
                        arGetTransMat(&(markerInfo[k]), center, markerSize, para);
                    }
                    // convert to OpenCV Matrix
                    Mat transMat (3,4, CV_64F);
                    int i, j;
                    for (j=0; j<3; j++) {
                        for (i=0; i<4; i++ ) {
                            transMat.at<double>(j,i) = para[j][i];
                        }
                    }

                    // draw vector in x/y/z-direction
                    double len = 30;
                    Vec3d O (0,0,0);
                    Vec3d xaxis (len,0,0);
                    Vec3d yaxis (0,len,0);
                    Vec3d zaxis (0,0,len);

                    line(frame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, xaxis), CV_RGB(0,0,255),3);
                    line(frame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, yaxis), CV_RGB(255,0,0),3);
                    line(frame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, zaxis), CV_RGB(0,255,0),3);

                    // get camera orientation
                    Mat rotMat = Mat(3,3,CV_64F);
                    rotMat = transMat(Rect(0,0,3,3));

                    // camera position (translation vector)
                    Vec3f camPos = transMat.col(3);
                    //cout << cartesian2spherical(camPos) << endl;

                    // print position, rotation
                    ss.str(string());;
                    ss << camPos;
                    putText(frame, ss.str().c_str(), Point(20,440), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);
                    
                    ss.str(string());
                    ss << cart2spher(camPos);
                    putText(frame, ss.str().c_str(), Point(20,460), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);
                    
                }
            }
        }

        if (useHistory) putText(frame, "using history", Point(100,20), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);

        // threshold
        stringstream ss;
        ss << thresh;
        putText(frame, ss.str().c_str(), Point(20,20), FONT_HERSHEY_PLAIN, 1, CV_RGB(0,0,255), 1);
        
        
        
        if (showThreshold) {
            threshold(frame, frame, (double)(useInverted?(255-thresh):thresh), 255, THRESH_BINARY);
        }
        imshow("main", frame);
            

    } // main loop
    
    // cleanup
    destroyWindow("main");
    capt.release();
    
    return 0;
}


/**
  ARToolkit and OpenCV: multi marker codepath
*/
int run_multi (int argc, char* argv[]) 
{
    //
    // OpenCV
    //
    

    Mat frame;
    VideoCapture capt;
    
    // if arg given: use videofile
    string inFile;
    if (argc >= 8) { 
        cout << argv[7] << endl;
        capt.open(argv[7]);
        if ( ! capt.isOpened() ) {
            // interpret argument as device id
            capt.open(atoi(argv[7]));
            if ( ! capt.isOpened() ) {
               capt.open(0);
               if ( ! capt.isOpened() ) {
                  cout << " could not open video file or video device ID " << argv[7] << endl;
                  return -1;
               }
            }
        } else {
            inFile = argv[7]; // remember video filename
        }
        
    // default: use device 0
    } else {
        capt.open(0);
    }
    
    if ( ! capt.isOpened() ) {
        cout << " could not open the first video device" << endl;
        return -1;
    }
    // read first image
    capt >> frame;
    
     
    // check frame size
//    if (not (frame.size().width == VIDEO_WIDTH && frame.size().height == VIDEO_HEIGHT)) {
//        cout << "error: video frame size is not " << VIDEO_WIDTH << " x " << VIDEO_HEIGHT << " but " << frame.size().width << " x " << frame.size().height << endl;
//        exit (-1);
//    }

    // load camera parameters
    Mat cameraMatrix, distCoeffs;
    Mat map1, map2;
    cout << "loading camera parameters " << argv[1] << endl;
    FileStorage fs(argv[1], FileStorage::READ);
    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    fs.release();
    // calculate undistort maps
    initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, frame.size(), CV_16SC2, map1, map2);



    //
    // ARToolkit
    //

    // set up camera parameters for artoolkit
    ARParam cameraParam;

    // fixed frame size
    cameraParam.xsize=frame.size().width;
    cameraParam.ysize=frame.size().height;
    
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
 
    // debug: print artoolkit camera parmeters
    for (int i=0; i<3; i++) cout << cameraParam.mat[i][0] << " " << cameraParam.mat[i][1] << " " << cameraParam.mat[i][2] << " " << cameraParam.mat[i][3] << endl;
    cout << endl;
    
    for (int i=0; i<4; i++) cout << cameraParam.dist_factor[i] << " ";
    cout << endl;
    
    
    arInitCparam(&cameraParam);
    
        
    // load multi AR marker file
    ARMultiMarkerInfoT  *config;
    if( (config = arMultiReadConfigFile(argv[3]) ) == NULL ) {
        cout << "Error while loading multi AR marker config " << argv[3]  << endl;
        capt.release();
        return -1;
    }
    
    int thresh = 50;    
    if (argc >= 5) thresh = atoi(argv[4]);
    
    bool useColor = true;       // flag for use of color or grey
    if (argc >= 6 && (strcmp(argv[5], "0") && strcasecmp(argv[5], "false") == 0 )) {
       useColor = false;
    }
    
    bool useInverted = false;   // flag for inverting color
    if (argc >= 7 && (strcmp(argv[6], "1") && strcasecmp(argv[6], "true") == 0)) {
       useInverted = true;
    }
    
    // logfile
    string outFile;
    ofstream ofLog;
    if (argc >= 9)  {
        outFile = argv[8];
        ofLog.open(outFile);
    }
    
    
    namedWindow("main", WINDOW_NORMAL );
    cvSetWindowProperty("main", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);


    ARMarkerInfo* markerInfo;
    int markerNum = -1;

    bool showThreshold = true;
    bool useUndistort = true;
    
    int key;                    // last pressed key
    
    int frameidx = 0;      // current frame index
    bool doWait = inFile.empty(); // live input vs. video file input
//    timespec tstart, tend;  // for high precision stopwatch
//    timespec tlast, tnow;   // whole loop time; for FPS calculation
    double FPS = 0;
    
    while (true) {
    
//        clock_gettime(CLOCK_REALTIME, &tlast);    
        
        if (doWait) {
            key = waitKey(30) & 0xFF;
            if (key == 27) break;
            if (key == 82) { thresh = min(thresh+5,255); }  // up-arrow
            if (key == 84) { thresh = max(thresh-5,0); }    // down-arrow
            if (key == 't') { showThreshold = not showThreshold; }
            if (key == 'c') { useColor = not useColor; }
            if (key == 'i') { useInverted = not useInverted; }
            if (key == 'd') { useUndistort = not useUndistort; }
        } else {
            // for highui window update
            waitKey(1);
        }
    
        // undistort image
        if (useUndistort && !map1.empty())
           remap(frame,frame, map1, map2, INTER_LINEAR);
 
        if (not useColor) {
            Mat grey; cvtColor(frame,grey,CV_BGR2GRAY);
            Mat gchannels[3] = {grey,grey,grey};
            merge(gchannels, 3, frame);
        }
 
        if (useInverted) {
            frame = Mat(frame.size(), CV_8UC3, CV_RGB(255,255,255)) - frame;
        }
        
        // detect all marker
        markerInfo = NULL;
        markerNum = -1;
        if ( arDetectMarker((ARUint8 *)frame.data, (useInverted?(255-thresh):thresh), &markerInfo, &markerNum) < 0 ) {
            cout << "Error while detecting marker" << endl;
            return 1;
        }
        
       
        // draw border on all visible marker (even unidentified ones)
        for( int k=0; k<markerNum; k++ ) {
                
                Point2i corners[4];
                for (int i=0; i<4; i++)
                corners[i] = Point2i (markerInfo[k].vertex[i][0], markerInfo[k].vertex[i][1]);

                // draw border
                for (int i=0; i<4; i++) {
                    line(frame, corners[i], corners [(i+1)%4],CV_RGB(255,0,0),1);
                }

                // print ID
                stringstream ss;
                ss << k << " (" << markerInfo[k].cf << ") ";
                putText(frame, ss.str().c_str(),  Point(markerInfo[k].pos[0], markerInfo[k].pos[1]), FONT_HERSHEY_PLAIN, 1.5, CV_RGB(0,255,0), 2);
        }


        // get 3x4 transformation matrix
        Mat transMat (3,4, CV_64F);
        
        // error (independent of markers used);
        double err =  arMultiGetTransMat(markerInfo, markerNum, config);
        int numMarkersUsed = 0;
        for (int i=0; i<config->marker_num; i++) {
            if (config->marker[i].visible != -1) numMarkersUsed++;
        }
        
        err /= (float)numMarkersUsed;
        
        if( err < 0 ) {
            putText(frame,  "Error getting multi marker transformation matrix", Point(20,300), FONT_HERSHEY_PLAIN, 1, CV_RGB(255,255,255), 1);
             
        } else {
        
        
            int i, j;
            for (j=0; j<3; j++) {
                for (i=0; i<4; i++ ) {
                    transMat.at<double>(j,i) = config->trans[j][i];
                }
            }

            // get camera orientation
            Mat rotMat = Mat(3,3,CV_64F);
            rotMat = transMat(Rect(0,0,3,3));

            // camera position (translation vector)
            Mat camPos = - rotMat.inv() * transMat.col(3);
           
            // rotation vector
            Mat rotVec (3,1,CV_64F);
            //rotVec = cart2spher(camPos);
            
            // important vectors
            //cout << camPos << endl;
            Mat up = -rotMat.inv().col(1); // Y = down
            Mat right = -rotMat.inv().col(0); //X = left
            Mat forward = rotMat.inv().col(2); //Z = forward
            
             for( int k=0; k<markerNum; k++ ) {
                for (int m=0; m<config->marker_num; m++ ) {
                    if ( config->marker[m].patt_id == markerInfo[k].id  && config->marker[i].visible != -1 ) {  
                        Point2i corners[4];
                        for (int i=0; i<4; i++)
                        corners[i] = Point2i (markerInfo[k].vertex[i][0], markerInfo[k].vertex[i][1]);

                        // draw border
                        for (int i=0; i<4; i++) {
                            line(frame, corners[i], corners [(i+1)%4],CV_RGB(0,0,255),1);
                        }

                        // print ID
                        //stringstream ss;    
                        //ss << config->marker[m].patt_id << " (" << markerInfo[k].cf << ") ";
                        //putText(frame, ss.str().c_str(),  Point(markerInfo[k].pos[0], markerInfo[k].pos[1]), FONT_HERSHEY_PLAIN, 1.5, CV_RGB(0,255,0), 2);
                    }
                }
            }

            // draw vector in x/y/z-direction
            double len = 30;
            Vec3d O (0,0,0);
            Vec3d xaxis (len,0,0);
            Vec3d yaxis (0,len,0);
            Vec3d zaxis (0,0,len);

            line(frame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, xaxis), CV_RGB(0,0,255),3);
            line(frame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, yaxis), CV_RGB(255,0,0),3);
            line(frame, space2screen(cameraMatrix, transMat, O), space2screen(cameraMatrix, transMat, zaxis), CV_RGB(0,255,0),3);
         
            
            // print position, rotation
            stringstream ss;
            ss << camPos;
            putText(frame, ss.str().c_str(), Point(20,440), FONT_HERSHEY_PLAIN, 1, CV_RGB(0,0,255), 1);

            ss.str(string());
            ss << cart2spher(camPos);
            putText(frame, ss.str().c_str(), Point(20,460), FONT_HERSHEY_PLAIN, 1, CV_RGB(0,0,255), 1);

            ss.str(string());
            ss << err;
            putText(frame, ss.str().c_str(), Point(20,420), FONT_HERSHEY_PLAIN, 1, CV_RGB(0,0,255), 1);

            
            // write log line
            if ( !outFile.empty() ) {
                ofLog << frameidx << " " << err << " " << numMarkersUsed << " " << cart2spher(camPos) << " " << camPos << endl;
            }
            cout << frameidx << " " << err << " " << numMarkersUsed << " " << cart2spher(camPos) << " " << camPos << endl;
        } 
        
        // print threshold
        stringstream ss;
        ss << thresh;
        putText(frame, ss.str().c_str(), Point(20,20), FONT_HERSHEY_PLAIN, 1, CV_RGB(0,0,255), 1);
        
        // preview threshold
        if (showThreshold) {
            threshold(frame, frame, (double)(useInverted?(255-thresh):thresh), 255, THRESH_BINARY);
        }
        
        imshow("main", frame);
        
        // read next frame
        if (!capt.read(frame)) {
            // if video file: no more frames -> end main loop
            if (!inFile.empty()) { 
                break;
            }
        }
          
  
        frameidx++;
        
        
    } // end main loop
    
    // cleanup
    if ( !outFile.empty() ) ofLog.close();
    destroyWindow("main");
    capt.release();
    
    return 0;
}

/**
  Check if first arg is --multi and run either the multi or the single codepath.
*/
int main (int argc, char* argv[])
{

    if (argc < 3) {
        help();
        return -1;
    }
    
    if ( strcmp(argv[2], "--multi") == 0 ) {
        return run_multi (argc, argv);
    } else {
        return run_single (argc, argv);
    }

}
 
