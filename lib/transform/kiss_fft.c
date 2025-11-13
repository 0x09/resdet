/*
 * KISS FFT transform backend.
 * This file is part of libresdet.
 */

#include "resdet_internal.h"

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
