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

/*
todo:
	statistical selection
	union/intersect results
	other methods from RDView
	better accuracy could be had for videos by averaging across frames
*/

#include <stdio.h>

#include "resdet_internal.h"

#ifdef HAVE_FFTW
	#include <fftw3.h> //still needed for wisdom, cleanup
#endif

RDContext* resdet_open_context() {
	RDContext* ctx = malloc(sizeof(*ctx));
	if(!ctx) goto error;
#ifdef HAVE_MAGIC
	if(!(ctx->db = magic_open(MAGIC_MIME_TYPE))) goto error;
#ifdef RSRC_DIR //for server
	if(magic_load(ctx->db,RSRC_DIR "/magic")) goto error;
#else
	if(magic_load(ctx->db,NULL)) goto error;
#endif
#endif

#if defined(HAVE_FFTW) && defined(RSRC_DIR)
	fftwp(import_wisdom_from_filename)(RSRC_DIR "/wisdom");
#endif

	return ctx;
error:
	resdet_close_context(ctx);
	return NULL;
}

void resdet_close_context(RDContext* ctx) {
#ifdef HAVE_FFTW
	fftwp(cleanup)();
#endif
#ifdef HAVE_MAGIC
	if(ctx && ctx->db) magic_close(ctx->db);
#endif
	free(ctx);
}

static RDError detect_dimension(const coeff* restrict f, size_t length, size_t n, size_t stride, size_t dist, RDResolution** detect, size_t* count, RDMethod* method, size_t range, float threshold) {
	if(!detect)
		return RDEOK;

	size_t min_length = range*2;
	if(!(*detect = malloc(sizeof(**detect) * (length-min_length+1))))
		return RDENOMEM;
	(*detect)[(*count)++] = (RDResolution){length,-1};

	if(min_length >= length)
		return RDEOK; //can't do anything

	double* result = malloc(sizeof(*result) * (length-min_length));
	if(!result)
		return RDENOMEM;

	// bounds of result (range of meaningful outputs)
	// may be narrowed by methods
	rdint_index start = min_length, end = length - min_length;

	RDError ret = ((RDetectFunc)method->func)(f,length,n,stride,dist,range,result,&start,&end);

	if(ret == RDEOK)
		for(rdint_index i = 0; i < end-start; i++)
			if(result[i] >= threshold)
				(*detect)[(*count)++] = (RDResolution){i+start,result[i]};

	free(result);
	return ret;
}

RDError resdetect_with_params(unsigned char* restrict image, size_t width, size_t height, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                              RDMethod* method, size_t range, float threshold) {

	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }
	if(!(method && range))
		return RDEINVAL;

	if(!(width && height) || (width > PIXEL_MAX / height))
		return RDEINVAL;
	coeff* f = NULL;
	if(!(f = resdet_alloc_coeffs(width,height)))
		return RDENOMEM;

	RDError ret = RDEOK;
	for(rdint_index i = 0; i < width*height; i++)
		f[i] = image[i];

	resdet_plan* p;
	if((ret = resdet_create_plan(&p,f,width,height)))
		goto end;
	resdet_transform(p);

	if((ret = detect_dimension(f,width,height,width,1,rw,cw,method,range,threshold)) != RDEOK)
		goto end;
	if((ret = detect_dimension(f,height,width,1,width,rh,ch,method,range,threshold)) != RDEOK)
		goto end;

end:
	resdet_free_plan(p);
	resdet_free_coeffs(f);
	return ret;
}

RDMethod* resdet_get_method(const char* name) {
	if(!name)
		return resdet_methods(); //default
	for(RDMethod* m = resdet_methods(); m->name; m++)
		if(!strcmp(m->name,name))
			return m;
	return NULL;
}

RDError resdetect(unsigned char* restrict image, size_t width, size_t height, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch, RDMethod* method) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }
	if(!method)
		return RDEINVAL;

	return resdetect_with_params(image,width,height,rw,cw,rh,ch,method,DEFAULT_RANGE,method->threshold);
}

RDError resdetect_file_with_params(RDContext* ctx, const char* filename, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                                   RDMethod* method, size_t range, float threshold) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }

	unsigned char* image = NULL;
	size_t width, height;
	RDError ret = resdet_read_image(ctx,filename,&image,&width,&height);
	if(ret == RDEOK)
		ret = resdetect_with_params(image,width,height,rw,cw,rh,ch,method,range,threshold);
	free(image);
	return ret;
}

RDError resdetect_file(RDContext* ctx, const char* filename, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch, RDMethod* method) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }
	if(!method)
		return RDEINVAL;

	return resdetect_file_with_params(ctx,filename,rw,cw,rh,ch,method,DEFAULT_RANGE,method->threshold);
}

