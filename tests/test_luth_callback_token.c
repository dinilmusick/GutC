#include "gutc/luth_callback_token.h"
#include "gutc/json_dom.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct test_host {
  char token[8192];
  char path[32768];
  size_t token_len,path_len;
  int store_calls,replace_calls;
} test_host;
static gutc_status store_token(void *context,const char *bytes,size_t n){
  test_host *h=(test_host*)context;
  if(!bytes||n>=sizeof h->token)return GUTC_RANGE_ERROR;
  if(n)memcpy(h->token,bytes,n);
  h->token[n]='\0';h->token_len=n;h->store_calls++;
  return GUTC_OK;
}
static gutc_status replace_history(void *context,const char *url,size_t n){
  test_host *h=(test_host*)context;
  if(!url||n>=sizeof h->path)return GUTC_RANGE_ERROR;
  if(n)memcpy(h->path,url,n);
  h->path[n]='\0';h->path_len=n;h->replace_calls++;
  return GUTC_OK;
}
static gutc_json_dom *parse(const char *json){
  gutc_json_dom *v=NULL;
  assert(gutc_json_dom_parse((const uint8_t*)json,strlen(json),16,100,&v,NULL)==GUTC_OK);
  assert(v);return v;
}
static const char *str(const gutc_json_dom *v){
  if(v->kind==GUTC_JSON_NULL)return NULL;
  assert(v->kind==GUTC_JSON_STRING);
  assert(!memchr(v->as.string.bytes,0,v->as.string.length));
  return v->as.string.bytes;
}
static void run(const char *raw_path,const char *raw_search,const char *raw_hash,
                const char *raw_token,const char *raw_history){
  gutc_json_dom *p=parse(raw_path),*q=parse(raw_search),*h=parse(raw_hash),
                *token=parse(raw_token),*history=parse(raw_history);
  test_host actual={{0},{0},0,0,0,0};
  gutc_luth_callback_host host={&actual,store_token,replace_history};
  int done=-1;
  gutc_status rc=gutc_luth_hydrate_callback_token(str(p),str(q),str(h),&host,&done);
  if(rc!=GUTC_OK)fprintf(stderr,"Callback parsing error %s for %s\n",
                         gutc_status_name(rc),raw_search);
  assert(rc==GUTC_OK);
  if(str(token)){
    assert(done==1&&actual.store_calls==1&&actual.replace_calls==1);
    if(strcmp(actual.token,str(token))||strcmp(actual.path,str(history)))
      fprintf(stderr,"callback mismatch %s token %s wanted %s; path %s wanted %s\n",
              raw_search,actual.token,str(token),actual.path,str(history));
    assert(!strcmp(actual.token,str(token)));
    assert(!strcmp(actual.path,str(history)));
  }else assert(done==0&&!actual.store_calls&&!actual.replace_calls);
  gutc_json_dom_free(p);gutc_json_dom_free(q);gutc_json_dom_free(h);
  gutc_json_dom_free(token);gutc_json_dom_free(history);
}
static gutc_status failing_store(void *context,const char *s,size_t n){
  (void)context;(void)s;(void)n;
  return GUTC_DEADLINE_EXCEEDED;
}
int main(void){
  FILE *file=fopen(GUTC_LUTH_TOKEN_GOLDEN_PATH,"r");
  char line[18000];size_t rows=0;
  assert(file);
  while(fgets(line,sizeof line,file)){
    char *a,*b,*c,*d,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    d=strchr(c,'\t');assert(d);*d++='\0';
    run(line,a,b,c,d);rows++;
  }
  fclose(file);assert(rows>=30);
  {test_host actual={{0},{0},0,0,0,0};
   gutc_luth_callback_host host={&actual,store_token,replace_history};
   int done=1;
   assert(gutc_luth_hydrate_callback_token("/cb","?token=%ZZ","",&host,&done)==GUTC_UNSUPPORTED);
   assert(done==0&&actual.store_calls==0);
   assert(gutc_luth_hydrate_callback_token("/cb","?token=%00","",&host,&done)==GUTC_UNSUPPORTED);
   assert(gutc_luth_hydrate_callback_token("/cb","?token=x","",&host,NULL)==GUTC_INVALID_ARGUMENT);
   host.store_token=failing_store;
   assert(gutc_luth_hydrate_callback_token("/cb","?token=x","",&host,&done)==GUTC_DEADLINE_EXCEEDED);
   assert(done==0&&actual.replace_calls==0);}
  puts("GutC source-verified Luth callback token and host effects passed");
  return 0;
}
