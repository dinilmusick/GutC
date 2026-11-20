#ifndef GUTC_PORTABLE_TYPES_H
#define GUTC_PORTABLE_TYPES_H
#include "json_dom.h"
#include "semantic_cache.h"
#include "semantic_rank.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed source-pinned C11 views for GutTS JSON, API and semantic types.
 * Optional fields use present=0. JSON DOM pointers remain caller-owned.
 * No inference, authorization, agent spawning, IO or implicit allocation. */
typedef struct gutc_type_text {const char *bytes;size_t length;} gutc_type_text;
typedef struct gutc_type_optional_text {int present;gutc_type_text value;} gutc_type_optional_text;
typedef struct gutc_type_text_array {const gutc_type_text *items;size_t count;} gutc_type_text_array;
typedef struct gutc_type_optional_double {int present;double value;} gutc_type_optional_double;
typedef struct gutc_type_optional_boolean {int present;int value;} gutc_type_optional_boolean;
typedef gutc_json_dom gutc_type_json_primitive;
typedef gutc_json_dom gutc_type_json_value;
typedef gutc_json_dom gutc_type_json_object;
int gutc_type_is_json_primitive(const gutc_json_dom *value);
int gutc_type_is_json_object(const gutc_json_dom *value);

typedef struct gutc_type_json_schema gutc_type_json_schema;
typedef struct gutc_type_schema_property {
 gutc_type_text name;const gutc_type_json_schema *schema;
} gutc_type_schema_property;
struct gutc_type_json_schema {
 gutc_type_optional_text type,description;
 const gutc_type_schema_property *properties;size_t property_count;
 const gutc_type_json_schema *items;
 gutc_type_text_array required;
 gutc_type_optional_boolean additional_properties;
 const gutc_json_dom *const *enum_values;size_t enum_count;
};
typedef struct gutc_type_api_parameter {
 gutc_type_text name,description;
 int required;
 gutc_type_json_schema schema;
} gutc_type_api_parameter;
typedef struct gutc_type_api_semantics {
 int deterministic,pure,side_effects;
} gutc_type_api_semantics;
typedef struct gutc_type_api_example {
 const gutc_json_dom *input_object,*output;
} gutc_type_api_example;
typedef struct gutc_type_api_definition {
 gutc_type_text id,name,category,description;
 const gutc_type_api_parameter *parameters;size_t parameter_count;
 gutc_type_json_schema output;
 gutc_type_api_semantics semantics;
 gutc_type_text_array errors;
 const gutc_type_api_example *examples;size_t example_count;
 gutc_type_json_schema input_schema;
} gutc_type_api_definition;

typedef enum gutc_semantic_error_code {
 GUTC_SEMANTIC_EMPTY_INPUT,GUTC_SEMANTIC_MODEL_FAILURE,
 GUTC_SEMANTIC_MODEL_OUTPUT_EMPTY,GUTC_SEMANTIC_SCHEMA_MISMATCH,
 GUTC_SEMANTIC_AMBIGUOUS_CANDIDATE,GUTC_SEMANTIC_UNSUPPORTED_TYPE,
 GUTC_SEMANTIC_ABORTED,GUTC_SEMANTIC_ERROR_CODE_COUNT
} gutc_semantic_error_code;
typedef enum gutc_semantic_risk {
 GUTC_SEMANTIC_RISK_LOW,GUTC_SEMANTIC_RISK_MEDIUM,
 GUTC_SEMANTIC_RISK_HIGH,GUTC_SEMANTIC_RISK_COUNT
} gutc_semantic_risk;
typedef enum gutc_semantic_phase {
 GUTC_SEMANTIC_PHASE_STARTED,GUTC_SEMANTIC_PHASE_DETERMINISTIC,
 GUTC_SEMANTIC_PHASE_RANKING,GUTC_SEMANTIC_PHASE_STRUCTURED,
 GUTC_SEMANTIC_PHASE_COMPLETED,GUTC_SEMANTIC_PHASE_COUNT
} gutc_semantic_phase;
typedef enum gutc_semantic_event_type {
 GUTC_SEMANTIC_EVENT_STARTED,GUTC_SEMANTIC_EVENT_DETERMINISTIC,
 GUTC_SEMANTIC_EVENT_RESOLVED,GUTC_SEMANTIC_EVENT_AMBIGUOUS,
 GUTC_SEMANTIC_EVENT_CLARIFICATION_REQUIRED,GUTC_SEMANTIC_EVENT_INVALID,
 GUTC_SEMANTIC_EVENT_FAILED,GUTC_SEMANTIC_EVENT_TYPE_COUNT
} gutc_semantic_event_type;
typedef enum gutc_semantic_resolution_status {
 GUTC_SEMANTIC_RESOLVED,GUTC_SEMANTIC_AMBIGUOUS,GUTC_SEMANTIC_INVALID,
 GUTC_SEMANTIC_UNSUPPORTED,GUTC_SEMANTIC_UNRESOLVED,
 GUTC_SEMANTIC_FAILED,GUTC_SEMANTIC_RESOLUTION_STATUS_COUNT
} gutc_semantic_resolution_status;

typedef struct gutc_semantic_schema gutc_semantic_schema;
typedef struct gutc_semantic_schema_property {
 gutc_type_text key;const gutc_semantic_schema *value;
} gutc_semantic_schema_property;
struct gutc_semantic_schema {
 gutc_type_text type;
 const gutc_semantic_schema_property *properties;size_t property_count;
 gutc_type_text_array required;
 const gutc_json_dom *const *enum_values;size_t enum_count;
 const gutc_semantic_schema *items;
 gutc_type_optional_text description;
 const gutc_json_dom *additional_schema_fields;
};
typedef struct gutc_semantic_context {
 gutc_type_optional_text application,workspace,page,section,form,field,label;
 gutc_type_optional_text description,semantic_type,locale;
 gutc_type_text_array vocabulary,examples,aliases,constraints,related_fields;
} gutc_semantic_context;
typedef struct gutc_semantic_progress {
 gutc_semantic_phase phase;gutc_type_optional_text detail;
} gutc_semantic_progress;
typedef struct gutc_semantic_event {
 gutc_semantic_event_type type;
 gutc_type_optional_text input,question;
 const gutc_json_dom *value;gutc_type_text_array candidates;
 gutc_semantic_error_code code;
} gutc_semantic_event;
typedef gutc_semantic_ranking_policy gutc_semantic_model_policy;
typedef struct gutc_semantic_policy {
 int has_risk;gutc_semantic_risk risk;
 gutc_type_optional_boolean require_confirmation;
 int has_model_policy;gutc_semantic_model_policy model_policy;
} gutc_semantic_policy;
typedef struct gutc_semantic_provenance {
 gutc_type_text request_fingerprint,resolver;
 gutc_semantic_model_policy model_policy;
 gutc_type_text models[3];
 int deterministic;
} gutc_semantic_provenance;
typedef struct gutc_semantic_candidate {
 gutc_type_text value;
 gutc_type_optional_double similarity,relevance;
} gutc_semantic_candidate;
typedef struct gutc_semantic_evidence {
 const gutc_json_dom *needle_output;
 const gutc_semantic_candidate *candidates;size_t candidate_count;
 gutc_type_optional_text selected_candidate;
 gutc_type_optional_double similarity_threshold,relevance_margin_threshold;
} gutc_semantic_evidence;
typedef struct gutc_semantic_clarification {
 gutc_type_text question;gutc_type_text_array candidates;
} gutc_semantic_clarification;
typedef struct gutc_semantic_resolution_error {
 gutc_semantic_error_code code;gutc_type_text message;
} gutc_semantic_resolution_error;
typedef struct gutc_semantic_resolution {
 gutc_semantic_resolution_status status;
 const gutc_json_dom *value,*model_output;
 gutc_type_optional_text raw_input;
 const gutc_semantic_candidate *candidates;size_t candidate_count;
 int has_evidence;gutc_semantic_evidence evidence;
 int has_clarification;gutc_semantic_clarification clarification;
 int has_error;gutc_semantic_resolution_error error;
 gutc_type_optional_boolean requires_confirmation;
 int has_provenance;gutc_semantic_provenance provenance;
} gutc_semantic_resolution;
typedef gutc_type_text gutc_semantic_cache_key;
typedef struct gutc_semantic_provenance_record {
 gutc_semantic_provenance provenance;
 gutc_semantic_resolution_status status;
 gutc_type_text timestamp;
} gutc_semantic_provenance_record;
typedef gutc_status (*gutc_semantic_provenance_append)(
 void *context,const gutc_semantic_provenance_record *record);
typedef struct gutc_semantic_provenance_store {
 void *context;gutc_semantic_provenance_append append;
} gutc_semantic_provenance_store;
typedef void (*gutc_semantic_event_callback)(
 void *context,const gutc_semantic_event *event);
typedef void (*gutc_semantic_progress_callback)(
 void *context,const gutc_semantic_progress *progress);
typedef struct gutc_semantic_request {
 gutc_type_text input;
 gutc_semantic_schema schema;
 const gutc_semantic_context *context;
 gutc_type_text_array candidates;
 gutc_type_optional_boolean preserve_raw_input;
 gutc_type_optional_double min_candidate_similarity,min_candidate_margin;
 const gutc_semantic_policy *policy;
 gutc_semantic_cache *cache;
 void *signal_context;int (*is_aborted)(void *signal_context);
 void *events_context;gutc_semantic_event_callback on_event;
 void *progress_context;gutc_semantic_progress_callback on_progress;
 const gutc_semantic_provenance_store *provenance_store;
} gutc_semantic_request;
const char *gutc_semantic_error_code_name(gutc_semantic_error_code code);
const char *gutc_semantic_risk_name(gutc_semantic_risk risk);
const char *gutc_semantic_phase_name(gutc_semantic_phase phase);
const char *gutc_semantic_event_name(gutc_semantic_event_type event);
const char *gutc_semantic_resolution_status_name(gutc_semantic_resolution_status status);
const char *gutc_semantic_model_policy_name(gutc_semantic_model_policy policy);
int gutc_semantic_error_code_parse(const char *name,size_t length,gutc_semantic_error_code *out);
#ifdef __cplusplus
}
#endif
#endif
