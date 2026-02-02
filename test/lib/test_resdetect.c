#include "test.h"

struct image_ctx {
	float* image;
};

int setup_resdetect_group(void** state) {
	float* image;
	size_t width, height, nimages;
	int err = resdet_read_image("test/files/blue_marble_2012_resized.pfm",NULL,&image,&nimages,&width,&height);

	*state = image;

	if(err)
		return 1;
	return 0;
}

int teardown_resdetect_group(void** state) {
	free(*state);
	return 0;
}

void test_resdetect_detects_resolutions(void** state) {
	float* image = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdetect(image,1,768,768,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_resdetect_returns_error_and_nullifies_outputs_with_no_dimensions(void** state) {
	float* image = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdetect(image,1,0,0,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_int_equal(err,RDEINVAL);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(countw,0);
	assert_uint_equal(counth,0);
}

void test_resdetect_returns_error_and_nullifies_outputs_with_no_images(void** state) {
	float* image = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdetect(image,0,768,768,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_int_equal(err,RDENOIMG);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(countw,0);
	assert_uint_equal(counth,0);
}

static void run_method_tests(void** state, const char* methodname, size_t detected_widths, size_t detected_heights) {
	float* image = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	RDMethod* method = resdet_get_method(methodname);

	assert_non_null(method);

	int err = resdetect(image,1,768,768,&resw,&countw,&resh,&counth,method,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,detected_widths,detected_heights);
}

void test_sign_method(void** state) {
	run_method_tests(state,"sign",2,2);
}

void test_mag_method(void** state) {
	run_method_tests(state,"mag",2,2);
}

void test_orig_method(void** state) {
	run_method_tests(state,"orig",2,2);
}

void test_zerox_method(void** state) {
	run_method_tests(state,"zerox",3,3);
}

void test_zero_threshold_returns_all_analyzed_resolutions(void** state) {
	float* image = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	RDParameters* params = resdet_alloc_default_parameters();

	assert_non_null(params);

	resdet_parameters_set_threshold(params,0);
	int err = resdetect(image,1,768,768,&resw,&countw,&resh,&counth,NULL,params);
	free(params);

	assert_false(err);

	assert_uint_equal(countw,745);
	assert_uint_equal(counth,745);
}

void test_lower_range_gives_more_results(void** state) {
	float* image = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	RDParameters* params = resdet_alloc_default_parameters();

	assert_non_null(params);

	resdet_parameters_set_range(params,8);
	int err = resdetect(image,1,768,768,&resw,&countw,&resh,&counth,NULL,params);
	free(params);

	assert_false(err);

	assert_uint_equal(countw,3);
	assert_uint_equal(counth,5);
}