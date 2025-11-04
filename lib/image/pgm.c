#include "image.h"

#include <inttypes.h>

static float* read_pgm(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	*nimages = 1;
	float* image = NULL;
	FILE* f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!f)
		return NULL;
	uint16_t depth;
	if(
		fscanf(f,"P5 %zu %zu %" SCNu16,width,height,&depth) == 3 &&
		fgetc(f) != EOF &&
		depth < 256 &&
		!resdet_dims_exceed_limit(*width,*height,1,*image) &&
		(image = malloc(sizeof(*image) * *width * *height))
	) {
		int c;
		for(size_t i = 0; i < *width * *height; i++) {
			if((c = fgetc(f)) == EOF) {
				free(image);
				image = NULL;
				break;
			}
			else image[i] = c/(float)depth;
		}
	}
	if(f != stdin)
		fclose(f);
	return image;
}

#define little_endian() (union { int i; char c; }){1}.c

static bool read_pfm_plane_gray(FILE* f, float* image, size_t width, size_t height, float endianness_scale) {
	if(endianness_scale < 0 == little_endian()) {
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
	if(endianness_scale < 0 == little_endian()) {
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

static float* read_pfm(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	*nimages = 1;
	float* image = NULL;
	FILE* f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin;
	if(!f)
		return NULL;
	float endianness_scale;
	char format;
	if(!(
	   fscanf(f,"P%c %zu %zu %f\n",&format,width,height,&endianness_scale) == 4 &&
	   (format == 'f' || format == 'F') &&
	   !resdet_dims_exceed_limit(*width,*height,*nimages,*image) &&
	   (image = malloc(sizeof(*image) * *width * *height))
	))
		goto end;

	if(!read_pfm_plane(f,image,*width,*height,endianness_scale,format)) {
        free(image);
		image = NULL;
		goto end;
	}

	size_t next_width, next_height;
	float next_endianness_scale;
	char next_format;
	int next_char;
	float* next_image = NULL;
	while(
	    (next_char = fgetc(f)) == 'P' &&
	    fscanf(f,"%c %zu %zu %f\n",&next_format,&next_width,&next_height,&next_endianness_scale) == 4 &&
	    (next_format == 'f' || next_format == 'F') &&
	    next_width == *width && next_height == *height && next_endianness_scale == endianness_scale &&
	    !resdet_dims_exceed_limit(*width,*height,*nimages,*image) &&
	    (next_image = realloc(image,sizeof(*image) * *width * *height * ++*nimages))
	) {
		if(!next_image) {
            free(image);
			image = NULL;
			goto end;
		}
		image = next_image;
		if(!read_pfm_plane(f,image+*width * *height * (*nimages-1),*width,*height,endianness_scale,next_format))
			break;
	}

	if(next_char != EOF) {
		free(image);
		image = NULL;
	}

end:
	if(f != stdin)
		fclose(f);
	return image;
}

struct image_reader resdet_image_reader_pgm = {
	.read = read_pgm
};
struct image_reader resdet_image_reader_pfm = {
	.read = read_pfm
};
