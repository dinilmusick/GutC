#include "gutc/auth_contracts.h"
#include "gutc/gutc.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

static gutc_status fail(const char **reason,const char *value) {
  if(reason)*reason=value;
  return GUTC_INVALID_FORMAT;
}
static int text_eq(gutc_json_text a,gutc_json_text b) {
  return a.length==b.length &&
    (a.length==0 || (a.bytes && b.bytes && memcmp(a.bytes,b.bytes,a.length)==0));
}
static int text_lit(gutc_json_text a,const char *value) {
  size_t n=strlen(value);
  return a.length==n && (n==0||(a.bytes&&memcmp(a.bytes,value,n)==0));
}
static const gutc_json_dom *field(const gutc_json_dom *o,const char *key) {
  return gutc_json_dom_get(o,key,strlen(key));
}
static int kind(const gutc_json_dom *v,gutc_json_kind t) {
  return v!=NULL && v->kind==t;
}
static int equals(const gutc_json_dom *v,const char *s) {
  return kind(v,GUTC_JSON_STRING)&&text_lit(v->as.string,s);
}
static int same(const gutc_json_dom *a,const gutc_json_dom *b) {
  return kind(a,GUTC_JSON_STRING)&&kind(b,GUTC_JSON_STRING)
    &&text_eq(a->as.string,b->as.string);
}
static int allowed(const gutc_json_dom *o,const char *const *keys,size_t count) {
  size_t i,j;
  if(!kind(o,GUTC_JSON_OBJECT))return 0;
  for(i=0;i<o->as.object.count;i++){
    int present=0;
    for(j=0;j<count;j++)if(text_lit(o->as.object.members[i].key,keys[j])) {
      present=1;break;
    }
    if(!present)return 0;
  }
  return 1;
}
static int ws(unsigned cp) {
  return (cp>=9&&cp<=13)||cp==32||cp==0xA0u||cp==0x1680u||
    (cp>=0x2000u&&cp<=0x200Au)||cp==0x2028u||cp==0x2029u||
    cp==0x202Fu||cp==0x205Fu||cp==0x3000u||cp==0xFEFFu;
}
static int decode_utf8(const char *s,size_t n,size_t *i,unsigned *cp) {
  unsigned char b;unsigned code;size_t k,j;
  if(*i>=n)return 0;
  b=(unsigned char)s[*i];
  if(b<0x80u){k=1;code=b;}
  else if(b>=0xC2u&&b<=0xDFu){k=2;code=b&31u;}
  else if(b>=0xE0u&&b<=0xEFu){k=3;code=b&15u;}
  else if(b>=0xF0u&&b<=0xF4u){k=4;code=b&7u;}
  else return 0;
  if(k>n-*i)return 0;
  for(j=1;j<k;j++){
    unsigned char c=(unsigned char)s[*i+j];
    if((c&0xC0u)!=0x80u)return 0;
    code=(code<<6)|(unsigned)(c&63u);
  }
  if((k==2&&code<128u)||(k==3&&code<2048u)||
     (k==4&&code<65536u)||code>0x10FFFFu||
     (code>=0xD800u&&code<=0xDFFFu))return 0;
  *i+=k;*cp=code;return 1;
}
static int units(gutc_json_text s,size_t *n,int *nonblank) {
  size_t i=0,count=0;unsigned code;
  int has_nonblank=0;
  if(!s.bytes&&s.length)return 0;
  while(i<s.length) {
    if(!decode_utf8(s.bytes,s.length,&i,&code))return 0;
    if(!ws(code))has_nonblank=1;
    count+=code>0xFFFFu?2u:1u;
    if(count>1000000)return 0;
  }
  if(n)*n=count;
  if(nonblank)*nonblank=has_nonblank;
  return 1;
}
static int contract_id(const gutc_json_dom *v) {
  size_t i=0,count=0;unsigned cp;
  gutc_json_text s;
  if(!kind(v,GUTC_JSON_STRING))return 0;
  s=v->as.string;
  if(!s.bytes||s.length==0)return 0;
  while(i<s.length) {
    if(!decode_utf8(s.bytes,s.length,&i,&cp))return 0;
    count+=cp>0xFFFFu?2u:1u;
    if(count>256||ws(cp)||cp=='\\')return 0;
  }
  return count>0;
}
static int app_id(const gutc_json_dom *v) {
  gutc_json_text s;size_t i;
  int scheme=0;
  if(!kind(v,GUTC_JSON_STRING))return 0;
  s=v->as.string;
  if(!gutc_identity_id_valid(s.bytes,s.length))return 0;
  /* The generic identity regex accepts colons but app IDs never represent
   * a URI scheme such as mailto:app or https:example. */
  if(s.bytes[0]>='A'&&s.bytes[0]<='Z')scheme=1;
  if(s.bytes[0]>='a'&&s.bytes[0]<='z')scheme=1;
  for(i=1;scheme&&i<s.length;i++){
    char c=s.bytes[i];
    if(c==':')return 0;
    if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
         (c>='0'&&c<='9')||c=='+'||c=='.'||c=='-'))scheme=0;
  }
  return 1;
}
static int safe_int(const gutc_json_dom *v,double lo,double hi) {
  return kind(v,GUTC_JSON_NUMBER) && isfinite(v->as.number) &&
    floor(v->as.number)==v->as.number &&
    v->as.number>=lo && v->as.number<=hi;
}
static int version_one(const gutc_json_dom *o) {
  const gutc_json_dom *v=field(o,"schemaVersion");
  return kind(v,GUTC_JSON_NUMBER)&&v->as.number==1.0;
}
static int unique_strings(const gutc_json_dom *array,size_t max,
                          int (*validate)(const gutc_json_dom*)) {
  size_t i,j;
  if(!kind(array,GUTC_JSON_ARRAY)||array->as.array.count>max)return 0;
  for(i=0;i<array->as.array.count;i++){
    const gutc_json_dom *a=array->as.array.values[i];
    if(!validate(a))return 0;
    for(j=0;j<i;j++)if(same(a,array->as.array.values[j]))return 0;
  }
  return 1;
}
static int params(const gutc_json_dom *array) {
  static const char *const fields[]={"name","kind","required"};
  size_t i,j;
  if(!kind(array,GUTC_JSON_ARRAY)||array->as.array.count>128)return 0;
  for(i=0;i<array->as.array.count;i++){
    const gutc_json_dom *p=array->as.array.values[i],*name,*pk;
    if(!allowed(p,fields,3))return 0;
    name=field(p,"name");pk=field(p,"kind");
    if(!contract_id(name)||
      !(equals(pk,"string")||equals(pk,"number")||equals(pk,"integer")||
        equals(pk,"boolean")||equals(pk,"object")||equals(pk,"array"))||
      !kind(field(p,"required"),GUTC_JSON_BOOLEAN))return 0;
    for(j=0;j<i;j++)if(same(name,field(array->as.array.values[j],"name")))return 0;
  }
  return 1;
}
gutc_status gutc_validate_capability_descriptor_v1(const gutc_json_dom *o,
                                                     const char **reason) {
  static const char *const base[]={"schemaVersion","appId","operationId",
    "revision","displayName","requiredScopes","inputParameters","outputParameters","kind"};
  static const char *const workflow[]={"schemaVersion","appId","operationId","revision",
    "displayName","requiredScopes","inputParameters","outputParameters","kind",
    "workflowId","ownerUserId","sourceWorkspaceId","published"};
  static const char *const stat[]={"schemaVersion","appId","operationId","revision",
    "displayName","requiredScopes","inputParameters","outputParameters","kind",
    "deploymentId","sessionPermission"};
  const gutc_json_dom *display,*cap_kind;
  size_t len=0;int nonblank=0;
  if(reason)*reason=NULL;
  display=field(o,"displayName");cap_kind=field(o,"kind");
  if(!kind(o,GUTC_JSON_OBJECT)||!version_one(o)||
     !app_id(field(o,"appId"))||!contract_id(field(o,"operationId"))||
     !contract_id(field(o,"revision"))||!kind(display,GUTC_JSON_STRING)||
     !units(display->as.string,&len,&nonblank)||!nonblank||len>160||
     !unique_strings(field(o,"requiredScopes"),64,contract_id)||
     !params(field(o,"inputParameters"))||!params(field(o,"outputParameters")))
    return fail(reason,"INVALID_CAPABILITY_BASE");
  (void)base;
  if(equals(cap_kind,"workflow")) {
    if(!allowed(o,workflow,sizeof(workflow)/sizeof(workflow[0]))||
       !contract_id(field(o,"workflowId"))||!contract_id(field(o,"ownerUserId"))||
       !contract_id(field(o,"sourceWorkspaceId"))||
       !kind(field(o,"published"),GUTC_JSON_BOOLEAN)||
       !field(o,"published")->as.boolean)
      return fail(reason,"INVALID_OR_PRIVATE_WORKFLOW_PUBLICATION");
  } else if(equals(cap_kind,"backend")||equals(cap_kind,"frontend")) {
    const gutc_json_dom *permission=field(o,"sessionPermission");
    if(!allowed(o,stat,sizeof(stat)/sizeof(stat[0]))||
       !contract_id(field(o,"deploymentId"))||
       (equals(cap_kind,"frontend") &&
        !(equals(permission,"inspect")||equals(permission,"act"))) ||
       (equals(cap_kind,"backend")&&permission))
      return fail(reason,"INVALID_STATIC_CAPABILITY");
  } else return fail(reason,"UNKNOWN_CAPABILITY_KIND");
  return GUTC_OK;
}
static int app_chain(const gutc_json_dom *array,const gutc_json_dom *caller) {
  size_t i,n;
  if(!kind(array,GUTC_JSON_ARRAY))return 0;
  n=array->as.array.count;
  if(n<1||n>16)return 0;
  for(i=0;i<n;i++)if(!app_id(array->as.array.values[i]))return 0;
  return same(array->as.array.values[n-1],caller);
}
gutc_status gutc_validate_scoped_invocation_v1(const gutc_json_dom *o,
                                                const char **reason) {
  static const char *const fields[]={
    "schemaVersion","callerAppId","targetAppId","audienceAppId","principalUserId",
    "callerWorkspaceId","targetWorkspaceId","operationId","expectedRevision",
    "operationKind","frontendSessionId","frontendPermission","frontendConsentId",
    "grantId","traceId","activeAppChain","deadlineAtMs","maxDepth",
    "remainingCalls","remainingRetries"
  };
  const gutc_json_dom *operation;
  if(reason)*reason=NULL;
  operation=field(o,"operationKind");
  if(!allowed(o,fields,sizeof(fields)/sizeof(fields[0]))||!version_one(o)||
     !app_id(field(o,"callerAppId"))||!app_id(field(o,"targetAppId"))||
     !app_id(field(o,"audienceAppId"))||!contract_id(field(o,"principalUserId"))||
     !contract_id(field(o,"callerWorkspaceId"))||!contract_id(field(o,"targetWorkspaceId"))||
     !contract_id(field(o,"operationId"))||!contract_id(field(o,"expectedRevision"))||
     !contract_id(field(o,"grantId"))||!contract_id(field(o,"traceId"))||
     !(equals(operation,"backend")||equals(operation,"frontend")||equals(operation,"workflow"))||
     !same(field(o,"audienceAppId"),field(o,"targetAppId")))
    return fail(reason,"INVALID_INVOCATION_IDENTITY_OR_AUDIENCE");
  if(equals(operation,"frontend")) {
    if(!contract_id(field(o,"frontendSessionId"))||
       !contract_id(field(o,"frontendConsentId"))||
       !(equals(field(o,"frontendPermission"),"inspect")||
         equals(field(o,"frontendPermission"),"act")))
      return fail(reason,"FRONTEND_REQUIRES_LIVE_SESSION_AND_CONSENT");
  } else if(field(o,"frontendSessionId")||field(o,"frontendPermission")||
            field(o,"frontendConsentId"))
    return fail(reason,"NON_FRONTEND_SESSION_NOT_ALLOWED");
  if(!app_chain(field(o,"activeAppChain"),field(o,"callerAppId")))
    return fail(reason,"INVALID_CALL_ANCESTRY");
  if(!safe_int(field(o,"deadlineAtMs"),-9007199254740991.0,9007199254740991.0)||
     !safe_int(field(o,"maxDepth"),1,16)||
     !safe_int(field(o,"remainingCalls"),0,1000)||
     !safe_int(field(o,"remainingRetries"),0,20))
    return fail(reason,"INVALID_EXECUTION_BUDGET");
  return GUTC_OK;
}
gutc_status gutc_check_scoped_invocation_budget_v1(const gutc_json_dom *o,
                                                   double now,const char **reason) {
  const gutc_json_dom *chain,*target;
  size_t i;
  gutc_status rc=gutc_validate_scoped_invocation_v1(o,reason);
  if(rc!=GUTC_OK)return rc;
  if(!isfinite(now)||now>=field(o,"deadlineAtMs")->as.number){
    if(reason)*reason="INVOCATION_DEADLINE_EXCEEDED";
    return GUTC_DEADLINE_EXCEEDED;
  }
  chain=field(o,"activeAppChain");
  if((double)chain->as.array.count>=field(o,"maxDepth")->as.number) {
    if(reason)*reason="INVOCATION_MAX_DEPTH";
    return GUTC_MAX_DEPTH;
  }
  target=field(o,"targetAppId");
  for(i=0;i<chain->as.array.count;i++)if(same(chain->as.array.values[i],target)) {
    if(reason)*reason="INVOCATION_CYCLE";
    return GUTC_CYCLE;
  }
  if(field(o,"remainingCalls")->as.number==0.0) {
    if(reason)*reason="INVOCATION_WORK_BUDGET_EXHAUSTED";
    return GUTC_BUDGET_EXHAUSTED;
  }
  return GUTC_OK;
}
gutc_status gutc_advance_scoped_invocation_v1(const gutc_json_dom *o,double now,
                                              unsigned *calls_left,const char **reason) {
  gutc_status rc;
  if(!calls_left)return GUTC_INVALID_ARGUMENT;
  rc=gutc_check_scoped_invocation_budget_v1(o,now,reason);
  if(rc==GUTC_OK)*calls_left=(unsigned)field(o,"remainingCalls")->as.number-1u;
  return rc;
}
gutc_status gutc_retry_scoped_invocation_v1(const gutc_json_dom *o,double now,
                                            unsigned *retries_left,const char **reason) {
  gutc_status rc;
  if(!retries_left)return GUTC_INVALID_ARGUMENT;
  rc=gutc_check_scoped_invocation_budget_v1(o,now,reason);
  if(rc!=GUTC_OK)return rc;
  if(field(o,"remainingRetries")->as.number==0.0) {
    if(reason)*reason="INVOCATION_RETRY_BUDGET_EXHAUSTED";
    return GUTC_BUDGET_EXHAUSTED;
  }
  *retries_left=(unsigned)field(o,"remainingRetries")->as.number-1u;
  return GUTC_OK;
}
gutc_status gutc_match_invocation_descriptor_v1(const gutc_json_dom *o,
                                                  const gutc_json_dom *descriptor,
                                                  double now,const char **reason) {
  gutc_status rc=gutc_check_scoped_invocation_budget_v1(o,now,reason);
  if(rc!=GUTC_OK)return rc;
  rc=gutc_validate_capability_descriptor_v1(descriptor,reason);
  if(rc!=GUTC_OK)return rc;
  if(!same(field(o,"targetAppId"),field(descriptor,"appId"))||
     !same(field(o,"operationId"),field(descriptor,"operationId"))||
     !same(field(o,"expectedRevision"),field(descriptor,"revision"))||
     !same(field(o,"operationKind"),field(descriptor,"kind")))
    return fail(reason,"TARGET_CAPABILITY_REVISION_OR_KIND_MISMATCH");
  if(equals(field(descriptor,"kind"),"workflow") &&
     !same(field(descriptor,"sourceWorkspaceId"),field(o,"targetWorkspaceId")))
    return fail(reason,"WORKFLOW_WORKSPACE_MISMATCH");
  if(equals(field(descriptor,"kind"),"frontend") &&
     !same(field(descriptor,"sessionPermission"),field(o,"frontendPermission")))
    return fail(reason,"FRONTEND_PERMISSION_MISMATCH");
  if(reason)*reason=NULL;
  return GUTC_OK;
}
