/*
 * Core image reading routines.
 * This file is part of libresdet.
 */

#include "image.h"

#include <ctype.h>

static bool resdet_strieq(const char* left, const char* right) {
	while(*left && *right)
		if(tolower(*left++) != tolower(*right++))
			return false;
	return *left == *right;
}

static const char* mimetype_from_ext(const char* filename) {
	char* ext = strrchr(filename,'.');
	if(!ext)
		return "";

	ext++;
	if(resdet_strieq(ext,"jpg") || resdet_strieq(ext,"jpeg"))
		return "image/jpeg";
	if(resdet_strieq(ext,"png"))
		return "image/png";
	if(resdet_strieq(ext,"y4m"))
		return "video/yuv4mpeg";
	if(resdet_strieq(ext,"pgm"))
		return "image/x-portable-graymap";
	if(resdet_strieq(ext,"pfm"))
		return "image/x-portable-floatmap";
	return "";
}

RDError resdet_read_image(const char* filename, const char* mimetype, float** image, size_t* nimages, size_t* width, size_t* height) {
	*width = *height = *nimages = 0;
	*image = NULL;

	if(!filename)
		return RDEINTERNAL;

	const char* c = mimetype;
	if(!c)
		c = mimetype_from_ext(filename);

	struct image_reader* reader = NULL;
	if(false)
		;
#ifndef OMIT_NATIVE_PGM_READER
	else if(!strcmp(c,"image/x-portable-graymap")) {
		extern struct image_reader resdet_image_reader_pgm;
		reader = &resdet_image_reader_pgm;
	}
	else if(!strcmp(c,"image/x-portable-floatmap")) {
		extern struct image_reader resdet_image_reader_pfm;
		reader = &resdet_image_reader_pfm;
	}
#endif
#ifdef HAVE_LIBJPEG
	else if(!strcmp(c,"image/jpeg")) {
		extern struct image_reader resdet_image_reader_libjpeg;
		reader = &resdet_image_reader_libjpeg;
	}
#endif
#ifdef HAVE_LIBPNG
	else if(!strcmp(c,"image/png")) {
		extern struct image_reader resdet_image_reader_libpng;
		reader = &resdet_image_reader_libpng;
	}
#endif
#ifdef HAVE_MJPEGTOOLS
	else if(!strcmp(c,"video/yuv4mpeg")) {
		extern struct image_reader resdet_image_reader_mjpegtools;
		reader = &resdet_image_reader_mjpegtools;
	}
#endif
#ifdef HAVE_MAGICKWAND
	else {
		extern struct image_reader resdet_image_reader_magickwand;
		reader = &resdet_image_reader_magickwand;
	}
#endif

	if(!reader)
		return RDEUNSUPP;

	RDError e;
	*image = reader->read(filename,width,height,nimages,&e);

	return e;
}
