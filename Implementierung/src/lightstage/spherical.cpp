// spherical environment map sampling
#include "spherical.h"

// constructor
SphericalEnvMap::SphericalEnvMap (Mat _envMap, 
                                  SVRInfo _svr, 
                                  Size _screenSizePixel, 
                                  Size _screenSizeMm)
 :svr(_svr), 
  screenSizePixel(_screenSizePixel), 
  screenSizeMm(_screenSizeMm),
  envMapOriginal(_envMap)
{
    envMapOriginal.copyTo(envMap);
    
    // size of pixels
    delX = (double)screenSizeMm.width/(double)screenSizePixel.width;
    delY = (double)screenSizeMm.height/(double)screenSizePixel.height;
    cout << delX << " " << delY << endl;

    // calculate maximum relative radiance we can produce at each pixel;
    maxLight = Mat(svr.screenSize-svr.borderSize, CV_32FC3, CV_RGB(1,1,1));
    apply_response_svr (maxLight, svr);
    resize(maxLight, maxLight, screenSizePixel); // linear interpolated


    // for calculating the size of environment map sphere surface element (delta phi and delta theta)
    delPhi  = M_PI  / envMap.size().height;
    delTheta  = M_2_PI / envMap.size().width;
    
    screen = Mat::zeros(screenSizePixel, CV_32FC3);
    

}

SphericalEnvMap::~SphericalEnvMap () {}


    //////////////////////////////
    //                          //
    //  HDR EnvMap code (core)  //
    //                          //
    //////////////////////////////
    

/////////////////////

// Method A: pixel-wise
//
//  foreach pixel on display:
//    1. find closest light direction on environment map
//    2. calculate maximum possible radiance m: 
//       if m < 1.0 : p=r^-1(m)
//       else : p=1.0;
//    3. subtract from envmap
//    4. display (flash) image

/////////////////////

// Method B: pixel-wise with neighborhood awareness
//
//  foreach pixel on display:
//    1. find closest light direction on environment map
//    2. save into neighborhood struct
//  foreach neighborhood: 
//    1. calculate area (number_of_pixels * pixel_area)
//    2. calculate maximum possible radiance m of this neighborhood: 
//       if m < 1.0 : p=r^-1(m)
//       else : p=1.0;
//    3. subtract light from envmap
//    4. display (flash) image

/////////////////////

// Method A:
// perform one HDR illumination step; return screen image
Mat& SphericalEnvMap::illuminate (Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{


    // screen normal    // vvv matx misses some operations?!
    Matx31d screenNormal = Mat(down).cross(right); 
    
    //
    // iterate over  display pixels  (efficient using row pointers)
    //
    
    Vec3f *ps;  // pointer to a data row in screen 
    Vec3f *pe;  // pointer to a data row in envmap
    Vec3f lReq; // reqired light at envmap point (remaining light)
    Vec3f lMax; // maximum of light we can produce
    
    Matx31d posCart, pixelPosY; // position in cartesian coordinates; separate up-vector component for efficiency
    Matx31d posSpher;           // pixel coordinates in spherical coordinates
    Point2i envMapPos;          // point on envmap
    
    for (int y=0; y<screenSizePixel.height; y++) {
        
        // update row pointers and calculate up-vector component of pixel world coordinate
        pixelPosY = down * ((double)(y-screenSizePixel.height/2) + 0.5) * delY*2;
        ps=screen.ptr<Vec3f>(int(y+0.5));

        for (int x=0; x<screenSizePixel.width; x++) {
        
            // calculate pixel world coordinate
            posCart = screenCenter +  pixelPosY + right * ((double)(x-screenSizePixel.width/2) + 0.5) * delX*2; 
            posSpher = cart2spher(posCart);
            
            // calculate closest point in envmap
            envMapPos.x =  (int)(0.5 + ( (posSpher(2)/M_PI + 1.0)/2.0 * (float)envMap.size().width-1) );
            envMapPos.y =  (int)(0.5 + (  posSpher(1)/M_PI            * (float)envMap.size().height-1));
            pe=envMap.ptr<Vec3f>(envMapPos.y);
            
            
            
            // get remaining radiant intensity from envmap
            //     radiance        * area of sphere surface element (A = r^2 * sin(phi) * delta phi * delta theta
            // * (posSpher(0)*posSpher(0) * sin(posSpher(1)) * delPhi * delTheta);
            
            // No! just use radiance, but apply sin(phi) to  envmap
            lReq = pe[envMapPos.x] * sin(posSpher(1)); // SPEEDUP can be done beforehand
            
            
            //
            // calculate how much light we can produce with our display pixel :
            //
            //  -> the envmap is modeled as a sphere surface and the display as a flat light source 
            //  -> we know the radiance, area and orientation and of both elements (on on the sphere and on the display)
            //  -> assumes sources emit directional light in direction of origin
            //  
            // area of pixel is: pw*ph
            // approximated area of envmap element is:  sin(phi)*delta_phi*delta_  
            // (error is minimal; correct solution would use sphere cap area, we use surface area)
            //  radiant intensity equation:
            //  radiance_pixel * cos (phi) * area_pixel === radiance_envmap * cos(0) * area_envmap_element 
            // 
            // 
            
            Matx31d envMapNormal = (-posCart);
            double cosPhi = screenNormal.dot(envMapNormal) / (norm(screenNormal) * norm(envMapNormal));
            
            // new (correct?):
            lMax = maxLight.ptr<Vec3f>(y)[x] * cosPhi;
            
            
            for (int c=0; c<3; c++) {
                
                if (lReq[c] <= 0) {
                    ps[x] = 0;
                    continue;
                }
            
                // if more light required than possible in one frame, use the maximum value and subtract from environment map
                if ( lMax[c] < lReq[c] ) {
                
                    pe[envMapPos.x][c] -= lMax[c];// / sin(posSpher(1)) * delPhi * delTheta;
                    ps[x][c] = 1.0;
                
                // if less light is required than possible, get pixel value by applying response curve and set environment map to zero
                } else {
                    
                    ps[x][c] = apply_response_svr_subpixel( lReq[c], svr, (double)x, (double)y, c );

  //                  assert ( ps[x][c] >= 0 && ps[x][c] <= 1.0);
                    pe[envMapPos.x][c] = 0;

                }
                
                    
            }
            
            

            
        }
    }

    return screen;
}



// just show the environment map on the screen
// note: envmap dynamic range has to fit into display range!
Mat& SphericalEnvMap::showEnvironment (Matx31d& screenCenter, Matx31d& down, Matx31d& right)
{



    // screen normal    // vvv matx misses some operations?!
   // Matx31d screenNormal = Mat(down).cross(right); 
    //
    // iterate over  display pixels  (efficient using row pointers)
    //
    
    Vec3f *ps;  // pointer to a data row in screen 
    Vec3f *pe;  // pointer to a data row in envmap
    
    Matx31d posCart, pixelPosY; // position in cartesian coordinates; separate up-vector component for efficiency
    Matx31d posSpher;           // pixel coordinates in spherical coordinates
    Point2i envMapPos;          // point on envmap
    
    for (int y=0; y<screenSizePixel.height; y++) {
        
        // update row pointers and calculate up-vector component of pixel world coordinate
        pixelPosY = down * ((double)(y-screenSizePixel.height/2) + 0.5) * delY*2;
        ps=screen.ptr<Vec3f>(y);
       
        for (int x=0; x<screenSizePixel.width; x++) {
           // double cosPhi = screenNormal.dot(envMapNormal) / (norm(screenNormal) * norm(envMapNormal));
           
            // calculate pixel world coordinate
            posCart = screenCenter +  pixelPosY + right * ((double)(x-screenSizePixel.width/2) + 0.5) * delX*2; 
            posSpher = cart2spher(posCart);
            
            // calculate closest point in envmap
            envMapPos.x =  (int)(0.5 + ( (posSpher(2)/M_PI + 1.0)/2.0 * (float)envMap.size().width-1) );
            envMapPos.y =  (int)(0.5 + (  posSpher(1)/M_PI            * (float)envMap.size().height-1));
            
            pe=envMap.ptr<Vec3f>(envMapPos.y);
            
            for (int c=0; c<3; c++) {
                //ps[x][c] = apply_response_svr_subpixel(  pe[envMapPos.x][c], svr, Point(x,y), c );
                ps[x][c] = pe[envMapPos.x][c];
            }
        }
    }

    return screen;
}


// if illumination is completed (= envmap is zero)
bool SphericalEnvMap::isComplete ()
{
    const double epsilon = 1e-4;
    Vec3f *pe; // pointer to one row
    Vec3f val;
    for (int r=0; r<envMap.rows; r++ ) {
        pe =envMap.ptr<Vec3f>(r);
        for (int c=0; c<envMap.cols; r++ ) {
            if (pe[c][0] > epsilon || 
                pe[c][1] > epsilon || 
                pe[c][2] > epsilon) 
            {
                return false;
            }
        }
    }
    return true;
}
    
