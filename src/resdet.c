/*
 * resdet - Detect source resolution of upscaled images.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "resdet.h"

#ifndef VERSION_SUFFIX
#define VERSION_SUFFIX
#endif

#define RESDET_VERSION_STRING "2.3.1" VERSION_SUFFIX

#define MAX(a,b) ((a) > (b) ? (a) : (b))

void seek_progress(void* ctx, uint64_t offset) {
	fprintf(stderr,"Seeking past frame %" PRIu64 "\r",offset);
}

void usage(const char* self) {
	fprintf(stderr,"Usage: %s [-h -V -m <method> -v <verbosity> -t <filetype> -R <image_reader> -r <range> -x <threshold> -p -o <offset> -n <nframes>] image\n",self);
	exit(1);
}

void help(const char* self) {
	printf("Usage: %s [-h -V -m <method> -v <verbosity> -t <filetype> -r <range> -x <threshold> -p -o <offset> -n <nframes>] image\n"
		" -h   This help text.\n"
		" -V   Show the resdet CLI and library version.\n"
		"\n"
		" -m   Optional detection method, see below.\n"
		" -v   verbosity: -1 - Human-readable output, default.\n"
		"                  0 - Behave like a shell test, returning true if upscaling was detected, false if not or on error.\n"
		"                  1 - Print only the best guess width and height.\n"
		"                  2 - All detected widths and heights in confidence order.\n"
		"                  3 - -v2 plus the floating point confidence value.\n"
		" -t   filetype: Override the input type. May be an extension or MIME type.\n"
		" -R   image reader: Select a specific image reader to read the file. Mutually exclusive with -t.\n"
		"                    Use -R list to see available image readers.\n"
		" -r   range: Number of neighboring values to search (%zu).\n"
		" -x   threshold: Print all detection results above this method-specific confidence level (0-100).\n"
		" -p   Show progress in number of frames analyzed so far.\n"
		" -o   offset: Seek to this frame number before starting detection.\n"
		" -n   nframes: Limit detection to this number of frames.\n"
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
	const char* method = NULL,* type = NULL,* image_reader = NULL;
	const char* range_opt = NULL,* threshold_opt = NULL;
	uint64_t offset = 0, nframes = 0;
	bool progress = false;
	char* endptr;
	while((c = getopt(argc,argv,"v:m:t:x:r:pn:o:R:hV")) != -1) {
		switch(c) {
			case 'v': verbosity = strtol(optarg,NULL,10); break;
			case 'm': method = optarg; break;
			case 't': type = optarg; break;
			case 'R':
				if(!strcmp(optarg,"list")) {
					for(const char* const* image_reader = resdet_list_image_readers(); *image_reader; image_reader++)
						puts(*image_reader);
					return 0;
				}
				image_reader = optarg;
				break;
			case 'x': threshold_opt = optarg; break;
			case 'r': range_opt = optarg; break;
			case 'o':
				offset = strtoull(optarg,&endptr,10);
				if(optarg == endptr) {
					fprintf(stderr,"Invalid offset %s\n",optarg);
					return 1;
				}
				break;
			case 'n':
				nframes = strtoull(optarg,&endptr,10);
				if(!nframes || optarg == endptr) {
					fprintf(stderr,"Invalid nframes %s\n",optarg);
					return 1;
				}
				break;
			case 'p': progress = true; break;
			case 'h': help(argv[0]); break;
			case 'V':
				printf("resdet version %s\nlibresdet version %s\nBuilt with image readers: ",RESDET_VERSION_STRING,resdet_libversion());
				for(const char* const* image_reader = resdet_list_image_readers(); *image_reader; image_reader++)
					printf("%s%s",*image_reader,*(image_reader+1) ? ", " : "\n");
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

	RDParameters* params = resdet_alloc_default_parameters();
	if(!params) {
		fputs("Out of memory",stderr);
		return 1;
	}

	if(threshold_opt) {
		float threshold = strtod(threshold_opt,&endptr)/100;
		if(threshold_opt == endptr || resdet_parameters_set_threshold(params,threshold)) {
			fprintf(stderr,"Invalid threshold value %s\n",threshold_opt);
			free(params);
			return 1;
		}
	}
	if(range_opt) {
		size_t range = strtol(range_opt,&endptr,10);
		if(range_opt == endptr || resdet_parameters_set_range(params,range)) {
			fprintf(stderr,"Invalid range value %s\n",range_opt);
			free(params);
			return 1;
		}
	}
	if(type && image_reader) {
		fputs("Type option (-t) cannot be used with an image reader (-R)",stderr);
		return 1;
	}

	RDResolution* rw = NULL,* rh = NULL;
	size_t cw, ch;

	size_t width, height;
	float* image;
	RDError e;
	int ret = 0;
	RDAnalysis* analysis = NULL;
	RDImage* rdimage;
	if(image_reader)
		rdimage = resdet_open_image_with_reader(input,image_reader,&width,&height,&image,&e);
	else
		rdimage = resdet_open_image(input,type,&width,&height,&image,&e);
	if(e)
		goto end;

	if(offset) {
		if(!resdet_seek_frame(rdimage,offset,progress ? seek_progress : NULL,NULL,&e)) {
			if(!e) {
				fprintf(stderr,"Passed end of file while seeking to frame %" PRIu64 "\n",offset);
				ret = 1;
				goto end;
			}
			goto end;
		}
		if(progress)
			fputs("\n",stderr);
	}

	analysis = resdet_create_analysis(m,width,height,params,&e);
	if(e)
		goto end;

	size_t ct = 1;
	while(resdet_read_image_frame(rdimage,image,&e)) {
		if(progress)
			fprintf(stderr,"Analyzing frame %" PRIu64 "\r",ct+offset);
		if((e = resdet_analyze_image(analysis,image)))
			break;
		ct++;
		if(nframes && ct > nframes)
			break;
	}

	if(!e && ct == 1) {
		fprintf(stderr,"No frames left in input.\n");
		ret = 1;
		goto end;
	}

	if(progress)
		fputs("\n",stderr);

	if(e)
		goto end;

	e = resdet_analysis_results(analysis,&rw,&cw,&rh,&ch);

	if(e || !verbosity)
		goto end;

	if(verbosity == 1) {
		printf("%zu %zu\n",rw[0].index,rh[0].index);
		goto end;
	}
	if(verbosity == 2 || verbosity == 3) {
		for(size_t i = 0; i < cw; i++) {
			printf("%zu",rw[i].index);
			if(verbosity == 3)
				printf(":%f",rw[i].confidence);
			putchar(' ');
		}
		putchar('\n');
		for(size_t i = 0; i < ch; i++) {
			printf("%zu",rh[i].index);
			if(verbosity == 3)
				printf(":%f",rh[i].confidence);
			putchar(' ');
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
			printf("%5zu (%5.2f%%)   ",rw[i].index,rw[i].confidence*100);
		else printf("%17s","");
		if(i < ch)
			printf("%5zu (%5.2f%%)",rh[i].index,rh[i].confidence*100);
		putchar('\n');
	}

end:
	resdet_destroy_analysis(analysis);
	resdet_close_image(rdimage);
	free(image);
	free(params);
	free(rw);
	free(rh);
	if(e) {
		fprintf(stderr,"%s\n",resdet_error_str(e));
		ret = !!e;
	}
	if(ret)
		return ret;

	return !verbosity ? (cw==1 && ch==1) : 0;
}
