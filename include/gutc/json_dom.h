#ifndef GUTC_JSON_DOM_H
#define GUTC_JSON_DOM_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
  GUTC_JSON_NULL, GUTC_JSON_BOOLEAN, GUTC_JSON_NUMBER,
  GUTC_JSON_STRING, GUTC_JSON_ARRAY, GUTC_JSON_OBJECT
} gutc_json_kind;
typedef struct gutc_json_dom gutc_json_dom;
typedef struct gutc_json_text {char *bytes;size_t length;} gutc_json_text;
typedef struct gutc_json_member {gutc_json_text key;gutc_json_dom *value;} gutc_json_member;
struct gutc_json_dom {
  gutc_json_kind kind;
  union {
    int boolean; double number; gutc_json_text string;
    struct {gutc_json_dom **values;size_t count;} array;
    struct {gutc_json_member *members;size_t count;} object;
  } as;
};
/* Parsed tree and all descendants are owned by caller; free exactly once.
 * C recursion is capped at 128 and node count at 100000. On parse failure,
 * *output is NULL. Duplicate object keys use last value, first key order.
 * Unicode escapes must be paired correctly; unlike JS, lone surrogates error.
 * Caller must not mutate returned tree into a cyclic graph. */
gutc_status gutc_json_dom_parse(const unsigned char *text,size_t length,
                                unsigned max_depth,size_t max_nodes,
                                gutc_json_dom **output,size_t *error_offset);
void gutc_json_dom_free(gutc_json_dom *root);
/* Two-pass no-partial-write serialization, size excludes trailing NUL.
 * Nonfinite numbers serialize as null. IEEE-754 numbers use shortest-roundtrip
 * ECMAScript-style output via gutc_js_number_format, subject to libc numeric
 * C-locale/rounding preconditions and documented exhaustive-proof limitation. */
gutc_status gutc_json_dom_stringify(const gutc_json_dom *root,
                                    char *output,size_t capacity,size_t *needed);
const gutc_json_dom *gutc_json_dom_get(const gutc_json_dom *object,
                                      const char *key,size_t key_len);
int gutc_json_dom_is_json_value(const gutc_json_dom *root);
#ifdef __cplusplus
}
#endif
#endif
