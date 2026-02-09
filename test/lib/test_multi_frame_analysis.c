#include "test.h"

struct multi_frame_analysis_ctx {
	RDAnalysis* analysis;
	float* image;
};

int setup_multi_frame_analysis_group(void** state) {
	struct multi_frame_analysis_ctx* ctx = malloc(sizeof(*ctx));
	if(!ctx)
		return 1;

	size_t width, height, nimages;
	RDError err = resdet_read_image("test/files/rotating_blue_marble_resized.y4m",NULL,&ctx->image,&nimages,&width,&height);
	if(err)
		return 1;

	if(!(ctx->analysis = resdet_create_analysis(NULL,1920,1080,NULL,NULL)))
		return 1;

	*state = ctx;
	return 0;
}

int teardown_multi_frame_analysis_group(void** state) {
	struct multi_frame_analysis_ctx* ctx = *state;
	resdet_destroy_analysis(ctx->analysis);
	free(ctx->image);
	free(ctx);
	return 0;
}

void test_resdetect_detects_with_multiple_frames(void** state) {
	struct multi_frame_analysis_ctx* ctx = *state;
	RDResolution* resw,* resh;
	size_t countw, counth;

	RDError err = resdetect(ctx->image,2,1920,1080,&resw,&countw,&resh,&counth,NULL,NULL);

	assert_false(err);

	assert_non_null(resw);
	assert_non_null(resh);

	assert_true(countw > 1);
	assert_true(counth > 1);

	assert_uint_equal(resw[0].index,1280);
	assert_uint_equal(resh[0].index,720);

	assert_uint_equal(resw[countw-1].index,1920);
	assert_uint_equal(resh[counth-1].index,1080);
}

void test_analyze_image_with_multiple_frames_narrows_down_results(void** state) {
	struct multi_frame_analysis_ctx* ctx = *state;
	RDResolution* resw1,* resw2;
	size_t countw1, countw2;
	RDError err;

	err = resdet_analyze_image(ctx->analysis,ctx->image);

	assert_false(err);

	err = resdet_analysis_results(ctx->analysis,&resw1,&countw1,NULL,NULL);
	free(resw1);

	assert_false(err);

	assert_true(countw1 > 0);

	err = resdet_analyze_image(ctx->analysis,ctx->image+1920*1080);

	assert_false(err);

	err = resdet_analysis_results(ctx->analysis,&resw2,&countw2,NULL,NULL);
	free(resw2);

	assert_false(err);

	assert_true(countw2 < countw1);
}
