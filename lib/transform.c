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

coeff* resdet_alloc_coeffs(size_t length) {
	if(length > SIZE_MAX/sizeof(coeff))
		return NULL;
	return fftwp(alloc_real)(length);
}

RDError resdet_create_plan(resdet_plan** p, coeff* f, size_t length) {
	if(!(*p = malloc(sizeof(**p))))
		return RDENOMEM;
	if(!((*p)->plan = fftwp(plan_r2r_1d)(length,f,f,FFTW_REDFT10,FFTW_ESTIMATE))) {
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
	size_t length;
	kiss_fftr_cfg cfg;
	kiss_fft_scalar* mirror;
	kiss_fft_cpx* F,* shift;
};

coeff* resdet_alloc_coeffs(size_t length) {
	if(length > SIZE_MAX/sizeof(coeff))
		return NULL;
	return malloc(sizeof(coeff)*length);
}

RDError resdet_create_plan(resdet_plan** p, coeff* f, size_t length) {
	if(!(( *p            = calloc(1,sizeof(**p))                    ) && /* tower of malloc failures */
	     ((*p)->mirror   = malloc(sizeof(kiss_fft_scalar)*length*2) ) &&
	     ((*p)->F        = malloc(sizeof(kiss_fft_cpx)*(length+1))  ) &&
	     ((*p)->shift    = malloc(sizeof(kiss_fft_cpx)*length)      ) &&
	     ((*p)->cfg      = kiss_fftr_alloc(length*2,false,NULL,NULL))
	    )
	) {
		resdet_free_plan(*p);
		*p = NULL;
		return RDENOMEM;
	}
	(*p)->f = f;
	(*p)->length = length;

	// Precalculating this offers a decent speedup, especially with multiple frames
	for(size_t x = 0; x < length; x++) {
		(*p)->shift[x].r = mi(cos)(-M_PI*x/(2*length));
		(*p)->shift[x].i = mi(sin)(-M_PI*x/(2*length));
	}
	return RDEOK;
}

void resdet_transform(resdet_plan* p) {
	size_t length = p->length;
	kiss_fft_scalar* mirror = p->mirror;
	kiss_fft_cpx* F = p->F;
	coeff* f = p->f;
	kiss_fft_cpx* shift = p->shift;
	for(size_t x = 0; x < length; x++)
		mirror[x] = mirror[length*2-1-x] = f[x];
	kiss_fftr(p->cfg,mirror,F);
	for(size_t x = 0; x < length; x++)
		f[x] = F[x].r * shift[x].r - F[x].i * shift[x].i;
}

void resdet_free_plan(resdet_plan* p) {
	if(p) {
		free(p->cfg);
		free(p->shift);
		free(p->mirror);
		free(p->F);
		free(p);
	}
}

void resdet_free_coeffs(coeff* f) {
	free(f);
}
#endif
