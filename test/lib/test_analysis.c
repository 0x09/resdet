#include "test.h"

struct analysis_ctx {
	RDAnalysis* analysis;
	float* image;
};

int setup_analysis_group(void** state) {
	struct analysis_ctx* ctx = malloc(sizeof(*ctx));
	if(!ctx)
		return 1;

	size_t width, height, nimages;
	int err = resdet_read_image("test/files/blue_marble_2012_resized.pfm",NULL,&ctx->image,&nimages,&width,&height);
	if(err)
		return 1;

	*state = ctx;
	return 0;
}

int teardown_analysis_group(void** state) {
	struct analysis_ctx* ctx = *state;
	free(ctx->image);
	free(ctx);
	return 0;
}

int setup_analysis_tests(void** state) {
	struct analysis_ctx* ctx = *state;
	if(!(ctx->analysis = resdet_create_analysis(NULL,768,768,NULL,NULL)))
		return 1;
	return 0;
}

int teardown_analysis_tests(void** state) {
	struct analysis_ctx* ctx = *state;
	resdet_destroy_analysis(ctx->analysis);
	return 0;
}

// teardown: teardown_analysis_tests
void test_creates_analysis(void** state) {
	struct analysis_ctx* ctx = *state;
	int err;

	ctx->analysis = resdet_create_analysis(NULL,768,768,NULL,&err);

	assert_non_null(ctx->analysis);
	assert_false(err);
}

void test_create_analysis_returns_error_on_zero_size_image(void** state) {
	int err;
	RDAnalysis* analysis = resdet_create_analysis(NULL,0,0,NULL,&err);

	assert_null(analysis);
	assert_int_equal(err,RDEINVAL);
}

void test_create_analysis_returns_error_on_oversized_image(void** state) {
	int err;
	RDAnalysis* analysis = resdet_create_analysis(NULL,SIZE_MAX,SIZE_MAX,NULL,&err);

	assert_null(analysis);
	assert_int_equal(err,RDETOOBIG);
}

void test_analyze_image_with_no_analysis_returns_error(void** state) {
	struct analysis_ctx* ctx = *state;

	int err = resdet_analyze_image(NULL,ctx->image);

	assert_int_equal(err,RDEPARAM);
}

// setup: setup_analysis_tests
// teardown: teardown_analysis_tests
void test_analyze_image_with_no_image_returns_error(void** state) {
	struct analysis_ctx* ctx = *state;

	int err = resdet_analyze_image(ctx->analysis,NULL);

	assert_int_equal(err,RDEPARAM);
}

void test_analysis_results_with_no_analysis_returns_error(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdet_analysis_results(NULL,&resw,&countw,&resh,&counth);

	assert_int_equal(err,RDEPARAM);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(countw,0);
	assert_uint_equal(counth,0);
}

// setup: setup_analysis_tests
// teardown: teardown_analysis_tests
void test_analysis_results_with_no_images_returns_error(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	int err = resdet_analysis_results(ctx->analysis,&resw,&countw,&resh,&counth);

	assert_int_equal(err,RDENOIMG);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(countw,0);
	assert_uint_equal(counth,0);
}

// setup: setup_analysis_tests
// teardown: teardown_analysis_tests
void test_analysis_results_with_resw_but_no_countw_returns_error(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t counth;

	int err = resdet_analysis_results(ctx->analysis,&resw,NULL,&resh,&counth);

	assert_int_equal(err,RDEPARAM);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(counth,0);
}

// setup: setup_analysis_tests
// teardown: teardown_analysis_tests
void test_analysis_results_with_resh_but_no_counth_returns_error(void** state) {
	struct analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw;

	int err = resdet_analysis_results(ctx->analysis,&resw,&countw,&resh,NULL);

	assert_int_equal(err,RDEPARAM);

	assert_null(resw);
	assert_null(resh);

	assert_uint_equal(countw,0);
}

// setup: setup_analysis_tests
// teardown: teardown_analysis_tests
void test_analysis_detects_resolutions(void** state) {
	struct analysis_ctx* ctx = *state;
	int err;

	err = resdet_analyze_image(ctx->analysis,ctx->image);

	assert_false(err);

	RDResolution* resw,* resh;
	size_t countw, counth;

	err = resdet_analysis_results(ctx->analysis,&resw,&countw,&resh,&counth);

	run_sample_image_assertions(resw,resh,countw,counth,2,2);
}
