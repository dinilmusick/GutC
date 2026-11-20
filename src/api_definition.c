#include "gutc/api_definition.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static gutc_json_dom *new_node(gutc_json_kind kind) {
  gutc_json_dom *v=(gutc_json_dom*)calloc(1,sizeof(*v));
  if(v)v->kind=kind;return v;
}
static const gutc_json_dom *get(const gutc_json_dom *o,const char *key) {
  return gutc_json_dom_get(o,key,strlen(key));
}
static int eq(gutc_json_text a,const char *key,size_t n) {
  return a.length==n && (!n||(a.bytes&&!memcmp(a.bytes,key,n)));
}
static gutc_json_dom *new_string(const char *text,size_t n) {
  gutc_json_dom *v;
  if((!text&&n)||n==SIZE_MAX)return NULL;
  v=new_node(GUTC_JSON_STRING);
  if(!v)return NULL;
  v->as.string.bytes=(char*)malloc(n+1);
  if(!v->as.string.bytes){free(v);return NULL;}
  if(n)memcpy(v->as.string.bytes,text,n);
  v->as.string.bytes[n]='\0';v->as.string.length=n;
  return v;
}
static gutc_status set(gutc_json_dom *o,const char *key,size_t klen,gutc_json_dom *value) {
  size_t i,n;
  gutc_json_member *entries;
  char *copy;
  if(!o||o->kind!=GUTC_JSON_OBJECT||(!key&&klen)||!value)return GUTC_INVALID_ARGUMENT;
  for(i=0;i<o->as.object.count;i++)if(eq(o->as.object.members[i].key,key,klen)){
    gutc_json_dom_free(o->as.object.members[i].value);
    o->as.object.members[i].value=value;
    return GUTC_OK;
  }
  n=o->as.object.count;
  if(n>=100000||n>=SIZE_MAX/sizeof(*entries)||klen==SIZE_MAX)return GUTC_RANGE_ERROR;
  copy=(char*)malloc(klen+1);
  if(!copy)return GUTC_NO_MEMORY;
  if(klen)memcpy(copy,key,klen);
  copy[klen]='\0';
  entries=(gutc_json_member*)realloc(o->as.object.members,(n+1)*sizeof(*entries));
  if(!entries){free(copy);return GUTC_NO_MEMORY;}
  entries[n].key.bytes=copy;entries[n].key.length=klen;entries[n].value=value;
  o->as.object.members=entries;o->as.object.count=n+1;
  return GUTC_OK;
}
static gutc_status transfer(gutc_json_dom *o,const char *key,size_t len,gutc_json_dom *value) {
  gutc_status rc;
  if(!value)return GUTC_NO_MEMORY;
  rc=set(o,key,len,value);
  if(rc!=GUTC_OK)gutc_json_dom_free(value);
  return rc;
}
static gutc_status clone(const gutc_json_dom *input,gutc_json_dom **out) {
  size_t n=0,written=0;
  char *buf;
  gutc_status rc;
  *out=NULL;
  if(!gutc_json_dom_is_json_value(input))return GUTC_INVALID_FORMAT;
  rc=gutc_json_dom_stringify(input,NULL,0,&n);
  if(rc!=GUTC_OK)return rc;
  if(n==SIZE_MAX)return GUTC_OVERFLOW;
  buf=(char*)malloc(n+1);
  if(!buf)return GUTC_NO_MEMORY;
  rc=gutc_json_dom_stringify(input,buf,n+1,&written);
  if(rc==GUTC_OK)rc=gutc_json_dom_parse((const uint8_t*)buf,written,128,100000,out,NULL);
  free(buf);return rc;
}
static gutc_status put_clone(gutc_json_dom *out,gutc_json_text name,
                              const gutc_json_dom *value) {
  gutc_json_dom *copy=NULL;
  gutc_status rc=clone(value,&copy);
  if(rc!=GUTC_OK)return rc;
  return transfer(out,name.bytes,name.length,copy);
}
static gutc_status push(gutc_json_dom *arr,gutc_json_dom *child) {
  size_t n=arr->as.array.count;
  gutc_json_dom **items;
  if(n>=100000||n>=SIZE_MAX/sizeof(*items))return GUTC_RANGE_ERROR;
  items=(gutc_json_dom**)realloc(arr->as.array.values,(n+1)*sizeof(*items));
  if(!items)return GUTC_NO_MEMORY;
  items[n]=child;
  arr->as.array.values=items;arr->as.array.count=n+1;
  return GUTC_OK;
}
static gutc_status push_clone(gutc_json_dom *arr,const gutc_json_dom *value) {
  gutc_json_dom *copy=NULL;
  gutc_status rc=clone(value,&copy);
  if(rc!=GUTC_OK)return rc;
  rc=push(arr,copy);
  if(rc!=GUTC_OK)gutc_json_dom_free(copy);
  return rc;
}
gutc_status gutc_define_api(const gutc_json_dom *definition,gutc_json_dom **output) {
  gutc_json_dom *root=NULL,*schema=NULL,*properties=NULL,*required=NULL,*boolval=NULL;
  const gutc_json_dom *parameters,*api_name;
  size_t i;
  gutc_status rc=GUTC_INVALID_FORMAT;
  if(!output)return GUTC_INVALID_ARGUMENT;
  *output=NULL;
  if(!definition||definition->kind!=GUTC_JSON_OBJECT)return rc;
  api_name=get(definition,"name");parameters=get(definition,"parameters");
  if(!api_name||api_name->kind!=GUTC_JSON_STRING||!api_name->as.string.bytes||
     !parameters||parameters->kind!=GUTC_JSON_ARRAY||
     parameters->as.array.count>4096)return rc;
  rc=clone(definition,&root);
  if(rc!=GUTC_OK)return rc;
  schema=new_node(GUTC_JSON_OBJECT);
  properties=new_node(GUTC_JSON_OBJECT);
  required=new_node(GUTC_JSON_ARRAY);
  if(!schema||!properties||!required){rc=GUTC_NO_MEMORY;goto fail;}
  rc=transfer(schema,"type",4,new_string("object",6));
  if(rc!=GUTC_OK)goto fail;
  {static const char prefix[]="Arguments for ";
   size_t n=sizeof(prefix)-1,m=api_name->as.string.length;
   char *buf;
   if(m>SIZE_MAX-n-1){rc=GUTC_OVERFLOW;goto fail;}
   buf=(char*)malloc(n+m+2);
   if(!buf){rc=GUTC_NO_MEMORY;goto fail;}
   memcpy(buf,prefix,n);
   if(m)memcpy(buf+n,api_name->as.string.bytes,m);
   buf[n+m]='.';
   rc=transfer(schema,"description",11,new_string(buf,n+m+1));
   free(buf);
   if(rc!=GUTC_OK)goto fail;
  }
  for(i=0;i<parameters->as.array.count;i++){
    const gutc_json_dom *param=parameters->as.array.values[i],*name,*description,*required_flag,*nested_schema;
    gutc_json_dom *nested=NULL;
    if(!param||param->kind!=GUTC_JSON_OBJECT){rc=GUTC_INVALID_FORMAT;goto fail;}
    name=get(param,"name");description=get(param,"description");
    required_flag=get(param,"required");nested_schema=get(param,"schema");
    if(!name||name->kind!=GUTC_JSON_STRING||
       !description||description->kind!=GUTC_JSON_STRING||
       !required_flag||required_flag->kind!=GUTC_JSON_BOOLEAN||
       !nested_schema||nested_schema->kind!=GUTC_JSON_OBJECT){
      rc=GUTC_INVALID_FORMAT;goto fail;
    }
    rc=clone(nested_schema,&nested);
    if(rc!=GUTC_OK)goto fail;
    rc=put_clone(nested, (gutc_json_text){"description",11}, description);
    if(rc!=GUTC_OK){gutc_json_dom_free(nested);goto fail;}
    rc=transfer(properties,name->as.string.bytes,name->as.string.length,nested);
    if(rc!=GUTC_OK)goto fail;
    if(required_flag->as.boolean){
      rc=push_clone(required,name);
      if(rc!=GUTC_OK)goto fail;
    }
  }
  rc=transfer(schema,"properties",10,properties);
  if(rc!=GUTC_OK){properties=NULL;goto fail;}properties=NULL;
  rc=transfer(schema,"required",8,required);
  if(rc!=GUTC_OK){required=NULL;goto fail;}required=NULL;
  boolval=new_node(GUTC_JSON_BOOLEAN);
  if(!boolval){rc=GUTC_NO_MEMORY;goto fail;}
  boolval->as.boolean=0;
  rc=transfer(schema,"additionalProperties",20,boolval);
  boolval=NULL;
  if(rc!=GUTC_OK)goto fail;
  rc=transfer(root,"inputSchema",11,schema);
  schema=NULL;
  if(rc!=GUTC_OK)goto fail;
  *output=root;return GUTC_OK;
fail:
  gutc_json_dom_free(schema);
  gutc_json_dom_free(properties);
  gutc_json_dom_free(required);
  gutc_json_dom_free(boolval);
  gutc_json_dom_free(root);
  return rc;
}
