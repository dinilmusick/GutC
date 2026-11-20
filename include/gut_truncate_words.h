#ifndef GUT_TRUNCATE_WORDS_H
#define GUT_TRUNCATE_WORDS_H
#include <stddef.h>
#include "gut_truncate.h"
#ifdef __cplusplus
extern "C" {
#endif
/* C11 UTF-8 equivalent of GutTS truncateWords: ECMAScript whitespace,
 * single ASCII space between retained words, default U+2026 omission.
 * No overlapping input/output. Invalid UTF-8 and size overflow fail closed.
 * required is bytes including NUL; no partial output on failure. */
gut_truncate_status gut_truncate_words_utf8(const char *input, size_t max_words,
 const char *suffix, char *output, size_t capacity, size_t *required);
gut_truncate_status gut_truncate_words_utf8_default(const char *input, size_t max_words,
 char *output, size_t capacity, size_t *required);
#ifdef __cplusplus
}
#endif
#endif
