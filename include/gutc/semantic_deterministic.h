#ifndef GUTC_SEMANTIC_DETERMINISTIC_H
#define GUTC_SEMANTIC_DETERMINISTIC_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native deterministic subset of GutTS resolveDeterministicSemanticInput().
 * Input: valid UTF-8 span (<=8192 bytes), borrowed schema JSON DOM object.
 * A successfully resolved result is a heap-owned JSON DOM with the exact
 * {status,value,candidates,modelOutput} properties; caller frees it.
 * Unresolved valid input: GUTC_OK, *resolved=0, *output=NULL.
 * Unsupported JS RegExp pattern schema: GUTC_UNSUPPORTED (never fake success).
 * No model execution, no ranking, no tokens or IO. Numeric parsing uses
 * current C locale for decimal strtod and differs on exotic ECMAScript forms.
 * Reentrant except process-global locale changes made by external code. */
gutc_status gutc_resolve_deterministic_semantic(const char *input,size_t length,
                                                const gutc_json_dom *schema,
                                                gutc_json_dom **output,int *resolved);
#ifdef __cplusplus
}
#endif
#endif
