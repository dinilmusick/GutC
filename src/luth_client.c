#include "gutc/luth_client.h"
#include "gutc/gutc.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define LUTH_MAX_TOKEN 4096u
#define LUTH_MAX_BODY 16384u

static void wipe(void *p,size_t n){
  volatile unsigned char *b=(volatile unsigned char*)p;
  while(n--)*b++=0;
}
static int copy_text(char *dst,size_t cap,gutc_auth_text src){
  size_t cp=0;
  if((!src.bytes&&src.length)||src.length>=cap||
     (src.length&&memchr(src.bytes,0,src.length)))return 0;
  if(gutc_utf8_validate((const uint8_t*)(src.length?src.bytes:""),
                          src.length,&cp)!=GUTC_OK)return 0;
  if(src.length)memcpy(dst,src.bytes,src.length);
  dst[src.length]='\0';return 1;
}
static int has_text(const gutc_auth_text t){
  return t.bytes&&t.length>0;
}
static unsigned bounded_option(gutc_auth_optional_number value,unsigned fallback,
                               unsigned limit,int *valid){
  if(!value.present)return fallback;
  if(!isfinite(value.value)||value.value<0||value.value>limit||
     floor(value.value)!=value.value){*valid=0;return fallback;}
  return (unsigned)value.value;
}
static int ascii_equal_ci(const char *a,const char *b,size_t n){
  size_t i;
  for(i=0;i<n;i++){
    unsigned char x=(unsigned char)a[i],y=(unsigned char)b[i];
    if(x>='A'&&x<='Z')x=(unsigned char)(x+('a'-'A'));
    if(y>='A'&&y<='Z')y=(unsigned char)(y+('a'-'A'));
    if(x!=y)return 0;
  }
  return 1;
}
gutc_status gutc_luth_client_add_url(gutc_luth_client *c,
 const char *url,size_t length){
  char input[GUTC_LUTH_MAX_BASE_URL],norm[GUTC_LUTH_MAX_BASE_URL];
  size_t n=0,i;
  gutc_status rc;
  if(!c||(!url&&length)||!length||length>=sizeof(input))
    return GUTC_INVALID_ARGUMENT;
  if(memchr(url,0,length))return GUTC_INVALID_FORMAT;
  memcpy(input,url,length);input[length]='\0';
  while(length&&input[length-1]=='/')input[--length]='\0';
  if(!length)return GUTC_INVALID_FORMAT;
  rc=gutc_luth_normalize_origin(input,norm,sizeof(norm),&n);
  if(rc!=GUTC_OK)return rc;
  if(!n||n!=length||!ascii_equal_ci(input,norm,n))
    return GUTC_UNSUPPORTED; /* no path, query, credentials or opaque origin */
  if(strncmp(norm,"https://",8)!=0){
    const char *local[]={"http://localhost","http://127.0.0.1","http://[::1]"};
    int localhost_allowed=0;
    for(i=0;i<3;i++){
      size_t prefix=strlen(local[i]);
      if(!strncmp(norm,local[i],prefix) &&
         (norm[prefix]=='\0'||norm[prefix]==':'))localhost_allowed=1;
    }
    if(!localhost_allowed)return GUTC_UNSUPPORTED;
  }
  for(i=0;i<c->url_count;i++)
    if(!strcmp(c->urls[i],norm))return GUTC_OK;
  if(c->url_count>=GUTC_LUTH_MAX_ENDPOINTS)return GUTC_NO_SPACE;
  memcpy(c->urls[c->url_count],norm,n+1);
  c->url_count++;
  return GUTC_OK;
}
gutc_status gutc_luth_client_init(gutc_luth_client *c,
 const gutc_luth_auth_client_config *cfg,const char *host_runtime_origin,
 const gutc_luth_http_host *host){
  char explicit_origin[GUTC_LUTH_MAX_BASE_URL];
  const char *explicit_text=NULL;
  size_t needed=0,i;
  gutc_status rc;
  int valid=1;
  if(!c||!cfg)return GUTC_INVALID_ARGUMENT;
  memset(c,0,sizeof(*c));
  if(!has_text(cfg->ecosystem_id)||!has_text(cfg->application_id)||
     !copy_text(c->ecosystem_id,sizeof(c->ecosystem_id),cfg->ecosystem_id)||
     !copy_text(c->application_id,sizeof(c->application_id),cfg->application_id))
    return GUTC_INVALID_ARGUMENT;
  if(cfg->origin.present){
    if(!copy_text(explicit_origin,sizeof(explicit_origin),cfg->origin.value))
      return GUTC_INVALID_ARGUMENT;
    explicit_text=explicit_origin;
  }
  rc=gutc_luth_resolve_client_origin(explicit_text,host_runtime_origin,
      c->origin,sizeof(c->origin),&needed);
  if(rc!=GUTC_OK)return rc;
  if(!needed)return GUTC_INVALID_ARGUMENT;
  if(cfg->has_environment)c->environment=cfg->environment;
  else {
    rc=gutc_luth_resolve_client_environment(c->origin,&c->environment);
    if(rc!=GUTC_OK)return rc;
  }
  if((unsigned)c->environment>GUTC_LUTH_PRODUCTION)return GUTC_INVALID_ARGUMENT;
  c->max_retries=bounded_option(cfg->max_retries,3,8,&valid);
  c->retry_delay_ms=bounded_option(cfg->retry_delay_ms,100,30000,&valid);
  c->timeout_ms=bounded_option(cfg->timeout_ms,10000,600000,&valid);
  if(!valid)return GUTC_INVALID_ARGUMENT;
  c->auto_register=cfg->auto_register.present?!!cfg->auto_register.value:1;
  if(cfg->token_store)c->token_store=*cfg->token_store;
  if(host)c->http=*host;
  if(cfg->luth_urls.count){
    if(!cfg->luth_urls.items||cfg->luth_urls.count>GUTC_LUTH_MAX_ENDPOINTS)
      return GUTC_INVALID_ARGUMENT;
    for(i=0;i<cfg->luth_urls.count;i++){
      rc=gutc_luth_client_add_url(c,cfg->luth_urls.items[i].bytes,
                                   cfg->luth_urls.items[i].length);
      if(rc!=GUTC_OK)return rc;
    }
  }else if(cfg->luth_url.present){
    rc=gutc_luth_client_add_url(c,cfg->luth_url.value.bytes,
                                 cfg->luth_url.value.length);
    if(rc!=GUTC_OK)return rc;
  }else {
    const char *url=gutc_luth_default_url();
    rc=gutc_luth_client_add_url(c,url,strlen(url));
    if(rc!=GUTC_OK)return rc;
  }
  return GUTC_OK;
}
void gutc_luth_client_destroy(gutc_luth_client *c){
 if(c)wipe(c,sizeof(*c));
}
gutc_status gutc_luth_client_clear_token(gutc_luth_client *c){
 if(!c)return GUTC_INVALID_ARGUMENT;
 return c->token_store.clear?
    c->token_store.clear(c->token_store.context):GUTC_OK;
}
static int push(char *out,size_t *pos,const char *s,size_t len){
 if(*pos>=LUTH_MAX_BODY||len>=LUTH_MAX_BODY-*pos)return 0;
 memcpy(out+*pos,s,len);*pos+=len;out[*pos]='\0';return 1;
}
static int quoted(char *out,size_t *pos,gutc_auth_text t){
 static const char hex[]="0123456789ABCDEF";
 size_t i,cp=0;
 if((!t.bytes&&t.length)||t.length>LUTH_MAX_BODY||
    gutc_utf8_validate((const uint8_t*)(t.length?t.bytes:""),t.length,&cp)!=GUTC_OK)
    return 0;
 if(!push(out,pos,"\"",1))return 0;
 for(i=0;i<t.length;i++){
  unsigned char ch=(unsigned char)t.bytes[i];
  char esc[6];
  if(ch=='"'||ch=='\\'){
   esc[0]='\\';esc[1]=(char)ch;
   if(!push(out,pos,esc,2))return 0;
  }else if(ch<32){
   esc[0]='\\';esc[1]='u';esc[2]='0';esc[3]='0';
   esc[4]=hex[ch>>4];esc[5]=hex[ch&15u];
   if(!push(out,pos,esc,6))return 0;
  }else if(!push(out,pos,(const char*)&t.bytes[i],1))return 0;
 }
 return push(out,pos,"\"",1);
}
static gutc_auth_text view(const char *s){gutc_auth_text t={s,strlen(s)};return t;}
static int field(char *buffer,size_t *at,const char *key,gutc_auth_text value,int comma){
 return (!comma||push(buffer,at,",",1)) &&
   quoted(buffer,at,view(key)) && push(buffer,at,":",1) &&
   quoted(buffer,at,value);
}
static const gutc_json_dom *prop(const gutc_json_dom *body,const char *key){
 return gutc_json_dom_get(body,key,strlen(key));
}
static int string_present(const gutc_json_dom *o,const char *key){
 const gutc_json_dom *v=prop(o,key);
 return v&&v->kind==GUTC_JSON_STRING&&v->as.string.length>0;
}
static void release(gutc_luth_http_response *r){
 if(!r)return;
 gutc_json_dom_free(r->body);r->body=NULL;
}
static gutc_status token_update(gutc_luth_client *c,gutc_luth_http_response *r){
 const gutc_json_dom *value,*session;
 if(r->auth_token.bytes&&r->auth_token.length){
  if(!c->token_store.set)return GUTC_UNSUPPORTED;
  return c->token_store.set(c->token_store.context,
     r->auth_token.bytes,r->auth_token.length);
 }
 if(!r->body||r->body->kind!=GUTC_JSON_OBJECT)return GUTC_OK;
 value=prop(r->body,"token");
 session=prop(r->body,"session");
 if(!value&&session&&session->kind==GUTC_JSON_OBJECT)
  value=prop(session,"token");
 if(value&&value->kind==GUTC_JSON_STRING&&value->as.string.length){
  if(!c->token_store.set)return GUTC_UNSUPPORTED;
  return c->token_store.set(c->token_store.context,
     value->as.string.bytes,value->as.string.length);
 }
 return GUTC_OK;
}
static gutc_status send(gutc_luth_client *c,const char *method,
 const char *path,const char *body,gutc_luth_http_response *out){
 char token[LUTH_MAX_TOKEN]={0};
 size_t token_length=0,cycle,offset,start,index;
 gutc_status rc=GUTC_UNSUPPORTED,last=GUTC_UNSUPPORTED;
 if(!c||!out||!method||!path)return GUTC_INVALID_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(!c->http.request||!c->url_count)return GUTC_UNSUPPORTED;
 if(c->token_store.get){
  rc=c->token_store.get(c->token_store.context,token,sizeof(token),&token_length);
  if(rc!=GUTC_OK){wipe(token,sizeof(token));return rc;}
  if(token_length>sizeof(token)){wipe(token,sizeof(token));return GUTC_OVERFLOW;}
 }
 start=c->url_count>1?c->next_index%c->url_count:0;
 for(cycle=0;cycle<=(size_t)c->max_retries;cycle++){
  for(offset=0;offset<c->url_count;offset++){
   index=(start+offset)%c->url_count;
   memset(out,0,sizeof(*out));
   rc=c->http.request(c->http.context,c->urls[index],method,path,body,
      token_length?token:NULL,token_length,c->timeout_ms,1,1,out);
   if(rc!=GUTC_OK){last=rc;release(out);continue;}
   if(out->status==502||out->status==503||out->status==504){
    last=GUTC_UNSUPPORTED;release(out);continue;
   }
   c->active_index=index;
   c->next_index=offset==0?(index+1)%c->url_count:index;
   wipe(token,sizeof(token));
   return GUTC_OK;
  }
  if(cycle<c->max_retries && c->http.sleep){
   size_t factor=(size_t)1u<<cycle;
   size_t delay=(size_t)c->retry_delay_ms*factor;
   if(delay>60000u)delay=60000u;
   rc=c->http.sleep(c->http.context,(unsigned)delay);
   if(rc!=GUTC_OK){wipe(token,sizeof(token));return rc;}
  }
 }
 wipe(token,sizeof(token));
 return last;
}
static gutc_status transfer(gutc_luth_http_response *r,gutc_json_dom **output){
 if(!output||!r)return GUTC_INVALID_ARGUMENT;
 *output=r->body;r->body=NULL;
 return GUTC_OK;
}
static gutc_status ensure(gutc_luth_client *c){
 gutc_json_dom *registration=NULL;
 gutc_status rc;
 if(!c->auto_register||c->registered)return GUTC_OK;
 rc=gutc_luth_client_register(c,&registration);
 gutc_json_dom_free(registration);
 return rc;
}
gutc_status gutc_luth_client_register(gutc_luth_client *c,
 gutc_json_dom **registration){
 char body[LUTH_MAX_BODY]={0};
 size_t pos=0,attempt;
 gutc_luth_http_response response={0};
 gutc_status rc=GUTC_INVALID_ARGUMENT;
 if(!c||!registration)return GUTC_INVALID_ARGUMENT;
 *registration=NULL;
 if(!push(body,&pos,"{",1)||
    !field(body,&pos,"ecosystemId",view(c->ecosystem_id),0)||
    !field(body,&pos,"applicationId",view(c->application_id),1)||
    !field(body,&pos,"origin",view(c->origin),1)||
    !field(body,&pos,"environment",view(gutc_luth_environment_name(c->environment)),1)||
    !push(body,&pos,"}",1))return GUTC_NO_SPACE;
 for(attempt=0;attempt<=(size_t)c->max_retries;attempt++){
  rc=send(c,"POST","/api/registry/register",body,&response);
  if(rc!=GUTC_OK)return rc;
  if(response.status>=200&&response.status<300&&response.body&&
     response.body->kind==GUTC_JSON_OBJECT){
    c->registered=1;transfer(&response,registration);return GUTC_OK;
  }
  if(response.status<500){
   release(&response);return GUTC_INVALID_FORMAT;
  }
  release(&response);
  if(attempt<c->max_retries&&c->http.sleep){
   size_t delay=(size_t)c->retry_delay_ms*((size_t)1u<<attempt);
   if(delay>60000)delay=60000;
   rc=c->http.sleep(c->http.context,(unsigned)delay);
   if(rc!=GUTC_OK)return rc;
  }
 }
 return GUTC_UNSUPPORTED;
}
static gutc_status anonymous(gutc_json_dom **out){
 static const char body[]="{\"authenticated\":false,\"error\":\"SESSION_INVALID\"}";
 return gutc_json_dom_parse((const uint8_t*)body,sizeof(body)-1,16,100,out,NULL);
}
gutc_status gutc_luth_client_get_session(gutc_luth_client *c,gutc_json_dom **session){
 gutc_luth_http_response response={0};
 const gutc_json_dom *authenticated,*user,*record;
 gutc_status rc;
 if(!c||!session)return GUTC_INVALID_ARGUMENT;
 *session=NULL;
 rc=ensure(c);if(rc!=GUTC_OK)return rc;
 rc=send(c,"GET","/api/session",NULL,&response);
 if(rc!=GUTC_OK)return rc;
 if(response.status==401){
  release(&response);
  rc=gutc_luth_client_clear_token(c);
  return rc==GUTC_OK?anonymous(session):rc;
 }
 if(response.status<200||response.status>=300||!response.body||
    response.body->kind!=GUTC_JSON_OBJECT){
  release(&response);return GUTC_INVALID_FORMAT;
 }
 authenticated=prop(response.body,"authenticated");
 if(!authenticated||authenticated->kind!=GUTC_JSON_BOOLEAN){
  release(&response);return GUTC_INVALID_FORMAT;
 }
 if(authenticated->as.boolean){
  user=prop(response.body,"user");record=prop(response.body,"session");
  if(!user||user->kind!=GUTC_JSON_OBJECT||!record||
     record->kind!=GUTC_JSON_OBJECT||
     !string_present(user,"id")||!string_present(user,"email")||
     !string_present(record,"id")){
    release(&response);return GUTC_INVALID_FORMAT;
  }
 }
 rc=token_update(c,&response);
 if(rc!=GUTC_OK){release(&response);return rc;}
 transfer(&response,session);
 return GUTC_OK;
}
gutc_status gutc_luth_client_refresh_session(gutc_luth_client *c,gutc_json_dom **session){
 return gutc_luth_client_get_session(c,session);
}
static gutc_status post(gutc_luth_client *c,const char *path,
 char *body,gutc_json_dom **result){
 gutc_luth_http_response response={0};
 gutc_status rc;
 if(!c||!result)return GUTC_INVALID_ARGUMENT;
 *result=NULL;
 rc=ensure(c);if(rc!=GUTC_OK)return rc;
 rc=send(c,"POST",path,body,&response);
 if(rc!=GUTC_OK)return rc;
 rc=token_update(c,&response);
 if(rc!=GUTC_OK){release(&response);return rc;}
 if(response.status<200||response.status>=300||!response.body){
  release(&response);return GUTC_INVALID_FORMAT;
 }
 transfer(&response,result);
 return GUTC_OK;
}
gutc_status gutc_luth_client_sign_in_email(gutc_luth_client *c,
 const gutc_luth_email_sign_in *credentials,gutc_json_dom **result){
 char body[LUTH_MAX_BODY]={0};
 size_t at=0;
 gutc_status rc=GUTC_NO_SPACE;
 if(!c||!credentials||!result)return GUTC_INVALID_ARGUMENT;
 *result=NULL;
 if(!push(body,&at,"{",1)||
    !field(body,&at,"email",credentials->email,0)||
    !field(body,&at,"password",credentials->password,1)||
    !push(body,&at,"}",1))goto done;
 rc=post(c,"/api/auth/sign-in/email",body,result);
done:
 wipe(body,sizeof(body));
 return rc;
}
gutc_status gutc_luth_client_sign_up_email(gutc_luth_client *c,
 const gutc_luth_email_sign_up *credentials,gutc_json_dom **result){
 char body[LUTH_MAX_BODY]={0};
 size_t at=0;
 gutc_status rc=GUTC_NO_SPACE;
 if(!c||!credentials||!result)return GUTC_INVALID_ARGUMENT;
 *result=NULL;
 if(!push(body,&at,"{",1)||
    !field(body,&at,"email",credentials->email,0)||
    !field(body,&at,"password",credentials->password,1)||
    !field(body,&at,"name",credentials->name,1)||
    !push(body,&at,"}",1))goto done;
 rc=post(c,"/api/auth/sign-up/email",body,result);
done:
 wipe(body,sizeof(body));return rc;
}
gutc_status gutc_luth_client_sign_in_google(gutc_luth_client *c,
 const gutc_luth_google_sign_in *options,gutc_json_dom **result){
 char body[LUTH_MAX_BODY]={0};
 gutc_auth_text callback;
 size_t at=0;
 gutc_status rc;
 if(!c||!result)return GUTC_INVALID_ARGUMENT;
 *result=NULL;
 callback=options&&options->callback_url.present&&
    has_text(options->callback_url.value)?options->callback_url.value:view(c->origin);
 if(!push(body,&at,"{",1)||
    !field(body,&at,"provider",view("google"),0)||
    !field(body,&at,"callbackURL",callback,1)||
    !push(body,&at,"}",1))return GUTC_NO_SPACE;
 rc=post(c,"/api/auth/sign-in/social",body,result);
 return rc;
}
gutc_status gutc_luth_client_sign_out(gutc_luth_client *c){
 gutc_luth_http_response response={0};
 gutc_json_dom *session=NULL;
 const gutc_json_dom *auth;
 gutc_status rc;
 if(!c)return GUTC_INVALID_ARGUMENT;
 rc=send(c,"POST","/api/auth/sign-out","{}",&response);
 if(rc!=GUTC_OK)return rc;
 if(response.status<200||response.status>=300){
  release(&response);return GUTC_INVALID_FORMAT;
 }
 release(&response);
 rc=gutc_luth_client_clear_token(c);
 if(rc!=GUTC_OK)return rc;
 rc=gutc_luth_client_get_session(c,&session);
 if(rc!=GUTC_OK){gutc_json_dom_free(session);return rc;}
 auth=prop(session,"authenticated");
 if(auth&&auth->kind==GUTC_JSON_BOOLEAN&&auth->as.boolean)
   rc=GUTC_INVALID_FORMAT;
 gutc_json_dom_free(session);
 return rc;
}
