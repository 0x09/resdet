/*
 * resdet - Detect source resolution of upscaled images.
 * Public API header.
 * This file is part of libresdet.
 */

#ifndef RESDET_H
#define RESDET_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
	#ifdef RESDET_EXPORT
		#define RESDET_API __declspec(dllexport)
	#else
		#define RESDET_API __declspec(dllimport)
	#endif
#else
	#define RESDET_API __attribute__((visibility("default")))
#endif

typedef int RDError; // an RDErrors value or negated errno code

enum RDErrors {
	RDEOK = 0,
	RDENOMEM,
	RDEINTERNAL,
	RDEINVAL,
	RDEUNSUPP,
	RDETOOBIG,
	RDEPARAM,
	RDENOIMG,
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

RESDET_API const char* resdet_libversion(void);

RESDET_API const char* resdet_error_str(RDError);
RESDET_API enum RDErrors resdet_get_max_error(void);

RESDET_API RDMethod* resdet_methods(void);
RESDET_API RDMethod* resdet_get_method(const char* name);

RESDET_API size_t resdet_default_range(void);

RESDET_API RDParameters* resdet_alloc_default_parameters(void);

RESDET_API RDError resdet_parameters_set_range(RDParameters*, size_t range);
RESDET_API RDError resdet_parameters_set_threshold(RDParameters*, float threshold);


RESDET_API RDImage* resdet_open_image(const char* filename, const char* type, size_t* width, size_t* height, float** imagebuf, RDError* error);

RESDET_API bool resdet_read_image_frame(RDImage*, float* image, RDError* error);

RESDET_API bool resdet_seek_frame(RDImage*, uint64_t offset, void(*progress)(void* ctx, uint64_t frameno), void* progress_ctx, RDError* error);

RESDET_API void resdet_close_image(RDImage*);

RESDET_API RDError resdet_read_image(const char* filename, const char* filetype, float** image, size_t* nimages, size_t* width, size_t* height);


RESDET_API RDAnalysis* resdet_create_analysis(RDMethod* method, size_t width, size_t height, const RDParameters* params, RDError* error);

RESDET_API RDError resdet_analyze_image(RDAnalysis*, float* image);

RESDET_API RDError resdet_analysis_results(RDAnalysis*,
                                           RDResolution** restrict resw, size_t* restrict countw,
                                           RDResolution** restrict resh, size_t* restrict counth);

RESDET_API void resdet_destroy_analysis(RDAnalysis*);


RESDET_API RDError resdetect(float* image, size_t nimages, size_t width, size_t height,
                             RDResolution** restrict resw, size_t* restrict countw,
                             RDResolution** restrict resh, size_t* restrict counth,
                             RDMethod* method, const RDParameters* params);

RESDET_API RDError resdetect_file(const char* filename, const char* filetype,
                                  RDResolution** restrict resw, size_t* restrict countw,
                                  RDResolution** restrict resh, size_t* restrict counth,
                                  RDMethod* method, const RDParameters* params);


#endif
