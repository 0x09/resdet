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

#ifndef RESDET_INTERNAL_H
#define RESDET_INTERNAL_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef HAVE_MAGIC
#include <magic.h>
#else
typedef int magic_t;
#endif

#include "resdet.h"
#include "precision.h"

#ifdef RESDET_HOSTED //for server, ignoreme
#define PIXEL_MAX    (4096*4096)
typedef int_fast32_t rdint_index;
typedef uint32_t     rdint_storage;
#else
#define PIXEL_MAX SIZE_MAX
typedef size_t    rdint_index;
typedef size_t    rdint_storage;
#endif

#ifndef DEFAULT_RANGE
//32 originally but diminishing returns after 8 (8 seems to be ideal). smaller is faster, lower accuracy
#define DEFAULT_RANGE 12
#endif


struct RDContext {
	magic_t db;
};

typedef RDError(*RDetectFunc)(const coeff* restrict,size_t,size_t,size_t,size_t,size_t,double*,rdint_index*restrict,rdint_index*restrict);

coeff* resdet_alloc_coeffs(size_t,size_t);
RDError resdet_transform(coeff* restrict f, size_t width, size_t height);
void resdet_free_coeffs(coeff*);

#endif