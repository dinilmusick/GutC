#include "../include/gutc_trusted_api.h"
#include <assert.h>
#include <string.h>
int main(void){
  char out[256]="unchanged";size_t need=0,i;
  const char *paths[]={"/../admin","/%2e%2e/admin","/%252e%252e/admin",
    "/a%2f..%2fadmin","//evil.com/x","https://evil.com/x",
    "/bad%","/a%00b","/a%3Fb","/a#fragment","/a\\b"};
  const char *origins[]={"http://gitlab.com","https://user@gitlab.com",
    "https://gitlab.com/evil","https://gitlab.com:0","https://gitlab.com:99999",
    "https://gitlab.com?x=1","https://gitlab.com#x","https://gitlab.com:bad"};
  assert(gutc_resolve_trusted_api_url("https://gitlab.com",
    "/api/v4/projects/a%2Fb?x=1",out,sizeof(out),&need)==0);
  assert(strcmp(out,"https://gitlab.com/api/v4/projects/a%2Fb?x=1")==0);
  assert(need==strlen(out)+1);
  assert(gutc_resolve_trusted_api_url("https://gitlab.com","/safe",out,2,&need)==-2);
  assert(strcmp(out,"https://gitlab.com/api/v4/projects/a%2Fb?x=1")==0);
  for(i=0;i<sizeof(paths)/sizeof(paths[0]);i++)
    assert(gutc_resolve_trusted_api_url("https://gitlab.com",paths[i],out,sizeof(out),0)==-1);
  for(i=0;i<sizeof(origins)/sizeof(origins[0]);i++)
    assert(gutc_resolve_trusted_api_url(origins[i],"/api",out,sizeof(out),0)==-1);
  assert(gutc_resolve_trusted_api_url("https://gitlab.com:443","/api",out,sizeof(out),0)==0);
  return 0;
}
