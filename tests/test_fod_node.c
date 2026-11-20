#include "gutc/fod_node.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void exercise(const char *source,const char *expected) {
  gutc_json_dom *api=NULL,*node=NULL;
  size_t needed=0;
  char output[16000];
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)source,strlen(source),
                                     64,10000,&api,NULL);
  assert(rc==GUTC_OK&&api);
  rc=gutc_api_to_fod_node(api,&node);
  if(rc!=GUTC_OK)fprintf(stderr,"FOD conversion failed: %s\n",gutc_status_name(rc));
  assert(rc==GUTC_OK&&node);
  gutc_json_dom_free(api);
  assert(gutc_json_dom_is_json_value(node));
  assert(gutc_json_dom_stringify(node,NULL,0,&needed)==GUTC_OK);
  assert(needed==strlen(expected));
  assert(gutc_json_dom_stringify(node,output,sizeof output,&needed)==GUTC_OK);
  if(strcmp(output,expected))
    fprintf(stderr,"FOD wanted %s\nactual %s\n",expected,output);
  assert(!strcmp(output,expected));
  gutc_json_dom_free(node);
}
int main(void) {
  FILE *file=fopen(GUTC_FOD_GOLDEN_PATH,"r");
  char line[40000];size_t count=0;
  assert(file);
  while(fgets(line,sizeof line,file)) {
    char *tab,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    tab=strchr(line,'\t');assert(tab);*tab++='\0';
    exercise(line,tab);count++;
  }
  fclose(file);assert(count>=8);
  {
    gutc_json_dom *bad=NULL,*out=NULL;
    const char *text="{\"id\":\"bad\"}";
    assert(gutc_json_dom_parse((const uint8_t*)text,strlen(text),
                               8,100,&bad,NULL)==GUTC_OK);
    assert(gutc_api_to_fod_node(bad,&out)==GUTC_INVALID_FORMAT&&out==NULL);
    gutc_json_dom_free(bad);
  }
  puts("GutC pinned FOD node metadata fixtures passed");
  return 0;
}
