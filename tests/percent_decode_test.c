#include "gutc/percent_decode.h"
#include "gutc/url.h"
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

    /* Exhaustively validate every possible byte, including embedded NUL,
     * high-bit bytes, and the RFC3986 literal '+' policy. */
    {
        unsigned char bytes[256], decoded[257];
        char escaped[769];
        size_t escaped_len;
        for (size_t i = 0; i < 256; ++i) bytes[i] = (unsigned char)i;
        escaped_len = gutc_url_percent_encode(bytes, sizeof bytes, escaped, sizeof escaped);
        assert(escaped_len != SIZE_MAX);
        assert(gutc_url_percent_decode(escaped, escaped_len, NULL, 0) == sizeof bytes);
        memset(decoded, 0xA5, sizeof decoded);
        assert(gutc_url_percent_decode(escaped, escaped_len, decoded, sizeof bytes) == sizeof bytes);
        for (size_t i = 0; i < sizeof decoded; ++i) assert(decoded[i] == 0xA5);
        assert(gutc_url_percent_decode(escaped, escaped_len, decoded, sizeof decoded) == sizeof bytes);
        assert(memcmp(decoded, bytes, sizeof bytes) == 0 && decoded[256] == 0);
    }

    /* A malformed % sequence is fail-closed and must never mutate the output.
     * Check all 65,536 possible byte pairs after a '%' escape. */
    {
        char candidate[3] = { '%', 0, 0 };
        unsigned char result[2];
        for (unsigned int hi = 0; hi < 256; ++hi) {
            for (unsigned int lo = 0; lo < 256; ++lo) {
                candidate[1] = (char)hi;
                candidate[2] = (char)lo;
                result[0] = 0xA5;
                result[1] = 0xA5;
                size_t n = gutc_url_percent_decode(candidate, sizeof candidate, result, sizeof result);
                int valid_hi = (hi >= '0' && hi <= '9') || (hi >= 'a' && hi <= 'f') || (hi >= 'A' && hi <= 'F');
                int valid_lo = (lo >= '0' && lo <= '9') || (lo >= 'a' && lo <= 'f') || (lo >= 'A' && lo <= 'F');
                if (valid_hi && valid_lo) {
                    assert(n == 1);
                    assert(result[1] == 0);
                } else {
                    assert(n == SIZE_MAX);
                    assert(result[0] == 0xA5 && result[1] == 0xA5);
                }
            }
        }
    }

    /* Even a late malformed escape cannot trigger a partial write. */
    {
        unsigned char untouched[8];
        memset(untouched, 0xA5, sizeof untouched);
        assert(gutc_url_percent_decode("ok%20bad%GG", 11, untouched, sizeof untouched) == SIZE_MAX);
        for (size_t i = 0; i < sizeof untouched; ++i) assert(untouched[i] == 0xA5);
    }
    return 0;
}
