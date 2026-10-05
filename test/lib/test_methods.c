#include "test.h"

void test_sign_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("sign");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect_file("test/files/blue_marble_2012_resized.pfm",NULL,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_mag_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("mag");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect_file("test/files/blue_marble_2012_resized.pfm",NULL,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_orig_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("orig");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect_file("test/files/blue_marble_2012_resized.pfm",NULL,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_zerox_method_detects_resolutions(void** state) {
	RDMethod* method = resdet_get_method("zerox");
	assert_non_null(method);

	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect_file("test/files/blue_marble_2012_resized.pfm",NULL,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,3,3);
}
