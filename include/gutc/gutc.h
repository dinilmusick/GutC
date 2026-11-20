#ifndef GUTC_GUTC_H
#define GUTC_GUTC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* All functions are reentrant and use no global mutable state or heap allocation.
 * Caller retains ownership of all inputs and outputs. Output capacity includes
 * the terminating NUL for textual operations; 'needed' excludes that NUL.
 * With output=NULL and capacity=0, textual functions only calculate size.
 * On an error, output is unchanged and 'needed' is set only if documented.
 * Inputs must refer to readable memory of the declared length; non-null-terminated
 * C strings and overlapping input/output storage are not supported. */
typedef enum gutc_status {
  GUTC_OK=0, GUTC_INVALID_ARGUMENT, GUTC_INVALID_ENCODING,
  GUTC_INVALID_FORMAT, GUTC_RANGE_ERROR, GUTC_OVERFLOW,
  GUTC_NO_SPACE, GUTC_DEADLINE_EXCEEDED, GUTC_MAX_DEPTH,
  GUTC_CYCLE, GUTC_BUDGET_EXHAUSTED, GUTC_NO_MEMORY
} gutc_status;
const char *gutc_status_name(gutc_status status);
gutc_status gutc_utf8_validate(const uint8_t *bytes, size_t length, size_t *codepoints);
gutc_status gutc_percent_encode(const uint8_t *bytes, size_t length,
                                 char *output, size_t capacity, size_t *needed);
gutc_status gutc_percent_decode(const char *encoded, size_t length,
                                 uint8_t *output, size_t capacity, size_t *needed);
gutc_status gutc_join_url(const char *base, const char *const *parts, size_t count,
                           char *output, size_t capacity, size_t *needed);
int gutc_identity_kind_valid(const char *kind);
int gutc_identity_id_valid(const char *value, size_t length);
gutc_status gutc_string_blank_ascii(const char *text, size_t length, int *blank);
gutc_status gutc_string_starts_with_any(const char *text, size_t length,
                              const char *const *values, size_t count, int *matched);
gutc_status gutc_string_ends_with_any(const char *text, size_t length,
                              const char *const *values, size_t count, int *matched);
gutc_status gutc_number_clamp(double value, double minimum, double maximum, double *result);
gutc_status gutc_number_in_range(double value, double minimum, double maximum, int *result);
gutc_status gutc_number_sum(const double *values, size_t count, double *result);
gutc_status gutc_array_chunk_count(size_t count, size_t chunk_size, size_t *chunks);

/* Pure invocation-budget checks ONLY: never substitute for authenticated target
 * admission, grant verification, workspace membership or frontend consent. */
typedef struct gutc_invocation_budget {
  const char *caller_app_id;
  const char *target_app_id;
  const char *audience_app_id;
  const char *const *active_app_chain;
  size_t chain_length;
  int64_t deadline_at_ms;
  unsigned max_depth;
  unsigned remaining_calls;
  unsigned remaining_retries;
} gutc_invocation_budget;
gutc_status gutc_invocation_budget_check(const gutc_invocation_budget *budget, int64_t now_ms);
gutc_status gutc_invocation_budget_advance(const gutc_invocation_budget *budget, int64_t now_ms,
                                            unsigned *next_remaining_calls);

#ifdef __cplusplus
}
#endif
#endif
