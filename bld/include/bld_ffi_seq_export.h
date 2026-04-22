/* bld/include/bld_ffi_seq_export.h
 * Phase 18-B — C++ editor pushes seq.export JSON into bld-ffi for Rust tools.
 */
#ifndef BLD_FFI_SEQ_EXPORT_H_
#define BLD_FFI_SEQ_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

void bld_ffi_seq_export_set_json(const char* utf8);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* BLD_FFI_SEQ_EXPORT_H_ */
