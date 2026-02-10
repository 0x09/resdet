/*
 * Image reader for multiple file formats using ImageMagick's MagickWand library.
 * This file is part of libresdet.
 */

#include "image.h"

#if HAVE_MAGICKWAND > 6
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif

struct magickwand_context {
	MagickWand* wand;
};

static void magickwand_reader_close(void* reader_ctx) {
	struct magickwand_context* ctx = reader_ctx;
	if(ctx) {
		DestroyMagickWand(ctx->wand);
		free(ctx);
	}
}

static void* magickwand_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	struct magickwand_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		goto error;
	}

	ctx->wand = NewMagickWand();
	if(MagickReadImage(ctx->wand,filename) == MagickFalse) {
		ExceptionType ex;
		char* exception = MagickGetException(ctx->wand,&ex);
		if(ex == MissingDelegateError) {
			if(!strcmp(filename,"-"))
				*error = RDEUNSUPP;
			else {
				FILE* tmp = fopen(filename,"r");
				if(tmp) {
					*error = RDEUNSUPP;
					fclose(tmp);
				}
				else
					*error = -errno;
			}
		}
		else
			*error = RDEINVAL;
        RelinquishMagickMemory(exception);
		goto error;
	}

	*width = MagickGetImageWidth(ctx->wand);
	*height = MagickGetImageHeight(ctx->wand);

	MagickResetIterator(ctx->wand);

	return ctx;

error:
	magickwand_reader_close(ctx);
	return NULL;
}

static bool magickwand_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	struct magickwand_context* ctx = reader_ctx;

	if(MagickNextImage(ctx->wand) == MagickFalse)
		return false;

	MagickExportImagePixels(ctx->wand,0,0,width,height,"I",FloatPixel,image);
	return true;
}

static bool magickwand_reader_seek_frame(void* reader_ctx, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, size_t width, size_t height, RDError* error) {
	struct magickwand_context* ctx = reader_ctx;
	for(uint64_t i = 0; i < offset; i++) {
		if(MagickNextImage(ctx->wand) == MagickFalse)
			return false;
		if(progress)
			progress(progress_ctx,i+1);
	}
	return true;
}

static bool magickwand_reader_supports_ext(const char* ext) {
	return true; // let MagickWand attempt to handle any format
}

struct image_reader resdet_image_reader_magickwand = {
	.open = magickwand_reader_open,
	.read_frame = magickwand_reader_read_frame,
	.seek_frame = magickwand_reader_seek_frame,
	.close = magickwand_reader_close,
	.supports_ext = magickwand_reader_supports_ext,
};
