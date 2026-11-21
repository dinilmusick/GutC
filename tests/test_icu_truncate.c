#include "gutc/icu_truncate.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void check(const char *text,size_t limit,const char *suffix,const char *expected){
  char result[4096];size_t need=0,other=0;
  assert(gut_truncate_graphemes_icu(text,limit,suffix,NULL,0,&need)==GUT_TRUNCATE_NOSPACE);
  assert(need==strlen(expected)+1);
  memset(result,0x51,sizeof(result));
  assert(gut_truncate_graphemes_icu(text,limit,suffix,result,need-1,&other)==GUT_TRUNCATE_NOSPACE);
  assert(other==need && result[0]==0x51 && result[need-1]==0x51);
  assert(gut_truncate_graphemes_icu(text,limit,suffix,result,need,&other)==GUT_TRUNCATE_OK);
  assert(!strcmp(result,expected) && other==need);
}
int main(void){
  const char *family="👨‍👩‍👦";
  const char *flag="🇮🇳";
  char bad[10]="unchanged";size_t need=777;
  check("ABCDE",3,"…","AB…");
  check("ABCDE",3,"----","----");
  check("ABCDE",0,"…","…");
  check("ABCDE",0,"","");
  check("ABC",50,"…","ABC");
  check("","0"[0]-'0',"…","");
  check("A👨‍👩‍👦🇮🇳é",3,"…","A👨‍👩‍👦…");
  check("éXYZ",3,"…","éX…");
  check("🇮🇳🇺🇸🇯🇵",2,"…","🇮🇳…");
  check("😊😊😊",2,"…","😊…");
  check("abcd",4,"…","abcd");
  check("ABCDE",3,"","ABC");
  (void)family;(void)flag;
  assert(gut_truncate_graphemes_icu(NULL,2,"…",bad,sizeof(bad),&need)==GUT_TRUNCATE_INVALID);
  assert(gut_truncate_graphemes_icu("ABC",2,NULL,bad,sizeof(bad),&need)==GUT_TRUNCATE_INVALID);
  assert(gut_truncate_graphemes_icu("\xC0\xAF",2,"…",bad,sizeof(bad),&need)==GUT_TRUNCATE_INVALID);
  assert(gut_truncate_graphemes_icu("ABCDE",2,"\xED\xA0\x80",bad,sizeof(bad),&need)==GUT_TRUNCATE_INVALID);
  assert(!strcmp(bad,"unchanged"));
  assert(gut_truncate_graphemes_icu_default("A👨‍👩‍👦🇮🇳é",3,bad,sizeof(bad),&need)==GUT_TRUNCATE_NOSPACE);
  puts("GutC optional ICU native Unicode segmentation/strict buffer/invalid-input contracts passed");
  return 0;
}
