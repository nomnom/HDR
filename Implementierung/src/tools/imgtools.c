/*
 * simple image tools
 *
 * @author Manuel Jerger
 *
 */

#include "image.h"
#include <string>
#include <getopt.h>
#include <stdlib.h>
#include <algorithm>

using namespace std;
using namespace imglib;


struct rgb { rgb() : r(0), g(0), b(0) {}; float r; float g; float b; };

enum MODE { NONE, ADD, SUB, MUL, MIN, MAX, MEDIAN, THRESH, LSD, DIFF, STATS, HIST, PLOT, HEIGHTMAP, AVG, TOLUMINANCE};
int mode = NONE;

// input/output images
string outfile;
vector<Image<float> > infiles;

// command line parameters
float minthresh=0;
int size;
bool normalize=false;
float scale=0;
bool whitepoint=false;
rgb wp;
bool mapInput=false;
rgb lsd;
// 1d slice
int plotpos;
int plotdir = 0;
// dump pixel values
int channel = 0;
int patchsize = 1;

int histsize;

// response curves
string responseFile;
int responseSize;
float* responseCurve [3];

// add B to A // modifies A
Image<float>& imgAdd(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)+B(x,y,c);

    return A;
}

// subtract B from A // modifies A
Image<float>& imgSub(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)-B(x,y,c);

    return A;
}


// multiply two images // modifies A
Image<float>& imgMul(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)*B(x,y,c);
    return A;
}


// multiply by a scalar // modifies A
Image<float>& imgMulScalar(Image<float>& A, float scalar)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c) * scalar;

    return A;
}



// get maximum pixel value (across all channels)
float imgMax(Image<float>& A) {
    float maxVal = 0;
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                maxVal = A(x,y,c) > maxVal ? A(x,y,c) : maxVal;
    return maxVal;
}

// get minimum pixel value (across all channels)
float imgMin(Image<float>& A) {
    float minVal = 1e20;
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                minVal = A(x,y,c) < minVal ? A(x,y,c) : minVal;
    return minVal;
}

// clamp between 0 .. 1
Image<float>& clamp(Image<float>& A)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++ ) {
                if (A(x,y,c) < 0) A(x,y,c)=0;
                if (A(x,y,c) > 1) A(x,y,c)=1;
            }
    return A;
}

// scale max value to 1.0; preserves color // modifies A
Image<float>& imgScaleMax(Image<float>& A)
{
    imgMulScalar(A, 1.0/imgMax(A));
    return A;
}


//(2n+1)x(2n+1) min filter
Image<float>& minmax(Image<float>& A, int n, bool usemax)
{
    Image<float> O (A);
    float val;
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
            {
                val = usemax ? 0 : 1e20;
                for (int dx=-n; dx<=n; dx++)
                    for (int dy=-n; dy<=n; dy++)
                        if ( (x+dx >= 0) && (x+dx < A.getWidth()) && (y+dy >= 0) && (y+dy < A.getHeight()) )
                            if (usemax) { val = (O(x+dx,y+dy,c)>val) ? O(x+dx,y+dy,c) : val; }
                            else { val = (O(x+dx,y+dy,c)<val) ? O(x+dx,y+dy,c) : val; }

                A(x,y,c) = val;
            }
    return A;
}



//(2n+1) x (2n+1) median filter
Image<float>& median(Image<float>& A, int n)
{
    Image<float> O (A);

    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
            {
                vector<float> vals;
                for (int dx=-n; dx<=n; dx++)
                    for (int dy=-n; dy<=n; dy++)
                        if ( (x+dx >= 0) && (x+dx < A.getWidth()) && (y+dy >= 0) && (y+dy < A.getHeight()) )
                            vals.push_back( O(x+dx,y+dy,c) );

                // sort vector and pic center element
                sort(vals.begin(), vals.end());
                if (vals.size() % 2 == 0) {
                    A(x,y,c) = vals[vals.size() / 2 - 1] / 2 + vals[vals.size() / 2] / 2;
                } else {
                    A(x,y,c) = vals[vals.size()/2];
                }
            }
    return A;
}

//lower threshold
Image<float>& threshold (Image<float>& A, float minval)
{
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
                A(x,y,c) = (A(x,y,c)<minval) ? 0 : A(x,y,c);
    return A;
}



// sum of square differences // modifies A and sets per-pixel-residues
rgb sumLSD (Image<float>& A, Image<float> B)
{
    rgb sum;
    int numSamples = 0;

    // over all pixels
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++) {
            for (int c=0; c<3; c++)
                A(x,y,c) = (A(x,y,c) - B(x,y,c)) * (A(x,y,c) - B(x,y,c));

            sum.r += A(x,y,0);
            sum.g += A(x,y,1);
            sum.b += A(x,y,2);
            numSamples++;
        }


    sum.r /= (float)numSamples;
    sum.g /= (float)numSamples;
    sum.b /= (float)numSamples;
    return sum;
}



Image<float>& diff(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<3; c++)
                A(x,y,c) = abs(A(x,y,c) - B(x,y,c));
    return A;
}


// rgb to luminance; modifies A
Image<float>& toLuminance(Image<float>& A)
{
   assert (A.getNumChannels() == 3);

    // TODO

}

// no treshold for now
#define FLOOR_NOISE_THRESHOLD  (0.000)
#define HIGHLIGHT_TRESHOLD (1.0)

// apply response curve to value
float linearize(float value, int channel) {
    assert (value >= 0.0);
    assert (value <= 1.0);

    if (value < FLOOR_NOISE_THRESHOLD) value=0;
    if (value > HIGHLIGHT_TRESHOLD) value = HIGHLIGHT_TRESHOLD;  // clips the canon response curve to the linear part

    float result;
    int idx=int(round(value*float(responseSize-1)));
    assert (idx>=0);
    assert (idx < responseSize);
    result = responseCurve[channel][ idx ];

    return result;
}

//map with response curve
Image<float>& map (Image<float>& A)
{
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
                A(x,y,c) = linearize(A(x,y,c),c);
    return A;
}

float clamp (float val) {
    if (val < 0.0) val = 0.0;
    if (val > 1.0) val = 1.0;
    return val;
}

rgb clamp (rgb val) {
    if (val.r < 0.0) val.r = 0.0;
    if (val.r > 1.0) val.r = 1.0;
    if (val.g < 0.0) val.g = 0.0;
    if (val.g > 1.0) val.g = 1.0;
    if (val.b < 0.0) val.b = 0.0;
    if (val.b > 1.0) val.b = 1.0;
    return val;
}

// linear scale value v, so that whitepoint wp is mapped to 1.0 and bp is mapped to 0.0
float mapLinear (float val, float wp, float bp) {
    return 1/(wp-bp) * val + bp/(bp-wp);
}
rgb mapLinear (rgb val, rgb wp, rgb bp) {
    rgb result;
    result.r = mapLinear(val.r, wp.r, bp.r);
    result.g = mapLinear(val.g, wp.g, bp.g);
    result.b = mapLinear(val.b, wp.b, bp.b);
    return result;
}

Image<float> mapImgLinear ( Image<float> A, rgb wp)
{
   for (int x=0; x<A.getWidth(); x++)
      for (int y=0; y<A.getHeight(); y++) {
         A(x,y,0) = mapLinear(A(x,y,0), wp.r, 0);
         A(x,y,1) = mapLinear(A(x,y,1), wp.g, 0);
         A(x,y,2) = mapLinear(A(x,y,2), wp.b, 0);
      }

   return A;
}

// loads rgb response curve (either .m format created with hdrcalibrate, or a three-column list w/o indices)
void loadResponseCurve (string filename)
{
    ifstream in;
    in.open(filename.c_str(), ios::in);
    vector<rgb> values;

    // fileextension determines format
    string suffix = ".m";
    if (std::equal(filename.begin() + filename.size() - 2, filename.end(), suffix.begin() ) ) {

        // load .m format
        int color=-1; // current color
        int numRows=0;
        string tmp;
        while (!in.eof()) {
            getline (in,tmp);
            if (tmp.c_str()[0] == '#') {
                if (tmp.length() == 35)  {
                    if (tmp.substr(25,10) == "channel IR") { color = 0; }
                    else if (tmp.substr(25,10) == "channel IG") { color = 1; }
                    else if (tmp.substr(25,10) == "channel IB") { color = 2; }
                } else if (tmp.length() > 7) {

                    if (tmp.substr(0,7) == "# rows:") {
                        stringstream ss; ss << tmp.substr(8); ss >> numRows;

                        if (values.size() < numRows) {
                            for (int i=values.size(); i<numRows; i++) {
                                values.push_back(rgb());
                            }
                        }
                    }
                }
            } else if (color >= 0 && numRows > 0) {
                float logVal, val;
                int index;
                for (int i=0; i<numRows; i++) {

                    stringstream ss (tmp);

                    ss >> logVal; ss >> index; ss >> val;

                    switch (color) {
                        case 0: values[index].r = val; break;
                        case 1: values[index].g = val; break;
                        case 2: values[index].b = val; break;
                        default: break;
                    }

                    getline(in, tmp);
                }
                if (color == 2) break;
            }
        }
        responseSize = numRows;
    } else {
        // assume values in three columns
        rgb tmp;
        while (in >> tmp.r && in >> tmp.g && in >> tmp.b) {
            values.push_back (tmp);
        }
        responseSize = values.size();
    }

    in.close();
    responseCurve[0] = new float [responseSize];
    responseCurve[1] = new float [responseSize];
    responseCurve[2] = new float [responseSize];

    for (int i=0; i<responseSize; i++) {
        responseCurve[0][i] = values[i].r;
        responseCurve[1][i] = values[i].g;
        responseCurve[2][i] = values[i].b;
        //cout << ">>>>" << i << " : " << values[i].r << " " <<  values[i].g << " " <<  values[i].b << endl;
    }

    // fits image range of response curve to  (0:1);
	float maxVal = max(max(responseCurve[0][responseSize-1], responseCurve[1][responseSize-1]), responseCurve[2][responseSize-1]);
	float minVal = max(max(responseCurve[0][0], responseCurve[1][0]), responseCurve[2][0]);
	for (int c=0; c<3; c++) {
		for (int i=0; i<responseSize; i++) {
			responseCurve[c][i] = clamp(mapLinear(responseCurve[c][i], maxVal, minVal));
		}
	}
   cout << "loaded response curve of size " << responseSize  << " from " << filename << endl;
}


void dumpPixelLine (Image<float>& A, int pos, char dir)
{
  // each channel separate
  for (int c=0; c<A.getNumChannels(); c++) {
    cout << "channel " << c << endl;

    // horizontal
    if (dir == 0) {
        assert ( (pos >= 0) && (pos < A.getHeight()) );
        for (int x=0; x<A.getWidth(); x++) {
            cout << A(x,pos,c) << " ";
        }
        cout << endl;

    // vertical
    } else {

        assert ( (pos >= 0) && (pos < A.getWidth()) );
        for (int y=0; y<A.getHeight(); y++) {
            cout << A(pos,y,c) << " ";
        }
        cout << endl;
    }
  }
}



void dumpPixels(Image<float>& A, int channel, int patchsize, string outfile)
{
  assert (channel >= 0 && channel <= A.getNumChannels());
  ofstream of;
  of.open(outfile.c_str());
  int sizex = A.getWidth()/patchsize;
  int sizey = A.getHeight()/patchsize;

  float patchval; int numsamples;
  for (int x=0; x<sizex; x++) {
     for (int y=0; y<sizey; y++) {
      if (patchsize>1) {
          patchval=0; numsamples=0; // note: patches on the borders may be smaller than patchsize
          for (int px=0; px<patchsize && x*patchsize+px < A.getWidth(); px++) {
             for (int py=0; py<patchsize && y*patchsize+py < A.getHeight(); py++) {
                patchval += A(x*patchsize+px,y*patchsize+py,channel);
                numsamples++;
             }
          }
          patchval /= (float)numsamples;
          of << x << " " << y << " " << patchval << endl;
      } else {
          of << x << " " << y << " " << A(x,y,channel) << endl;
      }
     }
     of << endl; // gnuplot pm3d spacer
  }


  of.close();
}

void printStats (Image<float>& A)
{
    assert (A.getNumChannels() == 3);
    float vmin[4], vmax[4], avg[4];

    for (int c=0; c<3; c++) {
        vmin[c]=1e20;
        vmax[c]=0;
        avg[c]=0;
        int avgnum=0;
        for (int x=0; x<A.getWidth(); x++) {
            for (int y=0; y<A.getHeight(); y++) {
                vmin[c] = A(x,y,c) < vmin[c] ? A(x,y,c) : vmin[c];
                vmax[c] = A(x,y,c) > vmax[c] ? A(x,y,c) : vmax[c];
                avg[c] += A(x,y,c);
                avgnum++;
            }
        }
        avg[c] /= (float)avgnum;
    }
    vmin[3] = min(vmin[0], min(vmin[1],vmin[2]));
    vmax[3] = max(vmax[0], max(vmax[1],vmax[2]));
    avg[3] = (avg[0]+avg[1]+avg[2]) / 3.0;

    cout << " minrgb: " << vmin[0] << " " << vmin[1] << " " << vmin[2] << endl;
    cout << " min: " << vmin[3] << endl;
    cout << " maxrgb: " << vmax[0] << " " << vmax[1] << " " << vmax[2] << endl;
    cout << " max: " << vmax[3] << endl;
    cout << " avgrgb: " << avg[0] << " " << avg[1] << " " << avg[2] << endl;
    cout << " avg: " << avg[3] << endl;
}

////////// CLI parsing  ////////////////////////////////////////////////
void parseArgs(int argc, char *argv[])
{
    int option_index = 0, c=0;
    while ( ( c = getopt (argc, argv, "i:o:ASMn:w:C:O:m:t:lr:dh:H:V:saX:L")) != -1)
    {
        string tmp;
        stringstream arg;
        if (c) arg << (optarg);
        switch (c) {
           case 'i': arg >> tmp; cout << "loading " << tmp << endl; infiles.push_back(Image<float>(tmp));break;
           case 'o': arg >> outfile; break;
           case 'A': mode=ADD; break;
           case 'S': mode=SUB; break;
           case 'M': mode=MUL; break;
           case 'n':  arg >> scale; normalize=true; break;
           case 'w':  arg >> wp.r; arg >> wp.g; arg >> wp.b; whitepoint=true; break;
           case 'C': mode=MIN; arg >> size; break;
           case 'O': mode=MAX; arg >> size; break;
           case 'm': mode=MEDIAN; arg >> size; break;
           case 't': mode=THRESH; arg >> minthresh; break;
           case 'l': mode=LSD; break;
           case 'r': mapInput=true; arg >> responseFile; loadResponseCurve(responseFile); break;
           case 'd': mode=DIFF;  break;
           case 'h': mode=HIST; arg >> histsize; break;
           case 'H': mode=PLOT; plotdir=0; arg >> plotpos; break;
           case 'V': mode=PLOT; plotdir=1; arg >> plotpos; break;
           case 'X': mode=HEIGHTMAP; arg >> channel; arg >> patchsize; break;
           case 's': mode=STATS; break;
           case 'a': mode=AVG; break;
           case 'L': mode=TOLUMINANCE; break;
        }
    }

}


////////// MAIN  ///////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    // process commandline
    parseArgs(argc, argv);

    // map input images

    if (mapInput) {
        cout << "mapping input images via response curve" << endl;
        for (int i=0; i<infiles.size(); i++) {
            infiles[i] = map(infiles[i]);
        }
    }
    if (whitepoint) {
        cout << "correcting whitepoint ("<<wp.r<<" "<<wp.g<<" "<<wp.b<<")";
        for (int i=0; i<infiles.size(); i++) {
            infiles[i] = mapImgLinear(infiles[i], wp);
        }
    }

    Image<float> result (infiles[0]);
    switch (mode) {
        case ADD:
                    cout << "adding " << infiles.size() << " images" << endl;
                    for (int i=1; i<infiles.size(); ++i) { imgAdd(result, infiles[i]); }
                    break;
        case SUB:
                    cout << "subtracting " << (infiles.size()-1) << " images from the first" << endl;
                    for (int i=1; i<infiles.size(); ++i) { imgSub(result, infiles[i]); }
                    break;
        case MUL:
                    cout << "multiplying " << (infiles.size()) << " images" << endl;
                    for (int i=1; i<infiles.size(); ++i) { imgMul(result, infiles[i]); }
                    break;
        case MIN:
                    cout << "running "<<2*size+1<<"x"<<2*size+1<<" min filter on first image" << endl;
                    result = minmax(result,size,false);
                    break;
        case MAX:
                    cout << "running "<<2*size+1<<"x"<<2*size+1<<" max filter on first image" << endl;
                    result = minmax(result,size,true);
                    break;
        case MEDIAN:
                    cout << "running "<<2*size+1<<"x"<<2*size+1<<" median filter on first image" << endl;
                    result = median(result,size);
                    break;
        case THRESH:
                    cout << "tresholding image with " << minthresh << endl;
                    threshold (result, minthresh);
                    break;
        case LSD:
                    cout << "calculating least square difference on pixels of first two images" << endl;
                    assert (infiles.size() >= 2);
                    lsd = sumLSD(result, infiles[1]);
                    cout << "LSD is " << (lsd.r+lsd.g+lsd.b)/3.0 << " (r=" << lsd.r << " g=" << lsd.g << " b=" << lsd.b << ")" << endl;
                    break;
        case DIFF:
                    cout << "diffing first two input images" << endl;
                    assert (infiles.size() >= 2);
                    result = diff (infiles[0], infiles[1]);
                    break;
        case HIST:
                    cout << "histogram values (" << histsize << "buckets):" << endl;
                    // TODO histogram.. use opencv??
                    outfile="";
                    break;
        case PLOT:
                    cout << "printing pixel values of the " << plotpos << "'th " << ( (plotdir==0)?"row":"column") << endl;
                    for (int i=0; i<infiles.size(); ++i) {
                       cout << "image " << i << ":" << endl;
                       dumpPixelLine (infiles[i], plotpos, plotdir);
                    }
                    outfile="";
                    break;
        case HEIGHTMAP:
                    cout << "dumping pixel values of the first image (channel " << channel << ", patchsize " << patchsize << ") to " << outfile << endl;
                    dumpPixels(infiles[0], channel, patchsize, outfile);
                    outfile="";
                    break;
        case STATS:
                    for (int i=0; i<infiles.size(); ++i) {
                        cout << "image statistics for input image " << i << ":" << endl;
                        printStats(infiles[i]);
                    }
                    outfile="";
                    break;

        case AVG:
                    cout << "calculating average over " << infiles.size() << " images" << endl;
                    for (int i=1; i<infiles.size(); ++i) { imgAdd(result, infiles[i]); }
                    imgMulScalar(result, 1.0 / (float)infiles.size());
                    break;

        default: cout << "no mode specified" << endl; break;
    }

    if (not outfile.empty()) {
        if (normalize) {
            cout << "rescaling result";
            if (scale == 0) {
                cout << " automatically" << endl;
                imgScaleMax(result);
            } else {
                cout << " with " << scale << endl;
                imgMulScalar(result,scale);
            }
        } else {
            cout << "clamping result" << endl;
            clamp(result);
        }
        cout << "writing result to " << outfile << endl;
        result.save(outfile);
    }
    return 0;
}





