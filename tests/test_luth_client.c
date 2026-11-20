#include "gutc/luth_client.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct route {char name[64],method[16],path[128],body[4096];} route;
typedef struct mock {
 route calls[12];size_t count,next;
 int logged_out,clear_count,token_sets,failover_requests;
 char token[128];
} mock;
static gutc_status get_token(void *ctx,char *out,size_t cap,size_t *needed){
 mock *m=(mock*)ctx;size_t n=strlen(m->token);
 assert(needed);*needed=n;
 if(cap<n)return GUTC_NO_SPACE;
 if(n)memcpy(out,m->token,n);
 return GUTC_OK;
}
static gutc_status set_token(void *ctx,const char *token,size_t n){
 mock *m=(mock*)ctx;
 if(n>=sizeof m->token)return GUTC_NO_SPACE;
 memcpy(m->token,token,n);m->token[n]='\0';m->token_sets++;
 return GUTC_OK;
}
static gutc_status clear_token(void *ctx){
 mock *m=(mock*)ctx;
 memset(m->token,0,sizeof m->token);
 m->clear_count++;
 return GUTC_OK;
}
static gutc_status as_json(const char *json,gutc_luth_http_response *r){
 return gutc_json_dom_parse((const uint8_t*)json,strlen(json),32,1000,&r->body,NULL);
}
static gutc_status http_request(void *ctx,const char *base,const char *method,
  const char *path,const char *body,const char *bearer,size_t bearer_len,
  unsigned timeout,int credentials,int no_store,gutc_luth_http_response *r){
 mock *m=(mock*)ctx;
 const route *route;
 static const char registered[]="{\"registered\":true,\"ecosystemId\":\"eco\",\"applicationId\":\"app\",\"origin\":\"https://app.example\",\"environment\":\"production\",\"authenticationEnabled\":true}";
 static const char session[]="{\"authenticated\":true,\"user\":{\"id\":\"u1\",\"email\":\"e@example.org\"},\"session\":{\"id\":\"s1\"}}";
 assert(credentials==1&&no_store==1&&timeout==10000);
 assert(!strcmp(base,"https://luth.example"));
 assert(m->next<m->count);route=&m->calls[m->next++];
 if(strcmp(method,route->method)||strcmp(path,route->path)||
     strcmp(body?body:"_",route->body)){
  fprintf(stderr,"Unexpected HTTP call %s %s with %s; wanted %s %s with %s\n",
          method,path,body?body:"_",route->method,route->path,route->body);
 }
 assert(!strcmp(method,route->method));
 assert(!strcmp(path,route->path));
 assert(!strcmp(body?body:"_",route->body));
 if(!strcmp(route->name,"register"))
  {r->status=200;return as_json(registered,r);}
 if(!strcmp(route->name,"session")){
  r->status=200;r->auth_token=(gutc_auth_text){"header-token",12};
  return as_json(session,r);
 }
 if(!strcmp(route->name,"session-after-signout")){
  assert(m->logged_out);
  r->status=401;return GUTC_OK;
 }
 if(!strcmp(route->name,"signout"))m->logged_out=1;
 if(!strcmp(route->name,"signin")){
  r->status=200;return as_json("{\"token\":\"body-token\"}",r);
 }
 assert(bearer_len==strlen(bearer?bearer:""));
 r->status=200;return as_json("{\"success\":true}",r);
}
static gutc_auth_text text(const char *x){
 gutc_auth_text t={x,strlen(x)};return t;
}
static void sequence(void){
 FILE *f=fopen(GUTC_LUTH_GOLDEN_PATH,"r");
 char line[8192];mock state={0};
 gutc_luth_client client={0};
 gutc_luth_auth_token_store store={0};
 gutc_luth_http_host http={0};
 gutc_luth_auth_client_config cfg={0};
 gutc_json_dom *result=NULL;
 const gutc_json_dom *authed;
 gutc_luth_email_sign_in email={0};
 gutc_luth_email_sign_up signup={0};
 gutc_status rc;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *a,*b,*c,*end;
  route *q;
  if(line[0]=='#'||line[0]=='\n')continue;
  assert(state.count<sizeof(state.calls)/sizeof(state.calls[0]));
  q=&state.calls[state.count++];
  end=strchr(line,'\n');if(end)*end='\0';
  a=strchr(line,'\t');assert(a);*a++='\0';
  b=strchr(a,'\t');assert(b);*b++='\0';
  c=strchr(b,'\t');assert(c);*c++='\0';
  assert(strlen(line)<sizeof q->name&&strlen(a)<sizeof q->method);
  assert(strlen(b)<sizeof q->path&&strlen(c)<sizeof q->body);
  memcpy(q->name,line,strlen(line)+1);memcpy(q->method,a,strlen(a)+1);
  memcpy(q->path,b,strlen(b)+1);memcpy(q->body,c,strlen(c)+1);
 }
 fclose(f);assert(state.count==7);
 store.context=&state;store.get=get_token;store.set=set_token;store.clear=clear_token;
 http.context=&state;http.request=http_request;
 cfg.ecosystem_id=text("eco");cfg.application_id=text("app");
 cfg.origin.present=1;cfg.origin.value=text("https://app.example");
 cfg.luth_url.present=1;cfg.luth_url.value=text("https://luth.example");
 cfg.auto_register.present=1;cfg.auto_register.value=0;
 cfg.max_retries.present=1;cfg.max_retries.value=0;
 cfg.token_store=&store;
 assert(gutc_luth_client_init(&client,&cfg,NULL,&http)==GUTC_OK);
 assert(client.url_count==1&&client.environment==GUTC_LUTH_PRODUCTION);
 assert(gutc_luth_client_register(&client,&result)==GUTC_OK);
 assert(result&&result->kind==GUTC_JSON_OBJECT);
 gutc_json_dom_free(result);result=NULL;
 assert(gutc_luth_client_get_session(&client,&result)==GUTC_OK);
 authed=gutc_json_dom_get(result,"authenticated",13);
 assert(authed&&authed->kind==GUTC_JSON_BOOLEAN&&authed->as.boolean);
 assert(!strcmp(state.token,"header-token"));
 gutc_json_dom_free(result);result=NULL;
 email.email=text("e@example.org");email.password=text("pa\\ss\"123");
 assert(gutc_luth_client_sign_in_email(&client,&email,&result)==GUTC_OK);
 assert(!strcmp(state.token,"body-token"));
 gutc_json_dom_free(result);result=NULL;
 signup.email=text("e@example.org");signup.password=text("password2");
 signup.name=text("A Woman");
 assert(gutc_luth_client_sign_up_email(&client,&signup,&result)==GUTC_OK);
 gutc_json_dom_free(result);result=NULL;
 assert(gutc_luth_client_sign_in_google(&client,NULL,&result)==GUTC_OK);
 gutc_json_dom_free(result);result=NULL;
 assert(gutc_luth_client_sign_out(&client)==GUTC_OK);
 assert(state.logged_out&&state.clear_count>=2);
 assert(state.token[0]=='\0'&&state.next==state.count);
 assert(state.token_sets==2);
 assert(gutc_luth_client_refresh_session(&client,NULL)==GUTC_INVALID_ARGUMENT);
 gutc_luth_client_destroy(&client);
 (void)rc;
}
static gutc_status failover(void *ctx,const char *base,const char *method,
 const char *path,const char *body,const char *token,size_t token_len,
 unsigned timeout,int credentials,int no_store,gutc_luth_http_response *r){
 mock *m=(mock*)ctx;
 (void)body;(void)token;(void)token_len;
 assert(!strcmp(method,"GET")&&!strcmp(path,"/api/session"));
 assert(timeout==10000&&credentials==1&&no_store==1);
 m->failover_requests++;
 if(!strcmp(base,"https://luth-a.example")){
  r->status=503;return GUTC_OK;
 }
 assert(!strcmp(base,"https://luth-b.example"));
 r->status=200;
 return as_json("{\"authenticated\":false,\"error\":\"SESSION_INVALID\"}",r);
}
static void negative_and_failover(void){
 mock state={0};
 gutc_luth_client c={0};
 gutc_luth_http_host http={0};
 gutc_luth_auth_client_config cfg={0};
 gutc_auth_text urls[2];
 gutc_json_dom *result=NULL;
 cfg.ecosystem_id=text("eco");cfg.application_id=text("app");
 cfg.origin=(gutc_auth_optional_text){1,{"https://app.example",19}};
 cfg.auto_register=(gutc_auth_optional_boolean){1,0};
 cfg.max_retries=(gutc_auth_optional_number){1,0};
 urls[0]=text("https://luth-a.example");
 urls[1]=text("https://luth-b.example");
 cfg.luth_urls.items=urls;cfg.luth_urls.count=2;
 http.context=&state;http.request=failover;
 assert(gutc_luth_client_init(&c,&cfg,NULL,&http)==GUTC_OK);
 assert(c.url_count==2);
 assert(gutc_luth_client_get_session(&c,&result)==GUTC_OK);
 assert(c.active_index==1&&state.failover_requests==2);
 gutc_json_dom_free(result);result=NULL;
 assert(gutc_luth_client_add_url(&c,"http://localhost.evil.example",29)==GUTC_UNSUPPORTED);
 assert(gutc_luth_client_add_url(&c,"http://luth.example",19)==GUTC_UNSUPPORTED);
 assert(gutc_luth_client_add_url(&c,"https://luth-b.example",22)==GUTC_OK);
 assert(c.url_count==2);
 gutc_luth_client_destroy(&c);
 http.request=NULL;
 assert(gutc_luth_client_init(&c,&cfg,NULL,&http)==GUTC_OK);
 assert(gutc_luth_client_get_session(&c,&result)==GUTC_UNSUPPORTED);
 assert(!result);
 gutc_luth_client_destroy(&c);
}
int main(void){
 sequence();
 negative_and_failover();
 puts("GutC native Luth HTTP client request, failover, token store and signout tests passed");
 return 0;
}
