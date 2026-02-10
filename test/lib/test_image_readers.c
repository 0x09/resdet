#include "test.h"

struct image_reader_ctx {
	float* imagebuf;
	float checkerboard[2][4];
	RDImage* image;
};

int setup_image_reader_group(void** state) {
	struct image_reader_ctx* ctx = malloc(sizeof(*ctx));
	if(!ctx)
		return 1;

	memcpy(ctx->checkerboard,(float[2][4]){
		{
			1, 0,
			0, 1,
		},
		{
			0, 1,
			1, 0
		}
	},sizeof(ctx->checkerboard));

	*state = ctx;

	return 0;
}

int teardown_image_reader_group(void** state) {
	free(*state);
	return 0;
}

int setup_image_reader_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

int teardown_image_reader_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	free(ctx->imagebuf);
	resdet_close_image(ctx->image);
	return 0;
}

int teardown_file_format_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	free(ctx->imagebuf);
	return 0;
}

static void run_image_reader_test(void** state, const char* filename, size_t num_images) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height, nimages;

	RDError err = resdet_read_image(filename,NULL,&ctx->imagebuf,&nimages,&width,&height);

	assert_false(err);
	assert_non_null(ctx->imagebuf);
	assert_uint_equal(nimages,num_images);
	assert_uint_equal(width,2);
	assert_uint_equal(height,2);
	assert_array_equal(ctx->imagebuf,ctx->checkerboard[0]);
	if(num_images > 1)
	    assert_array_equal(ctx->imagebuf+4,ctx->checkerboard[1]);
}

// teardown: teardown_file_format_tests
// guard: HAVE_LIBPNG
void test_reads_png(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.png",1);
}

// teardown: teardown_file_format_tests
// guard: HAVE_LIBJPEG
void test_reads_jpg(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.jpg",1);
}

// teardown: teardown_file_format_tests
void test_reads_pgm(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.pgm",1);
}

// teardown: teardown_file_format_tests
void test_reads_pfm(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.pfm",2);
}

// teardown: teardown_file_format_tests
void test_reads_y4m(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.y4m",2);
}

// teardown: teardown_file_format_tests
void test_reads_y4m_16_bit(void** state) {
	run_image_reader_test(state,"test/files/checkerboard_16_bit.y4m",2);
}

// teardown: teardown_file_format_tests
// guard: HAVE_MAGICKWAND
void test_reads_miff(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.miff",2);
}

// teardown: teardown_file_format_tests
// guard: HAVE_FFMPEG
void test_reads_avi(void** state) {
	run_image_reader_test(state,"test/files/checkerboard.avi",2);
}

void run_seek_frame_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_true(ret);
	assert_false(err);
	assert_array_equal(ctx->imagebuf,ctx->checkerboard[1]);

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}

int setup_seek_frame_tests_pfm(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// setup: setup_seek_frame_tests_pfm
// teardown: teardown_image_reader_tests
void test_seeks_frame_pfm(void** state) {
	run_seek_frame_tests(state);
}

int setup_seek_frame_tests_y4m(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.y4m",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// setup: setup_seek_frame_tests_y4m
// teardown: teardown_image_reader_tests
void test_seeks_frame_y4m(void** state) {
	run_seek_frame_tests(state);
}

int setup_seek_frame_tests_miff(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.miff",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// setup: setup_seek_frame_tests_miff
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_seeks_frame_miff(void** state) {
	run_seek_frame_tests(state);
}

int setup_seek_frame_tests_avi(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.avi",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// setup: setup_seek_frame_tests_avi
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_seeks_frame_avi(void** state) {
	run_seek_frame_tests(state);
}

void test_open_image_errors_on_corrupt_y4m_header(void** state) {
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image("test/files/corrupt_header.y4m",NULL,&width,&height,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEINVAL);
}

void test_open_image_errors_on_corrupt_pgm_header(void** state) {
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image("test/files/corrupt_header.pgm",NULL,&width,&height,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEINVAL);
}

void test_open_image_errors_on_corrupt_pfm_header(void** state) {
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image("test/files/corrupt_header.pfm",NULL,&width,&height,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEINVAL);
}

int setup_partial_y4m_data_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/partial_data.y4m",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

static void read_frame_errors_on_partial_data(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;

	bool ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_false(ret);
	assert_int_equal(err,RDEINVAL);
}

// setup: setup_partial_y4m_data_test
// teardown: teardown_image_reader_tests
void test_read_frame_errors_on_partial_y4m_data(void** state) {
	read_frame_errors_on_partial_data(state);
}

int setup_partial_pgm_data_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/partial_data.pgm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// setup: setup_partial_pgm_data_test
// teardown: teardown_image_reader_tests
void test_read_frame_errors_on_partial_pgm_data(void** state) {
	read_frame_errors_on_partial_data(state);
}

int setup_partial_pfm_data_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/partial_data.pfm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// setup: setup_partial_pfm_data_test
// teardown: teardown_image_reader_tests
void test_read_frame_errors_on_partial_pfm_data(void** state) {
	read_frame_errors_on_partial_data(state);
}

// teardown: teardown_image_reader_tests
void test_can_open_pgm_files_with_comments(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image("test/files/with_comment.pgm",NULL,&width,&height,&ctx->imagebuf,&err);

	assert_false(err);
	assert_non_null(ctx->image);
}

// teardown: teardown_image_reader_tests
void test_can_open_pfm_files_with_comments(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image("test/files/with_comment.pfm",NULL,&width,&height,&ctx->imagebuf,&err);

	assert_false(err);
	assert_non_null(ctx->image);
}
