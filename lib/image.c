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

#include "image.h"

static const char* mimetype_from_ext(const char* filename) {
	char* ext = strrchr(filename,'.');
	if(!ext)
		return "";
	if(!strcasecmp(ext+1,"jpg") || !strcasecmp(ext+1,"jpeg"))
		return "image/jpeg";
	if(!strcasecmp(ext+1,"png"))
		return "image/png";
	if(!strcasecmp(ext+1,"y4m"))
		return "video/yuv4mpeg";
	if(!strcasecmp(ext+1,"pgm"))
		return "image/x-portable-graymap";
	return "";
}

RDError resdet_read_image(const char* filename, const char* mimetype, unsigned char** image, size_t* nimages, size_t* width, size_t* height) {
	*width = *height = *nimages = 0;
	*image = NULL;

	if(!filename)
		return RDEINTERNAL;

	const char* c = mimetype;
	if(!c)
		c = mimetype_from_ext(filename);

	struct image_reader* reader = NULL;
	if(!strcmp(c,"image/x-portable-graymap")) {
		extern struct image_reader resdet_image_reader_pgm;
		reader = &resdet_image_reader_pgm;
	}
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

	if(reader)
		*image = reader->read(filename,width,height,nimages);

	return *image ? RDEOK : RDEINVAL;
}
