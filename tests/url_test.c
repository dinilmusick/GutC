#include "gutc/url.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    char out[128];
    const unsigned char raw[] = {'A', ' ', '/', '?', 0, 0xff, '~', '-', '_', '.'};
    assert(gutc_url_percent_encode(raw, sizeof raw, NULL, 0) == 20);
    assert(gutc_url_percent_encode(raw, sizeof raw, out, sizeof out) == 20);
    assert(strcmp(out, "A%20%2F%3F%00%FF~-_.") == 0);
    strcpy(out, "unchanged");
    assert(gutc_url_percent_encode(raw, sizeof raw, out, 5) == 20);
    assert(strcmp(out, "unchanged") == 0);
    assert(gutc_url_percent_encode(NULL, 0, out, sizeof out) == 0);
    assert(strcmp(out, "") == 0);
    assert(gutc_url_percent_encode(NULL, 1, out, sizeof out) == SIZE_MAX);
    return 0;
}
