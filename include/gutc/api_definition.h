#ifndef GUTC_API_DEFINITION_H
#define GUTC_API_DEFINITION_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native portable GutTS defineApi (src/api/define-api.ts).
 * Input: a JSON-owned API definition without inputSchema, with schema-valued
 * parameters. Output: a distinct heap-owned JSON DOM; release via
 * gutc_json_dom_free(). Output=NULL on all errors.
 * Builds inputSchema from each parameter's name, schema and description,
 * required parameter order and additionalProperties=false. No execution,
 * network, model loading or registry dispatch takes place here.
 * Invalid input shapes fail closed; GutTS itself may throw JS exceptions.
 * All functions are reentrant and operate on immutable caller input. */
gutc_status gutc_define_api(const gutc_json_dom *definition,
                            gutc_json_dom **output);
#ifdef __cplusplus
}
#endif
#endif
