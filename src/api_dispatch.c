#include "gutc/api_dispatch.h"
#include "gutc/api_registry.h"
#include "gutc/collections.h"
#include "gutc/object_path.h"
#include "gutc/gutc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern gutc_status gutc_dispatch_text_api(const char *id,size_t id_len,
  const gutc_json_dom *input,gutc_json_dom **output,int *defined);

static int id_is(const char *id,size_t len,const char *wanted) {
  size_t n=strlen(wanted);
  return n==len && (!n||memcmp(id,wanted,n)==0);
}
static const gutc_json_dom *field(const gutc_json_dom *o,const char *key) {
  return gutc_json_dom_get(o,key,strlen(key));
}
static int is_kind(const gutc_json_dom *v,gutc_json_kind kind) {
  return v && v->kind==kind;
}
static gutc_json_dom *node(gutc_json_kind kind) {
  gutc_json_dom *v=(gutc_json_dom*)calloc(1,sizeof(*v));
  if(v)v->kind=kind;
  return v;
}
static gutc_status new_bool(int yes,gutc_json_dom **out) {
  gutc_json_dom *v=node(GUTC_JSON_BOOLEAN);
  if(!v)return GUTC_NO_MEMORY;
  v->as.boolean=yes?1:0;*out=v;return GUTC_OK;
}
static gutc_status new_number(double value,gutc_json_dom **out) {
  gutc_json_dom *v;
  if(!isfinite(value))return GUTC_OVERFLOW;
  v=node(GUTC_JSON_NUMBER);
  if(!v)return GUTC_NO_MEMORY;
  v->as.number=value;*out=v;return GUTC_OK;
}
static gutc_status new_string(const char *s,size_t len,gutc_json_dom **out) {
  gutc_json_dom *v;
  size_t count=0;
  gutc_status rc;
  if((!s&&len)||len==SIZE_MAX)return GUTC_INVALID_ARGUMENT;
  rc=gutc_utf8_validate((const uint8_t*)s,len,&count);
  if(rc!=GUTC_OK)return rc;
  v=node(GUTC_JSON_STRING);
  if(!v)return GUTC_NO_MEMORY;
  v->as.string.bytes=(char*)malloc(len+1);
  if(!v->as.string.bytes){free(v);return GUTC_NO_MEMORY;}
  if(len)memcpy(v->as.string.bytes,s,len);
  v->as.string.bytes[len]='\0';v->as.string.length=len;
  *out=v;return GUTC_OK;
}
static gutc_status clone(const gutc_json_dom *input,gutc_json_dom **out) {
  size_t len=0,at=0;
  char *tmp;
  gutc_status rc;
  *out=NULL;
  if(!gutc_json_dom_is_json_value(input))return GUTC_INVALID_FORMAT;
  rc=gutc_json_dom_stringify(input,NULL,0,&len);
  if(rc!=GUTC_OK)return rc;
  if(len==SIZE_MAX)return GUTC_OVERFLOW;
  tmp=(char*)malloc(len+1);
  if(!tmp)return GUTC_NO_MEMORY;
  rc=gutc_json_dom_stringify(input,tmp,len+1,&at);
  if(rc==GUTC_OK)rc=gutc_json_dom_parse((const uint8_t*)tmp,at,128,100000,out,NULL);
  free(tmp);return rc;
}
static gutc_status array_push(gutc_json_dom *array,const gutc_json_dom *value) {
  gutc_json_dom *copy=NULL,**values;
  size_t n;
  gutc_status rc;
  if(!is_kind(array,GUTC_JSON_ARRAY))return GUTC_INVALID_ARGUMENT;
  n=array->as.array.count;
  if(n>=100000||n>=SIZE_MAX/sizeof(*values))return GUTC_RANGE_ERROR;
  rc=clone(value,&copy);
  if(rc!=GUTC_OK)return rc;
  values=(gutc_json_dom**)realloc(array->as.array.values,(n+1)*sizeof(*values));
  if(!values){gutc_json_dom_free(copy);return GUTC_NO_MEMORY;}
  values[n]=copy;array->as.array.values=values;array->as.array.count=n+1;
  return GUTC_OK;
}
static int truthy(const gutc_json_dom *v) {
  if(!v)return 0;
  switch(v->kind) {
    case GUTC_JSON_NULL:return 0;
    case GUTC_JSON_BOOLEAN:return v->as.boolean!=0;
    case GUTC_JSON_NUMBER:return v->as.number!=0&&!isnan(v->as.number);
    case GUTC_JSON_STRING:return v->as.string.length!=0;
    case GUTC_JSON_OBJECT:case GUTC_JSON_ARRAY:return 1;
    default:return 0;
  }
}
static int same_primitive(const gutc_json_dom *a,const gutc_json_dom *b) {
  if(!a||!b||a->kind!=b->kind)return 0;
  switch(a->kind) {
    case GUTC_JSON_NULL:return 1;
    case GUTC_JSON_BOOLEAN:return a->as.boolean==b->as.boolean;
    case GUTC_JSON_NUMBER:
      return a->as.number==b->as.number ||
        (isnan(a->as.number)&&isnan(b->as.number));
    case GUTC_JSON_STRING:
      return a->as.string.length==b->as.string.length &&
        (a->as.string.length==0 ||
        memcmp(a->as.string.bytes,b->as.string.bytes,a->as.string.length)==0);
    case GUTC_JSON_ARRAY:case GUTC_JSON_OBJECT:return a==b;
    default:return 0;
  }
}
static gutc_status dispatch_array(const char *id,size_t len,
                                 const gutc_json_dom *input,
                                 gutc_json_dom **output,int *defined) {
  const gutc_json_dom *values=field(input,"values");
  gutc_json_dom *result;
  gutc_status rc=GUTC_OK;
  size_t i,j,count;
  if(!is_kind(values,GUTC_JSON_ARRAY))return GUTC_INVALID_FORMAT;
  count=values->as.array.count;
  if(id_is(id,len,"gutts.arrays.last")) {
    if(!count)return GUTC_OK;
    rc=clone(values->as.array.values[count-1],output);
    if(rc==GUTC_OK)*defined=1;
    return rc;
  }
  result=node(GUTC_JSON_ARRAY);
  if(!result)return GUTC_NO_MEMORY;
  if(id_is(id,len,"gutts.arrays.chunk")) {
    const gutc_json_dom *size=field(input,"size");
    size_t chunk_size=0;
    if(!is_kind(size,GUTC_JSON_NUMBER)||!isfinite(size->as.number)||
       size->as.number<1||size->as.number>100000||
       floor(size->as.number)!=size->as.number){
      rc=GUTC_RANGE_ERROR;goto fail;
    }
    chunk_size=(size_t)size->as.number;
    for(i=0;i<count;) {
      gutc_json_dom *group=node(GUTC_JSON_ARRAY);
      gutc_json_dom **outer;
      size_t n=result->as.array.count;
      if(!group){rc=GUTC_NO_MEMORY;goto fail;}
      for(j=0;j<chunk_size&&i<count;j++,i++){
        rc=array_push(group,values->as.array.values[i]);
        if(rc!=GUTC_OK){gutc_json_dom_free(group);goto fail;}
      }
      if(n>=SIZE_MAX/sizeof(*outer)){rc=GUTC_OVERFLOW;gutc_json_dom_free(group);goto fail;}
      outer=(gutc_json_dom**)realloc(result->as.array.values,(n+1)*sizeof(*outer));
      if(!outer){rc=GUTC_NO_MEMORY;gutc_json_dom_free(group);goto fail;}
      outer[n]=group;result->as.array.values=outer;result->as.array.count=n+1;
    }
  } else if(id_is(id,len,"gutts.arrays.flatten")) {
    for(i=0;i<count;i++){
      const gutc_json_dom *v=values->as.array.values[i];
      if(is_kind(v,GUTC_JSON_ARRAY)){
        for(j=0;j<v->as.array.count;j++){
          rc=array_push(result,v->as.array.values[j]);
          if(rc!=GUTC_OK)goto fail;
        }
      }else{
        rc=array_push(result,v);
        if(rc!=GUTC_OK)goto fail;
      }
    }
  } else {
    const int unique=id_is(id,len,"gutts.arrays.unique");
    if(!unique&&!id_is(id,len,"gutts.arrays.compact")){rc=GUTC_UNSUPPORTED;goto fail;}
    for(i=0;i<count;i++) {
      int keep=unique||truthy(values->as.array.values[i]);
      if(unique)for(j=0;j<i;j++) {
        if(same_primitive(values->as.array.values[i],values->as.array.values[j])) {
          keep=0;break;
        }
      }
      if(keep){rc=array_push(result,values->as.array.values[i]);if(rc!=GUTC_OK)goto fail;}
    }
  }
  *output=result;*defined=1;return GUTC_OK;
fail:
  gutc_json_dom_free(result);return rc;
}
static gutc_status dispatch_number(const char *id,size_t len,
                                  const gutc_json_dom *input,
                                  gutc_json_dom **output,int *defined) {
  const gutc_json_dom *value=field(input,"value");
  gutc_status rc;
  int match=0;
  double result=0.0;
  if(id_is(id,len,"gutts.numbers.isFiniteNumber")){
    rc=new_bool(is_kind(value,GUTC_JSON_NUMBER)&&isfinite(value->as.number),output);
    if(rc==GUTC_OK)*defined=1;
    return rc;
  }
  if(id_is(id,len,"gutts.numbers.sum")) {
    const gutc_json_dom *values=field(input,"values");
    size_t i;
    if(!is_kind(values,GUTC_JSON_ARRAY))return GUTC_INVALID_FORMAT;
    for(i=0;i<values->as.array.count;i++) {
      const gutc_json_dom *item=values->as.array.values[i];
      if(!is_kind(item,GUTC_JSON_NUMBER)||!isfinite(item->as.number))
        return GUTC_INVALID_FORMAT;
      result+=item->as.number;
      if(!isfinite(result))return GUTC_OVERFLOW;
    }
  } else {
    if(!is_kind(value,GUTC_JSON_NUMBER))return GUTC_INVALID_FORMAT;
    if(id_is(id,len,"gutts.numbers.roundTo")){
      const gutc_json_dom *decimals=field(input,"decimals");
      unsigned d=0;
      if(decimals) {
        if(!is_kind(decimals,GUTC_JSON_NUMBER)||!isfinite(decimals->as.number)||
           decimals->as.number<0||floor(decimals->as.number)!=decimals->as.number)
          return GUTC_RANGE_ERROR;
        if(decimals->as.number>15.0)return GUTC_UNSUPPORTED;
        d=(unsigned)decimals->as.number;
      }
      rc=gutc_number_round_to(value->as.number,d,&result);
      if(rc!=GUTC_OK)return rc;
    } else {
      const gutc_json_dom *min=field(input,"min"),*max=field(input,"max");
      if(!is_kind(min,GUTC_JSON_NUMBER)||!is_kind(max,GUTC_JSON_NUMBER))
        return GUTC_INVALID_FORMAT;
      if(id_is(id,len,"gutts.numbers.inRange")) {
        rc=gutc_number_in_range(value->as.number,min->as.number,max->as.number,&match);
        if(rc!=GUTC_OK)return rc;
        rc=new_bool(match,output);
        if(rc==GUTC_OK)*defined=1;
        return rc;
      }
      if(!id_is(id,len,"gutts.numbers.clamp"))return GUTC_UNSUPPORTED;
      rc=gutc_number_clamp(value->as.number,min->as.number,max->as.number,&result);
      if(rc!=GUTC_OK)return rc;
    }
  }
  rc=new_number(result,output);
  if(rc==GUTC_OK)*defined=1;
  return rc;
}
static gutc_status dispatch_object(const char *id,size_t len,
                                  const gutc_json_dom *input,
                                  gutc_json_dom **output,int *defined) {
  const gutc_json_dom *value=field(input,"value");
  gutc_status rc;
  if(id_is(id,len,"gutts.objects.isPlainObject")){
    rc=new_bool(gutc_json_is_plain_object(value),output);
    if(rc==GUTC_OK)*defined=1;
    return rc;
  }
  if(!is_kind(value,GUTC_JSON_OBJECT))return GUTC_INVALID_FORMAT;
  if(id_is(id,len,"gutts.objects.hasOwn")){
    const gutc_json_dom *key=field(input,"key");
    int found=0;
    if(!is_kind(key,GUTC_JSON_STRING))return GUTC_INVALID_FORMAT;
    rc=gutc_json_path_has(value,key->as.string.bytes,key->as.string.length,&found);
    if(rc!=GUTC_OK)return rc;
    rc=new_bool(found,output);
  } else {
    const gutc_json_dom *keys=field(input,"keys");
    gutc_json_text *paths;
    size_t i,n;
    if(!is_kind(keys,GUTC_JSON_ARRAY))return GUTC_INVALID_FORMAT;
    n=keys->as.array.count;
    if(n>256)return GUTC_RANGE_ERROR;
    paths=(gutc_json_text*)calloc(n?n:1,sizeof(*paths));
    if(!paths)return GUTC_NO_MEMORY;
    for(i=0;i<n;i++) {
      const gutc_json_dom *key=keys->as.array.values[i];
      if(!is_kind(key,GUTC_JSON_STRING)){free(paths);return GUTC_INVALID_FORMAT;}
      paths[i]=key->as.string;
    }
    if(id_is(id,len,"gutts.objects.pick"))rc=gutc_json_pick_paths(value,paths,n,output);
    else if(id_is(id,len,"gutts.objects.omit"))rc=gutc_json_omit_paths(value,paths,n,output);
    else rc=GUTC_UNSUPPORTED;
    free(paths);
  }
  if(rc==GUTC_OK)*defined=1;
  return rc;
}
static gutc_status dispatch_json(const char *id,size_t len,
                                const gutc_json_dom *input,
                                gutc_json_dom **output,int *defined) {
  const gutc_json_dom *value=field(input,"value");
  const gutc_json_dom *text=field(input,"text");
  gutc_status rc;
  if(id_is(id,len,"gutts.json.isJsonValue")){
    rc=new_bool(gutc_json_dom_is_json_value(value),output);
    if(rc==GUTC_OK)*defined=1;
    return rc;
  }
  if(id_is(id,len,"gutts.json.stringifyJson")){
    const gutc_json_dom *space=field(input,"space");
    size_t len_out=0,written=0;
    char *temp;
    if(!value)return GUTC_INVALID_FORMAT;
    if(space){
      if(!is_kind(space,GUTC_JSON_NUMBER)||!isfinite(space->as.number))
        return GUTC_INVALID_FORMAT;
      if(space->as.number>0)return GUTC_UNSUPPORTED; /* pending pretty serializer */
    }
    rc=gutc_json_dom_stringify(value,NULL,0,&len_out);
    if(rc!=GUTC_OK)return rc;
    if(len_out==SIZE_MAX)return GUTC_OVERFLOW;
    temp=(char*)malloc(len_out+1);
    if(!temp)return GUTC_NO_MEMORY;
    rc=gutc_json_dom_stringify(value,temp,len_out+1,&written);
    if(rc==GUTC_OK)rc=new_string(temp,written,output);
    free(temp);
  }else{
    if(!is_kind(text,GUTC_JSON_STRING))return GUTC_INVALID_FORMAT;
    rc=gutc_json_dom_parse((const uint8_t*)text->as.string.bytes,
           text->as.string.length,128,100000,output,NULL);
    if(rc!=GUTC_OK&&id_is(id,len,"gutts.json.tryParseJson")&&
       (rc==GUTC_INVALID_FORMAT||rc==GUTC_INVALID_ENCODING))return GUTC_OK;
  }
  if(rc==GUTC_OK && *output)*defined=1;
  return rc;
}

gutc_status gutc_execute_api(const char *id,size_t len,const gutc_json_dom *input,
                              const gutc_model_api_adapter *adapter,
                              gutc_json_dom **output,int *defined) {
  gutc_json_dom *descriptor=NULL,*result=NULL;
  gutc_status rc;
  int found=0;
  if(!output||!defined||(!id&&len))return GUTC_INVALID_ARGUMENT;
  *output=NULL;*defined=0;
  if(!id||!is_kind(input,GUTC_JSON_OBJECT))return GUTC_INVALID_ARGUMENT;
  rc=gutc_api_registry_lookup(id,len,&descriptor,&found);
  if(rc!=GUTC_OK)return rc;
  gutc_json_dom_free(descriptor);
  if(!found)return GUTC_UNSUPPORTED;
  if(len>=6&&!memcmp(id,"model.",6)){
    if(!adapter||!adapter->invoke)return GUTC_UNSUPPORTED;
    rc=adapter->invoke(adapter->context,id,len,input,&result,defined);
    if(rc==GUTC_OK && (!*defined||!result||!gutc_json_dom_is_json_value(result)))
      rc=GUTC_INVALID_FORMAT;
    if(rc!=GUTC_OK){gutc_json_dom_free(result);*defined=0;return rc;}
    *output=result;return GUTC_OK;
  }
  if(len>=13&&!memcmp(id,"gutts.arrays.",13))
    return dispatch_array(id,len,input,output,defined);
  if(len>=14&&!memcmp(id,"gutts.numbers.",14))
    return dispatch_number(id,len,input,output,defined);
  if(len>=14&&!memcmp(id,"gutts.objects.",14))
    return dispatch_object(id,len,input,output,defined);
  if(len>=11&&!memcmp(id,"gutts.json.",11))
    return dispatch_json(id,len,input,output,defined);
  return gutc_dispatch_text_api(id,len,input,output,defined);
}
