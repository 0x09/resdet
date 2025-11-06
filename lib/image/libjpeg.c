#include "image.h"

#include <stdlib.h>
#include <setjmp.h>

#include <jpeglib.h>

static void jerr_emit_message(j_common_ptr cinfo, int msg_level) {}
static void jerr_error_exit(j_common_ptr cinfo) { longjmp(cinfo->client_data,1); }
static void jerr_reset_error_mgr(j_common_ptr cinfo) {}

static float* read_jpeg(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	FILE* f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!f)
		return NULL;

	*nimages = 1;
	unsigned char* image = NULL;
	float* imagef = NULL;
	struct jpeg_decompress_struct cinfo;
	cinfo.err = &(struct jpeg_error_mgr){
		.error_exit      = jerr_error_exit,
		.emit_message    = jerr_emit_message,
		.reset_error_mgr = jerr_reset_error_mgr
	};
	if(setjmp(cinfo.client_data = (jmp_buf){0}))
		goto end;

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo,f);
	jpeg_read_header(&cinfo,TRUE);
	cinfo.out_color_space = JCS_GRAYSCALE;
	jpeg_start_decompress(&cinfo);
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	if(resdet_dims_exceed_limit(*width,*height,1,*imagef) || !(image = malloc(*width * *height)))
		goto finish;

	unsigned char* it = image;
	while(cinfo.output_scanline < cinfo.output_height) {
		unsigned char* rows[cinfo.rec_outbuf_height];
		for(int i = 0; i < cinfo.rec_outbuf_height; i++)
			rows[i] = it+i*cinfo.output_width;
		it += jpeg_read_scanlines(&cinfo,rows,cinfo.rec_outbuf_height) * cinfo.output_width;
	}
	if(!(imagef = malloc(*width * *height * sizeof(*imagef))))
		goto end;
	for(size_t i = 0; i < *width * *height; i++)
		imagef[i] = image[i]/255.f;
finish:
	jpeg_finish_decompress(&cinfo);
end:
	jpeg_destroy_decompress(&cinfo);
    free(image);
	if(f != stdin)
		fclose(f);
	return imagef;
}

struct image_reader resdet_image_reader_libjpeg = {
	.read = read_jpeg
};
