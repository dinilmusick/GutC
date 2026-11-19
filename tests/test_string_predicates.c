#include "../include/gut_string_predicates.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
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
 const char raw[] = {' ', '\t', 'x', ' '};
 assert(gut_is_blank_utf8_n(raw, 2) == 1);
 assert(gut_is_nonblank_utf8_n(raw, 4) == 1);
 assert(gut_is_blank_utf8_n(raw, 4) == 0);
 const char nul[] = {' ', 0, 'x'};
 assert(gut_is_blank_utf8_n(nul, sizeof nul) == -1);
 assert(gut_is_nonblank_utf8_n(nul, sizeof nul) == -1);
 const char truncated[] = {(char)0xE2, (char)0x82, (char)0xAC};
 assert(gut_is_nonblank_utf8_n(truncated, 2) == -1);
 assert(gut_is_nonblank_utf8_n(truncated, 3) == 1);
 assert(gut_is_blank_utf8_n(NULL, 0) == -1);
 assert(gut_is_blank_utf8_n(raw, 0) == 1);
 const char *prefixes[] = {"no", "\xC3\xA9", ""};
 const char *suffixes[] = {"\xF0\x9F\x8C\x8D", "no"};
 assert(gut_starts_with_any_utf8("\xC3\xA9" "clair", prefixes, 3) == 1);
 assert(gut_ends_with_any_utf8("hello \xF0\x9F\x8C\x8D", suffixes, 2) == 1);
 assert(gut_starts_with_any_utf8("abc", NULL, 0) == 0);
 assert(gut_ends_with_any_utf8("abc", NULL, 0) == 0);
 const char *empty[] = {""};
 assert(gut_starts_with_any_utf8("", empty, 1) == 1);
 assert(gut_ends_with_any_utf8("", empty, 1) == 1);
 const char *missing[] = {"xyz"};
 assert(gut_starts_with_any_utf8("abc", missing, 1) == 0);
 assert(gut_ends_with_any_utf8("abc", missing, 1) == 0);
 const char *bad[] = {"\xC0\xAF"};
 assert(gut_starts_with_any_utf8("abc", bad, 1) == -1);
 assert(gut_ends_with_any_utf8("abc", bad, 1) == -1);
 const char *with_null[] = {"abc", NULL};
 assert(gut_starts_with_any_utf8("abc", with_null, 2) == -1);
 assert(gut_ends_with_any_utf8("abc", with_null, 2) == -1);
 assert(gut_starts_with_any_utf8(NULL, empty, 1) == -1);
 assert(gut_ends_with_any_utf8("abc", NULL, 1) == -1);
 assert(gut_starts_with_any_utf8("\xE2\x82", empty, 1) == -1);
 puts("gut_string_predicates passed");
 return 0;
}
