/*
 * resdet - Detect source resolution of upscaled images.
 * Public API header.
 * This file is part of libresdet.
 */

#ifndef RESDET_H
#define RESDET_H

#include <stddef.h>
#include <stdbool.h>

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

typedef const struct RDMethod {
	const char* name;
	void (*func)(void);
	float threshold;
} RDMethod;

typedef struct RDAnalysis RDAnalysis;

typedef struct RDImage RDImage;

const char* resdet_libversion(void);

const char* resdet_error_str(RDError);

RDMethod* resdet_methods(void);
RDMethod* resdet_get_method(const char* name);

size_t resdet_default_range(void);


RDImage* resdet_open_image(const char* filename, const char* mimetype, size_t* width, size_t* height, float** imagebuf, RDError* error);

bool resdet_read_image_frame(RDImage*, float* image, RDError* error);

void resdet_close_image(RDImage*);

RDError resdet_read_image(const char* filename, const char* mimetype, float** image, size_t* nimages, size_t* width, size_t* height);


RDAnalysis* resdet_create_analysis(RDMethod* method, size_t width, size_t height, RDError* error);

RDAnalysis* resdet_create_analysis_with_params(RDMethod* method, size_t width, size_t height, RDError* error, size_t range, float threshold);

RDError resdet_analyze_image(RDAnalysis*, float* image);

RDError resdet_analysis_results(RDAnalysis*, RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth);

void resdet_destroy_analysis(RDAnalysis*);


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
