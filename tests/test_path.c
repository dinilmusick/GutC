#include "gutc_path.h"
#include <assert.h>
#include <string.h>
int main(void) {
  char out[128];
  assert(gutc_normalize_storage_path("///foo//bar/",out,sizeof out)==GUTC_PATH_OK && strcmp(out,"foo/bar")==0);
  assert(gutc_normalize_storage_path("/",out,sizeof out)==GUTC_PATH_OK && !*out);
  const char *bad[]={"../secret","foo/./bar","%2e%2e/x","%252e%252e/x","%25252e%25252e/x","a%2fb","a%255cb","x?y","x#y","x\\y","x%00y","x%","%252525252525252525252e",NULL};
  for (int i=0;bad[i];i++) assert(gutc_normalize_storage_path(bad[i],out,sizeof out)==GUTC_PATH_INVALID && !*out);
  assert(gutc_normalize_storage_path("hello/world",out,5)==GUTC_PATH_OVERFLOW && !*out);
  assert(gutc_normalize_storage_path("a%20b/file.txt",out,sizeof out)==GUTC_PATH_OK && strcmp(out,"a%20b/file.txt")==0);
  return 0;
}
