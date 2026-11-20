#ifndef GUTC_PATH_H
#define GUTC_PATH_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { GUTC_PATH_OK=0, GUTC_PATH_INVALID, GUTC_PATH_OVERFLOW } gutc_path_result;
/* Normalize storage-relative paths; empty/slash-only input produces empty output.
   Reject traversal, unsafe delimiters, encoded separators and nested escapes.
   On error, output is cleared when capacity is nonzero. */
gutc_path_result gutc_normalize_storage_path(const char *input, char *output, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
