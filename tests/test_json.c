#include "gutc/json.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void check(const char *json,int expected) {
  size_t index=999;
  gutc_status status=gutc_json_validate((const uint8_t*)json,strlen(json),32,&index);
  if((status==GUTC_OK)!=expected) {
    fprintf(stderr,"json case [%s] got %s at %zu\n",json,gutc_status_name(status),index);
    assert(0);
  }
}
static void check_fixture(void) {
  FILE *f=fopen(GUTC_JSON_FIXTURE_PATH,"r");
  char line[1024];
  size_t rows=0;
  assert(f);
  while(fgets(line,sizeof(line),f)) {
    char *end,*tab;
    if(line[0]=='#')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    tab=strrchr(line,'\t');assert(tab);
    *tab++='\0';
    check(line,tab[0]=='1');
    rows++;
  }
  fclose(f);assert(rows>=10);
}
int main(void) {
  static const uint8_t raw[]={ 'q','"', '\n', '\\', 0, 0xC3u,0xA9u };
  static const uint8_t bad_utf8[]={0xC0u,0xAFu};
  char result[128]="unchanged";
  size_t need=0,at=0;
  check_fixture();
  check("{\"ok\":[1,\"x\",null]}",1);
  check("  false  ",1);
  check("true garbage",0);
  check("[-1.3e+2,0,1E-2]",1);
  check("01",0);
  check("1.",0);
  check("1e",0);
  check("[1,]",0);
  check("{\"a\":1,}",0);
  check("{\"a\" 1}",0);
  check("\"\\uD800\"",1); /* JSON.parse accepts an escaped lone surrogate */
  check("\"\\uD80X\"",0);
  assert(gutc_json_validate((const uint8_t*)"[[0]]",5,1,&at)==GUTC_RANGE_ERROR);
  assert(gutc_json_quote_string(raw,sizeof(raw),NULL,0,&need)==GUTC_OK);
  assert(gutc_json_quote_string(raw,sizeof(raw),result,5,&need)==GUTC_NO_SPACE);
  assert(strcmp(result,"unchanged")==0);
  assert(gutc_json_quote_string(raw,sizeof(raw),result,sizeof(result),&need)==GUTC_OK);
  assert(strcmp(result,"\"q\\\"\\n\\\\\\u0000\xC3\xA9\"")==0);
  assert(gutc_json_quote_string(bad_utf8,sizeof(bad_utf8),result,sizeof(result),&need)
         ==GUTC_INVALID_ENCODING);
  assert(gutc_json_validate(NULL,0,8,&at)==GUTC_INVALID_FORMAT);
  puts("GutC JSON tests passed");
  return 0;
}
