from libresdet import libresdet, RDMethod, RDResolution, RDParameters

import ctypes
from ctypes.util import find_library
from dataclasses import dataclass
from functools import singledispatch, singledispatchmethod
from typing import Optional, Callable, TYPE_CHECKING
import os
from pathlib import Path
import array
from enum import IntEnum
import platform

if platform.system() == "Windows":
    libc = ctypes.WinDLL("ucrtbase")
else:
    libc = ctypes.CDLL(find_library("c"))

libc.malloc.restype = ctypes.c_void_p
libc.malloc.argtypes = [ctypes.c_size_t]
libc.free.restype = None
libc.free.argtypes = [ctypes.c_void_p]

if TYPE_CHECKING:
    RDMethodPtr = ctypes._Pointer[RDMethod]
    RDParametersPtr = ctypes._Pointer[RDParameters]
    RDResolutionPtr = ctypes._Pointer[RDResolution]
    c_float_ptr = ctypes._Pointer[ctypes.c_float]
else:
    RDMethodPtr = ctypes.POINTER(RDMethod)
    RDParametersPtr = ctypes.POINTER(RDParameters)
    RDResolutionPtr = ctypes.POINTER(RDResolution)
    c_float_ptr = ctypes.POINTER(ctypes.c_float)

# public classes

class Error(IntEnum):
    OK       = 0
    NOMEM    = 1
    INTERNAL = 2
    INVAL    = 3
    UNSUPP   = 4
    TOOBIG   = 5
    PARAM    = 6
    NOIMG    = 7

# for NOMEM we use MemoryError()

class InternalError(Exception):
    pass

class InvalidImageError(Exception):
    pass

class UnsupportedFormatError(Exception):
    pass

class ImageTooBigErrror(Exception):
    pass

class ParameterError(Exception):
    pass

class NoImagesError(Exception):
    pass

class Method:
    name: str
    threshold: float
    _c_method: RDMethodPtr

    def __init__(self, name: str, threshold: float, c_method: RDMethodPtr) -> None:
        self.name = name
        self.threshold = threshold
        self._c_method = c_method
        self._as_parameter_ = self._c_method

@dataclass
class Resolution:
    index: int
    confidence: float

class ImageBuffer:
    data: c_float_ptr
    width: int
    height: int
    nimages: int

    def __init__(self, buffer: Optional[c_float_ptr] = None) -> None:
        self.data = buffer if buffer else ctypes.POINTER(ctypes.c_float)()
        self._as_parameter_ = self.data

    @classmethod
    def with_dimensions(cls, width: int, height: int, nimages: int = 1) -> "ImageBuffer":
        imagebuffer = cls(ctypes.cast(libc.malloc(width*height*ctypes.sizeof(ctypes.c_float)), ctypes.POINTER(ctypes.c_float)))
        imagebuffer.width = width
        imagebuffer.height = height
        imagebuffer.nimages = nimages
        return imagebuffer

    def shape(self):
        shape = (self.width, self.height)
        if self.nimages > 1:
            shape += (self.nimages,)
        return shape

    def __getitem__(self, index):
        if index > self.width * self.height:
            raise IndexError(f"index {index} is out of bounds for size {self.width}x{self.height}")
        return self.data[index]

    def close(self) -> None:
        libc.free(self.data)
        self.data = ctypes.POINTER(ctypes.c_float)()

    def __enter__(self) -> "ImageBuffer":
        return self

    def __exit__(self, type, value, traceback) -> None:
        self.close()
        return None

    def __del__(self) -> None:
        self.close()

class Image:
    width: int
    height: int

    def __init__(self, filename: str | os.PathLike, type: Optional[str] = None, buffer: Optional[ImageBuffer] = None) -> None:
        type_arg = type.encode("utf-8") if type else None
        width = ctypes.c_size_t()
        height = ctypes.c_size_t()
        err = ctypes.c_int()
        buf = ctypes.byref(buffer.data) if buffer else None
        self._rdimage = libresdet.resdet_open_image(str(filename).encode("utf-8"), type_arg, width, height, buf, err)
        if not self._rdimage:
            raise _rderror_to_exception(err)

        self.width = width.value
        self.height = height.value
        if buffer:
            buffer.width = self.width
            buffer.height = self.height
            buffer.nimages = 1

    def read_image_frame(self, buffer: ImageBuffer) -> Optional[list]:
        if not buffer.data:
            raise Exception("image buffer must be initialized")

        err = ctypes.c_int()
        ret = libresdet.resdet_read_image_frame(self._rdimage, buffer, err)
        if err:
            raise _rderror_to_exception(err)
        return ret

    def seek_frame(self, offset: int, progress: Optional[Callable[[int], None]] = None):
        progress_cb_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_uint64)
        if progress:
            progress_arg = progress_cb_type(lambda _, frameno: progress(frameno))
        else:
            progress_arg = progress_cb_type()

        err = ctypes.c_int()
        ret = libresdet.resdet_seek_frame(self._rdimage, offset, progress_arg, None, err)
        if err:
            raise _rderror_to_exception(err)
        return ret

    def close_image(self) -> None:
        libresdet.resdet_close_image(self._rdimage)
        self._rdimage = None

    def __enter__(self) -> "Image":
        return self

    def __exit__(self, type, value, traceback) -> None:
        self.close_image()
        return None

    def __del__(self) -> None:
        self.close_image()

class Analysis:
    def __init__(self, width: int, height: int, method: Optional[Method] = None, parameters: dict = {}) -> None:
        params_arg = _dict_to_rdparameters(parameters)
        err = ctypes.c_int()
        self._rdanalysis = libresdet.resdet_create_analysis(method, width, height, params_arg, err)
        libc.free(params_arg)

        if not self._rdanalysis:
            raise _rderror_to_exception(err)

    @singledispatchmethod
    def analyze_image(self, image: c_float_ptr) -> None:
        if not image:
            raise Exception("image data not initialized")

        err = libresdet.resdet_analyze_image(self._rdanalysis, image)
        if err:
            raise _rderror_to_exception(err)

    @analyze_image.register
    def _(self, image: ImageBuffer) -> None:
        self.analyze_image(image.data)

    @analyze_image.register
    def _(self, image: array.array) -> None:
        address, _ = image.buffer_info()
        self.analyze_image(ctypes.cast(address, ctypes.POINTER(ctypes.c_float)))

    @analyze_image.register
    def _(self, image: list) -> None:
        arraytype = ctypes.c_float * len(image)
        self.analyze_image(arraytype(*image))

    def analysis_results(self) -> dict:
        resw = ctypes.POINTER(RDResolution)()
        resh = ctypes.POINTER(RDResolution)()
        countw = ctypes.c_size_t()
        counth = ctypes.c_size_t()

        err = libresdet.resdet_analysis_results(self._rdanalysis, resw, countw, resh, counth)
        if err:
            raise _rderror_to_exception(err)

        widths = _resolution_list_from_rdresolutions(resw, countw)
        libc.free(resw)
        heights = _resolution_list_from_rdresolutions(resh, counth)
        libc.free(resh)

        return {"widths": widths, "heights": heights}

    def destroy_analysis(self) -> None:
        libresdet.resdet_destroy_analysis(self._rdanalysis)
        self._rdanalysis = None

    def __enter__(self) -> "Analysis":
        return self

    def __exit__(self, type, value, traceback) -> None:
        self.destroy_analysis()
        return None

    def __del__(self) -> None:
        self.destroy_analysis()

# internal functions

def _rderror_to_exception(err: ctypes.c_int | int) -> Exception:
    msg = libresdet.resdet_error_str(err).decode("utf-8")
    e = err.value if type(err) is ctypes.c_int else err
    match e:
        case Error.NOMEM:
            return MemoryError(msg)
        case Error.INTERNAL:
            return InternalError(msg)
        case Error.INVAL:
            return InvalidImageError(msg)
        case Error.UNSUPP:
            return UnsupportedFormatError(msg)
        case Error.TOOBIG:
            return ImageTooBigErrror(msg)
        case Error.PARAM:
            return ParameterError(msg)
        case Error.NOIMG:
            return NoImagesError(msg)
        case _ if e < 0:
            return OSError(-e, msg)
        case _: # this shouldn't happen
            return Exception(msg)

def _dict_to_rdparameters(parameters: dict) -> RDParametersPtr:
    if parameters:
        rdparameters = libresdet.resdet_alloc_default_parameters()
        if "range" in parameters:
            libresdet.resdet_parameters_set_range(rdparameters, parameters["range"])
        if "threshold" in parameters:
            libresdet.resdet_parameters_set_threshold(rdparameters, parameters["threshold"])
    else:
        rdparameters = None
    return rdparameters

def _resolution_list_from_rdresolutions(rdresolution: RDResolutionPtr, count: ctypes.c_size_t) -> list:
    resolutions = []
    for i in range(count.value):
        resolution = ctypes.cast(ctypes.c_void_p(ctypes.addressof(rdresolution.contents) + ctypes.sizeof(RDResolution) * i), ctypes.POINTER(RDResolution))
        resolutions.append(Resolution(resolution.contents.index, resolution.contents.confidence))
    return resolutions

# public functions

def libversion() -> str:
    return libresdet.resdet_libversion().decode("utf-8")

def methods() -> dict:
    methods = {}
    method = libresdet.resdet_methods()
    while method.contents.name:
        name_str = method.contents.name.decode("utf-8")
        methods[name_str] = Method(name=name_str, threshold=method.contents.threshold, c_method=method)
        method = ctypes.cast(ctypes.c_void_p(ctypes.addressof(method.contents) + ctypes.sizeof(RDMethod)), ctypes.POINTER(RDMethod))
    return methods

def get_method(methodname: str) -> Method:
    method = libresdet.resdet_get_method(methodname.encode("utf-8"))
    return Method(method.contents.name.decode("utf-8"), method.contents.threshold, method)

def default_range() -> int:
    return libresdet.resdet_default_range()

def read_image(filename: str | os.PathLike, type: Optional[str] = None) -> ImageBuffer:
    type_arg = type.encode("utf-8") if type else None
    width = ctypes.c_size_t()
    height = ctypes.c_size_t()
    nimages = ctypes.c_size_t()
    buf = ImageBuffer()
    err = libresdet.resdet_read_image(str(filename).encode("utf-8"), type_arg, buf.data, nimages, width, height)
    if err:
        raise _rderror_to_exception(err)
    buf.width = width.value
    buf.height = height.value
    buf.nimages = nimages.value
    return buf

@singledispatch
def resdetect(image, nimages: int, width: int, height: int, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    if not image:
        raise Exception("image data not initialized")

    params_arg = _dict_to_rdparameters(parameters)

    resw = ctypes.POINTER(RDResolution)()
    resh = ctypes.POINTER(RDResolution)()
    countw = ctypes.c_size_t()
    counth = ctypes.c_size_t()

    err = libresdet.resdetect(image, nimages, width, height, resw, countw, resh, counth, method, params_arg)

    libc.free(params_arg)

    if err:
        raise _rderror_to_exception(err)

    widths = _resolution_list_from_rdresolutions(resw, countw)
    libc.free(resw)
    heights = _resolution_list_from_rdresolutions(resh, counth)
    libc.free(resh)

    return {"widths": widths, "heights": heights}

@resdetect.register
def _(image: ImageBuffer, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    return resdetect(image.data, image.nimages, image.width, image.height, method, parameters)

@resdetect.register
def _(image: array.array, nimages: int, width: int, height: int, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    address, _ = image.buffer_info()
    return resdetect(ctypes.cast(address, ctypes.POINTER(ctypes.c_float)), nimages, width, height, method, parameters)

@resdetect.register
def _(image: list, nimages: int, width: int, height: int, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    arraytype = ctypes.c_float * len(image)
    return resdetect(arraytype(*image), nimages, width, height, method, parameters)

def resdetect_file(filename: str | os.PathLike, type: Optional[str] = None, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    params_arg = _dict_to_rdparameters(parameters)

    type_arg = type.encode("utf-8") if type else None

    resw = ctypes.POINTER(RDResolution)()
    resh = ctypes.POINTER(RDResolution)()
    countw = ctypes.c_size_t()
    counth = ctypes.c_size_t()

    err = libresdet.resdetect_file(str(filename).encode("utf-8"), type_arg, resw, countw, resh, counth, method, params_arg)

    libc.free(params_arg)

    if err:
        raise _rderror_to_exception(err)

    widths = _resolution_list_from_rdresolutions(resw, countw)
    libc.free(resw)
    heights = _resolution_list_from_rdresolutions(resh, counth)
    libc.free(resh)

    return {"widths": widths, "heights": heights}
