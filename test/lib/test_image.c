#include "test.h"

struct image_ctx {
	float* imagebuf;
	RDImage* image;
};

int setup_image_group(void** state) {
	struct image_ctx* ctx = malloc(sizeof(*ctx));
	if(!ctx)
		return 1;

	*state = ctx;

	return 0;
}

int teardown_image_group(void** state) {
	free(*state);
	return 0;
}

int setup_image_tests(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	if(!(ctx->image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,&height,&ctx->imagebuf,NULL)))
		return 1;
	return 0;
}

int teardown_image_tests(void** state) {
	struct image_ctx* ctx = *state;
	free(ctx->imagebuf);
	resdet_close_image(ctx->image);
	return 0;
}

int teardown_imagebuf_tests(void** state) {
	struct image_ctx* ctx = *state;
	free(ctx->imagebuf);
	return 0;
}

int teardown_rdimage_tests(void** state) {
	struct image_ctx* ctx = *state;
	resdet_close_image(ctx->image);
	return 0;
}

void test_lists_image_readers(void** state) {
	const char* const* image_readers = resdet_list_image_readers();

	assert_string_equal(image_readers[0],"PGM");

	// assert that this terminates
	for(; *image_readers; image_readers++)
		;
}

// teardown: teardown_image_tests
void test_opens_image(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,&height,&ctx->imagebuf,&err);

	assert_false(err);
	assert_non_null(ctx->image);
	assert_non_null(ctx->imagebuf);
	assert_uint_equal(width,2);
	assert_uint_equal(height,2);
}

// teardown: teardown_rdimage_tests
void test_imagebuf_can_be_null(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,&height,NULL,&err);

	assert_false(err);
	assert_non_null(ctx->image);
}

void test_open_image_with_no_filename_returns_error(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image(NULL,NULL,&width,&height,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEPARAM);
	assert_uint_equal(width,0);
	assert_uint_equal(height,0);
}

void test_open_image_with_no_width_or_height_returns_error(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;
	RDImage* image;

	image = resdet_open_image("test/files/checkerboard.pfm",NULL,NULL,&height,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEPARAM);
	assert_uint_equal(height,0);

	image = resdet_open_image("test/files/checkerboard.pfm",NULL,&width,NULL,NULL,&err);

	assert_null(image);
	assert_int_equal(err,RDEPARAM);
	assert_uint_equal(width,0);
}

// teardown: teardown_rdimage_tests
void test_opens_image_by_extension(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image("test/files/checkerboard","pfm",&width,&height,NULL,&err);

	assert_false(err);
	assert_non_null(ctx->image);
}

// teardown: teardown_rdimage_tests
void test_opens_image_by_mimetype(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image("test/files/checkerboard","image/x-portable-float-map",&width,&height,NULL,&err);

	assert_false(err);
	assert_non_null(ctx->image);
}

// teardown: teardown_rdimage_tests
void test_opens_image_by_image_reader(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	ctx->image = resdet_open_image_with_reader("test/files/checkerboard","PFM",&width,&height,NULL,&err);

	assert_false(err);
	assert_non_null(ctx->image);
}

void test_open_image_fails_with_wrong_reader(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image_with_reader("test/files/checkerboard","PGM",&width,&height,NULL,&err);

	assert_int_equal(err,RDEINVAL);
	assert_null(image);
	assert_uint_equal(width,0);
	assert_uint_equal(height,0);
}

void test_open_image_by_reader_requires_an_image_reader(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image_with_reader("test/files/checkerboard.pfm",NULL,&width,&height,NULL,&err);

	assert_int_equal(err,RDEPARAM);
	assert_null(image);
	assert_uint_equal(width,0);
	assert_uint_equal(height,0);
}

void test_open_image_errors_on_nonexistent_file(void** state) {
	size_t width, height;
	RDError err;

	RDImage* image = resdet_open_image("test/files/doesntexist.pfm",NULL,&width,&height,NULL,&err);

	assert_true(err < 0);
	assert_null(image);
	assert_uint_equal(width,0);
	assert_uint_equal(height,0);
}

// teardown: teardown_imagebuf_tests
void test_reads_still_frame_image(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height, nimages;

	RDError err = resdet_read_image("test/files/checkerboard.pgm",NULL,&ctx->imagebuf,&nimages,&width,&height);

	assert_false(err);
	assert_non_null(ctx->imagebuf);
	assert_uint_equal(nimages,1);
	assert_uint_equal(width,2);
	assert_uint_equal(height,2);
	assert_array_equal(ctx->imagebuf,((float[]){
		1, 0,
		0, 1,
	}));
}

// teardown: teardown_imagebuf_tests
void test_reads_multi_frame_image(void** state) {
	struct image_ctx* ctx = *state;
	size_t width, height, nimages;

	RDError err = resdet_read_image("test/files/checkerboard.pfm",NULL,&ctx->imagebuf,&nimages,&width,&height);

	assert_false(err);
	assert_non_null(ctx->imagebuf);
	assert_uint_equal(nimages,2);
	assert_uint_equal(width,2);
	assert_uint_equal(height,2);
	assert_array_equal(ctx->imagebuf,((float[]){
		1, 0,
		0, 1,
		0, 1,
		1, 0
	}));
}

// setup: setup_image_tests
// teardown: teardown_image_tests
void test_reads_image_frames(void** state) {
	struct image_ctx* ctx = *state;
	RDError err;
	bool ret;

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_true(ret);
	assert_false(err);
	assert_array_equal(ctx->imagebuf,((float[]){
		1, 0,
		0, 1,
	}));

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);

	assert_true(ret);
	assert_false(err);
	assert_array_equal(ctx->imagebuf,((float[]){
		0, 1,
		1, 0
	}));

	ret = resdet_read_image_frame(ctx->image,ctx->imagebuf,&err);
	assert_false(ret);
	assert_false(err);
}

// setup: setup_image_tests
void test_reading_with_null_rdimage_returns_error(void** state) {
	struct image_ctx* ctx = *state;
	RDError err;

	bool ret = resdet_read_image_frame(NULL,ctx->imagebuf,&err);

	assert_false(ret);
	assert_int_equal(err,RDEPARAM);
}

// setup: setup_image_tests
// teardown: teardown_rdimage_tests
void test_reading_with_null_image_buffer_returns_error(void** state) {
	struct image_ctx* ctx = *state;
	RDError err;

	bool ret = resdet_read_image_frame(ctx->image,NULL,&err);

	assert_false(ret);
	assert_int_equal(err,RDEPARAM);
}
