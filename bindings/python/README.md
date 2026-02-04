Python bindings for libresdet.

The files in this directory include full bindings for the [resdet API](https://github.com/0x09/resdet/blob/master/doc/libresdet.md) as well as additional modules supporting [NumPy](https://numpy.org/) and [Pillow](https://pypi.org/project/pillow/) data types.

Use of these bindings requires that libresdet be built as a shared library. This can be done from the resdet C sources using `./configure --enable-shared`, `make lib`, and optionally `make install-lib` if installation of the library is desired.

**Modules:**
* [resdet](resdet.py) - The native Python interfaces for resdet.
* [libresdet_api](libresdet_api.py) - Contains the raw ctypes mappings, this need not be used directly.
* [resdet_numpy](resdet_numpy.py) - Optional module which includes support for numpy ndarrays.
* [resdet_PIL](resdet_PIL.py) - Optional module which includes support for PIL images.

For detection with `resdetect` and `Analysis.analyze_image`, image data is accepted as any of the following  types:
* A raw `ctypes.POINTER(ctypes.c_float)`.
* A `list` of floats.
* An `array` created with type code "f".
* A `numpy.ndarray` with the resdet_numpy module.
* A `PIL.image` with the resdet_PIL module.

# Examples

These examples assume `image_path` has been set to the path of an image. Paths may be strings or PathLike objects.

---
Run resdet and print each detected width and height:
```python
from resdet import resdetect

resolutions = resdetect(image_path)

print("widths:")
for resolution in resolutions["widths"]:
    print(f"{resolution.index} {resolution.confidence}")

print("heights")
for resolution in resolutions["heights"]:
    print(f"{resolution.index} {resolution.confidence}")
```

resdet's detection parameters can be controlled by calling resdetect or the Analysis constructor with `parameters = { "threshold": the_threshold, "range": the_range }`  
A detection method can be provided with `method = the_method`. Methods can be obtained as a dict of their name to the method object using `resdetect.methods()`.

---

Run resdet on a NumPy ndarray:
```python
from resdet_numpy import resdetect

resolutions = resdetect(myndarray)
```
---
Run resdet on a Pillow image:
```python
from resdet_PIL import resdetect

image = PIL.Image.open(image_path)

resolutions = resdetect(image)
```
---
Use resdet's image reading API together with its sequential analysis API. Note that this example is equivalent to calling `resdetect(image_path)`:
```python
from resdet import ImageBuffer, Image, Analysis

imagebuf = ImageBuffer()
image = Image(image_path, buffer = imagebuf)
analysis = Analysis(image.width, image.height)

while image.read_image_frame(imagebuf):
    analysis.analyze_image(imagebuf)

resolutions = analysis.analysis_results()
```

These classes are also context managers, so the equivalent code using `with` is:
```python
with ImageBuffer() as imagebuf:
    with Image(image_path, buffer = imagebuf) as image:
        with Analysis(image.width, image.height) as analysis:
            while image.read_image_frame(imagebuf):
              analysis.analyze_image(imagebuf)
            resolutions = analysis.analysis_results()
```
