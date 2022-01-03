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

static RDError setup_dimension(size_t length, size_t range, RDResolution** detect, size_t* count, double** buf, rdint_index bounds[2]) {
	if(!detect)
		return RDEOK;

	size_t maxlen = 0;
	if(range < (length+1)/2)
		maxlen = length - range*2;

	if(!(*detect = malloc(sizeof(**detect) * (maxlen+1))))
		return RDENOMEM;
	(*detect)[(*count)++] = (RDResolution){length,-1};

	if(!maxlen)
		return RDEOK;

	if(!(*buf = calloc(maxlen,sizeof(**buf))))
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

	RDError ret = RDEOK;
	coeff* fx = NULL,* fy = NULL;
	double* xresult = NULL,* yresult = NULL;
	resdet_plan* px = NULL,* py = NULL;

	if(!(fx = resdet_alloc_coeffs(width)))
		return RDENOMEM;
	if((ret = resdet_create_plan(&px,fx,width)))
		goto end;

	if(width == height) {
		fy = fx;
		py = px;
	}
	else {
		if(!(fy = resdet_alloc_coeffs(height))) {
			ret = RDENOMEM;
			goto end;
		}
		if((ret = resdet_create_plan(&py,fy,height)))
			goto end;
	}

	rdint_index xbound[2], ybound[2];
	if((ret = setup_dimension(width,range,rw,cw,&xresult,xbound)) != RDEOK)
		goto end;
	if((ret = setup_dimension(height,range,rh,ch,&yresult,ybound)) != RDEOK)
		goto end;

	for(size_t z = 0; z < nimages; z++) {
		for(rdint_index y = 0; y < height; y++) {
			for(rdint_index x = 0; x < width; x++)
				fx[x] = image[z*width*height+y*width+x];
			resdet_transform(px);
			if(rw && *rw && (ret = ((RDetectFunc)method->func)(fx,width,range,xresult,xbound,xbound+1)) != RDEOK)
				goto end;
		}
		for(rdint_index x = 0; x < width; x++) {
			for(rdint_index y = 0; y < height; y++)
				fy[y] = image[z*width*height+y*width+x];
			resdet_transform(py);
			if(rh && *rh && (ret = ((RDetectFunc)method->func)(fy,height,range,yresult,ybound,ybound+1)) != RDEOK)
				goto end;
		}
	}

	for(rdint_index i = 0; rw && *rw && i < xbound[1]-xbound[0]; i++)
		if(xresult[i]/(height*nimages) >= threshold)
			(*rw)[(*cw)++] = (RDResolution){i+xbound[0],xresult[i]/(height*nimages)};

	for(rdint_index i = 0; rh && *rh && i < ybound[1]-ybound[0]; i++)
		if(yresult[i]/(width*nimages) >= threshold)
			(*rh)[(*ch)++] = (RDResolution){i+ybound[0],yresult[i]/(width*nimages)};

end:
	free(xresult);
	free(yresult);
	resdet_free_plan(px);
	resdet_free_coeffs(fx);
	if(width != height) {
		resdet_free_plan(py);
		resdet_free_coeffs(fy);
	}
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

RDError resdetect_file_with_params(const char* filename, const char* mimetype, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch,
                                   RDMethod* method, size_t range, float threshold) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }

	unsigned char* image = NULL;
	size_t width, height, nimages;
	RDError ret = resdet_read_image(filename,mimetype,&image,&nimages,&width,&height);
	if(ret == RDEOK)
		ret = resdetect_with_params(image,nimages,width,height,rw,cw,rh,ch,method,range,threshold);
	free(image);
	return ret;
}

RDError resdetect_file(const char* filename, const char* mimetype, RDResolution** rw, size_t* cw, RDResolution** rh, size_t* ch, RDMethod* method) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }
	if(!method)
		return RDEINVAL;

	return resdetect_file_with_params(filename,mimetype,rw,cw,rh,ch,method,DEFAULT_RANGE,method->threshold);
}

size_t resdet_default_range() {
	return DEFAULT_RANGE;
}
