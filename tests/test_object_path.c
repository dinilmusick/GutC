#include "gutc/object_path.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static gutc_json_dom *parse(const char *json) {
  gutc_json_dom *out=NULL;size_t at=0;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)json,strlen(json),64,10000,&out,&at);
  if(rc!=GUTC_OK)fprintf(stderr,"DOM parse error at %zu for %s\n",at,json);
  assert(rc==GUTC_OK&&out!=NULL);return out;
}
static void check(const char *op,const char *src,
                  const char *pathspec,const char *expected) {
  gutc_json_dom *root=parse(src),*paths=parse(pathspec),*output=NULL;
  gutc_json_text view[256];size_t i,n;
  char buffer[8192];
  gutc_status rc;
  assert(paths->kind==GUTC_JSON_ARRAY && paths->as.array.count<=256);
  n=paths->as.array.count;
  for(i=0;i<n;i++) {
    const gutc_json_dom *v=paths->as.array.values[i];
    assert(v->kind==GUTC_JSON_STRING);
    view[i]=v->as.string;
  }
  if(!strcmp(op,"has")) {
    int found=-1;
    assert(n==1);
    rc=gutc_json_path_has(root,view[0].bytes,view[0].length,&found);
    assert(rc==GUTC_OK);
    if((found&&strcmp(expected,"true"))||(!found&&strcmp(expected,"false"))) {
      fprintf(stderr,"has path %s expected %s got %d\n",view[0].bytes,expected,found);
      abort();
    }
  } else if(!strcmp(op,"plain")) {
    int plain=gutc_json_is_plain_object(root);
    assert((plain&&!strcmp(expected,"true"))||(!plain&&!strcmp(expected,"false")));
  } else {
    if(!strcmp(op,"pick"))rc=gutc_json_pick_paths(root,view,n,&output);
    else if(!strcmp(op,"omit"))rc=gutc_json_omit_paths(root,view,n,&output);
    else abort();
    if(rc!=GUTC_OK)fprintf(stderr,"%s failed %s for paths %s\n",
                           op,gutc_status_name(rc),pathspec);
    assert(rc==GUTC_OK && output!=NULL);
    /* The owned result must survive freeing the original source tree. */
    gutc_json_dom_free(root);root=NULL;
    {size_t needed=0;
     assert(gutc_json_dom_stringify(output,buffer,sizeof(buffer),&needed)==GUTC_OK);}
    if(strcmp(buffer,expected)) {
      fprintf(stderr,"%s paths %s\nexpected %s\ngot      %s\n",
              op,pathspec,expected,buffer);
      abort();
    }
    gutc_json_dom_free(output);
  }
  gutc_json_dom_free(root);
  gutc_json_dom_free(paths);
}
int main(void) {
  FILE *f=fopen(GUTC_OBJECT_GOLDEN_PATH,"r");
  char line[24000];
  size_t rows=0;
  assert(f);
  while(fgets(line,sizeof(line),f)) {
    char *a,*b,*c,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    check(line,a,b,c);rows++;
  }
  fclose(f);assert(rows>=30);
  {gutc_json_dom *v=parse("{\"safe\":1}"),*out=NULL;
   const gutc_json_text paths[]={{"__proto__.polluted",18}};
   assert(gutc_json_pick_paths(v,paths,1,&out)==GUTC_UNSUPPORTED&&out==NULL);
   gutc_json_dom_free(v);
  }
  puts("GutC pinned nested object-path fixtures passed");
  return 0;
}
