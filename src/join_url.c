#include "gutc/join_url.h"
#include <stdint.h>
#include <string.h>

size_t gutc_join_url(const char *base, const char *const *segments,
                     size_t count, char *out, size_t capacity) {
    size_t required = 0, written = 0, i;
    if (!base || (count && !segments)) return SIZE_MAX;
    for (i = 0; i <= count; ++i) {
        const char *part = i == 0 ? base : segments[i - 1];
        size_t start = 0, end, n;
        if (!part) return SIZE_MAX;
        end = strlen(part);
        if (i == 0) {
            while (end && part[end - 1] == '/') --end;
        } else {
            while (start < end && part[start] == '/') ++start;
            while (end > start && part[end - 1] == '/') --end;
        }
        n = end - start;
        if (!n) continue;
        if (required && required == SIZE_MAX) return SIZE_MAX;
        if (n > SIZE_MAX - required - (required ? 1u : 0u)) return SIZE_MAX;
        required += n + (required ? 1u : 0u);
    }
    if (!out || capacity <= required) return required;
    for (i = 0; i <= count; ++i) {
        const char *part = i == 0 ? base : segments[i - 1];
        size_t start = 0, end = strlen(part), n;
        if (i == 0) {
            while (end && part[end - 1] == '/') --end;
        } else {
            while (start < end && part[start] == '/') ++start;
            while (end > start && part[end - 1] == '/') --end;
        }
        n = end - start;
        if (!n) continue;
        if (written) out[written++] = '/';
        memcpy(out + written, part + start, n);
        written += n;
    }
    out[written] = '\0';
    return required;
}
