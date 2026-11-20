#include "gutc/semantic_rank.h"
#include "gutc/gutc.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define RANK_LIMIT 256u

static int simple_ascii(gutc_json_text text) {
  size_t i;
  for(i=0;i<text.length;i++){
    unsigned char c=(unsigned char)text.bytes[i];
    if(c<'a'||c>'z')return 0;
  }
  return 1;
}
static int cmp(size_t a,size_t b,const gutc_json_text *values,
               const double *similarity,const double *relevance,
               int *unsupported) {
  size_t n;
  if(relevance[a]>relevance[b])return -1;
  if(relevance[a]<relevance[b])return 1;
  if(similarity[a]>similarity[b])return -1;
  if(similarity[a]<similarity[b])return 1;
  /* Default JS localeCompare requires ICU for punctuation/Unicode/case
   * collation; restrict tie comparison to plain ASCII a-z without guessing. */
  if(!simple_ascii(values[a])||!simple_ascii(values[b])){
    *unsupported=1;return 0;
  }
  n=values[a].length<values[b].length?values[a].length:values[b].length;
  if(n){
    int c=memcmp(values[a].bytes,values[b].bytes,n);
    if(c<0)return -1;
    if(c>0)return 1;
  }
  if(values[a].length<values[b].length)return -1;
  if(values[a].length>values[b].length)return 1;
  return 0;
}
gutc_status gutc_semantic_rank_order(const gutc_json_text *candidates,
                                     const double *similarity,const double *relevance,
                                     size_t count,size_t *order,size_t capacity,size_t *needed) {
  size_t i,j,cp;
  size_t sorted[RANK_LIMIT];
  int unsupported=0;
  if(!needed||count>RANK_LIMIT||(count&&(!candidates||!similarity||!relevance))||
     (!order&&capacity))return GUTC_INVALID_ARGUMENT;
  for(i=0;i<count;i++){
    const gutc_json_text s=candidates[i];
    if((!s.bytes&&s.length)||!isfinite(similarity[i])||!isfinite(relevance[i]))
      return GUTC_INVALID_ARGUMENT;
    if(gutc_utf8_validate((const uint8_t*)(s.length?s.bytes:""),s.length,&cp)!=GUTC_OK)
      return GUTC_INVALID_ENCODING;
    sorted[i]=i;
  }
  for(i=1;i<count;i++){
    size_t candidate=sorted[i];
    j=i;
    while(j){
      int order_cmp=cmp(candidate,sorted[j-1],candidates,similarity,relevance,&unsupported);
      if(unsupported)return GUTC_UNSUPPORTED;
      if(order_cmp>=0)break;
      sorted[j]=sorted[j-1];
      j--;
    }
    sorted[j]=candidate;
  }
  *needed=count;
  if(!order)return GUTC_OK;
  if(capacity<count)return GUTC_NO_SPACE;
  if(count)memcpy(order,sorted,count*sizeof(size_t));
  return GUTC_OK;
}
gutc_status gutc_semantic_rank_with_host(const char *input,size_t input_length,
                                         const gutc_json_text *candidates,size_t count,
                                         gutc_semantic_ranking_policy policy,
                                         const gutc_semantic_score_adapter *adapter,
                                         size_t *order,size_t capacity,size_t *needed) {
  double *scores=NULL,*relevance=NULL;
  size_t i,cp=0;
  gutc_status rc;
  if(!needed||(count&&(!candidates||(!input&&input_length)))||
     (!order&&capacity)||count>RANK_LIMIT||input_length>8192u||
     policy>GUTC_SEMANTIC_ACCURATE)return GUTC_INVALID_ARGUMENT;
  if(gutc_utf8_validate((const uint8_t*)(input_length?input:""),input_length,&cp)!=GUTC_OK)
    return GUTC_INVALID_ENCODING;
  if(!count){*needed=0;return GUTC_OK;}
  if(!input)input="";
  if(!adapter||!adapter->similarity||!adapter->rerank)return GUTC_UNSUPPORTED;
  scores=(double*)malloc(count*sizeof(double));
  relevance=(double*)malloc(count*sizeof(double));
  if(!scores||!relevance){free(scores);free(relevance);return GUTC_NO_MEMORY;}
  for(i=0;i<count;i++){scores[i]=NAN;relevance[i]=NAN;}
  for(i=0;i<count;i++){
    if(!candidates[i].bytes&&candidates[i].length){rc=GUTC_INVALID_ARGUMENT;goto done;}
    if(gutc_utf8_validate((const uint8_t*)(candidates[i].length?candidates[i].bytes:""),
                          candidates[i].length,&cp)!=GUTC_OK){
      rc=GUTC_INVALID_ENCODING;goto done;
    }
    rc=adapter->similarity(adapter->context,input,input_length,candidates[i],&scores[i]);
    if(rc!=GUTC_OK)goto done;
    if(!isfinite(scores[i])){rc=GUTC_INVALID_FORMAT;goto done;}
  }
  rc=adapter->rerank(adapter->context,input,input_length,candidates,count,policy,relevance);
  if(rc!=GUTC_OK)goto done;
  for(i=0;i<count;i++)if(!isfinite(relevance[i])){rc=GUTC_INVALID_FORMAT;goto done;}
  rc=gutc_semantic_rank_order(candidates,scores,relevance,count,order,capacity,needed);
done:
  free(scores);free(relevance);return rc;
}
