#ifndef GUTC_JOIN_URL_H
#define GUTC_JOIN_URL_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Joins base and segments using GutTS joinUrl slash trimming semantics.
 * Empty normalized components are omitted. Returns output length excluding NUL;
 * SIZE_MAX indicates invalid arguments or size overflow. On insufficient output
 * capacity the destination remains untouched. Components must not overlap out.
 */
size_t gutc_join_url(const char *base, const char *const *segments,
                     size_t count, char *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
