/*
 * resdet-native image reader for PGM and PFM files.
 * This file is part of libresdet.
 */

#include "image.h"

#include <inttypes.h>

struct pgm_context {
	FILE* f;
	bool eof;
	uint16_t depth;
};

static void pgm_reader_close(void* reader_ctx) {
	struct pgm_context* ctx = (struct pgm_context*)reader_ctx;
	if(!ctx)
		return;

	if(ctx->f && ctx->f != stdin)
		fclose(ctx->f);
	free(ctx);
}

static void* pgm_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	*error = RDEOK;
	struct pgm_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		return NULL;
	}

	ctx->eof = false;

	ctx->f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!ctx->f) {
		*error = -errno;
		goto error;
	}

	if(
		fscanf(ctx->f,"P5 %zu %zu %" SCNu16,width,height,&ctx->depth) != 3 ||
		fgetc(ctx->f) == EOF ||
		ctx->depth > 255
	) {
		*error = RDEINVAL;
		goto error;
	}

	return ctx;

error:
	pgm_reader_close(ctx);
	return NULL;
}

static bool pgm_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct pgm_context* ctx = (struct pgm_context*)reader_ctx;

	if(ctx->eof)
		return false;

	int c;
	for(size_t i = 0; i < width * height; i++) {
		if((c = fgetc(ctx->f)) == EOF) {
			*error = RDEINVAL;
			break;
		}
		else image[i] = c/(float)ctx->depth;
	}

	ctx->eof = true;
	return *error == RDEOK;
}

struct image_reader resdet_image_reader_pgm = {
	.open = pgm_reader_open,
	.read_frame = pgm_reader_read_frame,
	.close = pgm_reader_close
};
