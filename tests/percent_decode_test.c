#include "gutc/percent_decode.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void) {
    unsigned char out[32];
    const char encoded[] = "A%2fb%00%FF+%7e";
    const unsigned char expected[] = {'A','/','b',0,255,'+','~'};
    assert(gutc_url_percent_decode(encoded, sizeof(encoded)-1, NULL, 0) == 7);
    memset(out, 0xAA, sizeof(out));
    assert(gutc_url_percent_decode(encoded, sizeof(encoded)-1, out, 7) == 7);
    assert(out[0] == 0xAA && out[7] == 0xAA);
    assert(gutc_url_percent_decode(encoded, sizeof(encoded)-1, out, sizeof(out)) == 7);
    assert(memcmp(out, expected, 7) == 0 && out[7] == 0);
    assert(gutc_url_percent_decode("%", 1, out, sizeof(out)) == SIZE_MAX);
    assert(gutc_url_percent_decode("%2", 2, out, sizeof(out)) == SIZE_MAX);
    assert(gutc_url_percent_decode("%GG", 3, out, sizeof(out)) == SIZE_MAX);
    assert(gutc_url_percent_decode("%00%", 4, out, sizeof(out)) == SIZE_MAX);
    assert(gutc_url_percent_decode(NULL, 1, out, sizeof(out)) == SIZE_MAX);
    assert(gutc_url_percent_decode(NULL, 0, out, sizeof(out)) == 0);
    return 0;
}
