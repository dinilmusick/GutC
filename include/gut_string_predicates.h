#ifndef GUT_STRING_PREDICATES_H
#define GUT_STRING_PREDICATES_H
#ifdef __cplusplus
extern "C" {
#endif
/* Matches JavaScript String.trim() whitespace for valid UTF-8 input.
 * Returns -1 for NULL or malformed UTF-8, 0 for nonblank, 1 for blank. */
int gut_is_blank_utf8(const char *value);
/* Returns -1 for invalid input, 0 for blank, 1 for nonblank. */
int gut_is_nonblank_utf8(const char *value);
#ifdef __cplusplus
}
#endif
#endif
