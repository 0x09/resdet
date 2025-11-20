/*
 * resdet-native image reader for PFM files.
 * This file is part of libresdet.
 */

#include "image.h"

struct pfm_context {
	FILE* f;
	float endianness_scale;
	char format;
	bool header_consumed;
};

#define little_endian() (union { int i; char c; }){1}.c

static bool read_pfm_plane_gray(FILE* f, float* image, size_t width, size_t height, float endianness_scale) {
	if((endianness_scale < 0) == little_endian()) {
		for(size_t j = 0; j < height; j++)
			if(fread(image+(height-j-1)*width,sizeof(float),width,f) < width)
				return false;
	}
	else {
		for(size_t j = 0; j < height; j++)
			for(size_t i = 0; i < width; i++) {
				char buf[4];
				if(fread(buf,4,1,f) < 1)
					return false;
				image[(height-j-1)*width+i] = (union { char c[4]; float f; }){{ buf[3], buf[2], buf[1], buf[0] }}.f;
			}
	}
	return true;
}

static bool read_pfm_plane_rgb(FILE* f, float* image, size_t width, size_t height, float endianness_scale) {
    // PF is packed RGB
	if((endianness_scale < 0) == little_endian()) {
		for(size_t j = 0; j < height; j++)
			for(size_t i = 0; i < width; i++) {
				float pixel[3];
				if(fread(pixel,sizeof(float),3,f) < 3)
					return false;
				image[(height-j-1)*width+i] = (pixel[0] + pixel[1] + pixel[2])/3;
			}
	}
	else {
		for(size_t j = 0; j < height; j++)
			for(size_t i = 0; i < width; i++) {
				char buf[3][4];
				if(fread(buf,4,3,f) < 3)
					return false;
				float pixel[3];
				for(int c = 0; c < 3; c++)
					pixel[c] = (union { char c[4]; float f; }){{ buf[c][3], buf[c][2], buf[c][1], buf[c][0] }}.f;
				image[(height-j-1)*width+i] = (pixel[0] + pixel[1] + pixel[2])/3;
			}
	}
	return true;
}

static bool read_pfm_plane(FILE* f, float* image, size_t width, size_t height, float endianness_scale, char format) {
	return format == 'f' ? read_pfm_plane_gray(f,image,width,height,endianness_scale) : read_pfm_plane_rgb(f,image,width,height,endianness_scale);
}

static void pfm_reader_close(void* reader_ctx) {
	struct pfm_context* ctx = (struct pfm_context*)reader_ctx;
	if(!ctx)
		return;

	if(ctx->f && ctx->f != stdin)
		fclose(ctx->f);
	free(ctx);
}

static void* pfm_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	*error = RDEOK;
	struct pfm_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		return NULL;
	}

	ctx->f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!ctx->f) {
		*error = -errno;
		goto error;
	}

	if(
	   fscanf(ctx->f,"P%c %zu %zu %f\n",&ctx->format,width,height,&ctx->endianness_scale) != 4 ||
	   !(ctx->format == 'f' || ctx->format == 'F')
	) {
		*error = RDEINVAL;
		goto error;
	}

	ctx->header_consumed = true;

	return ctx;

error:
	pfm_reader_close(ctx);
	return NULL;
}

static bool pfm_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct pfm_context* ctx = (struct pfm_context*)reader_ctx;

	if(!ctx->header_consumed) {
		size_t next_width, next_height;
		float next_endianness_scale;
		char next_format;

		int next_char = fgetc(ctx->f);
		if(next_char == EOF && feof(ctx->f))
			return false;

		if(
		    next_char != 'P' ||
		    fscanf(ctx->f,"%c %zu %zu %f\n",&next_format,&next_width,&next_height,&next_endianness_scale) != 4 ||
		    !(next_format == 'f' || next_format == 'F') ||
		    next_width != width ||
			next_height != height
		) {
			*error = RDEINVAL;
			goto end;
		}

		ctx->endianness_scale = next_endianness_scale;
		ctx->format = next_format;
	}

	if(!read_pfm_plane(ctx->f,image,width,height,ctx->endianness_scale,ctx->format)) {
		*error = RDEINVAL;
		goto end;
	}

	ctx->header_consumed = false;

end:
	return *error == RDEOK;
}

static bool pfm_reader_supports_ext(const char* ext) {
	return resdet_strieq(ext,"pfm");
}

struct image_reader resdet_image_reader_pfm = {
	.open = pfm_reader_open,
	.read_frame = pfm_reader_read_frame,
	.close = pfm_reader_close,
	.supports_ext = pfm_reader_supports_ext,
};
