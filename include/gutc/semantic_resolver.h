#ifndef GUTC_SEMANTIC_RESOLVER_H
#define GUTC_SEMANTIC_RESOLVER_H
#include "semantic_rank.h"
#include "semantic_deterministic.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Native, synchronous core of pinned GutTS resolveSemanticInput().
 * Accepts caller-borrowed JSON request matching public SemanticInputRequest
 * fields: input, schema, context, candidates, policy, preserveRawInput,
 * minCandidateSimilarity and minCandidateMargin. Returns owned JSON DOM
 * SemanticInputResolution, caller frees with gutc_json_dom_free().
 *
 * Deterministic matching runs locally. For non-deterministic paths, real
 * MiniLM/Ettin ranking and Needle3 structured completion MUST be supplied
 * by the host. Without suitable host callbacks returns GUTC_UNSUPPORTED
 * and *output=NULL; NO fake scores or synthetic completion responses.
 *
 * The host completion is called with GutTS-compatible prompt and target
 * schema. Host owns all model weights/runtime/network. Callback returns a
 * heap-owned JSON DOM result, transferred to resolver and freed here.
 * All request/input pointers are borrowed and valid during callback only.
 * Synchronous C core is NOT exact JS Promise, browser AbortSignal,
 * provenance timestamp, onEvent/onProgress or cache semantics. Separate
 * host adapters must implement those effects. Regex, Unicode/locale
 * divergences explicitly fail or follow existing native contract limits.
 */
typedef gutc_status (*gutc_semantic_complete)(
  void *context,const char *prompt,size_t prompt_len,
  const gutc_json_dom *schema,gutc_json_dom **model_output);
typedef struct gutc_semantic_resolver_host {
  void *context;
  gutc_semantic_score_adapter ranking;
  gutc_semantic_complete complete;
  int (*is_aborted)(void *context);
} gutc_semantic_resolver_host;

gutc_status gutc_resolve_semantic_input(
  const gutc_json_dom *request,
  const gutc_semantic_resolver_host *host,
  gutc_json_dom **resolution);
#ifdef __cplusplus
}
#endif
#endif
