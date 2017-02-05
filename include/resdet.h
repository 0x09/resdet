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

#ifndef RESDET_H
#define RESDET_H

#include <stddef.h>

typedef enum RDError {
	RDEOK = 0,
	RDENOMEM,
	RDEINTERNAL,
	RDEINVAL
} RDError;

static const char* const RDErrStr[] = {
	[RDEOK]       = "",
	[RDENOMEM]    = "Out of memory",
	[RDEINTERNAL] = "Internal error",
	[RDEINVAL]    = "Invalid image"
};

typedef struct RDContext RDContext;

typedef struct RDResolution {
	size_t index;
	float confidence;
} RDResolution;

RDContext* resdet_open_context();
void resdet_close_context(RDContext* ctx);

typedef struct RDMethod {
	const char* name;
	void (*const func)(void);
	const float threshold;
} RDMethod;

RDMethod* resdet_methods();
RDMethod* resdet_get_method(const char* name);

RDError resdet_read_image(RDContext* ctx, const char* filename, unsigned char** image, size_t* nimages, size_t* width, size_t* height);


RDError resdetect(unsigned char* restrict image, size_t nimages, size_t width, size_t height,
                  RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
                  RDMethod* method);

RDError resdetect_file(RDContext* ctx, const char* filename, RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth, RDMethod* method);

RDError resdetect_with_params(unsigned char* restrict image, size_t nimages, size_t width, size_t height,
                              RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
                              RDMethod* method, size_t range, float threshold);

RDError resdetect_file_with_params(RDContext* ctx, const char* filename,
                                   RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
                                   RDMethod* method, size_t range, float threshold);


#endif
