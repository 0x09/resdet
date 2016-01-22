/*
 * resdet - Detect source resolution of upscaled images.
 * Copyright 2013-2016 0x09.net.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "resdet.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

int sortres(const void* left, const void* right) {
	return ((const RDResolution*)right)->confidence*10000 - ((const RDResolution*)left)->confidence*10000;
}

void usage(const char* self, bool help) {
	printf("Usage: %s -h -m <method> -v <verbosity> image\n",self);
	if(help) {
		printf(
"\n"
" -m   Optional detection method, see below.\n"
" -v   verbosity: -1 - Human-readable output, default.\n"
"                  0 - Behave like a shell test, returning true if upscaling was detected, false if not or on error.\n"
"                  1 - Print only the best guess width and height.\n"
"                  2 - All detected widths and heights in confidence order.\n"
"                  3 - -v2 plus the floating point confidence value.\n"
"\n"
);
		puts("Available detection methods:");
		RDMethod* m = resdet_methods();
		printf("   %s (default)\n",m->name);
		for(m++; m->name; m++)
			printf("   %s\n",m->name);
	}
	exit(0);
}
int main(int argc, char* argv[]) {
	int c;
	int verbosity = -1;
	const char* method = NULL;
	while((c = getopt(argc,argv,"v:m:h")) != -1) {
		switch(c) {
			case 'v': verbosity = strtol(optarg,NULL,10); break;
			case 'm': method = optarg; break;
			case 'h': usage(argv[0],true); break;
			default: usage(argv[0],false);
		}
	}

	const char* input = argv[optind];
	if(!input)
		usage(argv[0],false);

	RDContext* ctx = resdet_open_context();
	if(!ctx) {
		fprintf(stderr,"Context creation failed.\n");
		return 64;
	}
	RDResolution* rw,* rh;
	size_t cw, ch;
	RDError e = resdetect_file(ctx,input,&rw,&cw,&rh,&ch,resdet_get_method(method));
	resdet_close_context(ctx);
	if(e || !verbosity)
		goto end;

	qsort(rw,cw,sizeof(*rw),sortres);
	qsort(rh,ch,sizeof(*rh),sortres);

	if(verbosity == 1) {
		printf("%zu %zu\n",rw[0].index,rh[0].index);
		goto end;
	}
	if(verbosity == 2 || verbosity == 3) {
		for(size_t i = 0; i < cw; i++) {
			if(verbosity == 2)
				printf("%zu ",rw[i].index);
			else
				printf("%zu:%f ",rw[i].index,rw[i].confidence);
		}
		putchar('\n');
		for(size_t i = 0; i < ch; i++) {
			if(verbosity == 2)
				printf("%zu ",rh[i].index);
			else
				printf("%zu:%f ",rh[i].index,rh[i].confidence);
		}
		putchar('\n');
		goto end;
	}

	printf("given: %zux%zu\nbest guess: %zux%zu%s\n",rw[cw-1].index,rh[ch-1].index,rw[0].index,rh[0].index, (cw==1 && ch==1 ? " (not upsampled)" : ""));
	cw--; ch--;
	if(MAX(cw,ch))
		puts("all width        height");
	for(size_t i = 0; i < MAX(cw,ch); i++) {
		if(i < cw)
			printf("%5ld (%5.2f%%)   ",rw[i].index,rw[i].confidence*100);
		else printf("%17s","");
		if(i < ch)
			printf("%5ld (%5.2f%%)",rh[i].index,rh[i].confidence*100);
		putchar('\n');
	}

end:
	free(rw);
	free(rh);
	if(e) {
		fprintf(stderr,"%s\n",RDErrStr[e]);
		return e + 64;
	}
	return !verbosity ? (cw==1 && ch==1) : 0;
}
