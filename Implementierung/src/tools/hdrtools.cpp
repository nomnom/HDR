/*
 * hdr tools (mostly just a interface to the opencv hdr functions)
 *
 * @author Manuel Jerger
 *
 */

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc/imgproc.hpp>  // Image Processing
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O and image loading/writing
#include <opencv2/photo/photo.hpp>      // HDR tools

#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <assert.h>

using namespace std;
using namespace cv;


const char* PROGNAME = "hdrtools";


void help() 
{
    cout << "Shows images and environment maps on a display. Utilizes calibration data to achieve linear light output.\n" <<
        "Usage: \n" <<
        " " << PROGNAME << " --calibrate <method> <outfile> [<image0> <exposure0> <image2> <exposure2> ...]" << endl <<
        "     <method>              A single character: 'd'ebevec or 'r'obertson." <<  endl <<
        "     <outfile>             Filename of response curve (yml format)" <<  endl <<
        "     <image#>              One image of the exposure stack." << endl <<
        "     <exposure#>           Exposure time in seconds." << endl << endl <<
        
        " " << PROGNAME << " --merge <method> <outfile> [<image0> <exposure0> <image2> <exposure2> ...]" << endl <<
        "     <method>              Single character: 'd'ebevec, 'r'obertson or 'm'ertens" <<  endl <<
        "     <outfile>             Filename of the created HDR image." <<  endl <<
        "     <image#>              One image of the exposure stack." << endl <<
        "     <exposure#>           Exposure time in seconds." << endl << endl <<
        
        " " << PROGNAME << " --tonemap <method> <infile> <outfile>" << endl <<
        "     <method>              Single character: 'd'rago, d'u'rand, 'r'einhard or 'm'antiuk" <<  endl <<
        "     <infile>              HDR image that should be processed." <<  endl <<
        "     <outfile>             Filename of the created LDR image." <<  endl << endl;
}




Mat& run_merge( vector<Mat>& imgs, vector<double>& exps, Mat& result, char mode)
{    
    cout << "running HDR merge using " << (mode=='d'?"Debevec":"Robertson") << flush;
    
    cout << " done!" << endl;
    return result;
}


Mat& run_tonemap( vector<double>& img, Mat& result, int mode)
{

    return result;
}


int main(int argc, char *argv[])
{

    
    
    cout << PROGNAME << " started" << endl;
    
    int ret = 0;
    
    if (argc < 5) {
        help();
        ret = -1;
        
    } else {
        if ( (strcmp( argv[1], "--calibrate" ) == 0) && (argc >= 8)) { 
            
            
            //
            //  OECF recovery
            //    
        
            cout << "performing response curve calibration" << endl;
            
            
            // arg 3: mode
            char mode = '-';
            if (strcmp( argv[2], "d" ) == 0) { 
                mode = 'd'; 
            } else if (strcmp( argv[2], "r" ) == 0) { 
                mode = 'r';
            } else { 
                help(); ret=-1;
            }
            
            // arg 4: outfile
            string outfile = argv[3];
            
                    
            // exposure stack
            vector<Mat> imgs;    // images
            vector<double> exps; // exposures
            
            // arg >4: images and exposures
            int cnt=0;
            for (int a=4; a+1<argc; a+=2) {
                Mat img;
                img = imread(argv[a], -1);\

                assert (img.type() == CV_32FC3);
                if (img.data == NULL) {
                    cout << "Error loading " << argv[a] << endl;
                    return -1;
                }
                img.convertTo(img, CV_8UC3, 255.0, 0.0);
                imgs.push_back(img);
                
                double exp = atof(argv[a+1]);
                exps.push_back(exp);
                
                cnt++;
            }
            cout << "loaded " << cnt << " exposures" << endl;
            
            Mat curve (256, 1, CV_32FC3);
            
            switch (mode) {

                // Debevec
                case 'd':
                {
                    cout << "using Debevec method" << endl;
                    int samples = 70;
                    float lambda = 10.0f;
                    bool random = false;
                    cout << "Params: " << samples << " samples, lambda=" << lambda << (random?", ":", no ") << "random" << endl;
                    Ptr<CalibrateDebevec> cal = createCalibrateDebevec(samples, lambda, random); 
                    cal->process(imgs, curve, exps);
                    cout << " done!" << endl;
                } break;
                
                // Robertson
                case 'r':
                {
                    cout << "using Robertson method" << endl;
                    int max_iter = 30;
                    float threshold = 0.01f;
                    cout << "Params: max_iter=" << max_iter << " threshold=" << threshold << endl;
                    Ptr<CalibrateRobertson  > cal = createCalibrateRobertson(max_iter, threshold); 
                    cal->process(imgs, curve, exps);
                    cout << " done!" << endl;
                } break;
            }
            
            cout << "curve: " << endl;
            cout << curve << endl;
            
            cout << "writing curve to " << outfile << endl;
            
            // TODO write yml
            
            
        } else if ( (strcmp( argv[1], "--merge" ) == 0) && (argc >= 8)) { 
        
            //
            //  HDR MERGE
            //    
        
            cout << "performing HDR merge" << endl;
            
            // arg 3: mode
            char mode = '-';
            if (strcmp( argv[2], "d" ) == 0) { 
                mode = 'd'; 
            } else if (strcmp( argv[2], "r" ) == 0) { 
                mode = 'r';
            } else if (strcmp( argv[2], "m" ) == 0) { 
                mode = 'm';
            } else { 
                help(); ret=-1;
            }
            
            // arg 4: outfile
            string outfile = argv[3];
            
            // exposure stack
            vector<Mat> imgs;    // images
            vector<double> exps; // exposures
            
            // arg >4: images and exposures
            int cnt=0;
            for (int a=4; a+1<argc; a+=2) {
                Mat img;
                img = imread(argv[a], -1);
                assert (img.type() == CV_32FC3);
                if (img.data == NULL) {
                    cout << "Error loading " << argv[a] << endl;
                    return -1;
                }
                img.convertTo(img, CV_8UC3, 255, 0.0);
                imgs.push_back(img);
                
                double exp = atof(argv[a+1]);
                exps.push_back(1.0/ exp);
                
                cnt++;
            }
            cout << "loaded " << cnt << " exposures" << endl;
            for (int i=0; i<exps.size(); i++) {
                cout << exps[i] << " ";
            }
            cout << endl;
            
            Mat result;          // resulting HDR or LDR images
            
            switch (mode) {

                // Debevec
                case 'd':
                {
                    cout << "using Debevec method" << endl;
                    Ptr<MergeDebevec> mrg = createMergeDebevec ();
                    mrg->process(imgs, result, Mat(exps));
                    cout << " done!" << endl;
                } break;
                
                // Robertson
                case 'r':
                {
                    cout << "using Robertson method" << endl;
                    Ptr<MergeRobertson> mrg = createMergeRobertson ();
                    mrg->process(imgs, result, Mat(exps));
                    cout << " done!" << endl;
                } break;
                
                // Mertens
                case 'm':
                {
                    cout << "using Mertens method" << endl;
                    float contrast_weight=1.0f;
                    float saturation_weight=1.0f;
                    float exposure_weight=0.0f;
                    cout << "Params: contrast_weight=" << contrast_weight << " saturation_weight=" << saturation_weight << " exposure_weight=" << exposure_weight << endl;
                    Ptr<MergeMertens> mrg = createMergeMertens (contrast_weight, saturation_weight, exposure_weight);
                    mrg->process(imgs, result);
                    cout << " done!" << endl;
                } break;
            }
            
            cout << "writing resulting image to " << outfile << endl;
            imwrite(outfile.c_str(), result);
            
            
            
        } else if ( (strcmp( argv[1], "--tonemap" ) == 0) && (argc == 5)) {
            
            //
            //  TONEMAPPING
            //    
            
            cout << "performing tonemapping" << endl;
            
              // arg 3: mode
            char mode = '-';
            if (strcmp( argv[2], "d" ) == 0) { 
                mode = 'd'; 
            } else if (strcmp( argv[2], "u" ) == 0) { 
                mode = 'u';
            } else if (strcmp( argv[2], "r" ) == 0) { 
                mode = 'r';
            } else if (strcmp( argv[2], "m" ) == 0) { 
                mode = 'm';
            } else { 
                help(); ret=-1;
            }
            
            // arg 4: infile
            Mat img = imread(argv[3]);
            if (img.data == NULL) { 
                cout << "Error loading " << argv[3] << endl;
                return -1;
            }
            
            // arg 5: outfile
            string outfile = argv[4];
            
            
            Mat result;          // resulting HDR or LDR images
            
            switch (mode) {

                // Drago
                case 'd':
                {
                    cout << "using Drago method" << endl;
                    float gamma=1.0f;
                    float saturation=1.0f;
                    float bias=0.85f;
                    cout << "Params: gamma=" << gamma << " saturation=" << saturation << " bias=" << bias << endl;
                    Ptr<TonemapDrago> tonemap = createTonemapDrago (gamma, saturation, bias);
                    tonemap->process(img, result);
                    cout << " done!" << endl;
                } break;
                
                // Durand
                case 'u':
                {
                    cout << "using Durand method" << endl;
                    float gamma=1.0f;
                    float contrast=4.0f;
                    float saturation=1.0f;
                    float sigma_space=2.0f;
                    float sigma_color=2.0f;
                    cout << "Params: gamma=" << gamma << " contrast=" << contrast << " saturation=" << saturation << " sigma_space=" << sigma_space << " sigma_color" << sigma_color << endl;
                    Ptr<TonemapDurand> tonemap = createTonemapDurand (gamma, contrast, saturation, sigma_space, sigma_color);
                    tonemap->process(img, result);
                    cout << " done!" << endl;
                } break;
                
                // Reinhard
                case 'r':
                {
                    cout << "using Reinhard method" << endl;
                    float gamma=1.0f;
                    float intensity=0.0f;
                    float light_adapt=1.0f;
                    float color_adapt=0.0f;
                    cout << "Params: gamma=" << gamma << " intensity=" << intensity << " light_adapt=" << light_adapt << " color_adapt=" << color_adapt << endl;
                    Ptr<TonemapReinhard> tonemap = createTonemapReinhard (gamma, intensity, light_adapt, color_adapt);
                    tonemap->process(img, result);
                    cout << " done!" << endl;
                } break;
                
                // Mantiuk
                case 'm':
                {
                    cout << "using Mantiuk method" << endl;
                    float gamma=1.0f;
                    float scale=0.7f;
                    float saturation=1.0f;
                    cout << "Params: gamma=" << gamma << " scale=" << scale << " saturation=" << saturation << endl;
                    Ptr<TonemapMantiuk> tonemap = createTonemapMantiuk (gamma, scale, saturation);
                    tonemap->process(img, result);
                    cout << " done!" << endl;
                } break;
                
            }
            
            cout << "writing resulting image to " << outfile << endl;
            imwrite(outfile.c_str(), result);
            
            
        } else {
            help(); ret=-1;
        }
    }
    

    cout << PROGNAME << " finished" << endl;
    return ret;
}





