#include <gutc/gutc.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  const char *parts[]={"users","44"};
  char result[128];
  size_t needed=0;
  if(gutc_join_url("https://example.test/",parts,2,result,sizeof(result),&needed)!=GUTC_OK) return 1;
  if(strcmp(result,"https://example.test/users/44")!=0) return 2;
  puts(result);
  return 0;
}
