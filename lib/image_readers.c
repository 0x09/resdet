/*
 * Image reader declarations.
 * This file is part of libresdet.
 */

#include "image.h"

#ifndef OMIT_PGM_READER
extern const struct image_reader resdet_image_reader_pgm;
#endif
#ifndef OMIT_PFM_READER
extern const struct image_reader resdet_image_reader_pfm;
#endif
#ifndef OMIT_Y4M_READER
extern const struct image_reader resdet_image_reader_y4m;
#endif
#ifdef HAVE_LIBJPEG
extern const struct image_reader resdet_image_reader_libjpeg;
#endif
#ifdef HAVE_LIBPNG
extern const struct image_reader resdet_image_reader_libpng;
#endif
#ifdef HAVE_MAGICKWAND
extern const struct image_reader resdet_image_reader_magickwand;
#endif
#ifdef HAVE_FFMPEG
extern const struct image_reader resdet_image_reader_ffmpeg;
#endif

const struct image_reader** resdet_image_readers(void) {
	static const struct image_reader* image_readers[] = {
#ifndef OMIT_PGM_READER
		&resdet_image_reader_pgm,
#endif
#ifndef OMIT_PFM_READER
		&resdet_image_reader_pfm,
#endif
#ifndef OMIT_Y4M_READER
		&resdet_image_reader_y4m,
#endif
#ifdef HAVE_LIBJPEG
		&resdet_image_reader_libjpeg,
#endif
#ifdef HAVE_LIBPNG
		&resdet_image_reader_libpng,
#endif
#ifdef HAVE_FFMPEG
		&resdet_image_reader_ffmpeg,
#endif
#ifdef HAVE_MAGICKWAND
		&resdet_image_reader_magickwand,
#endif
		NULL
	};

	return image_readers;
}

RESDET_API const char* const* resdet_list_image_readers(void) {
	static const char* const image_reader_names[] = {
#ifndef OMIT_PGM_READER
		"PGM",
#endif
#ifndef OMIT_PFM_READER
		"PFM",
#endif
#ifndef OMIT_Y4M_READER
		"Y4M",
#endif
#ifdef HAVE_LIBJPEG
		"libjpeg",
#endif
#ifdef HAVE_LIBPNG
		"libpng",
#endif
#ifdef HAVE_FFMPEG
		"FFmpeg",
#endif
#ifdef HAVE_MAGICKWAND
		"MagickWand",
#endif
		NULL
	};

	return image_reader_names;
}