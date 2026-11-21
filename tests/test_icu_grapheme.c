#include "gutc/icu_truncate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do {if(!(x)){fprintf(stderr,"ICU grapheme contract failed: %s at line %d\n",#x,__LINE__);exit(1);}}while(0)
static void accepted(const char *input,size_t count,const char *suffix,const char *want) {
 char out[256],scratch[256];
 size_t needed=0,second=0;
 REQUIRE(gut_truncate_graphemes_icu(input,count,suffix,NULL,0,&needed)==GUT_TRUNCATE_NOSPACE);
 REQUIRE(needed==strlen(want)+1);
 memset(scratch,'X',sizeof scratch);
 REQUIRE(gut_truncate_graphemes_icu(input,count,suffix,scratch,needed-1,&second)==GUT_TRUNCATE_NOSPACE);
 REQUIRE(scratch[0]=='X'&&scratch[needed]=='X'&&second==needed);
 REQUIRE(gut_truncate_graphemes_icu(input,count,suffix,out,sizeof out,&second)==GUT_TRUNCATE_OK);
 REQUIRE(second==needed&&!strcmp(out,want));
}
int main(void){
 accepted("abcdef",4,"...", "a...");
 accepted("abcdef",3,"...", "...");
 accepted("abcdef",0,"...", "...");
 accepted("abcdef",10,"...", "abcdef");
 accepted("abc",3,"...", "abc");
 accepted("abc",1,"", "a");
 accepted("a\xCC\x81bc",2,"...","...");
 accepted("a\xCC\x81bc",2,"…","a\xCC\x81…");
 accepted("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB3" "X",1,"…","…");
 accepted("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB3" "X",2,"…","\xF0\x9F\x87\xAE\xF0\x9F\x87\xB3…");
 accepted("👩‍👩‍👧‍👦.txt",2,"…","👩‍👩‍👧‍👦…");
 accepted("👩‍👩‍👧‍👦.txt",20,"…","👩‍👩‍👧‍👦.txt");
 accepted("a",0,"",""); 
 accepted("",0,"…","");
 {
  char out[15];size_t required=0;
  memset(out,'Y',sizeof out);
  REQUIRE(gut_truncate_graphemes_icu("\xC0\xAF",1,"…",out,sizeof out,&required)==GUT_TRUNCATE_INVALID);
  REQUIRE(out[0]=='Y');
  REQUIRE(gut_truncate_graphemes_icu("a\xED\xA0\x80",2,"…",out,sizeof out,&required)==GUT_TRUNCATE_INVALID);
  REQUIRE(out[0]=='Y');
  REQUIRE(gut_truncate_graphemes_icu("abc",2,"\xFF",out,sizeof out,&required)==GUT_TRUNCATE_INVALID);
  REQUIRE(out[0]=='Y');
  REQUIRE(gut_truncate_graphemes_icu(NULL,1,"…",out,sizeof out,&required)==GUT_TRUNCATE_INVALID);
 }
 puts("Real ICU extended grapheme segmentation and UTF-8 buffer/negative tests passed");
}
