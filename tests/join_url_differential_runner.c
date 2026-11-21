#include "gutc/gutc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
  const char *parts[64];
  size_t needed=0;
  char *out;
  gutc_status status;
  if(argc<2 || argc>66) return 2;
  for(int i=2;i<argc;i++) parts[i-2]=argv[i];
  status=gutc_join_url(argv[1],argc>2?parts:NULL,(size_t)(argc-2),NULL,0,&needed);
  if(status!=GUTC_OK || needed==SIZE_MAX) return 3;
  out=(char*)malloc(needed+1);
  if(!out)return 4;
  status=gutc_join_url(argv[1],argc>2?parts:NULL,(size_t)(argc-2),out,needed+1,&needed);
  if(status!=GUTC_OK) {free(out);return 5;}
  if(fwrite(out,1,needed,stdout)!=needed) {free(out);return 6;}
  free(out);
  return 0;
}
