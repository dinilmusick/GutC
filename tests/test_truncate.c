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
 CHECK(gut_truncate_utf8("abc",2,"!",out,2,&needed)==GUT_TRUNCATE_NOSPACE);
 CHECK(needed==3);
 CHECK(gut_truncate_utf8("abc",2,"!",NULL,0,&needed)==GUT_TRUNCATE_NOSPACE);
 CHECK(needed==3);
 CHECK(gut_truncate_utf8("abc",2,"!",out,3,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"a!")==0);
 CHECK(gut_truncate_utf8("abc",2,"",out,sizeof out,&needed)==GUT_TRUNCATE_OK);
 CHECK(strcmp(out,"ab")==0 && needed==3);
 CHECK(gut_truncate_utf8("abc",2,NULL,out,sizeof out,NULL)==GUT_TRUNCATE_INVALID);
 puts("gut_truncate regression passed");
 return 0;
}
