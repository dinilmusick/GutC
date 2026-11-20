#ifndef GUTC_STRING_CASE_H
#define GUTC_STRING_CASE_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum gutc_string_case {
  GUTC_CASE_CAPITALIZE,GUTC_CASE_CAMEL,GUTC_CASE_KEBAB,GUTC_CASE_SNAKE
} gutc_string_case;
/* Locale-independent ASCII counterpart of Lodash case transforms.
 * Recognizes punctuation separators and acronym -> normal-word boundaries.
 * Unicode/deburr semantics require a Unicode host adapter: non-ASCII inputs
 * fail explicitly with GUTC_UNSUPPORTED, never silently corrupt bytes.
 * input is an explicit readable byte span; output is caller-owned and must
 * not overlap input. Sizing: output=NULL,capacity=0, needed excludes NUL.
 * No heap allocation, no global mutable state, no partial short-buffer write. */
gutc_status gutc_string_transform_ascii(const char *input,size_t length,
                                         gutc_string_case kind,
                                         char *output,size_t capacity,size_t *needed);
/* ASCII unquoted-email subset of GutTS validator.isEmail. A non-ASCII or
 * quoted local-part variant returns GUTC_UNSUPPORTED; conventional invalid
 * ASCII addresses return result=0 with GUTC_OK. No false success fallback.
 * No DNS resolution or verification of recipient ownership. */
gutc_status gutc_is_email_ascii(const char *input,size_t length,int *result);
#ifdef __cplusplus
}
#endif
#endif
