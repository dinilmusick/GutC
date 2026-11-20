#ifndef GUTC_OAUTH_CONTRACTS_H
#define GUTC_OAUTH_CONTRACTS_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum gutc_oauth_platform {
  GUTC_OAUTH_WEB,GUTC_OAUTH_IOS,GUTC_OAUTH_ANDROID,GUTC_OAUTH_DESKTOP
} gutc_oauth_platform;
/* Pure shape and redirect checks ONLY, never verify app/client ownership,
 * verified mobile domains, or authorize actual OAuth callbacks. Host must
 * independently verify ownership, exact registered callbacks and PKCE.
 * C allows canonical ASCII HTTP(S) URLs with DNS names/loopback; IDNA and
 * other WHATWG URL canonicalization remain explicit unsupported exceptions.
 * reason is static borrowed memory (NULL on success). */
gutc_status gutc_validate_oauth_redirect_v1(const gutc_json_dom *uri,
                                              gutc_oauth_platform platform,
                                              const gutc_json_dom *verified_hosts,
                                              const char **reason);
gutc_status gutc_validate_oauth_registration_v1(const gutc_json_dom *registration,
                                                  const char **reason);
#ifdef __cplusplus
}
#endif
#endif
