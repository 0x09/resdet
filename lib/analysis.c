/*
 * Sequential analysis functions.
 * This file is part of libresdet.
 */

#include <stdio.h>

#include "resdet_internal.h"

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

RESDET_API RDAnalysis* resdet_create_analysis(RDMethod* method, size_t width, size_t height, const RDParameters* params, RDError* error) {
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

	// this method has an effective range of 1, so allow a larger bounds
	if(!strcmp(method->name,"zerox"))
		analysis->params.range = 1;

	if(!(width && height)) {
		e = RDEINVAL;
		goto error;
	}
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

RESDET_API RDError resdet_analyze_image(RDAnalysis* analysis, float* image) {
	if(!(analysis && image))
		return RDEPARAM;

	RDError ret = RDEOK;
	size_t width = analysis->width, height = analysis->height;

	for(rdint_index i = 0; i < width*height; i++) {
		if(!isfinite(image[i])) {
			ret = RDEINVAL;
			goto end;
		}
		analysis->f[i] = image[i];
	}

	resdet_transform(analysis->p);

	if(analysis->xresult &&
	  (ret = ((RDetectFunc)analysis->method->func)(analysis->f,width,height,width,1,analysis->params.range,analysis->xresult,analysis->xbound,analysis->xbound+1)) != RDEOK)
		goto end;
	if(analysis->yresult &&
	  (ret = ((RDetectFunc)analysis->method->func)(analysis->f,height,width,1,width,analysis->params.range,analysis->yresult,analysis->ybound,analysis->ybound+1)) != RDEOK)
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

RESDET_API RDError resdet_analysis_results(RDAnalysis* analysis, RDResolution** restrict rw, size_t* restrict cw, RDResolution** restrict  rh, size_t* restrict ch) {
	RDError error = RDEOK;

	if(rw) *rw = NULL;
	if(cw) *cw = 0;
	if(rh) *rh = NULL;
	if(ch) *ch = 0;
	if((rw && !cw) || (rh && !ch))
		return RDEPARAM;

	if(!analysis)
		return RDEPARAM;

	if(!analysis->nimages)
		return RDENOIMG;

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

RESDET_API void resdet_destroy_analysis(RDAnalysis* analysis) {
	if(!analysis)
		return;

	free(analysis->xresult);
	free(analysis->yresult);
	resdet_free_plan(analysis->p);
	resdet_free_coeffs(analysis->f);
	free(analysis);
}
