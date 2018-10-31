/*
 * imgread - Sanity check resdet's built in image readers.
 * Copyright (C) 2013-2017 0x09.net.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "resdet.h"

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("Usage: %s input_image output.pgm", argv[0]);
		return 0;
	}

	RDContext* ctx = resdet_open_context();
	if(!ctx) {
		fprintf(stderr,"Context creation failed.\n");
		return 1;
	}

	unsigned char* image;
	size_t nimages, width, height;
	RDError e = resdet_read_image(ctx, argv[1], NULL, &image, &nimages, &width, &height);
	resdet_close_context(ctx);

	if(e) {
		fprintf(stderr,"%s\n",RDErrStr[e]);
		return 1;
	}

	FILE* out = fopen(argv[2], "w");
	fprintf(out, "P2\n%zu %zu\n256", width, height);
	for(size_t i = 0; i < width*height; i++) {
		if(!(i%70)) fprintf(out, "\n");
		fprintf(out, "%u ", image[i]);
	}
	fclose(out);
	free(image);
}