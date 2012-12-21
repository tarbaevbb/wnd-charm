//---------------------------------------------------------------------------

#include <math.h>
#include "../statistics/CombFirst4Moments.h"
#include "tamura.h"



double contrast(ImageMatrix *image) {
	double *vec;
	unsigned int x,y;
	double avg,std,k;
	readOnlyPixels pix_plane = image->ReadablePixels();

	vec = new double[image->width*image->height];
	avg = 0;
	for (x = 0; x < image->width; x++) {
		for (y = 0; y < image->height; y++) {
			vec[x*image->height+y] = pix_plane(y,x);
			avg += pix_plane(y,x);
		}
	}
	avg = avg/(image->width*image->height);
	std = 0;
	for (x = 0; x < image->width; x++)
		for (y = 0; y < image->height; y++)
			std += (pix_plane(y,x) - avg)*(pix_plane(y,x) - avg);
	std = sqrt(std/(image->width*image->height));
	k = kurtosis(vec, avg, std, image->width*image->height);
	delete [] vec;
	if (std < 0.0000000001) return(0);
	else return(std / pow(k/pow(std,4),0.25)  );
}


#define  NBINS 125
double directionality(ImageMatrix *image) {
	double sum,sum_r;
	long a;
	unsigned int x, xdim = image->width;
	unsigned int y, ydim = image->height;
	double Hd[NBINS];

	ImageMatrix deltaH (*image);
	ImageMatrix deltaV (*image);

	ImageMatrix matrixH (3,3);
	pixData &matrixH_pix_plane = matrixH.WriteablePixels();
	matrixH_pix_plane.setZero();

	ImageMatrix matrixV (3,3);
	pixData &matrixV_pix_plane = matrixV.WriteablePixels();
	matrixV_pix_plane.setZero();


	//step1
	matrixH_pix_plane(0,0) = -1; matrixH_pix_plane(1,0) = -2; matrixH_pix_plane(2,0) = -1;
	matrixH_pix_plane(0,2) =  1; matrixH_pix_plane(1,2) =  2; matrixH_pix_plane(2,2) = -1;

	matrixV_pix_plane(0,0) =  1; matrixH_pix_plane(0,1) =  2; matrixH_pix_plane(0,2) =  1;
	matrixV_pix_plane(2,0) = -1; matrixH_pix_plane(2,1) = -2; matrixH_pix_plane(2,2) = -1;
	matrixH.WriteablePixelsFinish();
	matrixV.WriteablePixelsFinish();

	deltaH.convolve(matrixH);
	deltaV.convolve(matrixV);
	deltaH.WriteablePixelsFinish();
	deltaV.WriteablePixelsFinish();
	readOnlyPixels deltaH_pix_plane = deltaH.ReadOnlyPixels();
	readOnlyPixels deltaV_pix_plane = deltaV.ReadOnlyPixels();
	

	//step2
	ImageMatrix phi (xdim,ydim);
	pixData &phi_pix_plane = phi.WriteablePixels();

	sum_r = 0;
	for (y = 0; y < ydim; ++y) {
		for (x = 0; x < xdim; ++x) {
			if (deltaH_pix_plane(y,x) >= 0.0001) {
				phi_pix_plane(y,x) = atan(deltaV_pix_plane(y,x) / deltaH_pix_plane(y,x))+(M_PI/2.0+0.001); //+0.001 because otherwise sometimes getting -6.12574e-17
				sum_r += pow(deltaH_pix_plane(y,x),2)+pow(deltaV_pix_plane(y,x),2)+pow(phi_pix_plane(y,x),2);
			} else phi_pix_plane(y,x) = 0.0;
		}
	}
	phi.WriteablePixelsFinish();
	phi.histogram(Hd,NBINS,0);

	double max = 0.0;
	long fmx = 0;
	for (a = 0; a < NBINS; a++) {
		if (Hd[a] > max) {
			max = Hd[a];
			fmx = a;
		}
	}

	sum = 0;
	for (a = 0; a < NBINS; a++)
		sum += Hd[a]*pow((double)(a+1-fmx),2);

	return(fabs(log(sum/sum_r+0.0000001)));

}


double efficientLocalMean(const int x,const int y,const int k, ImageMatrix *laufendeSumme) {
	int k2 = k/2;
	readOnlyPixels laufendeSumme_pix_plane = laufendeSumme->ReadablePixels();

	int dimx = laufendeSumme->width;
	int dimy = laufendeSumme->height;

	//wanting average over area: (y-k2,x-k2) ... (y+k2-1, x+k2-1)
	int starty = y-k2;
	int startx = x-k2;
	int stopy = y+k2-1;
	int stopx = x+k2-1;

	if (starty < 0) starty = 0;
	if (startx < 0) startx = 0;
	if (stopx > dimx-1) stopx = dimx-1;
	if (stopy > dimy-1) stopy = dimy-1;

	double unten, links, oben, obenlinks;

	if (startx-1 < 0) links = 0;
	else links = laufendeSumme_pix_plane(stopy,startx-1);

	if (starty-1 < 0) oben = 0;
	else oben = laufendeSumme_pix_plane(stopy-1,startx);

	if ((starty-1 < 0) || (startx-1 <0)) obenlinks = 0;
	else obenlinks = laufendeSumme_pix_plane(stopy-1,startx-1);

	unten = laufendeSumme_pix_plane(stopy,startx);

	//   cout << "obenlinks = " << obenlinks << " oben = " << oben << " links = " << links << " unten = " <<unten << endl;
	int counter = (stopy-starty+1)*(stopx-startx+1);
	return (unten-links-oben+obenlinks)/counter;
}


/* coarseness
   hist -array of double- a pre-allocated array of "nbins" enetries
*/
// K_VALUE can also be 5
#define K_VALUE 7
double coarseness(ImageMatrix *image, double *hist,int nbins) {
	int x,y,k,max;
	const unsigned int yDim = image->height;
	const unsigned int xDim = image->width;
	double sum = 0.0;
	ImageMatrix *laufendeSumme,*Ak[K_VALUE],*Ekh[K_VALUE],*Ekv[K_VALUE],*Sbest;

	readOnlyPixels image_pix_plane = image->ReadablePixels();
	//  laufendeSumme = new ImageMatrix;
	laufendeSumme = new ImageMatrix (*image);
	pixData &laufendeSumme_pix_plane = laufendeSumme->WriteablePixels();


	// initialize for running sum calculation
	double links, oben, obenlinks;
	for(y = 0; y < (int)yDim; ++y) {
		for(x = 0; x < (int)xDim; ++x) {
			if(x < 1) links = 0;
			else links = laufendeSumme_pix_plane(y,x-1);

			if(y < 1) oben = 0;
			else oben = laufendeSumme_pix_plane(y-1,x);

			if(y < 1 || x < 1) obenlinks = 0;
			else obenlinks = laufendeSumme_pix_plane(y-1,x-1);

			laufendeSumme_pix_plane(y,x) = image_pix_plane(y,x) + links + oben - obenlinks;
		}
	}
	laufendeSumme->WriteablePixelsFinish();

	for (k = 1; k <= K_VALUE; k++) {
		Ak[k-1] = new ImageMatrix(xDim,yDim);
		Ekh[k-1] = new ImageMatrix(xDim,yDim);
		Ekv[k-1] = new ImageMatrix(xDim,yDim);
	}
	Sbest = new ImageMatrix(image->width,image->height);


	//step 1
	int lenOfk = 1;
	for(k = 1; k <= K_VALUE; ++k) {
		lenOfk *= 2;
		pixData &Ak_pix_plane = Ak[k-1]->WriteablePixels();
		for(y = 0; y < (int)yDim; ++y)
			for(x = 0; x < (int)xDim; ++x)
				Ak_pix_plane(y,x) = efficientLocalMean(x,y,lenOfk,laufendeSumme);
		Ak[k-1]->WriteablePixelsFinish();
	}

	//step 2
	lenOfk = 1;
	for(k = 1; k <= K_VALUE; ++k) {
		int k2 = lenOfk;
		lenOfk *= 2;
		pixData &Ekh_pix_plane = Ekh[k-1]->WriteablePixels();
		pixData &Ekv_pix_plane = Ekv[k-1]->WriteablePixels();
		readOnlyPixels Ak_pix_plane = Ak[k-1]->ReadablePixels();
		for(y = 0; y < (int)yDim; ++y) {
			for(x = 0; x < (int)xDim; ++x) {
				int posx1 = x+k2;
				int posx2 = x-k2;

				int posy1 = y+k2;
				int posy2 = y-k2;
				if(posx1 < (int)xDim && posx2 >= 0)
					Ekh_pix_plane(y,x) = fabs(Ak_pix_plane(y,posx1) - Ak_pix_plane(y,posx2));
				else Ekh_pix_plane(y,x) = 0;
				if(posy1 < (int)yDim && posy2 >= 0)
					Ekv_pix_plane(y,x) = fabs(Ak_pix_plane(posy1,x) - Ak_pix_plane(posy2,x));
				else Ekv_pix_plane(y,x) = 0;
			}
		}
		Ekh[k-1]->WriteablePixelsFinish();
		Ekv[k-1]->WriteablePixelsFinish();
	}

	//step3
	pixData &Sbest_pix_plane = Sbest->WriteablePixels();
	for(y = 0; y < (int)yDim; ++y) {
		for(x = 0; x < (int)xDim; ++x) {
			double maxE = 0;
			int maxk = 0;
			for(int k = 1; k <= K_VALUE; ++k) {
				double Ekh_val = Ekh[k-1]->ReadablePixels()(y,x);
				double Ekv_val = Ekv[k-1]->ReadablePixels()(y,x);
				if(Ekh_val > maxE) {
					maxE = Ekh_val;
					maxk = k;
				}
				if(Ekv_val > maxE) {
					maxE = Ekv_val;
					maxk = k;
				}
			}
			Sbest_pix_plane(y,x) = maxk;
			sum += maxk;
		}
	}
	Sbest->WriteablePixelsFinish();

	/* calculate the average coarseness */
	if (yDim == 32 || xDim == 32) sum /= ((xDim+1-32)*(yDim+1-32));     /* prevent division by zero */
	else sum /= ((yDim-32)*(xDim-32));

	/* calculate the 3-bin histogram */
	Sbest->histogram(hist,nbins,0);

	/* normalize the 3-bin histogram */
	max = (int)-INF;
	for (k = 0; k < nbins; k++)
		if (hist[k] > max) max = (int)(hist[k]);
	for (k = 0; k < nbins; k++)
		hist[k] = hist[k]/max;

	/* free allocated memory */
	delete laufendeSumme;
	for (k = 1; k <= K_VALUE; k++) {
		delete Ak[k-1];
		delete Ekh[k-1];
		delete Ekv[k-1];
	}
	delete Sbest;
	return(sum);  /* return the mean coarseness */
}




/* Tamura3Sigs
   vec -array of double- a pre-allocated array of 6 doubles
*/
void Tamura3Sigs2D(ImageMatrix *Im, double *vec) {
	double temp[6];
	temp[0] = coarseness(Im,&(temp[1]),3);
	temp[4] = directionality(Im);
	temp[5] = contrast(Im);

	/* rearange the order of the value so it will fit OME */
	vec[0] = temp[1];
	vec[1] = temp[2];
	vec[2] = temp[3];
	vec[3] = temp[5];
	vec[4] = temp[4];
	vec[5] = temp[0];
}

//---------------------------------------------------------------------------
