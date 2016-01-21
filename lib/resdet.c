/*
 * libresdet - Detect source resolution of upscaled images.
 * Copyright (C) 2012-2016 0x09.net
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

#include <fftw3.h>

#include "resdet_internal.h"

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

#ifdef RSRC_DIR
	fftwf_import_wisdom_from_filename(RSRC_DIR "/wisdom");
#endif

	return ctx;
error:
	resdet_close_context(ctx);
	return NULL;
}

void resdet_close_context(RDContext* ctx) {
	fftwf_cleanup();
#ifdef HAVE_MAGIC
	if(ctx && ctx->db) magic_close(ctx->db);
#endif
	free(ctx);
}

RDError resdetect_with_params(unsigned char* restrict image, size_t width, size_t height, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                              RDMethod* method, size_t range, float threshold) {
	if(!method)
		return RDEINVAL;

	if(rw) {
		*rw = NULL; *cw = 0;
	}
	if(rh) {
		*rh = NULL; *ch = 0;
	}

	RDError ret = RDEOK;
	float* f = fftwf_malloc(sizeof(*f)*width*height);
	if(!f) { ret = RDENOMEM; goto end; }
	fftwf_plan p = fftwf_plan_r2r_2d(height,width,f,f,FFTW_REDFT10,FFTW_REDFT10,FFTW_ESTIMATE);
	if(!p) { ret = RDEINTERNAL; goto end; }
	for(rdint_index i = 0; i < width*height; i++)
		f[i] = image[i];
	fftwf_execute(p);
	fftwf_destroy_plan(p);

	if(rw)
		if(!((ret = method->func(f,width,height,width,1,rw,cw,range,threshold))) == RDEOK)
			goto end;
	if(rh)
		if(!((ret = method->func(f,height,width,1,width,rh,ch,range,threshold))) == RDEOK)
			goto end;
	end:
	fftwf_free(f);
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
	if(!method)
		return RDEINVAL;
	return resdetect_with_params(image,width,height,rw,cw,rh,ch,method,DEFAULT_RANGE,method->threshold);
}

RDError resdetect_file_with_params(RDContext* ctx, const char* filename, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                                   RDMethod* method, size_t range, float threshold) {
	if(!method)
		return RDEINVAL;

	unsigned char* image = NULL;
	size_t width, height;
	RDError ret = resdet_read_image(ctx,filename,&image,&width,&height);
	if(ret == RDEOK)
		ret = resdetect_with_params(image,width,height,rw,cw,rh,ch,method,range,threshold);
	free(image);
	return ret;
}

RDError resdetect_file(RDContext* ctx, const char* filename, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch, RDMethod* method) {
	return resdetect_file_with_params(ctx,filename,rw,cw,rh,ch,method,DEFAULT_RANGE,method->threshold);
}

