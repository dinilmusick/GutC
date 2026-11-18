#ifndef GUTC_URL_H
#define GUTC_URL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RFC 3986 unreserved bytes remain unchanged; other bytes become %HH.
 * Returns required output bytes excluding NUL. SIZE_MAX signals overflow.
 * If out is non-NULL and capacity is sufficient, writes a NUL-terminated
 * string. On insufficient capacity, out is not modified.
 * Input is a byte sequence and may contain embedded NUL bytes.
 */
size_t gutc_url_percent_encode(const unsigned char *input, size_t length,
                               char *out, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
