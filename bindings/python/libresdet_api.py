import ctypes
import platform

match platform.system():
    case "Windows":
        lib_name = "libresdet.dll"
    case "Darwin":
        lib_name = "libresdet.dylib"
    case _:
        lib_name = "libresdet.so"

if platform.system() == "Windows":
    libresdet = ctypes.WinDLL(lib_name)
else:
    libresdet = ctypes.CDLL(lib_name)

class RDResolution(ctypes.Structure):
    _fields_ = [
        ("index", ctypes.c_size_t),
        ("confidence", ctypes.c_float)
    ]

class RDMethod(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("func", ctypes.CFUNCTYPE(None)),
        ("threshold", ctypes.c_float),
    ]

class RDParameters(ctypes.Structure):
    pass

class RDAnalysis(ctypes.Structure):
    pass

class RDImage(ctypes.Structure):
    pass

libresdet.resdet_libversion.restype = ctypes.c_char_p

libresdet.resdet_error_str.restype = ctypes.c_char_p
libresdet.resdet_error_str.argtypes = [ctypes.c_int]

libresdet.resdet_methods.restype = ctypes.POINTER(RDMethod)
libresdet.resdet_methods.argtypes = []

libresdet.resdet_get_method.restype = ctypes.POINTER(RDMethod)
libresdet.resdet_get_method.argtypes = [ctypes.c_char_p]

libresdet.resdet_default_range.restype = ctypes.c_size_t

libresdet.resdet_alloc_default_parameters.restype = ctypes.POINTER(RDParameters)

libresdet.resdet_parameters_set_range.restype = ctypes.c_int
libresdet.resdet_parameters_set_range.argtypes = [ctypes.POINTER(RDParameters), ctypes.c_size_t]

libresdet.resdet_parameters_set_threshold.restype = ctypes.c_int
libresdet.resdet_parameters_set_threshold.argtypes = [ctypes.POINTER(RDParameters), ctypes.c_float]

libresdet.resdet_open_image.restype = ctypes.POINTER(RDImage)
libresdet.resdet_open_image.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.POINTER(ctypes.c_float)),
    ctypes.POINTER(ctypes.c_int)
]

libresdet.resdet_read_image_frame.restype = ctypes.c_bool
libresdet.resdet_read_image_frame.argtypes = [ctypes.POINTER(RDImage), ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_int)]

libresdet.resdet_seek_frame.restype = ctypes.c_bool
libresdet.resdet_seek_frame.argtypes = [ctypes.POINTER(RDImage), ctypes.c_uint64, ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_uint64), ctypes.c_void_p, ctypes.POINTER(ctypes.c_int)]

libresdet.resdet_close_image.restype = None
libresdet.resdet_close_image.argtypes = [ctypes.POINTER(RDImage)]

libresdet.resdet_read_image.restype = ctypes.c_int
libresdet.resdet_read_image.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_float)),
    ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t),
]

libresdet.resdet_create_analysis.restype = ctypes.POINTER(RDAnalysis)
libresdet.resdet_create_analysis.argtypes = [ctypes.POINTER(RDMethod), ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(RDParameters), ctypes.POINTER(ctypes.c_int)]

libresdet.resdet_analyze_image.restype = ctypes.c_int
libresdet.resdet_analyze_image.argtypes = [ctypes.POINTER(RDAnalysis), ctypes.POINTER(ctypes.c_float)]

libresdet.resdet_analysis_results.restype = ctypes.c_int
libresdet.resdet_analysis_results.argtypes = [
    ctypes.POINTER(RDAnalysis),
    ctypes.POINTER(ctypes.POINTER(RDResolution)), ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.POINTER(RDResolution)), ctypes.POINTER(ctypes.c_size_t)
]

libresdet.resdet_destroy_analysis.restype = None
libresdet.resdet_destroy_analysis.argtypes = [ctypes.POINTER(RDAnalysis)]

libresdet.resdetect.restype = ctypes.c_int
libresdet.resdetect.argtypes = [
    ctypes.POINTER(ctypes.c_float), ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t,
    ctypes.POINTER(ctypes.POINTER(RDResolution)), ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.POINTER(RDResolution)), ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(RDMethod), ctypes.POINTER(RDParameters)
]

libresdet.resdetect_file.restype = ctypes.c_int
libresdet.resdetect_file.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p,
    ctypes.POINTER(ctypes.POINTER(RDResolution)), ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.POINTER(RDResolution)), ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(RDMethod), ctypes.POINTER(RDParameters)
]
