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
*/

#include <stdio.h>

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

	return ctx;
error:
	resdet_close_context(ctx);
	return NULL;
}

void resdet_close_context(RDContext* ctx) {
#ifdef HAVE_MAGIC
	if(ctx && ctx->db) magic_close(ctx->db);
#endif
	free(ctx);
}


static RDError setup_dimension(size_t length, size_t range, RDResolution** detect, size_t* count, double** buf, rdint_index bounds[2]) {
	size_t min_length = range*2;
	if(!detect || min_length >= length)
		return RDEOK; // can't do anything

	if(!(*detect = malloc(sizeof(**detect) * (length-min_length+1))))
		return RDENOMEM;
	(*detect)[(*count)++] = (RDResolution){length,-1};

	if(!(*buf = calloc(length-min_length,sizeof(**buf))))
		return RDENOMEM;
	// bounds of result (range of meaningful outputs)
	// may be narrowed by methods
	bounds[0] = range;
	bounds[1] = length - range;
	return RDEOK;
}

RDError resdetect_with_params(unsigned char* restrict image, size_t nimages, size_t width, size_t height,
                                    RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                                    RDMethod* method, size_t range, float threshold) {

	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }
	if(!(method && range))
		return RDEINVAL;

	if(!(width && height && nimages) || (width > PIXEL_MAX / height) || (width*height > PIXEL_MAX / nimages))
		return RDEINVAL;
	coeff* f = NULL;
	if(!(f = resdet_alloc_coeffs(width,height)))
		return RDENOMEM;

	RDError ret = RDEOK;
	double* xresult = NULL,* yresult = NULL;

	resdet_plan* p;
	if((ret = resdet_create_plan(&p,f,width,height)))
		goto end;

	rdint_index xbound[2], ybound[2];
	if((ret = setup_dimension(width,range,rw,cw,&xresult,xbound)) != RDEOK)
		goto end;
	if((ret = setup_dimension(height,range,rh,ch,&yresult,ybound)) != RDEOK)
		goto end;

	for(size_t z = 0; z < nimages; z++) {
		for(rdint_index i = 0; i < width*height; i++)
			f[i] = image[z*width*height+i];
		resdet_transform(p);

		if(rw && *rw && (ret = ((RDetectFunc)method->func)(f,width,height,width,1,range,xresult,xbound,xbound+1)) != RDEOK)
			goto end;
		if(rh && *rh && (ret = ((RDetectFunc)method->func)(f,height,width,1,width,range,yresult,ybound,ybound+1)) != RDEOK)
			goto end;
	}

	for(rdint_index i = 0; rw && *rw && i < xbound[1]-xbound[0]; i++)
		if(xresult[i]/nimages >= threshold)
			(*rw)[(*cw)++] = (RDResolution){i+xbound[0],xresult[i]/nimages};

	for(rdint_index i = 0; rh && *rh && i < ybound[1]-ybound[0]; i++)
		if(yresult[i]/nimages >= threshold)
			(*rh)[(*ch)++] = (RDResolution){i+ybound[0],yresult[i]/nimages};

end:
	free(xresult);
	free(yresult);
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

RDError resdetect(unsigned char* restrict image, size_t nimages, size_t width, size_t height, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch, RDMethod* method) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }
	if(!method)
		return RDEINVAL;

	return resdetect_with_params(image,nimages,width,height,rw,cw,rh,ch,method,DEFAULT_RANGE,method->threshold);
}

RDError resdetect_file_with_params(RDContext* ctx, const char* filename, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                                   RDMethod* method, size_t range, float threshold) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }

	unsigned char* image = NULL;
	size_t width, height, nimages;
	RDError ret = resdet_read_image(ctx,filename,&image,&nimages,&width,&height);
	if(ret == RDEOK)
		ret = resdetect_with_params(image,nimages,width,height,rw,cw,rh,ch,method,range,threshold);
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
