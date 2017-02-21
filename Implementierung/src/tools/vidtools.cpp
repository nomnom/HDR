/*
 * simple video tools (opencv-based)
 *
 * @author Manuel Jerger
 *
 */

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O and image loading/writing
#include <string>
#include <getopt.h>   // command line options
#include <stdlib.h>    
#include <iostream>   //stringstream

using namespace std;
using namespace cv;


// numFrames = 0 -> read until end
bool vidread (VideoCapture capt, vector<Mat>& frames, int numFrames, bool showInWindow)
{
    if (!capt.isOpened()) return false;
    cout << "reading " << numFrames << " frames" << flush;
    bool success = true; 
    int idx=0;
    while (success && (numFrames == 0 || idx < numFrames)) {
        Mat frame;
        success = capt.isOpened() && capt.read(frame) ;
        
        frames.push_back(frame);
        idx++;
        cout << "." << flush;
        if (showInWindow) {
            imshow("main", frame);
            int key = waitKey(1) & 255;
            if (key == 27 || key == 13) success = false;
        }
    }
    if (idx < numFrames-1) { 
        cout << " missing " << numFrames-(idx+1) << " frames" << endl;
        return false;
    } else if (idx == 0) {
        cout << " no frames!"<< endl;
        return false;
    } else {
        cout << " done!" << endl;
    }
    return true;
}


bool vidwrite (VideoWriter writer, vector<Mat>& frames)
{
    if (!writer.isOpened()) return false;
    for (int f=0; f<frames.size(); f++) {
        writer << frames[f];
    }
    return true;
}

////////// MAIN  ///////////////////////////////////////////////////////


int main(int argc, char *argv[])
{

    // input/output videos (cache)
    vector< vector<Mat> > vids;
    
    // equal fps and framesize for all videos
    float fps = 16;  
    Size frameSize;
    
    // iterate over arguments, perform actions
    int option_index = 0, c=0;
    while ( ( c = getopt (argc, argv, "i:o:O:p:")) != -1)
    {
        string tmp;
        stringstream arg;
        if (c) arg << (optarg);
        switch (c) {
        
            // load videos / open device
            case 'i':
            {
                arg >> tmp;
                
                VideoCapture capt (tmp);
                
                // in case of video device 
                if (tmp.length() == 1) {   
                    cout << "opening device /dev/video" << tmp << endl;
                    
                    int devNumber = atoi(tmp.c_str());
                    
                    
                    int numFrames;
                    arg >> numFrames;
                    
                    int showInWindow = 0;
                    arg >> showInWindow;
                    
                    VideoCapture capt (devNumber);
                    vector<Mat> frames;
                    
                    
                    if (showInWindow) {
                        namedWindow("main", CV_WINDOW_AUTOSIZE);
                    }
                    
                    if (!vidread(capt, frames, numFrames, (showInWindow > 0))) {
                        cout << "Error while opening the device!";
                        
                    } else {
                        // first loaded frame defines frame size and fps
                        if (vids.size() == 0) {
                            frameSize = frames[0].size();
                            cout << "video frame size is " << frameSize << endl;
                        } else {
                            // check against frameSize
                            assert (frames[0].size().width == frameSize.width &&
                                    frames[0].size().height == frameSize.height);
                        }    
                        vids.push_back(frames);
                    }
                    capt.release();
                    if (showInWindow) {
                        destroyWindow("main");
                    }
                
                // open file
                } else {
                    cout << "opening video file " << tmp << endl;
                    
                    vector<Mat> frames;
                    
                    arg >> fps;
                    
                    if (!vidread(capt, frames, 0, false)) {
                        cout << "Error while reading the file!";
                    } else {
                        // first loaded frame defines frame size
                        if (vids.size() == 0) {
                            frameSize = frames[0].size();
                        } else {
                            // check against frameSize
                            assert (frames[0].size().width == frameSize.width &&
                                    frames[0].size().height == frameSize.height);
                        }    
                        vids.push_back(frames);
                    }
                    
                }
                
                capt.release();
                    
                break;
            }
            
            // write out resulting video
            case 'o':
            {   
                string outfile; arg >> outfile;
                cout << "writing result to " << outfile << endl;
                if (vids.size() > 1) cout << "Warning: more than one video in cache ! Dumping only the first one!" << endl;
                VideoWriter writer(outfile, CV_FOURCC('M','P','E','G'), fps, frameSize);
                vidwrite (writer, vids[0]);
                writer.release();
                
                break;
            }
           
      
            // write out frames as 24 bit bmp files
            case 'O':
            {   
                string outfile; arg >> outfile;
                if (vids.size() > 1) cout << "Warning: more than one video in cache ! Dumping only the first one!" << endl;
                
                for (int f=0; f<vids[0].size(); f++) {
                    stringstream ss;
                    ss << outfile << "_" << f << ".bmp"; 
                    
                    cout << "writing frame " << f << " to " << outfile << "_" << f << ".bmp" << endl;
                    imwrite (ss.str(), vids[0][f]);
                }
                
                
                break;
            }
            
            case 'p':
            {  
                string camParamsFile; arg >> camParamsFile;
                            
                Mat cameraMatrix;
                Mat distCoeffs;
                Mat map1, map2;
                
                // load camera parameters
                cout << "loading camera parameters " << camParamsFile << endl;
                FileStorage fs(camParamsFile, FileStorage::READ);
                fs["camera_matrix"] >> cameraMatrix;
                fs["distortion_coefficients"] >> distCoeffs;
                fs.release();
                
                // calculate undistort maps
                initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, vids[0][0].size(), CV_16SC2, map1, map2);

                cout << "undistorting " << vids.size() << " videos " << flush;
                for (int v=0; v<vids.size(); v++) {
                    for (int f=0; f<vids[v].size(); f++) {
                        remap(vids[v][f],vids[v][f], map1, map2, INTER_LINEAR);
                    }
                    cout << "." << flush;
                    
                }
                cout << " done!" << endl;
                break;
            }
            
        }
    }
    
    return 0;
}





