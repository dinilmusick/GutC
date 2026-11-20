#ifndef GUTC_OBJECT_PATH_H
#define GUTC_OBJECT_PATH_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* JSON object analogue of GutTS lodash-backed hasOwn/pick/omit.
 * Paths are borrowed UTF-8 strings; supported syntax:
 *   name, name.child, rows[0].name, record["literal.key"], record['key']
 * Up to 32 segments and 512 path bytes. No prototype chain, symbol keys or
 * JS object property descriptors exist in JSON. Pick/omit output is an
 * independent heap-owned tree: caller must free with gutc_json_dom_free().
 * Non-canonical/unsupported syntaxes return GUTC_UNSUPPORTED, never
 * silently select a different property. Prototype-pollution keys are
 * rejected. Input remains immutable; partial results are freed on failure. */
gutc_status gutc_json_path_has(const gutc_json_dom *root,
                               const char *path,size_t length,int *found);
gutc_status gutc_json_pick_paths(const gutc_json_dom *root,
                                 const gutc_json_text *paths,size_t count,
                                 gutc_json_dom **output);
gutc_status gutc_json_omit_paths(const gutc_json_dom *root,
                                 const gutc_json_text *paths,size_t count,
                                 gutc_json_dom **output);
int gutc_json_is_plain_object(const gutc_json_dom *value);
#ifdef __cplusplus
}
#endif
#endif
