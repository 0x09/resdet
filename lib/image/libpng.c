#include "image.h"

#include <png.h>

#if PNG_LIBPNG_VER < 10500
#define png_longjmp(p,n) longjmp(p->jmpbuf,n)
#endif
static void pngerr_error_fn(png_structp png_ptr, png_const_charp error_msg) { png_longjmp(png_ptr,1); }
static void pngerr_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {}

static float* read_png(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	FILE* f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!f)
		return NULL;

	*nimages = 1;
	unsigned char* image = NULL;
	float* imagef = NULL;
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

	if(resdet_dims_exceed_limit(*width,*height,1,*imagef) || !(image = malloc(*width * *height)))
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

	if(!(imagef = malloc(*width * *height * sizeof(*imagef))))
		goto end;
	for(size_t i = 0; i < *width * *height; i++)
		imagef[i] = image[i]/255.f;

end:
	if(png_ptr)
		png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
    free(image);
	if(f != stdin)
		fclose(f);
	return imagef;
}

struct image_reader resdet_image_reader_libpng = {
	.read = read_png
};
