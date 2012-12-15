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


#include <vector>
#include <math.h>
#include <stdio.h>
#include "cmatrix.h"
#include "colors/FuzzyCalc.h"
#include "transforms/fft/bcb_fftw3/fftw3.h"
#include "transforms/chebyshev.h"
#include "transforms/ChebyshevFourier.h"
#include "transforms/wavelet/Symlet5.h"
#include "transforms/wavelet/DataGrid2D.h"
#include "transforms/wavelet/DataGrid3D.h"
#include "transforms/radon.h"
#include "statistics/CombFirst4Moments.h"
#include "statistics/FeatureStatistics.h"
#include "textures/gabor.h"
#include "textures/tamura.h"
#include "textures/haralick/haralick.h"
#include "textures/zernike/zernike.h"

#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <time.h>
#include <sys/time.h>

#ifndef WIN32
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>
#else
#include "libtiff32/tiffio.h"
#endif


using namespace std;

//-----------------------------------------------------------------------






/* LoadTIFF
   filename -char *- full path to the image file
*/
int ImageMatrix::LoadTIFF(char *filename) {
	unsigned int h,w,x,y;
	unsigned short int spp,bps;
	TIFF *tif = NULL;
	unsigned char *buf8;
	unsigned short *buf16;
	double max_range = pow((double)2,bits)-1;
	RGBcolor rgb;
	TIFFSetWarningHandler(NULL);
	if( (tif = TIFFOpen(filename, "r")) ) {
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		width = w;
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		height = h;
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
		bits=bps;
		if ( ! (bits == 8 || bits == 16) ) return (0); // only 8 and 16-bit images supported.
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
		if (!spp) spp=1;  /* assume one sample per pixel if nothing is specified */
		if ( TIFFNumberOfDirectories(tif) > 1) return(0);   /* get the number of slices (Zs) */

		/* allocate the data */
		allocate (width, height);

		/* read TIFF header and determine image size */
		buf8 = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(tif)*spp);
		buf16 = (unsigned short *)_TIFFmalloc( (tsize_t)sizeof(unsigned short)*TIFFScanlineSize(tif)*spp );
		for (y = 0; y < height; y++) {
			int col;
			if (bits==8) TIFFReadScanline(tif, buf8, y);
			else TIFFReadScanline(tif, buf16, y);
			x=0;col=0;
			while (x<width) {
				unsigned char byte_data;
				unsigned short short_data;
				double val=0;
				int sample_index;
				for (sample_index=0;sample_index<spp;sample_index++) {
					byte_data=buf8[col+sample_index];
					short_data=buf16[col+sample_index];
					if (bits==8) val=(double)byte_data;
					else val=(double)(short_data);
					if (spp==3) {  /* RGB image */
						if (sample_index==0) rgb.r=(unsigned char)(255*(val/max_range));
						if (sample_index==1) rgb.g=(unsigned char)(255*(val/max_range));
						if (sample_index==2) rgb.b=(unsigned char)(255*(val/max_range));
					}
				}
				if (spp==3) set (x, y, rgb);
				else set (x, y, val);
			x++;
			col+=spp;
			}
		}
		_TIFFfree(buf8);
		_TIFFfree(buf16);
		TIFFClose(tif);
	} else return(0);

	return(1);
}

/*  SaveTiff
    Save a matrix in TIFF format (16 bits per pixel)
*/
int ImageMatrix::SaveTiff(char *filename) {
	unsigned int x,y;
	TIFF* tif = TIFFOpen(filename, "w");
	if (!tif) return(0);
	unsigned short *BufImage16 = new unsigned short[width*height];
	if (!BufImage16) {
		TIFFClose(tif);
		return (0);
	}
	unsigned char *BufImage8 = new unsigned char[width*height];
	if (!BufImage8) {
		TIFFClose(tif);
		delete BufImage16;
		return (0);
	}


	for (y = 0; y < height; y++)
		for (x = 0; x < width ; x++) {
			if (bits==16) BufImage16[x + (y * width)] = (unsigned short) ( pix_plane (y,x) );
			else BufImage8[x + (y * width)] = (unsigned char) ( pix_plane (y,x) );
		}

	TIFFSetField(tif,TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(tif,TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG,1);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, 1);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, 1);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);

	for (y = 0; y < height; y ++) {
		if (bits==16) TIFFWriteScanline (tif, &(BufImage16[y*width]), y,0 );
		else TIFFWriteScanline (tif, &(BufImage8[y*width]), y,0 );
	}

	TIFFClose(tif);
	delete BufImage16;
	delete BufImage8;
	return(1);
}

int ImageMatrix::OpenImage(char *image_file_name, int downsample, rect *bounding_rect, double mean, double stddev) {  
	int res=0;
	if (strstr(image_file_name,".tif") || strstr(image_file_name,".TIF")) {  
		res=LoadTIFF(image_file_name);
	}
	
	// add the image only if it was loaded properly
	if (res) {
		// compute features only from an area of the image
		if (bounding_rect && bounding_rect->x>=0) { 
			ImageMatrix *temp;
			temp = new ImageMatrix(this,
				bounding_rect->x, bounding_rect->y,
				bounding_rect->x+bounding_rect->w-1, bounding_rect->y+bounding_rect->h-1
			);
			copy (temp);
			delete temp;
		}
		if (downsample>0 && downsample<100)  /* downsample by a given factor */
			Downsample(((double)downsample)/100.0,((double)downsample)/100.0);   /* downsample the image */
		if (mean>0)  /* normalize to a given mean and standard deviation */
			normalize(-1,-1,-1,mean,stddev);
	}
	return(res);
}

/* simple constructors */

// This sets default values for the different constructors
void ImageMatrix::init() {
	width=0;
	height=0;
	has_stats = false;
	has_median = false;
	ColorMode=cmHSV;     /* set a default color mode */
	bits=8; /* set some default value */
}

void ImageMatrix::allocate (unsigned int w, unsigned int h) {
	width  = 0;
	height = 0;
	// These throw exceptions, which we don't catch (catch in main?)
	// N.B. Eigen matrix parameter order is rows, cols, not X, Y
	pix_plane = pixData (h, w);
	if (ColorMode != cmGRAY) {
		clr_plane = clrData (h, w);
	}

	width  = w;
	height = h;
}

void ImageMatrix::copy(ImageMatrix *copy) {
	width = copy->width;
	height = copy->height;
	has_stats = copy->has_stats;
	has_median = copy->has_median;
	ColorMode = copy->ColorMode;
	bits = copy->bits;
	allocate(width, height);
	pix_plane = copy->pix_plane;
	if (ColorMode != cmGRAY) {
		clr_plane = copy->clr_plane;
	}
}

ImageMatrix::ImageMatrix() {
	init();
}

ImageMatrix::ImageMatrix(unsigned int width, unsigned int height) {  
	init();
	allocate (width, height);
}

ImageMatrix::ImageMatrix(ImageMatrix *matrix) {  
	copy (matrix);
}

/* create an image which is part of the image
   (x1,y1) - top left
   (x2,y2) - bottom right
*/
ImageMatrix::ImageMatrix(ImageMatrix *matrix, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {

	init();
	bits=matrix->bits;
	ColorMode=matrix->ColorMode;
	/* verify that the image size is OK */
	if (x1<0) x1=0;
	if (y1<0) y1=0;
	if (x2>=matrix->width) x2=matrix->width-1;
	if (y2>=matrix->height) y2=matrix->height-1;

	width=x2-x1+1;
	height=y2-y1+1;
	allocate (width, height);
	// Copy the Eigen matrixes
	// N.B. Eigen matrix parameter order is rows, cols, not X, Y
	pix_plane = matrix->pix_plane.block(y1,x1,height,width);
	if (ColorMode != cmGRAY) {
		clr_plane = matrix->clr_plane.block(y1,x1,height,width);
	}
}

/* free the memory allocated in "ImageMatrix::LoadImage" */
ImageMatrix::~ImageMatrix()
{
}


/* to8bits
   convert a 16 bit matrix to 8 bits
   N.B.: This assumes that the data takes up the entire 16-bit range, which is almost certainly wrong
*/
void ImageMatrix::to8bits() {
	double max_range = pow((double)2,bits)-1;
	if (bits==8) return;
	bits=8;
	pix_plane = 255.0 * (pix_plane / max_range);
}

/* flipV
   flip an image vertically
*/

void ImageMatrix::flipV() {
	pix_plane = pix_plane.rowwise().reverse();
	if (ColorMode != cmGRAY) {
		clr_plane.rowwise().reverse();
	}
}
/* flipH
   flip an image horizontally
*/
void ImageMatrix::flipH() {
	pix_plane = pix_plane.colwise().reverse();
	if (ColorMode != cmGRAY) {
		clr_plane.colwise().reverse();
	}
}

void ImageMatrix::invert() {
	double max_range = pow((double)2,bits)-1;
	pix_plane = max_range - pix_plane.array();
}

/* Downsample
   down sample an image
   x_ratio, y_ratio -double- (0 to 1) the size of the new image comparing to the old one
   FIXME: Since this is done in-place, there is potential for aliasing (i.e. new pixel values interfering with old pixel values)
*/
void ImageMatrix::Downsample(double x_ratio, double y_ratio) {
	double x,y,dx,dy,frac;
	unsigned int new_x,new_y,a;

	if (x_ratio>1) x_ratio=1;
	if (y_ratio>1) y_ratio=1;
	dx=1/x_ratio;
	dy=1/y_ratio;

	if (dx==1 && dy==1) return;   /* nothing to scale */

	/* first downsample x */
	for (new_y = 0; new_y < height; new_y++) {
		x=0;
		new_x=0;
		while (x < width) {
    	    double sum_i=0;
			double sum_c1=0;
			double sum_c2=0;
			double sum_c3=0;

			/* the leftmost fraction of pixel */
			a=(int)(floor(x));
			frac=ceil(x)-x;
			if (frac>0 && a<width) {
				sum_i += pix_plane(new_y,a)*frac;
				if (ColorMode != cmGRAY) {
					sum_c1 += clr_plane(new_y,a).h*frac;
					sum_c2 += clr_plane(new_y,a).s*frac;
					sum_c3 += clr_plane(new_y,a).v*frac;
				}
			} 
			/* the middle full pixels */
			for (a=(int)(ceil(x));a<floor(x+dx);a=a+1) {
				if (a<width) {
					sum_i += pix_plane(new_y,a)*frac;
					if (ColorMode != cmGRAY) {
						sum_c1 += clr_plane(new_y,a).h*frac;
						sum_c2 += clr_plane(new_y,a).s*frac;
						sum_c3 += clr_plane(new_y,a).v*frac;
					}
				}
			}
			/* the right fraction of pixel */
			frac=x+dx-floor(x+dx);
			if (frac>0 && a<width) {
				sum_i += pix_plane(new_y,a)*frac;
				if (ColorMode != cmGRAY) {
					sum_c1 += clr_plane(new_y,a).h*frac;
					sum_c2 += clr_plane(new_y,a).s*frac;
					sum_c3 += clr_plane(new_y,a).v*frac;
				}
			}

			pix_plane (new_y, new_x) = sum_i/(dx);
			if (ColorMode != cmGRAY) {
					sum_c1 += clr_plane(new_y,a).h*frac;
					sum_c2 += clr_plane(new_y,a).s*frac;
					sum_c3 += clr_plane(new_y,a).v*frac;
				clr_plane(new_y, new_x).h = (byte)(sum_c1/(dx));
				clr_plane(new_y, new_x).s = (byte)(sum_c2/(dx));
				clr_plane(new_y, new_x).v = (byte)(sum_c3/(dx));
			}
			x+=dx;
			new_x++;
		}
	}
 
      /* downsample y */
	for (new_x=0;new_x<x_ratio*width;new_x++) {
		y=0;
		new_y=0;
		while (y<height) {
			double sum_i=0;
			double sum_c1=0;
			double sum_c2=0;
			double sum_c3=0;

			a=(int)(floor(y));
			frac=ceil(y)-y;
			if (frac>0 && a<height) {   /* take also the part of the leftmost pixel (if needed) */
				sum_i += pix_plane(a,new_x)*frac;
				if (ColorMode != cmGRAY) {
					sum_c1 += clr_plane(a,new_x).h*frac;
					sum_c2 += clr_plane(a,new_x).s*frac;
					sum_c3 += clr_plane(a,new_x).v*frac;
				}
			}
			for (a=(int)(ceil(y));a<floor(y+dy);a=a+1) {
				if (a<height) {
					sum_i += pix_plane(a,new_x)*frac;
					if (ColorMode != cmGRAY) {
						sum_c1 += clr_plane(a,new_x).h*frac;
						sum_c2 += clr_plane(a,new_x).s*frac;
						sum_c3 += clr_plane(a,new_x).v*frac;
					}
				}
			}
			frac=y+dy-floor(y+dy);
			if (frac>0 && a<height) {
				sum_i += pix_plane(a,new_x)*frac;
				if (ColorMode != cmGRAY) {
					sum_c1 += clr_plane(a,new_x).h*frac;
					sum_c2 += clr_plane(a,new_x).s*frac;
					sum_c3 += clr_plane(a,new_x).v*frac;
				}
			}

			pix_plane (new_y, new_x) = sum_i/(dy);
			if (ColorMode != cmGRAY) {
				clr_plane(new_y, new_x).h = (byte)(sum_c1/(dy));
				clr_plane(new_y, new_x).s = (byte)(sum_c2/(dy));
				clr_plane(new_y, new_x).v = (byte)(sum_c3/(dy));
			}

			y+=dy;
			new_y++;
		}
	}

	width=(int)(x_ratio*width);
	height=(int)(y_ratio*height);
}


/* Rotate
   Rotate an image by 90, 120, or 270 degrees
   angle -double- (0 to 360) the degrees of rotation.  Only values of 90, 180, 270 are currently allowed
*/
ImageMatrix* ImageMatrix::Rotate(double angle) {
	ImageMatrix *new_matrix;
	unsigned int new_width,new_height;

	// Only deal with right angles
	if (! ( (angle == 90) || (angle == 180) || (angle == 270) ) ) return (this);

	// switch width/height if 90 or 270
	if ( (angle == 90) || (angle == 270) ) {
		new_width = height;
		new_height = width;
	} else {
		new_width = width;
		new_height = height;
	}

	// Make a new image matrix
	new_matrix=new ImageMatrix (new_width, new_height);
	new_matrix->bits=bits;
	new_matrix->ColorMode=ColorMode;

	// a 180 is simply a reverse of the matrix
	// a 90 is m.transpose().rowwise.reverse()
	// a 270 is m.transpose()
	switch ((int)angle) {
		case 90:
			new_matrix->pix_plane = pix_plane.transpose().rowwise().reverse();
		break;

		case 180:
			new_matrix->pix_plane = pix_plane.reverse();
		break;

		case 270:
			new_matrix->pix_plane = pix_plane.transpose();
		break;
	}
	return(new_matrix);
}


/* find basic intensity statistics */


/* BasicStatistics
   get basic statistical properties of the intensity of the image
   mean -double *- pre-allocated one double for the mean intensity of the image
   median -double *- pre-allocated one double for the median intensity of the image
   std -double *- pre-allocated one double for the standard deviation of the intensity of the image
   min -double *- pre-allocated one double for the minimum intensity of the image
   max -double *- pre-allocated one double for the maximal intensity of the image
   histogram -double *- a pre-allocated vector for the histogram. If NULL then histogram is not calculated
   nbins -int- the number of bins for the histogram
   
   if one of the pointers is NULL, the corresponding value is not computed.
*/
void ImageMatrix::BasicStatistics(double *mean_p, double *median_p, double *std_p, double *min_p, double *max_p, double *hist_p, int bins) {
	unsigned long pixel_index,num_pixels;
	double *pixels=NULL, *pix_ptr;
	double min = INF, max = -INF, mean = 0, median = 0, delta = 0, M2 = 0, val = 0, var = 0, std = 0;
	bool calc_stats, calc_median, calc_hist;
	unsigned int x, y;

	calc_stats = (mean_p || std_p || min_p || max_p);
	calc_median = (median_p);
	calc_hist = (hist_p);
	if (has_stats && calc_stats) {
		if (mean_p) *mean_p = _mean;
		if (std_p) *std_p = _std;
		if (min_p) *min_p = _min;
		if (max_p) *max_p = _max;
		calc_stats = false;
	}

	if (has_median && calc_median) {
		*median_p = _median;
		calc_median = false;
	}
	
	if (! (calc_stats || calc_median || calc_hist) ) return;

	if (calc_stats) {

		// zero-out any pointers passed in
		if (mean_p) *mean_p = 0;
		if (std_p) *std_p = 0;
		if (min_p) *min_p = 0;
		if (max_p) *max_p = 0;	 

		num_pixels=height*width;
		// only need this for the median and histogram
		if (calc_median) {
			pixels = new double[num_pixels];
			if (!pixels) return;
		}

		/* compute the average, min and max */
		pixel_index = 0;
		pix_ptr = pixels;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				val = pix_plane (y, x);
				if (pixels) *pix_ptr++ = val;
				if (val > max) max = val;
				if (val < min) min = val;
				// This is Welford's cumulative mean+variance algorithm as reported by Knuth
				pixel_index++;
				delta = val - mean;
				mean += delta/pixel_index;
				M2 += delta * (val - mean);
			}
		}
		var = M2 / (pixel_index - 1);
		std = sqrt (var);
		_mean = mean;
		_std = std;
		_min = min;
		_max = max;
		has_stats = true;
		if (mean_p) *mean_p = mean;
		if (std_p) *std_p = std;
		if (min_p) *min_p = min;
		if (max_p) *max_p = max;	 
	
	}
	
	if (calc_median && pixels) {
		qsort(pixels,num_pixels,sizeof(double),compare_doubles);
		median=pixels[num_pixels/2];
		*median_p = median;
		_median = median;
		has_median = true;
		delete [] pixels;
	}
	
	if (calc_hist) {
		histogram(hist_p,bins,0);
	}
}

/* normalize the pixel values into a given range 
   min -double- the min pixel value (ignored if <0)
   max -double- the max pixel value (ignored if <0)
   range -long- nominal dynamic range (ignored if <0)
   n_mean -double- the mean of the normalized image (ignored if <0)
   n_std -double- the stddev of the normalized image (ignored if <0)
*/
void ImageMatrix::normalize(double n_min, double n_max, long n_range, double n_mean, double n_std) {
	unsigned int x,y;
	double val;
	/* normalized to n_min and n_max */
	if (n_min >= 0 && n_max > 0 && n_range > 0) {
		double norm_fact = n_range / (n_max-n_min);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				val = pix_plane (y,x);
				if (val < n_min) val = 0;
				else if (val > n_max) val = n_range;
				else val = norm_fact * (val - n_min);
				pix_plane (y,x) = val;
			}
		}
	}

    /* normalize to n_mean and n_std */
	if (n_mean > 0) {
		double d_mean = mean() - n_mean, std_fact = n_std/std();
		double max_range = pow((double)2,bits)-1;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				val = pix_plane (y,x) - d_mean;
				if (n_std > 0)
					val = n_mean + (val-n_mean) * std_fact;
				if (val < 0) val = 0;
				else if (val > max_range) val = max_range;
				pix_plane (y,x) = val;
			}
        }
	}	   
}

/* convolve
*/
void ImageMatrix::convolve(ImageMatrix *filter) {
	unsigned int x, y, i, j, xx, yy;
	unsigned int height2=filter->height/2;
	unsigned int width2=filter->width/2;
	ImageMatrix *copy;
	double tmp;

	copy = new ImageMatrix (this);
	for (x = 0; x < width; ++x) {
		for (y = 0; y < height; ++y) {
			tmp=0.0;
			for (i = -width2; i <= width2; ++i) {
				xx=x+i;
				if (xx < width && xx >= 0) {
					for(j = -height2; j <= height2; ++j) {
						yy=y+j;
						if (yy >= 0 && yy < height) {
							tmp += filter->pix_plane (j+height2, i+width2) * copy->pix_plane(yy,xx);
						}
					}
				}
			}
			set(x,y,tmp);
		}
	}
	delete copy;
}

/* find the basic color statistics
   hue_avg_p -double *- average hue
   hue_std_p -double *- standard deviation of the hue
   sat_avg_p -double *- average saturation
   sat_std_p -double *- standard deviation of the saturation
   val_avg_p -double *- average value
   val_std_p -double *- standard deviation of the value
   max_color_p -double *- the most popular color
   colors -double *- a histogram of colors
   if values are NULL - the value is not computed
*/

void ImageMatrix::GetColorStatistics(double *hue_avg_p, double *hue_std_p, double *sat_avg_p, double *sat_std_p, double *val_avg_p, double *val_std_p, double *max_color_p, double *colors) {
	double hue_avg=0, hue_std=0, sat_avg=0, sat_std=0, val_avg=0, val_std=0;
	double delta, M2h=0, M2s=0, M2v=0;
	unsigned int a, x, y, pixel_index=0;
	unsigned long color_index=0;
	double max,pixel_num;
	double certainties[COLORS_NUM+1];
	byte h, s, v;

	pixel_num=height*width;

	/* calculate the average hue, saturation, value */
	if (hue_avg_p) *hue_avg_p=0;
	if (sat_avg_p) *sat_avg_p=0;
	if (val_avg_p) *val_avg_p=0;
	if (colors)
		for (a=0;a<=COLORS_NUM;a++)
			colors[a]=0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			h = clr_plane(y,x).h;
			s = clr_plane(y,x).s;
			v = clr_plane(y,x).v;
			// This is Welford's cumulative mean+variance algorithm as reported by Knuth
			pixel_index++;
			// h
			delta = h - hue_avg;
			hue_avg += delta/pixel_index;
			M2h += delta * (h - hue_avg);
			// s
			delta = s - sat_avg;
			sat_avg += delta/pixel_index;
			M2s += delta * (s - sat_avg);
			// v
			delta = v - val_avg;
			val_avg += delta/pixel_index;
			M2v += delta * (v - val_avg);

			color_index=FindColor(h,s,v,certainties);
			colors[color_index]+=1;
		}
	}
	hue_std = sqrt ( M2h / (pixel_index - 1) );
	sat_std = sqrt ( M2s / (pixel_index - 1) );
	val_std = sqrt ( M2v / (pixel_index - 1) );

	if (hue_avg_p) *hue_avg_p = hue_avg;
	if (sat_avg_p) *sat_avg_p = sat_avg;
	if (val_avg_p) *val_avg_p = val_avg;
	if (hue_std_p) *hue_std_p = hue_std;
	if (sat_std_p) *sat_std_p = sat_std;
	if (val_std_p) *val_std_p = val_std;

	/* max color (the most common color in the image) */
	if (max_color_p) {
		*max_color_p=0;
		max=0.0;
		for (a = 0; a <= COLORS_NUM; a++) {
			if (colors[a] > max) {
				max=colors[a];
				*max_color_p=a;
			}
		}
	}
	/* colors */
	if (colors)
		for (a = 0; a <= COLORS_NUM; a++)
			colors[a]=colors[a]/pixel_num;
}

/* ColorTransform
   Transform a color image to a greyscale image such that each
   color_hist -double *- a histogram (of COLOR_NUM + 1 bins) of the colors. This parameter is ignored if NULL
   use_hue -int- 0 if classifying colors, 1 if using the hue component of the HSV vector
   grey level represents a different color
*/
void ImageMatrix::ColorTransform(double *color_hist, int use_hue) {  
	unsigned int x,y; //,base_color;
	double cb_intensity;
	double max_range = pow((double)2,bits)-1;
	HSVcolor hsv_pixel;
	unsigned long color_index=0;   
	RGBcolor rgb;
	double certainties[COLORS_NUM+1];

	// initialize the color histogram
	if( color_hist ) 
		for( color_index = 0; color_index <= COLORS_NUM; color_index++ )
			color_hist[color_index]=0;
	// find the colors
	for( y = 0; y < height; y++ ) {
		for( x = 0; x < width; x++ ) { 
			hsv_pixel = clr_plane (y, x);
			if( use_hue == 0 ) { // not using hue  
				color_index = FindColor( hsv_pixel.h,  hsv_pixel.s, hsv_pixel.v, certainties );
				if( color_hist )
					color_hist[ color_index ] ++;
				// convert the color index to a greyscale value
				cb_intensity = int( ( max_range * color_index ) / COLORS_NUM );
			} else { // using hue
				cb_intensity = hsv_pixel.h;
			}
			rgb.r = rgb.g = rgb.b = (byte)( 255 * ( cb_intensity / max_range ) );
			clr_plane (y, x) = RGB2HSV( rgb );
			pix_plane (y, x) = cb_intensity;
		}
	}
	/* normalize the color histogram */
	if (color_hist) 
		for (color_index=0;color_index<=COLORS_NUM;color_index++)
			color_hist[color_index]/=(width*height);	 
}

/* get image histogram */
void ImageMatrix::histogram(double *bins,unsigned short bins_num, int imhist) {
	unsigned long a;
	double h_min=INF,h_max=-INF;
	/* find the minimum and maximum */
	if (imhist == 1) {    /* similar to the Matlab imhist */
		h_min = 0;
		h_max = pow((double)2,bits)-1;

	} else {
		h_min = min();
		h_max = max();
	}
	/* initialize the bins */
	for (a = 0; a < bins_num; a++)
		bins[a] = 0;

	/* build the histogram */
	for (a = 0; a < width*height; a++) {
		if (pix_plane.array().coeff(a) >= h_max) bins[bins_num-1]++;
		else if (pix_plane.array().coeff(a) <= h_min) bins[0]++;
		else bins[(int)(((pix_plane.array().coeff(a) - h_min)/(h_max - h_min)) * bins_num)]++;
	}

	return;
}

/* fft 2 dimensional transform */
// http://www.fftw.org/doc/
double ImageMatrix::fft2() {
	fftw_complex *out;
	double *in;
	fftw_plan p;
	unsigned int x,y;

	in = (double*) fftw_malloc(sizeof(double) * width*height);
	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * width*height);

	p = fftw_plan_dft_r2c_2d(width,height,in,out, FFTW_MEASURE /* FFTW_ESTIMATE */);

	for (x=0;x<width;x++)
		for (y=0;y<height;y++)
			in[height*x+y]=pix_plane.coeff(y,x);

	fftw_execute(p); /* execute the transformation (repeat as needed) */

	unsigned int half_height=height/2+1, idx;
	/* find the abs and angle */
	for (x=0;x<width;x++) {
		for (y=0;y<half_height;y++) {
			idx = half_height*x+y;
			pix_plane (y,x) = sqrt( pow( out[idx][0],2)+pow(out[idx][1],2));    /* sqrt(real(X).^2 + imag(X).^2) */
		}
	}

	/* complete the first column */
	for (y=half_height;y<height;y++)
		pix_plane (y,x) = pix_plane (height - y, 0);

	/* complete the rows */
	for (y=half_height;y<height;y++)
		for (x=1;x<width;x++)   /* 1 because the first column is already completed */
			pix_plane (y,x) = pix_plane (height - y, width - x);
   
   fftw_destroy_plan(p);
   fftw_free(in);
   fftw_free(out);

   /* calculate the magnitude and angle */

   return(0);
}

/* chebyshev transform */
void ImageMatrix::ChebyshevTransform(unsigned int N) {
	double *out;
	unsigned int x,y;

	if (N<2)
		N = MIN( width, height );
	out=new double[height*N];
	Chebyshev2D(this, out,N);

	width=N;
	height = MIN( height, N );   /* prevent error */

	for(y=0;y<height;y++)
		for(x=0;x<width;x++)
			pix_plane (y,x) = out[y * width + x];
	delete [] out;
}

/* chebyshev transform
   coeff -array of double- a pre-allocated array of 32 doubles
*/
void ImageMatrix::ChebyshevFourierTransform2D(double *coeff) {
	ImageMatrix *matrix;
	matrix = new ImageMatrix (this);
	if( (width * height) > (300 * 300) )
		matrix->Downsample( MIN( 300.0/(double)width, 300.0/(double)height ), MIN( 300.0/(double)width, 300.0/(double)height ) );  /* downsample for avoiding memory problems */
	ChebyshevFourier2D(matrix, 0, coeff,32);
	delete matrix;
}


/* Symlet5 transform */
void ImageMatrix::Symlet5Transform() {
	unsigned int x,y;
	DataGrid2D *grid2d=NULL;
	DataGrid *grid;
	Symlet5 *Sym5;

	grid = new DataGrid2D(width,height,-1);
	grid=grid2d;
  
	for (y=0;y<height;y++)
		for(x=0;x<width;x++)
			grid->setData(x,y,-1,pix_plane(y,x));
	Sym5=new Symlet5(0,1);
	Sym5->transform2D(grid);
	
	allocate (grid->getX(), grid->getY());
	for (y=0;y<height;y++)
		for(x=0;x<width;x++)
			pix_plane (y,x) = grid->getData(x,y,-1);
		 
	delete Sym5;
	delete grid2d;
}

/* chebyshev statistics
   coeff -array of double- pre-allocated memory of 20 doubles
   nibs_num - (32 is normal)
*/
void ImageMatrix::ChebyshevStatistics2D(double *coeff, unsigned int N, unsigned int bins_num)
{
   if (N<2) N=20;
   if (N>MIN(width,height)) N=MIN(width,height);   
   ChebyshevTransform(N);
   histogram(coeff,bins_num,0);
}

/* CombFirstFourMoments
   vec should be pre-alocated array of 48 doubles
*/
int ImageMatrix::CombFirstFourMoments2D(double *vec) {
	int count;
	ImageMatrix *matrix;
	if (bits==16) {
		matrix = new ImageMatrix (this);
		matrix->to8bits();
	} else matrix = this;
	
	count = CombFirst4Moments2D (matrix, vec);   
	vd_Comb4Moments (vec);   
	if (bits == 16) delete matrix;
	return (count);
}

/* Edge Transform */
void ImageMatrix::EdgeTransform() {
	unsigned int x,y;
	double max_x=0,max_y=0;

	ImageMatrix *TempMatrix;
	TempMatrix = new ImageMatrix (this);
	for (y = 0; y < TempMatrix->height; y++)
		for (x = 0; x < TempMatrix->width; x++) {
			if (y > 0 && y < height-1) max_y=MAX(fabs(TempMatrix->pix_plane(y,x) - TempMatrix->pix_plane(y-1,x)), fabs(TempMatrix->pix_plane(y,x) - TempMatrix->pix_plane(y+1,x)));
			if (x > 0 && x < width-1)  max_x=MAX(fabs(TempMatrix->pix_plane(y,x) - TempMatrix->pix_plane(y,x-1)), fabs(TempMatrix->pix_plane(y,x) - TempMatrix->pix_plane(y,x+1)));
			pix_plane(y,x) = MAX(max_x,max_y);
		}
   delete TempMatrix;
}

/* Perwitt gradient magnitude
   output - a pre-allocated matrix that will hold the output (the input matrix is not changed)
            output should be of the same size as the input matrix
*/
void ImageMatrix::PerwittMagnitude2D(ImageMatrix *output) {
	long x,y,i,j,w=width,h=height;
	double sumx,sumy;
	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			sumx=0;
			sumy=0;		  
			for (j = y-1; j <= y+1; j++)
				if (j >= 0 && j < h && x-1 >= 0)
					sumx += pix_plane(j,x-1)*1;//0.3333;
			for (j = y-1; j <= y+1; j++)
				if (j >= 0 && j < h && x+1 < w)
					sumx += pix_plane(j,x+1)*-1;//-0.3333;
			for (i = x-1; i <= x+1; i++)
				if (i >= 0 && i < w && y-1 >= 0)
					sumy += pix_plane(y-1,i)*1;//-0.3333;
			for (i = x-1; i <= x+1; i++)
				if (i >= 0 && i < w && y+1 < h)
					sumy += pix_plane(y+1,i)*-1;//0.3333;
			output->pix_plane(y,x) = sqrt(sumx*sumx+sumy*sumy);
		}
	}
}

/* Perwitt gradient direction
   output - a pre-allocated matrix that will hold the output (the input matrix is not changed)
            output should be of the same size as the input matrix
*/
void ImageMatrix::PerwittDirection2D(ImageMatrix *output) {
	long x,y,i,j,w=width,h=height;
	double sumx,sumy;
	for (x = 0; x < w; x++)
		for (y = 0;y < h; y++) {
			sumx=0;
			sumy=0;
			for (j = y-1; j <= y+1; j++)
				if (j >= 0 && j < h && x-1 >= 0)
					sumx += pix_plane(j,x-1)*1;//0.3333;
			for (j = y-1; j <= y+1; j++)
				if (j >= 0 && j < h && x+1 < w)
					sumx += pix_plane(j,x+1)*-1;//-0.3333;
			for (i = x-1; i <= x+1; i++)
				if (i >= 0 && i < w && y-1 >= 0)
					sumy += pix_plane(y-1,i)*1;//-0.3333;
			for (i = x-1; i <= x+1; i++)
				if (i >= 0 && i < w && y+1 < h)
					sumy += pix_plane(y+1,i)*-1;//0.3333;
			if (sumy == 0 || fabs(sumy)<1/INF) output->pix_plane(y,x) = 3.1415926 * (sumx < 0 ? 1 : 0);
			else output->pix_plane(y,x) = atan2(sumy,sumx);
		}
}


/* edge statistics */
//#define NUM_BINS 8
//#define NUM_BINS_HALF 4
/* EdgeArea -long- number of edge pixels
   MagMean -double- mean of the gradient magnitude
   MagMedian -double- median of the gradient magnitude
   MagVar -double- variance of the gradient magnitude
   MagHist -array of double- histogram of the gradient magnitude. array of size "num_bins" should be allocated before calling the function
   DirecMean -double- mean of the gradient direction
   DirecMedian -double- median of the gradient direction
   DirecVar -double- variance of the gradient direction
   DirecHist -array of double- histogram of the gradient direction. array of size "num_bins" should be allocated before calling the function
   DirecHomogeneity -double-
   DiffDirecHist -array of double- array of size num_bins/2 should be allocated
*/

void ImageMatrix::EdgeStatistics(unsigned long *EdgeArea, double *MagMean, double *MagMedian, double *MagVar, double *MagHist, double *DirecMean, double *DirecMedian, double *DirecVar, double *DirecHist, double *DirecHomogeneity, double *DiffDirecHist, unsigned int num_bins) {
	ImageMatrix *GradientMagnitude,*GradientDirection;
	unsigned int a,bin_index;
	double min,max,sum, max_range = pow((double)2,bits)-1;

	GradientMagnitude = new ImageMatrix (this);
	PerwittMagnitude2D(GradientMagnitude);
	GradientDirection = new ImageMatrix (this);
	PerwittDirection2D(GradientDirection);

	/* find gradient statistics */
	GradientMagnitude->BasicStatistics(MagMean, MagMedian, MagVar, &min, &max, MagHist, num_bins);
	*MagVar = pow(*MagVar,2);

	/* find the edge area (number of edge pixels) */
	*EdgeArea = 0;
//	level = min+(max-min)/2;   // level=duplicate->OtsuBinaryMaskTransform()   // level=MagMean

	for (a = 0; a < GradientMagnitude->height*GradientMagnitude->width; a++)
		if (GradientMagnitude->pix_plane.array().coeff(a) > max_range*0.5) (*EdgeArea)+=1; /* find the edge area */
//   GradientMagnitude->OtsuBinaryMaskTransform();

	/* find direction statistics */
	GradientDirection->BasicStatistics(DirecMean, DirecMedian, DirecVar, &min, &max, DirecHist, num_bins);
	*DirecVar=pow(*DirecVar,2);

	/* Calculate statistics about edge difference direction
	   Histogram created by computing differences amongst histogram bins at angle and angle+pi
	*/
	for (bin_index = 0; bin_index < (num_bins/2); bin_index++)
		DiffDirecHist[bin_index] = fabs(DirecHist[bin_index]-DirecHist[bin_index+(int)(num_bins/2)]);
	sum=0;
	for (bin_index = 0; bin_index < (num_bins/2); bin_index++) {
		if (DirecHist[bin_index] + DirecHist[bin_index+(int)(num_bins/2)] != 0)  /* protect from a numeric flaw */
			DiffDirecHist[bin_index] = DiffDirecHist[bin_index]/(DirecHist[bin_index]+DirecHist[bin_index+(int)(num_bins/2)]);
		sum += (DirecHist[bin_index]+DirecHist[bin_index+(int)(num_bins/2)]);
	}

	/* The fraction of edge pixels that are in the first two bins of the histogram measure edge homogeneity */
	if (sum > 0) *DirecHomogeneity = (DirecHist[0]+DirecHist[1])/sum;

	delete GradientMagnitude;
	delete GradientDirection;
}

/* radon transform
   vec -array of double- output column. a pre-allocated vector of the size 3*4=12
*/
void ImageMatrix::RadonTransform2D(double *vec) {
	unsigned int x,y,val_index,output_size,vec_index,bin_index;
	double *pixels,*ptr,bins[3];
	int angle,num_angles=4;
	double theta[4]={0,45,90,135};
	//double min,max;
	int rLast,rFirst;
	rLast = (int) ceil(sqrt(pow( (double)(width-1-(width-1)/2),2)+pow( (double)(height-1-(height-1)/2),2))) + 1;
	rFirst = -rLast;
	output_size=rLast-rFirst+1;

	ptr = new double[output_size*num_angles];
	for (val_index = 0; val_index < output_size*num_angles; val_index++)
		ptr[val_index] = 0;  /* initialize the output vector */

    pixels=new double[width*height];
    vec_index = 0;

	for (x = 0; x < width; x++)
		for (y = 0; y < height; y++)
			pixels[y+height*x] = pix_plane(y,x);

    radon(ptr,pixels, theta, height, width, (width-1)/2, (height-1)/2, num_angles, rFirst, output_size);

	for (angle = 0; angle < num_angles; angle++) {
		//radon(ptr,pixels, &theta, height, width, (width-1)/2, (height-1)/2, 1, rFirst, output_size);
		/* create histogram */
		double min=INF,max=-INF;
		/* find the minimum and maximum values */
		for (val_index = angle*output_size; val_index < (angle+1)*output_size; val_index++) {
			if (ptr[val_index]>max) max = ptr[val_index];
			if (ptr[val_index]<min) min = ptr[val_index];
		}

		for (val_index=0;val_index<3;val_index++)   /* initialize the bins */
			bins[val_index]=0;
		for (val_index=angle*output_size;val_index<(angle+1)*output_size;val_index++)
			if (ptr[val_index]==max) bins[2]+=1;
			else bins[(int)(((ptr[val_index]-min)/(max-min))*3)]+=1;

		for (bin_index=0;bin_index<3;bin_index++)
			vec[vec_index++]=bins[bin_index];
	}
	vd_RadonTextures(vec);
	delete [] pixels;
	delete [] ptr;
}

//-----------------------------------------------------------------------------------
/* Otsu
   Find otsu threshold
*/
double ImageMatrix::Otsu() {
	long a; //,x,y;
	double hist[256],omega[256],mu[256],sigma_b2[256],maxval=-INF,sum,count;
	double max_range = pow((double)2,bits)-1;
	histogram(hist,256,1);
	omega[0] = hist[0] / (width*height);
	mu[0] = 1*hist[0] / (width*height);

	for (a = 1; a < 256; a++) {
		omega[a] = omega[a-1] + hist[a] / (width*height);
		mu[a] = mu[a-1] + (a+1) * hist[a] / (width*height);
	}
	for (a=0;a<256;a++) {
		if (omega[a] == 0 || 1-omega[a] == 0) sigma_b2[a]=0;
		else sigma_b2[a] = pow(mu[255]*omega[a]-mu[a],2) / (omega[a]*(1-omega[a]));
		if (sigma_b2[a] > maxval) maxval = sigma_b2[a];
	}
	sum = 0.0;
	count = 0.0;
	for (a = 0; a < 256; a++)
		if (sigma_b2[a] == maxval) {
			sum += a;
			count++;
		}	 
   return((pow((double)2,bits)/256.0)*((sum/count)/max_range));
}

//-----------------------------------------------------------------------------------
/*
  OtsuBinaryMaskTransform
  Transforms an image to a binary image such that the threshold is otsu global threshold
*/
double ImageMatrix::OtsuBinaryMaskTransform() {
	double OtsuGlobalThreshold;
	double max_range = pow((double)2,bits)-1;

	OtsuGlobalThreshold=Otsu();

	/* classify the pixels by the threshold */
	for (unsigned int a = 0; a < width*height; a++)
		if (pix_plane.array().coeff(a) > OtsuGlobalThreshold*max_range) (pix_plane.array())(a) = 1;
		else (pix_plane.array())(a) = 0;
	 
	return(OtsuGlobalThreshold);
}

/*  BWlabel
    label groups of connected pixel (4 or 8 connected dependes on the value of the parameter "level").
    This is an implementation of the Matlab function bwlabel
    returned value -int- the number of objects found
*/
//--------------------------------------------------------
unsigned long ImageMatrix::BWlabel(int level) {
	return(bwlabel(this,level));
}

//--------------------------------------------------------

void ImageMatrix::centroid(double *x_centroid, double *y_centroid) {
	GlobalCentroid(this,x_centroid,y_centroid);
}

//--------------------------------------------------------

/*
  FeatureStatistics
  Find feature statistics. Before calling this function the image should be transformed into a binary
  image using "OtsuBinaryMaskTransform".

  count -int *- the number of objects detected in the binary image
  Euler -int *- the euler number (number of objects - number of holes
  centroid_x -int *- the x coordinate of the centroid of the binary image
  centroid_y -int *- the y coordinate of the centroid of the binary image
  AreaMin -int *- the smallest area
  AreaMax -int *- the largest area
  AreaMean -int *- the mean of the areas
  AreaMedian -int *- the median of the areas
  AreaVar -int *- the variance of the areas
  DistMin -int *- the smallest distance
  DistMax -int *- the largest distance
  DistMean -int *- the mean of the distance
  DistMedian -int *- the median of the distances
  DistVar -int *- the variance of the distances

*/

int compare_ints (const void *a, const void *b) {
	if (*((int *)a) > *((int *)b)) return(1);
	if (*((int *)a) == *((int *)b)) return(0);
	return(-1);
}

void ImageMatrix::FeatureStatistics(unsigned long *count, unsigned long *Euler, double *centroid_x, double *centroid_y, unsigned long *AreaMin, unsigned long *AreaMax,
	double *AreaMean, unsigned int *AreaMedian, double *AreaVar, unsigned int *area_histogram,double *DistMin, double *DistMax,
	double *DistMean, double *DistMedian, double *DistVar, unsigned int *dist_histogram, unsigned int num_bins
) {
	unsigned long object_index,inv_count;
	double sum_areas,sum_dists;
	ImageMatrix *BWImage,*BWInvert,*temp;
	unsigned long *object_areas;
	double *centroid_dists,sum_dist;

	BWInvert=new ImageMatrix (this);   // check if the background is brighter or dimmer
	BWInvert->invert();
	BWInvert->OtsuBinaryMaskTransform();
	inv_count=BWInvert->BWlabel(8);


	BWImage=new ImageMatrix (this);
	BWImage->OtsuBinaryMaskTransform();
	BWImage->centroid(centroid_x,centroid_y);
	*count = BWImage->BWlabel(8);
	if (inv_count > *count) {
		temp = BWImage;
		BWImage = BWInvert;
		BWInvert = temp;
		*count = inv_count;
		BWImage->centroid(centroid_x,centroid_y);	  
	}
	delete BWInvert;
	*Euler=EulerNumber(BWImage,*count)+1;

	// calculate the areas 
	sum_areas = 0;
	sum_dists = 0;
	object_areas = new unsigned long[*count];
	centroid_dists = new double[*count];
	for (object_index = 1; object_index <= *count; object_index++) {
		double x_centroid,y_centroid;
		object_areas[object_index-1] = FeatureCentroid(BWImage, object_index, &x_centroid, &y_centroid);
		centroid_dists[object_index-1] = sqrt(pow(x_centroid-(*centroid_x),2)+pow(y_centroid-(*centroid_y),2));
		sum_areas += object_areas[object_index-1];
		sum_dists += centroid_dists[object_index-1];
	}
	/* compute area statistics */
	qsort(object_areas,*count,sizeof(unsigned int),compare_ints);
	*AreaMin = object_areas[0];
	*AreaMax = object_areas[*count-1];
	if (*count > 0) *AreaMean = sum_areas/(*count);
	else *AreaMean = 0.0;
	*AreaMedian = (unsigned int)object_areas[(*count)/2];
	for (object_index = 0; object_index < num_bins; object_index++)
		area_histogram[object_index] = 0;
	/* compute the variance and the histogram */
	sum_areas = 0;
	if (*AreaMax-*AreaMin > 0)
		for (object_index = 1; object_index <= *count; object_index++) {
			sum_areas += pow(object_areas[object_index-1]-*AreaMean,2);
			if (object_areas[object_index-1] == *AreaMax) area_histogram[num_bins-1] += 1;
			else area_histogram[((object_areas[object_index-1] - *AreaMin) / (*AreaMax-*AreaMin))*num_bins] += 1;
		}
	if (*count > 1) *AreaVar = sum_areas / ((*count)-1);
	else *AreaVar = sum_areas;

	/* compute distance statistics */
	qsort(centroid_dists,*count,sizeof(double),compare_doubles);
	*DistMin = centroid_dists[0];
	*DistMax = centroid_dists[*count-1];
	if (*count > 0) *DistMean=sum_dists / (*count);
	else *DistMean = 0;
	*DistMedian = centroid_dists[(*count)/2];
	for (object_index = 0; object_index < num_bins; object_index++)
		dist_histogram[object_index] = 0;

	/* compute the variance and the histogram */
	sum_dist = 0;
	for (object_index = 1; object_index <= *count; object_index++) {
		sum_dist += pow(centroid_dists[object_index-1] - *DistMean, 2);
		if (centroid_dists[object_index-1] == *DistMax) dist_histogram[num_bins-1] += 1;
		else dist_histogram[(int)(((centroid_dists[object_index-1] - *DistMin) / (*DistMax-*DistMin))*num_bins)] += 1;
	}
	if (*count>1) *DistVar = sum_dist / ((*count)-1);
	else *DistVar = sum_dist;

	delete BWImage;
	delete [] object_areas;
	delete [] centroid_dists;
}

/* GaborFilters */
/* ratios -array of double- a pre-allocated array of double[7]
*/
void ImageMatrix::GaborFilters2D(double *ratios) {
	GaborTextureFilters2D(this, ratios);
}


/* haralick
   output -array of double- a pre-allocated array of 28 doubles
*/
void ImageMatrix::HaralickTexture2D(double distance, double *out) {
	if (distance<=0) distance=1;
	haralick2D(this,distance,out);
}

/* MultiScaleHistogram
   histograms into 3,5,7,9 bins
   Function computes signatures based on "multiscale histograms" idea.
   Idea of multiscale histogram came from the belief of a unique representativity of an
   image through infinite series of histograms with sequentially increasing number of bins.
   Here we used 4 histograms with number of bins being 3,5,7,9.
   out -array of double- a pre-allocated array of 24 bins
*/
void ImageMatrix::MultiScaleHistogram(double *out) {
	int a;
	double max=0;
	histogram(out,3,0);
	histogram(&(out[3]),5,0);
	histogram(&(out[8]),7,0);
	histogram(&(out[15]),9,0);
	for (a=0;a<24;a++)
		if (out[a]>max) max = out[a];
	for (a=0;a<24;a++)
		out[a] = out[a] / max;
}

/* TamuraTexture
   Tamura texture signatures: coarseness, directionality, contrast
   vec -array of double- a pre-allocated array of 6 doubles
*/
void ImageMatrix::TamuraTexture2D(double *vec) {
	Tamura3Sigs2D(this,vec);
}

/* zernike
   zvalue -array of double- a pre-allocated array of double of a suficient size
                            (the actual size is returned by "output_size))
   output_size -* long- the number of enteries in the array "zvalues" (normally 72)
*/
void ImageMatrix::zernike2D(double *zvalues, long *output_size) {
	mb_zernike2D(this, 0, 0, zvalues, output_size);
}



