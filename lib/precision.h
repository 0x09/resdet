/*
 * Defines macros for toggling floating point precision of specific types at compile time.
 * This file is part of libresdet.
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
