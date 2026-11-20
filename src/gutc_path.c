#include "gutc_path.h"
#include <stdlib.h>
#include <string.h>
static int hexval(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
static int forbidden(unsigned char c) {
  return c == '\\' || c == '?' || c == '#' || c < 32 || c == 127;
}
static int validate_segment(const char *s, size_t len) {
  char *a = (char *)malloc(len + 1);
  char *b = (char *)malloc(len + 1);
  if (!a || !b) { free(a); free(b); return 0; }
  memcpy(a, s, len); a[len] = 0;
  size_t n = len;
  for (int depth = 0; depth < 8; depth++) {
    size_t w = 0;
    for (size_t i = 0; i < n; i++) {
      unsigned char c = (unsigned char)a[i];
      if (c == '%') {
        if (i + 2 >= n || hexval((unsigned char)a[i+1]) < 0 || hexval((unsigned char)a[i+2]) < 0) goto fail;
        c = (unsigned char)(hexval((unsigned char)a[i+1])*16 + hexval((unsigned char)a[i+2]));
        i += 2;
      }
      if (forbidden(c) || c == '/') goto fail;
      b[w++] = (char)c;
    }
    if ((w == 1 && b[0] == '.') || (w == 2 && b[0] == '.' && b[1] == '.')) goto fail;
    int has_percent = 0;
    for (size_t i = 0; i < w; i++) if (b[i] == '%') has_percent = 1;
    if (!has_percent) { free(a); free(b); return 1; }
    char *tmp = a; a = b; b = tmp; n = w;
  }
fail:
  free(a); free(b); return 0;
}
gutc_path_result gutc_normalize_storage_path(const char *input, char *output, size_t capacity) {
  if (!output || !capacity) return GUTC_PATH_OVERFLOW;
  output[0] = 0;
  if (!input) return GUTC_PATH_INVALID;
  size_t n = strlen(input), written = 0;
  for (size_t i = 0; i < n;) {
    while (i < n && input[i] == '/') i++;
    if (i == n) break;
    size_t start = i;
    while (i < n && input[i] != '/') i++;
    size_t len = i - start;
    if (!validate_segment(input + start, len)) { output[0] = 0; return GUTC_PATH_INVALID; }
    if (len > capacity - 1 - written || (written && written == capacity - 1)) { output[0] = 0; return GUTC_PATH_OVERFLOW; }
    if (written) output[written++] = '/';
    memcpy(output + written, input + start, len); written += len; output[written] = 0;
  }
  return GUTC_PATH_OK;
}
