#ifndef GUTC_MODEL_REGISTRY_H
#define GUTC_MODEL_REGISTRY_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pure, immutable, exact pinned GutTS model metadata (8 definitions).
 * No model loading or inference is performed by catalog operations.
 * Return values are independently owned GutC JSON DOM trees. */
size_t gutc_model_registry_count(void);
gutc_status gutc_model_registry_at(size_t index,gutc_json_dom **output);
gutc_status gutc_model_registry_lookup(const char *id,size_t length,
                                       gutc_json_dom **output,int *found);
gutc_status gutc_model_registry_list(gutc_json_dom **output);
#ifdef __cplusplus
}
#endif
#endif
