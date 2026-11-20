#ifndef GUTC_AUTH_TYPES_H
#define GUTC_AUTH_TYPES_H
#include "luth_origin.h"
#include "oauth_contracts.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Borrowed, explicitly tagged native C11 counterparts to pinned GutTS auth
 * types (luthAuthTypes.ts, identityIds.ts, oauthClientContract.ts,
 * capabilityContract.ts). No struct validates a signed principal, owns a
 * credential, creates a session, authorizes a grant, or proves OAuth host
 * ownership. Server-side callers MUST authenticate and reauthorize every hop.
 * Every string/array is borrowed and has an explicit length/count.
 * Optional fields use present=0, distinguishing absent from empty.
 * Raw passwords/tokens MUST NOT be logged, serialized into traces or retained.
 */
typedef struct gutc_auth_text {const char *bytes;size_t length;} gutc_auth_text;
typedef struct gutc_auth_optional_text {int present;gutc_auth_text value;} gutc_auth_optional_text;
typedef struct gutc_auth_nullable_text {
 int present;int is_null;gutc_auth_text value;
} gutc_auth_nullable_text;
typedef struct gutc_auth_text_array {
 const gutc_auth_text *items;size_t count;
} gutc_auth_text_array;
typedef struct gutc_auth_optional_number {int present;double value;} gutc_auth_optional_number;
typedef struct gutc_auth_optional_boolean {int present;int value;} gutc_auth_optional_boolean;

typedef enum gutc_auth_identity_kind {
 GUTC_AUTH_APPLICATION,GUTC_AUTH_DEPLOYMENT,GUTC_AUTH_OAUTH_CLIENT,
 GUTC_AUTH_USER,GUTC_AUTH_WORKSPACE,GUTC_AUTH_UI_SESSION,
 GUTC_AUTH_IDENTITY_KIND_COUNT
} gutc_auth_identity_kind;
typedef struct gutc_auth_identity_id {gutc_auth_text value;} gutc_auth_identity_id;
typedef struct gutc_auth_application_id {gutc_auth_identity_id id;} gutc_auth_application_id;
typedef struct gutc_auth_deployment_id {gutc_auth_identity_id id;} gutc_auth_deployment_id;
typedef struct gutc_auth_oauth_client_id {gutc_auth_identity_id id;} gutc_auth_oauth_client_id;
typedef struct gutc_auth_user_id {gutc_auth_identity_id id;} gutc_auth_user_id;
typedef struct gutc_auth_workspace_id {gutc_auth_identity_id id;} gutc_auth_workspace_id;
typedef struct gutc_auth_ui_session_id {gutc_auth_identity_id id;} gutc_auth_ui_session_id;
typedef struct gutc_auth_identity_ref {
 gutc_auth_identity_kind kind;gutc_auth_identity_id id;
} gutc_auth_identity_ref;

/* Luth client/deployment session and credentials metadata types. */
typedef struct gutc_luth_deployment_record {
 gutc_auth_text id,url;
 gutc_auth_optional_text label;
 int has_environment;
 gutc_luth_environment environment;
 int is_active;
 gutc_auth_optional_boolean is_primary;
 gutc_auth_optional_number priority;
} gutc_luth_deployment_record;
typedef gutc_status (*gutc_luth_token_get)(
 void *context,char *buffer,size_t capacity,size_t *needed);
typedef gutc_status (*gutc_luth_token_set)(
 void *context,const char *token,size_t token_length);
typedef gutc_status (*gutc_luth_token_clear)(void *context);
typedef struct gutc_luth_auth_token_store {
 void *context;
 gutc_luth_token_get get;
 gutc_luth_token_set set;
 gutc_luth_token_clear clear;
} gutc_luth_auth_token_store;
typedef gutc_status (*gutc_luth_url_resolve)(
 void *context,gutc_auth_text_array *borrowed_urls);
typedef struct gutc_luth_auth_client_config {
 gutc_auth_text ecosystem_id,application_id;
 gutc_auth_optional_text luth_url;
 gutc_auth_text_array luth_urls;
 void *resolver_context;
 gutc_luth_url_resolve luth_url_resolver;
 gutc_auth_optional_text origin;
 int has_environment;
 gutc_luth_environment environment;
 gutc_auth_optional_boolean auto_register;
 gutc_auth_optional_number max_retries,retry_delay_ms,timeout_ms;
 const gutc_luth_auth_token_store *token_store;
} gutc_luth_auth_client_config;
typedef struct gutc_luth_registration_result {
 int registered;
 gutc_auth_text ecosystem_id,application_id,origin;
 gutc_luth_environment environment;
 int authentication_enabled;
} gutc_luth_registration_result;
typedef struct gutc_luth_user {
 gutc_auth_text id,name,email;
 gutc_auth_nullable_text image;
 gutc_auth_optional_boolean email_verified;
} gutc_luth_user;
typedef struct gutc_luth_session {
 gutc_auth_text id,user_id,expires_at;
 gutc_auth_optional_text token; /* trusted backend only, never browser */
} gutc_luth_session;
typedef struct gutc_luth_session_result {
 int authenticated;
 int has_user;gutc_luth_user user;
 int has_session;gutc_luth_session session;
 gutc_auth_optional_text error;
} gutc_luth_session_result;
typedef struct gutc_luth_email_sign_in {
 gutc_auth_text email,password;
} gutc_luth_email_sign_in;
typedef struct gutc_luth_email_sign_up {
 gutc_auth_text email,password,name;
} gutc_luth_email_sign_up;
typedef struct gutc_luth_google_sign_in {
 gutc_auth_optional_text callback_url;
} gutc_luth_google_sign_in;
typedef struct gutc_luth_auth_error {
 gutc_auth_text code,message;
 gutc_auth_optional_number http_status;
} gutc_luth_auth_error;

/* OAuth registration is untrusted metadata: Luth must verify ownership. */
typedef struct gutc_auth_oauth_registration_v1 {
 unsigned schema_version;
 gutc_auth_text app_id,client_id;
 gutc_oauth_platform platform;
 int public_client;
 gutc_auth_text pkce_method; /* S256, validated by OAuth contract */
 gutc_auth_text_array redirect_uris,verified_hosts;
} gutc_auth_oauth_registration_v1;
typedef struct gutc_auth_contract_check {
 int ok;
 const gutc_json_dom *borrowed_value;
 gutc_auth_optional_text reason;
} gutc_auth_contract_check;
typedef enum gutc_auth_parameter_kind {
 GUTC_AUTH_PARAM_STRING,GUTC_AUTH_PARAM_NUMBER,GUTC_AUTH_PARAM_INTEGER,
 GUTC_AUTH_PARAM_BOOLEAN,GUTC_AUTH_PARAM_OBJECT,GUTC_AUTH_PARAM_ARRAY,
 GUTC_AUTH_PARAM_KIND_COUNT
} gutc_auth_parameter_kind;
typedef struct gutc_auth_capability_parameter_v1 {
 gutc_auth_text name;
 gutc_auth_parameter_kind kind;
 int required;
} gutc_auth_capability_parameter_v1;
typedef struct gutc_auth_capability_base_v1 {
 unsigned schema_version;
 gutc_auth_text app_id,operation_id,revision,display_name;
 gutc_auth_text_array required_scopes;
 const gutc_auth_capability_parameter_v1 *input_parameters,*output_parameters;
 size_t input_parameter_count,output_parameter_count;
} gutc_auth_capability_base_v1;
typedef enum gutc_auth_capability_kind {
 GUTC_AUTH_BACKEND,GUTC_AUTH_FRONTEND,GUTC_AUTH_WORKFLOW,
 GUTC_AUTH_CAPABILITY_KIND_COUNT
} gutc_auth_capability_kind;
typedef enum gutc_auth_session_permission {
 GUTC_AUTH_INSPECT,GUTC_AUTH_ACT,GUTC_AUTH_SESSION_PERMISSION_COUNT
} gutc_auth_session_permission;
typedef struct gutc_auth_static_capability_v1 {
 gutc_auth_capability_base_v1 base;
 gutc_auth_capability_kind kind; /* backend or frontend only */
 gutc_auth_text deployment_id;
 int has_session_permission;
 gutc_auth_session_permission session_permission;
} gutc_auth_static_capability_v1;
typedef struct gutc_auth_workflow_publication_v1 {
 gutc_auth_capability_base_v1 base;
 gutc_auth_text workflow_id,owner_user_id,source_workspace_id;
 int published;
} gutc_auth_workflow_publication_v1;
typedef struct gutc_auth_capability_descriptor_v1 {
 gutc_auth_capability_kind kind;
 union {
  gutc_auth_static_capability_v1 static_capability;
  gutc_auth_workflow_publication_v1 workflow;
 } value;
} gutc_auth_capability_descriptor_v1;

/* Pure exact literal decoding, no authorization or coercion. */
const char *gutc_auth_identity_kind_name(gutc_auth_identity_kind kind);
int gutc_auth_identity_kind_parse(const char *value,size_t n,gutc_auth_identity_kind *out);
int gutc_auth_identity_ref_valid(const gutc_auth_identity_ref *reference);
const char *gutc_auth_parameter_kind_name(gutc_auth_parameter_kind kind);
int gutc_auth_parameter_kind_parse(const char *value,size_t n,gutc_auth_parameter_kind *out);
const char *gutc_auth_capability_kind_name(gutc_auth_capability_kind kind);
const char *gutc_auth_session_permission_name(gutc_auth_session_permission permission);
const char *gutc_auth_oauth_platform_name(gutc_oauth_platform platform);
int gutc_auth_deployment_environment_parse(const char *value,size_t n,gutc_luth_environment *out);
#ifdef __cplusplus
}
#endif
#endif
