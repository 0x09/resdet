/*
 * resdet core routines.
 * This file is part of libresdet.
 */

#include <stdio.h>
#include <math.h>

#include "resdet_internal.h"

static const RDParameters default_params = {
	.range = DEFAULT_RANGE,
	.threshold = -1
};

static int sortres(const void* left, const void* right) {
	float left_confidence = ((const RDResolution*)left)->confidence,
	      right_confidence = ((const RDResolution*)right)->confidence;

	return left_confidence < right_confidence ? 1 : (left_confidence > right_confidence ? -1 : 0);
}

static RDError setup_dimension(size_t length, size_t range, intermediate** buf, rdint_index bounds[2]) {
	size_t maxlen = 0;
	if(range < (length+1)/2)
		maxlen = length - range*2;

	if(!maxlen)
		return RDEOK;

	if(!(*buf = calloc(maxlen,sizeof(**buf))))
		return RDENOMEM;
	// bounds of result (range of meaningful outputs)
	// may be narrowed by methods
	bounds[0] = range;
	bounds[1] = length - range;
	return RDEOK;
}

RDAnalysis* resdet_create_analysis(RDMethod* method, size_t width, size_t height, const RDParameters* params, RDError* error) {
	RDError e;

	RDAnalysis* analysis = malloc(sizeof(*analysis));
	if(!analysis) {
		e = RDENOMEM;
		goto error;
	}

	if(!method)
		method = resdet_get_method(NULL);

	analysis->method = method;
	analysis->width = width;
	analysis->height = height;
	analysis->params = params ? *params : default_params;
	analysis->nimages = 0;
	analysis->xresult = analysis->yresult = NULL;
	analysis->p = NULL;
	analysis->f = NULL;

	if(analysis->params.threshold < 0)
		analysis->params.threshold = method->threshold;

	if(resdet_dims_exceed_limit(width,height,1,coeff)) {
		e = RDETOOBIG;
		goto error;
	}

	if(!(analysis->f = resdet_alloc_coeffs(width,height))) {
		e = RDENOMEM;
		goto error;
	}

	analysis->p = resdet_create_plan(analysis->f,width,height,&e);
	if(e)
		goto error;

	if((e = setup_dimension(width,analysis->params.range,&analysis->xresult,analysis->xbound)) != RDEOK)
		goto error;
	if((e = setup_dimension(height,analysis->params.range,&analysis->yresult,analysis->ybound)) != RDEOK)
		goto error;

	if(error)
		*error = RDEOK;

	return analysis;

error:
	resdet_destroy_analysis(analysis);

	if(error)
		*error = e;

	return NULL;
}

RDError resdet_analyze_image(RDAnalysis* analysis, float* image) {
	if(!analysis)
		return RDEPARAM;

	RDError ret;
	size_t width = analysis->width, height = analysis->height;

	for(rdint_index i = 0; i < width*height; i++) {
		if(!isfinite(image[i])) {
			ret = RDEINVAL;
			goto end;
		}
		analysis->f[i] = image[i];
	}

	resdet_transform(analysis->p);

	if((ret = ((RDetectFunc)analysis->method->func)(analysis->f,width,height,width,1,analysis->params.range,analysis->xresult,analysis->xbound,analysis->xbound+1)) != RDEOK)
		goto end;
	if((ret = ((RDetectFunc)analysis->method->func)(analysis->f,height,width,1,width,analysis->params.range,analysis->yresult,analysis->ybound,analysis->ybound+1)) != RDEOK)
		goto end;

	analysis->nimages++;

end:
	return ret;
}

static RDError generate_dimension_results(RDAnalysis* analysis, size_t length, rdint_index bounds[2], intermediate* result, RDResolution** res, size_t* count) {
	size_t nresults = 1;

	if(result)
		for(rdint_index i = 0; i < bounds[1]-bounds[0]; i++)
			if(result[i]/analysis->nimages >= analysis->params.threshold)
				nresults++;

	if(!(*res = malloc(nresults*sizeof(**res))))
		return RDENOMEM;

	(*res)[(*count)++] = (RDResolution){length,-1};

	if(result)
		for(rdint_index i = 0; i < bounds[1]-bounds[0]; i++)
			if(result[i]/analysis->nimages >= analysis->params.threshold)
				(*res)[(*count)++] = (RDResolution){i+bounds[0],result[i]/analysis->nimages};

	qsort(*res,*count,sizeof(**res),sortres);

	return RDEOK;
}

RDError resdet_analysis_results(RDAnalysis* analysis, RDResolution** restrict rw, size_t* restrict cw, RDResolution** restrict  rh, size_t* restrict ch) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }

	if(!analysis)
		return RDEPARAM;

	RDError error;

	if(rw && (error = generate_dimension_results(analysis,analysis->width,analysis->xbound,analysis->xresult,rw,cw)))
		goto error;
	if(rh && (error = generate_dimension_results(analysis,analysis->height,analysis->ybound,analysis->yresult,rh,ch)))
		goto error;

	return RDEOK;

error:
	if(rw) {
		free(*rw);
		*rw = NULL;
		*cw = 0;
	}
	if(rh) {
		free(*rh);
		*rh = NULL;
		*ch = 0;
	}

	return error;
}

void resdet_destroy_analysis(RDAnalysis* analysis) {
	if(!analysis)
		return;

	free(analysis->xresult);
	free(analysis->yresult);
	resdet_free_plan(analysis->p);
	resdet_free_coeffs(analysis->f);
	free(analysis);
}

RDError resdetect(float* image, size_t nimages, size_t width, size_t height,
                                    RDResolution** restrict rw, size_t* restrict cw,
                                    RDResolution** restrict rh, size_t* restrict ch,
                                    RDMethod* method, const RDParameters* params) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }

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

RDError resdetect_file(const char* filename, const char* filetype,
                                   RDResolution** restrict rw, size_t* restrict cw,
                                   RDResolution** restrict rh, size_t* restrict ch,
                                   RDMethod* method, const RDParameters* params) {
	if(rw) { *rw = NULL; *cw = 0; }
	if(rh) { *rh = NULL; *ch = 0; }

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

RDMethod* resdet_get_method(const char* name) {
	if(!name)
		return resdet_methods(); //default
	for(RDMethod* m = resdet_methods(); m->name; m++)
		if(!strcmp(m->name,name))
			return m;
	return NULL;
}

RDParameters* resdet_alloc_default_parameters(void) {
	 RDParameters* params = malloc(sizeof(*params));
	 if(!params)
		 return NULL;

	 *params = default_params;
	 return params;
}

RDError resdet_parameters_set_range(RDParameters* params, size_t range) {
	if(!params || !range)
		return RDEPARAM;

	params->range = range;
	return RDEOK;
}

RDError resdet_parameters_set_threshold(RDParameters* params, float threshold) {
	if(!params || isnan(threshold) || threshold < 0 || threshold > 1)
		return RDEPARAM;

	params->threshold = threshold;
	return RDEOK;
}


size_t resdet_default_range(void) {
	return DEFAULT_RANGE;
}

const char* resdet_libversion(void) {
	return RESDET_LIBVERSION_STRING;
}

const char* resdet_error_str(RDError e) {
	static const char* const RDErrStr[] = {
		[RDEOK]       = "",
		[RDENOMEM]    = "Out of memory",
		[RDEINTERNAL] = "Internal error",
		[RDEINVAL]    = "Invalid image",
		[RDEUNSUPP]   = "Unsupported image file format",
		[RDETOOBIG]   = "Image size exceeds limit",
		[RDEPARAM]    = "Parameter missing or invalid"
	};

	if(e < 0)
		return strerror(-e);

	if(e >= sizeof(RDErrStr)/sizeof(*RDErrStr))
		return NULL;

	return RDErrStr[e];
}
