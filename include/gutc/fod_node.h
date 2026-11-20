#ifndef GUTC_FOD_NODE_H
#define GUTC_FOD_NODE_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native metadata-only counterpart of GutTS guttsApiToFodNode.
 * Input is a borrowed, JSON DOM ApiDefinition (normally constructed with
 * gutc_define_api); output is an independent owned DOM freed with
 * gutc_json_dom_free(). Requires typed fields and explicit schemas.
 * No workflow dispatch, model loading, database or authorization occurs. */
gutc_status gutc_api_to_fod_node(const gutc_json_dom *api,
                                 gutc_json_dom **output);
#ifdef __cplusplus
}
#endif
#endif
