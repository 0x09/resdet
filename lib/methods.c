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

#include <math.h>
#include <float.h>

#include "resdet_internal.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Sweeps the image looking for boundaries with many sign inversions.
// Fast, simple, and conveniently one of the most accurate methods.
static RDError detect_method_sign(const coeff* restrict f, size_t length, size_t range, double* result, rdint_index* restrict start, rdint_index* restrict end) {
	for(rdint_index x = *start; x < *end; x++) {
		rdint_storage sign_diff = 0;
		for(rdint_index i = 1; i <= range; i++)
			sign_diff += coeff_signbit(f[x-i]) != coeff_signbit(f[x+i]);
		result[x-*start] += sign_diff / (double)(range);
	}
	return RDEOK;
}

// Looks for similar magnitude coefficients with inverted signs.
static RDError detect_method_magnitude(const coeff* restrict f, size_t length, size_t range, double* result, rdint_index* restrict start, rdint_index* restrict end) {
	for(rdint_index x = *start; x < *end; x++) {
		rdint_storage mag_match = 0;
		for(rdint_index i = 1; i <= range; i++) {
			int e;
			mi(frexp)(f[x-i]/mc(copysign)(MAX(mc(fabs)(f[x+i]),EPSILON),f[x+i])+1,&e);
			mag_match += e <= 0;
		}
		result[x-*start] += mag_match / (double)(range);
	}
	return RDEOK;
}

// not quite identical to original method, but closest linear equivalent
static RDError detect_method_original(const coeff* restrict f, size_t length, size_t range, double* result, rdint_index* restrict start, rdint_index* restrict end) {
#ifndef MAG_RANGE
#define MAG_RANGE range
#endif
	for(rdint_index x = *start; x < *end; x++) {
		intermediate left = 0, right = 0, mid = mi(fabs)(f[x]) * MAG_RANGE;
		for(rdint_index i = 1; i <= MAG_RANGE; i++) {
			left += mi(fabs)(f[x-i]);
			right += mi(fabs)(f[x+i]);
		}
		intermediate lrdiff = mi(fabs)(left-right);
		if(MIN(left,right) > mid && lrdiff < mi(fabs)(left-mid) && lrdiff < mi(fabs)(right-mid)) {
			rdint_storage sign = 0;
			for(rdint_index i = 1; i <= range; i++)
				sign += coeff_signbit(f[x-i]) != coeff_signbit(f[x+i]);
			result[x-*start] += sign / (double)(range);
		}
	}
	return RDEOK;
}

// Lightweight version of original method
// Disregards range, only checks explicitly for zero crossings over all 3-element spans
static RDError detect_method_zerocrossing(const coeff* restrict f, size_t length, size_t range, double* result, rdint_index* restrict start, rdint_index* restrict end) {
	for(rdint_index x = *start; x < *end; x++) {
		rdint_storage zero_crossings = 0;
		coeff l = f[x-1], c = f[x], r = f[x+1];
		zero_crossings += coeff_signbit(l) != coeff_signbit(r) && ((l < c && c < r) || (l > c && c > r));
		result[x-*start] += zero_crossings;
	}
	return RDEOK;
}

static RDMethod methods[] = {
	{
		.name = "sign",
		.func = (void(*)(void))detect_method_sign,
		.threshold = 0.60
	},{
		.name = "mag",
		.func = (void(*)(void))detect_method_magnitude,
		.threshold = 0.45
	},{
		.name = "orig",
		.func = (void(*)(void))detect_method_original,
		.threshold = 0.32
	},{
		.name = "zerox",
		.func = (void(*)(void))detect_method_zerocrossing,
		.threshold = 0.45
	},
	{0}
};

RDMethod* resdet_methods() {
	return methods;
}
