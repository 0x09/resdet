libresdet is a small library for analyzing potential original resolutions in an image.

# Functions

	RDErrStr[]

Most library functions return `RDError` to indicate any failure. While not actually a function, use `RDErrStr[error]` to map an `RDError` to a descriptive string.

---

	RDContext* resdet_open_context();

Open a new context -- currently only needed if compiled with libmagic support and using resdetect_file\*.

---

	void resdet_close_context(RDContext*);

Close and free a context. If not using a context, `resdet_close_context(NULL)` can still be used to cleanup FFTW.

---

	RDMethod* resdet_methods();

Returns list of available methods, terminated by an empty RDMethod. The first element can be assumed to be the library default. Can be iterated using

	for(RDMethod* m = resdet_methods(); m->name; m++)
		...;

---

	RDMethod* resdet_get_method(const char* name);

Lookup a method by name.

* name - The name of a method, or NULL for the default method.

Returns NULL if no name matches.

---
	RDError resdetect_file(RDContext* ctx, const char* filename, RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth, RDMethod* method);

* ctx - The context returned by `resdet_open_context`.
* filename - Path of the image.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.

---

	RDError resdetect_file_with_params(RDContext* ctx, const char* filename,
	                                   RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
	                                   RDMethod* method, size_t range, float threshold);

Detect with specified parameters.

* ctx - The context returned by `resdet_open_context`.
* filename - Path of the image.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.
* range - Range of coefficients to consider when looking for inversions. Lower values are faster, but may return many more misidentified results. The default is currently 12, with reasonable values between 8-32 (DEFAULT_RANGE).
* threshold - Method-specific value (RDMethod->threshold) under which detected resolutions won't be considered meaningful. A value of 0 will return an RDResolution result for every single line/column.

---

	RDError resdet_read_image(RDContext* ctx, const char* filename, unsigned char** image, size_t* width, size_t* height);

Read an image using whatever image loaders the library was built with.

* ctx - The context returned by `resdet_open_context`.
* filename - Path of the image.
* image - Out parameter containing the 8-bit planar grayscale bitmap. Allocated by the library, must be freed by caller.
* width, height - Out parameters containing the bitmap dimensions.

---

	RDError resdetect(unsigned char* restrict image, size_t width, size_t height,
	                   RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
	                   RDMethod* method);

Detect from a bitmap directly.

* image - 8-bit grayscale bitmap with no padding.
* width, height - dimensions of the bitmap.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.

---

	RDError resdetect_with_params(unsigned char* restrict image, size_t width, size_t height,
	                              RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
	                              RDMethod* method, size_t range, float threshold);

Detect from a bitmap directly with specified parameters.

* image - 8-bit grayscale bitmap with no padding.
* width, height - dimensions of the bitmap.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.
* range - Range of coefficients to consider when looking for inversions. Lower values are faster, but may return many more misidentified results. The default is currently 12, with reasonable values between 8-32 (DEFAULT_RANGE).
* threshold - Method-specific value (RDMethod->threshold) under which detected resolutions won't be considered meaningful. A value of 0 will return an RDResolution result for every single line/column.

# Example

Detect potential widths in a file, using the defaults:

	const char* filename = ...;
	
	RDContext* ctx = resdet_open_context();
	if(!ctx)
		//handle error
	RDResolution* rw;
	size_t cw;
	RDError e = resdetect_file(ctx, filename, &rw, &cw, NULL, NULL, resdet_get_method(NULL));
	resdet_close_context(ctx);
	if(e)
		//handle error
	
	for(size_t i = 0; i < cw; i++)
		printf("%zu\n", rw[i].index);
