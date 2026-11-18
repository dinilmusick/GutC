#ifndef GUTC_PERCENT_DECODE_H
#define GUTC_PERCENT_DECODE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Decode %HH byte escapes without treating '+' as a space.
 * Returns decoded byte count, or SIZE_MAX on malformed escapes, invalid
 * arguments, or size overflow. out may be NULL for sizing.
 * If capacity is insufficient, out is not modified. A successful write
 * appends a NUL terminator; decoded bytes may themselves contain NUL.
 * Input and output must not overlap.
 */
size_t gutc_url_percent_decode(const char *input, size_t length,
                               unsigned char *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
