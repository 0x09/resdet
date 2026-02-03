#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>

#include <cmocka.h>

int main(int argc, char* argv[]) {
	int c;
	while((c = getopt(argc,argv,"s:")) != -1) {
		switch(c) {
			case 's':
				cmocka_set_skip_filter(argv[2]);
				break;
			default:
				fprintf(stderr,"usage: %s [-s test_pattern] [test_pattern]\n",argv[0]);
				return 1;
		}
	}
	if(argc > optind)
		cmocka_set_test_filter(argv[optind]);

	int ret = 0;

#include "tests_main.c"

	return ret;
}