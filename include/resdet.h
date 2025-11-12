/*
 * libresdet - Detect source resolution of upscaled images.
 */

#ifndef RESDET_H
#define RESDET_H

#include <stddef.h>

typedef int RDError; // an RDErrors value or negated errno code

enum RDErrors {
	RDEOK = 0,
	RDENOMEM,
	RDEINTERNAL,
	RDEINVAL,
	RDEUNSUPP,
	RDETOOBIG,
	RDEPARAM,
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
