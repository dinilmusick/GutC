#include "gutc/model_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct model_mock {
 char events[1024];
 int model_tag,load_count,unload_count,invoke_count;
 int fail_load,fail_invoke,fail_unload;
} model_mock;
static void event(model_mock *m,const char *name){
 size_t offset=strlen(m->events),n=strlen(name);
 assert(offset+n+2<sizeof(m->events));
 if(offset)m->events[offset++]=',';
 memcpy(m->events+offset,name,n+1);
}
static gutc_status load(void *context,const gutc_json_dom *metadata,
 const gutc_model_load_context *load_context,void **out_model){
 model_mock *m=(model_mock*)context;
 const gutc_json_dom *id=gutc_json_dom_get(metadata,"id",2);
 assert(id&&id->kind==GUTC_JSON_STRING&&id->as.string.length>0);
 assert(load_context);
 event(m,"load");m->load_count++;
 if(m->fail_load)return GUTC_NO_MEMORY;
 *out_model=&m->model_tag;
 return GUTC_OK;
}
static gutc_status unload(void *context,void *model){
 model_mock *m=(model_mock*)context;
 assert(model==&m->model_tag);
 event(m,"unload");m->unload_count++;
 return m->fail_unload?GUTC_INVALID_FORMAT:GUTC_OK;
}
static gutc_status invoke(void *context,void *model,const char *operation,
 const gutc_json_dom *input,gutc_json_dom **out){
 model_mock *m=(model_mock*)context;
 static const char result[]="{\"output\":\"test-host-only\"}";
 assert(model==&m->model_tag&&operation&&!strcmp(operation,"test"));
 assert(input&&input->kind==GUTC_JSON_OBJECT);
 event(m,"operation");m->invoke_count++;
 if(m->fail_invoke)return GUTC_INVALID_FORMAT;
 return gutc_json_dom_parse((const uint8_t*)result,sizeof(result)-1,16,100,out,NULL);
}
static gutc_json_dom *parse(const char *json){
 gutc_json_dom *out=NULL;
 assert(gutc_json_dom_parse((const uint8_t*)json,strlen(json),16,100,&out,NULL)==GUTC_OK);
 return out;
}
static gutc_model_runtime_host make_host(model_mock *mock){
 gutc_model_runtime_host host={0};
 host.context=mock;host.load=load;host.unload=unload;host.invoke=invoke;
 return host;
}
static void one(const char *scenario,const char *expected){
 model_mock mock={0};
 gutc_model_runtime_host host=make_host(&mock);
 gutc_model_library library={0};
 gutc_json_dom *input=parse("{}"),*result=NULL;
 gutc_status rc;
 assert(input);
 if(!strcmp(scenario,"load-failure"))mock.fail_load=1;
 if(!strcmp(scenario,"operation-failure"))mock.fail_invoke=1;
 rc=gutc_model_library_init(&library,"model.cactus-needle3",
   strlen("model.cactus-needle3"),&host);
 assert(rc==GUTC_OK&&library.metadata);
 if(!strcmp(scenario,"sequential")){
  rc=gutc_model_library_invoke(&library,"test",input,&result);
  assert(rc==GUTC_OK&&result);gutc_json_dom_free(result);result=NULL;
  rc=gutc_model_library_invoke(&library,"test",input,&result);
  assert(rc==GUTC_OK&&result);gutc_json_dom_free(result);result=NULL;
 }else if(!strcmp(scenario,"nested")){
  gutc_model_lease outer={0};
  rc=gutc_model_residency_acquire(&library.residency,&library.runtime,
                                   library.metadata,&outer);
  assert(rc==GUTC_OK&&outer.active);
  assert(gutc_model_residency_is_loaded(&library.residency));
  assert(gutc_model_residency_users(&library.residency)==1);
  event(&mock,"outer");
  rc=gutc_model_library_invoke(&library,"test",input,&result);
  assert(rc==GUTC_OK&&result);
  assert(gutc_model_residency_users(&library.residency)==1);
  gutc_json_dom_free(result);result=NULL;
  assert(gutc_model_lease_release(&outer)==GUTC_OK);
  assert(!outer.active);
  assert(gutc_model_lease_release(&outer)==GUTC_INVALID_ARGUMENT);
 }else if(!strcmp(scenario,"operation-failure")){
  assert(gutc_model_library_invoke(&library,"test",input,&result)==GUTC_INVALID_FORMAT);
  assert(!result);
 }else if(!strcmp(scenario,"load-failure")){
  assert(gutc_model_library_invoke(&library,"test",input,&result)==GUTC_NO_MEMORY);
  assert(!result);
 }else if(!strcmp(scenario,"library")){
  rc=gutc_model_library_invoke(&library,"test",input,&result);
  assert(rc==GUTC_OK&&result);gutc_json_dom_free(result);result=NULL;
 }else abort();
 if(strcmp(mock.events,expected))
  fprintf(stderr,"Model host sequence %s actual %s wanted %s\n",scenario,mock.events,expected);
 assert(!strcmp(mock.events,expected));
 assert(gutc_model_residency_users(&library.residency)==0);
 assert(!gutc_model_residency_is_loaded(&library.residency));
 assert(gutc_model_library_destroy(&library)==GUTC_OK);
 gutc_json_dom_free(input);
}
static void failure_cleanup(void){
 model_mock mock={0};
 gutc_model_runtime_host host=make_host(&mock);
 gutc_model_library library={0};
 gutc_json_dom *input=parse("{}"),*result=NULL;
 gutc_model_size size={GUTC_MODEL_ARTIFACT_SIZE,GUTC_MODEL_MB,12,0,0};
 assert(size.kind==GUTC_MODEL_ARTIFACT_SIZE&&size.unit==GUTC_MODEL_MB);
 assert(gutc_model_library_init(&library,"unknown",7,&host)==GUTC_UNSUPPORTED);
 assert(!library.metadata);
 assert(gutc_model_library_init(&library,"model.cactus-needle3",
                              strlen("model.cactus-needle3"),NULL)==GUTC_OK);
 assert(gutc_model_library_invoke(&library,"test",input,&result)==GUTC_UNSUPPORTED);
 assert(!result);
 assert(gutc_model_library_destroy(&library)==GUTC_OK);
 assert(gutc_model_library_init(&library,"model.cactus-needle3",
                              strlen("model.cactus-needle3"),&host)==GUTC_OK);
 mock.fail_unload=1;
 assert(gutc_model_library_invoke(&library,"test",input,&result)==GUTC_INVALID_FORMAT);
 assert(!result&&library.residency.cleanup_pending);
 assert(gutc_model_residency_is_loaded(&library.residency));
 assert(gutc_model_library_destroy(&library)==GUTC_INVALID_FORMAT);
 mock.fail_unload=0;
 assert(gutc_model_residency_cleanup(&library.residency)==GUTC_OK);
 assert(!library.residency.cleanup_pending);
 assert(gutc_model_library_destroy(&library)==GUTC_OK);
 gutc_json_dom_free(input);
}
int main(void){
 FILE *f=fopen(GUTC_MODEL_RESIDENCY_GOLDEN_PATH,"r");
 char line[4096];size_t cases=0;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *tab,*end;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  tab=strchr(line,'\t');assert(tab);*tab++='\0';
  one(line,tab);cases++;
 }
 fclose(f);assert(cases==5);
 failure_cleanup();
 puts("GutC source-derived model residency and host runtime lifecycle tests passed");
 return 0;
}
