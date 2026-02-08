/*
 * Image reader for JPEG files using libjpeg.
 * This file is part of libresdet.
 */

#include "image.h"

#include <stdlib.h>
#include <setjmp.h>

#include <jpeglib.h>

static void jerr_emit_message(j_common_ptr cinfo, int msg_level) {}
static void jerr_error_exit(j_common_ptr cinfo) { longjmp(cinfo->client_data,1); }
static void jerr_reset_error_mgr(j_common_ptr cinfo) {}

struct libjpeg_context {
	FILE* f;
	bool eof;
	struct jpeg_decompress_struct cinfo;
};

static void libjpeg_reader_close(void* reader_ctx) {
	struct libjpeg_context* ctx = reader_ctx;
	if(!ctx)
		return;

	jpeg_destroy_decompress(&ctx->cinfo);
	if(ctx->f != stdin)
		fclose(ctx->f);
	free(ctx);
}

static void* libjpeg_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	struct libjpeg_context* ctx = malloc(sizeof(*ctx));
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

	ctx->cinfo.err = &(struct jpeg_error_mgr){
		.error_exit      = jerr_error_exit,
		.emit_message    = jerr_emit_message,
		.reset_error_mgr = jerr_reset_error_mgr
	};

	if(setjmp(ctx->cinfo.client_data = (jmp_buf){0})) {
		*error = RDEINVAL;
		goto error;
	}

	jpeg_create_decompress(&ctx->cinfo);
	jpeg_stdio_src(&ctx->cinfo,ctx->f);
	jpeg_read_header(&ctx->cinfo,TRUE);
	ctx->cinfo.out_color_space = JCS_GRAYSCALE;
	jpeg_start_decompress(&ctx->cinfo);

	*width = ctx->cinfo.output_width;
	*height = ctx->cinfo.output_height;

	return ctx;

error:
	libjpeg_reader_close(ctx);
	return NULL;
}

static bool libjpeg_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	struct libjpeg_context* ctx = reader_ctx;

	if(ctx->eof)
		return false;

	unsigned char* imagec = malloc(width * height);
	if(!imagec) {
		*error = RDENOMEM;
		goto end;
	}

	if(setjmp(ctx->cinfo.client_data = (jmp_buf){0})) {
		*error = RDEINVAL;
		goto end;
	}

	unsigned char* it = imagec;
	while(ctx->cinfo.output_scanline < ctx->cinfo.output_height) {
		unsigned char* rows[ctx->cinfo.rec_outbuf_height];
		for(int i = 0; i < ctx->cinfo.rec_outbuf_height; i++)
			rows[i] = it+i*ctx->cinfo.output_width;
		it += jpeg_read_scanlines(&ctx->cinfo,rows,ctx->cinfo.rec_outbuf_height) * ctx->cinfo.output_width;
	}
	for(size_t i = 0; i < width * height; i++)
		image[i] = imagec[i]/255.f;

end:
	ctx->eof = true;
	free(imagec);
	return *error == RDEOK;
}

static bool libjpeg_reader_seek_frame(void* reader_ctx, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct libjpeg_context* ctx = reader_ctx;
	bool ret = !(ctx->eof || offset > 1);
	if(offset)
		ctx->eof = true;
	return ret;
}

static bool libjpeg_reader_supports_ext(const char* ext) {
	return resdet_strieq(ext,"jpg") || resdet_strieq(ext,"jpeg");
}

struct image_reader resdet_image_reader_libjpeg = {
	.open = libjpeg_reader_open,
	.read_frame = libjpeg_reader_read_frame,
	.seek_frame = libjpeg_reader_seek_frame,
	.close = libjpeg_reader_close,
	.supports_ext = libjpeg_reader_supports_ext,
};
