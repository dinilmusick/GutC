#include "../include/gut_truncate.h"
#include <assert.h>
#include <string.h>
int main(void){
 char out[64];size_t needed=0;
 assert(gut_truncate_utf8("hello",4,"...",out,sizeof out,&needed)==GUT_TRUNCATE_OK&&strcmp(out,"h...")==0);
 assert(gut_truncate_utf8("hello",2,"...",out,sizeof out,0)==GUT_TRUNCATE_OK&&strcmp(out,"..")==0);
 assert(gut_truncate_utf8("hello",0,"...",out,sizeof out,0)==GUT_TRUNCATE_OK&&strcmp(out,"")==0);
 assert(gut_truncate_utf8("hello",5,"...",out,sizeof out,0)==GUT_TRUNCATE_OK&&strcmp(out,"hello")==0);
 assert(gut_truncate_utf8("\xC3\xA9\xE2\x98\x83Z",2,"!",out,sizeof out,0)==GUT_TRUNCATE_OK&&strcmp(out,"\xC3\xA9!")==0);
 assert(gut_truncate_utf8("\xC0\xAF",3,"!",out,sizeof out,0)==GUT_TRUNCATE_INVALID);
 assert(gut_truncate_utf8("abc",2,"!",out,2,&needed)==GUT_TRUNCATE_NOSPACE&&needed==3);
 return 0;
}
