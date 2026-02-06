#include "test.h"

void test_resdetect_file_detects_resolutions(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdetect_file("test/files/blue_marble_2012_resized.pfm",NULL,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_false(err);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}

void test_images_smaller_than_range_return_only_their_input_dimensions(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdetect_file("test/files/checkerboard.pfm",NULL,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_false(err);

	assert_non_null(resw);
	assert_non_null(resh);

	assert_uint_equal(countw,1);
	assert_uint_equal(counth,1);

	assert_uint_equal(resw[0].index,2);
	assert_uint_equal(resh[0].index,2);
}

void test_resdetect_file_nullifies_outputs_on_error(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdetect_file("test/files/doesntexist.pfm",NULL,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_true(err < 0);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(countw,0);
	assert_uint_equal(counth,0);
}
