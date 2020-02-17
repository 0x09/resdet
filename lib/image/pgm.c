#include "image.h"

#include <inttypes.h>

static unsigned char* read_pgm(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	*nimages = 1;
	unsigned char* image = NULL;
	FILE* f = strcmp(filename,"-") ? fopen(filename,"r") : stdin;
	uint16_t depth;
	if(
		f &&
		fscanf(f,"P5 %zu %zu %" SCNu16,width,height,&depth) == 3 &&
		fgetc(f) != EOF &&
		depth < 256 &&
		*width && *height &&
		!(*width > PIXEL_MAX / *height) &&
		(image = malloc(*width * *height)) &&
		fread(image,1,*width * *height,f) == *width * *height
	)
		return image;
	free(image);
	return NULL;
}

struct image_reader resdet_image_reader_pgm = {
	.read = read_pgm
};
