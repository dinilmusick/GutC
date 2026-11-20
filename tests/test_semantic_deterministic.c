#include "gutc/semantic_deterministic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static gutc_json_dom *parse(const char *json) {
  gutc_json_dom *v=NULL;
  size_t at=0;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)json,strlen(json),32,10000,&v,&at);
  if(rc!=GUTC_OK)fprintf(stderr,"semantic source fixture parse: %s at %zu\n",gutc_status_name(rc),at);
  assert(rc==GUTC_OK&&v!=NULL);
  return v;
}
static void one(const char *input_json,const char *schema_json,const char *expected) {
  gutc_json_dom *input=parse(input_json),*schema=parse(schema_json),*output=NULL;
  int resolved=0;
  gutc_status rc;
  char actual[8192];
  size_t n=0;
  assert(input->kind==GUTC_JSON_STRING);
  rc=gutc_resolve_deterministic_semantic(input->as.string.bytes,
          input->as.string.length,schema,&output,&resolved);
  if(rc!=GUTC_OK)fprintf(stderr,"semantic error %s for %s\n",
                          gutc_status_name(rc),input_json);
  assert(rc==GUTC_OK);
  if(!strcmp(expected,"_")){
    assert(!resolved && output==NULL);
  } else {
    assert(resolved && output!=NULL);
    assert(gutc_json_dom_stringify(output,actual,sizeof actual,&n)==GUTC_OK);
    if(strcmp(actual,expected))
      fprintf(stderr,"semantic %s expected %s actual %s\n",input_json,expected,actual);
    assert(!strcmp(actual,expected));
  }
  gutc_json_dom_free(input);
  gutc_json_dom_free(schema);
  gutc_json_dom_free(output);
}
int main(void) {
  FILE *file=fopen(GUTC_SEMANTIC_GOLDEN_PATH,"r");
  char line[16000];
  size_t count=0;
  assert(file);
  while(fgets(line,sizeof(line),file)){
    char *a,*b,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    one(line,a,b);count++;
  }
  fclose(file);assert(count>=30);
  {
    gutc_json_dom *schema=parse("{\"type\":\"string\",\"pattern\":\"^a+$\"}");
    gutc_json_dom *out=NULL;int resolved=0;
    assert(gutc_resolve_deterministic_semantic("aaa",3,schema,&out,&resolved)==GUTC_UNSUPPORTED);
    assert(out==NULL&&!resolved);
    gutc_json_dom_free(schema);
  }
  puts("GutC deterministic semantic golden fixtures passed");
  return 0;
}
