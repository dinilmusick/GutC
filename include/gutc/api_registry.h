#ifndef GUTC_API_REGISTRY_H
#define GUTC_API_REGISTRY_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact pinned GutTS 3e2cbc0 API metadata catalog.
 * All outputs are independently heap-owned and must be freed with
 * gutc_json_dom_free(). There is no global mutable state or hidden host IO.
 * The catalog includes five model descriptors, but does not imply that
 * GutC can execute those models without a configured real host adapter. */
size_t gutc_api_registry_count(void);
gutc_status gutc_api_registry_at(size_t index,gutc_json_dom **output);
gutc_status gutc_api_registry_lookup(const char *id,size_t length,
                                      gutc_json_dom **output,int *found);
gutc_status gutc_api_registry_list(gutc_json_dom **output);
gutc_status gutc_api_registry_fod_nodes(gutc_json_dom **output);
#ifdef __cplusplus
}
#endif
#endif
