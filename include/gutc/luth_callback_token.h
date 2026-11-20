#ifndef GUTC_LUTH_CALLBACK_TOKEN_H
#define GUTC_LUTH_CALLBACK_TOKEN_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Portable, host-integrated counterpart to GutTS hydrateLuthTokenFromUrl.
 *
 * The host supplies ALREADY-PARSED canonical location.pathname, .search and
 * .hash, plus real storage and history callbacks. Native code never reads
 * browser globals or authenticates a token. It extracts the first token
 * URLSearchParams value, trims ECMAScript whitespace, removes ALL token query
 * entries, serializes the other parameters as URLSearchParams and dispatches:
 *   1. store_token synchronously (must COPY borrowed ephemeral token)
 *   2. replace_history synchronously (must COPY borrowed relative URL)
 *
 * hydrated=1 only if both callbacks report success.
 * URLSearchParams malformed percent/UTF-8 replacement behavior is not
 * emulated; those cases fail explicitly. Path/hash are assumed host-canonical.
 * Output is never persisted by this library. On history callback failure the
 * token-store side effect may already have occurred, matching JS action order.
 * No hidden IO, globals, URL authority assumptions or token logging.
 * All inputs are NUL-terminated bounded strings of at most 8191 bytes. */
typedef gutc_status (*gutc_luth_store_token_callback)(
  void *context,const char *token,size_t token_bytes);
typedef gutc_status (*gutc_luth_history_replace_callback)(
  void *context,const char *relative_url,size_t url_bytes);
typedef struct gutc_luth_callback_host {
  void *context;
  gutc_luth_store_token_callback store_token;
  gutc_luth_history_replace_callback replace_history;
} gutc_luth_callback_host;
gutc_status gutc_luth_hydrate_callback_token(
  const char *pathname,const char *search,const char *hash,
  const gutc_luth_callback_host *host,int *hydrated);
#ifdef __cplusplus
}
#endif
#endif
