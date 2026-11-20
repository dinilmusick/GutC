#ifndef GUTC_SEMANTIC_CACHE_H
#define GUTC_SEMANTIC_CACHE_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native C11 analogue of GutTS SemanticInputCache (insertion-order LRU).
 * Explicit ownership contract:
 * - init allocates internal slot storage, destroy frees slots and values.
 * - set takes ownership of owned_value on SUCCESS only, never on failure;
 *   overwriting a key frees its former value. A max_entries=0 instance
 *   immediately discards successful inserts, matching upstream behavior.
 * - get returns a BORROWED pointer. It must NOT be freed, and can become
 *   invalid after mutation, eviction, clear, or destroy. A get also moves
 *   the entry to the most-recently-used end, as in GutTS.
 * - keys are byte spans of UTF-8 with explicit lengths (embedded NUL okay).
 * - Every instance is reentrant but NOT synchronized; concurrent access to
 *   the same instance requires external locking. No global state.
 */
typedef struct gutc_semantic_cache_entry {
  char *key;
  size_t length;
  gutc_json_dom *value;
} gutc_semantic_cache_entry;
typedef struct gutc_semantic_cache {
  gutc_semantic_cache_entry *entries;
  size_t size;
  size_t capacity;
} gutc_semantic_cache;

gutc_status gutc_semantic_cache_init(gutc_semantic_cache *cache,size_t max_entries);
void gutc_semantic_cache_clear(gutc_semantic_cache *cache);
void gutc_semantic_cache_destroy(gutc_semantic_cache *cache);
size_t gutc_semantic_cache_size(const gutc_semantic_cache *cache);
gutc_status gutc_semantic_cache_set(gutc_semantic_cache *cache,
  const char *key,size_t length,gutc_json_dom *owned_value);
gutc_status gutc_semantic_cache_get(gutc_semantic_cache *cache,
  const char *key,size_t length,const gutc_json_dom **borrowed,int *found);
#ifdef __cplusplus
}
#endif
#endif
