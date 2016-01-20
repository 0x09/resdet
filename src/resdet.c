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

#include "resdet.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

int sortres(const void* left, const void* right) {
	return ((const RDResolution*)right)->confidence*10000 - ((const RDResolution*)left)->confidence*10000;
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: %s image <method>\n",argv[0]);
		puts("Methods:");
		RDMethod* m = res_detect_methods();
		printf("\t%s (default)\n",m->name);
		for(m++; m->name; m++)
			printf("\t%s\n",m->name);
		return 0;
	}

	RDContext* ctx = resdet_open_context();
	if(!ctx) {
		fprintf(stderr,"Context creation failed.\n");
		return 1;
	}
	RDResolution* rw,* rh;
	size_t cw, ch;
	RDError e = res_detect_file(ctx,argv[1],&rw,&cw,&rh,&ch,res_detect_get_method(argv[2]));
	resdet_close_context(ctx);
	if(e) goto end;

	qsort(rw,cw,sizeof(*rw),sortres);
	qsort(rh,ch,sizeof(*rh),sortres);

	printf("given: %zux%zu\n",rw[cw-1].index,rh[ch-1].index);

	printf("best guess: %zux%zu",rw[0].index,rh[0].index);
	if(cw + ch == 2) printf(" (not upsampled)");
	putchar('\n');

	cw--; ch--;
	if(MAX(cw,ch))
		puts("all width        height");
	for(int i = 0; i < MAX(cw,ch); i++) {
		if(i < cw)
			printf("%5ld (%5.2f%%)   ",rw[i].index,rw[i].confidence*100);
		else printf("                 ");
		if(i < ch)
			printf("%5ld (%5.2f%%)",rh[i].index,rh[i].confidence*100);
		putchar('\n');
	}
end:
	free(rw);
	free(rh);
	if(e) fprintf(stderr,"%s\n",RDErrStr[e]);
	return e;
}
