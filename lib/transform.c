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

struct resdet_plan {
	fftwp(plan) plan;
};

coeff* resdet_alloc_coeffs(size_t width, size_t height) {
	if(width > SIZE_MAX/height || width*height > SIZE_MAX/sizeof(coeff))
		return NULL;
	return fftwp(alloc_real)(width*height);
}

RDError resdet_create_plan(resdet_plan** p, coeff* f, size_t width, size_t height) {
	if(!(*p = malloc(sizeof(**p))))
		return RDENOMEM;
	if(!((*p)->plan = fftwp(plan_r2r_2d)(height,width,f,f,FFTW_REDFT10,FFTW_REDFT10,FFTW_ESTIMATE))) {
		resdet_free_plan(*p);
		*p = NULL;
		return RDEINTERNAL;
	}
	return RDEOK;
}

void resdet_transform(resdet_plan* p) {
	fftwp(execute)(p->plan);
}

void resdet_free_plan(resdet_plan* p) {
	if(p) {
		if(p->plan)
			fftwp(destroy_plan)(p->plan);
		free(p);
	}
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

struct resdet_plan {
	coeff* f;
	size_t width, height;
	kiss_fftr_cfg cfg[2];
	kiss_fft_scalar* mirror;
	kiss_fft_cpx* F;
};

coeff* resdet_alloc_coeffs(size_t width, size_t height) {
	if(width > SIZE_MAX/height || width*height > SIZE_MAX/sizeof(coeff))
		return NULL;
	return malloc(sizeof(coeff)*width*height);
}

RDError resdet_create_plan(resdet_plan** p, coeff* f, size_t width, size_t height) {
	size_t bufsize = width > height ? width : height;

	if(!((*p          = malloc(sizeof(**p)))                       && /* tower of malloc failures */
	    ((*p)->cfg[0] = kiss_fftr_alloc(width*2,false,NULL,NULL))  &&
	    ((*p)->cfg[1] = kiss_fftr_alloc(height*2,false,NULL,NULL)) &&
	    ((*p)->mirror = malloc(sizeof(kiss_fft_scalar)*bufsize*2)) &&
	    ((*p)->F      = malloc(sizeof(kiss_fft_cpx)*bufsize+1))
	)) {
		resdet_free_plan(*p);
		*p = NULL;
		return RDENOMEM;
	}
	(*p)->f = f;
	(*p)->width = width;
	(*p)->height = height;
	return RDEOK;
}

void resdet_transform(resdet_plan* p) {
	size_t width = p->width, height = p->height;
	kiss_fft_scalar* mirror = p->mirror;
	kiss_fft_cpx* F = p->F;
	coeff* f = p->f;
	intermediate shift;

	shift = -M_PI/(2*width);
	for(size_t y = 0; y < height; y++) {
		for(size_t x = 0; x < width; x++)
			mirror[x] = mirror[width*2-1-x] = f[y*width+x];
		kiss_fftr(p->cfg[0],mirror,F);
		for(size_t x = 0; x < width; x++)
			f[y*width+x] = F[x].r * mi(cos)(shift*x) - F[x].i * mi(sin)(shift*x);
	}

	shift = -M_PI/(2*height);
	for(size_t x = 0; x < width; x++) {
		for(size_t y = 0; y < height; y++)
			mirror[y] = mirror[height*2-1-y] = f[y*width+x];
		kiss_fftr(p->cfg[1],mirror,F);
		for(size_t y = 0; y < height; y++)
			f[y*width+x] = F[y].r * mi(cos)(shift*y) - F[y].i * mi(sin)(shift*y);
	}
}

void resdet_free_plan(resdet_plan* p) {
	if(p) {
		free(p->cfg[0]);
		free(p->cfg[1]);
		free(p->mirror);
		free(p->F);
		free(p);
	}
}

void resdet_free_coeffs(coeff* f) {
	free(f);
}
#endif
