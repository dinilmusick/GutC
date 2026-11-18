#include "gutc/join_url.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void) {
    char out[128];
    const char *parts[] = {"/users/", "//42//", "///", "?x=1"};
    const char *empty[] = {"///", ""};
    size_t n = gutc_join_url("https://example.com///", parts, 4, NULL, 0);
    assert(n == strlen("https://example.com/users/42/?x=1"));
    memset(out, 'X', sizeof out);
    assert(gutc_join_url("https://example.com///", parts, 4, out, n) == n);
    for (size_t i = 0; i < sizeof out; ++i) assert(out[i] == 'X');
    assert(gutc_join_url("https://example.com///", parts, 4, out, n + 1) == n);
    assert(strcmp(out, "https://example.com/users/42/?x=1") == 0);
    assert(gutc_join_url("///", empty, 2, out, sizeof out) == 0 && strcmp(out, "") == 0);
    assert(gutc_join_url("base", NULL, 0, out, sizeof out) == 4 && strcmp(out, "base") == 0);
    assert(gutc_join_url(NULL, NULL, 0, out, sizeof out) == SIZE_MAX);
    assert(gutc_join_url("x", NULL, 1, out, sizeof out) == SIZE_MAX);
    return 0;
}
