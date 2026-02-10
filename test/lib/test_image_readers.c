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

int teardown_image_reader_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	free(ctx->imagebuf);
	resdet_close_image(ctx->image);
	return 0;
}

static void run_opens_test(void** state, const char* filename) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image(filename,NULL,&width,&height,&ctx->imagebuf,&err);

	assert_false(err);
	assert_non_null(ctx->image);
	assert_non_null(ctx->imagebuf);
	assert_uint_equal(width,2);
	assert_uint_equal(height,2);
}

static void run_open_image_errors_on_nonexistent_file_test(void** state, const char* filename) {
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image(filename,NULL,&width,&height,NULL,&err);

	assert_true(err < 0);
	assert_null(image);
	assert_uint_equal(width,0);
	assert_uint_equal(height,0);
}

static void run_still_frame_reads_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_true(ret);
	assert_false(err);
	assert_array_equal(ctx->imagebuf,ctx->checkerboard[0]);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);
	assert_false(ret);
	assert_false(err);
}

static void run_multi_frame_reads_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_true(ret);
	assert_false(err);
	assert_array_equal(ctx->imagebuf,ctx->checkerboard[0]);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_true(ret);
	assert_false(err);
	assert_array_equal(ctx->imagebuf,ctx->checkerboard[1]);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);
	assert_false(ret);
	assert_false(err);
}

static void run_still_frame_seeks_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_false(ret);
	assert_false(err);

	ret = resdet_seek_frame(ctx->image,0,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}

static void run_multi_frame_seeks_test(void** state) {
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

	ret = resdet_seek_frame(ctx->image,0,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}

static void run_still_frame_seeks_to_eof_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}

static void run_multi_frame_seeks_to_eof_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,2,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}

static void run_still_frame_seeking_past_end_of_file_returns_false_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,2,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}

static void run_multi_frame_seeking_past_end_of_file_returns_false_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,3,NULL,NULL,&err);

	assert_false(ret);
	assert_false(err);
}


void run_still_frame_reading_past_end_of_file_returns_false_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,1,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_false(ret);
	assert_false(err);
}

static void run_multi_frame_reading_past_end_of_file_returns_false_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_seek_frame(ctx->image,2,NULL,NULL,&err);

	assert_true(ret);
	assert_false(err);

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_false(ret);
	assert_false(err);
}

static void progress(void* ctx, uint64_t frameno) {
	*(uint64_t*)ctx = frameno;
}

static void run_still_frame_seeks_with_progress_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	uint64_t counter = 0;

	bool ret = resdet_seek_frame(ctx->image,1,progress,&counter,&err);

	assert_true(ret);
	assert_false(err);
	assert_uint_equal(counter,1);
}


static void run_multi_frame_seeks_with_progress_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;
	uint64_t counter = 0;

	bool ret = resdet_seek_frame(ctx->image,2,progress,&counter,&err);

	assert_true(ret);
	assert_false(err);
	assert_uint_equal(counter,2);
}

static void run_read_frame_errors_on_partial_data_test(void** state) {
	struct image_reader_ctx* ctx = *state;
	RDError err;

	bool ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_false(ret);
	assert_int_equal(err,RDEINVAL);
}

static void run_open_image_errors_on_corrupt_header_test(void** state, const char* filename) {
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image(filename,NULL,&width,&height,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEINVAL);
	assert_uint_equal(width,0);
	assert_uint_equal(height,0);
}

int setup_pgm_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.pgm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
void test_opens_pgm(void** state) {
	run_opens_test(state,"test/files/checkerboard.pgm");
}


void test_open_image_errors_on_nonexistent_pgm(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.pgm");
}

int setup_png_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.png",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_opens_png(void** state) {
	run_opens_test(state,"test/files/checkerboard.png");
}


// guard: HAVE_LIBPNG
void test_open_image_errors_on_nonexistent_png(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.png");
}

int setup_jpg_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.jpg",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_opens_jpg(void** state) {
	run_opens_test(state,"test/files/checkerboard.jpg");
}


// guard: HAVE_LIBJPEG
void test_open_image_errors_on_nonexistent_jpg(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.jpg");
}

int setup_pfm_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
void test_opens_pfm(void** state) {
	run_opens_test(state,"test/files/checkerboard.pfm");
}


void test_open_image_errors_on_nonexistent_pfm(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.pfm");
}

int setup_y4m_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.y4m",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
void test_opens_y4m(void** state) {
	run_opens_test(state,"test/files/checkerboard.y4m");
}


void test_open_image_errors_on_nonexistent_y4m(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.y4m");
}

int setup_avi_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.avi",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_opens_avi(void** state) {
	run_opens_test(state,"test/files/checkerboard.avi");
}


// guard: HAVE_FFMPEG
void test_open_image_errors_on_nonexistent_avi(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.avi");
}

int setup_miff_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.miff",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_opens_miff(void** state) {
	run_opens_test(state,"test/files/checkerboard.miff");
}


// guard: HAVE_MAGICKWAND
void test_open_image_errors_on_nonexistent_miff(void** state) {
	run_open_image_errors_on_nonexistent_file_test(state,"test/files/doesntexist.miff");
}

// setup: setup_pgm_tests
// teardown: teardown_image_reader_tests
void test_reads_pgm(void** state) {
	run_still_frame_reads_test(state);
}

// setup: setup_png_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_reads_png(void** state) {
	run_still_frame_reads_test(state);
}

// setup: setup_jpg_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_reads_jpg(void** state) {
	run_still_frame_reads_test(state);
}

// setup: setup_pfm_tests
// teardown: teardown_image_reader_tests
void test_reads_pfm(void** state) {
	run_multi_frame_reads_test(state);
}

// setup: setup_y4m_tests
// teardown: teardown_image_reader_tests
void test_reads_y4m(void** state) {
	run_multi_frame_reads_test(state);
}

// setup: setup_avi_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_reads_avi(void** state) {
	run_multi_frame_reads_test(state);
}

// setup: setup_miff_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_reads_miff(void** state) {
	run_multi_frame_reads_test(state);
}

// setup: setup_pgm_tests
// teardown: teardown_image_reader_tests
void test_seeks_pgm(void** state) {
	run_still_frame_seeks_test(state);
}

// setup: setup_png_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_seeks_png(void** state) {
	run_still_frame_seeks_test(state);
}

// setup: setup_jpg_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_seeks_jpg(void** state) {
	run_still_frame_seeks_test(state);
}

// setup: setup_pfm_tests
// teardown: teardown_image_reader_tests
void test_seeks_pfm(void** state) {
	run_multi_frame_seeks_test(state);
}

// setup: setup_y4m_tests
// teardown: teardown_image_reader_tests
void test_seeks_y4m(void** state) {
	run_multi_frame_seeks_test(state);
}

// setup: setup_avi_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_seeks_avi(void** state) {
	run_multi_frame_seeks_test(state);
}

// setup: setup_miff_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_seeks_miff(void** state) {
	run_multi_frame_seeks_test(state);
}

// setup: setup_pgm_tests
// teardown: teardown_image_reader_tests
void test_seeks_to_eof_pgm(void** state) {
	run_still_frame_seeks_to_eof_test(state);
}

// setup: setup_png_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_seeks_to_eof_png(void** state) {
	run_still_frame_seeks_to_eof_test(state);
}

// setup: setup_jpg_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_seeks_to_eof_jpg(void** state) {
	run_still_frame_seeks_to_eof_test(state);
}

// setup: setup_pfm_tests
// teardown: teardown_image_reader_tests
void test_seeks_to_eof_pfm(void** state) {
	run_multi_frame_seeks_to_eof_test(state);
}

// setup: setup_y4m_tests
// teardown: teardown_image_reader_tests
void test_seeks_to_eof_y4m(void** state) {
	run_multi_frame_seeks_to_eof_test(state);
}

// setup: setup_avi_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_seeks_to_eof_avi(void** state) {
	run_multi_frame_seeks_to_eof_test(state);
}

// setup: setup_miff_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_seeks_to_eof_miff(void** state) {
	run_multi_frame_seeks_to_eof_test(state);
}

// setup: setup_pgm_tests
// teardown: teardown_image_reader_tests
void test_seeking_past_end_of_file_returns_false_pgm(void** state) {
	run_still_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_png_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_seeking_past_end_of_file_returns_false_png(void** state) {
	run_still_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_jpg_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_seeking_past_end_of_file_returns_false_jpg(void** state) {
	run_still_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_pfm_tests
// teardown: teardown_image_reader_tests
void test_seeking_past_end_of_file_returns_false_pfm(void** state) {
	run_multi_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_y4m_tests
// teardown: teardown_image_reader_tests
void test_seeking_past_end_of_file_returns_false_y4m(void** state) {
	run_multi_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_avi_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_seeking_past_end_of_file_returns_false_avi(void** state) {
	run_multi_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_miff_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_seeking_past_end_of_file_returns_false_miff(void** state) {
	run_multi_frame_seeking_past_end_of_file_returns_false_test(state);
}

// setup: setup_pgm_tests
// teardown: teardown_image_reader_tests
void test_reading_past_end_of_file_returns_false_pgm(void** state) {
	run_still_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_png_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_reading_past_end_of_file_returns_false_png(void** state) {
	run_still_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_jpg_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_reading_past_end_of_file_returns_false_jpg(void** state) {
	run_still_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_pfm_tests
// teardown: teardown_image_reader_tests
void test_reading_past_end_of_file_returns_false_pfm(void** state) {
	run_multi_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_y4m_tests
// teardown: teardown_image_reader_tests
void test_reading_past_end_of_file_returns_false_y4m(void** state) {
	run_multi_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_avi_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_reading_past_end_of_file_returns_false_avi(void** state) {
	run_multi_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_miff_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_reading_past_end_of_file_returns_false_miff(void** state) {
	run_multi_frame_reading_past_end_of_file_returns_false_test(state);
}

// setup: setup_pgm_tests
// teardown: teardown_image_reader_tests
void test_seeks_with_progress_pgm(void** state) {
	run_still_frame_seeks_with_progress_test(state);
}

// setup: setup_png_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBPNG
void test_seeks_with_progress_png(void** state) {
	run_still_frame_seeks_with_progress_test(state);
}

// setup: setup_jpg_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_LIBJPEG
void test_seeks_with_progress_jpg(void** state) {
	run_still_frame_seeks_with_progress_test(state);
}

// setup: setup_pfm_tests
// teardown: teardown_image_reader_tests
void test_seeks_with_progress_pfm(void** state) {
	run_multi_frame_seeks_with_progress_test(state);
}

// setup: setup_y4m_tests
// teardown: teardown_image_reader_tests
void test_seeks_with_progress_y4m(void** state) {
	run_multi_frame_seeks_with_progress_test(state);
}

// setup: setup_avi_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_FFMPEG
void test_seeks_with_progress_avi(void** state) {
	run_multi_frame_seeks_with_progress_test(state);
}

// setup: setup_miff_tests
// teardown: teardown_image_reader_tests
// guard: HAVE_MAGICKWAND
void test_seeks_with_progress_miff(void** state) {
	run_multi_frame_seeks_with_progress_test(state);
}

int setup_partial_pgm_data_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/partial_data.pgm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

void test_open_image_errors_on_corrupt_header_pgm(void** state) {
	run_open_image_errors_on_corrupt_header_test(state,"test/files/corrupt_header.pgm");
}

// setup: setup_partial_pgm_data_tests
// teardown: teardown_image_reader_tests
void test_read_frame_errors_on_partial_data_pgm(void** state) {
	run_read_frame_errors_on_partial_data_test(state);
}

int setup_partial_pfm_data_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/partial_data.pfm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

void test_open_image_errors_on_corrupt_header_pfm(void** state) {
	run_open_image_errors_on_corrupt_header_test(state,"test/files/corrupt_header.pfm");
}

// setup: setup_partial_pfm_data_tests
// teardown: teardown_image_reader_tests
void test_read_frame_errors_on_partial_data_pfm(void** state) {
	run_read_frame_errors_on_partial_data_test(state);
}

int setup_partial_y4m_data_tests(void** state) {
	struct image_reader_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/partial_data.y4m",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

void test_open_image_errors_on_corrupt_header_y4m(void** state) {
	run_open_image_errors_on_corrupt_header_test(state,"test/files/corrupt_header.y4m");
}

// setup: setup_partial_y4m_data_tests
// teardown: teardown_image_reader_tests
void test_read_frame_errors_on_partial_data_y4m(void** state) {
	run_read_frame_errors_on_partial_data_test(state);
}

