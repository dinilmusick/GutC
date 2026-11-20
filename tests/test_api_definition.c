#include "gutc/api_definition.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void run(const char *input,const char *expected) {
  gutc_json_dom *source=NULL,*api=NULL;
  char buffer[16384],sentinel[]="keep";
  size_t at=0,needed=0;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)input,strlen(input),32,10000,&source,&at);
  assert(rc==GUTC_OK && source!=NULL);
  rc=gutc_define_api(source,&api);
  if(rc!=GUTC_OK)fprintf(stderr,"defineApi failed: %s\n",gutc_status_name(rc));
  assert(rc==GUTC_OK && api!=NULL);
  /* Output must be independently owned and valid after input release. */
  gutc_json_dom_free(source);
  assert(gutc_json_dom_is_json_value(api));
  assert(gutc_json_dom_stringify(api,NULL,0,&needed)==GUTC_OK);
  assert(needed==strlen(expected));
  assert(gutc_json_dom_stringify(api,sentinel,sizeof(sentinel),&needed)==GUTC_NO_SPACE);
  assert(!strcmp(sentinel,"keep"));
  assert(gutc_json_dom_stringify(api,buffer,sizeof(buffer),&needed)==GUTC_OK);
  if(strcmp(expected,buffer))fprintf(stderr,"defineApi mismatch\nwanted %s\ngot %s\n",expected,buffer);
  assert(!strcmp(expected,buffer));
  gutc_json_dom_free(api);
}
int main(void) {
  FILE *file=fopen(GUTC_API_GOLDEN_PATH,"r");
  char line[32000];
  size_t count=0;
  assert(file);
  while(fgets(line,sizeof(line),file)) {
    char *tab,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    tab=strchr(line,'\t');assert(tab);*tab++='\0';
    run(line,tab);count++;
  }
  fclose(file);assert(count>=8);
  {
    gutc_json_dom *bad=NULL,*out=NULL;
    const char *json="{\"name\":23,\"parameters\":[]}";
    assert(gutc_json_dom_parse((const uint8_t*)json,strlen(json),32,1000,&bad,NULL)==GUTC_OK);
    assert(gutc_define_api(bad,&out)==GUTC_INVALID_FORMAT && out==NULL);
    gutc_json_dom_free(bad);
  }
  puts("GutC pinned defineApi metadata fixtures passed");
  return 0;
}
