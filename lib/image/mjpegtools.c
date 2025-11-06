#include "image.h"

#include <mjpegtools/yuv4mpeg.h>
#include <fcntl.h>

static float* read_y4m(const char* filename, size_t* width, size_t* height, size_t* nimages) {
	unsigned char* image = NULL,* discard = NULL;
	float* imagef = NULL;
	int fd = strcmp(filename,"-") ? open(filename,O_RDONLY) : 0;
	if(fd < 0)
		return NULL;

	y4m_stream_info_t st;
	y4m_frame_info_t frame;
	y4m_init_stream_info(&st);
	y4m_init_frame_info(&frame);
	if(y4m_read_stream_header(fd,&st) != Y4M_OK)
		goto end;
	*width = y4m_si_get_width(&st);
	*height = y4m_si_get_height(&st);
	int frame_length = y4m_si_get_framelength(&st);
	if(resdet_dims_exceed_limit(*width,*height,1,*imagef) || frame_length < 0 || (size_t)frame_length < *width * *height)
		goto end;
	frame_length -= *width * *height; // u/v plane skip
	discard = malloc(frame_length);
	while(y4m_read_frame_header(fd,&st,&frame) == Y4M_OK) {
		(*nimages)++;
		unsigned char* tmp;
		if((*width * *height > PIXEL_MAX / *nimages) ||
		   !(tmp = realloc(image,*width * *height * *nimages))) {
			free(image);
			image = NULL;
			break;
		}

		image = tmp;
		if(y4m_read(fd,image + *width * *height * (*nimages - 1),*width * *height) < 0 ||
		   y4m_read(fd,discard,frame_length) != 0) {
			free(image);
			image = NULL;
			break;
		}
	}

	if(!(imagef = malloc(*width * *height * *nimages * sizeof(*imagef))))
		goto end;
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
