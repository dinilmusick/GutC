#include "gutc/js_number_format.h"
#include "gutc/json_dom.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void test_case(const char *literal,const char *wanted,size_t index){
 gutc_json_dom *root=NULL;
 char rendered[512],serial[512],unchanged[512];
 size_t needed=0,serial_len=0,n;
 gutc_status rc;
 rc=gutc_json_dom_parse((const uint8_t*)literal,strlen(literal),
                        16,100,&root,NULL);
 if(rc!=GUTC_OK||!root||root->kind!=GUTC_JSON_NUMBER){
  fprintf(stderr,"JSON input %zu parse failed: %s, status %d\n",index,literal,(int)rc);
 }
 assert(rc==GUTC_OK&&root&&root->kind==GUTC_JSON_NUMBER);
 assert(gutc_js_number_format(root->as.number,NULL,0,&needed)==GUTC_OK);
 assert(needed<sizeof rendered);
 memset(rendered,'Q',sizeof rendered);
 assert(gutc_js_number_format(root->as.number,rendered,sizeof rendered,&n)==GUTC_OK);
 assert(n==needed);
 assert(!strcmp(rendered,wanted));
 memset(unchanged,'X',sizeof unchanged);
 assert(gutc_js_number_format(root->as.number,unchanged,needed,&n)==GUTC_NO_SPACE);
 assert(unchanged[0]=='X'&&unchanged[needed]=='X');
 rc=gutc_json_dom_stringify(root,serial,sizeof serial,&serial_len);
 if(rc!=GUTC_OK||strcmp(serial,wanted))
  fprintf(stderr,"Numeric JS mismatch #%zu: input %s got %s wanted %s\n",
          index,literal,rc==GUTC_OK?serial:"[format-error]",wanted);
 assert(rc==GUTC_OK);
 assert(!strcmp(serial,wanted));
 assert(serial_len==needed);
 gutc_json_dom_free(root);
}
int main(void){
 FILE *f=fopen(GUTC_JSON_NUMBERS_PATH,"r");
 char line[1024];size_t count=0;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *tab,*end;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  tab=strchr(line,'\t');assert(tab);*tab++='\0';
  test_case(line,tab,count);count++;
 }
 fclose(f);assert(count==61);
 {char out[64];size_t n;
  assert(gutc_js_number_format(NAN,out,sizeof out,&n)==GUTC_OK);
  assert(!strcmp(out,"null"));
  assert(gutc_js_number_format(INFINITY,out,sizeof out,&n)==GUTC_OK);
  assert(!strcmp(out,"null"));
  assert(gutc_js_number_format(-INFINITY,out,sizeof out,&n)==GUTC_OK);
  assert(!strcmp(out,"null"));
  assert(gutc_js_number_format(-0.0,out,sizeof out,&n)==GUTC_OK);
  assert(!strcmp(out,"0"));}
 puts("GutC source-pinned shortest-roundtrip JSON number and negative buffer tests passed");
 return 0;
}
