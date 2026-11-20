#include "gutc/semantic_rank.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct fake_host {
  const gutc_json_text *candidates;
  const double *similarity;
  const double *relevance;
  size_t count;
  size_t similarity_calls;
  size_t rerank_calls;
  gutc_semantic_ranking_policy seen_policy;
} fake_host;
static gutc_json_dom *parse(const char *json) {
  gutc_json_dom *out=NULL;
  gutc_status rc=gutc_json_dom_parse((const uint8_t*)json,strlen(json),32,1000,&out,NULL);
  assert(rc==GUTC_OK&&out);return out;
}
static gutc_status fake_similarity(void *context,const char *input,size_t len,
                                    gutc_json_text candidate,double *score) {
  fake_host *h=(fake_host*)context;size_t i;
  if(len!=3||memcmp(input,"cat",3))return GUTC_INVALID_FORMAT;
  for(i=0;i<h->count;i++)
    if(h->candidates[i].length==candidate.length &&
       (!candidate.length||!memcmp(h->candidates[i].bytes,candidate.bytes,candidate.length))) {
      *score=h->similarity[i];
      h->similarity_calls++;
      return GUTC_OK;
    }
  return GUTC_INVALID_FORMAT;
}
static gutc_status fake_rerank(void *context,const char *input,size_t len,
                                const gutc_json_text *candidates,size_t count,
                                gutc_semantic_ranking_policy policy,double *scores) {
  fake_host *h=(fake_host*)context;size_t i;
  if(len!=3||memcmp(input,"cat",3)||count!=h->count||!candidates)return GUTC_INVALID_FORMAT;
  for(i=0;i<count;i++)scores[i]=h->relevance[i];
  h->rerank_calls++;h->seen_policy=policy;
  return GUTC_OK;
}
static void one(const char *cands,const char *sims,const char *rels,const char *expected) {
  gutc_json_dom *array=parse(cands),*sim=parse(sims),*rel=parse(rels),*wanted=parse(expected);
  gutc_json_text texts[256];
  double similarity[256],relevance[256];
  size_t order[256],i,n,needed=0;
  gutc_status rc;
  assert(array->kind==GUTC_JSON_ARRAY&&sim->kind==GUTC_JSON_ARRAY&&
         rel->kind==GUTC_JSON_ARRAY&&wanted->kind==GUTC_JSON_ARRAY);
  n=array->as.array.count;
  assert(n<=256&&n==sim->as.array.count&&n==rel->as.array.count&&n==wanted->as.array.count);
  for(i=0;i<n;i++){
    assert(array->as.array.values[i]->kind==GUTC_JSON_STRING);
    assert(sim->as.array.values[i]->kind==GUTC_JSON_NUMBER);
    assert(rel->as.array.values[i]->kind==GUTC_JSON_NUMBER);
    texts[i]=array->as.array.values[i]->as.string;
    similarity[i]=sim->as.array.values[i]->as.number;
    relevance[i]=rel->as.array.values[i]->as.number;
  }
  rc=gutc_semantic_rank_order(texts,similarity,relevance,n,NULL,0,&needed);
  assert(rc==GUTC_OK&&needed==n);
  rc=gutc_semantic_rank_order(texts,similarity,relevance,n,order,256,&needed);
  assert(rc==GUTC_OK&&needed==n);
  for(i=0;i<n;i++){
    assert(wanted->as.array.values[i]->kind==GUTC_JSON_NUMBER);
    if((double)order[i]!=wanted->as.array.values[i]->as.number)
      fprintf(stderr,"semantic rank mismatch for %s at %zu\n",cands,i);
    assert((double)order[i]==wanted->as.array.values[i]->as.number);
  }
  if(n==3 && !strcmp(cands,"[\"red\",\"blue\",\"green\"]")){
    fake_host h={texts,similarity,relevance,n,0,0,GUTC_SEMANTIC_BALANCED};
    gutc_semantic_score_adapter adapter={&h,fake_similarity,fake_rerank};
    size_t actual[3]={99,99,99};
    assert(gutc_semantic_rank_with_host("cat",3,texts,n,GUTC_SEMANTIC_ACCURATE,
                                       &adapter,actual,3,&needed)==GUTC_OK);
    assert(h.similarity_calls==3&&h.rerank_calls==1&&h.seen_policy==GUTC_SEMANTIC_ACCURATE);
    for(i=0;i<n;i++)assert(actual[i]==order[i]);
    assert(gutc_semantic_rank_with_host("cat",3,texts,n,GUTC_SEMANTIC_FAST,
                                       NULL,actual,3,&needed)==GUTC_UNSUPPORTED);
  }
  gutc_json_dom_free(array);gutc_json_dom_free(sim);
  gutc_json_dom_free(rel);gutc_json_dom_free(wanted);
}
int main(void){
  FILE *file=fopen(GUTC_SEMANTIC_RANK_GOLDEN_PATH,"r");
  char line[10000];size_t count=0;
  assert(file);
  while(fgets(line,sizeof line,file)){
    char *a,*b,*c,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    one(line,a,b,c);count++;
  }
  fclose(file);assert(count>=20);
  {gutc_json_text words[]={{"red",3},{"blue",4}},other[]={{"é",2},{"a",1}};
   const double same[]={1,1},unranked[]={NAN,1};
   size_t result[]={999,999},needed=0;
   assert(gutc_semantic_rank_order(words,same,same,2,result,1,&needed)==GUTC_NO_SPACE);
   assert(result[0]==999&&needed==2);
   assert(gutc_semantic_rank_order(words,unranked,same,2,result,2,&needed)==GUTC_INVALID_ARGUMENT);
   assert(gutc_semantic_rank_order(other,same,same,2,result,2,&needed)==GUTC_UNSUPPORTED);
   assert(gutc_semantic_rank_with_host("cat",3,words,2,GUTC_SEMANTIC_BALANCED,NULL,
                                      result,2,&needed)==GUTC_UNSUPPORTED);}
  puts("GutC semantic comparator and real-host callback contract tests passed");
  return 0;
}
