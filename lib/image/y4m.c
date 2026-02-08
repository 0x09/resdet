/*
 * Native image reader for yuv4mpeg.
 * This file is part of libresdet.
 */

#include "image.h"

#include <ctype.h>

struct y4m_context {
	FILE* f;
	unsigned char* buf;
	size_t y_plane_size, uv_plane_size;
	unsigned int depth;
	bool seekable;
};

static void y4m_reader_close(void* reader_ctx) {
	struct y4m_context* ctx = reader_ctx;
	if(ctx) {
		free(ctx->buf);
		if(ctx->f && ctx->f != stdin)
			fclose(ctx->f);
		free(ctx);
	}
}

static void* y4m_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	struct y4m_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		goto error;
	}

	ctx->buf = NULL;

	ctx->f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!ctx->f) {
		*error = -errno;
		goto error;
	}

	char magic[10];
	if(fread(magic,1,10,ctx->f) < 10 || memcmp(magic,"YUV4MPEG2 ",10))
		goto invalid;

	char csp[10] = "420jpeg";

	int c;
	while((c = fgetc(ctx->f)) != '\n')
		switch(c) {
			case 'W':
				if(fscanf(ctx->f,"%zu",width) != 1)
					goto invalid;
				break;
			case 'H':
				if(fscanf(ctx->f,"%zu",height) != 1)
					goto invalid;
				break;
			case 'C':
				if(fscanf(ctx->f,"%9s",csp) != 1)
					goto invalid;
				break;
			case ' ': break;
			case EOF: goto invalid;
			default:
				if(fscanf(ctx->f,"%*s") < 0)
					goto invalid;
				break;
		}


	if(!(*width && *height))
		goto invalid;
	if(resdet_dims_exceed_limit(*width,*height,4,unsigned char)) {
		*error = RDETOOBIG;
		goto error;
	}

	ctx->depth = 8;
	ctx->y_plane_size = *width * *height;

	if(!strncmp(csp,"mono",4)) {
		if(isdigit(csp[4]))
			ctx->depth = strtoul(csp+4,NULL,10);
		ctx->uv_plane_size = 0;
	}
	else {
		if(!strncmp(csp,"420",3) || !strncmp(csp,"411",3))
			ctx->uv_plane_size = ((*width+1)/2) * ((*height+1)/2) * 2;
		else if(!strncmp(csp,"422",3))
			ctx->uv_plane_size = (*width+1)/2 * *height * 2;
		else if(!strncmp(csp,"444",3))
			ctx->uv_plane_size = *width * *height * 2;
		else if(!strcmp(csp,"444alpha"))
			ctx->uv_plane_size = *width * *height * 3;
		else
			goto invalid;

		if(csp[3] == 'p' && isdigit(csp[4]))
			ctx->depth = strtoul(csp+4,NULL,10);
	}

	if(ctx->depth < 8 || ctx->depth > 16)
		goto invalid;

	size_t bytewidth = (ctx->depth+7) / 8;
	if(SIZE_MAX / bytewidth < ctx->y_plane_size + ctx->uv_plane_size)  {
		*error = RDETOOBIG;
		goto error;
	}

	ctx->y_plane_size *= bytewidth;
	ctx->uv_plane_size *= bytewidth;

	ctx->seekable = ctx->y_plane_size+ctx->uv_plane_size <= LONG_MAX && !fseek(ctx->f,0,SEEK_CUR);
	size_t bufsize;
	if(ctx->seekable)
		bufsize = ctx->y_plane_size;
	else
		bufsize = ctx->uv_plane_size < ctx->y_plane_size ? ctx->y_plane_size : ctx->uv_plane_size;

	if(!(ctx->buf = malloc(bufsize))) {
		*error = RDENOMEM;
		goto error;
	}

	return ctx;

error:
	y4m_reader_close(ctx);
	return NULL;

invalid:
	*error = RDEINVAL;
	goto error;
}

static bool read_frame_header(struct y4m_context* ctx, size_t width, size_t height, RDError* error) {
	char frame[5];
	size_t bytesread = fread(frame,1,5,ctx->f);
	if(bytesread < 5) {
		if(bytesread == 0 && feof(ctx->f))
			return false;
		goto invalid;
	}

	if(memcmp(frame,"FRAME",5))
		goto invalid;

	int c;
	while((c = fgetc(ctx->f)) != '\n')
		if(c == EOF)
			goto invalid;

	return true;

invalid:
	*error = RDEINVAL;
	return false;
}

static bool y4m_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	struct y4m_context* ctx = reader_ctx;
	if(!read_frame_header(ctx,width,height,error))
		return false;

	if(fread(ctx->buf,1,ctx->y_plane_size,ctx->f) != ctx->y_plane_size) {
		*error = RDEINVAL;
		return false;
	}

	float scale = (1u << ctx->depth)-1;
	for(size_t i = 0; i < width*height; i++) {
		uint16_t val;
		if(ctx->depth > 8)
			memcpy(&val,ctx->buf+i*2,sizeof(val));
		else
			val = ctx->buf[i];

		image[i] = val/scale;
	}

	// skip over u/v planes
	if((*error = resdet_fskip(ctx->f,ctx->uv_plane_size,ctx->seekable ? NULL : ctx->buf)))
		return false;

	return true;
}

static bool y4m_reader_seek_frame(void* reader_ctx, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, size_t width, size_t height, RDError* error) {
	struct y4m_context* ctx = reader_ctx;

	for(uint64_t i = 0; i < offset; i++) {
		if(!read_frame_header(ctx,width,height,error))
			return false;

		if((*error = resdet_fskip(ctx->f,ctx->y_plane_size,ctx->seekable ? NULL : ctx->buf)) ||
		   (*error = resdet_fskip(ctx->f,ctx->uv_plane_size,ctx->seekable ? NULL : ctx->buf)))
			return false;

		if(progress)
			progress(progress_ctx,i+1);
	}
	return true;
}

static bool y4m_reader_supports_ext(const char* ext) {
	return resdet_strieq(ext,"y4m");
}

struct image_reader resdet_image_reader_y4m = {
	.open = y4m_reader_open,
	.read_frame = y4m_reader_read_frame,
	.seek_frame = y4m_reader_seek_frame,
	.close = y4m_reader_close,
	.supports_ext = y4m_reader_supports_ext,
};
