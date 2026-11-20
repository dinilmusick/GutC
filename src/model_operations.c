#include "gutc/model_operations.h"
#include "gutc/gutc.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
static const char *const family_names[GUTC_MODEL_FAMILY_COUNT]={
 "Needle3","SmolLM2","Whistle","EttinReranker17M","EttinReranker32M",
 "EttinReranker68M","EttinReranker150M","MiniLM"
};
static const char *const family_ids[GUTC_MODEL_FAMILY_COUNT]={
 "model.cactus-needle3","model.smollm2-135m-instruct","model.cactus-whistle",
 "model.ettin-reranker-17m","model.ettin-reranker-32m",
 "model.ettin-reranker-68m","model.ettin-reranker-150m","model.minilm-l6-v2"
};
const char *gutc_model_family_name(gutc_model_family family){
 return (unsigned)family<GUTC_MODEL_FAMILY_COUNT?family_names[(size_t)family]:NULL;
}
const char *gutc_model_family_id(gutc_model_family family){
 return (unsigned)family<GUTC_MODEL_FAMILY_COUNT?family_ids[(size_t)family]:NULL;
}
gutc_status gutc_model_client_init(gutc_model_client *client,
 gutc_model_family family,const gutc_model_runtime_host *host){
 const char *id=gutc_model_family_id(family);
 gutc_status rc;
 if(!client)return GUTC_INVALID_ARGUMENT;
 if(!id)return GUTC_RANGE_ERROR;
 memset(client,0,sizeof(*client));
 rc=gutc_model_library_init(&client->library,id,strlen(id),host);
 if(rc==GUTC_OK)client->family=family;
 return rc;
}
static const gutc_json_dom *prop(const gutc_json_dom *o,const char *name){
 return gutc_json_dom_get(o,name,strlen(name));
}
static int is_string(const gutc_json_dom *v){
 size_t cp=0;
 return v&&v->kind==GUTC_JSON_STRING&&
   (!v->as.string.length||v->as.string.bytes!=NULL)&&
   gutc_utf8_validate((const uint8_t*)(v->as.string.length?v->as.string.bytes:""),
                       v->as.string.length,&cp)==GUTC_OK;
}
static int is_integer(const gutc_json_dom *v){
 return v&&v->kind==GUTC_JSON_NUMBER&&isfinite(v->as.number)&&
   floor(v->as.number)==v->as.number;
}
static int is_numeric_array(const gutc_json_dom *array){
 size_t i;
 if(!array||array->kind!=GUTC_JSON_ARRAY||array->as.array.count>10000000||
    (array->as.array.count&&!array->as.array.values))
   return 0;
 for(i=0;i<array->as.array.count;i++){
  const gutc_json_dom *v=array->as.array.values[i];
  if(!v||v->kind!=GUTC_JSON_NUMBER||!isfinite(v->as.number))return 0;
 }
 return 1;
}
static int is_string_array(const gutc_json_dom *array){
 size_t i;
 if(!array||array->kind!=GUTC_JSON_ARRAY||array->as.array.count>100000||
    (array->as.array.count&&!array->as.array.values))
   return 0;
 for(i=0;i<array->as.array.count;i++)
  if(!is_string(array->as.array.values[i]))return 0;
 return 1;
}
static int input_valid(gutc_model_family family,const char *op,
 const gutc_json_dom *input){
 const gutc_json_dom *v;
 size_t i;
 if(!input||input->kind!=GUTC_JSON_OBJECT)return 0;
 switch(family){
 case GUTC_MODEL_NEEDLE3:
  if(!strcmp(op,"complete")){
   if(!is_string(prop(input,"prompt")))return 0;
   v=prop(input,"tools");
   if(v&&(v->kind!=GUTC_JSON_ARRAY||v->as.array.count>10000))return 0;
   return 1;
  }
  if(!strcmp(op,"embed"))return is_string(prop(input,"input"));
  return 0;
 case GUTC_MODEL_SMOLLM2:
  if(strcmp(op,"generate")||!is_string(prop(input,"prompt")))return 0;
  v=prop(input,"maxNewTokens");
  return !v||(is_integer(v)&&v->as.number>=0&&v->as.number<=1000000);
 case GUTC_MODEL_WHISTLE:
  if(strcmp(op,"transcribe"))return 0;
  v=prop(input,"language");
  if(v&&!is_string(v))return 0;
  v=prop(input,"audio");
  if(!is_numeric_array(v)||v->as.array.count>8388608)return 0;
  for(i=0;i<v->as.array.count;i++){
   const gutc_json_dom *byte=v->as.array.values[i];
   if(byte->as.number<0||byte->as.number>255||
      floor(byte->as.number)!=byte->as.number)return 0;
  }
  return 1;
 case GUTC_MODEL_ETTIN17:case GUTC_MODEL_ETTIN32:
 case GUTC_MODEL_ETTIN68:case GUTC_MODEL_ETTIN150:
  return !strcmp(op,"rerank")&&is_string(prop(input,"query"))&&
    is_string_array(prop(input,"candidates"));
 case GUTC_MODEL_MINILM:
  return !strcmp(op,"similarity")&&is_string(prop(input,"left"))&&
    is_string(prop(input,"right"));
 default:return 0;
 }
}
static int output_valid(gutc_model_family family,const char *op,
 const gutc_json_dom *input,const gutc_json_dom *out){
 const gutc_json_dom *candidates;
 if(!out||!gutc_json_dom_is_json_value(out))return 0;
 if(family==GUTC_MODEL_NEEDLE3) {
  if(!strcmp(op,"complete"))return 1;
  if(!strcmp(op,"embed"))return is_numeric_array(out);
  return 0;
 }
 if(family==GUTC_MODEL_SMOLLM2||family==GUTC_MODEL_WHISTLE)
  return is_string(out);
 if(family==GUTC_MODEL_MINILM)
  return out->kind==GUTC_JSON_NUMBER&&isfinite(out->as.number);
 if(family>=GUTC_MODEL_ETTIN17&&family<=GUTC_MODEL_ETTIN150){
  candidates=prop(input,"candidates");
  return is_numeric_array(out)&&candidates&&candidates->kind==GUTC_JSON_ARRAY&&
    out->as.array.count==candidates->as.array.count;
 }
 return 0;
}
gutc_status gutc_model_client_call(gutc_model_client *client,
 const char *operation,const gutc_json_dom *input,gutc_json_dom **output){
 gutc_json_dom *result=NULL;
 gutc_status rc;
 if(!client||!operation||!output)return GUTC_INVALID_ARGUMENT;
 *output=NULL;
 if(!client->library.metadata)return GUTC_INVALID_ARGUMENT;
 if(!input_valid(client->family,operation,input))return GUTC_INVALID_FORMAT;
 rc=gutc_model_library_invoke(&client->library,operation,input,&result);
 if(rc!=GUTC_OK)return rc;
 if(!output_valid(client->family,operation,input,result)){
  gutc_json_dom_free(result);
  return GUTC_INVALID_FORMAT;
 }
 *output=result;return GUTC_OK;
}
gutc_status gutc_model_client_destroy(gutc_model_client *client){
 gutc_status rc;
 if(!client)return GUTC_INVALID_ARGUMENT;
 rc=gutc_model_library_destroy(&client->library);
 if(rc==GUTC_OK)client->family=GUTC_MODEL_NEEDLE3;
 return rc;
}
