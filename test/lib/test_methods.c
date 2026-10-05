#include "test.h"

int setup_methods_group(void** state) {
	float* image;
	size_t width, height, nimages;
	RDError err = resdet_read_image("test/files/blue_marble_2012_resized.pfm",NULL,&image,&nimages,&width,&height);

	*state = image;

	if(err)
		return 1;
	return 0;
}

int teardown_methods_group(void** state) {
	free(*state);
	return 0;
}

void test_sign_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("sign");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect(*state,1,768,768,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_mag_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("mag");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect(*state,1,768,768,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_orig_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("orig");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect(*state,1,768,768,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_zerox_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("zerox");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect(*state,1,768,768,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,3,3);
}
