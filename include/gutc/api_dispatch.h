#ifndef GUTC_API_DISPATCH_H
#define GUTC_API_DISPATCH_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif

/* GutTS executeGuttsApi native JSON-DOM equivalent.
 * Exact API ID is looked up in the pinned registry before dispatch.
 * Inputs are caller-owned, borrowed and never mutated. On success the
 * output DOM is exclusively caller-owned and freed using gutc_json_dom_free.
 * defined=0 with NULL output models JS undefined distinctly from null.
 * A failed/unsupported operation leaves output=NULL and defined=0.
 *
 * Host model inference is never fabricated: the five model API IDs require
 * an explicitly provided verified adapter which actually performs inference.
 * The callback must allocate a valid owned result; a fake GUTC_OK with NULL
 * output is rejected. No runtime state or credentials are retained.
 */
typedef gutc_status (*gutc_model_api_invoke)(
  void *context,const char *api_id,size_t id_length,
  const gutc_json_dom *input,gutc_json_dom **output,int *defined);
typedef struct gutc_model_api_adapter {
  gutc_model_api_invoke invoke;
  void *context;
} gutc_model_api_adapter;

gutc_status gutc_execute_api(const char *api_id,size_t id_length,
                              const gutc_json_dom *input,
                              const gutc_model_api_adapter *model_adapter,
                              gutc_json_dom **output,int *defined);
#ifdef __cplusplus
}
#endif
#endif
