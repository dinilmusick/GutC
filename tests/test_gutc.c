#include "gutc/gutc.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_utf8(void) {
  static const uint8_t good[]={0x41,0xC3,0xA9,0xF0,0x9F,0x8C,0x8D};
  static const uint8_t overlong[]={0xC0,0xAF};
  static const uint8_t surrogate[]={0xED,0xA0,0x80};
  static const uint8_t truncated[]={0xF0,0x9F};
  static const uint8_t excess[]={0xF4,0x90,0x80,0x80};
  size_t points=999;
  assert(gutc_utf8_validate(good,sizeof(good),&points)==GUTC_OK && points==3);
  assert(gutc_utf8_validate(NULL,0,&points)==GUTC_OK && points==0);
  assert(gutc_utf8_validate(overlong,sizeof(overlong),&points)==GUTC_INVALID_ENCODING);
  assert(gutc_utf8_validate(surrogate,sizeof(surrogate),&points)==GUTC_INVALID_ENCODING);
  assert(gutc_utf8_validate(truncated,sizeof(truncated),&points)==GUTC_INVALID_ENCODING);
  assert(gutc_utf8_validate(excess,sizeof(excess),&points)==GUTC_INVALID_ENCODING);
  assert(gutc_utf8_validate(NULL,1,&points)==GUTC_INVALID_ARGUMENT);
}
static void test_percent(void) {
  static const uint8_t binary[]={0,0x20,0x2f,0xff,'~','A'};
  static const char expected[]="%00%20%2F%FF~A";
  char encoded[32]="UNCHANGED";
  uint8_t decoded[16]={99};
  size_t need=0;
  assert(gutc_percent_encode(binary,sizeof(binary),NULL,0,&need)==GUTC_OK && need==strlen(expected));
  assert(gutc_percent_encode(binary,sizeof(binary),encoded,3,&need)==GUTC_NO_SPACE);
  assert(strcmp(encoded,"UNCHANGED")==0);
  assert(gutc_percent_encode(binary,sizeof(binary),encoded,sizeof(encoded),&need)==GUTC_OK);
  assert(strcmp(encoded,expected)==0);
  assert(gutc_percent_decode(expected,strlen(expected),decoded,2,&need)==GUTC_NO_SPACE);
  assert(decoded[0]==99);
  assert(gutc_percent_decode(expected,strlen(expected),decoded,sizeof(decoded),&need)==GUTC_OK);
  assert(need==sizeof(binary) && memcmp(decoded,binary,sizeof(binary))==0);
  assert(gutc_percent_decode("%",1,decoded,sizeof(decoded),&need)==GUTC_INVALID_ENCODING);
  assert(gutc_percent_decode("%0G",3,decoded,sizeof(decoded),&need)==GUTC_INVALID_ENCODING);
  assert(gutc_percent_decode("%C3%A9",6,decoded,sizeof(decoded),&need)==GUTC_OK && need==2);
  assert(gutc_percent_encode(NULL,0,encoded,1,&need)==GUTC_OK && encoded[0]=='\0');
}
static void test_url(void) {
  const char *parts[]={"/users/","//sub//", "/42/"};
  char out[100]="ORIGINAL";
  size_t need=0;
  assert(gutc_join_url("https://host///",parts,3,NULL,0,&need)==GUTC_OK);
  assert(need==strlen("https://host/users/sub/42"));
  assert(gutc_join_url("https://host///",parts,3,out,3,&need)==GUTC_NO_SPACE);
  assert(strcmp(out,"ORIGINAL")==0);
  assert(gutc_join_url("https://host///",parts,3,out,sizeof(out),&need)==GUTC_OK);
  assert(strcmp(out,"https://host/users/sub/42")==0);
  assert(gutc_join_url("/",NULL,0,out,sizeof(out),&need)==GUTC_OK && strcmp(out,"")==0);
  assert(gutc_join_url("a",NULL,1,out,sizeof(out),&need)==GUTC_INVALID_ARGUMENT);
}
static void test_identity_fixture(void) {
  FILE *f=fopen(GUTC_FIXTURE_PATH,"r");
  char line[512];
  size_t rows=0;
  assert(f!=NULL);
  while(fgets(line,sizeof(line),f)) {
    char *tab, *result;
    int expected;
    size_t n;
    if(line[0]=='#') continue;
    tab=strchr(line,'\t'); assert(tab); *tab++='\0';
    result=strchr(tab,'\n'); if(result) *result='\0';
    expected=tab[0]=='1';
    n=strlen(line);
    assert(gutc_identity_id_valid(line,n)==expected);
    rows++;
  }
  fclose(f); assert(rows>=10);
  assert(gutc_identity_kind_valid("application"));
  assert(gutc_identity_kind_valid("ui_session"));
  assert(!gutc_identity_kind_valid("unknown"));
}
static void test_strings_numbers(void) {
  const char *prefixes[]={"no","abc"};
  const char *suffixes[]={"no","\xC3\xA9"};
  int result=-1;
  double value=-1;
  size_t chunks=999;
  const double numbers[]={1.5,2.5,-1.0};
  assert(gutc_string_blank_ascii(" \t\n",3,&result)==GUTC_OK && result);
  assert(gutc_string_blank_ascii(" x",2,&result)==GUTC_OK && !result);
  assert(gutc_string_starts_with_any("abcd",4,prefixes,2,&result)==GUTC_OK && result);
  assert(gutc_string_ends_with_any("caf\xC3\xA9",5,suffixes,2,&result)==GUTC_OK && result);
  assert(gutc_number_clamp(12,0,10,&value)==GUTC_OK && value==10);
  assert(gutc_number_clamp(12,10,0,&value)==GUTC_RANGE_ERROR);
  assert(gutc_number_clamp(NAN,0,10,&value)==GUTC_INVALID_ARGUMENT);
  assert(gutc_number_in_range(10,0,10,&result)==GUTC_OK && !result);
  assert(gutc_number_in_range(9,0,10,&result)==GUTC_OK && result);
  assert(gutc_number_sum(numbers,3,&value)==GUTC_OK && value==3);
  assert(gutc_array_chunk_count(11,4,&chunks)==GUTC_OK && chunks==3);
  assert(gutc_array_chunk_count(0,4,&chunks)==GUTC_OK && chunks==0);
  assert(gutc_array_chunk_count(8,0,&chunks)==GUTC_RANGE_ERROR);
}
static void test_budget(void) {
  const char *chain[]={"agent_a"};
  gutc_invocation_budget b={"agent_a","target_b","target_b",chain,1,300,4,3,1};
  unsigned remaining=500;
  assert(gutc_invocation_budget_check(&b,100)==GUTC_OK);
  assert(gutc_invocation_budget_advance(&b,100,&remaining)==GUTC_OK && remaining==2);
  assert(gutc_invocation_budget_check(&b,300)==GUTC_DEADLINE_EXCEEDED);
  b.max_depth=1;
  assert(gutc_invocation_budget_check(&b,100)==GUTC_MAX_DEPTH);
  b.max_depth=4;b.target_app_id="agent_a";b.audience_app_id="agent_a";
  assert(gutc_invocation_budget_check(&b,100)==GUTC_CYCLE);
  b.target_app_id="target_b";b.audience_app_id="target_b";b.remaining_calls=0;
  assert(gutc_invocation_budget_check(&b,100)==GUTC_BUDGET_EXHAUSTED);
  assert(gutc_invocation_budget_advance(&b,100,&remaining)==GUTC_BUDGET_EXHAUSTED && remaining==2);
}
int main(void) {
  test_utf8();test_percent();test_url();test_identity_fixture();
  test_strings_numbers();test_budget();
  puts("GutC native foundation tests passed");
  return 0;
}
