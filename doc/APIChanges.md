**2025-11-15**
* The `method` argument to all resdetect functions may now be `NULL`, in which case the library default method will be used. Previously this caused an `RDEPARAM` error to be returned. (4a6acdd)

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
