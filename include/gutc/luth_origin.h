#ifndef GUTC_LUTH_ORIGIN_H
#define GUTC_LUTH_ORIGIN_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Portable, pure counterpart of GutTS resolveClientOrigin and
 * resolveClientEnvironment for canonical ASCII http(s) origins.
 * All text buffers are caller-owned; no browser globals are read.
 * Passing NULL explicit_origin models an omitted optional argument.
 * runtime_origin is a borrowed host-supplied location.origin fallback.
 * The parser normalizes protocol/hostname case, elides default ports,
 * excludes path/query/fragment, validates numeric ports and IPv6 brackets.
 * JavaScript-specific opaque schemes, IDNA, credentials and legacy host
 * interpretation are deliberately GUTC_UNSUPPORTED, never guessed.
 *
 * output=NULL,capacity=0 probes required byte length (excludes NUL).
 * A too-small output is unchanged. Inputs must not overlap outputs.
 * All functions are reentrant, allocation-free and locale-independent. */
typedef enum gutc_luth_environment {
  GUTC_LUTH_DEVELOPMENT,GUTC_LUTH_STAGING,
  GUTC_LUTH_PREVIEW,GUTC_LUTH_PRODUCTION
} gutc_luth_environment;
const char *gutc_luth_environment_name(gutc_luth_environment value);
const char *gutc_luth_default_url(void);
gutc_status gutc_luth_normalize_origin(const char *input,
                                       char *output,size_t capacity,size_t *needed);
gutc_status gutc_luth_resolve_client_origin(const char *explicit_origin,
                                            const char *runtime_origin,
                                            char *output,size_t capacity,size_t *needed);
gutc_status gutc_luth_resolve_client_environment(const char *origin,
                                                  gutc_luth_environment *out);
#ifdef __cplusplus
}
#endif
#endif
