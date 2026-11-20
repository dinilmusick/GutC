#ifndef GUTC_ENGINEERING_TYPES_H
#define GUTC_ENGINEERING_TYPES_H
#include "gutc.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Borrowed, immutable C structural counterparts to the pinned GutTS
 * engineering-types.ts public interfaces (not transport authorization).
 * Strings and arrays are UTF-8 length-aware views; ownership remains with
 * the caller. Optional fields use present=0 to distinguish omitted from "".
 * No function in this header executes commands, grants capabilities or
 * authenticates principals. Host executors must enforce authorization.
 */
typedef struct gutc_eng_text {const char *bytes;size_t length;} gutc_eng_text;
typedef gutc_eng_text gutc_eng_identifier;
typedef struct gutc_eng_optional_text {int present;gutc_eng_text value;} gutc_eng_optional_text;
typedef struct gutc_eng_text_array {const gutc_eng_text *values;size_t count;} gutc_eng_text_array;
typedef struct gutc_eng_optional_number {int present;double value;} gutc_eng_optional_number;

typedef enum gutc_eng_provider_kind {
 GUTC_ENG_GITHUB,GUTC_ENG_GITLAB,GUTC_ENG_BITBUCKET,GUTC_ENG_GIT,
 GUTC_ENG_LOCAL,GUTC_ENG_CONTAINER,GUTC_ENG_WEBCONTAINER,
 GUTC_ENG_REMOTE_SANDBOX,GUTC_ENG_CI,GUTC_ENG_PROVIDER_COUNT
} gutc_eng_provider_kind;

typedef enum gutc_eng_capability {
 GUTC_ENG_REPOSITORY_READ,GUTC_ENG_REPOSITORY_WRITE,GUTC_ENG_GIT_WRITE,
 GUTC_ENG_WORKSPACE_CREATE,GUTC_ENG_EXECUTION_RUN,GUTC_ENG_BROWSER_RUN,
 GUTC_ENG_VERIFICATION_RUN,GUTC_ENG_ARTIFACT_WRITE,
 GUTC_ENG_PACKAGE_PUBLISH,GUTC_ENG_DEPLOYMENT_WRITE,
 GUTC_ENG_PROMOTION_WRITE,GUTC_ENG_CAPABILITY_COUNT
} gutc_eng_capability;

typedef enum gutc_eng_network {
 GUTC_ENG_NETWORK_NONE,GUTC_ENG_NETWORK_RESTRICTED,GUTC_ENG_NETWORK_FULL,
 GUTC_ENG_NETWORK_COUNT
} gutc_eng_network;
typedef enum gutc_eng_execution_status {
 GUTC_ENG_EXECUTION_SUCCEEDED,GUTC_ENG_EXECUTION_FAILED,
 GUTC_ENG_EXECUTION_CANCELLED,GUTC_ENG_EXECUTION_TIMED_OUT,
 GUTC_ENG_EXECUTION_STATUS_COUNT
} gutc_eng_execution_status;
typedef enum gutc_eng_check_status {
 GUTC_ENG_CHECK_PASSED,GUTC_ENG_CHECK_FAILED,GUTC_ENG_CHECK_SKIPPED,
 GUTC_ENG_CHECK_BLOCKED,GUTC_ENG_CHECK_STATUS_COUNT
} gutc_eng_check_status;
typedef enum gutc_eng_report_status {
 GUTC_ENG_REPORT_PASSED,GUTC_ENG_REPORT_FAILED,GUTC_ENG_REPORT_BLOCKED,
 GUTC_ENG_REPORT_STATUS_COUNT
} gutc_eng_report_status;

typedef struct gutc_eng_repository_ref {
 gutc_eng_identifier id;
 gutc_eng_provider_kind provider;
 gutc_eng_optional_text owner;
 gutc_eng_text name;
 gutc_eng_optional_text default_branch;
 gutc_eng_optional_text remote;
} gutc_eng_repository_ref;
typedef struct gutc_eng_workspace_ref {
 gutc_eng_identifier id;
 gutc_eng_text root;
 const gutc_eng_repository_ref *repositories;
 size_t repository_count;
 gutc_eng_optional_text snapshot_id;
} gutc_eng_workspace_ref;
typedef struct gutc_eng_environment_pair {
 gutc_eng_text key;
 gutc_eng_text value;
} gutc_eng_environment_pair;
typedef struct gutc_eng_resources {
 gutc_eng_optional_number cpu;
 gutc_eng_optional_number memory_mb;
 gutc_eng_optional_number disk_mb;
} gutc_eng_resources;
typedef struct gutc_eng_execution_request {
 gutc_eng_identifier workspace_id;
 gutc_eng_text command;
 gutc_eng_optional_text cwd;
 const gutc_eng_environment_pair *environment;
 size_t environment_count;
 gutc_eng_optional_number timeout_ms;
 int has_network;
 gutc_eng_network network;
 int has_resources;
 gutc_eng_resources resources;
} gutc_eng_execution_request;
typedef struct gutc_eng_execution_result {
 gutc_eng_identifier execution_id;
 gutc_eng_execution_status status;
 gutc_eng_optional_number exit_code;
 gutc_eng_text stdout_text;
 gutc_eng_text stderr_text;
 gutc_eng_text_array artifact_ids;
} gutc_eng_execution_result;
typedef struct gutc_eng_verification_check {
 gutc_eng_identifier id;
 gutc_eng_text name;
 gutc_eng_check_status status;
 gutc_eng_optional_number duration_ms;
 gutc_eng_optional_text execution_id;
 gutc_eng_optional_text details;
} gutc_eng_verification_check;
typedef struct gutc_eng_verification_report {
 gutc_eng_identifier id;
 gutc_eng_report_status status;
 const gutc_eng_verification_check *checks;
 size_t check_count;
 gutc_eng_text_array artifact_ids;
} gutc_eng_verification_report;
typedef struct gutc_eng_artifact_ref {
 gutc_eng_identifier id;
 gutc_eng_text digest;
 gutc_eng_text type;
 gutc_eng_optional_text uri;
 gutc_eng_optional_text source_execution_id;
} gutc_eng_artifact_ref;
typedef struct gutc_eng_capability_grant {
 gutc_eng_capability capability;
 gutc_eng_optional_text resource;
 gutc_eng_optional_text expires_at;
} gutc_eng_capability_grant;
typedef struct gutc_eng_operation_context {
 gutc_eng_identifier task_id;
 gutc_eng_identifier trace_id;
 gutc_eng_optional_text workspace_id;
 const gutc_eng_capability_grant *grants;
 size_t grant_count;
} gutc_eng_operation_context;
typedef struct gutc_eng_provenance_record {
 gutc_eng_identifier operation_id;
 gutc_eng_identifier actor_id;
 gutc_eng_identifier task_id;
 gutc_eng_identifier trace_id;
 gutc_eng_text timestamp;
 int has_source;
 gutc_eng_repository_ref source;
 gutc_eng_optional_text commit;
 gutc_eng_text_array artifacts;
} gutc_eng_provenance_record;

/* Exact source-pinned tagged-union literals. Returns NULL for invalid enum.
 * Unknown/empty byte sequences reject, never coerce to a valid permission. */
const char *gutc_eng_provider_name(gutc_eng_provider_kind kind);
int gutc_eng_provider_parse(const char *value,size_t length,gutc_eng_provider_kind *out);
const char *gutc_eng_capability_name(gutc_eng_capability kind);
int gutc_eng_capability_parse(const char *value,size_t length,gutc_eng_capability *out);
const char *gutc_eng_network_name(gutc_eng_network value);
const char *gutc_eng_execution_status_name(gutc_eng_execution_status value);
const char *gutc_eng_check_status_name(gutc_eng_check_status value);
const char *gutc_eng_report_status_name(gutc_eng_report_status value);
#ifdef __cplusplus
}
#endif
#endif
