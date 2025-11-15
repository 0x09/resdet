/*
 * Image reader for yuv4mpeg files using libmjpegutils from the mjpegtools project.
 * This file is part of libresdet.
 */

#include "image.h"

#include <yuv4mpeg.h>
#include <fcntl.h>

static float* read_y4m(const char* filename, size_t* width, size_t* height, size_t* nimages, RDError* error) {
	*error = RDEOK;
	unsigned char* image = NULL,* discard = NULL;
	float* imagef = NULL;
	int fd = strcmp(filename,"-") ? open(filename,O_RDONLY) : 0;
	if(fd < 0) {
		*error = -errno;
		return NULL;
	}

	y4m_stream_info_t st;
	y4m_frame_info_t frame;
	y4m_init_stream_info(&st);
	y4m_init_frame_info(&frame);
	if(y4m_read_stream_header(fd,&st) != Y4M_OK) {
		*error = RDEINVAL;
		goto end;
	}
	*width = y4m_si_get_width(&st);
	*height = y4m_si_get_height(&st);
	int frame_length = y4m_si_get_framelength(&st);
	if(resdet_dims_exceed_limit(*width,*height,1,*imagef)) {
		*error = RDETOOBIG;
		goto end;
	}
	if(frame_length < 0 || (size_t)frame_length < *width * *height) {
		*error = RDEINVAL;
		goto end;
	}
	frame_length -= *width * *height; // u/v plane skip
	if(!(discard = malloc(frame_length))) {
		*error = RDENOMEM;
		goto end;
	}
	while(y4m_read_frame_header(fd,&st,&frame) == Y4M_OK) {
		(*nimages)++;
		unsigned char* tmp;
		if(resdet_dims_exceed_limit(*width,*height,*nimages,*imagef)) {
			*error = RDETOOBIG;
			free(image);
			image = NULL;
			break;
		}
		if(!(tmp = realloc(image,*width * *height * *nimages))) {
			*error = RDENOMEM;
			free(image);
			image = NULL;
			break;
		}

		image = tmp;
		if(y4m_read(fd,image + *width * *height * (*nimages - 1),*width * *height) < 0 ||
		   y4m_read(fd,discard,frame_length) != 0) {
		   *error = RDEINVAL;
			free(image);
			image = NULL;
			break;
		}
	}
	if(!image)
		goto end;

	if(!(imagef = malloc(*width * *height * *nimages * sizeof(*imagef)))) {
		*error = RDENOMEM;
		goto end;
	}
	for(size_t i = 0; i < *width * *height * *nimages; i++)
		imagef[i] = image[i]/255.f;
    free(image);
end:
	free(discard);
	y4m_fini_frame_info(&frame);
	y4m_fini_stream_info(&st);
	if(fd > 0)
		close(fd);
	return imagef;
}


struct image_reader resdet_image_reader_mjpegtools = {
	.read = read_y4m
};
