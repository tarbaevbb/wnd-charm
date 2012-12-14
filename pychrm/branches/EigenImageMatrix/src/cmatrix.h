/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                                                               */
/* Copyright (C) 2007 Open Microscopy Environment                                */
/*       Massachusetts Institue of Technology,                                   */
/*       National Institutes of Health,                                          */
/*       University of Dundee                                                    */
/*                                                                               */
/*                                                                               */
/*                                                                               */
/*    This library is free software; you can redistribute it and/or              */
/*    modify it under the terms of the GNU Lesser General Public                 */
/*    License as published by the Free Software Foundation; either               */
/*    version 2.1 of the License, or (at your option) any later version.         */
/*                                                                               */
/*    This library is distributed in the hope that it will be useful,            */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          */
/*    Lesser General Public License for more details.                            */
/*                                                                               */
/*    You should have received a copy of the GNU Lesser General Public           */
/*    License along with this library; if not, write to the Free Software        */
/*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  */
/*                                                                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                                                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Written by:  Lior Shamir <shamirl [at] mail [dot] nih [dot] gov>              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


//---------------------------------------------------------------------------
#ifndef cmatrixH
#define cmatrixH
//---------------------------------------------------------------------------

#include <vector>
#include <string> // for what_am_i definition
#include <Eigen/Dense>
#include "colors/FuzzyCalc.h"
//#define min(a,b) (((a) < (b)) ? (a) : (b))
//#define max(a,b) (((a) < (b)) ? (b) : (a))


#define INF 10E200

using namespace std;

class FeatureGroup;

typedef unsigned char byte;
typedef struct {
	byte r,g,b;
} RGBcolor;
typedef struct {
	byte h,s,v;
} HSVcolor;
typedef Eigen::Matrix< HSVcolor, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor > MatrixXhsv;
typedef MatrixXhsv clrData;

// the meaning of the color channels is specified by ColorMode, but it uses the HSVcolor structure for storage
// All color modes other than cmGRAY contain color planes as well as intensity planes
enum ColorMode { cmRGB, cmHSV, cmGRAY };


typedef Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor > pixData;
typedef struct {
	int x,y,w,h;
} rect;

//---------------------------------------------------------------------------
// global functions
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)

static inline int compare_doubles (const void *a, const void *b)
{
  if (*((double *)a) > *((double*)b)) return(1);
  if (*((double*)a) == *((double*)b)) return(0);
  return(-1);
}
static inline HSVcolor RGB2HSV(RGBcolor rgb) {
	double r,g,b,h,max,min,delta;
	HSVcolor hsv;

	r = (double)(rgb.r) / 255;
	g = (double)(rgb.g) / 255;
	b = (double)(rgb.b) / 255;

	max = MAX (r, MAX (g, b)), min = MIN (r, MIN (g, b));
	delta = max - min;

	hsv.v = (byte)(max*240.0);
	if (max != 0.0)
		hsv.s = (byte)((delta / max)*240.0);
	else
		hsv.s = 0;
	if (hsv.s == 0) hsv.h = 0; //-1;
	else {
		h = 0;
		if (r == max)
		h = (g - b) / delta;
		else if (g == max)
		h = 2 + (b - r) / delta;
		else if (b == max)
		h = 4 + (r - g) / delta;
		h *= 60.0;
		if (h >= 360) h -= 360.0;
		if (h < 0.0) h += 360.0;
		hsv.h = (byte)(h *(240.0/360.0));
	}
	return(hsv);
}
static inline RGBcolor HSV2RGB(HSVcolor hsv) {
	RGBcolor rgb;
	double R=0, G=0, B=0;
	double H, S, V;
	double i, f, p, q, t;

	H = hsv.h;
	S = (double)(hsv.s)/240;
	V = (double)(hsv.v)/240;
	if(S == 0 && H == 0) {R=G=B=V;}  /*if S=0 and H is undefined*/
	H = H*(360.0/240.0);
	if(H == 360) H=0;
	H = H/60;
	i = floor(H);
	f = H-i;
	p = V*(1-S);
	q = V*(1-(S*f));
	t = V*(1-(S*(1-f)));

	if(i==0) {R=V;  G=t;  B=p;}
	if(i==1) {R=q;  G=V;  B=p;}
	if(i==2) {R=p;  G=V;  B=t;}
	if(i==3) {R=p;  G=q;  B=V;}
	if(i==4) {R=t;  G=p;  B=V;}
	if(i==5) {R=V;  G=p;  B=q;}

	rgb.r = (byte)(R*255);
	rgb.g = (byte)(G*255);
	rgb.b = (byte)(B*255);
	return rgb;
}
static inline double RGB2GRAY(RGBcolor rgb) {
	return((0.2989*rgb.r+0.5870*rgb.g+0.1140*rgb.b));
}
//---------------------------------------------------------------------------

class ImageMatrix {
public:
	pixData pix_plane;                              // pixel plane data  
	clrData clr_plane;                              // 3-channel color data  
	//std::string what_am_i;                        // informative label
	enum ColorMode ColorMode;                       // can be cmRGB, cmHSV or cmGRAY
	unsigned short bits;                            // the number of intensity bits (8,16, etc)
	unsigned int width,height;                               // width and height of the picture
	double _min, _max, _mean, _std, _median;        // min, max, mean, std computed in single pass, median in separate pass
	bool has_stats, has_median;                     // has_stats applies to min, max, mean, std. has_median only to median
	int LoadTIFF(char *filename);                   // load from TIFF file
	int SaveTiff(char *filename);                   // save a matrix in TIF format
	int LoadPPM(char *filename, int ColorMode);     // load from a PPM file
	int OpenImage(char *image_file_name,            // load an image of any supported format
		int downsample, rect *bounding_rect,
		double mean, double stddev);
	// constructor helpers
	void 	init();
	void 	allocate (unsigned int w, unsigned int h);
	void 	copy(ImageMatrix *copy);
	ImageMatrix();                                  // basic constructor
	ImageMatrix(ImageMatrix *matrix);               // copy constructor

	ImageMatrix(unsigned int width,unsigned int height);              // construct a new empty, allocated matrix
	ImageMatrix(ImageMatrix *matrix,                // create a new matrix which is part of the original one
		unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
	~ImageMatrix();                                 // destructor

// set pixel value based on passed-in parameter type
	inline void set (unsigned int x, unsigned int y, RGBcolor val) {
		pix_plane (y,x) = RGB2GRAY (val);
		if (ColorMode == cmHSV) {
			HSVcolor hsv = RGB2HSV(val);
			clr_plane(y,x) = hsv;
		} else if (ColorMode == cmRGB) {
			HSVcolor hsv = {val.r, val.g, val.b};
			clr_plane(y,x) = hsv;
		}
	}
	inline void set (unsigned int x, unsigned int y, double val) {
		pix_plane (y,x) = val;
	}

	void diff(ImageMatrix *matrix);                 // compute the difference from another image
	void normalize(double min, double max, long range, double mean, double stddev); // normalized an image to either min/max or mean/stddev
	void to8bits();
	void flipV();                                   // flip an image around a vertical axis (left to right)
	void flipH();                                   // flip an image around a horizontal axis (upside down)
	void invert();                                  // invert the intensity of an image
	void Downsample(double x_ratio, double y_ratio);// down sample an image
	ImageMatrix *Rotate(double angle);              // rotate an image by 90,180,270 degrees
	void convolve(ImageMatrix *filter);
	void BasicStatistics(double *mean, double *median, double *std, double *min, double *max, double *histogram, int bins);
	inline double min() {
		if (!has_stats) {
			double var;
			BasicStatistics (&var, NULL, &var, &var, &var, NULL, 0);
		}
		return (_min);
	}
	inline double max() {
		if (!has_stats) {
			double var;
			BasicStatistics (&var, NULL, &var, &var, &var, NULL, 0);
		}
		return (_max);
	}
	inline double mean() {
		if (!has_stats) {
			double var;
			BasicStatistics (&var, NULL, &var, &var, &var, NULL, 0);
		}
		return (_mean);
	}
	inline double std() {
		if (!has_stats) {
			double var;
			BasicStatistics (&var, NULL, &var, &var, &var, NULL, 0);
		}
		return (_std);
	}
	inline double median() {
		if (!has_median) {
			double var;
			BasicStatistics (NULL, &var, NULL, NULL, NULL, NULL, 0);
		}
		return (_median);
	}
	void GetColorStatistics(double *hue_avg, double *hue_std, double *sat_avg, double *sat_std, double *val_avg, double *val_std, double *max_color, double *colors);
	void ColorTransform(double *color_hist, int use_hue);
	void histogram(double *bins,unsigned short bins_num, int imhist);
	double Otsu();                                  // Otsu gray threshold
	void MultiScaleHistogram(double *out);
	//   double AverageEdge();
	void EdgeTransform();                           // gradient binarized using otsu threshold
	double fft2();
	void ChebyshevTransform(unsigned int N);
	void ChebyshevFourierTransform2D(double *coeff);
	void Symlet5Transform();
	void GradientMagnitude(int span);
	void GradientDirection2D(int span);
	void PerwittMagnitude2D(ImageMatrix *output);
	void PerwittDirection2D(ImageMatrix *output);
	void ChebyshevStatistics2D(double *coeff, unsigned int N, unsigned int bins_num);
	int CombFirstFourMoments2D(double *vec);
	void EdgeStatistics(unsigned long *EdgeArea, double *MagMean, double *MagMedian, double *MagVar, double *MagHist, double *DirecMean, double *DirecMedian, double *DirecVar, double *DirecHist, double *DirecHomogeneity, double *DiffDirecHist, unsigned int num_bins);
	void RadonTransform2D(double *vec);
	double OtsuBinaryMaskTransform();
	unsigned long BWlabel(int level);
	void centroid(double *x_centroid, double *y_centroid);
	void FeatureStatistics(unsigned long *count, unsigned long *Euler, double *centroid_x, double *centroid_y, unsigned long *AreaMin, unsigned long *AreaMax,
		double *AreaMean, unsigned int *AreaMedian, double *AreaVar, unsigned int *area_histogram,double *DistMin, double *DistMax,
		double *DistMean, double *DistMedian, double *DistVar, unsigned int *dist_histogram, unsigned int num_bins
	);
	void GaborFilters2D(double *ratios);
	void HaralickTexture2D(double distance, double *out);
	void TamuraTexture2D(double *vec);
	void zernike2D(double *zvalues, long *output_size);
};

#endif
