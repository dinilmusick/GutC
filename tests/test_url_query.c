#include "gutc/url_query.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void get(const char *url,const char *key,const char *expected,int exists) {
  char out[256]="original";size_t need=999;int found=-1;
  assert(gutc_url_get_query_param(url,key,NULL,0,&need,&found)==GUTC_OK);
  assert(found==exists);
  if(!exists) {
    assert(need==0);
    assert(gutc_url_get_query_param(url,key,out,sizeof out,&need,&found)==GUTC_OK);
    assert(strcmp(out,"original")==0);
    return;
  }
  assert(need==strlen(expected));
  assert(gutc_url_get_query_param(url,key,out,2,&need,&found)==GUTC_NO_SPACE || need<2);
  assert(gutc_url_get_query_param(url,key,out,sizeof out,&need,&found)==GUTC_OK);
  assert(strcmp(out,expected)==0);
}
static void set(const char *url,const char *key,const char *value,const char *expected) {
  char out[512]="original";
  size_t need=0;
  assert(gutc_url_set_query_param(url,key,value,NULL,0,&need)==GUTC_OK);
  assert(need==strlen(expected));
  assert(gutc_url_set_query_param(url,key,value,out,2,&need)==GUTC_NO_SPACE);
  assert(strcmp(out,"original")==0);
  assert(gutc_url_set_query_param(url,key,value,out,sizeof out,&need)==GUTC_OK);
  if(strcmp(out,expected)!=0)fprintf(stderr,"want %s, got %s\n",expected,out);
  assert(strcmp(out,expected)==0);
}
int main(void) {
  char out[300]="untouched";
  size_t need=0;
  int found=0;
  assert(gutc_url_is_http_url("https://example.com/path"));
  assert(gutc_url_is_http_url("http://127.0.0.1:3000/?a=1"));
  assert(!gutc_url_is_http_url("https://"));
  assert(!gutc_url_is_http_url("not a url"));
  assert(!gutc_url_is_http_url("https://foo\nbad"));
  get("https://example.com/?q=hello+world&q=other#fragment","q","hello world",1);
  get("https://example.com/?name=%C3%A9","name","\xC3\xA9",1);
  get("https://example.com/?x=1&y=2","z","",0);
  get("https://example.com/?x=1&x=2","x","1",1);
  get("https://example.com/?empty","empty","",1);
  set("https://example.com/path","page","2","https://example.com/path?page=2");
  set("https://example.com/?page=1&page=2&x=a+b#f","page","hello world",
      "https://example.com/?page=hello+world&x=a+b#f");
  set("https://example.com/?q=old%20value&x=~","q","a/b",
      "https://example.com/?q=a%2Fb&x=%7E");
  set("https://example.com/?","x","1","https://example.com/?x=1");
  set("https://example.com/#hash","x","1","https://example.com/?x=1#hash");
  assert(gutc_url_get_query_param("https://example.com/?x=%FF","x",out,
          sizeof out,&need,&found)==GUTC_INVALID_ENCODING);
  assert(gutc_url_get_query_param("https://example.com/?x=%ZZ","x",out,
          sizeof out,&need,&found)==GUTC_INVALID_ENCODING);
  assert(gutc_url_set_query_param("https://example.com/?x=%GG","x","1",
          out,sizeof out,&need)==GUTC_INVALID_ENCODING);
  assert(strcmp(out,"untouched")==0);
  puts("GutC URL form-query tests passed");
  return 0;
}
