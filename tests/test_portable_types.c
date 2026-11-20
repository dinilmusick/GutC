#include "gutc/portable_types.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void verify_row(const char *group,const char *literal,size_t index){
 const char *actual=NULL;
 if(!strcmp(group,"error")){
  gutc_semantic_error_code parsed=GUTC_SEMANTIC_ABORTED;
  assert(gutc_semantic_error_code_parse(literal,strlen(literal),&parsed));
  assert((size_t)parsed==index);
  actual=gutc_semantic_error_code_name(parsed);
 }else if(!strcmp(group,"risk")){
  actual=gutc_semantic_risk_name((gutc_semantic_risk)index);
 }else if(!strcmp(group,"phase")){
  actual=gutc_semantic_phase_name((gutc_semantic_phase)index);
 }else if(!strcmp(group,"event")){
  actual=gutc_semantic_event_name((gutc_semantic_event_type)index);
 }else if(!strcmp(group,"status")){
  actual=gutc_semantic_resolution_status_name((gutc_semantic_resolution_status)index);
 }else if(!strcmp(group,"policy")){
  actual=gutc_semantic_model_policy_name((gutc_semantic_model_policy)index);
 }else abort();
 if(!actual||strcmp(actual,literal))
  fprintf(stderr,"Semantic %s index %zu: got %s expected %s\n",group,index,
          actual?actual:"NULL",literal);
 assert(actual&&!strcmp(actual,literal));
}
static void type_struct_smoke(void){
 const gutc_type_text input={"hello",5};
 const gutc_type_optional_text description={1,{"desc",4}};
 const gutc_type_text_array names={&input,1};
 const gutc_type_optional_boolean truth={1,1};
 const gutc_type_optional_double one={1,1.0};
 gutc_type_json_schema schema={0};
 gutc_type_schema_property property={0};
 gutc_type_api_parameter parameter={0};
 gutc_type_api_semantics semantics={1,1,0};
 gutc_type_api_example example={0};
 gutc_type_api_definition definition={0};
 gutc_semantic_schema sschema={0};
 gutc_semantic_schema_property sproperty={0};
 gutc_semantic_context context={0};
 gutc_semantic_progress progress={0};
 gutc_semantic_event event={0};
 gutc_semantic_policy policy={0};
 gutc_semantic_provenance provenance={0};
 gutc_semantic_candidate candidate={0};
 gutc_semantic_evidence evidence={0};
 gutc_semantic_clarification clarification={0};
 gutc_semantic_resolution_error error={0};
 gutc_semantic_resolution resolution={0};
 gutc_semantic_provenance_record record={0};
 gutc_semantic_provenance_store store={0};
 gutc_semantic_request request={0};
 gutc_semantic_cache_key key=input;
 schema.type=description;schema.description=description;
 schema.required=names;schema.additional_properties=truth;
 property.name=input;property.schema=&schema;
 schema.properties=&property;schema.property_count=1;
 parameter.name=input;parameter.description=input;parameter.required=1;
 parameter.schema=schema;
 example.input_object=NULL;example.output=NULL;
 definition.id=input;definition.name=input;
 definition.category=input;definition.description=input;
 definition.parameters=&parameter;definition.parameter_count=1;
 definition.output=schema;definition.semantics=semantics;
 definition.input_schema=schema;definition.errors=names;
 definition.examples=&example;definition.example_count=1;
 sschema.type=input;sschema.required=names;sschema.description=description;
 sproperty.key=input;sproperty.value=&sschema;
 sschema.properties=&sproperty;sschema.property_count=1;
 context.application=description;context.vocabulary=names;
 progress.phase=GUTC_SEMANTIC_PHASE_STARTED;progress.detail=description;
 event.type=GUTC_SEMANTIC_EVENT_STARTED;event.input=description;
 event.code=GUTC_SEMANTIC_EMPTY_INPUT;event.candidates=names;
 policy.has_risk=1;policy.risk=GUTC_SEMANTIC_RISK_HIGH;
 policy.require_confirmation=truth;policy.has_model_policy=1;
 policy.model_policy=GUTC_SEMANTIC_ACCURATE;
 provenance.request_fingerprint=input;provenance.resolver=input;
 provenance.model_policy=GUTC_SEMANTIC_BALANCED;
 provenance.models[0]=input;provenance.models[1]=input;
 provenance.models[2]=input;provenance.deterministic=1;
 candidate.value=input;candidate.similarity=one;candidate.relevance=one;
 evidence.candidates=&candidate;evidence.candidate_count=1;
 evidence.selected_candidate=description;
 clarification.question=input;clarification.candidates=names;
 error.code=GUTC_SEMANTIC_EMPTY_INPUT;error.message=input;
 resolution.status=GUTC_SEMANTIC_RESOLVED;
 resolution.candidates=&candidate;resolution.candidate_count=1;
 resolution.raw_input=description;resolution.has_evidence=1;
 resolution.evidence=evidence;resolution.has_clarification=1;
 resolution.clarification=clarification;resolution.has_error=1;
 resolution.error=error;resolution.requires_confirmation=truth;
 resolution.has_provenance=1;resolution.provenance=provenance;
 record.provenance=provenance;record.status=GUTC_SEMANTIC_RESOLVED;
 record.timestamp=input;
 store.append=NULL;store.context=NULL;
 request.input=input;request.schema=sschema;request.context=&context;
 request.candidates=names;request.preserve_raw_input=truth;
 request.min_candidate_similarity=one;request.min_candidate_margin=one;
 request.policy=&policy;request.provenance_store=&store;
 assert(key.length==5&&schema.property_count==1&&parameter.required);
 assert(definition.parameter_count==1&&definition.semantics.pure);
 assert(definition.examples[0].input_object==NULL);
 assert(sschema.property_count==1&&sproperty.key.length==5);
 assert(context.vocabulary.count==1&&progress.detail.present);
 assert(event.code==GUTC_SEMANTIC_EMPTY_INPUT&&policy.risk==GUTC_SEMANTIC_RISK_HIGH);
 assert(provenance.models[2].length==5&&candidate.similarity.value==1.0);
 assert(evidence.candidate_count==1&&clarification.candidates.count==1);
 assert(resolution.has_provenance&&resolution.has_error);
 assert(record.timestamp.length==5&&store.append==NULL);
 assert(request.min_candidate_margin.present&&request.provenance_store==&store);
 assert(gutc_semantic_error_code_name((gutc_semantic_error_code)999)==NULL);
 assert(gutc_semantic_risk_name((gutc_semantic_risk)999)==NULL);
 assert(gutc_semantic_phase_name((gutc_semantic_phase)999)==NULL);
 assert(gutc_semantic_event_name((gutc_semantic_event_type)999)==NULL);
 assert(gutc_semantic_resolution_status_name((gutc_semantic_resolution_status)999)==NULL);
 assert(gutc_semantic_model_policy_name((gutc_semantic_model_policy)999)==NULL);
 {
  gutc_semantic_error_code x=GUTC_SEMANTIC_ABORTED;
  assert(!gutc_semantic_error_code_parse("unknown",7,&x)&&x==GUTC_SEMANTIC_ABORTED);
  assert(!gutc_semantic_error_code_parse(NULL,1,&x));
  assert(!gutc_semantic_error_code_parse("EMPTY_INPUT",11,NULL));
 }
}
static void check_json_types(void){
 static const char *const literals[]={
  "null","true","12.5","\"text\"","[]","{}","{\"x\":1}"
 };
 size_t i;
 for(i=0;i<sizeof(literals)/sizeof(literals[0]);i++){
  gutc_json_dom *parsed=NULL;
  const char *source=literals[i];
  assert(gutc_json_dom_parse((const uint8_t*)source,strlen(source),16,100,
                             &parsed,NULL)==GUTC_OK);
  assert(gutc_json_dom_is_json_value(parsed));
  assert(gutc_type_is_json_primitive(parsed)==(i<4));
  assert(gutc_type_is_json_object(parsed)==(i>=5));
  gutc_json_dom_free(parsed);
 }
 {
  gutc_json_dom finite={0};
  finite.kind=GUTC_JSON_NUMBER;finite.as.number=NAN;
  assert(!gutc_type_is_json_primitive(&finite));
  assert(!gutc_type_is_json_object(&finite));
  assert(!gutc_type_is_json_primitive(NULL));
  assert(!gutc_type_is_json_object(NULL));
 }
}
int main(void){
 FILE *f=fopen(GUTC_PORTABLE_TYPE_KINDS_PATH,"r");
 char line[4096];size_t count=0;
 assert(f);
 while(fgets(line,sizeof(line),f)){
  char *a,*b,*end;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  a=strchr(line,'\t');assert(a);*a++='\0';
  b=strchr(a,'\t');assert(b);*b++='\0';
  verify_row(line,a,(size_t)strtoul(b,NULL,10));
  count++;
 }
 fclose(f);assert(count==31);
 type_struct_smoke();
 check_json_types();
 puts("GutC JSON, API and semantic-input source-pinned native type contracts passed");
 return 0;
}
