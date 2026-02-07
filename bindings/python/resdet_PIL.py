import resdet_numpy
from resdet_numpy import resdetect, Analysis, Method, Image

import PIL.Image
import numpy
from typing import Optional

class Image(resdet_numpy.Image):
    def read_image_frame_as_pil_image(self) -> Optional[PIL.Image.Image]:
        if (ndarray := self.read_image_frame_as_ndarray()) is not None:
            return PIL.Image.fromarray(ndarray, "F")

    def __iter__(self):
        return self

    def __next__(self):
        if ret := self.read_image_frame_as_pil_image():
            return ret
        raise StopIteration

@Analysis.analyze_image.register
def _(self, image: PIL.Image.Image) -> None:
    self.analyze_image(numpy.array(image.convert("F")))

@resdetect.register
def _(image: PIL.Image.Image, method: Optional[Method] = None, parameters: dict = {}) -> dict:
    analysis = Analysis(image.width, image.height, method, parameters)
    frameno = prev_frame = image.tell()
    while True:
        try:
            image.seek(frameno)
            frameno += 1
        except EOFError:
            break;
        analysis.analyze_image(image)

    image.seek(prev_frame)
    return analysis.analysis_results()
