#include "gutc/icu_truncate.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
  char *end=NULL;size_t required=0,bytes;
  if(argc!=4)return 2;
  errno=0;
  const unsigned long long max=strtoull(argv[1],&end,10);
  if(errno||!end||*end||max>(size_t)-1)return 3;
  if(gut_truncate_lodash_icu(argv[3],(size_t)max,argv[2],NULL,0,&required)!=GUT_TRUNCATE_NOSPACE||!required)return 4;
  char *out=(char*)malloc(required);
  if(!out)return 5;
  if(gut_truncate_lodash_icu(argv[3],(size_t)max,argv[2],out,required,&required)!=GUT_TRUNCATE_OK){free(out);return 6;}
  bytes=required-1;
  if(fwrite(out,1,bytes,stdout)!=bytes){free(out);return 7;}
  free(out);
  return 0;
}
