/*
 * Image reader for PNG files using libpng.
 * This file is part of libresdet.
 */

#include "image.h"

#include <png.h>

#if PNG_LIBPNG_VER < 10500
#define png_longjmp(p,n) longjmp(p->jmpbuf,n)
#endif
static void pngerr_error_fn(png_structp png_ptr, png_const_charp error_msg) { png_longjmp(png_ptr,1); }
static void pngerr_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {}

struct libpng_context {
	FILE* f;
	bool eof;
	png_structp png_ptr;
	png_infop info_ptr;
};

static void libpng_reader_close(void* reader_ctx) {
	struct libpng_context* ctx = (struct libpng_context*)reader_ctx;
	if(!ctx)
		return;

	if(ctx->png_ptr)
		png_destroy_read_struct(&ctx->png_ptr,&ctx->info_ptr,NULL);
	if(ctx->f != stdin)
		fclose(ctx->f);
	free(ctx);
}

static void* libpng_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	*error = RDEOK;
	struct libpng_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		return NULL;
	}
	ctx->eof = false;

	ctx->f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!ctx->f) {
		*error = -errno;
		free(ctx);
		return NULL;
	}

	ctx->png_ptr = NULL;
	ctx->info_ptr = NULL;

	unsigned char header[8];
	if(fread(header,1,8,ctx->f) != 8) {
		*error = RDEINVAL;
		goto error;
	}
	if(png_sig_cmp(header,0,8)) {
		*error = RDEINVAL;
		goto error;
	}
	if(!(ctx->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, pngerr_error_fn, pngerr_warning_fn))) {
		*error = RDENOMEM;
		goto error;
	}

	if(!(ctx->info_ptr = png_create_info_struct(ctx->png_ptr))) {
		*error = RDENOMEM;
		goto error;
	}
	if(setjmp(png_jmpbuf(ctx->png_ptr))) {
		*error = RDEINVAL;
		goto error;
	}

	png_init_io(ctx->png_ptr,ctx->f);
	png_set_sig_bytes(ctx->png_ptr,8);
	png_read_info(ctx->png_ptr,ctx->info_ptr);
	*width = png_get_image_width(ctx->png_ptr,ctx->info_ptr);
	*height = png_get_image_height(ctx->png_ptr,ctx->info_ptr);

	int bit_depth = png_get_bit_depth(ctx->png_ptr,ctx->info_ptr), color = png_get_color_type(ctx->png_ptr,ctx->info_ptr);

	if(png_get_valid(ctx->png_ptr,ctx->info_ptr,PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(ctx->png_ptr);
		color |= PNG_COLOR_MASK_ALPHA;
	}
	if(color & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(ctx->png_ptr);
	if(color == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(ctx->png_ptr);
	if(bit_depth == 16)
		png_set_strip_16(ctx->png_ptr);
	else if(bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(ctx->png_ptr);

	png_read_update_info(ctx->png_ptr,ctx->info_ptr);

	return ctx;

error:
	libpng_reader_close(ctx);
	return NULL;
}

static bool libpng_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct libpng_context* ctx = (struct libpng_context*)reader_ctx;

	if(ctx->eof)
		return false;

	unsigned char* imagec = NULL;

	if(setjmp(png_jmpbuf(ctx->png_ptr))) {
		*error = RDEINVAL;
		goto end;
	}

	int color = png_get_color_type(ctx->png_ptr,ctx->info_ptr);
	int channels = color & PNG_COLOR_MASK_COLOR ? 3 : 1;
	if(!(imagec = malloc(width * height * channels))) {
		*error = RDENOMEM;
		goto end;
	}

	int passes = png_set_interlace_handling(ctx->png_ptr);

	for(int i = 0; i < passes; i++) {
		unsigned char* it = imagec;
		for(size_t y = 0; y < height; y++, it += width * channels)
			png_read_row(ctx->png_ptr,it,NULL);
	}

	png_read_end(ctx->png_ptr, NULL);

	for(size_t i = 0; i < width * height; i++) {
		image[i] = 0;
		for(int c = 0; c < channels; c++)
			image[i] += imagec[i*channels+c]/255.f;
		image[i] /= channels;
	}

end:
	ctx->eof = true;
	free(imagec);
	return *error == RDEOK;
}

static bool libpng_reader_seek_frame(void* reader_ctx, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, size_t width, size_t height, RDError* error) {
	struct libpng_context* ctx = (struct libpng_context*)reader_ctx;
	bool ret = !(ctx->eof || offset > 1);
	if(offset)
		ctx->eof = true;
	return ret;
}

static bool libpng_reader_supports_ext(const char* ext) {
	return resdet_strieq(ext,"png");
}

struct image_reader resdet_image_reader_libpng = {
	.open = libpng_reader_open,
	.read_frame = libpng_reader_read_frame,
	.seek_frame = libpng_reader_seek_frame,
	.close = libpng_reader_close,
	.supports_ext = libpng_reader_supports_ext,
};

