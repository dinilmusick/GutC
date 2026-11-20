#include <gutc/gutc.h>
#include <gutc/collections.h>
#include <gutc/safe_path.h>
#include <gutc/identity.h>
#include <gutc/json_dom.h>
#include <gutc/url_query.h>
#include <gutc_path.h>
#include <gutc_trusted_api.h>
#include <gut_string_predicates.h>
#include <gut_truncate_words.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  const char *parts[]={"users","44"};
  char result[256];
  size_t needed=0;
  if(gutc_join_url("https://example.test/",parts,2,result,sizeof(result),&needed)!=GUTC_OK) return 1;
  if(strcmp(result,"https://example.test/users/44")!=0) return 2;
  if(gutc_normalize_storage_path("/root/data//",result,sizeof(result))!=GUTC_PATH_OK) return 3;
  if(strcmp(result,"root/data")!=0) return 4;
  if(gutc_resolve_trusted_api_url("https://api.example.test","/v1/users",result,sizeof(result),&needed)!=0) return 5;
  if(strcmp(result,"https://api.example.test/v1/users")!=0) return 6;
  if(gut_is_blank_utf8("\xE3\x80\x80")!=1) return 7;
  if(gut_truncate_words_utf8_default("alpha beta gamma",2,result,sizeof(result),&needed)!=GUT_TRUNCATE_OK) return 8;
  if(strcmp(result,"alpha beta\xE2\x80\xA6")!=0) return 9;
  { gutc_value values[2]={{.kind=GUTC_VALUE_NUMBER,.as.number=1.0},{.kind=GUTC_VALUE_NUMBER,.as.number=2.0}};
    gutc_chunk_view view[2];
    if(gutc_array_chunks(values,2,1,view,2,&needed)!=GUTC_OK||needed!=2)return 10; }
  if(gutc_encode_safe_path_segments("dir/a b",result,sizeof(result),&needed)!=GUTC_OK)return 11;
  if(strcmp(result,"dir/a%20b")!=0)return 12;
  {gutc_identity_ref ref;
    gutc_string_view kind={"user",4},id={"user_1",6};
    if(gutc_identity_ref_parse(kind,id,&ref)!=GUTC_OK)return 13;}
  {gutc_json_dom *dom=NULL;size_t consumed=0;
    const unsigned char document[]="{\"ok\":true}";
    if(gutc_json_dom_parse(document,sizeof(document)-1,12,100,&dom,&consumed)!=GUTC_OK)return 14;
    if(!gutc_json_dom_get(dom,"ok",2))return 15;
    gutc_json_dom_free(dom);}
  {int found=0;char value[16];size_t count=0;
    if(gutc_url_get_query_param("https://example.com/?n=12","n",value,sizeof(value),&count,&found)!=GUTC_OK)return 16;
    if(!found || strcmp(value,"12")!=0)return 17;}
  puts("GutC multi-module installed consumer passed");
  return 0;
}
