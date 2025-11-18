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

RDImage* resdet_open_image(const char* filename, const char* mimetype, size_t* width, size_t* height, float** imagebuf, RDError* error) {
	*width = *height = 0;
	RDError e;

	if(error)
		*error = RDEOK;
	if(imagebuf)
		*imagebuf = NULL;

	RDImage* rdimage = NULL;

	if(!filename) {
		e = RDEINVAL;
		goto error;
	}

	if(!(rdimage = malloc(sizeof(*rdimage)))) {
		e = RDENOMEM;
		goto error;
	}

	const char* c = mimetype;
	if(!c)
		c = mimetype_from_ext(filename);

	rdimage->reader = NULL;
	if(false)
		;
#ifndef OMIT_NATIVE_PGM_READER
	else if(!strcmp(c,"image/x-portable-graymap")) {
		extern struct image_reader resdet_image_reader_pgm;
		rdimage->reader = &resdet_image_reader_pgm;
	}
	else if(!strcmp(c,"image/x-portable-floatmap")) {
		extern struct image_reader resdet_image_reader_pfm;
		rdimage->reader = &resdet_image_reader_pfm;
	}
#endif
	else if(!strcmp(c,"video/yuv4mpeg")) {
		extern struct image_reader resdet_image_reader_y4m;
		rdimage->reader = &resdet_image_reader_y4m;
	}
#ifdef HAVE_LIBJPEG
	else if(!strcmp(c,"image/jpeg")) {
		extern struct image_reader resdet_image_reader_libjpeg;
		rdimage->reader = &resdet_image_reader_libjpeg;
	}
#endif
#ifdef HAVE_LIBPNG
	else if(!strcmp(c,"image/png")) {
		extern struct image_reader resdet_image_reader_libpng;
		rdimage->reader = &resdet_image_reader_libpng;
	}
#endif
#ifdef HAVE_MAGICKWAND
	else {
		extern struct image_reader resdet_image_reader_magickwand;
		rdimage->reader = &resdet_image_reader_magickwand;
	}
#endif

	if(!rdimage->reader) {
		e = RDEUNSUPP;
		goto error;
	}

	rdimage->reader_ctx = rdimage->reader->open(filename,width,height,&e);
	if(e)
		goto error;

	if(resdet_dims_exceed_limit(*width,*height,1,float)) {
		e = RDETOOBIG;
		goto error;
	}

	rdimage->width = *width;
	rdimage->height = *height;

	if(imagebuf && !(*imagebuf = malloc(*width * *height * sizeof(**imagebuf)))) {
		e = RDENOMEM;
		goto error;
	}

	return rdimage;

error:
	resdet_close_image(rdimage);

	if(error)
		*error = e;

	return NULL;
}

bool resdet_read_image_frame(RDImage* rdimage, float* image, RDError* error) {
	if(!rdimage) {
		*error = RDEPARAM;
		return false;
	}

	RDError e;
	bool ret = rdimage->reader->read_frame(rdimage->reader_ctx,image,rdimage->width,rdimage->height,&e);
	if(e) {
		if(error)
			*error = e;
		return false;
	}

	return ret;
}

void resdet_close_image(RDImage* rdimage) {
	if(!rdimage)
		return;

	if(rdimage->reader)
		rdimage->reader->close(rdimage->reader_ctx);
	free(rdimage);
}

RDError resdet_read_image(const char* filename, const char* mimetype, float** images, size_t* nimages, size_t* width, size_t* height) {
	*nimages = 0;
	*images = NULL;

	RDError error;
	RDImage* image = resdet_open_image(filename,mimetype,width,height,images,&error);
	if(error)
		return error;

	while(resdet_read_image_frame(image,*images + *width * *height * *nimages,&error)) {
		(*nimages)++;

		if(resdet_dims_exceed_limit(*width,*height,*nimages,float)) {
			error = RDETOOBIG;
			break;
		}

		float* img = realloc(*images,*width * *height * *nimages * sizeof(**images));
		if(!img) {
			error = RDENOMEM;
			break;
		}

		*images = img;
	}

	if(error) {
		free(*images);
		*images = NULL;
	}

	resdet_close_image(image);

	return error;
}
