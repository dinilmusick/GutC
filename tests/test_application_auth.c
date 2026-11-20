#include "gutc/application_auth.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct auth_mock {
  char navigation[25000];
  char last_resource[512];
  int navigation_calls,request_calls,status,after_logout,unexpected_request;
  const char *response_json;
} auth_mock;
static gutc_status navigate(void *ctx,const char *url){
  auth_mock *mock=(auth_mock*)ctx;
  size_t n=strlen(url);
  if(n>=sizeof(mock->navigation))return GUTC_RANGE_ERROR;
  memcpy(mock->navigation,url,n+1);
  mock->navigation_calls++;
  return GUTC_OK;
}
static gutc_status request(void *ctx,const char *verb,const char *resource,
                           const char *body,int same_origin,int no_store,
                           int *status,gutc_json_dom **response){
  auth_mock *m=(auth_mock*)ctx;
  size_t n=strlen(resource);
  if(n>=sizeof(m->last_resource))return GUTC_RANGE_ERROR;
  memcpy(m->last_resource,resource,n+1);
  m->request_calls++;
  if(!same_origin||!no_store||resource[0]!='/'||resource[1]=='/'||
      (strcmp(verb,"GET")&&strcmp(verb,"POST")))m->unexpected_request++;
  if(!strcmp(verb,"POST")){
    if(!body||strcmp(body,"{}")||!strstr(resource,"/logout"))m->unexpected_request++;
    m->after_logout=1;
    *status=m->status?m->status:200;*response=NULL;
    return GUTC_OK;
  }
  if(body||!strstr(resource,"/session"))m->unexpected_request++;
  if(m->after_logout){
    *status=401;*response=NULL;return GUTC_OK;
  }
  *status=m->status?m->status:200;
  if(m->response_json){
    gutc_status rc=gutc_json_dom_parse((const uint8_t*)m->response_json,
                   strlen(m->response_json),32,1000,response,NULL);
    if(rc!=GUTC_OK)return rc;
  }else *response=NULL;
  return GUTC_OK;
}
static gutc_json_dom *parse(const char *text){
  gutc_json_dom *v=NULL;
  assert(gutc_json_dom_parse((const uint8_t*)text,strlen(text),16,100,&v,NULL)==GUTC_OK);
  assert(v);return v;
}
static const char *as_string(const gutc_json_dom *v){
  if(v->kind==GUTC_JSON_NULL)return NULL;
  assert(v->kind==GUTC_JSON_STRING);
  assert(!memchr(v->as.string.bytes,0,v->as.string.length));
  return v->as.string.bytes;
}
static void check_path(const char *base_s,const char *ret_s,
                       const char *current_s,const char *expected_s,
                       const char *error_s){
  gutc_json_dom *base=parse(base_s),*ret=parse(ret_s),*current=parse(current_s),
                *expected=parse(expected_s),*expected_error=parse(error_s);
  auth_mock mock={{0},{0},0,0,0,0,0,NULL};
  gutc_app_auth_host host={&mock,request,navigate};
  gutc_application_auth_client client;
  const char *err=NULL,*want_error=as_string(expected_error);
  gutc_status rc=gutc_application_auth_init(&client,as_string(base),&host,&err);
  if(rc==GUTC_OK)rc=gutc_application_auth_start_sign_in(
    &client,as_string(ret),as_string(current),&err);
  if(want_error&&*want_error){
    if(!err||strcmp(err,want_error))
      fprintf(stderr,"Expected error %s, got %s from %s\n",
              want_error,err?err:"NULL",base_s);
    assert(err&&!strcmp(err,want_error));
    assert(rc!=GUTC_OK);
    assert(mock.navigation_calls==0);
  }else{
    if(rc!=GUTC_OK)fprintf(stderr,"unexpected auth status %d (%s)\n",rc,err?err:"NULL");
    assert(rc==GUTC_OK&&err==NULL);
    assert(mock.navigation_calls==1);
    if(strcmp(mock.navigation,as_string(expected)))
      fprintf(stderr,"navigation mismatch: %s expected %s\n",
              mock.navigation,as_string(expected));
    assert(!strcmp(mock.navigation,as_string(expected)));
  }
  gutc_json_dom_free(base);gutc_json_dom_free(ret);
  gutc_json_dom_free(current);gutc_json_dom_free(expected);
  gutc_json_dom_free(expected_error);
}
static void check_session(void){
  auth_mock mock={{0},{0},0,0,0,0,0,
    "{\"authenticated\":true,\"user\":{\"id\":\"u1\",\"email\":\"a@example.com\"},\"session\":{\"id\":\"s1\"}}"};
  gutc_app_auth_host host={&mock,request,navigate};
  gutc_application_auth_client client;
  gutc_json_dom *result=NULL;
  const char *error=NULL;
  assert(gutc_application_auth_init(&client,"/api/auth/oidc",&host,&error)==GUTC_OK);
  assert(gutc_application_auth_get_session(&client,&result,&error)==GUTC_OK);
  assert(result && result->kind==GUTC_JSON_OBJECT);
  assert(!strcmp(mock.last_resource,"/api/auth/oidc/session"));
  assert(mock.unexpected_request==0);
  gutc_json_dom_free(result);result=NULL;
  mock.response_json="{\"authenticated\":true,\"user\":{\"id\":\"\",\"email\":\"e\"},\"session\":{\"id\":\"s\"}}";
  assert(gutc_application_auth_get_session(&client,&result,&error)==GUTC_INVALID_FORMAT);
  assert(error&&!strcmp(error,"AUTH_SESSION_INVALID")&&result==NULL);
  mock.response_json=NULL;mock.status=401;
  assert(gutc_application_auth_get_session(&client,&result,&error)==GUTC_OK);
  assert(result && result->kind==GUTC_JSON_OBJECT);
  {
    const gutc_json_dom *v=gutc_json_dom_get(result,"authenticated",13);
    assert(v&&v->kind==GUTC_JSON_BOOLEAN&&!v->as.boolean);
  }
  gutc_json_dom_free(result);result=NULL;
  mock.status=503;
  assert(gutc_application_auth_get_session(&client,&result,&error)==GUTC_INVALID_FORMAT);
  assert(error&&!strcmp(error,"AUTH_SESSION_UNAVAILABLE"));
  mock.status=0;mock.response_json=
    "{\"authenticated\":true,\"user\":{\"id\":\"u1\",\"email\":\"a@example.com\"},\"session\":{\"id\":\"s1\"}}";
  mock.after_logout=0;
  assert(gutc_application_auth_sign_out(&client,&error)==GUTC_OK);
  assert(mock.after_logout && mock.unexpected_request==0);
  mock.status=500;mock.after_logout=0;
  assert(gutc_application_auth_sign_out(&client,&error)==GUTC_INVALID_FORMAT);
  assert(error&&!strcmp(error,"AUTH_LOGOUT_REJECTED"));
  {gutc_application_auth_client without_host;
   assert(gutc_application_auth_init(&without_host,NULL,NULL,&error)==GUTC_OK);
   assert(gutc_application_auth_get_session(&without_host,&result,&error)==GUTC_UNSUPPORTED);
   assert(error&&!strcmp(error,"AUTH_SESSION_UNAVAILABLE")&&result==NULL);
   assert(gutc_application_auth_start_sign_in(&without_host,"/",NULL,&error)==GUTC_UNSUPPORTED);
  }
}
int main(void){
  FILE *f=fopen(GUTC_APP_AUTH_GOLDEN_PATH,"r");
  char row[32000];size_t cases=0;
  assert(f);
  while(fgets(row,sizeof row,f)){
    char *a,*b,*c,*d,*end;
    if(row[0]=='#'||row[0]=='\n')continue;
    end=strchr(row,'\n');if(end)*end='\0';
    a=strchr(row,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    d=strchr(c,'\t');assert(d);*d++='\0';
    check_path(row,a,b,c,d);cases++;
  }
  fclose(f);assert(cases>=35);
  check_session();
  puts("GutC application-auth navigation and host session validation passed");
  return 0;
}
