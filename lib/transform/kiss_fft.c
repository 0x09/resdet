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
	size_t width, height;
	kiss_fftr_cfg cfg[2];
	kiss_fft_scalar* mirror;
	kiss_fft_cpx* F,* shift[2];
};

coeff* resdet_alloc_coeffs(size_t width, size_t height) {
	return malloc(sizeof(coeff)*width*height);
}

RDError resdet_create_plan(resdet_plan** p, coeff* f, size_t width, size_t height) {
	size_t bufsize = width > height ? width : height;

	if(!(( *p            = calloc(1,sizeof(**p))                    ) && /* tower of malloc failures */
	     ((*p)->mirror   = malloc(sizeof(kiss_fft_scalar)*bufsize*2)) &&
	     ((*p)->F        = malloc(sizeof(kiss_fft_cpx)*(bufsize+1))   ) &&
	     ((*p)->shift[0] = malloc(sizeof(kiss_fft_cpx)*width)       ) &&
	     ((*p)->cfg[0]   = kiss_fftr_alloc(width*2,false,NULL,NULL) ) &&
	     (width == height || (
	         ((*p)->shift[1] = malloc(sizeof(kiss_fft_cpx)*height)      ) &&
	         ((*p)->cfg[1]   = kiss_fftr_alloc(height*2,false,NULL,NULL))
	      ))
	)) {
		resdet_free_plan(*p);
		*p = NULL;
		return RDENOMEM;
	}
	if(width == height) {
		(*p)->shift[1] = (*p)->shift[0];
		(*p)->cfg[1] = (*p)->cfg[0];
	}
	(*p)->f = f;
	(*p)->width = width;
	(*p)->height = height;

	// Precalculating this offers a decent speedup, especially with multiple frames
	for(size_t x = 0; x < width; x++) {
		(*p)->shift[0][x].r = mi(cos)(-(mi(atan)(1)*4)*x/(2*width));
		(*p)->shift[0][x].i = mi(sin)(-(mi(atan)(1)*4)*x/(2*width));
	}
	for(size_t y = 0; width != height && y < height; y++) {
		(*p)->shift[1][y].r = mi(cos)(-(mi(atan)(1)*4)*y/(2*height));
		(*p)->shift[1][y].i = mi(sin)(-(mi(atan)(1)*4)*y/(2*height));
	}
	return RDEOK;
}

static void kiss_dct(kiss_fftr_cfg cfg, coeff* restrict f, kiss_fft_cpx* restrict F, kiss_fft_scalar* restrict mirror, kiss_fft_cpx* restrict shift, size_t n, size_t length, size_t stride, size_t dist) {
	for(size_t j = 0; j < n; j++) {
		for(size_t i = 0; i < length; i++)
			mirror[i] = mirror[length*2-1-i] = f[j*dist+i*stride];
		kiss_fftr(cfg,mirror,F);
		for(size_t i = 0; i < length; i++)
			f[j*dist+i*stride] = F[i].r * shift[i].r - F[i].i * shift[i].i;
	}
}

void resdet_transform(resdet_plan* p) {
	size_t width = p->width, height = p->height;
	kiss_dct(p->cfg[0],p->f,p->F,p->mirror,p->shift[0],height,width,1,width);
	kiss_dct(p->cfg[1],p->f,p->F,p->mirror,p->shift[1],width,height,width,1);
}

void resdet_free_plan(resdet_plan* p) {
	if(p) {
		for(int i = 0; i < (p->width != p->height) + 1; i++) {
			free(p->cfg[i]);
			free(p->shift[i]);
		}
		free(p->mirror);
		free(p->F);
		free(p);
	}
}

void resdet_free_coeffs(coeff* f) {
	free(f);
}
