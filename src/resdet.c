/*
 * resdet - Detect source resolution of upscaled images.
 * Copyright (C) 2013-2017 0x09.net.
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

#define RESDET_VERSION_STRING "1.0.1"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

int sortres(const void* left, const void* right) {
	return ((const RDResolution*)right)->confidence*10000 - ((const RDResolution*)left)->confidence*10000;
}

void usage(const char* self) {
	fprintf(stderr,"Usage: %s [-h -V -m <method> -v <verbosity> -t <mimetype> -r <range> -x <threshold>] image\n",self);
	exit(1);
}

void help(const char* self) {
	printf("Usage: %s [-h -V -m <method> -v <verbosity> -t <mimetype> -r <range> -x <threshold>] image\n"
		" -h   This help text.\n"
		" -V   Show the resdet CLI and library version.\n"
		"\n"
		" -m   Optional detection method, see below.\n"
		" -v   verbosity: -1 - Human-readable output, default.\n"
		"                  0 - Behave like a shell test, returning true if upscaling was detected, false if not or on error.\n"
		"                  1 - Print only the best guess width and height.\n"
		"                  2 - All detected widths and heights in confidence order.\n"
		"                  3 - -v2 plus the floating point confidence value.\n"
		" -t   mimetype: Override the input type.\n"
		" -r   range: Number of neighboring values to search (%zu)\n"
		" -x   threshold: Print all detection results above this method-specific confidence level (0-100)\n"
		"\n",
		self,
		resdet_default_range()
	);
	puts("Available detection methods and default thresholds:");
	RDMethod* m = resdet_methods();
	printf("   %-5s - %.0f%% (default method)\n",m->name,m->threshold*100);
	for(m++; m->name; m++)
		printf("   %-5s - %.0f%%\n",m->name,m->threshold*100);
	exit(0);
}
int main(int argc, char* argv[]) {
	int c;
	int verbosity = -1;
	const char* method = NULL,* type = NULL;
	double threshold = -1;
	size_t range = 0;
	while((c = getopt(argc,argv,"v:m:t:x:r:hV")) != -1) {
		switch(c) {
			case 'v': verbosity = strtol(optarg,NULL,10); break;
			case 'm': method = optarg; break;
			case 't': type = optarg; break;
			case 'x': threshold = strtod(optarg,NULL)/100; break;
			case 'r': range = strtol(optarg,NULL,10); break;
			case 'h': help(argv[0]); break;
			case 'V':
				printf("resdet version %s\nlibresdet version %s\n",RESDET_VERSION_STRING,resdet_libversion());
				return 0;
			default: usage(argv[0]);
		}
	}

	const char* input = argv[optind];
	if(!input)
		usage(argv[0]);

	RDMethod* m = resdet_get_method(method);
	if(!m) {
		fprintf(stderr,"Invalid method \"%s\"\nAvailable methods:\n", method);
		for(m = resdet_methods(); m->name; m++)
			fprintf(stderr,"%s\n",m->name);
		return 1;
	}

	RDResolution* rw,* rh;
	size_t cw, ch;

	if(threshold < 0)
		threshold = m->threshold;
	if(!range)
		range = resdet_default_range();

	RDError e = resdetect_file_with_params(input,type,&rw,&cw,&rh,&ch,m,range,threshold);
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
