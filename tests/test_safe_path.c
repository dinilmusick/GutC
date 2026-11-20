#include "gutc/safe_path.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void good(const char *input,const char *expected){
  char buffer[512]="unchanged";size_t needed=0;
  assert(gutc_encode_safe_path_segments(input,NULL,0,&needed)==GUTC_OK);
  assert(needed==strlen(expected));
  assert(gutc_encode_safe_path_segments(input,buffer,2,&needed)==GUTC_NO_SPACE);
  assert(strcmp(buffer,"unchanged")==0);
  assert(gutc_encode_safe_path_segments(input,buffer,sizeof(buffer),&needed)==GUTC_OK);
  assert(strcmp(buffer,expected)==0);
}
static void bad(const char *input){
  char buffer[512]="unchanged";size_t needed=0;
  gutc_status s=gutc_encode_safe_path_segments(input,buffer,sizeof(buffer),&needed);
  assert(s==GUTC_INVALID_FORMAT||s==GUTC_INVALID_ENCODING);
  assert(strcmp(buffer,"unchanged")==0);
}
int main(void){
  const char *cases[]={"","/","..","../secret","./a","%2E%2E/a","%252e%252e/x",
    "a/%2F","a%5cb","%FF","%25FF","a%","a?b","a#b","a\\b",NULL};
  size_t i;
  good("///photos///a b.txt/","photos/a%20b.txt");
  good("a!b/c'(d)*","a!b/c'(d)*");
  good("cafe\xCC\x81","cafe%CC%81");
  good("a%20b", "a%2520b");
  for(i=0;cases[i];i++)bad(cases[i]);
  puts("GutC safe path tests passed");
  return 0;
}
