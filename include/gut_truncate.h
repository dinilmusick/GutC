#ifndef GUT_TRUNCATE_H
#define GUT_TRUNCATE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { GUT_TRUNCATE_OK=0, GUT_TRUNCATE_INVALID=1, GUT_TRUNCATE_NOSPACE=2 } gut_truncate_status;
/* max_length counts Unicode scalar values; input/output must not overlap. */
gut_truncate_status gut_truncate_utf8(const char *input, size_t max_length, const char *suffix, char *output, size_t capacity, size_t *required);
/* GutTS truncate(value, maxLength) defaults to the Unicode ellipsis (U+2026).
 * This wrapper preserves the same capacity-query and validation contract. */
gut_truncate_status gut_truncate_utf8_default(const char *input, size_t max_length, char *output, size_t capacity, size_t *required);
#ifdef __cplusplus
}
#endif
#endif
