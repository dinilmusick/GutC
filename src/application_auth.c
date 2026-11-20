#include "gutc/application_auth.h"
#include "gutc/gutc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AUTH_MAX_URL 8192u
static gutc_status invalid(const char **error,const char *code) {
  if(error)*error=code;
  return GUTC_INVALID_FORMAT;
}
static gutc_status unsupported(const char **error,const char *code) {
  if(error)*error=code;
  return GUTC_UNSUPPORTED;
}
static int size_bounded(const char *str,size_t *n){
  size_t i;
  if(!str)return 0;
  for(i=0;i<AUTH_MAX_URL;i++)if(!str[i]){*n=i;return 1;}
  return 0;
}
static int base_char(unsigned char c){
  return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||
         (c>='0'&&c<='9')||c=='/'||c=='_'||c=='-';
}
static int valid_base_path(const char *path,size_t n){
  size_t i;
  if(n<2||path[0]!='/')return 0;
  for(i=1;i<n;i++)if(!base_char((unsigned char)path[i]))return 0;
  for(i=1;i<n;i++)if(path[i]=='/'&&path[i-1]=='/')return 0;
  return 1;
}
static int valid_return_path(const char *path,size_t n){
  size_t i,points=0;
  if(!n||path[0]!='/'||(n>=2&&path[1]=='/'))return 0;
  for(i=0;i<n;i++){
    unsigned char c=(unsigned char)path[i];
    if(c=='\\'||c<32u||c==127u)return 0;
  }
  return gutc_utf8_validate((const uint8_t*)path,n,&points)==GUTC_OK;
}
static int encode_char(unsigned char c){
  return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||
      (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='!'||
      c=='~'||c=='*'||c==39||c=='('||c==')';
}
static const gutc_json_dom *get(const gutc_json_dom *root,const char *name){
  return gutc_json_dom_get(root,name,strlen(name));
}
static int truthy_string(const gutc_json_dom *root,const char *key){
  const gutc_json_dom *value=get(root,key);
  return value&&value->kind==GUTC_JSON_STRING&&
    value->as.string.bytes&&value->as.string.length>0;
}
gutc_status gutc_application_auth_init(gutc_application_auth_client *client,
  const char *config_base_path,const gutc_app_auth_host *host,const char **error) {
  const char *path=config_base_path;
  size_t n;
  if(error)*error=NULL;
  if(!client)return GUTC_INVALID_ARGUMENT;
  memset(client,0,sizeof(*client));
  if(!path||!*path)path="/api/auth/oidc";
  if(!size_bounded(path,&n)||n>=sizeof(client->base_path)||
      !valid_base_path(path,n))
    return invalid(error,"INVALID_APPLICATION_AUTH_PATH");
  if(n>2&&path[n-1]=='/')n--;
  memcpy(client->base_path,path,n);
  client->base_path[n]='\0';
  if(host)client->host=*host;
  return GUTC_OK;
}
gutc_status gutc_application_auth_start_sign_in(
  const gutc_application_auth_client *client,const char *return_to,
  const char *current_host_path,const char **error) {
  char url[AUTH_MAX_URL*3+320];
  const char *next=return_to;
  size_t i,n=0,pos=0,base_n;
  static const char *hex="0123456789ABCDEF";
  gutc_status rc;
  if(error)*error=NULL;
  if(!client)return GUTC_INVALID_ARGUMENT;
  if(!client->host.navigate)return unsupported(error,"AUTH_NAVIGATION_UNAVAILABLE");
  if(!next||!*next)next=current_host_path;
  if(!next||!*next)next="/";
  if(!size_bounded(next,&n)||!valid_return_path(next,n))
    return invalid(error,"INVALID_AUTH_RETURN_PATH");
  base_n=strlen(client->base_path);
  if(base_n<2||base_n>=256)return GUTC_INVALID_ARGUMENT;
  memcpy(url,client->base_path,base_n);pos=base_n;
  memcpy(url+pos,"/start?next=",12);pos+=12;
  for(i=0;i<n;i++){
    unsigned char c=(unsigned char)next[i];
    if(pos>sizeof(url)-4)return GUTC_OVERFLOW;
    if(encode_char(c))url[pos++]=(char)c;
    else {
      url[pos++]='%';
      url[pos++]=hex[c>>4];
      url[pos++]=hex[c&15u];
    }
  }
  url[pos]='\0';
  rc=client->host.navigate(client->host.context,url);
  if(rc!=GUTC_OK&&error)*error="AUTH_NAVIGATION_UNAVAILABLE";
  return rc;
}
static gutc_status request(
 const gutc_application_auth_client *client,const char *verb,
 const char *suffix,const char *body,
 int *status,gutc_json_dom **response,const char **error) {
  char url[320];
  size_t prefix,suffix_len;
  gutc_status rc;
  if(!client||!status||!response)return GUTC_INVALID_ARGUMENT;
  *response=NULL;*status=0;
  if(!client->host.request)return unsupported(error,"AUTH_SESSION_UNAVAILABLE");
  prefix=strlen(client->base_path);
  if(prefix<2||prefix>=256)return GUTC_INVALID_ARGUMENT;
  memcpy(url,client->base_path,prefix);
  suffix_len=strlen(suffix);
  if(suffix_len>sizeof(url)-prefix-1)return GUTC_RANGE_ERROR;
  memcpy(url+prefix,suffix,suffix_len+1);
  rc=client->host.request(client->host.context,verb,url,body,1,1,status,response);
  if(rc!=GUTC_OK&&error)
    *error=!strcmp(suffix,"/logout")?"AUTH_LOGOUT_UNAVAILABLE":"AUTH_SESSION_UNAVAILABLE";
  return rc;
}
static gutc_status unauthenticated(gutc_json_dom **out){
  static const char value[]="{\"authenticated\":false,\"error\":\"SESSION_INVALID\"}";
  return gutc_json_dom_parse((const uint8_t*)value,sizeof(value)-1,16,64,out,NULL);
}
gutc_status gutc_application_auth_get_session(
 const gutc_application_auth_client *client,
 gutc_json_dom **session_result,const char **error) {
  gutc_json_dom *response=NULL;
  const gutc_json_dom *user,*session,*authenticated;
  gutc_status rc;
  int status=0;
  if(error)*error=NULL;
  if(!session_result||!client)return GUTC_INVALID_ARGUMENT;
  *session_result=NULL;
  rc=request(client,"GET","/session",NULL,&status,&response,error);
  if(rc!=GUTC_OK){gutc_json_dom_free(response);return rc;}
  if(status==401){
    gutc_json_dom_free(response);
    return unauthenticated(session_result);
  }
  if(status<200||status>=300){
    gutc_json_dom_free(response);
    return invalid(error,"AUTH_SESSION_UNAVAILABLE");
  }
  if(!response||response->kind!=GUTC_JSON_OBJECT){
    gutc_json_dom_free(response);
    return invalid(error,"AUTH_SESSION_INVALID");
  }
  authenticated=get(response,"authenticated");
  user=get(response,"user");
  session=get(response,"session");
  if(!authenticated||authenticated->kind!=GUTC_JSON_BOOLEAN||
     !authenticated->as.boolean||
     !user||user->kind!=GUTC_JSON_OBJECT||
     !session||session->kind!=GUTC_JSON_OBJECT||
     !truthy_string(user,"id")||!truthy_string(user,"email")||
     !truthy_string(session,"id")){
    gutc_json_dom_free(response);
    return invalid(error,"AUTH_SESSION_INVALID");
  }
  *session_result=response;
  return GUTC_OK;
}
gutc_status gutc_application_auth_sign_out(
  const gutc_application_auth_client *client,const char **error) {
  gutc_json_dom *response=NULL,*session_result=NULL;
  const gutc_json_dom *auth;
  int status=0;
  gutc_status rc;
  if(error)*error=NULL;
  if(!client)return GUTC_INVALID_ARGUMENT;
  rc=request(client,"POST","/logout","{}",&status,&response,error);
  gutc_json_dom_free(response);
  if(rc!=GUTC_OK)return rc;
  if(status<200||status>=300)
    return invalid(error,"AUTH_LOGOUT_REJECTED");
  rc=gutc_application_auth_get_session(client,&session_result,error);
  if(rc!=GUTC_OK){gutc_json_dom_free(session_result);return rc;}
  auth=get(session_result,"authenticated");
  if(auth&&auth->kind==GUTC_JSON_BOOLEAN&&auth->as.boolean){
    gutc_json_dom_free(session_result);
    return invalid(error,"AUTH_LOGOUT_UNVERIFIED");
  }
  gutc_json_dom_free(session_result);
  return GUTC_OK;
}
