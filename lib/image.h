#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>

#include "resdet_internal.h"

struct image_reader {
	unsigned char* (*read)(FILE*,size_t*,size_t*,size_t*);
};

#endif
