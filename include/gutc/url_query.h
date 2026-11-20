#ifndef GUTC_URL_QUERY_H
#define GUTC_URL_QUERY_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* The URLSearchParams semantics of GutTS getQueryParam / setQueryParam,
 * restricted to absolute http(s) URL strings. Other WHATWG schemes, IDNA
 * normalization, canonical numeric host/port and percent-invalid replacement
 * require host URL adapters; do not treat this as a general WHATWG parser.
 * Names/values must be UTF-8; invalid decoding is rejected, never silently
 * reinterpreted. Exposes strict, allocation-failure-reporting native results.
 * All functions reentrant. Input/output must not overlap and input C strings
 * must be NUL terminated within 1 MiB. Returns output UTF-8 byte count
 * excluding NUL. NULL/0 output means sizing; short buffers are not written. */
int gutc_url_is_http_url(const char *url);
gutc_status gutc_url_get_query_param(const char *url,const char *key,
                                     char *out,size_t capacity,size_t *needed,
                                     int *found);
gutc_status gutc_url_set_query_param(const char *url,const char *key,
                                     const char *value,char *out,
                                     size_t capacity,size_t *needed);
#ifdef __cplusplus
}
#endif
#endif
