/*
 * imgread - Sanity check resdet's built in image readers.
 */

#include <stdio.h>
#include <stdlib.h>

#include "resdet.h"

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr,"Usage: %s input_image output.pfm\n", argv[0]);
		return 1;
	}

	float* image;
	size_t nimages, width, height;
	RDError e = resdet_read_image(argv[1], NULL, &image, &nimages, &width, &height);

	if(e) {
		fprintf(stderr,"%s\n",resdet_error_str(e));
		return 1;
	}

	FILE* out = fopen(argv[2], "wb");
	if(!out) {
		perror("error opening output");
		free(image);
		return 1;
	}

	fprintf(out, "Pf\n%zu %zu\n-1.0\n", width, height);
	for(size_t i = height; i > 0; i--)
		fwrite(image+(i-1)*width,sizeof(*image),width,out);

	fclose(out);
	free(image);
}