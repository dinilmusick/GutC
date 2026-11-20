#include "gutc/s3_client.h"
#include "gutc/gutc.h"
#include <stdlib.h>
#include <string.h>
static const char *const names[GUTC_S3_COMMAND_COUNT]={
 "GetObjectCommand","HeadObjectCommand","ListObjectsV2Command",
 "PutObjectCommand","CopyObjectCommand","DeleteObjectCommand",
 "DeleteObjectsCommand"
};
const char *gutc_s3_command_name(gutc_s3_command cmd){
 return (unsigned)cmd<GUTC_S3_COMMAND_COUNT?names[(size_t)cmd]:NULL;
}
static const gutc_json_dom *prop(const gutc_json_dom *o,const char *name){
 return gutc_json_dom_get(o,name,strlen(name));
}
static int has_string(const gutc_json_dom *o,const char *name){
 const gutc_json_dom *v=prop(o,name);
 size_t codepoints=0;
 return v&&v->kind==GUTC_JSON_STRING&&v->as.string.length>0&&
   v->as.string.bytes&&
   gutc_utf8_validate((const uint8_t*)v->as.string.bytes,
                      v->as.string.length,&codepoints)==GUTC_OK;
}
static int valid_input(const gutc_s3_request *req){
 const gutc_json_dom *deletes,*items,*max_keys;
 size_t i;
 if(!req||!gutc_s3_command_name(req->command)||!req->input||
    req->input->kind!=GUTC_JSON_OBJECT||
    (!req->body&&req->body_length))
  return 0;
 if(!has_string(req->input,"Bucket"))return 0;
 switch(req->command){
 case GUTC_S3_GET_OBJECT:
 case GUTC_S3_HEAD_OBJECT:
 case GUTC_S3_PUT_OBJECT:
 case GUTC_S3_DELETE_OBJECT:
  return has_string(req->input,"Key");
 case GUTC_S3_COPY_OBJECT:
  return has_string(req->input,"Key")&&has_string(req->input,"CopySource");
 case GUTC_S3_LIST_OBJECTS_V2:
  max_keys=prop(req->input,"MaxKeys");
  if(max_keys&&(max_keys->kind!=GUTC_JSON_NUMBER||
      max_keys->as.number<0||max_keys->as.number>1000||
      (double)(unsigned)max_keys->as.number!=max_keys->as.number))
    return 0;
  return 1;
 case GUTC_S3_DELETE_OBJECTS:
  deletes=prop(req->input,"Delete");
  if(!deletes||deletes->kind!=GUTC_JSON_OBJECT)return 0;
  items=prop(deletes,"Objects");
  if(!items||items->kind!=GUTC_JSON_ARRAY||
     items->as.array.count==0||items->as.array.count>1000||
     !items->as.array.values)return 0;
  for(i=0;i<items->as.array.count;i++){
   const gutc_json_dom *entry=items->as.array.values[i];
   if(!entry||entry->kind!=GUTC_JSON_OBJECT||!has_string(entry,"Key"))
    return 0;
  }
  return 1;
 default:return 0;
 }
}
gutc_status gutc_s3_client_init(gutc_s3_client *c,
 const char *region,size_t region_len,const char *endpoint,size_t endpoint_len,
 const gutc_s3_host *host){
 size_t i;
 if(!c||!region||!region_len||region_len>=sizeof(c->region)||
    (!endpoint&&endpoint_len)||endpoint_len>=sizeof(c->endpoint))
  return GUTC_INVALID_ARGUMENT;
 for(i=0;i<region_len;i++){
  unsigned char ch=(unsigned char)region[i];
  if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
       (ch>='0'&&ch<='9')||ch=='-'))return GUTC_INVALID_FORMAT;
 }
 if(endpoint_len&&memchr(endpoint,0,endpoint_len))return GUTC_INVALID_FORMAT;
 memset(c,0,sizeof(*c));
 memcpy(c->region,region,region_len);c->region[region_len]='\0';
 if(endpoint_len){
  /* Optional endpoint is a trusted host setting, not untrusted user input.
     The host MUST enforce TLS, host allow-list and AWS SigV4 signing. */
  memcpy(c->endpoint,endpoint,endpoint_len);
  c->endpoint[endpoint_len]='\0';
 }
 if(host)c->host=*host;
 return GUTC_OK;
}
void gutc_s3_result_clear(gutc_s3_result *r){
 if(!r)return;
 gutc_json_dom_free(r->metadata);
 if(r->body){
  if(r->release_body)
   r->release_body(r->release_context,r->body,r->body_length);
  else free(r->body); /* unhooked host must have allocated via malloc */
 }
 memset(r,0,sizeof(*r));
}
gutc_status gutc_s3_client_execute(gutc_s3_client *c,
 const gutc_s3_request *req,gutc_s3_result *out){
 gutc_status rc;
 if(!c||!out||!req)return GUTC_INVALID_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(!valid_input(req))return GUTC_INVALID_FORMAT;
 if(!c->host.execute)return GUTC_UNSUPPORTED;
 rc=c->host.execute(c->host.context,c->region,c->endpoint,req,out);
 if(rc!=GUTC_OK){gutc_s3_result_clear(out);return rc;}
 if(out->http_status<200||out->http_status>=300||
    (out->metadata&&!gutc_json_dom_is_json_value(out->metadata))||
    (!out->body&&out->body_length)){
  gutc_s3_result_clear(out);return GUTC_INVALID_FORMAT;
 }
 return GUTC_OK;
}
