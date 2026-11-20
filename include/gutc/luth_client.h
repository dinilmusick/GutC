#ifndef GUTC_LUTH_CLIENT_H
#define GUTC_LUTH_CLIENT_H
#include "auth_types.h"
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Native, trusted-host LuthAuthClient. Explicit HTTP transport is mandatory.
 * Hosts MUST enforce TLS, same-origin/cookie policies, no-store, timeouts and
 * cancellation. This client never stores a token or password after a call.
 * Tokens are passed only to trusted host callbacks and token-store hooks.
 * Do not expose this privileged client in browser code; use application_auth.
 *
 * Callbacks are synchronous, may not retain request body/bearer pointers.
 * The transport transfers ownership of response->body to the caller.
 * response->auth_token is borrowed until the next transport callback; token
 * processing occurs before any further request. Missing transport is an
 * explicit unsupported error, never simulated success.
 */
#define GUTC_LUTH_MAX_ENDPOINTS 8u
#define GUTC_LUTH_MAX_BASE_URL 2304u
#define GUTC_LUTH_MAX_ID 256u
typedef struct gutc_luth_http_response {
  int status;
  gutc_json_dom *body; /* transfer ownership */
  gutc_auth_text auth_token; /* borrowed */
} gutc_luth_http_response;
typedef gutc_status (*gutc_luth_http_request)(
  void *ctx,const char *base_url,const char *method,const char *path,
  const char *json_body,const char *bearer_token,size_t bearer_length,
  unsigned timeout_ms,int include_credentials,int no_store,
  gutc_luth_http_response *response);
typedef gutc_status (*gutc_luth_delay)(void *ctx,unsigned milliseconds);
typedef struct gutc_luth_http_host {
  void *context;gutc_luth_http_request request;gutc_luth_delay sleep;
} gutc_luth_http_host;
typedef struct gutc_luth_client {
  char ecosystem_id[GUTC_LUTH_MAX_ID],application_id[GUTC_LUTH_MAX_ID];
  char origin[GUTC_LUTH_MAX_BASE_URL];
  char urls[GUTC_LUTH_MAX_ENDPOINTS][GUTC_LUTH_MAX_BASE_URL];
  size_t url_count,active_index,next_index;
  gutc_luth_environment environment;
  unsigned max_retries,retry_delay_ms,timeout_ms;
  int auto_register,registered;
  gutc_luth_auth_token_store token_store;
  gutc_luth_http_host http;
} gutc_luth_client;

/* src/auth/luthAuthClient.ts: env inferred by canonical native origin helper.
 * Config strings are copied at init, stack-lifetime is not retained.
 * URL resolver cannot be called implicitly; callers must refresh URLs through
 * gutc_luth_client_add_url(). No global browser location/sessionStorage.
 */
gutc_status gutc_luth_client_init(gutc_luth_client *client,
  const gutc_luth_auth_client_config *config,const char *host_runtime_origin,
  const gutc_luth_http_host *host);
gutc_status gutc_luth_client_add_url(gutc_luth_client *client,
  const char *url,size_t length);
void gutc_luth_client_destroy(gutc_luth_client *client);
gutc_status gutc_luth_client_register(gutc_luth_client *client,
  gutc_json_dom **registration);
gutc_status gutc_luth_client_get_session(gutc_luth_client *client,
  gutc_json_dom **session);
gutc_status gutc_luth_client_refresh_session(gutc_luth_client *client,
  gutc_json_dom **session);
gutc_status gutc_luth_client_sign_in_email(gutc_luth_client *client,
  const gutc_luth_email_sign_in *credentials,gutc_json_dom **result);
gutc_status gutc_luth_client_sign_up_email(gutc_luth_client *client,
  const gutc_luth_email_sign_up *credentials,gutc_json_dom **result);
gutc_status gutc_luth_client_sign_in_google(gutc_luth_client *client,
  const gutc_luth_google_sign_in *options,gutc_json_dom **result);
gutc_status gutc_luth_client_sign_out(gutc_luth_client *client);
gutc_status gutc_luth_client_clear_token(gutc_luth_client *client);
#ifdef __cplusplus
}
#endif
#endif
