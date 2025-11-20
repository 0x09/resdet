/*
 * Core image reading routines.
 * This file is part of libresdet.
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>

#include "resdet_internal.h"

struct image_reader {
	void* (*open)(const char* filename, size_t* width, size_t* height, RDError*);
	bool (*read_frame)(void* reader_ctx, float* image, size_t width, size_t height, RDError*);
	void (*close)(void*);
	bool (*supports_ext)(const char*);
};

struct RDImage {
	const struct image_reader* reader;
	void* reader_ctx;
	size_t width, height;
};

bool resdet_strieq(const char* left, const char* right);

#endif
