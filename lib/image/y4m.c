/*
 * Native image reader for yuv4mpeg.
 * This file is part of libresdet.
 */

#include "image.h"

#include <ctype.h>

struct y4m_context {
	FILE* f;
	unsigned char* buf;
	size_t frame_length;
	unsigned int depth;
};

static void y4m_reader_close(void* reader_ctx) {
	struct y4m_context* ctx = (struct y4m_context*)reader_ctx;
	if(ctx) {
		if(ctx->f && ctx->f != stdin)
			fclose(ctx->f);
		free(ctx);
	}
}

static void* y4m_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	*error = RDEOK;
	struct y4m_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		goto error;
	}

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
	ctx->frame_length = *width * *height;

	if(!strncmp(csp,"mono",4)) {
		if(isdigit(csp[4]))
			ctx->depth = strtoul(csp+4,NULL,10);
	}
	else {
		if(!strncmp(csp,"420",3) || !strncmp(csp,"411",3))
			ctx->frame_length += ((*width+1)/2) * ((*height+1)/2) * 2;
		else if(!strncmp(csp,"422",3))
			ctx->frame_length += (*width+1)/2 * *height * 2;
		else if(!strncmp(csp,"444",3))
			ctx->frame_length += *width * *height * 2;
		else if(!strcmp(csp,"444alpha"))
			ctx->frame_length += *width * *height * 3;
		else
			goto invalid;

		if(csp[3] == 'p' && isdigit(csp[4]))
			ctx->depth = strtoul(csp+4,NULL,10);
	}

	if(ctx->depth < 8 || ctx->depth > 16)
		goto invalid;

	size_t bytewidth = (ctx->depth+7) / 8;
	if(SIZE_MAX / bytewidth < ctx->frame_length)  {
		*error = RDETOOBIG;
		goto error;
	}

	ctx->frame_length *= bytewidth;

	if(!(ctx->buf = malloc(ctx->frame_length))) {
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

static bool y4m_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct y4m_context* ctx = (struct y4m_context*)reader_ctx;

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

	if(fread(ctx->buf,1,ctx->frame_length,ctx->f) != ctx->frame_length)
		goto invalid;

	float scale = (1u << ctx->depth)-1;
	for(size_t i = 0; i < width*height; i++) {
		uint16_t val;
		if(ctx->depth > 8)
			memcpy(&val,ctx->buf+i*2,sizeof(val));
		else
			val = ctx->buf[i];

		image[i] = val/scale;
	}

	return true;

invalid:
	*error = RDEINVAL;
	return false;
}

struct image_reader resdet_image_reader_y4m = {
	.open = y4m_reader_open,
	.read_frame = y4m_reader_read_frame,
	.close = y4m_reader_close
};
