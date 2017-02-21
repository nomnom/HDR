/**

  reconstruct : merges the images captured with the lightstage program
   
  @author Manuel Jerger <nom@nomnom.de>
*/

#include "reconstruct.h"

using namespace std;
using namespace cv;

const char* PROGNAME = "reconstruct";


/**
 Prints usage.
*/
void help()
{
    cout << "Reconstructs an image from lightstage recordings" << endl <<
        "Usage: " << PROGNAME << " <capture_dir> <exposures.log> <output_img>" << endl <<
        "     <capture_dir>          Directory with the lightstage recordings" << endl <<
        "     <exposures.log>          name of exposures logfile with the factors)" << endl <<
         
        "     <output_img>           The scene image that will be created." << endl << 
        "     <minval>           values below this specification will be clipped to 0" << endl << endl;
}


/**
  Values below min are set to 0
*/
Mat& clamp (Mat& A, float min)
{

    for (int t=0; t<A.total(); t++)
        for (int c=0; c<A.channels(); c++ ) {
            if (A.at<Vec3f>(t)[c] < min) A.at<Vec3f>(t)[c] = 0;
        }
    return A;
}


/**
  Do the thing.
*/
int main(int argc, char *argv[])
{
    cout << PROGNAME << " started" << endl;

    if (argc < 4) {
        help();
        return -1;
    }

    string captureDir = argv[1];
    string expLog = argv[2];
    string outFile = argv[3];
   
    double minval = 0;
    if (argc >= 5) minval = atof(argv[4]);
 
    // 1) open all light stage images and adjust the exposures
    
    // read exposures.log
    vector<double> exposures;
    vector<Mat> imgs;   // recorded images
    //vector<Mat> dfs;    // darkframes
    stringstream ss; ss << captureDir << "/" << expLog;
    ifstream in;
    in.open(ss.str().c_str(), ios::in);
    int frame;
    double exp;
    while (in >> frame >> exp) {
       cout << "frame " << frame << " exposure " << exp << endl;
       exposures.push_back(exp);
       
       // check for doubles
        //   for (double e : exposures ) { if (e == exp) { cout << "ERROR: double index occured in exposure logfile" << endl; exit (0); } }
       
       // load and check image
       stringstream is; is << captureDir << "/result/" << frame << "_res.exr";
       cout << "loading " << is.str() << " ..." << endl;
       Mat img = imread(is.str(), -1);
       imgs.push_back(img);
       assert (img.data != NULL);
       assert (img.type() == CV_32FC3);
       assert (imgs[0].size().width == img.size().width && imgs[0].size().height == img.size().height);
       
       // optional clamp of lowest value
       if (minval > 0) {
          clamp(img,minval);
       }
       
    }
    cout << "loaded " << exposures.size() << " images of size " << imgs[0].size() << endl;

    // 2) merge all iamges into one
    // TODO tonemap option here later
    
    Mat result = Mat::zeros(imgs[0].size(),CV_32FC3);
    
    cout << "merging " << flush;
    for (int i=0; i<imgs.size(); i++) {
       
        // subtract darkframe
       //imgs[i] -= dfs[i];
       
       if (exposures[i] > 0) { 
         // change exposure
         imgs[i].convertTo(imgs[i],-1,1.0/exposures[i],0.0);
       
         // add up  // TODO use better combining operation here
         result += imgs[i];
       }
       cout << "." << flush;
    }
    cout << " done!" << endl;

    cout << "writing result to " << outFile;
    imwrite(outFile, result);

 
 

  return 0;
}
