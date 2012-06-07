from pycharm import pymfg

#================================================================
def LoadFeatureAlgorithms():
	out = {}
	out[ 'Chebyshev Coefficients' ] = pymfg.ChebyshevCoefficients()
	out[ 'Chebyshev-Fourier Coefficients' ] = pymfg.ChebyshevFourierCoefficients()
	out[ 'Zernike Coefficients' ] = pymfg.ZernikeCoefficients()
	out[ 'Haralick Textures' ] = pymfg.HaralickTextures()
	out[ 'Multiscale Histograms' ] = pymfg.MultiscaleHistograms()
	out[ 'Tamura Textures' ] = pymfg.TamuraTextures()
	out[ 'Comb Moments' ] = pymfg.CombFirstFourMoments()
	out[ 'Radon Coefficients' ] = pymfg.RadonCoefficients()
	out[ 'Fractal Features' ] = pymfg.FractalFeatures()
	out[ 'Pixel Intensity Statistics' ] = pymfg.PixelIntensityStatistics()
	out[ 'Edge Features' ] = pymfg.EdgeFeatures()
	out[ 'Object Features' ] = pymfg.ObjectFeatures()
	out[ 'Gabor Textures' ] = pymfg.GaborTextures()
	out[ 'Gini Coefficient' ] = pymfg.GiniCoefficient()
	return out


#================================================================
def LoadFeatureTransforms():
	out = {}
	out[ 'Fourier' ] = pymfg.FourierTransform()
	out[ 'Chebyshev' ] = pymfg.ChebyshevTransform()
	out[ 'Wavelet' ] = pymfg.WaveletTransform()
	out[ 'Edge' ] = pymfg.EdgeTransform()
	out[ 'Color' ] = pymfg.ColorTransform()
	out[ 'Hue' ] = pymfg.HueTransform()
	return out


#================================================================
def LoadSmallAndLargeFeatureSetStringLists():

	small = """Chebyshev Coefficients ()
Chebyshev Coefficients (Fourier ())
Chebyshev-Fourier Coefficients ()
Chebyshev-Fourier Coefficients (Fourier ())
Comb Moments ()
Comb Moments (Chebyshev ())
Comb Moments (Chebyshev (Fourier ()))
Comb Moments (Fourier ())
Comb Moments (Wavelet ())
Comb Moments (Wavelet (Fourier ()))
Edge Features ()
Gabor Textures ()
Haralick Textures ()
Haralick Textures (Chebyshev ())
Haralick Textures (Chebyshev (Fourier ()))
Haralick Textures (Fourier ())
Haralick Textures (Wavelet ())
Haralick Textures (Wavelet (Fourier ()))
Multiscale Histograms ()
Multiscale Histograms (Chebyshev ())
Multiscale Histograms (Chebyshev (Fourier ()))
Multiscale Histograms (Fourier ())
Multiscale Histograms (Wavelet ())
Multiscale Histograms (Wavelet (Fourier ()))
Object Features ()
Radon Coefficients ()
Radon Coefficients (Chebyshev ())
Radon Coefficients (Chebyshev (Fourier ()))
Radon Coefficients (Fourier ())
Tamura Textures ()
Tamura Textures (Chebyshev ())
Tamura Textures (Chebyshev (Fourier ()))
Tamura Textures (Fourier ())
Tamura Textures (Wavelet ())
Tamura Textures (Wavelet (Fourier ()))
Zernike Coefficients ()
Zernike Coefficients (Fourier ())"""

	large = """Chebyshev Coefficients (Chebyshev ())
Chebyshev Coefficients (Edge ())
Chebyshev Coefficients (Fourier (Edge ()))
Chebyshev Coefficients (Fourier (Wavelet ()))
Chebyshev Coefficients (Wavelet ())
Chebyshev Coefficients (Wavelet (Edge ()))
Chebyshev-Fourier Coefficients (Chebyshev ())
Chebyshev-Fourier Coefficients (Edge ())
Chebyshev-Fourier Coefficients (Fourier (Edge ()))
Chebyshev-Fourier Coefficients (Fourier (Wavelet ()))
Chebyshev-Fourier Coefficients (Wavelet ())
Chebyshev-Fourier Coefficients (Wavelet (Edge ()))
Comb Moments (Chebyshev (Wavelet ()))
Comb Moments (Edge ())
Comb Moments (Fourier (Chebyshev ()))
Comb Moments (Fourier (Edge ()))
Comb Moments (Fourier (Wavelet ()))
Comb Moments (Wavelet (Edge ()))
Fractal Features ()
Fractal Features (Chebyshev ())
Fractal Features (Chebyshev (Fourier ()))
Fractal Features (Chebyshev (Wavelet ()))
Fractal Features (Edge ())
Fractal Features (Fourier ())
Fractal Features (Fourier (Chebyshev ()))
Fractal Features (Fourier (Edge ()))
Fractal Features (Fourier (Wavelet ()))
Fractal Features (Wavelet ())
Fractal Features (Wavelet (Edge ()))
Fractal Features (Wavelet (Fourier ()))
Gini Coefficient ()
Gini Coefficient (Chebyshev ())
Gini Coefficient (Chebyshev (Fourier ()))
Gini Coefficient (Chebyshev (Wavelet ()))
Gini Coefficient (Edge ())
Gini Coefficient (Fourier ())
Gini Coefficient (Fourier (Chebyshev ()))
Gini Coefficient (Fourier (Edge ()))
Gini Coefficient (Fourier (Wavelet ()))
Gini Coefficient (Wavelet ())
Gini Coefficient (Wavelet (Edge ()))
Gini Coefficient (Wavelet (Fourier ()))
Haralick Textures (Chebyshev (Wavelet ()))
Haralick Textures (Edge ())
Haralick Textures (Fourier (Chebyshev ()))
Haralick Textures (Fourier (Edge ()))
Haralick Textures (Fourier (Wavelet ()))
Haralick Textures (Wavelet (Edge ()))
Multiscale Histograms (Chebyshev (Wavelet ()))
Multiscale Histograms (Edge ())
Multiscale Histograms (Fourier (Chebyshev ()))
Multiscale Histograms (Fourier (Edge ()))
Multiscale Histograms (Fourier (Wavelet ()))
Multiscale Histograms (Wavelet (Edge ()))
Pixel Intensity Statistics ()
Pixel Intensity Statistics (Chebyshev ())
Pixel Intensity Statistics (Chebyshev (Fourier ()))
Pixel Intensity Statistics (Chebyshev (Wavelet ()))
Pixel Intensity Statistics (Edge ())
Pixel Intensity Statistics (Fourier ())
Pixel Intensity Statistics (Fourier (Chebyshev ()))
Pixel Intensity Statistics (Fourier (Edge ()))
Pixel Intensity Statistics (Fourier (Wavelet ()))
Pixel Intensity Statistics (Wavelet ())
Pixel Intensity Statistics (Wavelet (Edge ()))
Pixel Intensity Statistics (Wavelet (Fourier ()))
Radon Coefficients (Chebyshev (Wavelet ()))
Radon Coefficients (Edge ())
Radon Coefficients (Fourier (Chebyshev ()))
Radon Coefficients (Fourier (Edge ()))
Radon Coefficients (Fourier (Wavelet ()))
Radon Coefficients (Wavelet ())
Radon Coefficients (Wavelet (Edge ()))
Radon Coefficients (Wavelet (Fourier ()))
Tamura Textures (Chebyshev (Wavelet ()))
Tamura Textures (Edge ())
Tamura Textures (Fourier (Chebyshev ()))
Tamura Textures (Fourier (Edge ()))
Tamura Textures (Fourier (Wavelet ()))
Tamura Textures (Wavelet (Edge ()))
Zernike Coefficients (Chebyshev ())
Zernike Coefficients (Edge ())
Zernike Coefficients (Fourier (Edge ()))
Zernike Coefficients (Fourier (Wavelet ()))
Zernike Coefficients (Wavelet ())
Zernike Coefficients (Wavelet (Edge ()))"""

	return small, large
