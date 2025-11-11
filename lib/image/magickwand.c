#include "image.h"

#if HAVE_MAGICKWAND > 6
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif

static float* read_magick(const char* filename, size_t* width, size_t* height, size_t* nimages, RDError* error) {
	*error = RDEOK;
	float* image = NULL;
	MagickWand* wand = NewMagickWand();
	if(!MagickReadImage(wand,filename)) {
		*error = RDEINVAL;
		goto end;
	}
	*width = MagickGetImageWidth(wand);
	*height = MagickGetImageHeight(wand);
	*nimages = MagickGetNumberImages(wand);
	if(resdet_dims_exceed_limit(*width,*height,*nimages,*image)) {
		*error = RDEINVAL;
		goto end;
	}
	if(!(image = malloc(sizeof(*image) * *width * *height * *nimages))) {
		*error = RDENOMEM;
		goto end;
	}
	MagickResetIterator(wand);
	for(size_t i = 0; i < *nimages; i++) {
		MagickNextImage(wand);
		MagickExportImagePixels(wand,0,0,*width,*height,"I",FloatPixel,image + i * *width* *height);
	}
end:
	DestroyMagickWand(wand);
	return image;
}

struct image_reader resdet_image_reader_magickwand = {
	.read = read_magick
};
