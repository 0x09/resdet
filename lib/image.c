/*
 * Core image reading routines.
 * This file is part of libresdet.
 */

#include "image.h"

#include <ctype.h>

#ifndef OMIT_NATIVE_PGM_PFM_READERS
extern const struct image_reader resdet_image_reader_pgm;
extern const struct image_reader resdet_image_reader_pfm;
#endif
extern const struct image_reader resdet_image_reader_y4m;
#ifdef HAVE_LIBJPEG
extern const struct image_reader resdet_image_reader_libjpeg;
#endif
#ifdef HAVE_LIBPNG
extern const struct image_reader resdet_image_reader_libpng;
#endif
#ifdef HAVE_MAGICKWAND
extern const struct image_reader resdet_image_reader_magickwand;
#endif

static const struct image_reader* image_readers[] = {
#ifndef OMIT_NATIVE_PGM_PFM_READERS
	&resdet_image_reader_pgm,
	&resdet_image_reader_pfm,
#endif
	&resdet_image_reader_y4m,
#ifdef HAVE_LIBJPEG
	&resdet_image_reader_libjpeg,
#endif
#ifdef HAVE_LIBPNG
	&resdet_image_reader_libpng,
#endif
#ifdef HAVE_MAGICKWAND
	&resdet_image_reader_magickwand,
#endif
};

bool resdet_strieq(const char* left, const char* right) {
	while(*left && *right)
		if(tolower(*left++) != tolower(*right++))
			return false;
	return *left == *right;
}

static const char* ext_from_mimetype(const char* mimetype) {
	if(!strcmp(mimetype,"image/jpeg"))
		return "jpg";
	if(!strcmp(mimetype,"image/png"))
		return "png";
	if(!strcmp(mimetype,"video/yuv4mpeg"))
		return "y4m";
	if(!strcmp(mimetype,"image/x-portable-graymap"))
		return "pgm";
	if(!strcmp(mimetype,"image/x-portable-floatmap"))
		return "pfm";
	return "";
}

RDImage* resdet_open_image(const char* filename, const char* filetype, size_t* width, size_t* height, float** imagebuf, RDError* error) {
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

	const char* ext;
	if(filetype)
		ext = strchr(filetype,'/') ? ext_from_mimetype(filetype) : filetype;
	else {
		ext = strrchr(filename,'.');
		ext = ext ? ext+1 : "";
	}

	rdimage->reader = NULL;
	for(size_t i = 0; i < sizeof(image_readers)/sizeof(*image_readers); i++)
		if(image_readers[i]->supports_ext(ext)) {
			rdimage->reader = image_readers[i];
			break;
		}

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

RDError resdet_read_image(const char* filename, const char* filetype, float** images, size_t* nimages, size_t* width, size_t* height) {
	*nimages = 0;
	*images = NULL;

	RDError error;
	RDImage* image = resdet_open_image(filename,filetype,width,height,images,&error);
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
