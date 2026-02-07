/*
 * Core image reading routines.
 * This file is part of libresdet.
 */

#include "image.h"

#include <ctype.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// advance the file pointer by reading if buf is provided, seeking otherwise
RDError resdet_fskip(FILE* f, uint64_t offset, void* buf) {
	if(buf) {
		if(fread(buf,1,offset,f) != offset)
			return RDEINVAL;
	}
	else if(fseek(f,offset,SEEK_CUR) < 0)
		return -errno;
	return RDEOK;
}

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

RESDET_API RDImage* resdet_open_image(const char* filename, const char* filetype, size_t* width, size_t* height, float** imagebuf, RDError* error) {
	if(imagebuf)
		*imagebuf = NULL;

	RDError e = RDEOK;

	if(error)
		*error = RDEOK;

	RDImage* rdimage = NULL;

	if(!(width && height)) {
		e = RDEPARAM;
		goto error;
	}

	*width = *height = 0;

	if(!filename) {
		e = RDEPARAM;
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
	const struct image_reader** image_readers = resdet_image_readers();
	for(size_t i = 0; image_readers[i]; i++)
		if(image_readers[i]->supports_ext(ext)) {
			rdimage->reader = image_readers[i];
			break;
		}

	if(!rdimage->reader) {
		e = RDEUNSUPP;
		goto error;
	}

#ifdef _WIN32
	if(!strcmp(filename,"-"))
		_setmode(_fileno(stdin), _O_BINARY);
#endif

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

RESDET_API bool resdet_read_image_frame(RDImage* rdimage, float* image, RDError* error) {
	if(!(rdimage && image)) {
		if(error)
			*error = RDEPARAM;
		return false;
	}

	RDError e = RDEOK;
	bool ret = rdimage->reader->read_frame(rdimage->reader_ctx,image,rdimage->width,rdimage->height,&e);
	if(error)
		*error = e;
	if(e)
		return false;

	return ret;
}

RESDET_API bool resdet_seek_frame(RDImage* rdimage, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, RDError* error) {
	if(!rdimage) {
		if(error)
			*error = RDEPARAM;
		return false;
	}

	RDError e = RDEOK;
	bool ret = rdimage->reader->seek_frame(rdimage->reader_ctx,offset,progress,progress_ctx,rdimage->width,rdimage->height,&e);

	if(error)
		*error = e;
	if(e)
		return false;

	return ret;
}

RESDET_API void resdet_close_image(RDImage* rdimage) {
	if(!rdimage)
		return;

	if(rdimage->reader)
		rdimage->reader->close(rdimage->reader_ctx);
	free(rdimage);
}

RESDET_API RDError resdet_read_image(const char* filename, const char* filetype, float** images, size_t* nimages, size_t* width, size_t* height) {
	*nimages = 0;
	*images = NULL;

	RDError error;
	RDImage* image = resdet_open_image(filename,filetype,width,height,images,&error);
	if(error)
		return error;

	while(resdet_read_image_frame(image,*images + *width * *height * *nimages,&error)) {
		(*nimages)++;

		if(resdet_dims_exceed_limit(*width,*height,*nimages+1,float)) {
			error = RDETOOBIG;
			break;
		}

		float* img = realloc(*images,*width * *height * (*nimages+1) * sizeof(**images));
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
