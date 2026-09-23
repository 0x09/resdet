[Emscripten](https://emscripten.org/)-based JavaScript bindings for libresdet.

The bindings are written in TypeScript and compiled to a JavaScript module by the build system.

Note that these bindings only cover the subset of the resdet API  required to build the web version of resdet at https://0x09.net/resdet. Specifically, there is no image-reading functionality. Clients are expected to provide decoded image data as a (grayscale) `Float32Array`.

# Building
```shell
./configure --cc=emcc --ar=emar --disable-everything --omit-pgm-reader --omit-pfm-reader --omit-y4m-reader
make resdet.mjs
```


# Example
```JavaScript
import * as resdet from './resdet.mjs'

const image = // the image data as a Float32Array
const width = // the image width
const height = // the image height

let resolutions;
try {
    resolutions = resdet.resDetect(image,1,width,height)
}
catch (error) {
    //handle error
}

const widths = resolutions['widths'];
const heights = resolutions['heights'];

console.log('widths:')
for(let i = 0; i < widths.length; i++)
    console.log(widths[i].index);

console.log('heights:')
for(let i = 0; i < heights.length; i++)
    console.log(heights[i].index);
```
