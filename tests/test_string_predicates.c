#include "../include/gut_string_predicates.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 const char *spaces[] = {"", " \t\r\n", "\xC2\xA0", "\xE1\x9A\x80", "\xE2\x80\x83", "\xE2\x80\xAF", "\xE3\x80\x80", "\xEF\xBB\xBF"};
 for (size_t i = 0; i < sizeof spaces / sizeof spaces[0]; ++i) {
  assert(gut_is_blank_utf8(spaces[i]) == 1);
  assert(gut_is_nonblank_utf8(spaces[i]) == 0);
 }
 assert(gut_is_blank_utf8(" \xC2\xA0 x") == 0);
 assert(gut_is_nonblank_utf8("x") == 1);
 assert(gut_is_blank_utf8("\xC2\x85") == 0);
 assert(gut_is_blank_utf8("\xE2\x80\x8B") == 0);
 assert(gut_is_blank_utf8("\xC0\xAF") == -1);
 assert(gut_is_blank_utf8("\xED\xA0\x80") == -1);
 assert(gut_is_blank_utf8("\xF4\x90\x80\x80") == -1);
 assert(gut_is_blank_utf8("\xE2\x82") == -1);
 assert(gut_is_blank_utf8(NULL) == -1);
 assert(gut_is_nonblank_utf8("\xE2\x82") == -1);
 puts("gut_string_predicates passed");
 return 0;
}
