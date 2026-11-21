#ifndef GUTC_GOOGLE_DRIVE_H
#define GUTC_GOOGLE_DRIVE_H
#include "gutc.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit C11 Google Drive V3 REST equivalent for selected file methods.
 * The native library does NOT emulate the Node googleapis SDK class identity.
 * Trusted host executes actual HTTPS. No credentials are stored, and the
 * library never silently pretends to have network access or OAuth state. */
typedef enum gutc_drive_operation {
  GUTC_DRIVE_LIST_FILES,
  GUTC_DRIVE_GET_METADATA,
  GUTC_DRIVE_GET_MEDIA,
  GUTC_DRIVE_CREATE_METADATA,
  GUTC_DRIVE_UPLOAD_MEDIA,
  GUTC_DRIVE_UPDATE_METADATA,
  GUTC_DRIVE_UPDATE_MEDIA,
  GUTC_DRIVE_DELETE_FILE
} gutc_drive_operation;
typedef struct gutc_drive_request {
  gutc_drive_operation operation;
  const char *file_id;size_t file_id_length; /* required for get/update/delete */
  const char *filter;size_t filter_length;   /* list q, RFC3986 encoded */
  const char *page_token;size_t page_token_length; /* list pageToken */
  unsigned page_size; /* 0: service default; otherwise 1..1000 */
  const uint8_t *body;size_t body_length;    /* borrowed raw JSON/media */
  const char *content_type;                  /* optional MIME for media */
  const char *access_token;size_t access_token_length; /* borrowed OAuth bearer */
} gutc_drive_request;
typedef struct gutc_drive_http_request {
  const char *base_url; /* always https://www.googleapis.com */
  const char *method;   /* GET, POST, PATCH, DELETE */
  const char *path;     /* borrowed for duration of host invocation */
  const char *bearer;size_t bearer_length;  /* borrowed, never saved */
  const char *content_type; /* NULL for reads/deletes */
  const uint8_t *body;size_t body_length;   /* borrowed */
} gutc_drive_http_request;
typedef struct gutc_drive_response {
  int http_status; /* even 401/403/429 preserved; not treated as success */
  uint8_t *body;size_t body_length; /* host transfers ownership */
  void *release_context;
  void (*release_body)(void *context,uint8_t *buffer,size_t length);
} gutc_drive_response;
typedef gutc_status (*gutc_drive_http_execute)(
  void *context,const gutc_drive_http_request *request,
  gutc_drive_response *response);
typedef struct gutc_drive_host {
  void *context;
  gutc_drive_http_execute execute;
} gutc_drive_host;
typedef struct gutc_drive_client {
  gutc_drive_host host;
} gutc_drive_client;
/* Init accepts absent host, but execution then fails GUTC_UNSUPPORTED. */
gutc_status gutc_drive_client_init(gutc_drive_client *client,const gutc_drive_host *host);
/* Synchronous call; caller must provide HTTPS/OAuth/timeouts/retries.
 * GUTC_OK means transport ran; inspect response.http_status for HTTP outcome.
 * On callback failure, its partially allocated body is reclaimed.
 * Results must be cleared before the host context is destroyed. */
gutc_status gutc_drive_execute(const gutc_drive_client *client,
 const gutc_drive_request *request,gutc_drive_response *response);
void gutc_drive_response_clear(gutc_drive_response *response);
#ifdef __cplusplus
}
#endif
#endif
