#ifndef GUTC_TRUSTED_API_H
#define GUTC_TRUSTED_API_H
#include <stddef.h>
/* Returns 0 on success, -1 for invalid input, -2 for insufficient output.
 * required receives the necessary capacity including NUL on valid input.
 * Never partially writes output. Caller owns output storage. */
int gutc_resolve_trusted_api_url(const char *origin,const char *path,
                                char *output,size_t capacity,size_t *required);
#endif
