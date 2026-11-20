#include "gutc/api_dispatch.h"
#include "gutc/gutc.h"
#include "gutc/string_case.h"
#include "gutc/url_query.h"
#include "gut_string_predicates.h"
#include "gut_truncate.h"
#include "gut_truncate_words.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int id_is(const char *id,size_t len,const char *wanted) {
  size_t n=strlen(wanted);
  return n==len&&(!n||memcmp(id,wanted,n)==0);
}
static const gutc_json_dom *field(const gutc_json_dom *v,const char *key) {
  return gutc_json_dom_get(v,key,strlen(key));
}
static int kind(const gutc_json_dom *v,gutc_json_kind type) {
  return v && v->kind==type;
}
static gutc_status make_bool(int value,gutc_json_dom **out) {
  gutc_json_dom *v=(gutc_json_dom*)calloc(1,sizeof(*v));
  if(!v)return GUTC_NO_MEMORY;
  v->kind=GUTC_JSON_BOOLEAN;v->as.boolean=value!=0;*out=v;
  return GUTC_OK;
}
static gutc_status make_string(const char *data,size_t n,gutc_json_dom **out) {
  gutc_json_dom *v;
  size_t count=0;
  gutc_status rc;
  if((!data&&n)||n==SIZE_MAX)return GUTC_INVALID_ARGUMENT;
  rc=gutc_utf8_validate((const uint8_t*)data,n,&count);
  if(rc!=GUTC_OK)return rc;
  v=(gutc_json_dom*)calloc(1,sizeof(*v));
  if(!v)return GUTC_NO_MEMORY;
  v->kind=GUTC_JSON_STRING;
  v->as.string.bytes=(char*)malloc(n+1);
  if(!v->as.string.bytes){free(v);return GUTC_NO_MEMORY;}
  if(n)memcpy(v->as.string.bytes,data,n);
  v->as.string.bytes[n]='\0';v->as.string.length=n;
  *out=v;return GUTC_OK;
}
static int safe_cstr(const gutc_json_dom *v) {
  return kind(v,GUTC_JSON_STRING)&&v->as.string.bytes &&
    !memchr(v->as.string.bytes,'\0',v->as.string.length);
}
static gutc_status match_any(const gutc_json_dom *value,const gutc_json_dom *candidates,
                             int suffix,gutc_json_dom **out) {
  size_t i,na;
  if(!kind(value,GUTC_JSON_STRING)||!kind(candidates,GUTC_JSON_ARRAY))
    return GUTC_INVALID_FORMAT;
  na=value->as.string.length;
  for(i=0;i<candidates->as.array.count;i++){
    const gutc_json_dom *candidate=candidates->as.array.values[i];
    size_t nc;
    if(!kind(candidate,GUTC_JSON_STRING))return GUTC_INVALID_FORMAT;
    nc=candidate->as.string.length;
    if(nc<=na && (!nc||!memcmp(value->as.string.bytes+(suffix?na-nc:0),
                            candidate->as.string.bytes,nc)))
      return make_bool(1,out);
  }
  return make_bool(0,out);
}
static gutc_status case_transform(const gutc_json_dom *value,gutc_string_case mode,
                                   gutc_json_dom **out) {
  size_t needed=0;
  char *buffer;
  gutc_status rc;
  if(!kind(value,GUTC_JSON_STRING))return GUTC_INVALID_FORMAT;
  rc=gutc_string_transform_ascii(value->as.string.bytes,value->as.string.length,
                                 mode,NULL,0,&needed);
  if(rc!=GUTC_OK)return rc;
  if(needed==SIZE_MAX)return GUTC_OVERFLOW;
  buffer=(char*)malloc(needed+1);
  if(!buffer)return GUTC_NO_MEMORY;
  rc=gutc_string_transform_ascii(value->as.string.bytes,value->as.string.length,
                                 mode,buffer,needed+1,&needed);
  if(rc==GUTC_OK)rc=make_string(buffer,needed,out);
  free(buffer);return rc;
}
static gutc_status truncate_text(const gutc_json_dom *input,int words,
                                  gutc_json_dom **out) {
  const gutc_json_dom *text=field(input,"value");
  const gutc_json_dom *maximum=field(input,words?"maxWords":"maxLength");
  const gutc_json_dom *suffix=field(input,"suffix");
  const char *ending="\xE2\x80\xA6";
  size_t n=3,needed=0;
  size_t limit;
  gut_truncate_status tr;
  char *result;
  gutc_status rc;
  if(!safe_cstr(text)||!kind(maximum,GUTC_JSON_NUMBER)||
     !isfinite(maximum->as.number)||maximum->as.number<0||
     maximum->as.number>1000000)return GUTC_RANGE_ERROR;
  if(floor(maximum->as.number)!=maximum->as.number)
    return GUTC_UNSUPPORTED;
  if(suffix){
    if(!safe_cstr(suffix))return GUTC_UNSUPPORTED;
    ending=suffix->as.string.bytes;n=suffix->as.string.length;
  }
  limit=(size_t)maximum->as.number;
  if(words){
    (void)n;
    tr=gut_truncate_words_utf8(text->as.string.bytes,limit,ending,NULL,0,&needed);
  }else{
    tr=gut_truncate_utf8_n(text->as.string.bytes,text->as.string.length,
                           limit,ending,n,NULL,0,&needed);
  }
  if(tr==GUT_TRUNCATE_INVALID)return GUTC_INVALID_FORMAT;
  if(needed==0||needed>1048576u)return GUTC_RANGE_ERROR;
  result=(char*)malloc(needed);
  if(!result)return GUTC_NO_MEMORY;
  if(words)
    tr=gut_truncate_words_utf8(text->as.string.bytes,limit,ending,result,needed,&needed);
  else
    tr=gut_truncate_utf8_n(text->as.string.bytes,text->as.string.length,
                           limit,ending,n,result,needed,&needed);
  if(tr!=GUT_TRUNCATE_OK){free(result);return GUTC_INVALID_FORMAT;}
  rc=make_string(result,needed-1,out);
  free(result);return rc;
}
static gutc_status dispatch_string(const char *id,size_t len,
                                  const gutc_json_dom *input,
                                  gutc_json_dom **out) {
  const gutc_json_dom *v=field(input,"value");
  gutc_status rc;
  int matched;
  if(!kind(v,GUTC_JSON_STRING))return GUTC_INVALID_FORMAT;
  if(id_is(id,len,"gutts.strings.isBlank")||
     id_is(id,len,"gutts.strings.isNonBlank")) {
    matched=gut_is_blank_utf8_n(v->as.string.bytes,v->as.string.length);
    if(matched<0)return GUTC_INVALID_ENCODING;
    if(id_is(id,len,"gutts.strings.isNonBlank"))matched=!matched;
    return make_bool(matched,out);
  }
  if(id_is(id,len,"gutts.strings.isEmail")){
    rc=gutc_is_email_ascii(v->as.string.bytes,v->as.string.length,&matched);
    if(rc!=GUTC_OK)return rc;
    return make_bool(matched,out);
  }
  if(id_is(id,len,"gutts.strings.startsWithAny"))
    return match_any(v,field(input,"prefixes"),0,out);
  if(id_is(id,len,"gutts.strings.endsWithAny"))
    return match_any(v,field(input,"suffixes"),1,out);
  if(id_is(id,len,"gutts.strings.capitalize"))
    return case_transform(v,GUTC_CASE_CAPITALIZE,out);
  if(id_is(id,len,"gutts.strings.camelCase"))
    return case_transform(v,GUTC_CASE_CAMEL,out);
  if(id_is(id,len,"gutts.strings.kebabCase"))
    return case_transform(v,GUTC_CASE_KEBAB,out);
  if(id_is(id,len,"gutts.strings.snakeCase"))
    return case_transform(v,GUTC_CASE_SNAKE,out);
  if(id_is(id,len,"gutts.strings.truncate"))
    return truncate_text(input,0,out);
  if(id_is(id,len,"gutts.strings.truncateWords"))
    return truncate_text(input,1,out);
  return GUTC_UNSUPPORTED;
}
static gutc_status dispatch_url(const char *id,size_t len,
                               const gutc_json_dom *input,
                               gutc_json_dom **out) {
  const gutc_json_dom *url=field(input,"url");
  gutc_status rc;
  size_t needed=0;
  char *buffer;
  if(id_is(id,len,"gutts.urls.isUrl")){
    const gutc_json_dom *value=field(input,"value");
    size_t i;
    if(!safe_cstr(value))return GUTC_INVALID_FORMAT;
    if(gutc_url_is_http_url(value->as.string.bytes))return make_bool(1,out);
    /* A non-HTTP URL scheme may be valid according to WHATWG; do not return
     * a fabricated false result for mailto:, file:, custom:, ftp:, etc. */
    for(i=0;i<value->as.string.length;i++){
      if(value->as.string.bytes[i]==':')return GUTC_UNSUPPORTED;
      if(!((value->as.string.bytes[i]>='a'&&value->as.string.bytes[i]<='z')||
           (value->as.string.bytes[i]>='A'&&value->as.string.bytes[i]<='Z')||
           (i>0&&((value->as.string.bytes[i]>='0'&&value->as.string.bytes[i]<='9')||
             value->as.string.bytes[i]=='+'||value->as.string.bytes[i]=='-'||
             value->as.string.bytes[i]=='.'))))break;
    }
    return make_bool(0,out);
  }
  if(id_is(id,len,"gutts.urls.joinUrl")){
    const gutc_json_dom *base=field(input,"base"),*parts=field(input,"parts");
    const char **values;
    size_t i;
    if(!safe_cstr(base)||!kind(parts,GUTC_JSON_ARRAY))return GUTC_INVALID_FORMAT;
    if(parts->as.array.count>256)return GUTC_RANGE_ERROR;
    values=(const char**)calloc(parts->as.array.count?parts->as.array.count:1,sizeof(*values));
    if(!values)return GUTC_NO_MEMORY;
    for(i=0;i<parts->as.array.count;i++){
      const gutc_json_dom *item=parts->as.array.values[i];
      if(!safe_cstr(item)){free(values);return GUTC_UNSUPPORTED;}
      values[i]=item->as.string.bytes;
    }
    rc=gutc_join_url(base->as.string.bytes,values,parts->as.array.count,
                      NULL,0,&needed);
    if(rc!=GUTC_OK){free(values);return rc;}
    if(needed==SIZE_MAX){free(values);return GUTC_OVERFLOW;}
    buffer=(char*)malloc(needed+1);
    if(!buffer){free(values);return GUTC_NO_MEMORY;}
    rc=gutc_join_url(base->as.string.bytes,values,parts->as.array.count,
                      buffer,needed+1,&needed);
    free(values);
    if(rc==GUTC_OK)rc=make_string(buffer,needed,out);
    free(buffer);return rc;
  }
  if(!safe_cstr(url))return GUTC_INVALID_FORMAT;
  if(id_is(id,len,"gutts.urls.getQueryParam")) {
    const gutc_json_dom *key=field(input,"key");
    int found=0;
    gutc_json_dom *null_node;
    if(!safe_cstr(key))return GUTC_INVALID_FORMAT;
    rc=gutc_url_get_query_param(url->as.string.bytes,key->as.string.bytes,
                                NULL,0,&needed,&found);
    if(rc!=GUTC_OK)return rc;
    if(!found){
      null_node=(gutc_json_dom*)calloc(1,sizeof(*null_node));
      if(!null_node)return GUTC_NO_MEMORY;
      null_node->kind=GUTC_JSON_NULL;*out=null_node;return GUTC_OK;
    }
    if(needed==SIZE_MAX)return GUTC_OVERFLOW;
    buffer=(char*)malloc(needed+1);
    if(!buffer)return GUTC_NO_MEMORY;
    rc=gutc_url_get_query_param(url->as.string.bytes,key->as.string.bytes,
                                buffer,needed+1,&needed,&found);
    if(rc==GUTC_OK)rc=make_string(buffer,needed,out);
    free(buffer);return rc;
  }
  if(id_is(id,len,"gutts.urls.setQueryParam")) {
    const gutc_json_dom *key=field(input,"key"),*value=field(input,"value");
    if(!safe_cstr(key)||!safe_cstr(value))return GUTC_INVALID_FORMAT;
    rc=gutc_url_set_query_param(url->as.string.bytes,key->as.string.bytes,
                                value->as.string.bytes,NULL,0,&needed);
    if(rc!=GUTC_OK)return rc;
    if(needed==SIZE_MAX)return GUTC_OVERFLOW;
    buffer=(char*)malloc(needed+1);
    if(!buffer)return GUTC_NO_MEMORY;
    rc=gutc_url_set_query_param(url->as.string.bytes,key->as.string.bytes,
                                value->as.string.bytes,buffer,needed+1,&needed);
    if(rc==GUTC_OK)rc=make_string(buffer,needed,out);
    free(buffer);return rc;
  }
  return GUTC_UNSUPPORTED;
}
gutc_status gutc_dispatch_text_api(const char *id,size_t len,
  const gutc_json_dom *input,gutc_json_dom **output,int *defined) {
  gutc_status rc=GUTC_UNSUPPORTED;
  if(len>=14&&!memcmp(id,"gutts.strings.",14))
    rc=dispatch_string(id,len,input,output);
  else if(len>=11&&!memcmp(id,"gutts.urls.",11))
    rc=dispatch_url(id,len,input,output);
  if(rc==GUTC_OK&&*output)*defined=1;
  return rc;
}
