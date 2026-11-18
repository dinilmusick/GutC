#include "gutc/percent_decode.h"
#include <stdint.h>
static int hex_value(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
size_t gutc_url_percent_decode(const char *input, size_t length,
                               unsigned char *out, size_t capacity) {
    size_t i = 0, count = 0, at = 0;
    if (!input && length) return SIZE_MAX;
    while (i < length) {
        if (input[i] == '%') {
            if (length - i < 3 || hex_value((unsigned char)input[i+1]) < 0 ||
                hex_value((unsigned char)input[i+2]) < 0) return SIZE_MAX;
            i += 3;
        } else ++i;
        if (count == SIZE_MAX) return SIZE_MAX;
        ++count;
    }
    if (!out || capacity <= count) return count;
    i = 0;
    while (i < length) {
        if (input[i] == '%') {
            int hi = hex_value((unsigned char)input[i+1]);
            int lo = hex_value((unsigned char)input[i+2]);
            out[at++] = (unsigned char)((hi << 4) | lo);
            i += 3;
        } else out[at++] = (unsigned char)input[i++];
    }
    out[at] = 0;
    return count;
}
