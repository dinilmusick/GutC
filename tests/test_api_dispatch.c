#include "gutc/api_dispatch.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static gutc_json_dom *parse(const char *s) {
  gutc_json_dom *v=NULL;
  size_t at=0;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)s,strlen(s),64,10000,&v,&at);
  if(rc!=GUTC_OK)fprintf(stderr,"JSON parse failed at %zu for %s\n",at,s);
  assert(rc==GUTC_OK&&v);return v;
}
static int equal(const gutc_json_dom *a,const gutc_json_dom *b,unsigned depth) {
  size_t i;
  if(!a||!b||depth>128||a->kind!=b->kind)return 0;
  switch(a->kind) {
    case GUTC_JSON_NULL:return 1;
    case GUTC_JSON_BOOLEAN:return a->as.boolean==b->as.boolean;
    case GUTC_JSON_NUMBER:return a->as.number==b->as.number;
    case GUTC_JSON_STRING:
      return a->as.string.length==b->as.string.length &&
        (!a->as.string.length||
         memcmp(a->as.string.bytes,b->as.string.bytes,a->as.string.length)==0);
    case GUTC_JSON_ARRAY:
      if(a->as.array.count!=b->as.array.count)return 0;
      for(i=0;i<a->as.array.count;i++)if(!equal(a->as.array.values[i],b->as.array.values[i],depth+1))return 0;
      return 1;
    case GUTC_JSON_OBJECT:
      if(a->as.object.count!=b->as.object.count)return 0;
      for(i=0;i<a->as.object.count;i++) {
        gutc_json_member x=a->as.object.members[i],y=b->as.object.members[i];
        if(x.key.length!=y.key.length||
           (x.key.length&&memcmp(x.key.bytes,y.key.bytes,x.key.length)!=0)||
           !equal(x.value,y.value,depth+1))return 0;
      }
      return 1;
    default:return 0;
  }
}
static void run(const char *id,const char *json,const char *expected) {
  gutc_json_dom *input=parse(json),*actual=NULL,*want=NULL;
  int defined=0;
  gutc_status rc=gutc_execute_api(id,strlen(id),input,NULL,&actual,&defined);
  if(rc!=GUTC_OK)fprintf(stderr,"API %s returned %s for %s\n",
                         id,gutc_status_name(rc),json);
  assert(rc==GUTC_OK);
  if(!strcmp(expected,"_"))assert(!defined&&actual==NULL);
  else {
    assert(defined&&actual);
    want=parse(expected);
    if(!equal(actual,want,0)) {
      size_t size=0;
      char buffer[8192];
      assert(gutc_json_dom_stringify(actual,buffer,sizeof buffer,&size)==GUTC_OK);
      fprintf(stderr,"API %s expected %s actual %s\n",id,expected,buffer);
      abort();
    }
  }
  gutc_json_dom_free(want);
  gutc_json_dom_free(actual);
  gutc_json_dom_free(input);
}
static gutc_status invalid_adapter(void *context,const char *api,size_t n,
                                    const gutc_json_dom *in,gutc_json_dom **out,int *defined) {
  (void)context;(void)api;(void)n;(void)in;
  *out=NULL;*defined=1;
  return GUTC_OK;
}
int main(void) {
  FILE *file=fopen(GUTC_API_DISPATCH_GOLDEN_PATH,"r");
  char line[16000];size_t n=0;
  gutc_json_dom *input,*out=NULL;int defined=1;
  assert(file);
  while(fgets(line,sizeof line,file)){
    char *a,*b,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strrchr(a,'\t');assert(b);*b++='\0';
    run(line,a,b);n++;
  }
  fclose(file);assert(n>=45);
  input=parse("{\"values\":[1,2],\"size\":0}");
  assert(gutc_execute_api("gutts.arrays.chunk",strlen("gutts.arrays.chunk"),input,NULL,&out,&defined)==GUTC_RANGE_ERROR);
  assert(out==NULL&&!defined);
  gutc_json_dom_free(input);
  input=parse("{\"prompt\":\"hello\"}");
  assert(gutc_execute_api("model.smollm2-135m-instruct.generate",strlen("model.smollm2-135m-instruct.generate"),
                          input,NULL,&out,&defined)==GUTC_UNSUPPORTED);
  assert(out==NULL&&!defined);
  {
    const gutc_model_api_adapter adapter={invalid_adapter,NULL};
    assert(gutc_execute_api("model.smollm2-135m-instruct.generate",strlen("model.smollm2-135m-instruct.generate"),
                            input,&adapter,&out,&defined)==GUTC_INVALID_FORMAT);
    assert(out==NULL&&!defined);
  }
  gutc_json_dom_free(input);
  puts("GutC typed runtime dispatch tests passed");
  return 0;
}
