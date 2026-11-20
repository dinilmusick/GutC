#ifndef GUTC_S3_CLIENT_H
#define GUTC_S3_CLIENT_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Portable C11 host-binding counterpart to pinned GutTS /storage/s3
 * reexports of the EXTERNAL @aws-sdk/client-s3 JavaScript package.
 * This is NOT an implementation of that JS class hierarchy or AWS SigV4:
 * the host performs real endpoint IO, signing, retries and streaming.
 * Without host execute callback commands fail GUTC_UNSUPPORTED.
 * C API never receives AWS credentials or fabricates success.
 *
 * Inputs and request bodies are borrowed for the synchronous invocation.
 * Host transfers ownership of output metadata DOM to caller; any returned
 * binary buffer must have a matching release_body callback OR be allocated
 * via malloc (the default free() fallback). On failed command
 * all host results are cleaned before returning. Every result must be cleared
 * before its host context is destroyed. Caller must externally synchronize
 * a shared host/client, no global state or background tasks.
 */
typedef enum gutc_s3_command {
  GUTC_S3_GET_OBJECT,
  GUTC_S3_HEAD_OBJECT,
  GUTC_S3_LIST_OBJECTS_V2,
  GUTC_S3_PUT_OBJECT,
  GUTC_S3_COPY_OBJECT,
  GUTC_S3_DELETE_OBJECT,
  GUTC_S3_DELETE_OBJECTS,
  GUTC_S3_COMMAND_COUNT
} gutc_s3_command;
typedef struct gutc_s3_request {
  gutc_s3_command command;
  const gutc_json_dom *input; /* AWS SDK command-compatible JSON metadata */
  const uint8_t *body;size_t body_length; /* PutObject binary, optional */
} gutc_s3_request;
typedef struct gutc_s3_result {
  int http_status;
  gutc_json_dom *metadata; /* owned: gutc_json_dom_free */
  uint8_t *body;size_t body_length;
  void *release_context;
  void (*release_body)(void *context,uint8_t *data,size_t length);
} gutc_s3_result;
typedef gutc_status (*gutc_s3_host_execute)(
  void *ctx,const char *region,const char *endpoint,
  const gutc_s3_request *command,gutc_s3_result *result);
typedef struct gutc_s3_host {
  void *context;gutc_s3_host_execute execute;
} gutc_s3_host;
typedef struct gutc_s3_client {
  char region[128],endpoint[2304];
  gutc_s3_host host;
} gutc_s3_client;

/* Region required; endpoint may be empty for host's canonical AWS resolution.
 * No network operation occurs during init. */
gutc_status gutc_s3_client_init(gutc_s3_client *client,
  const char *region,size_t region_len,
  const char *endpoint,size_t endpoint_len,
  const gutc_s3_host *host);
const char *gutc_s3_command_name(gutc_s3_command command);
gutc_status gutc_s3_client_execute(gutc_s3_client *client,
  const gutc_s3_request *request,gutc_s3_result *result);
void gutc_s3_result_clear(gutc_s3_result *result);
#ifdef __cplusplus
}
#endif
#endif
