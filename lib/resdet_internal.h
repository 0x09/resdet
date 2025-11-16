/*
 * resdet internal API
 * This file is part of libresdet.
 */

#ifndef RESDET_INTERNAL_H
#define RESDET_INTERNAL_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#include "resdet.h"
#include "precision.h"

#ifndef VERSION_SUFFIX
#define VERSION_SUFFIX
#endif

#define RESDET_LIBVERSION_STRING "1.0.2" VERSION_SUFFIX

#if USE_BUILTIN_SIGNBIT
#define coeff_signbit(x) __builtin_signbit((x))
#else
#define coeff_signbit(x) signbit((x))
#endif

#ifndef PIXEL_MAX
#define PIXEL_MAX SIZE_MAX
#endif

#if PIXEL_MAX <= 0
#error "PIXEL_MAX must be greater than 0"
#elif PIXEL_MAX <= UINT32_MAX
typedef uint_fast32_t rdint_index;
typedef uint32_t rdint_storage;
#else
typedef size_t rdint_index;
typedef size_t rdint_storage;
#endif

#define resdet_dims_exceed_limit(width, height, nimages, buffer_type) !( \
	(width) && (height) && (nimages) && \
	((size_t)PIXEL_MAX) / (width) / (height) / (nimages) && \
	SIZE_MAX / (width) / (height) / (nimages) / sizeof(buffer_type) \
)

#ifndef DEFAULT_RANGE
#define DEFAULT_RANGE 12
#endif

#if DEFAULT_RANGE <= 0
#error "DEFAULT_RANGE must be greater than 0"
#endif

typedef RDError(*RDetectFunc)(const coeff* restrict,size_t,size_t,size_t,size_t,size_t,intermediate*,rdint_index*restrict,rdint_index*restrict);

typedef struct resdet_plan resdet_plan;

coeff* resdet_alloc_coeffs(size_t,size_t);
RDError resdet_create_plan(resdet_plan**,coeff* restrict, size_t, size_t);
void resdet_transform(resdet_plan*);
void resdet_free_plan(resdet_plan*);
void resdet_free_coeffs(coeff*);

#endif
