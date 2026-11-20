#ifndef GUTC_MODEL_OPERATIONS_H
#define GUTC_MODEL_OPERATIONS_H
#include "model_runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Typed operation layer for the eight pinned GutTS model implementations:
 * Needle3, SmolLM2, Whistle, Ettin 17/32/68/150M, MiniLM.
 *
 * NO model is bundled, downloaded or silently simulated. Callers supply
 * actual ModelRuntime callbacks; without callbacks the API returns
 * GUTC_UNSUPPORTED. The host performs inference, C validates input/output
 * contracts and manages load/use/unload via ModelResidency.
 *
 * Typed input JSON DOM: complete -> {prompt:string,tools?:array,schema?:any};
 * embed -> {input:string}; generate -> {prompt:string,maxNewTokens?:integer};
 * transcribe -> {audio:array(0..255),language?:string}, host reconstructs
 * Uint8Array from byte array; rerank -> {query:string,candidates:string[]};
 * similarity -> {left:string,right:string}.
 * On success output is heap-owned JSON DOM; free with gutc_json_dom_free.
 * Needle3.complete returns arbitrary JSON; embed float array; generate and
 * transcribe string; rerank numeric array matching candidates length; MiniLM
 * similarity finite number. Missing/invalid results FAIL CLOSED.
 */
typedef enum gutc_model_family {
 GUTC_MODEL_NEEDLE3,
 GUTC_MODEL_SMOLLM2,
 GUTC_MODEL_WHISTLE,
 GUTC_MODEL_ETTIN17,
 GUTC_MODEL_ETTIN32,
 GUTC_MODEL_ETTIN68,
 GUTC_MODEL_ETTIN150,
 GUTC_MODEL_MINILM,
 GUTC_MODEL_FAMILY_COUNT
} gutc_model_family;
typedef struct gutc_model_client {
 gutc_model_family family;
 gutc_model_library library;
} gutc_model_client;
const char *gutc_model_family_name(gutc_model_family family);
const char *gutc_model_family_id(gutc_model_family family);
gutc_status gutc_model_client_init(gutc_model_client *client,
 gutc_model_family family,const gutc_model_runtime_host *host);
gutc_status gutc_model_client_call(gutc_model_client *client,
 const char *operation,const gutc_json_dom *input,gutc_json_dom **output);
gutc_status gutc_model_client_destroy(gutc_model_client *client);
#ifdef __cplusplus
}
#endif
#endif
