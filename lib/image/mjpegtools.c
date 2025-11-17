/*
 * Image reader for yuv4mpeg files using libmjpegutils from the mjpegtools project.
 * This file is part of libresdet.
 */

#include "image.h"

#include <yuv4mpeg.h>
#include <fcntl.h>

struct mjpegtools_context {
	int fd;
	y4m_stream_info_t st;
	y4m_frame_info_t frame;
	int frame_length;
	unsigned char* image,* discard;
};

static void mjpegtools_reader_close(void* reader_ctx) {
	struct mjpegtools_context* ctx = (struct mjpegtools_context*)reader_ctx;
	if(!ctx)
		return;

	free(ctx->image);
	free(ctx->discard);
	y4m_fini_frame_info(&ctx->frame);
	y4m_fini_stream_info(&ctx->st);
	if(ctx->fd > 0)
		close(ctx->fd);
	free(ctx);
}

static void* mjpegtools_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	*error = RDEOK;
	struct mjpegtools_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		return NULL;
	}

	ctx->fd = strcmp(filename,"-") ? open(filename,O_RDONLY) : 0;
	if(ctx->fd < 0) {
		*error = -errno;
		free(ctx);
		return NULL;
	}

	ctx->image = ctx->discard = NULL;

	y4m_init_stream_info(&ctx->st);
	y4m_init_frame_info(&ctx->frame);
	if(y4m_read_stream_header(ctx->fd,&ctx->st) != Y4M_OK) {
		*error = RDEINVAL;
		goto error;
	}

	*width = y4m_si_get_width(&ctx->st);
	*height = y4m_si_get_height(&ctx->st);

	ctx->frame_length = y4m_si_get_framelength(&ctx->st);
	if(ctx->frame_length < 0 || (size_t)ctx->frame_length < *width * *height) {
		*error = RDEINVAL;
		goto error;
	}
	ctx->frame_length -= *width * *height; // u/v plane skip

	if(!(ctx->image = malloc(*width * *height))) {
		*error = RDENOMEM;
		goto error;
	}
	if(!(ctx->discard = malloc(ctx->frame_length))) {
		*error = RDENOMEM;
		goto error;
	}

	return ctx;

error:
	mjpegtools_reader_close(ctx);
	return NULL;
}

static bool mjpegtools_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct mjpegtools_context* ctx = (struct mjpegtools_context*)reader_ctx;

	int ret = y4m_read_frame_header(ctx->fd,&ctx->st,&ctx->frame);
	if(ret != Y4M_OK) {
		if(ret != Y4M_ERR_EOF)
			*error = RDEINVAL;
		return false;
	}

	if(y4m_read(ctx->fd,ctx->image,width * height) < 0 ||
	   y4m_read(ctx->fd,ctx->discard,ctx->frame_length) != 0) {
	   *error = RDEINVAL;
		return false;
	}

	for(size_t i = 0; i < width * height; i++)
		image[i] = ctx->image[i]/255.f;

	return true;
}


struct image_reader resdet_image_reader_mjpegtools = {
	.open = mjpegtools_reader_open,
	.read_frame = mjpegtools_reader_read_frame,
	.close = mjpegtools_reader_close
};
