#ifndef GUT_STRING_PREDICATES_H
#define GUT_STRING_PREDICATES_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Matches JavaScript String.trim() whitespace for valid UTF-8 input.
 * Returns -1 for NULL or malformed UTF-8, 0 for nonblank, 1 for blank. */
int gut_is_blank_utf8(const char *value);
/* Returns -1 for invalid input, 0 for blank, 1 for nonblank. */
int gut_is_nonblank_utf8(const char *value);
/* Explicit-length validation of non-NUL-terminated spans; embedded NUL is invalid. */
int gut_is_blank_utf8_n(const char *value, size_t length);
int gut_is_nonblank_utf8_n(const char *value, size_t length);
/* Returns 1 if any prefix/suffix matches, 0 if none, -1 for invalid UTF-8,
 * NULL pointers, or a NULL candidate array with nonzero count.
 * Empty candidates match every valid string, including the empty string.
 * Explicit counts avoid sentinel-based array traversal. */
int gut_starts_with_any_utf8(const char *value, const char *const *prefixes, size_t count);
int gut_ends_with_any_utf8(const char *value, const char *const *suffixes, size_t count);
#ifdef __cplusplus
}
#endif
#endif
