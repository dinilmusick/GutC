#include "../include/gut_truncate.h"
#include <stdio.h>
#include <string.h>

/* CHECK must remain active in Release builds (-DNDEBUG). */
#define CHECK(condition) do { if (!(condition)) { \
 fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #condition); \
 return 1; \
} } while (0)

int main(void) {
 char out[64]; size_t needed = 0;
 CHECK(gut_truncate_utf8("hello",4,"...",out,sizeof out,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"h...")==0 && needed==5);
 CHECK(gut_truncate_utf8("hello",2,"...",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"...")==0);
 CHECK(gut_truncate_utf8("hello",0,"...",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"...")==0);
 CHECK(gut_truncate_utf8("hello",5,"...",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"hello")==0);
 CHECK(gut_truncate_utf8("\xC3\xA9\xE2\x98\x83Z",2,"!",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"\xC3\xA9!")==0);
 CHECK(gut_truncate_utf8("\xC0\xAF",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xED\xA0\x80",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xF4\x90\x80\x80",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xE2\x82",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 /* Truncated UTF-8 must not read past the terminating NUL. */
 CHECK(gut_truncate_utf8("\xC2",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xE2",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xF0\x9F",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("abc",2,"!",out,2,&needed)==GUT_TRUNCATE_NOSPACE);
 CHECK(needed==3);
 CHECK(gut_truncate_utf8("abc",2,"!",NULL,0,&needed)==GUT_TRUNCATE_NOSPACE);
 CHECK(needed==3);
 CHECK(gut_truncate_utf8("abc",2,"!",out,3,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"a!")==0);
 CHECK(gut_truncate_utf8("abc",2,"",out,sizeof out,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"ab")==0 && needed==3);
 CHECK(gut_truncate_utf8("abc",2,NULL,out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("abcdef",4,"\xC3\xA9\xE2\x98\x83",out,sizeof out,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"ab\xC3\xA9\xE2\x98\x83")==0 && needed==8);
 CHECK(gut_truncate_utf8("abcdef",1,"\xC3\xA9\xE2\x98\x83",out,sizeof out,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"\xC3\xA9\xE2\x98\x83")==0 && needed==6);
 CHECK(gut_truncate_utf8("abcdef",4,"\xC0\xAF",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("",0,"!",out,sizeof out,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"")==0 && needed==1);
 CHECK(gut_truncate_utf8("abcdef",4,"!",out,4,&needed)==GUT_TRUNCATE_NOSPACE);
 CHECK(needed==5);
 CHECK(gut_truncate_utf8("abcdef",4,"!",out,5,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"abc!")==0);
 /* Exercise every possible leading byte and every continuation byte at a NUL boundary. */
 for (unsigned int lead = 0x80; lead <= 0xFF; ++lead) {
  char sample[3] = {(char)lead, 0, 0};
  gut_truncate_status status = gut_truncate_utf8(sample, 1, "!", out, sizeof out, NULL);
  CHECK(status == GUT_TRUNCATE_INVALID);
 }
 for (unsigned int continuation = 0; continuation <= 0xFF; ++continuation) {
  char sample[3] = {(char)0xC2, (char)continuation, 0};
  gut_truncate_status status = gut_truncate_utf8(sample, 1, "!", out, sizeof out, NULL);
  CHECK(status == ((continuation >= 0x80 && continuation <= 0xBF) ? GUT_TRUNCATE_OK : GUT_TRUNCATE_INVALID));
 }
 /* Valid UTF-8 boundary scalars must be retained intact. */
 const char *valid_scalars[] = {"\x7F", "\xC2\x80", "\xDF\xBF", "\xE0\xA0\x80",
  "\xED\x9F\xBF", "\xEE\x80\x80", "\xEF\xBF\xBF", "\xF0\x90\x80\x80",
  "\xF4\x8F\xBF\xBF"};
 for (size_t i = 0; i < sizeof valid_scalars / sizeof valid_scalars[0]; ++i) {
  CHECK(gut_truncate_utf8(valid_scalars[i], 1, "!", out, sizeof out, &needed) == GUT_TRUNCATE_OK);
  CHECK(strcmp(out, valid_scalars[i]) == 0);
  CHECK(needed == strlen(valid_scalars[i]) + 1);
 }
 /* Invalid encodings must fail before writing to the output buffer. */
 const char *invalid_scalars[] = {"\xC0\x80", "\xC1\xBF", "\xE0\x9F\xBF",
  "\xED\xA0\x80", "\xF0\x8F\xBF\xBF", "\xF4\x90\x80\x80",
  "\xF5\x80\x80\x80", "\x80", "\xBF", "\xE2\x82"};
 for (size_t i = 0; i < sizeof invalid_scalars / sizeof invalid_scalars[0]; ++i) {
  memset(out, 'Q', sizeof out);
  CHECK(gut_truncate_utf8(invalid_scalars[i], 1, "!", out, sizeof out, &needed) == GUT_TRUNCATE_INVALID);
  for (size_t j = 0; j < sizeof out; ++j) CHECK(out[j] == 'Q');
 }
 memset(out, 'Z', sizeof out);
 CHECK(gut_truncate_utf8("abcdef", 3, "!", out, 2, &needed) == GUT_TRUNCATE_NOSPACE);
 CHECK(needed == 4);
 for (size_t j = 0; j < sizeof out; ++j) CHECK(out[j] == 'Z');
 /* Exhaustively validate two-byte UTF-8 scalar boundaries and preservation. */
 for (unsigned int lead = 0xC2; lead <= 0xDF; ++lead) {
  for (unsigned int continuation = 0x80; continuation <= 0xBF; ++continuation) {
   char sample[3] = {(char)lead, (char)continuation, 0};
   CHECK(gut_truncate_utf8(sample, 1, "!", out, sizeof out, &needed) == GUT_TRUNCATE_OK);
   CHECK(strcmp(out, sample) == 0 && needed == 3);
   CHECK(gut_truncate_utf8(sample, 0, "!", out, sizeof out, &needed) == GUT_TRUNCATE_OK);
   CHECK(strcmp(out, "!") == 0 && needed == 2);
  }
 }
 /* Exhaust all legal three-byte UTF-8 scalar values and truncation boundaries. */
 for (unsigned int lead = 0xE0; lead <= 0xEF; ++lead) {
  for (unsigned int second = 0x80; second <= 0xBF; ++second) {
   for (unsigned int third = 0x80; third <= 0xBF; ++third) {
    char sample[4] = {(char)lead, (char)second, (char)third, 0};
    int valid = !(lead == 0xE0 && second < 0xA0) &&
                !(lead == 0xED && second >= 0xA0);
    memset(out, 'R', sizeof out);
    gut_truncate_status status = gut_truncate_utf8(sample, 1, "!", out, sizeof out, &needed);
    if (valid) {
     CHECK(status == GUT_TRUNCATE_OK);
     CHECK(strcmp(out, sample) == 0 && needed == 4);
     CHECK(gut_truncate_utf8(sample, 0, "!", out, sizeof out, &needed) == GUT_TRUNCATE_OK);
     CHECK(strcmp(out, "!") == 0 && needed == 2);
    } else {
     CHECK(status == GUT_TRUNCATE_INVALID);
     for (size_t j = 0; j < sizeof out; ++j) CHECK(out[j] == 'R');
    }
   }
  }
 }
 /* Four-byte UTF-8 boundary cases include the minimum and maximum planes. */
 const char *valid_four[] = {"\\xF0\\x90\\x80\\x80", "\\xF1\\x80\\x80\\x80",
  "\\xF3\\xBF\\xBF\\xBF", "\\xF4\\x8F\\xBF\\xBF"};
 for (size_t i = 0; i < sizeof valid_four / sizeof valid_four[0]; ++i) {
  CHECK(gut_truncate_utf8(valid_four[i], 1, "!", out, sizeof out, &needed) == GUT_TRUNCATE_OK);
  CHECK(strcmp(out, valid_four[i]) == 0 && needed == 5);
 }
 /* Ensure invalid omission cannot bypass validation even when input fits. */
 memset(out, 'X', sizeof out);
 CHECK(gut_truncate_utf8("a", 8, "\xC0\xAF", out, sizeof out, &needed) == GUT_TRUNCATE_INVALID);
 for (size_t i = 0; i < sizeof out; ++i) CHECK(out[i] == 'X');
 puts("gut_truncate regression passed");
 return 0;
}
