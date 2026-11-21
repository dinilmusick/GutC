#ifndef GUTC_ICU_TRUNCATE_H
#define GUTC_ICU_TRUNCATE_H
#include "gut_truncate.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Optional ICU-backed Unicode grapheme truncation. The ordinary gutc/gut
 * C11 target never links ICU. max_length counts extended grapheme clusters,
 * not UTF-8 bytes, UTF-16 units or Unicode scalar values.
 * NULL output queries required bytes (including trailing NUL) and returns
 * GUT_TRUNCATE_NOSPACE. A too-small output is left completely unchanged.
 * Input/output spans must not overlap. Invalid UTF-8 and allocation failure
 * return GUT_TRUNCATE_INVALID. The entire omission is emitted when its
 * grapheme count >= max_length, as in the pinned Lodash truncate contract.
 */
gut_truncate_status gut_truncate_graphemes_icu(
    const char *input,size_t max_length,const char *suffix,
    char *output,size_t capacity,size_t *required);
gut_truncate_status gut_truncate_graphemes_icu_default(
    const char *input,size_t max_length,
    char *output,size_t capacity,size_t *required);
#ifdef __cplusplus
}
#endif
#endif
