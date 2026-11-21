#ifndef GUTC_JS_NUMBER_FORMAT_H
#define GUTC_JS_NUMBER_FORMAT_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Portable C11 finite IEEE-754 binary64 JSON number rendering.
 * Produce shortest round-trip significant digits (1..17), then apply
 * ECMAScript JSON notation thresholds [1e-6,1e21). -0 -> 0; NaN,
 * infinities -> null. Unlike raw %.17g, output is JS-compatible on
 * the pinned, executable cross-language boundary corpus.
 *
 * This implementation uses libc snprintf/strtod for shortest candidate
 * selection, requiring the process's C numeric locale and correctly
 * rounded strtod. It does NOT globally change locale (thread unsafe).
 * For hostile/unknown locale or a non-roundtripping libc conversion,
 * returns GUTC_UNSUPPORTED instead of guessing. Full Schubfach/Ryu
 * equivalence for every double remains separately certifiable.
 * Two-pass: output=NULL cap=0 obtains needed length excluding trailing NUL;
 * too-small output unchanged. No heap allocations or global mutable state.
 */
gutc_status gutc_js_number_format(double number,
  char *output,size_t capacity,size_t *needed);
#ifdef __cplusplus
}
#endif
#endif
