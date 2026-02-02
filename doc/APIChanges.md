**2026-02-02**
* Addition of the `resdet_get_max_error` function which may be used to obtain the current largest value from the RDErrors enum. (d0201fb)
* Addition of the `RDENOIMG` `RDError` code for when an analysis was given no images. (c5a80af)
  `resdetect` now returns an `RDENOIMG` error when called with an `nimages` parameter of 0.
  `resdet_analysis_results` now returns an `RDENOIMG` error if no calls to `resdet_analyze_frame` for the given `RDAnalaysis` were ever made.
  Previously these succeeded and yielded output `RDResolution` arrays containing only the input dimensions in these cases.

---

**libresdet 2.2.0 released here**

---
---

**libresdet 2.1.0 released here**

---

**2025-11-20**
* Addition of the `resdet_seek_frame` function which may be used to seek an `RDImage` to the given frame number. (8e1a22e)

**2025-11-19**
* The `mimetype` (now `filetype`) field to `resdetect_file` and `resdet_open_image` functions may now alternatively contain a file extension. (86a3b6b)

---

**libresdet 2.0.0 released here**

---

**2025-11-18**
* The `OMIT_NATIVE_PGM_READER` configuration macro is changed to `OMIT_NATIVE_PGM_PFM_READERS` to reflect the fact that these are now separate translation units. (7e488d4)
* The `RDParameters` struct is made opaque. Addition of the `resdet_alloc_default_parameters` function to obtain an `RDParameters` instance, and `resdet_parameters_set_range` and `resdet_parameters_set_threshold` functions to set parameter values. This facilitates adding future parameters without breaking ABI. (113486f)
* `threshold` parameter values greater than 1 are no longer accepted and cause an `RDEPARAM` error to be returned from `resdet_parameters_set_threshold`. (113486f)

**2025-11-17**
* The `resw`, `countw`, `resh`, and `counth` output parameters to all detection functions are now marked `restrict`. (8da871d)
* `threshold` parameters below 0 or `NaN` now return an `RDEPARAM` error from `resdet_create_analysis_with_params` and the `resdetect` functions. (fe101a6)
* Addition of the `RDParameters` type and `params` argument to functions `resdetect`, `resdetect_file`, and `resdet_create_analysis`. Removal of functions `resdetect_with_params`, `resdetect_file_with_params`, and `resdet_create_analysis_with_params` in favor of calling the respective base functions with an `RDParameters` struct. (c9d0e7a)

**2025-11-16**
* Addition of the sequential analysis API. Adds a new opaque type `RDAnalysis` and functions to more efficiently analyze image sequences: `resdet_create_analysis`, `resdet_create_analysis_with_params`, `resdet_analyze_image`, `resdet_analysis_results`, and `resdet_destroy_analysis`. (d38a23f)
* Addition of the sequential image reading API. Adds a new opaque type `RDImage` and functions to read image sequences iteratively: `resdet_open_image`, `resdet_read_image_frame`, and `resdet_close_image`. (384eb93)

**2025-11-15**
* The `method` argument to all resdetect functions may now be `NULL`, in which case the library default method will be used. Previously this caused an `RDEPARAM` error to be returned. (4a6acdd)
* The underlying struct type of `RDMethod` is now const. (bcfa100)

**2025-11-11**

* The `RDErrStr` array is replaced with a function `resdet_error_str(RDError)`. (8aee400)
* `RDError` is now an alias for int. It may contain a value from `enum RDErrors` or a negated errno code. (de4d445)
* Addition of the `RDETOOBIG` error code. (7cef3ec)
* Addition of the `RDEPARAM` error code. (1ee035c)

**2025-11-10**

* Addition of the `RDEUNSUPP` error code. (6c22a18)

**2025-11-09**

* Output resolution arrays are now sorted in descending confidence order. (5d38fe5)

---

**libresdet 1.0.0 released here**

---
