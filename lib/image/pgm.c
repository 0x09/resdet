/*
 * resdet-native image reader for PGM and PFM files.
 * This file is part of libresdet.
 */

#include "image.h"

#include <inttypes.h>
#include <ctype.h>

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

static inline bool skip_comments(FILE* f) {
	int c;
	while((c = fgetc(f)) == '#') {
		do {
			if((c = fgetc(f)) == EOF)
				return false;
		} while(c != '\n');
	}
	return c != EOF && ungetc(c,f) == c;
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
	   fscanf(ctx->f,"P5 ") < 0 ||
	   !skip_comments(ctx->f) ||
	   fscanf(ctx->f,"%zu %zu %" SCNu16,width,height,&ctx->depth) != 3 ||
	   !isspace(fgetc(ctx->f)) ||
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

static bool pgm_reader_seek_frame(void* reader_ctx, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct pgm_context* ctx = (struct pgm_context*)reader_ctx;
	bool ret = !(ctx->eof || offset > 1);
	if(offset)
		ctx->eof = true;
	return ret;
}

static bool pgm_reader_supports_ext(const char* ext) {
	return resdet_strieq(ext,"pgm");
}

struct image_reader resdet_image_reader_pgm = {
	.open = pgm_reader_open,
	.read_frame = pgm_reader_read_frame,
	.seek_frame = pgm_reader_seek_frame,
	.close = pgm_reader_close,
	.supports_ext = pgm_reader_supports_ext,
};
