#include "gutc/runtime_app_url.h"
#include "gutc/json_dom.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static gutc_json_dom *parse(const char *source) {
  gutc_json_dom *v=NULL;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)source,strlen(source),
                                     8,100,&v,NULL);
  assert(rc==GUTC_OK&&v);return v;
}
static void check(const char *input,const char *expected){
  gutc_json_dom *in=parse(input),*want=parse(expected);
  char result[8192],marker[1]={'Q'};
  size_t need=0,readback=0;
  const char *text=in->kind==GUTC_JSON_NULL?NULL:in->as.string.bytes;
  gutc_status rc;
  assert(in->kind==GUTC_JSON_STRING||in->kind==GUTC_JSON_NULL);
  assert(want->kind==GUTC_JSON_STRING);
  assert(text==NULL||!memchr(text,0,in->as.string.length));
  rc=gutc_resolve_runtime_app_url(text,NULL,0,&need);
  assert(rc==GUTC_OK);
  assert(gutc_resolve_runtime_app_url(text,marker,0,&readback)==GUTC_NO_SPACE);
  assert(marker[0]=='Q'&&need==readback);
  assert(need<sizeof(result));
  rc=gutc_resolve_runtime_app_url(text,result,sizeof result,&readback);
  assert(rc==GUTC_OK&&readback==need);
  assert(need==want->as.string.length);
  if(need!=want->as.string.length||memcmp(result,want->as.string.bytes,need))
    fprintf(stderr,"Trim mismatch input=%s expected=%s actual=%s\n",
            input,expected,result);
  assert(!memcmp(result,want->as.string.bytes,need));
  rc=gutc_resolve_server_runtime_app_url(text,result,sizeof result,&readback);
  assert(rc==GUTC_OK&&readback==need);
  assert(!memcmp(result,want->as.string.bytes,need));
  gutc_json_dom_free(in);gutc_json_dom_free(want);
}
int main(void){
  FILE *f=fopen(GUTC_RUNTIME_APP_URL_GOLDEN_PATH,"r");
  char line[16000];size_t rows=0;
  assert(f);
  while(fgets(line,sizeof line,f)){
    char *tab,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    tab=strchr(line,'\t');assert(tab);*tab++='\0';
    check(line,tab);rows++;
  }
  fclose(f);assert(rows>=30);
  {char output[2]={'X','Y'};size_t needed=0;
   const char bad[]={(char)0xC0,(char)0xAF,0};
   assert(gutc_resolve_runtime_app_url(bad,output,sizeof output,&needed)==GUTC_INVALID_ENCODING);
   assert(output[0]=='X'&&output[1]=='Y');
   assert(gutc_resolve_server_runtime_app_url(NULL,output,1,&needed)==GUTC_OK);
   assert(needed==0&&output[0]=='\0');}
  puts("GutC pinned browser and server runtime APP_URL fixtures passed");
  return 0;
}
