/*
 * FFTW transform backend.
 * This file is part of libresdet.
 */

#include "resdet_internal.h"
#include <fftw3.h>

struct resdet_plan {
	fftwp(plan) plan;
};

coeff* resdet_alloc_coeffs(size_t width, size_t height) {
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
