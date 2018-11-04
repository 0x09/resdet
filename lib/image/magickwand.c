#include "image.h"

#if HAVE_MAGICKWAND > 6
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif

static unsigned char* read_magick(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	unsigned char* image = NULL;
	MagickWand* wand = NewMagickWand();
	if(!MagickReadImage(wand,filename))
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

struct image_reader resdet_image_reader_magickwand = {
	.read = read_magick
};
