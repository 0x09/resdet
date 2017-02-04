/*
 * libresdet - Detect source resolution of upscaled images.
 * Copyright (C) 2012-2017 0x09.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "resdet_internal.h"

#if HAVE_FFTW
#include <fftw3.h>

coeff* resdet_alloc_coeffs(size_t width, size_t height) {
	return fftwp(alloc_real)(width*height);
}

RDError resdet_transform(coeff* restrict f, size_t width, size_t height) {
	fftwp(plan) p = fftwp(plan_r2r_2d)(height,width,f,f,FFTW_REDFT10,FFTW_REDFT10,FFTW_ESTIMATE);
	if(!p)
		return RDEINTERNAL;
	fftwp(execute)(p);
	fftwp(destroy_plan)(p);
	return RDEOK;
}

void resdet_free_coeffs(coeff* f) {
	fftwp(free)(f);
}

#else

#if COEFF_PRECISION > 8
	#pragma message("KISS FFT does not support long or higher precision, KISS operations will use double instead.")
	#define kiss_fft_scalar double
#endif
#include "kiss_fftndr.h"

coeff* resdet_alloc_coeffs(size_t width, size_t height) {
	return malloc(sizeof(coeff)*width*height);
}

RDError resdet_transform(coeff* restrict f, size_t width, size_t height) {
	RDError ret = RDEOK;
	kiss_fftndr_cfg cfg = NULL;
	kiss_fft_scalar* mirror = NULL;
	kiss_fft_cpx* F = NULL;

	if(!((cfg    = kiss_fftndr_alloc((int[]){height*2,width*2},2,false,NULL,NULL)) && /* tower of malloc failures */
	     (mirror = malloc(sizeof(kiss_fft_scalar)*height*width*4))               &&
	     (F      = malloc(sizeof(kiss_fft_cpx)*(width+1)*height*2))
	)) {
		ret = RDENOMEM;
		goto end;
	}
	for(size_t y = 0; y < height; y++) {
		for(size_t x = 0; x < width; x++)
			mirror[y*width*2+x] = mirror[(y+1)*width*2-1-x] = f[y*width+x];
		memcpy(mirror + (height*2-1-y)*width*2, mirror + y*width*2, sizeof(kiss_fft_scalar)*width*2);
	}

	kiss_fftndr(cfg,mirror,F);

	// Shift is a simple factorization of
	// e^(-I*PI*n / 2N) * e^(-I*PI*m / 2M)
	intermediate EXP = -M_PI/(mi(2.)*width*height);
	for(size_t y = 0; y < height; y++)
		for(size_t x = 0; x < width; x++)
			f[y*width+x] =
				// In terms of C99 complex
				// creal((F[y*(width+1)+x].r + I*F[y*(width+1)+x].i) * cexp(I*(EXP*(x*width+y*height))));
				F[y*(width+1)+x].r * mi(cos)(EXP*(x*width+y*height)) -
				F[y*(width+1)+x].i * mi(sin)(EXP*(x*width+y*height));


end:
	free(F);
	free(mirror);
	free(cfg);
	return ret;
}

void resdet_free_coeffs(coeff* f) {
	free(f);
}
#endif
