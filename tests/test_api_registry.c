#include "gutc/api_registry.h"
#include "gutc/fod_node.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  FILE *f=fopen(GUTC_API_CATALOG_PATH,"rb");
  long file_size;
  char *expected=NULL,*actual=NULL;
  size_t i,required=0;
  gutc_status rc;
  gutc_json_dom *catalog=NULL,*fod=NULL;
  assert(f);
  assert(fseek(f,0,SEEK_END)==0);
  file_size=ftell(f);
  assert(file_size>2&&file_size<1048576L);
  assert(fseek(f,0,SEEK_SET)==0);
  expected=(char*)malloc((size_t)file_size+1);
  assert(expected);
  assert(fread(expected,1,(size_t)file_size,f)==(size_t)file_size);
  fclose(f);
  expected[file_size]='\0';
  while(file_size>0&&(expected[file_size-1]=='\n'||expected[file_size-1]=='\r'))
    expected[--file_size]='\0';
  assert(gutc_api_registry_count()==38);
  rc=gutc_api_registry_list(&catalog);
  assert(rc==GUTC_OK && catalog && catalog->kind==GUTC_JSON_ARRAY);
  assert(catalog->as.array.count==38);
  assert(gutc_json_dom_stringify(catalog,NULL,0,&required)==GUTC_OK);
  actual=(char*)malloc(required+1);
  assert(actual);
  assert(gutc_json_dom_stringify(catalog,actual,required+1,&required)==GUTC_OK);
  if(strcmp(actual,expected))
    fprintf(stderr,"Catalog serialization diverged from pinned GutTS snapshot\n");
  assert(strcmp(actual,expected)==0);
  for(i=0;i<gutc_api_registry_count();i++) {
    const gutc_json_dom *item=catalog->as.array.values[i];
    const gutc_json_dom *id=gutc_json_dom_get(item,"id",2);
    gutc_json_dom *by_index=NULL,*by_id=NULL,*node=NULL;
    int found=0;
    assert(id && id->kind==GUTC_JSON_STRING);
    assert(gutc_api_registry_at(i,&by_index)==GUTC_OK);
    assert(gutc_api_registry_lookup(id->as.string.bytes,id->as.string.length,
                                    &by_id,&found)==GUTC_OK);
    assert(found && by_id);
    assert(gutc_api_to_fod_node(by_index,&node)==GUTC_OK);
    assert(gutc_json_dom_get(node,"inputs",6)!=NULL);
    assert(gutc_json_dom_get(by_id,"inputSchema",11)!=NULL);
    gutc_json_dom_free(node);
    gutc_json_dom_free(by_index);
    gutc_json_dom_free(by_id);
  }
  {
    gutc_json_dom *missing=NULL;
    int found=1;
    assert(gutc_api_registry_lookup("no such API",11,&missing,&found)==GUTC_OK);
    assert(!found && missing==NULL);
    assert(gutc_api_registry_at(38,&missing)==GUTC_RANGE_ERROR);
  }
  assert(gutc_api_registry_fod_nodes(&fod)==GUTC_OK);
  assert(fod && fod->kind==GUTC_JSON_ARRAY && fod->as.array.count==38);
  gutc_json_dom_free(fod);
  gutc_json_dom_free(catalog);
  free(expected);free(actual);
  puts("GutC complete 38-definition pinned API catalog and native FOD registry passed");
  return 0;
}
