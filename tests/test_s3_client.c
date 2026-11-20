#include "gutc/s3_client.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct store {
  unsigned char primary[128],copy[128];
  size_t primary_size,copy_size;
  int primary_exists,copy_exists;
  unsigned calls[GUTC_S3_COMMAND_COUNT],binary_frees;
} store;
static const gutc_json_dom *prop(const gutc_json_dom *o,const char *name){
 return gutc_json_dom_get(o,name,strlen(name));
}
static int eq(const gutc_json_dom *value,const char *s){
 return value&&value->kind==GUTC_JSON_STRING&&value->as.string.length==strlen(s)&&
        !memcmp(value->as.string.bytes,s,strlen(s));
}
static gutc_status parse_result(const char *json,gutc_s3_result *r){
 return gutc_json_dom_parse((const uint8_t*)json,strlen(json),32,1000,&r->metadata,NULL);
}
static void release_body(void *ctx,uint8_t *bytes,size_t length){
 store *s=(store*)ctx;
 (void)length;
 s->binary_frees++;
 free(bytes);
}
static gutc_status execute(void *ctx,const char *region,const char *endpoint,
 const gutc_s3_request *req,gutc_s3_result *out){
 store *s=(store*)ctx;
 const gutc_json_dom *key=prop(req->input,"Key");
 assert(!strcmp(region,"us-east-1")&&!strcmp(endpoint,"https://s3.example"));
 assert(req->command>=GUTC_S3_GET_OBJECT&&req->command<GUTC_S3_COMMAND_COUNT);
 s->calls[req->command]++;
 if(req->command==GUTC_S3_PUT_OBJECT){
  assert(eq(key,"primary")&&req->body&&req->body_length==5);
  memcpy(s->primary,req->body,req->body_length);
  s->primary_size=req->body_length;s->primary_exists=1;
  out->http_status=200;return parse_result("{\"ETag\":\"test-hash\"}",out);
 }
 if(req->command==GUTC_S3_GET_OBJECT){
  if(!eq(key,"primary")||!s->primary_exists){
   out->http_status=404;return GUTC_OK;
  }
  out->body=(uint8_t*)malloc(s->primary_size);
  if(!out->body)return GUTC_NO_MEMORY;
  memcpy(out->body,s->primary,s->primary_size);
  out->body_length=s->primary_size;
  out->release_body=release_body;out->release_context=s;
  out->http_status=200;
  return parse_result("{\"ETag\":\"test-hash\"}",out);
 }
 if(req->command==GUTC_S3_HEAD_OBJECT){
  assert(eq(key,"primary")&&s->primary_exists);
  out->http_status=200;return parse_result("{\"ContentLength\":5}",out);
 }
 if(req->command==GUTC_S3_LIST_OBJECTS_V2){
  out->http_status=200;return parse_result("{\"KeyCount\":1}",out);
 }
 if(req->command==GUTC_S3_COPY_OBJECT){
  const gutc_json_dom *src=prop(req->input,"CopySource");
  assert(eq(src,"test-bucket/primary")&&eq(key,"copy"));
  assert(s->primary_exists);
  memcpy(s->copy,s->primary,s->primary_size);s->copy_size=s->primary_size;
  s->copy_exists=1;
  out->http_status=200;return parse_result("{\"CopyObjectResult\":{}}",out);
 }
 if(req->command==GUTC_S3_DELETE_OBJECT){
  assert(eq(key,"copy")&&s->copy_exists);
  s->copy_exists=0;out->http_status=204;return GUTC_OK;
 }
 if(req->command==GUTC_S3_DELETE_OBJECTS){
  const gutc_json_dom *deleted=prop(req->input,"Delete");
  const gutc_json_dom *items=prop(deleted,"Objects");
  assert(items&&items->kind==GUTC_JSON_ARRAY&&items->as.array.count==1);
  assert(eq(prop(items->as.array.values[0],"Key"),"primary"));
  s->primary_exists=0;out->http_status=200;
  return parse_result("{\"Deleted\":[{\"Key\":\"primary\"}]}",out);
 }
 return GUTC_UNSUPPORTED;
}
static gutc_json_dom *parse(const char *input){
 gutc_json_dom *out=NULL;
 assert(gutc_json_dom_parse((const uint8_t*)input,strlen(input),32,1000,&out,NULL)==GUTC_OK);
 assert(out);return out;
}
static void send(gutc_s3_client *c,gutc_s3_command command,const char *json,
 const uint8_t *body,size_t len,gutc_s3_result *out){
 gutc_json_dom *input=parse(json);
 gutc_s3_request req={command,input,body,len};
 gutc_status rc=gutc_s3_client_execute(c,&req,out);
 if(rc!=GUTC_OK)fprintf(stderr,"S3 command %s returned %d\n",
                        gutc_s3_command_name(command),(int)rc);
 assert(rc==GUTC_OK);
 gutc_json_dom_free(input);
}
int main(void){
 FILE *f=fopen(GUTC_S3_COMMANDS_PATH,"r");
 char line[1024];
 size_t n=0;
 store storage={0};
 gutc_s3_host host={&storage,execute};
 gutc_s3_client client={0},without_host={0};
 gutc_s3_result result={0};
 const char bucket[]="test-bucket";
 const char key[]="primary";
 const char *obj="{\"Bucket\":\"test-bucket\",\"Key\":\"primary\"}";
 static const uint8_t content[]={'h','e','l','l','o'};
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *idx,*end;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  idx=strchr(line,'\t');assert(idx);*idx++='\0';
  assert((size_t)strtoul(idx,NULL,10)==n);
  assert(!strcmp(gutc_s3_command_name((gutc_s3_command)n),line));n++;
 }
 fclose(f);assert(n==(size_t)GUTC_S3_COMMAND_COUNT);
 assert(gutc_s3_command_name((gutc_s3_command)999)==NULL);
 assert(gutc_s3_client_init(&client,"us-east-1",9,"https://s3.example",
                            strlen("https://s3.example"),&host)==GUTC_OK);
 send(&client,GUTC_S3_PUT_OBJECT,obj,content,sizeof content,&result);
 assert(result.http_status==200);gutc_s3_result_clear(&result);
 send(&client,GUTC_S3_GET_OBJECT,obj,NULL,0,&result);
 assert(result.body_length==5&&!memcmp(result.body,content,5));
 gutc_s3_result_clear(&result);
 assert(storage.binary_frees==1);
 send(&client,GUTC_S3_HEAD_OBJECT,obj,NULL,0,&result);
 assert(prop(result.metadata,"ContentLength")->as.number==5);
 gutc_s3_result_clear(&result);
 send(&client,GUTC_S3_LIST_OBJECTS_V2,"{\"Bucket\":\"test-bucket\",\"Prefix\":\"\"}",NULL,0,&result);
 assert(prop(result.metadata,"KeyCount")->as.number==1);
 gutc_s3_result_clear(&result);
 send(&client,GUTC_S3_COPY_OBJECT,
      "{\"Bucket\":\"test-bucket\",\"Key\":\"copy\",\"CopySource\":\"test-bucket/primary\"}",
      NULL,0,&result);
 assert(storage.copy_exists);
 gutc_s3_result_clear(&result);
 send(&client,GUTC_S3_DELETE_OBJECT,"{\"Bucket\":\"test-bucket\",\"Key\":\"copy\"}",NULL,0,&result);
 assert(!storage.copy_exists);gutc_s3_result_clear(&result);
 send(&client,GUTC_S3_DELETE_OBJECTS,
      "{\"Bucket\":\"test-bucket\",\"Delete\":{\"Objects\":[{\"Key\":\"primary\"}]}}",
      NULL,0,&result);
 assert(!storage.primary_exists);gutc_s3_result_clear(&result);
 {gutc_json_dom *input=parse(obj);
  gutc_s3_request req={GUTC_S3_GET_OBJECT,input,NULL,0};
  assert(gutc_s3_client_execute(&client,&req,&result)==GUTC_INVALID_FORMAT);
  assert(result.body==NULL&&result.metadata==NULL);
  gutc_json_dom_free(input);}
 for(n=0;n<(size_t)GUTC_S3_COMMAND_COUNT;n++)assert(storage.calls[n]>=1);
 assert(gutc_s3_client_init(&without_host,"us-east-1",9,NULL,0,NULL)==GUTC_OK);
 {gutc_json_dom *input=parse(obj);
  gutc_s3_request req={GUTC_S3_GET_OBJECT,input,NULL,0};
  assert(gutc_s3_client_execute(&without_host,&req,&result)==GUTC_UNSUPPORTED);
  gutc_json_dom_free(input);}
 {gutc_json_dom *input=parse("{\"Bucket\":\"test-bucket\",\"Delete\":{\"Objects\":[]}}");
  gutc_s3_request req={GUTC_S3_DELETE_OBJECTS,input,NULL,0};
  assert(gutc_s3_client_execute(&client,&req,&result)==GUTC_INVALID_FORMAT);
  gutc_json_dom_free(input);}
 assert(gutc_s3_client_init(&client,"bad region",10,NULL,0,&host)==GUTC_INVALID_FORMAT);
 assert(bucket[0]=='t'&&key[0]=='p');
 gutc_s3_result_clear(&result);
 puts("GutC host-executed S3 command validation, binary ownership and negative tests passed");
 return 0;
}
