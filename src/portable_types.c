#include "gutc/portable_types.h"
#include <math.h>
#include <string.h>
static const char *const semantic_errors[GUTC_SEMANTIC_ERROR_CODE_COUNT]={
 "EMPTY_INPUT","MODEL_FAILURE","MODEL_OUTPUT_EMPTY","SCHEMA_MISMATCH",
 "AMBIGUOUS_CANDIDATE","UNSUPPORTED_TYPE","ABORTED"
};
static const char *const risks[GUTC_SEMANTIC_RISK_COUNT]={
 "low","medium","high"
};
static const char *const phases[GUTC_SEMANTIC_PHASE_COUNT]={
 "started","deterministic","ranking","structured","completed"
};
static const char *const events[GUTC_SEMANTIC_EVENT_TYPE_COUNT]={
 "semantic.started","semantic.deterministic","semantic.resolved",
 "semantic.ambiguous","semantic.clarification.required","semantic.invalid",
 "semantic.failed"
};
static const char *const statuses[GUTC_SEMANTIC_RESOLUTION_STATUS_COUNT]={
 "resolved","ambiguous","invalid","unsupported","unresolved","failed"
};
static const char *const policies[]={"fast","balanced","accurate"};
int gutc_type_is_json_primitive(const gutc_json_dom *v){
 if(!v)return 0;
 switch(v->kind){
 case GUTC_JSON_NULL:case GUTC_JSON_BOOLEAN:case GUTC_JSON_STRING:return 1;
 case GUTC_JSON_NUMBER:return isfinite(v->as.number)!=0;
 default:return 0;
 }
}
int gutc_type_is_json_object(const gutc_json_dom *v){
 return v&&v->kind==GUTC_JSON_OBJECT&&gutc_json_dom_is_json_value(v);
}
const char *gutc_semantic_error_code_name(gutc_semantic_error_code c){
 return (unsigned)c<GUTC_SEMANTIC_ERROR_CODE_COUNT?semantic_errors[(size_t)c]:NULL;
}
const char *gutc_semantic_risk_name(gutc_semantic_risk c){
 return (unsigned)c<GUTC_SEMANTIC_RISK_COUNT?risks[(size_t)c]:NULL;
}
const char *gutc_semantic_phase_name(gutc_semantic_phase c){
 return (unsigned)c<GUTC_SEMANTIC_PHASE_COUNT?phases[(size_t)c]:NULL;
}
const char *gutc_semantic_event_name(gutc_semantic_event_type c){
 return (unsigned)c<GUTC_SEMANTIC_EVENT_TYPE_COUNT?events[(size_t)c]:NULL;
}
const char *gutc_semantic_resolution_status_name(gutc_semantic_resolution_status c){
 return (unsigned)c<GUTC_SEMANTIC_RESOLUTION_STATUS_COUNT?statuses[(size_t)c]:NULL;
}
const char *gutc_semantic_model_policy_name(gutc_semantic_model_policy c){
 return (unsigned)c<3u?policies[(size_t)c]:NULL;
}
int gutc_semantic_error_code_parse(const char *name,size_t length,gutc_semantic_error_code *out){
 size_t i;
 if(!out||(!name&&length))return 0;
 for(i=0;i<GUTC_SEMANTIC_ERROR_CODE_COUNT;i++){
  const size_t n=strlen(semantic_errors[i]);
  if(n==length&&(!n||memcmp(semantic_errors[i],name,n)==0)){
   *out=(gutc_semantic_error_code)i;
   return 1;
  }
 }
 return 0;
}
