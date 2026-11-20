#include "gutc/model_registry.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct model_entry {const char *id;const char *json;} model_entry;
static const model_entry entries[]={
  {"model.smollm2-135m-instruct", "{\"id\":\"model.smollm2-135m-instruct\",\"name\":\"SmolLM2-135M-Instruct\",\"provider\":\"Hugging Face\",\"version\":\"135M\",\"description\":\"Compact instruction-tuned language model for general text generation.\",\"examples\":[\"summarize text\",\"rewrite text\",\"draft a short response\"],\"avoidWhen\":\"Avoid for high-stakes reasoning, broad factual knowledge, or tasks requiring large context.\",\"whenToUse\":\"Use for short general-purpose text generation when a local model is preferred.\",\"size\":{\"value\":105,\"unit\":\"MB\",\"qualifier\":\"artifact\"},\"capabilities\":[\"text-generation\",\"instruction-following\"],\"input\":\"Natural-language prompt\",\"output\":\"Generated natural-language text\",\"runtime\":\"ModelRuntime adapter\",\"modelUri\":\"HuggingFaceTB/SmolLM2-135M-Instruct\",\"license\":\"Apache-2.0\"}"},
  {"model.cactus-needle3", "{\"id\":\"model.cactus-needle3\",\"name\":\"Needle 3\",\"provider\":\"Cactus Compute\",\"version\":\"3\",\"description\":\"Tiny structured inference model for tool selection, argument extraction, classification, and embeddings.\",\"examples\":[\"select a tool\",\"extract fields\",\"classify an intent\"],\"avoidWhen\":\"Avoid for open-ended long-form generation or complex multi-step reasoning.\",\"whenToUse\":\"Use when natural language must be mapped to a small tool set or structured schema.\",\"size\":{\"min\":8,\"max\":29,\"unit\":\"MB\",\"qualifier\":\"range\"},\"capabilities\":[\"tool-selection\",\"argument-extraction\",\"classification\",\"structured-output\",\"embeddings\"],\"input\":\"Natural-language text with optional tools or extraction schema\",\"output\":\"Structured JSON or embedding vector\",\"runtime\":\"Cactus Needle runtime adapter\",\"modelUri\":\"Cactus-Compute/needle3\"}"},
  {"model.cactus-whistle", "{\"id\":\"model.cactus-whistle\",\"name\":\"Whistle\",\"provider\":\"Cactus Compute\",\"version\":\"1\",\"description\":\"Compact CPU speech-recognition model for on-device transcription.\",\"examples\":[\"transcribe a voice note\",\"transcribe a command\"],\"avoidWhen\":\"Avoid when audio understanding beyond transcription is required.\",\"whenToUse\":\"Use for local speech-to-text when low resource use and privacy matter.\",\"size\":{\"value\":16.9,\"unit\":\"MB\",\"qualifier\":\"artifact\"},\"capabilities\":[\"speech-to-text\",\"on-device\",\"cpu\"],\"input\":\"Audio bytes\",\"output\":\"Transcribed natural-language text\",\"runtime\":\"Cactus Needle runtime adapter\",\"modelUri\":\"Cactus-Compute/whistle\"}"},
  {"model.ettin-reranker-17m", "{\"id\":\"model.ettin-reranker-17m\",\"name\":\"Ettin-Reranker-17M\",\"provider\":\"Hugging Face\",\"version\":\"17\",\"description\":\"Very small relevance model for ranking candidate text.\",\"examples\":[\"rank search results\",\"order retrieved documents by relevance\"],\"whenToUse\":\"Use for cross-encoder reranking when candidate texts must be sorted by semantic relevance.\",\"avoidWhen\":\"Avoid for text generation, embeddings, or transcription.\",\"size\":{\"value\":17000000,\"unit\":\"parameters\",\"qualifier\":\"parameters\"},\"capabilities\":[\"reranking\",\"relevance-scoring\"],\"input\":\"Query and candidate texts\",\"output\":\"Candidate relevance scores\",\"runtime\":\"ModelRuntime adapter\",\"modelUri\":\"cross-encoder/ettin-reranker-17m-v1\",\"license\":\"Apache-2.0\"}"},
  {"model.ettin-reranker-32m", "{\"id\":\"model.ettin-reranker-32m\",\"name\":\"Ettin-Reranker-32M\",\"provider\":\"Hugging Face\",\"version\":\"32\",\"description\":\"Small relevance model for ranking candidate text.\",\"examples\":[\"rank search results\",\"order retrieved documents by relevance\"],\"whenToUse\":\"Use for cross-encoder reranking when candidate texts must be sorted by semantic relevance.\",\"avoidWhen\":\"Avoid for text generation, embeddings, or transcription.\",\"size\":{\"value\":32000000,\"unit\":\"parameters\",\"qualifier\":\"parameters\"},\"capabilities\":[\"reranking\",\"relevance-scoring\"],\"input\":\"Query and candidate texts\",\"output\":\"Candidate relevance scores\",\"runtime\":\"ModelRuntime adapter\",\"modelUri\":\"cross-encoder/ettin-reranker-32m-v1\",\"license\":\"Apache-2.0\"}"},
  {"model.ettin-reranker-68m", "{\"id\":\"model.ettin-reranker-68m\",\"name\":\"Ettin-Reranker-68M\",\"provider\":\"Hugging Face\",\"version\":\"68\",\"description\":\"Compact relevance model for higher-quality candidate ranking.\",\"examples\":[\"rank search results\",\"order retrieved documents by relevance\"],\"whenToUse\":\"Use for cross-encoder reranking when candidate texts must be sorted by semantic relevance.\",\"avoidWhen\":\"Avoid for text generation, embeddings, or transcription.\",\"size\":{\"value\":68000000,\"unit\":\"parameters\",\"qualifier\":\"parameters\"},\"capabilities\":[\"reranking\",\"relevance-scoring\"],\"input\":\"Query and candidate texts\",\"output\":\"Candidate relevance scores\",\"runtime\":\"ModelRuntime adapter\",\"modelUri\":\"cross-encoder/ettin-reranker-68m-v1\",\"license\":\"Apache-2.0\"}"},
  {"model.ettin-reranker-150m", "{\"id\":\"model.ettin-reranker-150m\",\"name\":\"Ettin-Reranker-150M\",\"provider\":\"Hugging Face\",\"version\":\"150\",\"description\":\"Larger compact relevance model for candidate ranking.\",\"examples\":[\"rank search results\",\"order retrieved documents by relevance\"],\"whenToUse\":\"Use for cross-encoder reranking when candidate texts must be sorted by semantic relevance.\",\"avoidWhen\":\"Avoid for text generation, embeddings, or transcription.\",\"size\":{\"value\":150000000,\"unit\":\"parameters\",\"qualifier\":\"parameters\"},\"capabilities\":[\"reranking\",\"relevance-scoring\"],\"input\":\"Query and candidate texts\",\"output\":\"Candidate relevance scores\",\"runtime\":\"ModelRuntime adapter\",\"modelUri\":\"cross-encoder/ettin-reranker-150m-v1\",\"license\":\"Apache-2.0\"}"},
  {"model.minilm-l6-v2", "{\"id\":\"model.minilm-l6-v2\",\"name\":\"MiniLM-L6-v2\",\"provider\":\"Sentence Transformers\",\"version\":\"L6-v2\",\"description\":\"Compact sentence embedding model for semantic similarity.\",\"examples\":[\"compare two texts\",\"match a query to a short description\"],\"avoidWhen\":\"Avoid when generation, extraction, or tool calling is required.\",\"whenToUse\":\"Use for lightweight semantic similarity and embedding-based matching.\",\"size\":{\"value\":22700000,\"unit\":\"parameters\",\"qualifier\":\"parameters\"},\"capabilities\":[\"embeddings\",\"semantic-similarity\"],\"input\":\"Two text strings\",\"output\":\"Similarity score\",\"runtime\":\"ModelRuntime adapter\",\"modelUri\":\"sentence-transformers/all-MiniLM-L6-v2\",\"license\":\"Apache-2.0\"}"},
};
#define MODEL_COUNT (sizeof(entries)/sizeof(entries[0]))

size_t gutc_model_registry_count(void) {return MODEL_COUNT;}
gutc_status gutc_model_registry_at(size_t index,gutc_json_dom **output) {
  const char *json;
  if(!output)return GUTC_INVALID_ARGUMENT;
  *output=NULL;
  if(index>=MODEL_COUNT)return GUTC_RANGE_ERROR;
  json=entries[index].json;
  return gutc_json_dom_parse((const uint8_t*)json,strlen(json),
                              64,10000,output,NULL);
}
gutc_status gutc_model_registry_lookup(const char *id,size_t len,
                                       gutc_json_dom **output,int *found) {
  size_t i;
  if(!output||!found||(!id&&len))return GUTC_INVALID_ARGUMENT;
  *output=NULL;*found=0;
  for(i=0;i<MODEL_COUNT;i++) {
    size_t n=strlen(entries[i].id);
    if(n==len && (!len||memcmp(id,entries[i].id,len)==0)) {
      gutc_status rc=gutc_model_registry_at(i,output);
      if(rc==GUTC_OK)*found=1;
      return rc;
    }
  }
  return GUTC_OK;
}
gutc_status gutc_model_registry_list(gutc_json_dom **output) {
  size_t i,total=2,pos=0;
  char *buf;
  gutc_status rc;
  if(!output)return GUTC_INVALID_ARGUMENT;
  *output=NULL;
  for(i=0;i<MODEL_COUNT;i++) {
    size_t n=strlen(entries[i].json);
    if(n>SIZE_MAX-total-1)return GUTC_OVERFLOW;
    total+=n+1;
  }
  if(total>1048576u)return GUTC_RANGE_ERROR;
  buf=(char*)malloc(total+1);
  if(!buf)return GUTC_NO_MEMORY;
  buf[pos++]='[';
  for(i=0;i<MODEL_COUNT;i++) {
    size_t n=strlen(entries[i].json);
    if(i)buf[pos++]=',';
    memcpy(buf+pos,entries[i].json,n);pos+=n;
  }
  buf[pos++]=']';buf[pos]='\0';
  rc=gutc_json_dom_parse((const uint8_t*)buf,pos,128,100000,output,NULL);
  free(buf);return rc;
}
