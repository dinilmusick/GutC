#include "gutc/json_dom.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static gutc_json_dom *parse(const char *s) {
  gutc_json_dom *v=NULL;
  size_t at=0;
  gutc_status rc=gutc_json_dom_parse((const unsigned char*)s,strlen(s),32,1024,&v,&at);
  if(rc!=GUTC_OK)fprintf(stderr,"DOM parse failed: %s at %zu for %s\n",gutc_status_name(rc),at,s);
  assert(rc==GUTC_OK && at==strlen(s) && v!=NULL);
  return v;
}
static void roundtrip(const char *source,const char *expected) {
  char output[1024]="unchanged";
  gutc_json_dom *v=parse(source);
  size_t need=0;
  assert(gutc_json_dom_stringify(v,NULL,0,&need)==GUTC_OK);
  assert(need==strlen(expected));
  assert(gutc_json_dom_stringify(v,output,4,&need)==GUTC_NO_SPACE);
  assert(strcmp(output,"unchanged")==0);
  assert(gutc_json_dom_stringify(v,output,sizeof(output),&need)==GUTC_OK);
  if(strcmp(output,expected)!=0)
    fprintf(stderr,"Wanted %s, got %s\n",expected,output);
  assert(strcmp(output,expected)==0);
  assert(gutc_json_dom_is_json_value(v));
  gutc_json_dom_free(v);
}
int main(void) {
  gutc_json_dom *v=NULL;
  gutc_json_dom *arr;
  const gutc_json_dom *field;
  size_t offset=0;
  roundtrip("null","null");
  roundtrip("true","true");
  roundtrip("[1,2,3]","[1,2,3]");
  roundtrip("{\"ok\":[1,\"x\",null]}","{\"ok\":[1,\"x\",null]}");
  roundtrip("{\"a\":1,\"a\":2}","{\"a\":2}");
  roundtrip("{\"a\":-0,\"b\":1.5}","{\"a\":0,\"b\":1.5}");
  roundtrip("\"\\uD83C\\uDF0D\"","\"\xF0\x9F\x8C\x8D\"");
  roundtrip("\"slash\\/newline\\n\"","\"slash/newline\\n\"");
  roundtrip("\"\\u0000\"","\"\\u0000\"");
  assert(gutc_json_dom_parse((const unsigned char*)"\"\\uD800\"",8,32,1024,&v,&offset)
         ==GUTC_INVALID_ENCODING && v==NULL);
  assert(gutc_json_dom_parse((const unsigned char*)"[1,]",4,32,1024,&v,&offset)
         ==GUTC_INVALID_FORMAT && v==NULL);
  assert(gutc_json_dom_parse((const unsigned char*)"[[1]]",5,1,1024,&v,&offset)
         ==GUTC_RANGE_ERROR && v==NULL);
  assert(gutc_json_dom_parse((const unsigned char*)"[1,2]",5,32,2,&v,&offset)
         ==GUTC_RANGE_ERROR && v==NULL);
  v=parse("{\"a\":true,\"array\":[1,2]}");
  field=gutc_json_dom_get(v,"a",1);
  assert(field && field->kind==GUTC_JSON_BOOLEAN && field->as.boolean);
  field=gutc_json_dom_get(v,"array",5);
  assert(field && field->kind==GUTC_JSON_ARRAY && field->as.array.count==2);
  assert(gutc_json_dom_get(v,"missing",7)==NULL);
  gutc_json_dom_free(v);
  arr=parse("[1e999]");
  assert(arr->as.array.count==1 && !gutc_json_dom_is_json_value(arr));
  {char output[40];size_t needed=0;
   assert(gutc_json_dom_stringify(arr,output,sizeof output,&needed)==GUTC_OK);
   assert(strcmp(output,"[null]")==0);}
  gutc_json_dom_free(arr);
  puts("GutC owned JSON DOM tests passed");
  return 0;
}
