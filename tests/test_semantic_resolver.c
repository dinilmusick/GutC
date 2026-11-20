#include "gutc/semantic_resolver.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct host_mock {
 unsigned similarity_calls,rerank_calls,complete_calls;
 int ambiguous,empty_output,fail,aborted;
 gutc_semantic_ranking_policy policy;
} host_mock;
static const gutc_json_dom *get(const gutc_json_dom *o,const char *key){
 return gutc_json_dom_get(o,key,strlen(key));
}
static int eq(const gutc_json_dom *v,const char *want){
 return v&&v->kind==GUTC_JSON_STRING&&v->as.string.length==strlen(want)&&
   !memcmp(v->as.string.bytes,want,v->as.string.length);
}
static gutc_json_dom *parse(const char *json){
 gutc_json_dom *v=NULL;
 assert(gutc_json_dom_parse((const uint8_t*)json,strlen(json),64,1000,&v,NULL)==GUTC_OK);
 assert(v);return v;
}
static void fixtures(void){
 FILE *f=fopen(GUTC_SEMANTIC_RESOLVER_PATH,"r");
 char line[8192];size_t cases=0;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *status,*expected,*confirmation,*end;
  gutc_json_dom *request,*result=NULL;
  const gutc_json_dom *value,*required;
  char value_text[512];
  size_t written=0;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  status=strchr(line,'\t');assert(status);*status++='\0';
  expected=strchr(status,'\t');assert(expected);*expected++='\0';
  confirmation=strchr(expected,'\t');assert(confirmation);
  *confirmation++='\0';
  request=parse(line);
  assert(gutc_resolve_semantic_input(request,NULL,&result)==GUTC_OK);
  assert(result&&eq(get(result,"status"),status));
  value=get(result,"value");
  if(!strcmp(expected,"_"))assert(!value);
  else {
   assert(value);
   assert(gutc_json_dom_stringify(value,value_text,sizeof value_text,&written)==GUTC_OK);
   if(strcmp(value_text,expected))
    fprintf(stderr,"case %zu got %s expected %s\n",cases,value_text,expected);
   assert(!strcmp(value_text,expected));
  }
  required=get(result,"requiresConfirmation");
  assert((required&&required->kind==GUTC_JSON_BOOLEAN&&required->as.boolean)==
    (confirmation[0]=='1'));
  gutc_json_dom_free(result);gutc_json_dom_free(request);
  cases++;
 }
 fclose(f);assert(cases==12);
}
static gutc_status similarity(void *ctx,const char *input,size_t length,
 gutc_json_text candidate,double *score){
 host_mock *m=(host_mock*)ctx;
 assert(length==5&&!memcmp(input,"maybe",5));
 m->similarity_calls++;
 *score=!memcmp(candidate.bytes,"alpha",5)?(m->ambiguous?0.2:0.9):0.1;
 return GUTC_OK;
}
static gutc_status rerank(void *ctx,const char *input,size_t length,
 const gutc_json_text *candidates,size_t count,
 gutc_semantic_ranking_policy policy,double *scores){
 host_mock *m=(host_mock*)ctx;
 assert(length==5&&!memcmp(input,"maybe",5));
 assert(candidates&&count==2);
 m->policy=policy;m->rerank_calls++;
 scores[0]=m->ambiguous?0.51:0.9;
 scores[1]=m->ambiguous?0.5:0.1;
 return GUTC_OK;
}
static gutc_status complete(void *ctx,const char *prompt,size_t length,
 const gutc_json_dom *schema,gutc_json_dom **out){
 host_mock *m=(host_mock*)ctx;
 assert(schema&&schema->kind==GUTC_JSON_OBJECT);
 assert(prompt&&length==strlen(prompt));
 assert(strstr(prompt,"Convert the natural-language input into the supplied target schema."));
 assert(strstr(prompt,"Target schema: "));
 assert(strstr(prompt,"Input: "));
 m->complete_calls++;
 if(m->fail)return GUTC_UNSUPPORTED;
 if(m->empty_output)return gutc_json_dom_parse((const uint8_t*)"null",4,16,100,out,NULL);
 if(strstr(prompt,"Candidate values: "))
  return gutc_json_dom_parse((const uint8_t*)"\"not an option\"",15,16,100,out,NULL);
 return gutc_json_dom_parse((const uint8_t*)"{\"data\":{\"ok\":true}}",20,16,100,out,NULL);
}
static int aborted(void *ctx){return ((host_mock*)ctx)->aborted;}
static void host_cases(void){
 gutc_semantic_resolver_host host={0};
 host_mock state={0};
 gutc_json_dom *request=NULL,*result=NULL;
 const gutc_json_dom *array,*evidence,*clarification;
 gutc_status rc;
 host.context=&state;host.ranking.context=&state;
 host.ranking.similarity=similarity;host.ranking.rerank=rerank;
 host.complete=complete;host.is_aborted=aborted;
 request=parse("{\"input\":\"maybe\",\"schema\":{\"type\":\"string\"},\"candidates\":[\"alpha\",\"beta\"],\"context\":{\"label\":\"Color\"},\"policy\":{\"modelPolicy\":\"accurate\"}}");
 assert(gutc_resolve_semantic_input(request,&host,&result)==GUTC_OK);
 assert(eq(get(result,"status"),"resolved"));
 assert(eq(get(result,"value"),"alpha"));
 array=get(result,"candidates");
 assert(array&&array->kind==GUTC_JSON_ARRAY&&array->as.array.count==2);
 evidence=get(result,"evidence");assert(eq(get(evidence,"selectedCandidate"),"alpha"));
 assert(state.policy==GUTC_SEMANTIC_ACCURATE);
 assert(state.similarity_calls==2&&state.rerank_calls==1&&state.complete_calls==1);
 gutc_json_dom_free(result);result=NULL;
 state.ambiguous=1;
 assert(gutc_resolve_semantic_input(request,&host,&result)==GUTC_OK);
 assert(eq(get(result,"status"),"ambiguous"));
 clarification=get(result,"clarification");
 assert(clarification&&eq(get(clarification,"question"),
                             "Which value did you mean for Color: alpha, beta?"));
 gutc_json_dom_free(result);result=NULL;
 state.fail=1;
 assert(gutc_resolve_semantic_input(request,&host,&result)==GUTC_OK);
 assert(eq(get(result,"status"),"failed"));
 assert(eq(get(get(result,"error"),"code"),"MODEL_FAILURE"));
 gutc_json_dom_free(result);result=NULL;
 state.fail=0;state.empty_output=1;
 assert(gutc_resolve_semantic_input(request,&host,&result)==GUTC_OK);
 assert(eq(get(result,"status"),"unresolved"));
 gutc_json_dom_free(result);result=NULL;
 state.empty_output=0;state.aborted=1;
 assert(gutc_resolve_semantic_input(request,&host,&result)==GUTC_OK);
 assert(eq(get(result,"status"),"failed"));
 assert(eq(get(get(result,"error"),"code"),"ABORTED"));
 gutc_json_dom_free(result);result=NULL;
 state.aborted=0;
 assert(gutc_resolve_semantic_input(request,NULL,&result)==GUTC_UNSUPPORTED);
 assert(!result);
 gutc_json_dom_free(request);
 request=parse("{\"input\":\"hello\",\"schema\":{\"type\":\"object\"}}");
 assert(gutc_resolve_semantic_input(request,&host,&result)==GUTC_OK);
 assert(eq(get(result,"status"),"resolved"));
 assert(get(result,"value")&&get(result,"value")->kind==GUTC_JSON_OBJECT);
 assert(get(get(result,"value"),"ok")->as.boolean);
 gutc_json_dom_free(result);result=NULL;
 gutc_json_dom_free(request);
 request=parse("{\"input\":\"whatever\",\"schema\":{\"type\":\"string\",\"pattern\":\"^[a-z]+$\"}}");
 rc=gutc_resolve_semantic_input(request,&host,&result);
 assert(rc==GUTC_UNSUPPORTED&&!result); /* JS RegExp host exception */
 gutc_json_dom_free(request);
}
int main(void){
 fixtures();
 host_cases();
 puts("GutC native semantic input deterministic, real-host ranking, selection and failure tests passed");
 return 0;
}
