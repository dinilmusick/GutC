#ifndef GUTC_CONTRACT_PRIMITIVES_H
#define GUTC_CONTRACT_PRIMITIVES_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Source-pinned pure GutTS oauthClientContract helpers. No authorization.
 * Input trees and returned object references are caller-owned/borrowed.
 * Scalar predicates never allocate. A malformed JSON value returns false;
 * invalid native pointers produce GUTC_INVALID_ARGUMENT where applicable.
 * String byte spans may include NUL, and lengths are explicit. */
const gutc_json_dom *gutc_contract_record(const gutc_json_dom *value);
int gutc_contract_id(const gutc_json_dom *value);
int gutc_contract_app_id(const gutc_json_dom *value);
gutc_status gutc_contract_exact_keys(const gutc_json_dom *value,
                                      const gutc_json_text *allowed,
                                      size_t allowed_count,int *result);
unsigned gutc_app_contract_version(void);
#ifdef __cplusplus
}
#endif
#endif
