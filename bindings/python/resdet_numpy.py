import resdet
from resdet import resdetect, Analysis, Method, libresdet

import numpy
import os
import ctypes
from typing import Optional

class Image(resdet.Image):
    def __init__(self, filename: str | os.PathLike, type: Optional[str] = None) -> None:
        self.buffer = resdet.ImageBuffer()
        super().__init__(filename, type, self.buffer)

    def read_image_frame_as_ndarray(self) -> Optional[numpy.ndarray]:
        if self.read_image_frame(self.buffer):
            return numpy.ctypeslib.as_array(self.buffer.data, shape = self.buffer.shape()).copy()

    def __iter__(self):
        return self

    def __next__(self):
        if (ret := self.read_image_frame_as_ndarray()) is not None:
            return ret
        raise StopIteration

@Analysis.analyze_image.register
def _(self, image: numpy.ndarray) -> None:
    self.analyze_image(image.ctypes.data_as(ctypes.POINTER(ctypes.c_float)))

@resdetect.register
def _(image: numpy.ndarray, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    nimages = image.shape[2] if len(image.shape) > 2 else 1
    return resdetect(image.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), nimages, image.shape[0], image.shape[1], method, parameters)
