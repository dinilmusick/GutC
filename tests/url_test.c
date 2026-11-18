#include "gutc/url.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int unreserved(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '.' ||
           c == '_' || c == '~';
}

int main(void) {
    char out[1024];
    const unsigned char raw[] = {'A', ' ', '/', '?', 0, 0xff, '~', '-', '_', '.'};
    unsigned char all[256];
    size_t expected = 0, i, pos = 0;
    static const char hex[] = "0123456789ABCDEF";
    assert(gutc_url_percent_encode(raw, sizeof raw, NULL, 0) == 20);
    assert(gutc_url_percent_encode(raw, sizeof raw, out, sizeof out) == 20);
    assert(strcmp(out, "A%20%2F%3F%00%FF~-_.") == 0);
    strcpy(out, "unchanged");
    assert(gutc_url_percent_encode(raw, sizeof raw, out, 5) == 20);
    assert(strcmp(out, "unchanged") == 0);
    assert(gutc_url_percent_encode(NULL, 0, out, sizeof out) == 0);
    assert(strcmp(out, "") == 0);
    assert(gutc_url_percent_encode(NULL, 1, out, sizeof out) == SIZE_MAX);

    for (i = 0; i < sizeof all; ++i) {
        all[i] = (unsigned char)i;
        expected += unreserved(all[i]) ? 1u : 3u;
    }
    assert(gutc_url_percent_encode(all, sizeof all, NULL, 0) == expected);
    memset(out, 0x5a, sizeof out);
    assert(gutc_url_percent_encode(all, sizeof all, out, expected) == expected);
    for (i = 0; i < sizeof out; ++i) assert(out[i] == 'Z');
    assert(gutc_url_percent_encode(all, sizeof all, out, expected + 1) == expected);
    for (i = 0; i < sizeof all; ++i) {
        unsigned char c = all[i];
        if (unreserved(c)) assert(out[pos++] == (char)c);
        else {
            assert(out[pos++] == '%');
            assert(out[pos++] == hex[c >> 4]);
            assert(out[pos++] == hex[c & 15]);
        }
    }
    assert(pos == expected);
    assert(out[pos] == '\0');
    return 0;
}
