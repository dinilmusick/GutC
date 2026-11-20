#include "gutc/semantic_cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static gutc_json_dom *parse(const char *json){
  gutc_json_dom *out=NULL;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)json,strlen(json),64,1000,&out,NULL);
  assert(rc==GUTC_OK&&out);return out;
}
static void validate_result(const gutc_json_dom *actual,const char *want){
  char buffer[8192];
  size_t n=0;
  gutc_status rc;
  if(!strcmp(want,"_")){assert(actual==NULL);return;}
  assert(actual!=NULL);
  rc=gutc_json_dom_stringify(actual,buffer,sizeof(buffer),&n);
  assert(rc==GUTC_OK);
  if(strcmp(buffer,want))fprintf(stderr,"Cache mismatch: actual=%s wanted=%s\n",buffer,want);
  assert(!strcmp(buffer,want));
}
int main(void){
  FILE *f=fopen(GUTC_SEMANTIC_CACHE_GOLDEN_PATH,"r");
  char row[18000];size_t steps=0,gets=0;
  gutc_semantic_cache cache={NULL,0,0};
  assert(f);
  while(fgets(row,sizeof(row),f)){
    char *a,*b,*c,*d,*e,*end;
    gutc_json_dom *key,*value,*max;
    const gutc_json_dom *borrowed=NULL;
    const char *key_bytes=NULL;
    size_t key_len=0,expected_size=0;
    gutc_status rc;
    int found=0;
    if(row[0]=='#'||row[0]=='\n')continue;
    end=strchr(row,'\n');if(end)*end='\0';
    a=strchr(row,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    d=strchr(c,'\t');assert(d);*d++='\0';
    e=strchr(d,'\t');assert(e);*e++='\0';
    key=parse(a);value=parse(b);max=parse(c);
    if(key->kind==GUTC_JSON_STRING){
      key_bytes=key->as.string.bytes;key_len=key->as.string.length;
    }
    expected_size=(size_t)strtoul(e,NULL,10);
    if(!strcmp(row,"new")){
      assert(max->kind==GUTC_JSON_NUMBER&&max->as.number>=0&&
             max->as.number<=4096);
      gutc_semantic_cache_destroy(&cache);
      rc=gutc_semantic_cache_init(&cache,(size_t)max->as.number);
      assert(rc==GUTC_OK);
    }else if(!strcmp(row,"set")){
      assert(key->kind==GUTC_JSON_STRING);
      rc=gutc_semantic_cache_set(&cache,key_bytes,key_len,value);
      assert(rc==GUTC_OK);
      value=NULL; /* ownership transferred */
    }else if(!strcmp(row,"get")){
      assert(key->kind==GUTC_JSON_STRING);
      rc=gutc_semantic_cache_get(&cache,key_bytes,key_len,&borrowed,&found);
      assert(rc==GUTC_OK);
      assert(found==(strcmp(d,"_")!=0));
      validate_result(borrowed,d);gets++;
    }else if(!strcmp(row,"clear")){
      gutc_semantic_cache_clear(&cache);
    }else abort();
    assert(gutc_semantic_cache_size(&cache)==expected_size);
    gutc_json_dom_free(key);gutc_json_dom_free(value);gutc_json_dom_free(max);
    steps++;
  }
  fclose(f);assert(steps>=55&&gets>=20);
  {
    gutc_json_dom *value=parse("{\"status\":\"resolved\",\"candidates\":[],\"modelOutput\":123}");
    const gutc_json_dom *ptr=NULL;int found=0;size_t before;
    gutc_semantic_cache_destroy(&cache);
    assert(gutc_semantic_cache_init(&cache,1)==GUTC_OK);
    assert(gutc_semantic_cache_set(&cache,"only",4,value)==GUTC_OK);
    assert(gutc_semantic_cache_set(&cache,"second",6,value)==GUTC_INVALID_ARGUMENT);
    before=gutc_semantic_cache_size(&cache);
    assert(before==1);
    assert(gutc_semantic_cache_get(&cache,"only",4,&ptr,&found)==GUTC_OK);
    assert(found&&ptr==value);
    assert(gutc_semantic_cache_get(&cache,"missing",7,&ptr,&found)==GUTC_OK);
    assert(!found&&!ptr);
    assert(gutc_semantic_cache_get(&cache,"bad",3,NULL,&found)==GUTC_INVALID_ARGUMENT);
    gutc_semantic_cache_clear(&cache);
    assert(gutc_semantic_cache_size(&cache)==0);
    gutc_semantic_cache_destroy(&cache);
    gutc_semantic_cache_destroy(&cache);
  }
  puts("GutC source-pinned semantic input LRU cache tests passed");
  return 0;
}
