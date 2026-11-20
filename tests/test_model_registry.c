#include "gutc/model_registry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int same(const gutc_json_dom *a,const gutc_json_dom *b,unsigned depth) {
  size_t i;
  if(!a||!b||depth>128||a->kind!=b->kind)return 0;
  switch(a->kind) {
    case GUTC_JSON_NULL:return 1;
    case GUTC_JSON_BOOLEAN:return a->as.boolean==b->as.boolean;
    case GUTC_JSON_NUMBER:return a->as.number==b->as.number;
    case GUTC_JSON_STRING:
      return a->as.string.length==b->as.string.length&&
        (!a->as.string.length||!memcmp(a->as.string.bytes,b->as.string.bytes,a->as.string.length));
    case GUTC_JSON_ARRAY:
      if(a->as.array.count!=b->as.array.count)return 0;
      for(i=0;i<a->as.array.count;i++)
        if(!same(a->as.array.values[i],b->as.array.values[i],depth+1))return 0;
      return 1;
    case GUTC_JSON_OBJECT:
      if(a->as.object.count!=b->as.object.count)return 0;
      for(i=0;i<a->as.object.count;i++) {
        const gutc_json_member x=a->as.object.members[i],y=b->as.object.members[i];
        if(x.key.length!=y.key.length||
           (x.key.length&&memcmp(x.key.bytes,y.key.bytes,x.key.length)!=0)||
           !same(x.value,y.value,depth+1))return 0;
      }
      return 1;
    default:return 0;
  }
}
int main(void) {
  FILE *f=fopen(GUTC_MODEL_CATALOG_PATH,"rb");
  long bytes;
  char *text;
  gutc_json_dom *wanted=NULL,*catalog=NULL,*other=NULL;
  gutc_status rc;
  size_t i;
  assert(f && gutc_model_registry_count()==8);
  assert(fseek(f,0,SEEK_END)==0);
  bytes=ftell(f);assert(bytes>0&&bytes<1048576L);
  rewind(f);
  text=(char*)malloc((size_t)bytes+1);
  assert(text);
  assert(fread(text,1,(size_t)bytes,f)==(size_t)bytes);
  fclose(f);text[bytes]='\0';
  rc=gutc_json_dom_parse((const uint8_t*)text,(size_t)bytes,128,100000,&wanted,NULL);
  assert(rc==GUTC_OK);
  rc=gutc_model_registry_list(&catalog);
  assert(rc==GUTC_OK&&catalog&&catalog->kind==GUTC_JSON_ARRAY);
  assert(same(wanted,catalog,0));
  for(i=0;i<8;i++) {
    gutc_json_dom *by_index=NULL;
    const gutc_json_dom *name=gutc_json_dom_get(catalog->as.array.values[i],"id",2);
    int found=0;
    assert(name && name->kind==GUTC_JSON_STRING);
    assert(gutc_model_registry_at(i,&by_index)==GUTC_OK);
    assert(gutc_model_registry_lookup(name->as.string.bytes,name->as.string.length,
                                      &other,&found)==GUTC_OK);
    assert(found && other && same(by_index,other,0));
    gutc_json_dom_free(by_index);gutc_json_dom_free(other);other=NULL;
  }
  {int found=1;
   assert(gutc_model_registry_lookup("unknown",7,&other,&found)==GUTC_OK);
   assert(!found&&other==NULL);
   assert(gutc_model_registry_at(8,&other)==GUTC_RANGE_ERROR);}
  gutc_json_dom_free(wanted);gutc_json_dom_free(catalog);free(text);
  puts("GutC eight pinned model metadata definitions passed");
  return 0;
}
