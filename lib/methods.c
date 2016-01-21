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

#include <math.h>
#include <float.h>

#include <fftw3.h>

#include "resdet_internal.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Sweeps the image looking for boundaries with many sign inversions.
// Fastest, simplest, and conveniently one of the most accurate methods.
static RDError detect_method_sign(const coeff* restrict f, size_t length, size_t n, size_t stride, size_t dist, RDResolution**detect, size_t* count, size_t range, float threshold) {
	if(!(*detect = malloc(sizeof(**detect)*(*count+1)))) return RDENOMEM;
	**detect = (RDResolution){length,-1}; *count = 1;
	if(range*2 >= length)
		return RDEOK; //can't do anything

	for(rdint_index x = range; x < length-range; x++) {
		rdint_storage sign = 0;
		for(rdint_index y = 0; y < n; y++)
			for(rdint_index i = 1; i <= range; i++)
				sign += signbit(f[y*stride+x*dist-i*dist]) != signbit(f[y*stride+x*dist+i*dist]);
		if(sign > range*n*threshold) {
			if(!(*detect = realloc(*detect,sizeof(**detect)*(*count+1)))) return RDENOMEM;
			(*detect)[(*count)++] = (RDResolution){x,sign/(float)(range*n)};
		}
	}
	return RDEOK;
}

// Looks for similar magnitude coefficients with inverted signs.
static RDError detect_method_magnitude(const coeff* restrict f, size_t length, size_t n, size_t stride, size_t dist, RDResolution**detect, size_t* count, size_t range, float threshold) {
	if(!(*detect = malloc(sizeof(**detect)*(*count+1)))) return RDENOMEM;
	**detect = (RDResolution){length,-1}; *count = 1;
	if(range*2 >= length)
		return RDEOK; //can't do anything

	for(rdint_index x = range; x < length-range; x++) {
		rdint_storage sign = 0;
		for(rdint_index y = 0; y < n; y++)
			for(rdint_index i = 1; i <= range; i++) {
				int e;
				mi(frexp)(f[y*stride+x*dist-i*dist]/mc(copysign)(MAX(mc(fabs)(f[y*stride+x*dist+i*dist]),EPSILON),f[y*stride+x*dist+i*dist])+1,&e);
				sign += e <= 0;
			}
		if(sign > range*n*threshold) {
			if(!(*detect = realloc(*detect,sizeof(**detect)*(*count+1)))) return RDENOMEM;
			(*detect)[(*count)++] = (RDResolution){x,sign/(float)(range*n)};
		}
	}
	return RDEOK;
}


// Initial algorithm. Somewhat more complicated mixture of the previous.
// Tests for inverted sign, same magnitude, with lower magnitude/zero crossing in between.
static RDError detect_method_original(const coeff* restrict f, size_t length, size_t n, size_t stride, size_t dist, RDResolution**detect, size_t* count, size_t range, float threshold) {
#ifndef MAG_RANGE
#define MAG_RANGE range
#endif

	if(!(*detect = malloc(sizeof(**detect)*(*count+1)))) return RDENOMEM;
	**detect = (RDResolution){length,-1}; *count = 1;
	if(range*2 >= length)
		return RDEOK; //can't do anything

	RDError ret = RDEOK;
	intermediate* line = NULL,* sum = NULL; rdint_storage* sign = NULL;
	if(!(sum = calloc(length,sizeof(*sum)))) { ret = RDENOMEM; goto end; }
	if(!(sign = calloc(length-range*2,sizeof(*sign)))) { ret = RDENOMEM; goto end; }

	rdint_index maxrange = MAX(MAG_RANGE,range);

	for(rdint_index y = 0; y < n; y++) {
		for(rdint_index x = 0; x < length; x++)
			sum[x] += mi(fabs)(f[y*stride+x*dist]);
		for(rdint_index x = range; x < length-range; x++)
			for(rdint_index i = 1; i <= range; i++)
				sign[x-range] += signbit(f[y*stride+x*dist-i*dist]) != signbit(f[y*stride+x*dist+i*dist]);
	}
	for(rdint_index x = 0; x < length; x++)
		sum[x] /= n;
	for(rdint_index x = maxrange; x < length-maxrange; x++) {
		intermediate left = 0, right = 0, mid = sum[x] * MAG_RANGE;
		for(rdint_index i = 1; i <= MAG_RANGE; i++) {
			left += sum[x-i];
			right += sum[x+i];
		}
		int leftexp, rightexp, midexp;
		mi(frexp)(left,&leftexp); mi(frexp)(right,&rightexp); mi(frexp)(mid,&midexp);
		if((abs(leftexp - rightexp) < 2) && (!mid || MIN(leftexp,rightexp) >= midexp) && (MIN(left,right) > mid) && (mi(fabs)(left - right) < mi(fabs)(left - mid) && mi(fabs)(left - right) < mi(fabs)(right - mid)) && 
		   (sign[x-range] > threshold*range*n)) {
			   if(!(*detect = realloc(*detect,sizeof(**detect)*(*count+1)))) { ret = RDENOMEM; goto end; }
			   (*detect)[(*count)++] = (RDResolution){x,(sign[x-range]/**2-range*height*/)/(float)(range*n)};
		}
	}

end:
	free(sum);
	free(sign);
	return ret;
}

// A few different attempts to ID the zero-crossing on an odd-ish extension of a function, of many
// Someone with a better math background may certainly know a better one
static RDMethod methods[] = {
	{
		.name = "sign",
		.func = (const void(*)(void))detect_method_sign,
		.threshold = 0.55
	},{
		.name = "mag",
		.func = (const void(*)(void))detect_method_magnitude,
		.threshold = 0.40
	},{
		.name = "orig",
		.func = (const void(*)(void))detect_method_original,
		.threshold = 0.64
	},
	{}
};

RDMethod* resdet_methods() {
	return methods;
}
