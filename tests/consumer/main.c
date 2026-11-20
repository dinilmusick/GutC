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
#include <gutc/semantic_deterministic.h>
#include <gutc/fod_node.h>
#include <gutc/api_registry.h>
#include <gutc/api_dispatch.h>
#include <gutc/model_registry.h>
#include <gutc/luth_origin.h>
#include <gutc/contract_primitives.h>
#include <gutc/semantic_rank.h>
#include <gutc/runtime_app_url.h>
#include <gutc/luth_callback_token.h>
#include <gutc/application_auth.h>
#include <gutc/semantic_cache.h>
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
  {gutc_json_dom *schema=NULL,*resolved=NULL;size_t at=0;int found=0;
    const unsigned char input[]="{\"type\":\"boolean\"}";
    if(gutc_json_dom_parse(input,sizeof(input)-1,8,100,&schema,&at)!=GUTC_OK)return 32;
    if(gutc_resolve_deterministic_semantic("true",4,schema,&resolved,&found)!=GUTC_OK)return 33;
    if(!found||!resolved)return 34;
    gutc_json_dom_free(schema);gutc_json_dom_free(resolved);}
  {gutc_json_dom *api=NULL,*node=NULL;size_t at=0;
    const unsigned char input[]="{\"id\":\"sample\",\"name\":\"sample\",\"category\":\"test\",\"description\":\"test\",\"parameters\":[],\"output\":{},\"semantics\":{}}";
    if(gutc_json_dom_parse(input,sizeof(input)-1,8,100,&api,&at)!=GUTC_OK)return 35;
    if(gutc_api_to_fod_node(api,&node)!=GUTC_OK)return 36;
    if(!gutc_json_dom_get(node,"inputs",6))return 37;
    gutc_json_dom_free(api);gutc_json_dom_free(node);}
  {gutc_json_dom *catalog=NULL,*api=NULL;int found=0;
    if(gutc_api_registry_count()!=38)return 38;
    if(gutc_api_registry_lookup("gutts.arrays.compact",20,&api,&found)!=GUTC_OK)return 39;
    if(!found||!api)return 40;
    if(gutc_api_registry_fod_nodes(&catalog)!=GUTC_OK)return 41;
    if(catalog->kind!=GUTC_JSON_ARRAY||catalog->as.array.count!=38)return 42;
    gutc_json_dom_free(catalog);gutc_json_dom_free(api);}
  {gutc_json_dom *args=NULL,*out=NULL;size_t at=0;int defined=0;
    const unsigned char input[]="{\"values\":[1,2,3]}";
    if(gutc_json_dom_parse(input,sizeof(input)-1,8,100,&args,&at)!=GUTC_OK)return 43;
    if(gutc_execute_api("gutts.numbers.sum",strlen("gutts.numbers.sum"),args,NULL,&out,&defined)!=GUTC_OK)return 44;
    if(!defined||!out||out->kind!=GUTC_JSON_NUMBER||out->as.number!=6)return 45;
    gutc_json_dom_free(args);gutc_json_dom_free(out);}
  {gutc_json_dom *models=NULL,*model=NULL;int found=0;
    if(gutc_model_registry_count()!=8)return 46;
    if(gutc_model_registry_lookup("model.cactus-needle3",strlen("model.cactus-needle3"),&model,&found)!=GUTC_OK)return 47;
    if(!found||!model)return 48;
    if(gutc_model_registry_list(&models)!=GUTC_OK)return 49;
    if(models->kind!=GUTC_JSON_ARRAY||models->as.array.count!=8)return 50;
    gutc_json_dom_free(model);gutc_json_dom_free(models);}
  {char origin[128];size_t needed=0;gutc_luth_environment env;
    if(gutc_luth_resolve_client_origin(" HTTPS://STAGING.EXAMPLE.ORG:443/callback ",NULL,
                                      origin,sizeof(origin),&needed)!=GUTC_OK)return 51;
    if(strcmp(origin,"https://staging.example.org"))return 52;
    if(gutc_luth_resolve_client_environment(origin,&env)!=GUTC_OK)return 53;
    if(env!=GUTC_LUTH_STAGING)return 54;}
  {gutc_json_dom *id=NULL;const char id_text[]="\"app_01\"";
    if(gutc_json_dom_parse((const uint8_t*)id_text,strlen(id_text),8,100,&id,NULL)!=GUTC_OK)return 55;
    if(!gutc_contract_id(id)||!gutc_contract_app_id(id)||gutc_app_contract_version()!=1u)return 56;
    gutc_json_dom_free(id);}
  {gutc_json_text words[]={{"blue",4},{"red",3}};double sim[]={0.9,0.2},rel[]={0.1,0.9};
    size_t order[2]={99,99},required=0;
    if(gutc_semantic_rank_order(words,sim,rel,2,order,2,&required)!=GUTC_OK)return 57;
    if(required!=2||order[0]!=1||order[1]!=0)return 58;}
  {char url[128];size_t needed=0;
    if(gutc_resolve_runtime_app_url("  https://example.org/path  ",url,sizeof url,&needed)!=GUTC_OK)return 59;
    if(strcmp(url,"https://example.org/path"))return 60;
    if(gutc_resolve_server_runtime_app_url("  https://example.org  ",url,sizeof url,&needed)!=GUTC_OK)return 61;
    if(strcmp(url,"https://example.org"))return 62;}
  {int hydrated=1;
    if(gutc_luth_hydrate_callback_token("/cb","",NULL,NULL,&hydrated)!=GUTC_OK)return 63;
    if(hydrated)return 64;}
  {gutc_application_auth_client client;const char *error=NULL;
    if(gutc_application_auth_init(&client,"/api/auth/oidc",NULL,&error)!=GUTC_OK)return 65;
    if(strcmp(client.base_path,"/api/auth/oidc"))return 66;
    if(gutc_application_auth_start_sign_in(&client,"/",NULL,&error)!=GUTC_UNSUPPORTED)return 67;
    if(!error||strcmp(error,"AUTH_NAVIGATION_UNAVAILABLE"))return 68;}
  {gutc_semantic_cache cache;const gutc_json_dom *borrowed=NULL;int found=0;
    gutc_json_dom *value=NULL;const char input[]="42";
    if(gutc_semantic_cache_init(&cache,2)!=GUTC_OK)return 69;
    if(gutc_json_dom_parse((const uint8_t*)input,sizeof input-1,8,100,&value,NULL)!=GUTC_OK)return 70;
    if(gutc_semantic_cache_set(&cache,"abc",3,value)!=GUTC_OK)return 71;
    if(gutc_semantic_cache_get(&cache,"abc",3,&borrowed,&found)!=GUTC_OK)return 72;
    if(!found||!borrowed||borrowed->kind!=GUTC_JSON_NUMBER||borrowed->as.number!=42)return 73;
    gutc_semantic_cache_destroy(&cache);}
  puts("GutC multi-module installed consumer passed");
  return 0;
}
