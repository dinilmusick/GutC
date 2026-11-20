#include "gutc/oauth_contracts.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void exercise(const char *operation,const char *text,const char *expected) {
  gutc_json_dom *root=NULL;
  size_t offset=0;
  const char *reason=NULL;
  gutc_status rc=gutc_json_dom_parse((const unsigned char*)text,strlen(text),
                                     32,1024,&root,&offset);
  if(rc!=GUTC_OK)fprintf(stderr,"OAuth corpus parse failed at %zu: %s\n",
                         offset,gutc_status_name(rc));
  assert(rc==GUTC_OK);
  if(strcmp(operation,"registration")==0)
    rc=gutc_validate_oauth_registration_v1(root,&reason);
  else if(strncmp(operation,"redirect:",9)==0) {
    gutc_oauth_platform platform=GUTC_OAUTH_WEB;
    if(!strcmp(operation+9,"desktop"))platform=GUTC_OAUTH_DESKTOP;
    else if(!strcmp(operation+9,"ios"))platform=GUTC_OAUTH_IOS;
    else if(!strcmp(operation+9,"android"))platform=GUTC_OAUTH_ANDROID;
    else assert(!strcmp(operation+9,"web"));
    rc=gutc_validate_oauth_redirect_v1(root,platform,NULL,&reason);
  } else abort();
  if(rc==GUTC_OK && strcmp(expected,"OK")) {
    fprintf(stderr,"OAuth expected %s got OK: %s\n",expected,text);abort();
  }
  if(rc!=GUTC_OK && (!reason || strcmp(expected,reason))) {
    fprintf(stderr,"OAuth expected %s got %s (%s)\n",expected,
            reason?reason:"(null)",gutc_status_name(rc));abort();
  }
  gutc_json_dom_free(root);
}
int main(void) {
  FILE *f=fopen(GUTC_OAUTH_FIXTURE_PATH,"r");
  char line[10000];
  size_t cases=0;
  assert(f);
  while(fgets(line,sizeof(line),f)) {
    char *a,*b,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strrchr(a,'\t');assert(b);*b++='\0';
    exercise(line,a,b);cases++;
  }
  fclose(f);assert(cases>=25);
  puts("GutC pinned OAuth redirect and registration fixtures passed");
  return 0;
}
