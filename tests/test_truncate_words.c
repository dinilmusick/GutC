#include "../include/gut_truncate_words.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 char out[128];size_t n=0;
 CHECK(gut_truncate_words_utf8_default("the quick brown fox jumps",3,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"the quick brown\xE2\x80\xA6")==0&&n==19);
 CHECK(gut_truncate_words_utf8_default(" a  b \t c ",2,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"a b\xE2\x80\xA6")==0);
 CHECK(gut_truncate_words_utf8_default("a b",2,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"a b")==0&&n==4);
 CHECK(gut_truncate_words_utf8_default("a b",0,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"\xE2\x80\xA6")==0&&n==4);
 CHECK(gut_truncate_words_utf8_default("",0,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"")==0&&n==1);
 CHECK(gut_truncate_words_utf8_default("  ",0,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"\xE2\x80\xA6")==0);
 CHECK(gut_truncate_words_utf8_default("a\xC2\xA0" "b\xE3\x80\x80" "c",2,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"a b\xE2\x80\xA6")==0);
 CHECK(gut_truncate_words_utf8_default("a\xEF\xBB\xBF" "b c",2,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"a b\xE2\x80\xA6")==0);
 memset(out,'X',sizeof out);
 CHECK(gut_truncate_words_utf8_default("a b c",2,out,5,&n)==GUT_TRUNCATE_NOSPACE);
 CHECK(n==7);
 for(size_t i=0;i<sizeof out;i++)CHECK(out[i]=='X');
 CHECK(gut_truncate_words_utf8_default("a b c",2,NULL,0,&n)==GUT_TRUNCATE_NOSPACE&&n==7);
 CHECK(gut_truncate_words_utf8_default("\xC0\xAF",1,out,sizeof out,&n)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_words_utf8("a b",1,"\xC0\xAF",out,sizeof out,&n)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_words_utf8_default("\xF0\x9F\x98\x80" " b c",2,out,sizeof out,&n)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"\xF0\x9F\x98\x80" " b\xE2\x80\xA6")==0);
 /* All ECMAScript whitespace scalars must separate words consistently. */
 const char *separators[] = {
  "\\x09", "\\x0A", "\\x0B", "\\x0C", "\\x0D", " ",
  "\\xC2\\xA0", "\\xE1\\x9A\\x80", "\\xE2\\x80\\x80",
  "\\xE2\\x80\\x81", "\\xE2\\x80\\x82", "\\xE2\\x80\\x83",
  "\\xE2\\x80\\x84", "\\xE2\\x80\\x85", "\\xE2\\x80\\x86",
  "\\xE2\\x80\\x87", "\\xE2\\x80\\x88", "\\xE2\\x80\\x89",
  "\\xE2\\x80\\x8A", "\\xE2\\x80\\xA8", "\\xE2\\x80\\xA9",
  "\\xE2\\x80\\xAF", "\\xE2\\x81\\x9F", "\\xE3\\x80\\x80",
  "\\xEF\\xBB\\xBF"
 };
 for(size_t i=0;i<sizeof separators/sizeof separators[0];++i){
  char input[64];
  CHECK(snprintf(input,sizeof input,"a%sb c",separators[i])>0);
  CHECK(gut_truncate_words_utf8_default(input,2,out,sizeof out,&n)==GUT_TRUNCATE_OK);
  CHECK(strcmp(out,"a b\\xE2\\x80\\xA6")==0 && n==7);
 }
 /* Invalid suffix and input never mutate output, including size-query mode. */
 memset(out,'R',sizeof out);
 CHECK(gut_truncate_words_utf8("a b c",2,"\\xED\\xA0\\x80",out,sizeof out,&n)==GUT_TRUNCATE_INVALID);
 for(size_t i=0;i<sizeof out;i++)CHECK(out[i]=='R');
 CHECK(gut_truncate_words_utf8_default("\\xE2\\x82",2,NULL,0,&n)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_words_utf8("a b c",2,"!",out,0,&n)==GUT_TRUNCATE_NOSPACE && n==4);
 for(size_t i=0;i<sizeof out;i++)CHECK(out[i]=='R');
 puts("gut_truncate_words passed");return 0;
}
