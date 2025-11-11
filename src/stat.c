/*
 * stat - Analyze detection results.
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

#include "resdet.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

void statres(RDResolution* rw, size_t cw) {
	float median = (rw[cw/2].confidence + rw[(cw+1)/2].confidence) / 2;
	float lq = rw[cw/4].confidence*(cw/4.0-cw/4) + rw[(cw+1)/4].confidence*(1-(cw/4.0-cw/4));
	float hq = rw[cw*3/4].confidence*(cw*3/4.0-cw*3/4) + rw[(cw+1)*3/4].confidence*(1-(cw*3/4.0-cw*3/4));
	float iq = hq-lq;
	float inner = 3*iq+hq;
	float outer = 6*iq+hq;
	printf("median: %5.2f%%   inner fence: %5.2f%%   outer fence: %5.2f%%\n",median*100,inner*100,outer*100);
	int i;
	for(i = 0; i < cw && rw[i].confidence < inner; i++)
		;
	puts("mild outliers:");
	for(; i < cw && rw[i].confidence < outer; i++)
		printf("\t%4ld (%5.2f%%) - %5.2f%%\n",rw[i].index,rw[i].confidence*100,(rw[i].confidence-median)/(1-median)*100);
	puts("extreme outliers:");
	for(; i < cw; i++)
		printf("\t%4ld (%5.2f%%) - %5.2f%%\n",rw[i].index,rw[i].confidence*100,(rw[i].confidence-median)/(1-median)*100);
}
int main(int argc, char* argv[]) {
	if(argc < 2) {
		fprintf(stderr,"Usage: %s image <method>\n",argv[0]);
		fprintf(stderr,"Methods:\n");
		RDMethod* m = resdet_methods();
		fprintf(stderr,"\t%s (default)\n",m->name);
		for(m++; m->name; m++)
			fprintf(stderr,"\t%s\n",m->name);
		return 1;
	}

	RDResolution* rw,* rh;
	size_t cw, ch;
	RDError e = resdetect_file(argv[1],NULL,&rw,&cw,&rh,&ch,resdet_get_method(argv[2]));
	if(e) goto end;
	puts("width:");
	statres(rw,cw-1);
	puts("height:");
	statres(rh,ch-1);
end:
	free(rw);
	free(rh);
	if(e) fprintf(stderr,"%s\n",resdet_error_str(e));
	return e;
}
