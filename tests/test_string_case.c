#include "gutc/string_case.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check(const char *op,const char *input,const char *expected) {
  char out[256]="untouched";size_t needed=0;
  gutc_status rc;
  if(strcmp(op,"email")==0) {
    int valid=-1;
    rc=gutc_is_email_ascii(input,strlen(input),&valid);
    assert(rc==GUTC_OK);
    assert(valid==(expected[0]=='1'));
    return;
  }
  gutc_string_case style;
  if(!strcmp(op,"capitalize"))style=GUTC_CASE_CAPITALIZE;
  else if(!strcmp(op,"camel"))style=GUTC_CASE_CAMEL;
  else if(!strcmp(op,"kebab"))style=GUTC_CASE_KEBAB;
  else if(!strcmp(op,"snake"))style=GUTC_CASE_SNAKE;
  else abort();
  rc=gutc_string_transform_ascii(input,strlen(input),style,NULL,0,&needed);
  assert(rc==GUTC_OK&&needed==strlen(expected));
  if(needed>3) {
    assert(gutc_string_transform_ascii(input,strlen(input),style,out,3,&needed)==GUTC_NO_SPACE);
    assert(!strcmp(out,"untouched"));
  }
  assert(gutc_string_transform_ascii(input,strlen(input),style,out,sizeof(out),&needed)==GUTC_OK);
  if(strcmp(out,expected)!=0)fprintf(stderr,"transform %s input [%s] expected [%s] got [%s]\n",
                                      op,input,expected,out);
  assert(!strcmp(out,expected));
}
int main(void) {
  FILE *file=fopen(GUTC_STRINGS_FIXTURE_PATH,"r");
  char line[2048];
  size_t n=0,needed=0;
  char out[64]="safe";
  assert(file);
  while(fgets(line,sizeof(line),file)){
    char *a,*b,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strrchr(a,'\t');assert(b);*b++='\0';
    check(line,a,b);n++;
  }
  fclose(file);assert(n>=30);
  assert(gutc_string_transform_ascii("\xC3\xA9",2,GUTC_CASE_CAMEL,
          out,sizeof(out),&needed)==GUTC_UNSUPPORTED);
  assert(!strcmp(out,"safe"));
  {int valid=0;
   assert(gutc_is_email_ascii("\xC3\xA9@example.com",14,&valid)==GUTC_UNSUPPORTED);}
  puts("GutC ASCII string golden cases passed");
  return 0;
}
