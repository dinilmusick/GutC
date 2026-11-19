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
 CHECK(strcmp(out,"..")==0);
 CHECK(gut_truncate_utf8("hello",0,"...",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"")==0);
 CHECK(gut_truncate_utf8("hello",5,"...",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"hello")==0);
 CHECK(gut_truncate_utf8("\xC3\xA9\xE2\x98\x83Z",2,"!",out,sizeof out,NULL)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"\xC3\xA9!")==0);
 CHECK(gut_truncate_utf8("\xC0\xAF",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xED\xA0\x80",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xF4\x90\x80\x80",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\xE2\x82",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 /* Truncated UTF-8 must not read past the terminating NUL. */
 CHECK(gut_truncate_utf8("\\xC2",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\\xE2",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 CHECK(gut_truncate_utf8("\\xF0\\x9F",3,"!",out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
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
 CHECK(strcmp(out,"\xC3\xA9")==0 && needed==3);
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
 puts("gut_truncate regression passed");
 return 0;
}
