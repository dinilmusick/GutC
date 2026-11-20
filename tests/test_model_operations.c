#include "gutc/model_operations.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct model_mock {
  const char *expected_id,*expected_operation,*result_json;
  int loads,invokes,unloads;
  int model;
} model_mock;
static gutc_status load(void *context,const gutc_json_dom *metadata,
  const gutc_model_load_context *load_context,void **model){
  model_mock *m=(model_mock*)context;
  const gutc_json_dom *id=gutc_json_dom_get(metadata,"id",2);
  assert(load_context&&id&&id->kind==GUTC_JSON_STRING);
  assert(strlen(m->expected_id)==id->as.string.length);
  assert(!memcmp(m->expected_id,id->as.string.bytes,id->as.string.length));
  m->loads++;*model=&m->model;return GUTC_OK;
}
static gutc_status unload(void *context,void *model){
  model_mock *m=(model_mock*)context;
  assert(model==&m->model);m->unloads++;return GUTC_OK;
}
static gutc_status invoke(void *context,void *model,const char *operation,
  const gutc_json_dom *input,gutc_json_dom **output){
  model_mock *m=(model_mock*)context;
  assert(model==&m->model&&operation&&input&&input->kind==GUTC_JSON_OBJECT);
  assert(!strcmp(operation,m->expected_operation));
  m->invokes++;
  return gutc_json_dom_parse((const unsigned char*)m->result_json,
    strlen(m->result_json),32,1024,output,NULL);
}
static gutc_json_dom *parse(const char *json){
 gutc_json_dom *v=NULL;
 assert(gutc_json_dom_parse((const unsigned char*)json,
   strlen(json),64,10000,&v,NULL)==GUTC_OK&&v);
 return v;
}
static void check(const char *name,const char *id,const char *method,
                  const char *input_json,const char *output_json){
 gutc_model_client client={0},without_host={0};
 gutc_model_runtime_host host={0};
 model_mock mock={0};
 gutc_json_dom *input=parse(input_json),*output=NULL,*expected=parse(output_json);
 char got[4096],want[4096];
 size_t got_n=0,want_n=0,i;
 int found=0;
 gutc_status rc=GUTC_UNSUPPORTED;
 for(i=0;i<GUTC_MODEL_FAMILY_COUNT;i++)
  if(!strcmp(gutc_model_family_name((gutc_model_family)i),name)){
   assert(!strcmp(gutc_model_family_id((gutc_model_family)i),id));
   found=1;break;
  }
 assert(found);
 mock.expected_id=id;mock.expected_operation=method;mock.result_json=output_json;
 host.context=&mock;host.load=load;host.unload=unload;host.invoke=invoke;
 assert(gutc_model_client_init(&client,(gutc_model_family)i,&host)==GUTC_OK);
 rc=gutc_model_client_call(&client,method,input,&output);
 if(rc!=GUTC_OK)fprintf(stderr,"model call %s.%s returned %d\n",name,method,(int)rc);
 assert(rc==GUTC_OK&&output);
 assert(gutc_json_dom_stringify(output,got,sizeof got,&got_n)==GUTC_OK);
 assert(gutc_json_dom_stringify(expected,want,sizeof want,&want_n)==GUTC_OK);
 if(strcmp(got,want))fprintf(stderr,"%s got %s expected %s\n",name,got,want);
 assert(!strcmp(got,want));
 assert(mock.loads==1&&mock.unloads==1&&mock.invokes==1);
 assert(!gutc_model_residency_is_loaded(&client.library.residency));
 gutc_json_dom_free(output);output=NULL;
 assert(gutc_model_client_call(&client,"unpublished",input,&output)==GUTC_INVALID_FORMAT);
 assert(!output);
 assert(mock.invokes==1);
 /* Host may return malformed output, but that must fail closed and unload. */
 if(!strcmp(method,"rerank")||!strcmp(method,"similarity")||
    !strcmp(method,"generate")||!strcmp(method,"transcribe")||
    !strcmp(method,"embed")){
  mock.result_json="null";
  assert(gutc_model_client_call(&client,method,input,&output)==GUTC_INVALID_FORMAT);
  assert(!output);
  assert(mock.loads==2&&mock.unloads==2);
 }
 assert(gutc_model_client_destroy(&client)==GUTC_OK);
 assert(gutc_model_client_init(&without_host,(gutc_model_family)i,NULL)==GUTC_OK);
 assert(gutc_model_client_call(&without_host,method,input,&output)==GUTC_UNSUPPORTED);
 assert(!output);
 assert(gutc_model_client_destroy(&without_host)==GUTC_OK);
 gutc_json_dom_free(input);gutc_json_dom_free(expected);
}
int main(void){
 FILE *f=fopen(GUTC_MODEL_OPERATIONS_PATH,"r");
 char line[16384];size_t cases=0;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *a,*b,*c,*end;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  a=strchr(line,'\t');assert(a);*a++='\0';
  b=strchr(a,'\t');assert(b);*b++='\0';
  c=strchr(b,'\t');assert(c);*c++='\0';
  end=strchr(c,'\t');assert(end);*end++='\0';
  check(line,a,b,c,end);cases++;
 }
 fclose(f);assert(cases==9);
 assert(gutc_model_family_name((gutc_model_family)999)==NULL);
 assert(gutc_model_family_id((gutc_model_family)999)==NULL);
 {
  gutc_model_client c={0};
  assert(gutc_model_client_init(&c,(gutc_model_family)999,NULL)==GUTC_RANGE_ERROR);
  assert(gutc_model_client_destroy(NULL)==GUTC_INVALID_ARGUMENT);
 }
 puts("GutC host-backed typed model operations source contract tests passed");
 return 0;
}
