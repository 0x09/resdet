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

#ifndef PRECISION_H
#define PRECISION_H

#ifndef COEFF_PRECISION
#define COEFF_PRECISION 4
#endif

#ifndef INTER_PRECISION
#define INTER_PRECISION 8
#endif

#if COEFF_PRECISION == 8
	#define COEFF_SUFFIX
	typedef double coeff;
#elif COEFF_PRECISION == 12
	#define COEFF_SUFFIX l
	typedef long double coeff;
#else
	#define COEFF_SUFFIX f
	typedef float coeff;
#endif

#if INTER_PRECISION < COEFF_PRECISION
	#pragma message("Intermediate precision less than coefficient precision, changing")
	#undef INTER_PRECISION
	#define INTER_PRECISION COEFF_PRECISION
#endif

#if INTER_PRECISION == 4
	#define EPSILON FLT_EPSILON
	#define INTER_SUFFIX f
	typedef float intermediate;
#elif INTER_PRECISION == 12
	#define EPSILON LDBL_EPSILON
	#define INTER_SUFFIX l
	typedef long double intermediate;
#else
	#define EPSILON DBL_EPSILON
	#define INTER_SUFFIX
	typedef double intermediate;
#endif

#define CAT_(x,y) x##y
#define CAT(x,y) CAT_(x,y)
#define mc(call) CAT(call,COEFF_SUFFIX)
#define mi(call) CAT(call,INTER_SUFFIX)
#define fftwp(call) CAT(mc(fftw),_##call)

#endif