/*
 * Image reader using FFmpeg's libraries.
 * This file is part of libresdet.
 */

#include "image.h"

#include <libavutil/avstring.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

struct ffmpeg_context {
	AVFormatContext* fmt;
	AVCodecContext* codec;
	int stream_index;
	struct SwsContext* sws;
	AVFrame* swsframe,* frame;
	AVPacket* packet;
};

static RDError rderror_from_averror(int averr) {
	if(!averr)
		return RDEOK;

	// try to find out if this is an errno code
	if(!strcmp(av_err2str(averr),strerror(AVUNERROR(averr))))
		return -AVUNERROR(averr);

	return RDEINVAL;
}

static void ffmpeg_reader_close(void* reader_ctx) {
	struct ffmpeg_context* ctx = (struct ffmpeg_context*)reader_ctx;
	if(!ctx)
		return;

	avcodec_free_context(&ctx->codec);
	av_frame_free(&ctx->frame);
	av_frame_free(&ctx->swsframe);
	av_packet_free(&ctx->packet);
	sws_freeContext(ctx->sws);
	avformat_close_input(&ctx->fmt);
	free(ctx);
}

#define little_endian() (union { int i; char c; }){1}.c

static void* ffmpeg_reader_open(const char* filename, size_t* width, size_t* height, RDError* error) {
	*error = RDEOK;
	struct ffmpeg_context* ctx = malloc(sizeof(*ctx));
	if(!ctx) {
		*error = RDENOMEM;
		return NULL;
	}

	ctx->fmt = NULL;
	ctx->codec = NULL;
	ctx->sws = NULL;
	ctx->swsframe = ctx->frame = NULL;
	ctx->packet = NULL;

	if(!strcmp(filename,"-"))
		filename = "pipe:";

	int averr;
	if((averr = avformat_open_input(&ctx->fmt,filename,NULL,NULL)) ||
	   (averr = avformat_find_stream_info(ctx->fmt,NULL)) < 0)
		goto error;

	const AVCodec* dec;
	if((averr = (ctx->stream_index = av_find_best_stream(ctx->fmt,AVMEDIA_TYPE_VIDEO,-1,-1,&dec,0))) < 0)
		goto error;

	if(!(ctx->codec = avcodec_alloc_context3(dec))) {
		*error = RDENOMEM;
		goto error;
	}
	if((averr = avcodec_parameters_to_context(ctx->codec, ctx->fmt->streams[ctx->stream_index]->codecpar) < 0))
		goto error;

	if((averr = avcodec_open2(ctx->codec,dec,NULL)))
		goto error;

	if(!(ctx->sws = sws_alloc_context()) ||
	   !(ctx->frame = av_frame_alloc()) ||
	   !(ctx->swsframe = av_frame_alloc()) ||
	   !(ctx->packet = av_packet_alloc())) {
		*error = RDENOMEM;
		goto error;
	}

	av_opt_set_int(ctx->sws, "srcw", ctx->codec->width, 0);
	av_opt_set_int(ctx->sws, "srch", ctx->codec->height, 0);
	av_opt_set_int(ctx->sws, "src_format", ctx->codec->pix_fmt, 0);
	av_opt_set_int(ctx->sws, "dstw", ctx->codec->width, 0);
	av_opt_set_int(ctx->sws, "dsth", ctx->codec->height, 0);
	av_opt_set_int(ctx->sws, "dst_format", little_endian() ? AV_PIX_FMT_GRAYF32LE : AV_PIX_FMT_GRAYF32BE, 0);
	if((averr = sws_init_context(ctx->sws,NULL,NULL)) < 0)
		goto error;

	ctx->frame->width = ctx->codec->width;
	ctx->frame->height = ctx->codec->height;
	ctx->frame->format = little_endian() ? AV_PIX_FMT_GRAYF32LE : AV_PIX_FMT_GRAYF32BE;

	*width = ctx->codec->width;
	*height = ctx->codec->height;

	return ctx;

error:
	if(averr)
		*error = rderror_from_averror(averr);

	ffmpeg_reader_close(ctx);
	return NULL;
}

static int read_frame(struct ffmpeg_context* ctx) {
	int averr;
	while((averr = avcodec_receive_frame(ctx->codec, ctx->swsframe)) == AVERROR(EAGAIN)) {
		while(!(averr = av_read_frame(ctx->fmt,ctx->packet)) && ctx->packet->stream_index != ctx->stream_index)
			av_packet_unref(ctx->packet);
		if(averr)
			averr = avcodec_send_packet(ctx->codec, NULL);
		else {
			averr = avcodec_send_packet(ctx->codec, ctx->packet);
			av_packet_unref(ctx->packet);
		}
		if(averr)
			break;
	}
	return averr;
}

static bool ffmpeg_reader_read_frame(void* reader_ctx, float* image, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct ffmpeg_context* ctx = (struct ffmpeg_context*)reader_ctx;

	int averr = read_frame(ctx);
	if(averr || (averr = sws_scale_frame(ctx->sws,ctx->frame,ctx->swsframe)) < 0)
		goto averror;

	for(size_t y = 0; y < height; y++)
		memcpy(image+y*width,ctx->frame->data[0]+y*ctx->frame->linesize[0],width*sizeof(float));

	return true;

averror:
	if(averr != AVERROR_EOF)
		*error = rderror_from_averror(averr);
	return false;
}

static bool ffmpeg_reader_seek_frame(void* reader_ctx, uint64_t offset, void(*progress)(void*,uint64_t), void* progress_ctx, size_t width, size_t height, RDError* error) {
	*error = RDEOK;
	struct ffmpeg_context* ctx = (struct ffmpeg_context*)reader_ctx;

	int averr = 0;
	for(uint64_t i = 0; i < offset; i++) {
		if((averr = read_frame(ctx)))
			break;
		if(progress)
			progress(progress_ctx,i+1);
	}

	if(averr && averr != AVERROR_EOF)
		*error = rderror_from_averror(averr);
	return !averr;
}

static bool ffmpeg_reader_supports_ext(const char* ext) {
#if HAVE_MAGICKWAND
	void* state = NULL;
	const AVInputFormat* fmt;
	while((fmt = av_demuxer_iterate(&state)))
		if(av_match_name(ext,fmt->extensions))
			return true;

	return false;
#else
	return true; // let ffmpeg attempt to handle any format
#endif
}

struct image_reader resdet_image_reader_ffmpeg = {
	.open = ffmpeg_reader_open,
	.read_frame = ffmpeg_reader_read_frame,
	.seek_frame = ffmpeg_reader_seek_frame,
	.close = ffmpeg_reader_close,
	.supports_ext = ffmpeg_reader_supports_ext,
};
