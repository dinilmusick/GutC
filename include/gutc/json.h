#ifndef GUTC_JSON_H
#define GUTC_JSON_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* JSON syntax/UTF-8 validation over a bounded byte span. Does not construct a DOM.
 * max_depth 1..128. On syntax error, error_offset is the first rejected position;
 * on success it is the input length. No allocation or mutation. */
gutc_status gutc_json_validate(const uint8_t *text, size_t length,
                               unsigned max_depth, size_t *error_offset);
/* Serializes exactly one UTF-8 string as a JSON string literal, including quotes
 * and NUL terminator. 'needed' excludes NUL; NULL/0 permits sizing. */
gutc_status gutc_json_quote_string(const uint8_t *text, size_t length,
                                   char *output, size_t capacity, size_t *needed);
#ifdef __cplusplus
}
#endif
#endif
