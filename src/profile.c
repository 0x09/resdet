/*
 * profile - Profile and check accuracy of resdet methods.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include <errno.h>

#include "resdet.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct timeval diffts(struct timeval start, struct timeval end) {
	return end.tv_usec-start.tv_usec < 0 ? (struct timeval){
		.tv_sec = end.tv_sec-start.tv_sec-1,
		.tv_usec = 1000000+end.tv_usec-start.tv_usec
	} : (struct timeval){
		.tv_sec = end.tv_sec-start.tv_sec,
		.tv_usec = end.tv_usec-start.tv_usec
	};
}


void diffres(size_t* left, size_t leftlen, RDResolution* right, size_t rightlen, size_t* plus, size_t* minus) {
	int l = 0, r = 0;
	while(l < leftlen && r < rightlen)
		if(left[l] > right[r].index) {
			(*plus)++;
			r++;
		}
		else if(left[l] < right[r].index) {
			(*minus)++;
			l++;
		}
		else {
			l++;
			r++;
		}

	*plus += rightlen - r;
	*minus += leftlen - l;
}

void readres(char* line, size_t** ar, size_t* ct) {
	*ar = NULL; *ct = 0;
	char* token;
	if(*line) while((token = strsep(&line," "))) {
		*ar = realloc(*ar,sizeof(**ar)*++(*ct));
		(*ar)[*ct-1] = strtoull(token,NULL,10);
	}
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		fprintf(stderr,
"Usage: %s dict.txt\n"
"\n"
"dict.txt provides a set of images with known resolutions with the format\n"
"\tfilename\n"
"\twidth1 width2 ...\n"
"\theight1 height2 ...\n"
"\n"
"\tfilename\n"
"\t...\n"
"\n"
"one-line example: printf \"resized.png\\n512\\n512\\n\" | %s /dev/stdin\n"
		,argv[0],argv[0]);
		return 1;
	}

	RDMethod* methods = resdet_methods();
	size_t nmethods = 0;
	int padding = 0;
	for(RDMethod* m = methods; m->name; m++) {
		nmethods++;
		if(strlen(m->name) > padding)
			padding = strlen(m->name);
	}
	struct timeval tvs[nmethods];
	memset(tvs,0,sizeof(tvs));
	size_t diffplus[nmethods];
	memset(diffplus,0,sizeof(diffplus));
	size_t diffminus[nmethods];
	memset(diffminus,0,sizeof(diffminus));

	FILE* dict = fopen(argv[1],"r");
	if(!dict) {
		fprintf(stderr,"Error opening dictionary file: %s\n",strerror(errno));
		return 1;
	}
	int ret = 0;
	ssize_t len;
	size_t len2;
	char* line = NULL;
	while(!ret && (len = getline(&line,&len2,dict)) > 0) {
		if(*line == '\n')
			continue;
		line[len-1] = '\0';
		puts(line);
		float* image;
		size_t w, h, d;
		RDError e = resdet_read_image(line,NULL,&image,&d,&w,&h);
		if(e) {
			fprintf(stderr, "Error reading %s: %s\n",line,resdet_error_str(e));
			// skip over this image's resolution lists
			if(getline(&line,&len2,dict) <= 0 ||
			   getline(&line,&len2,dict) <= 0) {
				ret = 1;
				break;
			}
			continue;
		}

		size_t* knownw = NULL,* knownh  = NULL;
		size_t knownwct, knownhct;
		if((len = getline(&line,&len2,dict)) <= 0) {
			ret = 1;
			goto read_end;
		}
		line[len-1] = '\0';
		readres(line,&knownw,&knownwct);

		if((len = getline(&line,&len2,dict)) <= 0) {
			ret = 1;
			goto read_end;
		}
		line[len-1] = '\0';
		readres(line,&knownh,&knownhct);

		for(RDMethod* m = methods; m->name; m++) {
			RDResolution* rw = NULL,* rh = NULL;
			size_t cw = 0, ch = 0;
			struct rusage rusage;
			getrusage(RUSAGE_SELF,&rusage);
			struct timeval before = rusage.ru_utime;
			if((e = resdetect(image,d,w,h,&rw,&cw,&rh,&ch,m))) {
				fprintf(stderr, "Error during detection: %s\n",resdet_error_str(e));
				ret = 1;
				goto detect_end;
			}
			getrusage(RUSAGE_SELF,&rusage);
			struct timeval after = rusage.ru_utime;

			after = diffts(before,after);
			tvs[m-methods].tv_sec += after.tv_sec + (tvs[m-methods].tv_usec + after.tv_usec) / 1000000;
			tvs[m-methods].tv_usec = (tvs[m-methods].tv_usec + after.tv_usec) % 1000000;

			size_t plus = 0, minus = 0;
			diffres(knownw,knownwct,rw,cw-1,&plus,&minus);
			diffres(knownh,knownhct,rh,ch-1,&plus,&minus);
			diffplus[m-methods] += plus;
			diffminus[m-methods] += minus;

			printf("%-*s   %2ld.%.9ld   (+%zu -%zu)\n",padding,m->name,after.tv_sec,(long)after.tv_usec,plus,minus);
detect_end:
			free(rw);
			free(rh);
		}
read_end:
		free(knownw);
		free(knownh);
		free(image);
	}
	free(line);
	fclose(dict);

	puts("totals");
	for(RDMethod* m = methods; m->name; m++)
		printf("%-*s   %2ld.%.9ld   (+%zu -%zu)\n",padding,m->name,tvs[m-methods].tv_sec,(long)tvs[m-methods].tv_usec,diffplus[m-methods],diffminus[m-methods]);
	return ret;
}
