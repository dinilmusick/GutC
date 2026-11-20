#ifndef GUTC_SAFE_PATH_H
#define GUTC_SAFE_PATH_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Mirrors normalizeStoragePath -> encodeURIComponent per slash-separated
 * component, including JS punctuation !'()* and URI percent escaping.
 * Rejects invalid percent layers, embedded NUL, malformed UTF-8,
 * traversal/delimiter escapes and empty paths. Caller owns output.
 * A NULL/0 output is a sizing request; needed excludes trailing NUL.
 * Input must be readable, NUL-terminated, <= 1 MiB and not overlap output.
 * Temporary scratch allocations are released before return. */
gutc_status gutc_encode_safe_path_segments(const char *input,
                          char *output,size_t capacity,size_t *needed);
#ifdef __cplusplus
}
#endif
#endif
