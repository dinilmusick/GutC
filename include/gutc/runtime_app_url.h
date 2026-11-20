#ifndef GUTC_RUNTIME_APP_URL_H
#define GUTC_RUNTIME_APP_URL_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Portable host-injected equivalents of GutTS resolveRuntimeAppUrl and
 * resolveServerRuntimeAppUrl. Reading browser location or process.env must
 * be done by the calling host; no hidden environment or browser access.
 *
 * Host-provided UTF-8 C strings are trimmed with ECMAScript String.trim
 * whitespace, preserving all internal bytes. NULL is an absent host value.
 * output=NULL,capacity=0 probes needed bytes (excluding NUL).
 * Insufficient output capacity never writes a partial result.
 * Max input 8191 bytes; malformed UTF-8 fails explicitly.
 * No allocation, global state or IO; reentrant and locale independent. */
gutc_status gutc_resolve_runtime_app_url(const char *host_location_origin,
                                          char *output,size_t capacity,size_t *needed);
gutc_status gutc_resolve_server_runtime_app_url(const char *host_app_url,
                                                 char *output,size_t capacity,size_t *needed);
#ifdef __cplusplus
}
#endif
#endif
