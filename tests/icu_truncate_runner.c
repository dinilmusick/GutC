#include "gutc/icu_truncate.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
  if(argc!=4)return 2;
  errno=0;char *end=NULL;
  const unsigned long long length=strtoull(argv[1],&end,10);
  if(errno||!end||*end||length>(size_t)-1)return 3;
  size_t required=0;
  if(gut_truncate_graphemes_icu(argv[3],(size_t)length,argv[2],NULL,0,&required)!=GUT_TRUNCATE_NOSPACE || required==0)return 4;
  char *out=(char*)malloc(required);
  if(!out)return 5;
  if(gut_truncate_graphemes_icu(argv[3],(size_t)length,argv[2],out,required,&required)!=GUT_TRUNCATE_OK){free(out);return 6;}
  size_t bytes=required-1;
  if(fwrite(out,1,bytes,stdout)!=bytes){free(out);return 7;}
  free(out);
  return 0;
}
