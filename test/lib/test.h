#ifndef TEST_H
#define TEST_H

#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>
#include <cmocka.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "resdet.h"

// cmocka v1 compatibility
#ifndef assert_uint_equal
#define assert_uint_equal(a,b) assert_int_equal(a,b)
#include <math.h>
#define assert_float_in_range(a,min,max,epsilon) assert_true(fabsf(a-min) >= epsilon && fabsf(max-a) >= epsilon)
#endif

#define assert_array_equal(a,b) assert_memory_equal(a,b,sizeof(b))

static void run_sample_image_assertions(RDResolution* resw, RDResolution* resh, size_t countw, size_t counth, size_t detected_widths, size_t detected_heights) {
	assert_non_null(resw);
	assert_non_null(resh);

	assert_uint_equal(countw,detected_widths);
	assert_uint_equal(counth,detected_heights);

	assert_uint_equal(resw[0].index,512);
	assert_uint_equal(resh[0].index,512);

	assert_float_in_range(resw[0].confidence,0.99,1.0,0);
	assert_float_in_range(resh[0].confidence,0.99,1.0,0);

	assert_uint_equal(resw[countw-1].index,768);
	assert_uint_equal(resh[counth-1].index,768);

	assert_float_equal(resw[countw-1].confidence,-1,0);
	assert_float_equal(resh[counth-1].confidence,-1,0);
}

#endif