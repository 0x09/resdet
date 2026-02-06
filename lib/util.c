/*
 * Utility functions.
 * This file is part of libresdet.
 */

#include "resdet_internal.h"

RESDET_API RDMethod* resdet_get_method(const char* name) {
	if(!name)
		return resdet_methods(); //default
	for(RDMethod* m = resdet_methods(); m->name; m++)
		if(!strcmp(m->name,name))
			return m;
	return NULL;
}

RESDET_API RDParameters* resdet_alloc_default_parameters(void) {
	 RDParameters* params = malloc(sizeof(*params));
	 if(!params)
		 return NULL;

	 *params = default_params;
	 return params;
}

RESDET_API RDError resdet_parameters_set_range(RDParameters* params, size_t range) {
	if(!params || !range)
		return RDEPARAM;

	params->range = range;
	return RDEOK;
}

RESDET_API RDError resdet_parameters_set_threshold(RDParameters* params, float threshold) {
	if(!params || isnan(threshold) || threshold < 0 || threshold > 1)
		return RDEPARAM;

	params->threshold = threshold;
	return RDEOK;
}


RESDET_API size_t resdet_default_range(void) {
	return DEFAULT_RANGE;
}

RESDET_API const char* resdet_libversion(void) {
	return RESDET_LIBVERSION_STRING;
}

static const char* const RDErrStr[] = {
	[RDEOK]       = "",
	[RDENOMEM]    = "Out of memory",
	[RDEINTERNAL] = "Internal error",
	[RDEINVAL]    = "Invalid image",
	[RDEUNSUPP]   = "Unsupported image file format",
	[RDETOOBIG]   = "Image size exceeds limit",
	[RDEPARAM]    = "Parameter missing or invalid",
	[RDENOIMG]    = "No images were analyzed",
};

RESDET_API const char* resdet_error_str(RDError e) {

	if(e < 0)
		return strerror(-e);

	if(e > resdet_get_max_error())
		return NULL;

	return RDErrStr[e];
}

RESDET_API enum RDErrors resdet_get_max_error(void) {
	return sizeof(RDErrStr)/sizeof(*RDErrStr)-1;
}
