#ifndef GUTC_IDENTITY_H
#define GUTC_IDENTITY_H
#include "collections.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gutc_identity_ref {
  gutc_string_view kind;
  gutc_string_view id;
} gutc_identity_ref;
/* Parse a tagged identity into borrowed views. Never authorize from this alone. */
gutc_status gutc_identity_ref_parse(gutc_string_view kind,gutc_string_view id,
                                     gutc_identity_ref *result);
/* Alias precedence is not assumed: conflicting app_id / appId /
 * applicationId candidates fail closed. Missing aliases are invalid format.
 * Result references caller-owned input memory. */
gutc_status gutc_legacy_application_id(const gutc_object_entry *entries,
                                        size_t count,gutc_string_view *result);
/* Pure budget checks only; no grant/session/target authorization. */
gutc_status gutc_invocation_budget_retry(const gutc_invocation_budget *budget,
                                          int64_t now_ms,unsigned *remaining_retries);
#ifdef __cplusplus
}
#endif
#endif
