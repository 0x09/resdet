/*
 * stat - Analyze detection results.
 */

#include <stdio.h>
#include <stdlib.h>

#include "resdet.h"

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
