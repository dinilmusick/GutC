#include "gutc/icu_truncate.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static void check(const char *input,size_t limit,const char *suffix,const char *expected){
  char text[512];size_t required=0,short_required=0;
  assert(gut_truncate_lodash_icu(input,limit,suffix,NULL,0,&required)==GUT_TRUNCATE_NOSPACE);
  assert(required==strlen(expected)+1);
  memset(text,0x41,sizeof(text));
  assert(gut_truncate_lodash_icu(input,limit,suffix,text,required-1,&short_required)==GUT_TRUNCATE_NOSPACE);
  assert(short_required==required&&text[0]=='A'&&text[required-1]=='A');
  assert(gut_truncate_lodash_icu(input,limit,suffix,text,sizeof(text),&short_required)==GUT_TRUNCATE_OK);
  assert(!strcmp(text,expected)&&short_required==required);
}
int main(void){
  char unchanged[50]="unchanged";size_t needed=0;
  check("ABCDE",3,"…","AB…");
  check("ABCDE",0,"…","…");
  check("ABCDE",0,"","");
  check("ABCDE",3,"----","----");
  check("ABCDE",3,"","ABC");
  check("शिक्षा",1,"","श");
  check("शिक्षा",2,"","शि");
  check("शिक्षा",3,"…","शि…");
  check("शिक्षा",5,"","शिक्ष");
  check("शिक्षा",6,"…","शिक्षा");
  check("A👨‍👩‍👦🇮🇳é",3,"…","A👨‍👩‍👦…");
  check("🇮🇳🇺🇸🇯🇵",2,"…","🇮🇳…");
  check("👍🏽👍🏼👍🏻",2,"…","👍🏽…");
  check("école",3,"…","éc…");
  check("","0"[0]-'0',"…","");
  assert(gut_truncate_lodash_icu("\xC0\xAF",2,"…",unchanged,sizeof unchanged,&needed)==GUT_TRUNCATE_INVALID);
  assert(gut_truncate_lodash_icu("ABC",2,"\xED\xA0\x80",unchanged,sizeof unchanged,&needed)==GUT_TRUNCATE_INVALID);
  assert(gut_truncate_lodash_icu(NULL,2,"…",unchanged,sizeof unchanged,&needed)==GUT_TRUNCATE_INVALID);
  assert(gut_truncate_lodash_icu("ABC",2,NULL,unchanged,sizeof unchanged,&needed)==GUT_TRUNCATE_INVALID);
  assert(!strcmp(unchanged,"unchanged"));
  assert(gut_truncate_lodash_icu_default("शिक्षा",3,unchanged,sizeof unchanged,&needed)==GUT_TRUNCATE_OK);
  assert(!strcmp(unchanged,"शि…"));
  puts("GutC pinned Lodash Unicode symbol compatibility C11 native byte/UTF8/Devanagari/emoji and buffer tests passed");
  return 0;
}
