#include "gutc/url.h"
#include <stdint.h>

static int unreserved(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '.' ||
           c == '_' || c == '~';
}

size_t gutc_url_percent_encode(const unsigned char *input, size_t length,
                               char *out, size_t capacity) {
    static const char hex[] = "0123456789ABCDEF";
    size_t required = 0, i, at = 0;
    if (!input && length) return SIZE_MAX;
    for (i = 0; i < length; ++i) {
        size_t increment = unreserved(input[i]) ? 1u : 3u;
        if (required > SIZE_MAX - increment) return SIZE_MAX;
        required += increment;
    }
    if (!out || capacity <= required) return required;
    for (i = 0; i < length; ++i) {
        unsigned char c = input[i];
        if (unreserved(c)) out[at++] = (char)c;
        else {
            out[at++] = '%';
            out[at++] = hex[c >> 4];
            out[at++] = hex[c & 15];
        }
    }
    out[at] = '\0';
    return required;
}
