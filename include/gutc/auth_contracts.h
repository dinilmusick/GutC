#ifndef GUTC_AUTH_CONTRACTS_H
#define GUTC_AUTH_CONTRACTS_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pure schema/admission contracts: these DO NOT authenticate Luth principals,
 * verify grants, authorize target workspaces, attest ancestry, or prove live
 * frontend consent. A trusted host MUST enforce those separately on every hop.
 * Returned reason points to static library-owned storage and must not be freed.
 * NULL reason is permitted. All functions are reentrant; input is borrowed.
 * A native C parse error must be handled before invoking these validators. */
gutc_status gutc_validate_capability_descriptor_v1(const gutc_json_dom *descriptor,
                                                     const char **reason);
gutc_status gutc_validate_scoped_invocation_v1(const gutc_json_dom *invocation,
                                                const char **reason);
gutc_status gutc_check_scoped_invocation_budget_v1(const gutc_json_dom *invocation,
                                                   double now_ms,const char **reason);
gutc_status gutc_match_invocation_descriptor_v1(const gutc_json_dom *invocation,
                                                  const gutc_json_dom *descriptor,
                                                  double now_ms,const char **reason);
gutc_status gutc_advance_scoped_invocation_v1(const gutc_json_dom *invocation,
                                              double now_ms,unsigned *calls_left,
                                              const char **reason);
gutc_status gutc_retry_scoped_invocation_v1(const gutc_json_dom *invocation,
                                            double now_ms,unsigned *retries_left,
                                            const char **reason);
#ifdef __cplusplus
}
#endif
#endif
