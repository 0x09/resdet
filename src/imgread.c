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

	FILE* out = NULL;

	float* image;
	size_t width, height;
	RDError e;
	RDImage* rdimage = resdet_open_image(argv[1],NULL,&width,&height,&image,&e);

	if(e)
		goto end;

	out = fopen(argv[2], "wb");
	if(!out) {
		perror("error opening output");
		goto end;
	}

	while(resdet_read_image_frame(rdimage,image,&e)) {
		if(e)
			goto end;

		fprintf(out, "Pf\n%zu %zu\n-1.0\n", width, height);
		for(size_t i = height; i > 0; i--)
			fwrite(image+(i-1)*width,sizeof(*image),width,out);
	}

end:
	if(e)
		fprintf(stderr,"%s\n",resdet_error_str(e));

	resdet_close_image(rdimage);
	if(out)
		fclose(out);
	free(image);
}
