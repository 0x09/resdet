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

typedef int RDError;

enum RDErrors {
	RDEOK = 0,
	RDENOMEM,
	RDEINTERNAL,
	RDEINVAL,
	RDEUNSUPP,
};

typedef struct RDResolution {
	size_t index;
	float confidence;
} RDResolution;

typedef struct RDMethod {
	const char* name;
	void (*const func)(void);
	const float threshold;
} RDMethod;

const char* resdet_libversion(void);

const char* resdet_error_str(RDError);

RDMethod* resdet_methods(void);
RDMethod* resdet_get_method(const char* name);

size_t resdet_default_range(void);

RDError resdet_read_image(const char* filename, const char* mimetype, float** image, size_t* nimages, size_t* width, size_t* height);


RDError resdetect(float* image, size_t nimages, size_t width, size_t height,
                  RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
                  RDMethod* method);

RDError resdetect_file(const char* filename, const char* mimetype, RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth, RDMethod* method);

RDError resdetect_with_params(float* image, size_t nimages, size_t width, size_t height,
                              RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
                              RDMethod* method, size_t range, float threshold);

RDError resdetect_file_with_params(const char* filename, const char* mimetype,
                                   RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
                                   RDMethod* method, size_t range, float threshold);


#endif
