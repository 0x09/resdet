/*
 * libresdet - Detect source resolution of upscaled images.
 * Copyright (C) 2012-2017 0x09.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>

#include "resdet_internal.h"

#ifdef HAVE_LIBPNG
#include <png.h>

#if PNG_LIBPNG_VER < 10500
#define png_longjmp(p,n) longjmp(p->jmpbuf,n)
#endif
static void pngerr_error_fn(png_structp png_ptr, png_const_charp error_msg) { png_longjmp(png_ptr,1); }
static void pngerr_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {}

static unsigned char* read_png(FILE* f, size_t* width, size_t* height, size_t* nimages) {
	*nimages = 1;
	unsigned char* image = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	unsigned char header[8];
	if(fread(header,1,8,f) != 8) goto end;
	if(png_sig_cmp(header,0,8)) goto end;
	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, pngerr_error_fn, pngerr_warning_fn)))
		goto end;

	if(!(info_ptr = png_create_info_struct(png_ptr)))
		goto end;
	if(setjmp(png_jmpbuf(png_ptr)))
		goto end;

	png_init_io(png_ptr,f);
	png_set_sig_bytes(png_ptr,8);
	png_read_info(png_ptr,info_ptr);
	*width = png_get_image_width(png_ptr,info_ptr);
	*height = png_get_image_height(png_ptr,info_ptr);

	if(!(*width && *height) || (*width > PIXEL_MAX / *height) || !(image = malloc(*width * *height)))
		goto end;

	int bit_depth = png_get_bit_depth(png_ptr,info_ptr), color = png_get_color_type(png_ptr,info_ptr);

	if(png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png_ptr);
		color |= PNG_COLOR_MASK_ALPHA;
	}
	if(color & PNG_COLOR_MASK_ALPHA) png_set_strip_alpha(png_ptr);
	if(color == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if(color & PNG_COLOR_MASK_COLOR) png_set_rgb_to_gray(png_ptr,1,-1,-1);
	if(bit_depth == 16) png_set_strip_16(png_ptr);
	else if(bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
	png_read_update_info(png_ptr,info_ptr);
	int passes = png_set_interlace_handling(png_ptr);

	for(int i = 0; i < passes; i++) {
		unsigned char* it = image;
		for(rdint_index y = 0; y < *height; y++, it += *width)
			png_read_row(png_ptr,it,NULL);
	}

	png_read_end(png_ptr, NULL);
end:
	if(png_ptr)
		png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
	return image;
}
#else
//png.h complains if setjmp is already included
#include <setjmp.h>
#endif

#ifdef HAVE_LIBJPEG
#include <jpeglib.h>

static void jerr_emit_message(j_common_ptr cinfo, int msg_level) {}
static void jerr_error_exit(j_common_ptr cinfo) { longjmp(cinfo->client_data,1); }
static void jerr_reset_error_mgr(j_common_ptr cinfo) {}
static unsigned char* read_jpeg(FILE* f, size_t* width, size_t* height, size_t* nimages) {
	*nimages = 1;
	unsigned char* image = NULL;
	struct jpeg_decompress_struct cinfo;
	cinfo.err = &(struct jpeg_error_mgr){
		.error_exit      = jerr_error_exit,
		.emit_message    = jerr_emit_message,
		.reset_error_mgr = jerr_reset_error_mgr
	};
	if(setjmp(cinfo.client_data = (jmp_buf){0})) {
		free(image);
		image = NULL;
		goto end;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo,f);
	jpeg_read_header(&cinfo,TRUE);
	cinfo.out_color_space = JCS_GRAYSCALE;
	jpeg_start_decompress(&cinfo);
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	if(!(*width && *height) || (*width > PIXEL_MAX / *height) || !(image = malloc(*width * *height)))
		goto finish;

	unsigned char* it = image;
	while(cinfo.output_scanline < cinfo.output_height) {
		unsigned char* rows[cinfo.rec_outbuf_height];
		for(int i = 0; i < cinfo.rec_outbuf_height; i++)
			rows[i] = it+i*cinfo.output_width;
		it += jpeg_read_scanlines(&cinfo,rows,cinfo.rec_outbuf_height) * cinfo.output_width;
	}
finish:
	jpeg_finish_decompress(&cinfo);
end:
	jpeg_destroy_decompress(&cinfo);
	return image;
}
#endif

#ifdef HAVE_MJPEGTOOLS
#include <mjpegtools/yuv4mpeg.h>

static unsigned char* read_y4m(FILE* f, size_t* width, size_t* height, size_t* nimages) {
	unsigned char* image = NULL,* discard = NULL;
	int fd = fileno(f);
	y4m_stream_info_t st;
	y4m_frame_info_t frame;
	y4m_init_stream_info(&st);
	y4m_init_frame_info(&frame);
	if(y4m_read_stream_header(fd,&st) != Y4M_OK)
		goto end;
	*width = y4m_si_get_width(&st);
	*height = y4m_si_get_height(&st);
	int frame_length = y4m_si_get_framelength(&st);
	if(!(*width && *height) || (*width > PIXEL_MAX / *height) || frame_length < 0 || (size_t)frame_length < *width * *height)
		goto end;
	frame_length -= *width * *height; // u/v plane skip
	discard = malloc(frame_length);
	while(y4m_read_frame_header(fd,&st,&frame) == Y4M_OK) {
		(*nimages)++;
		unsigned char* tmp;
		if((*width * *height > PIXEL_MAX / *nimages) ||
		   !(tmp = realloc(image,*width * *height * *nimages))) {
			free(image); image = NULL; break;
		}

		image = tmp;
		if(y4m_read(fd,image + *width * *height * (*nimages - 1),*width * *height) < 0 ||
		   read(fd,discard,frame_length) != frame_length) {
   			free(image); image = NULL; break;
		}
	}

end:
	free(discard);
	y4m_fini_frame_info(&frame);
	y4m_fini_stream_info(&st);
	return image;
}
#endif

#ifdef HAVE_MAGICKWAND
#if HAVE_MAGICKWAND > 6
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif

static unsigned char* read_magick(FILE* f, size_t* width, size_t* height, size_t* nimages) {
	unsigned char* image = NULL;
	MagickWand* wand = NewMagickWand();
	if(!MagickReadImageFile(wand,f))
		goto end;
	*width = MagickGetImageWidth(wand);
	*height = MagickGetImageHeight(wand);
	*nimages = MagickGetNumberImages(wand);
	if(!(*width && *height && *nimages) || (*width > PIXEL_MAX / *height) || (*width * *height > PIXEL_MAX / *nimages) || !(image = malloc(*width * *height * *nimages)))
		goto end;
	MagickResetIterator(wand);
	for(size_t i = 0; i < *nimages; i++) {
		MagickNextImage(wand);
		MagickExportImagePixels(wand,0,0,*width,*height,"I",CharPixel,image + i * *width* *height);
	}
end:
	DestroyMagickWand(wand);
	return image;
}
#endif

static const char* mimetype_from_ext(const char* filename) {
	char* ext = strrchr(filename,'.');
	if(!ext)
		return "";
	if(!strcasecmp(ext+1,"jpg") || !strcasecmp(ext+1,"jpeg"))
		return "image/jpeg";
	if(!strcasecmp(ext+1,"png"))
		return "image/png";
	if(!strcasecmp(ext+1,"y4m"))
		return "video/yuv4mpeg";
	return "";
}

RDError resdet_read_image(const char* filename, const char* mimetype, unsigned char** image, size_t* nimages, size_t* width, size_t* height) {
	*width = *height = *nimages = 0;
	*image = NULL;
	FILE* f = stdin;
	if(strcmp(filename,"-"))
		f = fopen(filename,"r");
	if(!f)
		return RDEINTERNAL;

	const char* c = mimetype;
	if(!c)
		c = mimetype_from_ext(filename);

	unsigned char* (*read_image)(FILE*,size_t*,size_t*,size_t*) = NULL;
	if(false)
		;
#ifdef HAVE_LIBJPEG
	else if(!strcmp(c,"image/jpeg"))
		read_image = read_jpeg;
#endif
#ifdef HAVE_LIBPNG
	else if(!strcmp(c,"image/png"))
		read_image = read_png;
#endif
#ifdef HAVE_MJPEGTOOLS
	else if(!strcmp(c,"video/yuv4mpeg"))
		read_image = read_y4m;
#endif
#ifdef HAVE_MAGICKWAND
	else read_image = read_magick;
#endif

	if(read_image)
		*image = read_image(f,width,height,nimages);
	if(f != stdin)
		fclose(f);

	return *image ? RDEOK : RDEINVAL;
}
