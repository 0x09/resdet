#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

int main(int argc, char* argv[]) {
	int ret = 0;

#include "tests_main.c"

	return ret;
}