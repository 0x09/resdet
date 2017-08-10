resdet - Detect source resolution of upscaled images.

# Dependencies

resdet bundles [KISS FFT](http://kissfft.sourceforge.net) as its FFT implementation (permissively licensed) but can use [FFTW](http://fftw.org) (GPL, potentially faster) if available.  
Image loaders are available using any or all of libpng, libjpeg, mjpegtools (for yuv4mpeg), and MagickWand.  
  
libresdet can be used completely standalone provided the client supplies the image data.

# Building
The tools and supplied build scripts target a generic *nix environment with bash, pkg-config, GNU make, and a GCC compatible C compiler using an autotools-like build process:

    ./configure
	make
	make install

see `configure --help` for options

# Examples

Resize an image with [ImageMagick](http://www.imagemagick.org)'s `convert` command and detect it:

    $ convert Lenna.png -resize 150% resized.png

    $ resdet resized.png
    given: 768x768
    best guess: 512x512
    all width        height
      512 (70.12%)     512 (68.89%)

Just test if an image has been upscaled:

	$ resdet -v0 resized.png && echo upscaled
	upscaled

See resdet -h for more options.

# API

resdet can be used as a library by including [resdet.h](resdet.h) and linking with libresdet. To install the library, use `make install-lib` after building.  
A pkg-config file is included; linker flags can be gotten via `pkg-config --libs --static resdet`  
API documentation is available in the [doc](doc) subdirectory.  
Though the supplied build script is meant for Unix-likes, libresdet is written in standard C and should be usable on any platform with a C99 libc.

# FAQ
This utility takes an image or frames of video which may have been upscaled and attempts to exactly identify the original resolution.

## How?

Traditional resampling methods tend to manifest as an odd extension of a signal's frequency domain combined with a low-pass filter (where the shape of the filter differs among the various resampling methods). Conveniently the Discrete Cosine Transform causes the zero-crossings of this extension to fall at exactly the offset the source was upscaled from. resdet works by trying to identify these inversions.

## Getting the best results

resdet works best on images that are as close to the source as possible. Filtering and compression artifacts can significantly harm the accuracy of this test. In general, clearer and more detailed images will fare better.

### Video
For compressed video stills, the best results can be gotten by choosing a highly detailed keyframe with a low quantizer. Single-frame yuv4mpeg streams are preferred over png screenshots for videos with chroma subsampling as it preserves the separation of the chroma planes. Some ways to obtain a y4m frame:

FFmpeg/avconv: `ffmpeg -i source -ss timestamp -vframes 1 -pix_fmt yuv420p image.y4m`

mpv:  `mpv --start timestamp --frames 1 --vf format=yuv420p -o image.y4m source`

Better results should be possible by analyzing multiple frames together. resdet supports this with both the mjpegtools (y4m) and MagickWand image loaders. To obtain multiple frames in the examples above, simply replace the argument to `-vframes` for FFmpeg or `--frames` for mpv with the desired number of frames. Note that currently frames will be read in bulk, so choose only a small section of the video to avoid consuming too much memory. This is not an inherent limitation, and will likely change.

### JPEG
Moderate to heavily compressed JPEG sources tend to produce false positives at multiples of 1/8 the input resolution, generally with more appearing and in higher ranks as the quality decreases. resdet currently doesn't filter/penalize such results, although this can be mitigated somewhat by applying a deblocking filter to the image before analysis.

Deblocking example with FFmpeg: `ffmpeg -i source.jpg -vf pp=ha/va image.png`

### Caveats
resdet works well on images resampled with traditional methods, but will not work with newer neural network-based resizers.

If you think something might be upsampled but you aren't getting a good result with resdet, install [spec](https://github.com/0x09/dspfun/tree/master/spec) and have a look at an absolute value spectrum – it's usually possible to identify by sight. Our example earlier looks like this:

![Upscaled Lenna spectrum](http://0x09.net/i/g/Lenna_upscaled_150.png)

Note the solid black lines indicating zero-crossings 512px in from each dimension.

## Can the same method work for downsampling?

There's no direct equivalent. Since downsampling loses information by definition it's likely not possible. Certain spectral features can appear as a result of typical downsampling operations, but the author is unaware of a way to reliably identify or exploit them in a meaningful way.

## Source?
Looking at too many spectrograms.  
Specifically, this project was born out of a yet-unpublished image deduplication framework, while attempting to identify whether duplicates were scaled versions of one another.  
While [some resources](http://anibin.blogspot.ca) doing similar things via unspecified methods exist, I don't know of any comparable algorithm to resdet or publication describing something like it (but would be glad to read if they exist).
