/**
 undistort_display : remaps a photograph of a screen to screen coordinates using a checkerboard-image
 
 @author Manuel Jerger <nom@nomnom.de>
*/

#include "undistort_display.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "undistort_display";

/**
 The Main function is the only function, due to simplicity of the program
 It takes camera-parameters, a checkerimage, a sourceimage and screen size in pixels as input, and produces an undistorted, remapped screen image.
 The implementation uses OpenCV and goes as follows:
    - load images, camera params
    - apply camera parameters to both images using OpenCVs undistort methods
    - find corners on checker-board with findChessboardCorners() and refine them with cornerSubPix()
    - calculate screen coordinates of the corresponding points and calculate the perspective transformation with getPerspectiveTransform
    - undistort image with warpPerspective()
    - dump output or show on screen
*/
int main(int argc, char *argv[])
{

    Mat cameraMatrix, distCoeffs;
    int success = 0;

    if (argc < 6) {
        cout << "Usage: " << PROGNAME << " <cam_params>  <grid> <source> <width> <height> <destination>" << endl << endl <<
        "       <cam_params>        camera parameters created with calibrate_camera, yml format" << endl <<
        "       <grid>              image file: the screen displaying the checkerboard image (use \"display_control ... --checker ...\")" << endl <<
        "       <source>            image file: the screen content we want to undistort" << endl <<
        "       <width>             screen width in pixels" << endl <<
        "       <height>            screen height in pixels" << endl <<
        "       [destination]       output image (if not present : results will be shown in a window)" << endl;

        return -1;
    }

    // load camera parameters
    if (strcmp(argv[1], "-") != 0 ) {
        FileStorage fs(argv[1], FileStorage::READ);
        fs["camera_matrix"] >> cameraMatrix;
        fs["distortion_coefficients"] >> distCoeffs;
        fs.release();
    }


    // reference grid image
    Mat grid = imread(argv[2], 1);
    Mat grid_corrected;

    // screen image we want to rectify
    Mat unmapped = imread(argv[3], -1);
    assert (unmapped.type() == CV_32FC3);

    Mat unmapped_corrected;
    Mat mapped;

    // display resolution in pixels
    Size screenSize (atoi(argv[4]), atoi(argv[5]));

    assert (grid.size().width == unmapped.size().width && grid.size().height == unmapped.size().height);

    Size imageSize = grid.size();



    // path for the remapped image
    string destination = (argc >= 7) ? argv[6] : "";

    cout << PROGNAME << " started" << endl;

    // undistort images (camera distortion)
    if (cameraMatrix.empty()) {
        cout << "Note: camera matrix is zero" << endl;
        unmapped_corrected = unmapped;
        grid_corrected = grid;
    } else  {
        Mat map1, map2;
        initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(),
                                cameraMatrix,
                                //getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0),
                                imageSize, CV_16SC2, map1, map2);
        remap(unmapped, unmapped_corrected, map1, map2, INTER_LINEAR);
        remap(grid, grid_corrected, map1, map2, INTER_LINEAR);
    }

    //
    // undistort
    //

    // 1) user grid image to find the 4 corners (note: the _inner_, not the outer corners!)

    // convert to 8 bit grayscale
    Mat grid_gray;
    cvtColor(grid_corrected, grid_gray, COLOR_BGR2GRAY);

    // four points on grid image and the corrseponding points in screen coordinates
    vector<Point2f> checker_points; // all checker points
    vector<Point2f> grid_points;    // the four outer checker points
    vector<Point2f> screen_points;  // the four points in screen coordinates


    const Size boardSize(8,6);  // number of inner corners
    success = findChessboardCorners( grid_gray, boardSize, checker_points, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE); // | CV_CALIB_CB_FAST_CHECK
    if (!success) {
        cout << "Error: could not find a checkerboard in image!" << endl;
        exit (-1);
    }
    cornerSubPix( grid_gray, checker_points, Size(11,11), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));

    grid_points.push_back(checker_points[0]);
    grid_points.push_back(checker_points[boardSize.width-1]);
    grid_points.push_back(checker_points[(boardSize.height-1) * boardSize.width]);
    grid_points.push_back(checker_points[(boardSize.height * boardSize.width)-1]);


    // 2) the corresponding 4 points in screen coordinates
    float bfact = 0.1;   // size of white border as a factor of screen dimensions
    float border_dist_x = ((float)screenSize.width * (1.0 - 2.0 * bfact) )/(float)(boardSize.width+1) + (float)screenSize.width * bfact - 1.0;
    float border_dist_y = ((float)screenSize.height * (1.0 - 2.0 * bfact) )/(float)(boardSize.height+1) + (float)screenSize.height * bfact - 1.0;

    screen_points.push_back (Point2f(border_dist_x, border_dist_y));
    screen_points.push_back (Point2f((float)screenSize.width - border_dist_x , border_dist_y));
    screen_points.push_back (Point2f(border_dist_x, (float)screenSize.height - border_dist_y));
    screen_points.push_back (Point2f((float)screenSize.width - border_dist_x , (float)screenSize.height - border_dist_y));


    if (destination.size() == 0) {
        for (int i=0; i<4; i++) {
            circle (grid_corrected, grid_points[i], 13, cvScalar(255,0.0,0));
            circle (grid_corrected, grid_points[i], 1, cvScalar(0,0,255,0));
            circle (unmapped_corrected, grid_points[i], 13, cvScalar(255,0,0,0));
        }
    }
    // 3) get and apply perspective transform
    Mat ptrans = getPerspectiveTransform(grid_points, screen_points);

    warpPerspective(unmapped_corrected, mapped, ptrans, screenSize);

    if (destination.size() == 0) {
        for (int i=0; i<4; i++) {
            circle (mapped, screen_points[i], 13, cvScalar(0,0,1.0,0));
        }
    }
    // show images
    if (destination.size() == 0) {

        // open window so we can display the undistorted image
        namedWindow("results");

        for (;;) {
            imshow("results", grid_corrected);
            waitKey(0);
            imshow("results", unmapped_corrected);
            waitKey(0);
            imshow("results", mapped);
            int key = 0xff & waitKey(0);
            if( (key & 255) == 27 || key == 'q' || key == 'Q' )
                break;
        }

        destroyWindow("results");

    // dump image
    } else {
        assert (mapped.type() == CV_32FC3);
        imwrite(destination, mapped);
    }


    // dump image
    cout << PROGNAME << " finished" << endl;
    return 0;
}

