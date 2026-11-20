#ifndef GUTC_APPLICATION_AUTH_H
#define GUTC_APPLICATION_AUTH_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Native, first-party ApplicationAuthClient counterpart to pinned GutTS.
 * No OAuth code, tokens, verifiers, Luth cookies or credentials are exposed.
 * The owning host MUST enforce same-origin requests, no-store caching,
 * HttpOnly session cookie policy and real network navigation/fetch.
 * A test callback does not certify a live authentication deployment.
 *
 * All incoming pointers are borrowed and synchronous callbacks MUST NOT
 * retain them. request() transfers ownership of *response to this library;
 * on success get_session() transfers ownership to caller (free with
 * gutc_json_dom_free). Client is caller-owned and reentrant provided host
 * callbacks are. Errors are static borrowed codes, never heap allocated.
 */
typedef gutc_status (*gutc_app_auth_request_fn)(
  void *context,const char *method,const char *relative_path,
  const char *json_body,int same_origin_credentials,int no_store,
  int *http_status,gutc_json_dom **response);
typedef gutc_status (*gutc_app_auth_navigate_fn)(
  void *context,const char *relative_path);
typedef struct gutc_app_auth_host {
  void *context;
  gutc_app_auth_request_fn request;
  gutc_app_auth_navigate_fn navigate;
} gutc_app_auth_host;
typedef struct gutc_application_auth_client {
  char base_path[256];
  gutc_app_auth_host host;
} gutc_application_auth_client;

/* config_base_path NULL or empty selects /api/auth/oidc; no network IO.
 * Rejects nonlocal paths, trailing slash double, invalid characters.
 * A non-NULL error receives borrowed canonical GutTS error code. */
gutc_status gutc_application_auth_init(gutc_application_auth_client *client,
  const char *config_base_path,const gutc_app_auth_host *host,const char **error);

/* Starts actual navigation through host callback (no fake success).
 * return_to NULL/empty defaults to caller-provided current host path
 * or "/". Both are borrowed complete path+query+hash. */
gutc_status gutc_application_auth_start_sign_in(
  const gutc_application_auth_client *client,const char *return_to,
  const char *current_host_path,const char **error);

/* Calls host first-party GET /session with credentials same-origin and
 * cache no-store. 401 creates {authenticated:false,error:"SESSION_INVALID"}.
 * 2xx MUST contain authenticated=true, user.id/email, session.id;
 * malformed 2xx fails AUTH_SESSION_INVALID. Caller owns returned DOM. */
gutc_status gutc_application_auth_get_session(
  const gutc_application_auth_client *client,
  gutc_json_dom **session_result,const char **error);

/* POST /logout with JSON {}, then re-read GET /session and reject any
 * still-authenticated response; no pretend logout on missing host or failure. */
gutc_status gutc_application_auth_sign_out(
  const gutc_application_auth_client *client,const char **error);
#ifdef __cplusplus
}
#endif
#endif
