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

typedef struct RDParameters RDParameters;

typedef struct RDAnalysis RDAnalysis;

typedef struct RDImage RDImage;

const char* resdet_libversion(void);

const char* resdet_error_str(RDError);

RDMethod* resdet_methods(void);
RDMethod* resdet_get_method(const char* name);

size_t resdet_default_range(void);

RDParameters* resdet_alloc_default_parameters(void);

RDError resdet_parameters_set_range(RDParameters*, size_t range);
RDError resdet_parameters_set_threshold(RDParameters*, float threshold);


RDImage* resdet_open_image(const char* filename, const char* mimetype, size_t* width, size_t* height, float** imagebuf, RDError* error);

bool resdet_read_image_frame(RDImage*, float* image, RDError* error);

void resdet_close_image(RDImage*);

RDError resdet_read_image(const char* filename, const char* mimetype, float** image, size_t* nimages, size_t* width, size_t* height);


RDAnalysis* resdet_create_analysis(RDMethod* method, size_t width, size_t height, const RDParameters* params, RDError* error);

RDError resdet_analyze_image(RDAnalysis*, float* image);

RDError resdet_analysis_results(RDAnalysis*,
                                RDResolution** restrict resw, size_t* restrict countw,
                                RDResolution** restrict resh, size_t* restrict counth);

void resdet_destroy_analysis(RDAnalysis*);


RDError resdetect(float* image, size_t nimages, size_t width, size_t height,
                  RDResolution** restrict resw, size_t* restrict countw,
                  RDResolution** restrict resh, size_t* restrict counth,
                  RDMethod* method, const RDParameters* params);

RDError resdetect_file(const char* filename, const char* mimetype,
                       RDResolution** restrict resw, size_t* restrict countw,
                       RDResolution** restrict resh, size_t* restrict counth,
                       RDMethod* method, const RDParameters* params);


#endif
