#include "test.h"

void test_resdet_libversion(void** state) {
	assert_string_equal(resdet_libversion(),RESDET_LIBVERSION);
}

void test_resdet_error_str_returns_error_message(void** state) {
	const char* error = resdet_error_str(RDENOMEM);

	assert_string_equal(error,"Out of memory");
}

void test_resdet_error_str_returns_errno_message(void** state) {
	const char* error = resdet_error_str(-ERANGE);
	const char* actual_erange = strerror(ERANGE);

	assert_non_null(error);
	assert_string_equal(error,actual_erange);
}

void test_resdet_error_str_returns_null_on_unknown_error(void** state) {
	enum RDErrors max_error = resdet_get_max_error();
	const char* error = resdet_error_str(max_error+1);

	assert_null(error);
}

void test_all_rderror_values_return_an_error_message(void** state) {
	enum RDErrors max_error = resdet_get_max_error();

	for(enum RDErrors e = 0; e <= max_error; e++)
		assert_non_null(resdet_error_str(e));
}

void test_methods_return_sign_by_default(void** state) {
	RDMethod* method = resdet_methods();

	assert_string_equal(method->name,"sign");
}

void test_get_method_returns_sign_method(void** state) {
	RDMethod* method = resdet_get_method("sign");

	assert_string_equal(method->name,"sign");
}

void test_returns_default_range(void** state) {
	size_t range = resdet_default_range();

	assert_true(range);
}

int setup_rdparameter_tests(void** state) {
	if(!(*state = resdet_alloc_default_parameters()))
		return 1;
	return 0;
}

int teardown_rdparameter_tests(void** state) {
	free(*state);
	return 0;
}

// setup: setup_rdparameter_tests
// teardown: teardown_rdparameter_tests
void test_sets_range(void** state) {
	int err = resdet_parameters_set_range(*state,10);

	assert_false(err);
}

// setup: setup_rdparameter_tests
// teardown: teardown_rdparameter_tests
void test_sets_threshold(void** state) {
	int err = resdet_parameters_set_threshold(*state,0);

	assert_false(err);
}

// setup: setup_rdparameter_tests
// teardown: teardown_rdparameter_tests
void test_zero_range_returns_error(void** state) {
	int err = resdet_parameters_set_range(*state,0);

	assert_int_equal(err,RDEPARAM);
}

// setup: setup_rdparameter_tests
// teardown: teardown_rdparameter_tests
void test_negative_threshold_returns_error(void** state) {
	int err = resdet_parameters_set_threshold(*state,-1);

	assert_int_equal(err,RDEPARAM);
}

void test_setting_threshold_with_no_params_returns_error(void** state) {
	int err = resdet_parameters_set_threshold(NULL,0);

	assert_int_equal(err,RDEPARAM);
}

void test_setting_range_with_no_params_returns_error(void** state) {
	int err = resdet_parameters_set_range(NULL,10);

	assert_int_equal(err,RDEPARAM);
}
