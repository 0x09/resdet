**2025-12-17**
* The `resw`, `countw`, `resh`, and `counth` output parameters to all detection functions are now marked `restrict`. (8da871d)

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
