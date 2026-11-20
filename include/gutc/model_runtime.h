#ifndef GUTC_MODEL_RUNTIME_H
#define GUTC_MODEL_RUNTIME_H
#include "model_registry.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Synchronous, native C11 ModelResidency and ModelLibrary counterparts.
 * All actual model load/inference/unload operations MUST be supplied by a
 * real host runtime. Without callbacks they return GUTC_UNSUPPORTED. No
 * fake model scores, generated text, weights or network requests.
 *
 * The host owns the model handle returned by load until unload succeeds.
 * The native library owns the pinned metadata JSON tree for each library.
 * The host is borrowed/copied but its context must outlive the library.
 * Callbacks are synchronous, must not retain borrowed JSON, and must
 * allocate a GUTC JSON DOM result for successful invoke. Result ownership
 * passes to the caller, to be released with gutc_json_dom_free().
 *
 * A model residency may be re-entered synchronously; nested leases share
 * the loaded handle and final release unloads. Concurrent threads MUST
 * externally synchronize access to one residency/library. Promise-based
 * in-flight deduplication is a host-specific exception requiring an async
 * adapter. No background or resident agent is created.
 */
typedef enum gutc_model_size_kind {
 GUTC_MODEL_ARTIFACT_SIZE,GUTC_MODEL_PARAMETER_COUNT,GUTC_MODEL_SIZE_RANGE
} gutc_model_size_kind;
typedef enum gutc_model_size_unit {
 GUTC_MODEL_MB,GUTC_MODEL_GB,GUTC_MODEL_PARAMETERS
} gutc_model_size_unit;
typedef struct gutc_model_size {
 gutc_model_size_kind kind;
 gutc_model_size_unit unit;
 double value;
 double min_value,max_value;
} gutc_model_size;
typedef struct gutc_model_load_context {
 void *context;
 int (*is_cancelled)(void *context);
} gutc_model_load_context;

typedef gutc_status (*gutc_model_loader)(
 void *context,const gutc_json_dom *metadata,
 const gutc_model_load_context *load_context,void **out_model);
typedef gutc_status (*gutc_model_unloader)(void *context,void *model);
typedef gutc_status (*gutc_model_invoker)(
 void *context,void *model,const char *operation,
 const gutc_json_dom *input,gutc_json_dom **output);
typedef struct gutc_model_runtime_host {
 void *context;
 gutc_model_loader load;
 gutc_model_unloader unload;
 gutc_model_invoker invoke;
 gutc_model_load_context load_context;
} gutc_model_runtime_host;
typedef struct gutc_model_residency {
 void *model;
 size_t active_users;
 int cleanup_pending;
 gutc_model_runtime_host host;
} gutc_model_residency;
typedef struct gutc_model_lease {
 void *model;
 gutc_model_residency *owner;
 int active;
} gutc_model_lease;
typedef struct gutc_model_library {
 gutc_model_residency residency;
 gutc_model_runtime_host runtime;
 gutc_json_dom *metadata;
} gutc_model_library;

void gutc_model_residency_init(gutc_model_residency *residency);
int gutc_model_residency_is_loaded(const gutc_model_residency *residency);
size_t gutc_model_residency_users(const gutc_model_residency *residency);
gutc_status gutc_model_residency_acquire(
 gutc_model_residency *residency,const gutc_model_runtime_host *host,
 const gutc_json_dom *metadata,gutc_model_lease *lease);
gutc_status gutc_model_lease_release(gutc_model_lease *lease);
/* Retries a failed final unload; never discards an unclosed native handle. */
gutc_status gutc_model_residency_cleanup(gutc_model_residency *residency);

/* model_id must identify one of the exact eight pinned GutTS definitions.
 * Init can succeed with a NULL host to expose metadata; invoke must fail
 * explicitly without the real model runtime callbacks. */
gutc_status gutc_model_library_init(
 gutc_model_library *library,const char *model_id,size_t id_length,
 const gutc_model_runtime_host *host);
gutc_status gutc_model_library_invoke(
 gutc_model_library *library,const char *operation,
 const gutc_json_dom *input,gutc_json_dom **output);
gutc_status gutc_model_library_destroy(gutc_model_library *library);
#ifdef __cplusplus
}
#endif
#endif
