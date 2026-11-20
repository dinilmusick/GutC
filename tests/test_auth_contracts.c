#include "gutc/auth_contracts.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void exercise(const char *operation,const char *text,const char *expected) {
  gutc_json_dom *root=NULL;
  size_t at=0;
  const char *reason=NULL;
  gutc_status rc=gutc_json_dom_parse((const unsigned char*)text,strlen(text),
                                     32,2048,&root,&at);
  if(rc!=GUTC_OK)fprintf(stderr,"Fixture DOM parse error: %s at %zu\n",
                         gutc_status_name(rc),at);
  assert(rc==GUTC_OK);
  if(strcmp(operation,"capability")==0)
    rc=gutc_validate_capability_descriptor_v1(root,&reason);
  else if(strcmp(operation,"invocation")==0)
    rc=gutc_validate_scoped_invocation_v1(root,&reason);
  else if(strncmp(operation,"budget:",7)==0)
    rc=gutc_check_scoped_invocation_budget_v1(root,atof(operation+7),&reason);
  else if(strncmp(operation,"match:",6)==0)
    rc=gutc_match_invocation_descriptor_v1(
      gutc_json_dom_get(root,"invocation",10),
      gutc_json_dom_get(root,"descriptor",10),
      atof(operation+6),&reason);
  else abort();
  if(rc==GUTC_OK) {
    if(strcmp(expected,"OK")!=0) {
      fprintf(stderr,"%s wanted %s but returned success\n",operation,expected);abort();
    }
    assert(reason==NULL);
  } else {
    if(!reason||strcmp(reason,expected)!=0) {
      fprintf(stderr,"%s wanted %s but got %s (%s)\n",
              operation,expected,reason?reason:"(null)",gutc_status_name(rc));abort();
    }
  }
  gutc_json_dom_free(root);
}
int main(void) {
  FILE *file=fopen(GUTC_AUTH_FIXTURE_PATH,"r");
  char line[32000];
  size_t rows=0;
  assert(file);
  while(fgets(line,sizeof(line),file)) {
    char *a,*b,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strrchr(a,'\t');assert(b);*b++='\0';
    exercise(line,a,b);
    rows++;
  }
  fclose(file);
  assert(rows>=25);
  puts("GutC pinned auth contract fixtures passed");
  return 0;
}
