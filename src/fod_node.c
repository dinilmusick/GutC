#include "gutc/fod_node.h"
#include "gutc/json.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FOD_MAX_BYTES (16u*1024u*1024u)
typedef struct writer {
  char *data;
  size_t length,capacity;
  gutc_status failure;
} writer;
static const gutc_json_dom *field(const gutc_json_dom *o,const char *key) {
  return gutc_json_dom_get(o,key,strlen(key));
}
static int kind(const gutc_json_dom *v,gutc_json_kind type) {
  return v && v->kind==type;
}
static void append(writer *w,const char *bytes,size_t n) {
  size_t required,capacity;
  char *replacement;
  if(w->failure!=GUTC_OK)return;
  if((!bytes&&n)||n>FOD_MAX_BYTES-w->length) {
    w->failure=GUTC_OVERFLOW;return;
  }
  required=w->length+n;
  if(required+1>w->capacity) {
    capacity=w->capacity?w->capacity:256u;
    while(capacity<=required){
      if(capacity>FOD_MAX_BYTES/2u){capacity=FOD_MAX_BYTES+1u;break;}
      capacity*=2u;
    }
    if(capacity>FOD_MAX_BYTES+1u){w->failure=GUTC_OVERFLOW;return;}
    replacement=(char*)realloc(w->data,capacity);
    if(!replacement){w->failure=GUTC_NO_MEMORY;return;}
    w->data=replacement;w->capacity=capacity;
  }
  if(n)memcpy(w->data+w->length,bytes,n);
  w->length+=n;w->data[w->length]='\0';
}
static void literal(writer *w,const char *text) {append(w,text,strlen(text));}
static void quote(writer *w,const char *text,size_t n) {
  char *buf;
  size_t len=0;
  gutc_status rc;
  if(w->failure!=GUTC_OK)return;
  rc=gutc_json_quote_string((const uint8_t*)text,n,NULL,0,&len);
  if(rc!=GUTC_OK){w->failure=rc;return;}
  if(len==SIZE_MAX){w->failure=GUTC_OVERFLOW;return;}
  buf=(char*)malloc(len+1);
  if(!buf){w->failure=GUTC_NO_MEMORY;return;}
  rc=gutc_json_quote_string((const uint8_t*)text,n,buf,len+1,&len);
  if(rc==GUTC_OK)append(w,buf,len);
  else w->failure=rc;
  free(buf);
}
static void json_value(writer *w,const gutc_json_dom *value) {
  size_t needed=0;
  char *text;
  gutc_status rc;
  if(w->failure!=GUTC_OK)return;
  rc=gutc_json_dom_stringify(value,NULL,0,&needed);
  if(rc!=GUTC_OK){w->failure=rc;return;}
  if(needed==SIZE_MAX){w->failure=GUTC_OVERFLOW;return;}
  text=(char*)malloc(needed+1);
  if(!text){w->failure=GUTC_NO_MEMORY;return;}
  rc=gutc_json_dom_stringify(value,text,needed+1,&needed);
  if(rc==GUTC_OK)append(w,text,needed);
  else w->failure=rc;
  free(text);
}
static void schema_type(writer *w,const gutc_json_dom *schema) {
  const gutc_json_dom *type=field(schema,"type");
  if(!type){quote(w,"any",3);return;}
  if(!kind(type,GUTC_JSON_STRING)){w->failure=GUTC_INVALID_FORMAT;return;}
  json_value(w,type);
}
static void type_with_id(writer *w,gutc_json_text id) {
  char *value;
  if(id.length>SIZE_MAX-7u){w->failure=GUTC_OVERFLOW;return;}
  value=(char*)malloc(id.length+7u);
  if(!value){w->failure=GUTC_NO_MEMORY;return;}
  memcpy(value,"gutts/",6);
  if(id.length)memcpy(value+6,id.bytes,id.length);
  value[id.length+6]='\0';
  quote(w,value,id.length+6);
  free(value);
}
gutc_status gutc_api_to_fod_node(const gutc_json_dom *api,gutc_json_dom **output) {
  const gutc_json_dom *id,*name,*description,*category,*params,*result_schema,*semantics;
  writer w={NULL,0,0,GUTC_OK};
  gutc_status rc;
  size_t i;
  if(!output)return GUTC_INVALID_ARGUMENT;
  *output=NULL;
  if(!kind(api,GUTC_JSON_OBJECT))return GUTC_INVALID_FORMAT;
  id=field(api,"id");name=field(api,"name");
  description=field(api,"description");category=field(api,"category");
  params=field(api,"parameters");result_schema=field(api,"output");
  semantics=field(api,"semantics");
  if(!kind(id,GUTC_JSON_STRING)||!kind(name,GUTC_JSON_STRING)||
     !kind(description,GUTC_JSON_STRING)||!kind(category,GUTC_JSON_STRING)||
     !kind(params,GUTC_JSON_ARRAY)||!kind(result_schema,GUTC_JSON_OBJECT)||
     !kind(semantics,GUTC_JSON_OBJECT))return GUTC_INVALID_FORMAT;
  literal(&w,"{\"type\":");
  type_with_id(&w,id->as.string);
  literal(&w,",\"label\":");json_value(&w,name);
  literal(&w,",\"description\":");json_value(&w,description);
  literal(&w,",\"category\":\"gutts\",\"group\":");json_value(&w,category);
  literal(&w,",\"inputs\":[");
  for(i=0;i<params->as.array.count;i++){
    const gutc_json_dom *param=params->as.array.values[i],*pname,*pdesc,*prequired,*schema;
    if(!kind(param,GUTC_JSON_OBJECT)){w.failure=GUTC_INVALID_FORMAT;break;}
    pname=field(param,"name");pdesc=field(param,"description");
    prequired=field(param,"required");schema=field(param,"schema");
    if(!kind(pname,GUTC_JSON_STRING)||!kind(pdesc,GUTC_JSON_STRING)||
       !kind(prequired,GUTC_JSON_BOOLEAN)||!kind(schema,GUTC_JSON_OBJECT)){
      w.failure=GUTC_INVALID_FORMAT;break;
    }
    if(i)literal(&w,",");
    literal(&w,"{\"id\":");json_value(&w,pname);
    literal(&w,",\"name\":");json_value(&w,pname);
    literal(&w,",\"direction\":\"input\",\"dataType\":");schema_type(&w,schema);
    literal(&w,",\"required\":");json_value(&w,prequired);
    literal(&w,",\"description\":");json_value(&w,pdesc);
    literal(&w,",\"schema\":");json_value(&w,schema);
    literal(&w,"}");
  }
  literal(&w,"],\"outputs\":[{\"id\":\"result\",\"name\":\"result\",\"direction\":\"output\",\"dataType\":");
  schema_type(&w,result_schema);
  {
    const gutc_json_dom *output_description=field(result_schema,"description");
    if(output_description){
      if(!kind(output_description,GUTC_JSON_STRING))w.failure=GUTC_INVALID_FORMAT;
      else if(output_description->as.string.length){
        literal(&w,",\"description\":");json_value(&w,output_description);
      }
    }
  }
  literal(&w,",\"schema\":");json_value(&w,result_schema);
  literal(&w,"}],\"metadata\":{\"apiId\":");json_value(&w,id);
  literal(&w,",\"package\":\"@dinilmusick/gutts\",\"semantics\":");
  json_value(&w,semantics);literal(&w,"}}");
  if(w.failure!=GUTC_OK){rc=w.failure;free(w.data);return rc;}
  rc=gutc_json_dom_parse((const uint8_t*)w.data,w.length,128,100000,output,NULL);
  free(w.data);
  return rc;
}
