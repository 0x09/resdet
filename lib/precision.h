/*
 * Defines macros for toggling floating point precision of specific types at compile time.
 * This file is part of libresdet.
 */

#ifndef PRECISION_H
#define PRECISION_H

#define F 1
#define D 2
#define L 3

#ifndef COEFF_PRECISION
#define COEFF_PRECISION F
#endif

#ifndef INTER_PRECISION
#define INTER_PRECISION D
#endif

#if COEFF_PRECISION == F
	#define COEFF_SUFFIX f
	typedef float coeff;
#elif COEFF_PRECISION == D
	#define COEFF_SUFFIX
	typedef double coeff;
#elif COEFF_PRECISION == L
	#define COEFF_SUFFIX l
	typedef long double coeff;
#else
	#error "Invalid value for COEFF_PRECISION"
#endif

#if INTER_PRECISION < COEFF_PRECISION
	#pragma message("Intermediate precision less than coefficient precision, changing")
	#undef INTER_PRECISION
	#define INTER_PRECISION COEFF_PRECISION
#endif

#if INTER_PRECISION == F
	#define EPSILON FLT_EPSILON
	#define INTER_SUFFIX f
	typedef float intermediate;
#elif INTER_PRECISION == D
	#define EPSILON DBL_EPSILON
	#define INTER_SUFFIX
	typedef double intermediate;
#elif INTER_PRECISION == L
	#define EPSILON LDBL_EPSILON
	#define INTER_SUFFIX l
	typedef long double intermediate;
#else
	#error "Invalid value for INTER_PRECISION"
#endif

#undef F
#undef D
#undef L

#define CAT_(x,y) x##y
#define CAT(x,y) CAT_(x,y)
#define mc(call) CAT(call,COEFF_SUFFIX)
#define mi(call) CAT(call,INTER_SUFFIX)
#define fftwp(call) CAT(mc(fftw),_##call)

#endif
