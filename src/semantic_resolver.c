#include "gutc/semantic_resolver.h"
#include "gutc/gutc.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SEM_MAX_CANDIDATES 512u
#define SEM_MAX_PROMPT 65536u
typedef struct sem_buffer {char *bytes;size_t used,capacity;} sem_buffer;
static const gutc_json_dom *get(const gutc_json_dom *o,const char *key){
 return gutc_json_dom_get(o,key,strlen(key));
}
static int is_string(const gutc_json_dom *v){
 return v&&v->kind==GUTC_JSON_STRING &&
   (v->as.string.length==0||v->as.string.bytes!=NULL);
}
static int equal(const gutc_json_dom *v,const char *s){
 return is_string(v)&&v->as.string.length==strlen(s)&&
   (!v->as.string.length||!memcmp(v->as.string.bytes,s,v->as.string.length));
}
static int truth(const gutc_json_dom *v){
 return v&&v->kind==GUTC_JSON_BOOLEAN&&v->as.boolean!=0;
}
static gutc_json_dom *make(gutc_json_kind kind){
 gutc_json_dom *v=(gutc_json_dom*)calloc(1,sizeof(*v));
 if(v)v->kind=kind;
 return v;
}
static gutc_json_dom *str(const char *s,size_t n){
 gutc_json_dom *v;
 if((!s&&n)||n>1024u*1024u)return NULL;
 v=make(GUTC_JSON_STRING);if(!v)return NULL;
 v->as.string.bytes=(char*)malloc(n+1);
 if(!v->as.string.bytes){free(v);return NULL;}
 if(n)memcpy(v->as.string.bytes,s,n);
 v->as.string.bytes[n]='\0';v->as.string.length=n;return v;
}
static gutc_json_dom *cstr(const char *s){
 return str(s,strlen(s));
}
static gutc_json_dom *boolean(int value){
 gutc_json_dom *v=make(GUTC_JSON_BOOLEAN);
 if(v)v->as.boolean=!!value;
 return v;
}
static gutc_json_dom *number(double value){
 gutc_json_dom *v;
 if(!isfinite(value))return NULL;
 v=make(GUTC_JSON_NUMBER);if(v)v->as.number=value;
 return v;
}
static gutc_json_dom *copy_node(const gutc_json_dom *src){
 char *buffer;size_t needed=0;
 gutc_json_dom *out=NULL;
 if(!src||gutc_json_dom_stringify(src,NULL,0,&needed)!=GUTC_OK||
    needed>16u*1024u*1024u)return NULL;
 buffer=(char*)malloc(needed+1);if(!buffer)return NULL;
 if(gutc_json_dom_stringify(src,buffer,needed+1,&needed)==GUTC_OK)
  (void)gutc_json_dom_parse((const uint8_t*)buffer,needed,128,100000,&out,NULL);
 free(buffer);
 return out;
}
static int put(gutc_json_dom *obj,const char *key,gutc_json_dom *v){
 gutc_json_member *members;
 char *owned;size_t n;
 if(!v)return 0;
 if(!obj||obj->kind!=GUTC_JSON_OBJECT){gutc_json_dom_free(v);return 0;}
 n=strlen(key);
 if(obj->as.object.count>=1000||obj->as.object.count>SIZE_MAX/sizeof(*members)-1){
  gutc_json_dom_free(v);return 0;
 }
 owned=(char*)malloc(n+1);
 if(!owned){gutc_json_dom_free(v);return 0;}
 memcpy(owned,key,n+1);
 members=(gutc_json_member*)realloc(obj->as.object.members,
   (obj->as.object.count+1)*sizeof(*members));
 if(!members){free(owned);gutc_json_dom_free(v);return 0;}
 obj->as.object.members=members;
 members[obj->as.object.count].key.bytes=owned;
 members[obj->as.object.count].key.length=n;
 members[obj->as.object.count].value=v;
 obj->as.object.count++;
 return 1;
}
static int push(gutc_json_dom *arr,gutc_json_dom *v){
 gutc_json_dom **members;
 if(!v)return 0;
 if(!arr||arr->kind!=GUTC_JSON_ARRAY||
    arr->as.array.count>SIZE_MAX/sizeof(*members)-1){
  gutc_json_dom_free(v);return 0;
 }
 members=(gutc_json_dom**)realloc(arr->as.array.values,
  (arr->as.array.count+1)*sizeof(*members));
 if(!members){gutc_json_dom_free(v);return 0;}
 arr->as.array.values=members;
 arr->as.array.values[arr->as.array.count++]=v;
 return 1;
}
static int put_string(gutc_json_dom *o,const char *key,const char *val){
 return put(o,key,cstr(val));
}
static int put_view(gutc_json_dom *o,const char *key,const gutc_json_dom *val){
 return put(o,key,copy_node(val));
}
static int add_error(gutc_json_dom *o,const char *code,const char *message){
 gutc_json_dom *err=make(GUTC_JSON_OBJECT);
 if(!err)return 0;
 if(!put_string(err,"code",code)||!put_string(err,"message",message)){
  gutc_json_dom_free(err);return 0;
 }
 return put(o,"error",err);
}
static gutc_json_dom *base(const char *status,const gutc_json_dom *input,int preserve,
                           const gutc_json_dom *candidates){
 gutc_json_dom *o=make(GUTC_JSON_OBJECT);
 if(!o)return NULL;
 if(!put_string(o,"status",status)||
    (preserve&&!put_view(o,"rawInput",input))||
    !put(o,"candidates",candidates?copy_node(candidates):make(GUTC_JSON_ARRAY))){
  gutc_json_dom_free(o);return NULL;
 }
 return o;
}
static gutc_status simple_failure(const char *status,const char *code,
 const char *message,const gutc_json_dom *input,int preserve,
 const gutc_json_dom *candidates,gutc_json_dom **output){
 gutc_json_dom *o=base(status,input,preserve,candidates);
 if(!o)return GUTC_NO_MEMORY;
 if(!add_error(o,code,message)){gutc_json_dom_free(o);return GUTC_NO_MEMORY;}
 *output=o;return GUTC_OK;
}
static int buf_append(sem_buffer *b,const char *s,size_t n){
 if(!b||(!s&&n)||b->used>=b->capacity||n>=b->capacity-b->used)
  return 0;
 if(n)memcpy(b->bytes+b->used,s,n);
 b->used+=n;b->bytes[b->used]='\0';
 return 1;
}
static int buf_line(sem_buffer *b,const char *label,const char *s,size_t n){
 if(b->used&&!buf_append(b,"\n",1))return 0;
 return buf_append(b,label,strlen(label))&&buf_append(b,s,n);
}
static int line_field(sem_buffer *b,const gutc_json_dom *ctx,
 const char *prop,const char *label){
 const gutc_json_dom *v=get(ctx,prop);
 if(!v||!is_string(v)||!v->as.string.length)return 1;
 return buf_line(b,label,v->as.string.bytes,v->as.string.length);
}
static int line_array(sem_buffer *b,const gutc_json_dom *ctx,
 const char *key,const char *label,const char *sep){
 const gutc_json_dom *v=get(ctx,key);
 size_t i;
 if(!v||v->kind!=GUTC_JSON_ARRAY||!v->as.array.count)return 1;
 if(!v->as.array.values)return 0;
 if(b->used&&!buf_append(b,"\n",1))return 0;
 if(!buf_append(b,label,strlen(label)))return 0;
 for(i=0;i<v->as.array.count;i++){
  const gutc_json_dom *item=v->as.array.values[i];
  if(!is_string(item))return 0;
  if(i&&!buf_append(b,sep,strlen(sep)))return 0;
  if(!buf_append(b,item->as.string.bytes,item->as.string.length))return 0;
 }
 return 1;
}
static int build_prompt(sem_buffer *b,const gutc_json_dom *req,
 const gutc_json_dom *schema,const gutc_json_dom *input){
 const gutc_json_dom *ctx=get(req,"context");
 const gutc_json_dom *requested_candidates=get(req,"candidates");
 char *schema_json=NULL;size_t needed=0;
 size_t i;
 static const char *const keys[]={
  "application","workspace","page","section","form","field","label",
  "description","semanticType","locale"
 };
 static const char *const labels[]={
  "Application: ","Workspace: ","Page: ","Section: ","Form: ",
  "Field: ","Label: ","Description: ","Semantic type: ","Locale: "
 };
 if(!buf_line(b,"","Convert the natural-language input into the supplied target schema.",
    strlen("Convert the natural-language input into the supplied target schema."))||
    !buf_line(b,"","Return only data conforming to the target schema. Never invent information.",
    strlen("Return only data conforming to the target schema. Never invent information.")))
  return 0;
 for(i=0;i<9;i++)if(!line_field(b,ctx,keys[i],labels[i]))return 0;
 if(!line_array(b,ctx,"vocabulary","Vocabulary: ",", ")||
    !line_array(b,ctx,"examples","Examples: "," | ")||
    !line_array(b,ctx,"aliases","Aliases: ",", ")||
    !line_array(b,ctx,"constraints","Constraints: ","; ")||
    !line_array(b,ctx,"relatedFields","Related fields: ",", ")||
    !line_field(b,ctx,keys[9],labels[9]))return 0;
 if(requested_candidates&&requested_candidates->kind==GUTC_JSON_ARRAY&&
    requested_candidates->as.array.count){
  if(!line_array(b,req,"candidates","Candidate values: "," | "))return 0;
 }
 if(gutc_json_dom_stringify(schema,NULL,0,&needed)!=GUTC_OK||
    needed>SEM_MAX_PROMPT)return 0;
 schema_json=(char*)malloc(needed+1);if(!schema_json)return 0;
 if(gutc_json_dom_stringify(schema,schema_json,needed+1,&needed)!=GUTC_OK){
  free(schema_json);return 0;
 }
 i=buf_line(b,"Target schema: ",schema_json,needed);
 free(schema_json);if(!i)return 0;
 return buf_line(b,"Input: ",input->as.string.bytes,input->as.string.length);
}
static int score_json(gutc_json_dom *array,const gutc_json_text *cands,
 const double *sim,const double *rel,const size_t *order,size_t count){
 size_t j;
 for(j=0;j<count;j++){
  size_t i=order[j];
  gutc_json_dom *item=make(GUTC_JSON_OBJECT);
  if(!item)return 0;
  if(!put(item,"value",str(cands[i].bytes,cands[i].length))||
     !put(item,"similarity",number(sim[i]))||
     !put(item,"relevance",number(rel[i]))){
    gutc_json_dom_free(item);return 0;
  }
  if(!push(array,item))return 0;
 }
 return 1;
}
static int evidence(gutc_json_dom *resolution,const gutc_json_dom *model,
 const gutc_json_dom *candidates,const gutc_json_dom *selected,
 int has_candidates,double min_sim,double min_margin){
 gutc_json_dom *e=make(GUTC_JSON_OBJECT);
 if(!e)return 0;
 if(!put(e,"needleOutput",model?copy_node(model):make(GUTC_JSON_NULL))||
    !put(e,"candidates",copy_node(candidates))||
    (selected&&!put_view(e,"selectedCandidate",selected))||
    (has_candidates&&(!put(e,"similarityThreshold",number(min_sim))||
       !put(e,"relevanceMarginThreshold",number(min_margin))))){
  gutc_json_dom_free(e);return 0;
 }
 return put(resolution,"evidence",e);
}
static int add_clarification(gutc_json_dom *resolution,const gutc_json_dom *ctx,
 const gutc_json_dom *ranked){
 sem_buffer b={NULL,0,0};
 const gutc_json_dom *label=get(ctx,"label");
 gutc_json_dom *c=make(GUTC_JSON_OBJECT),*list=make(GUTC_JSON_ARRAY);
 size_t i,top;
 if(!c||!list){gutc_json_dom_free(c);gutc_json_dom_free(list);return 0;}
 if(!is_string(label))label=get(ctx,"field");
 b.capacity=4096;b.bytes=(char*)malloc(b.capacity);
 if(!b.bytes){gutc_json_dom_free(c);gutc_json_dom_free(list);return 0;}
 b.bytes[0]='\0';
 if(!buf_append(&b,"Which value did you mean for ",29)||
    !(is_string(label)?buf_append(&b,label->as.string.bytes,label->as.string.length):
       buf_append(&b,"this field",10))||
    !buf_append(&b,": ",2))goto fail;
 top=ranked->as.array.count<3?ranked->as.array.count:3;
 for(i=0;i<top;i++){
  const gutc_json_dom *v=get(ranked->as.array.values[i],"value");
  if(!is_string(v)||(i&&!buf_append(&b,", ",2))||
     !buf_append(&b,v->as.string.bytes,v->as.string.length)||
     !push(list,copy_node(v)))goto fail;
 }
 if(!buf_append(&b,"?",1)||
    !put(c,"question",str(b.bytes,b.used)))goto fail;
 if(!put(c,"candidates",list)){
  list=NULL;goto fail;
 }
 list=NULL;
 free(b.bytes);
 return put(resolution,"clarification",c);
fail:
 gutc_json_dom_free(list);
 gutc_json_dom_free(c);free(b.bytes);return 0;
}
gutc_status gutc_resolve_semantic_input(const gutc_json_dom *req,
 const gutc_semantic_resolver_host *host,gutc_json_dom **output){
 const gutc_json_dom *input,*schema,*ctx,*list,*policy,*p,*model=NULL,*selected=NULL;
 gutc_json_dom *deterministic=NULL,*raw=NULL,*ranked=NULL,*result=NULL;
 gutc_json_text *candidates=NULL;
 double *sim=NULL,*rel=NULL,min_sim=0.65,min_margin=0.15;
 size_t *order=NULL,count=0,j,need=0,chosen=0;
 int preserve=0,confirm=0,resolved=0;
 gutc_semantic_ranking_policy model_policy=GUTC_SEMANTIC_BALANCED;
 gutc_status rc=GUTC_INVALID_ARGUMENT;
 sem_buffer prompt={NULL,0,0};
 if(!req||!output||req->kind!=GUTC_JSON_OBJECT)return GUTC_INVALID_ARGUMENT;
 *output=NULL;
 input=get(req,"input");schema=get(req,"schema");ctx=get(req,"context");
 if(!is_string(input)||!schema||schema->kind!=GUTC_JSON_OBJECT||
    (ctx&&ctx->kind!=GUTC_JSON_OBJECT))return GUTC_INVALID_FORMAT;
 p=get(req,"preserveRawInput");preserve=truth(p);
 policy=get(req,"policy");
 if(policy&&policy->kind!=GUTC_JSON_OBJECT)return GUTC_INVALID_FORMAT;
 confirm=truth(get(policy,"requireConfirmation"))||equal(get(policy,"risk"),"high");
 p=get(policy,"modelPolicy");
 if(p){
  if(equal(p,"fast"))model_policy=GUTC_SEMANTIC_FAST;
  else if(equal(p,"balanced"))model_policy=GUTC_SEMANTIC_BALANCED;
  else if(equal(p,"accurate"))model_policy=GUTC_SEMANTIC_ACCURATE;
  else return GUTC_INVALID_FORMAT;
 }
 p=get(req,"minCandidateSimilarity");
 if(p){
  if(p->kind!=GUTC_JSON_NUMBER||!isfinite(p->as.number))return GUTC_INVALID_FORMAT;
  min_sim=p->as.number;
 }
 p=get(req,"minCandidateMargin");
 if(p){
  if(p->kind!=GUTC_JSON_NUMBER||!isfinite(p->as.number))return GUTC_INVALID_FORMAT;
  min_margin=p->as.number;
 }
 if(host&&host->is_aborted&&host->is_aborted(host->context))
  return simple_failure("failed","ABORTED","Semantic resolution was aborted.",
                        input,preserve,NULL,output);
 {
  int blank=0;
  rc=gutc_string_blank_ascii(input->as.string.bytes,input->as.string.length,&blank);
  if(rc!=GUTC_OK)return rc;
  if(blank)return simple_failure("invalid","EMPTY_INPUT","Semantic input is empty.",
                                 input,preserve,NULL,output);
 }
 rc=gutc_resolve_deterministic_semantic(
   input->as.string.bytes,input->as.string.length,schema,&deterministic,&resolved);
 if(rc!=GUTC_OK)return rc;
 if(resolved){
  if(!deterministic)return GUTC_INVALID_FORMAT;
  if(confirm&&!put(deterministic,"requiresConfirmation",boolean(1))){
    gutc_json_dom_free(deterministic);return GUTC_NO_MEMORY;
  }
  *output=deterministic;return GUTC_OK;
 }
 gutc_json_dom_free(deterministic);
 list=get(req,"candidates");
 if(!list||list->kind!=GUTC_JSON_ARRAY||list->as.array.count==0)
  list=get(ctx,"vocabulary");
 if(list&&list->kind!=GUTC_JSON_ARRAY)return GUTC_INVALID_FORMAT;
 if(list)count=list->as.array.count;
 if(count>SEM_MAX_CANDIDATES||(count&&(!list->as.array.values)))return GUTC_RANGE_ERROR;
 if(count){
  if(!host||!host->ranking.similarity||!host->ranking.rerank)
   return GUTC_UNSUPPORTED;
  candidates=(gutc_json_text*)calloc(count,sizeof(*candidates));
  sim=(double*)calloc(count,sizeof(*sim));
  rel=(double*)calloc(count,sizeof(*rel));
  order=(size_t*)calloc(count,sizeof(*order));
  if(!candidates||!sim||!rel||!order){rc=GUTC_NO_MEMORY;goto cleanup;}
  for(j=0;j<count;j++){
   const gutc_json_dom *v=list->as.array.values[j];
   if(!is_string(v)){rc=GUTC_INVALID_FORMAT;goto cleanup;}
   candidates[j]=v->as.string;
   rc=host->ranking.similarity(host->ranking.context,input->as.string.bytes,
       input->as.string.length,candidates[j],&sim[j]);
   if(rc!=GUTC_OK)goto cleanup;
  }
  rc=host->ranking.rerank(host->ranking.context,input->as.string.bytes,
      input->as.string.length,candidates,count,model_policy,rel);
  if(rc!=GUTC_OK)goto cleanup;
  rc=gutc_semantic_rank_order(candidates,sim,rel,count,order,count,&need);
  if(rc!=GUTC_OK)goto cleanup;
 }
 ranked=make(GUTC_JSON_ARRAY);
 if(!ranked){rc=GUTC_NO_MEMORY;goto cleanup;}
 if(count&&!score_json(ranked,candidates,sim,rel,order,count)){
  rc=GUTC_NO_MEMORY;goto cleanup;
 }
 if(host&&host->is_aborted&&host->is_aborted(host->context)){
  rc=simple_failure("failed","ABORTED","Semantic resolution was aborted.",
                    input,preserve,ranked,output);goto cleanup;
 }
 if(!host||!host->complete){rc=GUTC_UNSUPPORTED;goto cleanup;}
 prompt.capacity=SEM_MAX_PROMPT;prompt.bytes=(char*)malloc(prompt.capacity);
 if(!prompt.bytes){rc=GUTC_NO_MEMORY;goto cleanup;}
 prompt.bytes[0]='\0';
 if(!build_prompt(&prompt,req,schema,input)){rc=GUTC_NO_SPACE;goto cleanup;}
 rc=host->complete(host->context,prompt.bytes,prompt.used,schema,&raw);
 if(rc!=GUTC_OK){
  rc=simple_failure("failed","MODEL_FAILURE","Needle structured resolution failed.",
                    input,preserve,ranked,output);
  goto cleanup;
 }
 model=raw;
 if(raw&&raw->kind==GUTC_JSON_OBJECT){
  static const char *const keys[]={"output","result","data","structuredOutput"};
  for(j=0;j<4;j++)if(get(raw,keys[j])){model=get(raw,keys[j]);break;}
 }
 if(!model||model->kind==GUTC_JSON_NULL||
    (is_string(model)&&model->as.string.length==0)){
  result=base("unresolved",input,preserve,ranked);
  if(!result){rc=GUTC_NO_MEMORY;goto cleanup;}
  if(model&&!put_view(result,"modelOutput",model)){rc=GUTC_NO_MEMORY;goto cleanup;}
  if(!add_error(result,"MODEL_OUTPUT_EMPTY",
      "Needle returned no usable semantic value.")){rc=GUTC_NO_MEMORY;goto cleanup;}
  *output=result;result=NULL;rc=GUTC_OK;goto cleanup;
 }
 if(count&&is_string(model)){
  for(j=0;j<count;j++){
   if(candidates[order[j]].length==model->as.string.length&&
      !memcmp(candidates[order[j]].bytes,model->as.string.bytes,
              model->as.string.length)){
     selected=get(ranked->as.array.values[j],"value");break;
   }
  }
  if(!selected){
   size_t top=order[0];
   double margin=count>1?rel[top]-rel[order[1]]:INFINITY;
   if(sim[top]>=min_sim&&margin>=min_margin)
    selected=get(ranked->as.array.values[0],"value");
  }
 }else selected=model;
 if(!selected){
  result=base(count?"ambiguous":"invalid",input,preserve,ranked);
  if(!result){rc=GUTC_NO_MEMORY;goto cleanup;}
  if(!put_view(result,"modelOutput",model)||
     !evidence(result,model,ranked,NULL,count>0,min_sim,min_margin)){
    rc=GUTC_NO_MEMORY;goto cleanup;
  }
  if(count>1&&!add_clarification(result,ctx,ranked)){
    rc=GUTC_NO_MEMORY;goto cleanup;
  }
  if(!add_error(result,count?"AMBIGUOUS_CANDIDATE":"SCHEMA_MISMATCH",
      count?"Semantic evidence did not uniquely identify a candidate.":
      "The model output could not be resolved to the target schema.")){
    rc=GUTC_NO_MEMORY;goto cleanup;
  }
 }else {
  result=base("resolved",input,preserve,ranked);
  if(!result){rc=GUTC_NO_MEMORY;goto cleanup;}
  if(!put_view(result,"value",selected)||
     !put_view(result,"modelOutput",model)||
     !evidence(result,model,ranked,count&&is_string(model)?selected:NULL,
               count>0,min_sim,min_margin)||
     (confirm&&!put(result,"requiresConfirmation",boolean(1)))){
    rc=GUTC_NO_MEMORY;goto cleanup;
  }
 }
 *output=result;result=NULL;rc=GUTC_OK;
cleanup:
 gutc_json_dom_free(result);gutc_json_dom_free(raw);
 gutc_json_dom_free(ranked);
 free(candidates);free(sim);free(rel);free(order);free(prompt.bytes);
 return rc;
}
