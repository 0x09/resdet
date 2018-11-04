libresdet is a small library for analyzing potential original resolutions in an image.

# Functions

```C
	RDErrStr[]
```

Most library functions return `RDError` to indicate any failure. While not actually a function, use `RDErrStr[error]` to map an `RDError` to a descriptive string.

---

```C
	RDMethod* resdet_methods();
```

Returns list of available methods, terminated by an empty RDMethod. The first element can be assumed to be the library default. Can be iterated using

```C
	for(RDMethod* m = resdet_methods(); m->name; m++)
		...;
```

---

```C
	RDMethod* resdet_get_method(const char* name);
```

Lookup a method by name.

* name - The name of a method, or NULL for the default method.

Returns NULL if no name matches.

---

```C
	size_t resdet_default_range(void);
```

Returns the library default search range, as used when not explicitly specifying the range via the `redetect_with_params`.

Most detection methods use this value to determine how many neighboring values to consider when detecting a given resolution.

Higher values are more accurate up to a point, while lower values are faster.

---

```C
	RDError resdetect_file(const char* filename, RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth, RDMethod* method);
```

* filename - Path of the image, or "-" for standard input.
* mimetype - Optional MIME type of the image, for choosing an image reader. If NULL the file's extension will be used.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.

---

```C
	RDError resdetect_file_with_params(const char* filename,
	                                   RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
	                                   RDMethod* method, size_t range, float threshold);
```

Detect with specified parameters.

* filename - Path of the image, or "-" for standard input.
* mimetype - Optional MIME type of the image, for choosing an image reader. If NULL the file's extension will be used.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.
* range - Range of coefficients to consider when looking for inversions. Lower values are faster, but may return many more misidentified results. The default is currently 12, with reasonable values between 8-32 (DEFAULT_RANGE).
* threshold - Method-specific value (RDMethod->threshold) under which detected resolutions won't be considered meaningful. A value of 0 will return an RDResolution result for every single line/column.

---

```C
	RDError resdet_read_image(const char* filename, const char* mimetype, unsigned char** image, size_t* nimages, size_t* width, size_t* height);
```

Read an image using whatever image loaders the library was built with.

* filename - Path of the image, or "-" for standard input.
* mimetype - Optional MIME type of the image, for choosing an image reader. If NULL the file's extension will be used.
* image - Out parameter containing the 8-bit grayscale bitmap(s). Multiple images (i.e. y4m, gif) are simply contiguous such that image 2 begins at the address of image + width * height. Allocated by the library, must be freed by caller.
* nimages - Out parameter containing the number of images returned.
* width, height - Out parameters containing the bitmap dimensions.

---

```C
	RDError resdetect(unsigned char* restrict image, size_t nimages, size_t width, size_t height,
	                   RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
	                   RDMethod* method);
```

Detect from a bitmap or series of bitmaps directly.

* image - 8-bit grayscale bitmap.
* nimages - Number of contiguous images in the buffer. Must be at least 1.
* width, height - Dimensions of the bitmap.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.

---

```C
	RDError resdetect_with_params(unsigned char* restrict image, size_t nimages, size_t width, size_t height,
	                              RDResolution** resw, size_t* countw, RDResolution** resh, size_t* counth,
	                              RDMethod* method, size_t range, float threshold);
```

Detect from a bitmap directly with specified parameters.

* image - 8-bit grayscale bitmap.
* nimages - Number of contiguous images in the buffer. Must be at least 1.
* width, height - Dimensions of the bitmap.
* resw, resh - Output arrays of pixel index and confidence pairs describing a potential detected resolution. Either may be NULL to skip analyzing that dimension. If provided, respective count param must point to valid size_t memory. Guaranteed to be either allocated or nulled by the library, must be freed by caller.
* countw, counth - Size of resw and resh respectively.
* method - A detection method returned by `resdet_methods` or `resdet_get_method`.
* range - Range of coefficients to consider when looking for inversions. Lower values are faster, but may return many more misidentified results. The default is currently 12, with reasonable values between 8-32 (DEFAULT_RANGE).
* threshold - Method-specific value (RDMethod->threshold) under which detected resolutions won't be considered meaningful. A value of 0 will return an RDResolution result for every single line/column.

# Example

Detect potential widths in a file, using the defaults:

```C
	const char* filename = ...;
	
	RDResolution* rw;
	size_t cw;
	RDError e = resdetect_file(filename, NULL, &rw, &cw, NULL, NULL, resdet_get_method(NULL));
	if(e)
		//handle error
	
	for(size_t i = 0; i < cw; i++)
		printf("%zu\n", rw[i].index);
```

# Memory Requirements
libresdet's peak requirements when built with FFTW support (including the primary image buffer) can be calculated by:

	width * height * nimages +
	max(width, height) * COEFF_PRECISION +
	width  * sizeof(double) +
	height * sizeof(double) +
	width  * sizeof(RDResolution) +
	height * sizeof(RDResolution)

The KISS FFT transform requirements are more slightly more complex.

The above requirements may be limited indirectly by setting the PIXEL_MAX macro, which restricts the product of width, height, and nimages for valid images (SIZE_MAX by default).

Note that image libraries used by `resdet_read_image` will have their own separate requirements not included in this calculation.

Note also that at the time of this writing, multiple image formats like y4m are read into the buffer in their entirety. Therefore `resdetect_file` is not suitable for files with a large number of frames.

# Thread Safety
libresdet's own routines are thread safe, but some of its optional supporting libraries rely on global state. As libresdet does not mandate a threading model itself, it cannot enforce their safe execution in a multithreaded app.  
If your application will make calls to resdet from concurrent threads while one of these are enabled, your application must independently prepare these libraries for threaded use at the start of execution.

These measures are only necessary when both:

1. resdet is built with either FFTW or ImageMagick support, and
2. resdet routines will be called concurrently.

In all other cases no extra preparation is needed.

Steps to prepare each of these for threaded execution are:

* FFTW
 * Include `fftw.h`
 * Call [`fftw_make_planner_thread_safe()`](http://www.fftw.org/fftw3_doc/Thread-safety.html#Thread-safety)
 * Link with a threaded version of the FFTW library
 * Link with the system's threading library if necessary
* MagickWand
 * Include `MagickWand.h`
 * Call [`MagickWandGenesis()`](https://www.imagemagick.org/api/magick-wand.php#MagickWandGenesis)
