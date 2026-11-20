#ifndef GUTC_COLLECTIONS_H
#define GUTC_COLLECTIONS_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed values representing ECMAScript primitive/reference semantics.
 * Strings are byte spans; object/array identity is explicitly supplied via
 * reference, not inferred from shared element storage. No allocations. */
typedef enum {
  GUTC_VALUE_UNDEFINED, GUTC_VALUE_NULL, GUTC_VALUE_BOOL,
  GUTC_VALUE_NUMBER, GUTC_VALUE_STRING, GUTC_VALUE_OBJECT, GUTC_VALUE_ARRAY
} gutc_value_kind;
typedef struct gutc_value gutc_value;
typedef struct gutc_string_view {
  const char *bytes;
  size_t length;
} gutc_string_view;
typedef struct gutc_array_view {
  const gutc_value *items;
  size_t length;
  const void *identity;
} gutc_array_view;
struct gutc_value {
  gutc_value_kind kind;
  union {
    int boolean;
    double number;
    gutc_string_view string;
    const void *object_identity;
    gutc_array_view array;
  } as;
};
typedef struct gutc_object_entry {
  gutc_string_view key;
  gutc_value value;
} gutc_object_entry;
typedef struct gutc_chunk_view {
  const gutc_value *items;
  size_t count;
} gutc_chunk_view;

/* These are allocation-free reentrant operations with caller-owned output.
 * To size an output, pass NULL and cap=0; needed is the number of items.
 * On insufficient capacity, output is not modified. Input and output must
 * NOT overlap. String/object contents remain borrowed, not copied. */
int gutc_value_same_value_zero(const gutc_value *a, const gutc_value *b);
int gutc_value_truthy(const gutc_value *value);
gutc_status gutc_array_compact(const gutc_value *input,size_t count,
                                gutc_value *output,size_t capacity,size_t *needed);
gutc_status gutc_array_unique(const gutc_value *input,size_t count,
                               gutc_value *output,size_t capacity,size_t *needed);
gutc_status gutc_array_flatten_once(const gutc_value *input,size_t count,
                                    gutc_value *output,size_t capacity,size_t *needed);
gutc_status gutc_array_chunks(const gutc_value *input,size_t count,size_t chunk_size,
                               gutc_chunk_view *output,size_t capacity,size_t *needed);
gutc_status gutc_array_last(const gutc_value *input,size_t count,gutc_value *result,int *found);
/* Object key selection handles only direct keys; nested Lodash string paths
 * require a separate schema/parser. Duplicate entry keys are invalid. */
gutc_status gutc_object_has_direct(const gutc_object_entry *input,size_t count,
                                   gutc_string_view key,int *found);
gutc_status gutc_object_pick_direct(const gutc_object_entry *input,size_t count,
                                    const gutc_string_view *keys,size_t key_count,
                                    gutc_object_entry *output,size_t capacity,size_t *needed);
gutc_status gutc_object_omit_direct(const gutc_object_entry *input,size_t count,
                                    const gutc_string_view *keys,size_t key_count,
                                    gutc_object_entry *output,size_t capacity,size_t *needed);
/* Finite-only decimal rounding, unlike JS allowing infinities. The result
 * can diverge from lodashRound on difficult IEEE-754 tie cases. */
gutc_status gutc_number_round_to(double value,unsigned decimals,double *result);
int gutc_number_is_finite(double value);
#ifdef __cplusplus
}
#endif
#endif
