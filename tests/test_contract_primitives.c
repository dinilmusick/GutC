#include "gutc/contract_primitives.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gutc_json_dom *parse(const char *json) {
  gutc_json_dom *result=NULL;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)json,strlen(json),
                                     64,10000,&result,NULL);
  assert(rc==GUTC_OK&&result);
  return result;
}
static void run(const char *op,const char *input_text,
                const char *allowed_text,const char *expected_text){
  gutc_json_dom *input=parse(input_text),*allowed=parse(allowed_text);
  int result=0,expect;
  size_t i;
  gutc_json_text view[256];
  gutc_status rc;
  assert(!strcmp(expected_text,"true")||!strcmp(expected_text,"false"));
  expect=!strcmp(expected_text,"true");
  assert(allowed->kind==GUTC_JSON_ARRAY&&allowed->as.array.count<=256);
  for(i=0;i<allowed->as.array.count;i++){
    const gutc_json_dom *v=allowed->as.array.values[i];
    assert(v&&v->kind==GUTC_JSON_STRING);
    view[i]=v->as.string;
  }
  if(!strcmp(op,"record"))result=gutc_contract_record(input)!=NULL;
  else if(!strcmp(op,"id"))result=gutc_contract_id(input);
  else if(!strcmp(op,"appId"))result=gutc_contract_app_id(input);
  else if(!strcmp(op,"keys")){
    rc=gutc_contract_exact_keys(input,view,allowed->as.array.count,&result);
    assert(rc==GUTC_OK);
  }else abort();
  if(result!=expect)fprintf(stderr,"contract %s value %s expected %s got %d\n",
                            op,input_text,expected_text,result);
  assert(result==expect);
  gutc_json_dom_free(input);gutc_json_dom_free(allowed);
}
int main(void){
  FILE *file=fopen(GUTC_CONTRACT_PRIMITIVES_GOLDEN_PATH,"r");
  char line[9000];size_t rows=0;
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
  fclose(file);assert(rows>=60);
  assert(gutc_app_contract_version()==1u);
  {int out=1;
   assert(gutc_contract_exact_keys(NULL,NULL,0,&out)==GUTC_INVALID_FORMAT);
   assert(!out);
   assert(gutc_contract_exact_keys(NULL,NULL,0,NULL)==GUTC_INVALID_ARGUMENT);}
  puts("GutC pinned contract primitive helpers passed");
  return 0;
}
