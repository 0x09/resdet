resdet - Detect source resolution of upscaled images.

# Dependencies

Requires [FFTW](http://fftw.org).  
Image readers are available using libpng, libjpeg, mjpegtools (yuv4mpeg), and/or MagickWand, but these aren't required.

# Building

    ./configure
	make
	make install

Or for the library:

    make install-lib

see `configure --help` for options

# Examples

    $ convert Lenna.png -resize 200% resized.png
    $ resdet resized.png
	given: 1024x1024
	best guess: 512x512
	all width        height
	  512 (68.03%)     512 (67.83%)

# FAQ
This utility takes an image or frame of video which may have been upscaled and attempts to exactly identify the original resolution.

## How?

Traditional resampling methods tend to manifest as an odd extension of a signal's frequency domain combined with a low-pass filter (where the shape of the filter differs among the various resampling methods). Conveniently the Discrete Cosine Transform causes the zero-crossings of this extension to fall at exactly the offset the source was upscaled from. resdet works by trying to identify these inversions.

## Getting the best results

resdet works best on images that are as close to the source as possible. Filtering and compression artifacts can significantly harm the accuracy of this test. In general, clearer and more detailed images will fare better.

### Video
For compressed video, the best results can be gotten by choosing a highly detailed keyframe with a low quantizer. Single-frame yuv4mpeg streams are preferred over png screenshots for videos with chroma subsampling as it preserves the separation of the chroma planes. Some ways to obtain a y4m frame:

FFmpeg/avconv: `ffmpeg -i source -ss timestamp -frames 1 image.y4m`

mpv:  `mpv --start timestamp --frames 1 -o image.y4m source`

Better results should be possible by analyzing multiple frames together; this is on the roadmap.

### Caveats
resdet works well on images resampled with traditional methods, but will not work with newer neural network-based resizers.

If you think something might be upsampled but you aren't getting a good result with resdet, install [spec](https://github.com/0x09/dspfun/tree/master/spec) and have a look at an absolute value spectrum – it's usually possible to identify by sight. Our example earlier looks like this:

![Upscaled Lenna spectrum](http://0x09.net/i/g/Lenna_upscaled.png)

Note the solid black lines indicating zero-crossings 512px in from each dimension.

## Can the same method work for downsampling?

There's no direct equivalent. Since downsampling loses information by definition it's likely not possible. Certain spectral features can appear as a result of typical downsampling operations, but the author is unaware of a way to reliably identify or exploit them in a meaningful way.

## Source?
Looking at too many spectrograms.  
Specifically, this project was born out of a yet-unpublished image deduplication framework, while attempting to identify whether duplicates were scaled versions of one another.  
While [some resources](http://anibin.blogspot.ca) doing similar things via unspecified methods exist, I don't know of any comparable algorithm to resdet or publication describing something like it (but would be glad to read if they exist).