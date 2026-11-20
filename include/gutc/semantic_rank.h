#ifndef GUTC_SEMANTIC_RANK_H
#define GUTC_SEMANTIC_RANK_H
#include "json_dom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Portable selection/sort phase of GutTS rankSemanticInputCandidates.
 * MiniLM similarity and Ettin relevance must be produced by REAL caller
 * model runtimes; this library never generates fabricated scores.
 * Policies route "fast" to 17M, "balanced" to 32M, "accurate" to 68M.
 * String views and model contexts are borrowed; indices are caller-owned.
 * All scores must be finite. ICU localeCompare ties on non-simple-ASCII
 * strings are explicitly unsupported without an ICU host comparator.
 * The caller must supply valid UTF-8 candidate strings and read-only
 * memory. Output is not written on error or insufficient capacity. */
typedef enum gutc_semantic_ranking_policy {
  GUTC_SEMANTIC_FAST,GUTC_SEMANTIC_BALANCED,GUTC_SEMANTIC_ACCURATE
} gutc_semantic_ranking_policy;
typedef gutc_status (*gutc_similarity_score)(
  void *context,const char *input,size_t input_length,
  gutc_json_text candidate,double *score);
typedef gutc_status (*gutc_relevance_scores)(
  void *context,const char *input,size_t input_length,
  const gutc_json_text *candidates,size_t count,
  gutc_semantic_ranking_policy policy,double *out_scores);
typedef struct gutc_semantic_score_adapter {
  void *context;
  gutc_similarity_score similarity;
  gutc_relevance_scores rerank;
} gutc_semantic_score_adapter;
gutc_status gutc_semantic_rank_order(const gutc_json_text *candidates,
                                     const double *similarity,const double *relevance,
                                     size_t count,size_t *order,size_t capacity,size_t *needed);
gutc_status gutc_semantic_rank_with_host(const char *input,size_t input_length,
                                         const gutc_json_text *candidates,size_t count,
                                         gutc_semantic_ranking_policy policy,
                                         const gutc_semantic_score_adapter *adapter,
                                         size_t *order,size_t capacity,size_t *needed);
#ifdef __cplusplus
}
#endif
#endif
