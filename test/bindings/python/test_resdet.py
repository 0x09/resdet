import resdet
import resdet_numpy
import resdet_PIL

import PIL
import numpy
import array

import pytest
from typing import Generator
import os
import re

test_file_dir = os.path.join(os.path.dirname(__file__), "..", "..", "files")
test_file = os.path.join(test_file_dir, "checkerboard.y4m")

@pytest.fixture(scope="module")
def test_file_frames() -> Generator[list,None,None]:
    yield [[1, 0, 0, 1], [0, 1, 1, 0]]

@pytest.fixture(scope="module")
def test_file_resolution_dict() -> Generator[dict,None,None]:
    yield {
        "widths": [resdet.Resolution(2, -1.0)],
        "heights": [resdet.Resolution(2, -1.0)]
    }

@pytest.fixture(scope="class")
def test_array(test_file_frames) -> Generator[array.array,None,None]:
    yield array.array("f", test_file_frames[0])

@pytest.fixture(scope="class")
def numpy_array() -> Generator[numpy.ndarray,None,None]:
    yield numpy.array([[1,0],[0,1]])

@pytest.fixture(scope="class")
def pil_image() -> Generator[PIL.Image.Image,None,None]:
    with PIL.Image.open(os.path.join(test_file_dir, "checkerboard.png")) as image:
        yield image

@pytest.fixture
def image_buffer() -> Generator[resdet.ImageBuffer,None,None]:
    with resdet.ImageBuffer() as buffer:
        yield buffer

@pytest.fixture
def rdimage(image_buffer) -> Generator[resdet.Image,None,None]:
    with resdet.Image(test_file, buffer = image_buffer) as rdimage:
        yield rdimage

class TestResdet:
    def test_libversion(self):
        version = resdet.libversion()
        assert re.match(r"\d+\.\d+\.\d+(\+[a-f0-9]+)?", version) is not None

    def test_get_method(self):
        method = resdet.get_method("zerox")
        assert method.name == "zerox"

    def test_methods(self):
        methodnames = ["sign","mag","orig","zerox"]

        assert list(resdet.methods().keys()) == methodnames
        assert [method.name for method in resdet.methods().values()] == methodnames

    def test_raises_invalid_image_error(self):
        with pytest.raises(resdet.InvalidImageError):
            resdet.resdetect([1],1,0,0)

    def test_raises_no_images_error(self):
        with pytest.raises(resdet.NoImagesError):
            resdet.resdetect([1], 0, 1, 1)

    def test_raises_file_not_found_error(self):
        with pytest.raises(FileNotFoundError):
            resdet.resdetect_file("doesntexist.pgm")
        with pytest.raises(FileNotFoundError):
            resdet.Image("doesntexist.pgm")

    def test_raises_with_unrecognized_parameter(self):
        with pytest.raises(Exception):
            resdet.resdetect([1], 1, 1, 1, parameters = { "unrecognized": 1 })

    def test_resdetect_image_buffer(self, image_buffer, rdimage, test_file_resolution_dict):
        assert rdimage.read_image_frame(image_buffer)
        resolutions = resdet.resdetect(image_buffer)
        assert resolutions == test_file_resolution_dict

    def test_resdetect_list(self, test_file_frames, test_file_resolution_dict):
        resolutions = resdet.resdetect(test_file_frames[0], 1, 2, 2)
        assert resolutions == test_file_resolution_dict

    def test_resdetect_array(self, test_array, test_file_resolution_dict):
        resolutions = resdet.resdetect(test_array,1, 2, 2)
        assert resolutions == test_file_resolution_dict

    def test_analyzes_image_buffer(self, image_buffer, rdimage, test_file_resolution_dict):
        assert rdimage.read_image_frame(image_buffer)
        analysis = resdet.Analysis(rdimage.width, rdimage.height)
        analysis.analyze_image(image_buffer)
        resolutions = analysis.analysis_results()

        assert resolutions == test_file_resolution_dict

    def test_analyzes_list(self, test_file_frames, test_file_resolution_dict):
        analysis = resdet.Analysis(2, 2)
        analysis.analyze_image(test_file_frames[0])
        resolutions = analysis.analysis_results()

        assert resolutions == test_file_resolution_dict

    def test_analyzes_array(self, test_array, test_file_resolution_dict):
        analysis = resdet.Analysis(2, 2)
        analysis.analyze_image(test_array)
        resolutions = analysis.analysis_results()

        assert resolutions == test_file_resolution_dict

    def test_seeks_in_image(self, rdimage):
        frames = []
        rdimage.seek_frame(2, lambda frameno: frames.append(frameno))

        assert frames == [1, 2]

    def test_reads_image(self, test_file_frames):
        buffer = resdet.read_image(test_file)

        assert buffer.shape() == (2, 2, 2)
        assert list(buffer) == test_file_frames[0] + test_file_frames[1]

    def test_buffer_elements_are_accessible_by_integer_index(self):
        buffer = resdet.read_image(test_file)

        assert buffer[0] == 1
        with pytest.raises(IndexError):
            buffer[8]

    def test_buffer_elements_are_accessible_by_tuple_index(self):
        buffer = resdet.read_image(test_file)

        assert buffer[1, 1] == 1
        assert buffer[1, 1, 0] == 1
        with pytest.raises(IndexError):
            buffer[2, 0]
        with pytest.raises(IndexError):
            buffer[0, 2]
        with pytest.raises(IndexError):
            buffer[2, 0, 0]
        with pytest.raises(Exception):
            buffer[0, 0, 0, 0]

    def test_reads_image_from_input_buffer(self, test_file_frames):
        buffer = resdet.ImageBuffer()
        rdimage = resdet.Image(test_file, buffer = buffer)

        assert rdimage.read_image_frame(buffer)
        assert buffer.shape() == (2, 2)
        assert list(buffer) == test_file_frames[0]

    def test_reads_image_from_allocated_buffer(self, test_file_frames):
        rdimage = resdet.Image(test_file)
        buffer = resdet.ImageBuffer.with_dimensions(rdimage.width, rdimage.height)

        assert rdimage.read_image_frame(buffer)
        assert buffer.shape() == (2, 2)
        assert list(buffer) == test_file_frames[0]

    def test_reads_image_with_context_managers(self, test_file_frames):
        with resdet.ImageBuffer() as buffer:
            with resdet.Image(test_file, buffer = buffer) as rdimage:
                assert rdimage.read_image_frame(buffer)
                assert buffer.shape() == (2, 2)
                assert list(buffer) == test_file_frames[0]

    def test_read_image_frame_reads_all_frames(self, test_file_frames):
        frames = []
        nframes = 0
        buffer = resdet.ImageBuffer()
        rdimage = resdet.Image(test_file, buffer = buffer)
        while rdimage.read_image_frame(buffer):
            frames += list(buffer)
            nframes += 1

        assert nframes == 2
        assert frames == test_file_frames[0] + test_file_frames[1]

class TestResdetNumPy:
    def test_resdetect_numpy_array(self, numpy_array, test_file_resolution_dict):
        resolutions = resdet.resdetect(numpy_array)
        assert resolutions == test_file_resolution_dict

    def test_analyzes_numpy_array(self, numpy_array, test_file_resolution_dict):
        analysis = resdet.Analysis(*numpy_array.shape)
        analysis.analyze_image(numpy_array)
        resolutions = analysis.analysis_results()

        assert resolutions == test_file_resolution_dict

    def test_reads_image_from_numpy_array(self):
        rdimage = resdet_numpy.Image(test_file)
        ndarray = rdimage.read_image_frame_as_ndarray()

        assert ndarray is not None
        assert ndarray.tolist() == [[1.0, 0.0], [0.0, 1.0]]

    def test_enumerates_numpy_array(self):
        frames = []
        nframes = 0
        rdimage = resdet_numpy.Image(test_file)
        for frame in rdimage:
            frames.append(frame.tolist())
            nframes += 1

        assert nframes == 2
        assert frames == [[[1.0, 0.0], [0.0, 1.0]], [[0.0, 1.0], [1.0, 0.0]]]

class TestResdetPIL:
    def test_resdetect_PIL_image(self, pil_image, test_file_resolution_dict):
        resolutions = resdet.resdetect(pil_image)
        assert resolutions == test_file_resolution_dict

    def test_analyzes_PIL_image(self, pil_image, test_file_resolution_dict):
        analysis = resdet.Analysis(*pil_image.size)
        analysis.analyze_image(pil_image)
        resolutions = analysis.analysis_results()

        assert resolutions == test_file_resolution_dict

    def test_reads_image_from_PIL_image(self, test_file_frames):
        rdimage = resdet_PIL.Image(test_file)
        img = rdimage.read_image_frame_as_pil_image()

        assert img is not None
        assert list(img.get_flattened_data()) == test_file_frames[0]

    def test_enumerates_PIL_array(self, test_file_frames):
        frames = []
        nframes = 0
        rdimage = resdet_PIL.Image(test_file)
        for frame in rdimage:
            frames.append(frame.get_flattened_data())
            nframes += 1

        assert nframes == 2
        assert frames == [tuple(test_file_frames[0]), tuple(test_file_frames[1])]
