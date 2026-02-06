/*
 * High level detection functions.
 * This file is part of libresdet.
 */

#include "resdet_internal.h"

RESDET_API RDError resdetect(float* image, size_t nimages, size_t width, size_t height,
                             RDResolution** restrict rw, size_t* restrict cw,
                             RDResolution** restrict rh, size_t* restrict ch,
                             RDMethod* method, const RDParameters* params) {
	if(rw) *rw = NULL;
	if(cw) *cw = 0;
	if(rh) *rh = NULL;
	if(ch) *ch = 0;
	if((rw && !cw) || (rh && !ch))
		return RDEPARAM;

	if(!image)
		return RDEPARAM;

	if(!nimages)
		return RDENOIMG;

	RDError error;
	RDAnalysis* analysis = resdet_create_analysis(method,width,height,params,&error);
	if(!analysis)
		return error;

	for(size_t i = 0; i < nimages && !error; i++)
		error = resdet_analyze_image(analysis,image+i*width*height);

	if(!error)
		error = resdet_analysis_results(analysis,rw,cw,rh,ch);

	resdet_destroy_analysis(analysis);
	return error;
}

RESDET_API RDError resdetect_file(const char* filename, const char* filetype,
                                  RDResolution** restrict rw, size_t* restrict cw,
                                  RDResolution** restrict rh, size_t* restrict ch,
                                  RDMethod* method, const RDParameters* params) {
	if(rw) *rw = NULL;
	if(cw) *cw = 0;
	if(rh) *rh = NULL;
	if(ch) *ch = 0;
	if((rw && !cw) || (rh && !ch))
		return RDEPARAM;

	RDError error;
	size_t width, height;
	float* image;
	RDImage* rdimage = resdet_open_image(filename,filetype,&width,&height,&image,&error);
	if(error)
		return error;

	RDAnalysis* analysis = resdet_create_analysis(method,width,height,params,&error);
	if(error)
		goto end;

	while(resdet_read_image_frame(rdimage,image,&error))
		if((error = resdet_analyze_image(analysis,image)))
			break;

	if(!error)
		error = resdet_analysis_results(analysis,rw,cw,rh,ch);

end:
	free(image);
	resdet_destroy_analysis(analysis);
	resdet_close_image(rdimage);
	return error;
}
