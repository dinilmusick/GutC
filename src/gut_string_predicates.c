#include "../include/gut_string_predicates.h"
#include <stdint.h>
#include <string.h>
static int whitespace(uint32_t c) {
 return (c >= 9 && c <= 13) || c == 32 || c == 0xA0 || c == 0x1680 ||
        (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
        c == 0x202F || c == 0x205F || c == 0x3000 || c == 0xFEFF;
}
int gut_is_blank_utf8_n(const char *value, size_t length) {
 if (!value) return -1;
 const unsigned char *s = (const unsigned char *)value;
 size_t i = 0;
 int blank = 1;
 while (i < length) {
  unsigned char first = s[i];
  uint32_t cp;
  size_t width;
  if (first == 0) return -1;
  if (first < 0x80) { cp = first; width = 1; }
  else if (first >= 0xC2 && first <= 0xDF) { cp = first & 0x1F; width = 2; }
  else if (first >= 0xE0 && first <= 0xEF) { cp = first & 0x0F; width = 3; }
  else if (first >= 0xF0 && first <= 0xF4) { cp = first & 0x07; width = 4; }
  else return -1;
  if (width > length - i) return -1;
  for (size_t j = 1; j < width; ++j) {
   if ((s[i+j] & 0xC0) != 0x80) return -1;
   cp = (cp << 6) | (s[i+j] & 0x3F);
  }
  if ((width == 2 && cp < 0x80) || (width == 3 && cp < 0x800) ||
      (width == 4 && cp < 0x10000) || cp > 0x10FFFF ||
      (cp >= 0xD800 && cp <= 0xDFFF)) return -1;
  if (!whitespace(cp)) blank = 0;
  i += width;
 }
 return blank;
}
int gut_is_nonblank_utf8_n(const char *value, size_t length) {
 int result = gut_is_blank_utf8_n(value, length);
 return result < 0 ? -1 : !result;
}
int gut_is_blank_utf8(const char *value) {
 return value ? gut_is_blank_utf8_n(value, strlen(value)) : -1;
}
int gut_is_nonblank_utf8(const char *value) {
 return value ? gut_is_nonblank_utf8_n(value, strlen(value)) : -1;
}

/* A UTF-8 codepoint is never a proper byte-prefix of another valid codepoint.
 * Thus byte comparisons of validated strings preserve JS startsWith/endsWith
 * semantics without locale-dependent decoding or allocation. */
static int match_any(const char *value, const char *const *candidates,
                     size_t count, int from_end) {
 if (!value || (count && !candidates)) return -1;
 size_t length = strlen(value);
 if (gut_is_blank_utf8_n(value, length) < 0) return -1;
 int found = 0;
 for (size_t i = 0; i < count; ++i) {
  if (!candidates[i]) return -1;
  size_t n = strlen(candidates[i]);
  if (gut_is_blank_utf8_n(candidates[i], n) < 0) return -1;
  if (n <= length &&
      memcmp(value + (from_end ? length - n : 0), candidates[i], n) == 0)
   found = 1;
 }
 return found;
}
int gut_starts_with_any_utf8(const char *value, const char *const *prefixes, size_t count) {
 return match_any(value, prefixes, count, 0);
}
int gut_ends_with_any_utf8(const char *value, const char *const *suffixes, size_t count) {
 return match_any(value, suffixes, count, 1);
}

/* Bounded candidate spans permit embedded NUL rejection and nonterminated input. */
static int match_any_n(const char *value, size_t length,
                       const char *const *candidates, const size_t *sizes,
                       size_t count, int from_end) {
 if (!value || (count && (!candidates || !sizes))) return -1;
 if (gut_is_blank_utf8_n(value, length) < 0) return -1;
 int found = 0;
 for (size_t i = 0; i < count; ++i) {
  if (!candidates[i] || gut_is_blank_utf8_n(candidates[i], sizes[i]) < 0) return -1;
  if (sizes[i] <= length &&
      memcmp(value + (from_end ? length - sizes[i] : 0), candidates[i], sizes[i]) == 0)
   found = 1;
 }
 return found;
}
int gut_starts_with_any_utf8_n(const char *value, size_t length,
                               const char *const *prefixes, const size_t *sizes, size_t count) {
 return match_any_n(value, length, prefixes, sizes, count, 0);
}
int gut_ends_with_any_utf8_n(const char *value, size_t length,
                             const char *const *suffixes, const size_t *sizes, size_t count) {
 return match_any_n(value, length, suffixes, sizes, count, 1);
}
