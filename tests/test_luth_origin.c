#include "gutc/luth_origin.h"
#include "gutc/json_dom.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static gutc_json_dom *parse(const char *text){
  gutc_json_dom *v=NULL;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)text,strlen(text),8,100,&v,NULL);
  assert(rc==GUTC_OK && v);
  return v;
}
static const char *literal(const gutc_json_dom *value){
  if(value->kind==GUTC_JSON_NULL)return NULL;
  assert(value->kind==GUTC_JSON_STRING);
  assert(!memchr(value->as.string.bytes,0,value->as.string.length));
  return value->as.string.bytes;
}
static void run(const char *x,const char *y,const char *expected_json,const char *env_json){
  gutc_json_dom *explicit_origin=parse(x),*fallback=parse(y),
                *expected=parse(expected_json),*environment=parse(env_json);
  char output[4096],sentinel[1]={'Q'};
  size_t required=0,written=0;
  gutc_luth_environment actual;
  const char *a=literal(explicit_origin),*b=literal(fallback),*want=literal(expected);
  gutc_status rc=gutc_luth_resolve_client_origin(a,b,NULL,0,&required);
  assert(rc==GUTC_OK);
  rc=gutc_luth_resolve_client_origin(a,b,sentinel,0,&written);
  assert(rc==GUTC_NO_SPACE && sentinel[0]=='Q');
  assert(required<sizeof(output));
  rc=gutc_luth_resolve_client_origin(a,b,output,sizeof output,&written);
  assert(rc==GUTC_OK && written==required);
  assert(strlen(output)==required && !strcmp(output,want));
  rc=gutc_luth_resolve_client_environment(output,&actual);
  assert(rc==GUTC_OK);
  if(strcmp(gutc_luth_environment_name(actual),literal(environment)))
    fprintf(stderr,"environment %s got %s, wanted %s\n",
            output,gutc_luth_environment_name(actual),literal(environment));
  assert(!strcmp(gutc_luth_environment_name(actual),literal(environment)));
  gutc_json_dom_free(explicit_origin);gutc_json_dom_free(fallback);
  gutc_json_dom_free(expected);gutc_json_dom_free(environment);
}
int main(void){
  FILE *file=fopen(GUTC_LUTH_ORIGIN_GOLDEN_PATH,"r");
  char line[8192];size_t rows=0;
  assert(file);
  while(fgets(line,sizeof line,file)){
    char *a,*b,*c,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    run(line,a,b,c);rows++;
  }
  fclose(file);
  assert(rows>=25);
  assert(!strcmp(gutc_luth_default_url(),
                 "https://luth.blueprint-ninjastudios.workers.dev"));
  {size_t bytes=99;
   assert(gutc_luth_normalize_origin("ftp://example.com",NULL,0,&bytes)==GUTC_UNSUPPORTED);
   assert(gutc_luth_normalize_origin("https://user:pass@example.com",NULL,0,&bytes)==GUTC_UNSUPPORTED);
   assert(gutc_luth_normalize_origin("https://café.example.com",NULL,0,&bytes)==GUTC_UNSUPPORTED);
   assert(gutc_luth_normalize_origin("https://exämple.com",NULL,0,&bytes)==GUTC_UNSUPPORTED);}
  puts("GutC pinned Luth client origin and environment fixtures passed");
  return 0;
}
