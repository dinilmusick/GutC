#include <gutc/gutc.h>
#include <gutc/collections.h>
#include <gutc/safe_path.h>
#include <gutc/identity.h>
#include <gutc/json_dom.h>
#include <gutc/url_query.h>
#include <gutc/auth_contracts.h>
#include <gutc/oauth_contracts.h>
#include <gutc/string_case.h>
#include <gutc/object_path.h>
#include <gutc/api_definition.h>
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
  {gutc_json_dom *contract=NULL;size_t at=0;
    const unsigned char invalid[]="{\"schemaVersion\":1}";
    const char *reason=NULL;
    if(gutc_json_dom_parse(invalid,sizeof(invalid)-1,8,32,&contract,&at)!=GUTC_OK)return 18;
    if(gutc_validate_scoped_invocation_v1(contract,&reason)!=GUTC_INVALID_FORMAT)return 19;
    if(!reason||strcmp(reason,"INVALID_INVOCATION_IDENTITY_OR_AUDIENCE")!=0)return 20;
    gutc_json_dom_free(contract);}
  {gutc_json_dom *registration=NULL;size_t at=0;
    const unsigned char bad[]="{\"schemaVersion\":1}";
    const char *reason=NULL;
    if(gutc_json_dom_parse(bad,sizeof(bad)-1,8,32,&registration,&at)!=GUTC_OK)return 21;
    if(gutc_validate_oauth_registration_v1(registration,&reason)!=GUTC_INVALID_FORMAT)return 22;
    if(!reason||strcmp(reason,"INVALID_CLIENT_IDENTITY_OR_PKCE")!=0)return 23;
    gutc_json_dom_free(registration);}
  {char transformed[32];size_t length=0;
    if(gutc_string_transform_ascii("hello world",11,GUTC_CASE_CAMEL,
      transformed,sizeof transformed,&length)!=GUTC_OK)return 24;
    if(strcmp(transformed,"helloWorld")!=0)return 25;}
  {gutc_json_dom *document=NULL,*picked=NULL;size_t at=0;
    const unsigned char input[]="{\"profile\":{\"name\":\"Ada\"}}";
    const gutc_json_text keys[]={{"profile.name",12}};
    if(gutc_json_dom_parse(input,sizeof(input)-1,8,100,&document,&at)!=GUTC_OK)return 26;
    if(gutc_json_pick_paths(document,keys,1,&picked)!=GUTC_OK)return 27;
    if(!gutc_json_dom_get(picked,"profile",7))return 28;
    gutc_json_dom_free(document);gutc_json_dom_free(picked);}
  {gutc_json_dom *api=NULL,*defined=NULL;size_t at=0;
    const unsigned char input[]="{\"name\":\"echo\",\"parameters\":[]}";
    if(gutc_json_dom_parse(input,sizeof(input)-1,8,100,&api,&at)!=GUTC_OK)return 29;
    if(gutc_define_api(api,&defined)!=GUTC_OK)return 30;
    if(!gutc_json_dom_get(defined,"inputSchema",11))return 31;
    gutc_json_dom_free(api);gutc_json_dom_free(defined);}
  puts("GutC multi-module installed consumer passed");
  return 0;
}
