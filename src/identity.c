#include "gutc/identity.h"
#include <string.h>
#include <stdint.h>
static int vieweq(gutc_string_view a,const char *lit) {
  size_t n=strlen(lit);
  return a.length==n && (n==0 || (a.bytes&&memcmp(a.bytes,lit,n)==0));
}
static int kind_ok(gutc_string_view v) {
  return vieweq(v,"application")||vieweq(v,"deployment")||
    vieweq(v,"oauth_client")||vieweq(v,"user")||vieweq(v,"workspace")||
    vieweq(v,"ui_session");
}
gutc_status gutc_identity_ref_parse(gutc_string_view kind,gutc_string_view id,
                                     gutc_identity_ref *result) {
  if(!result||!kind_ok(kind)||!gutc_identity_id_valid(id.bytes,id.length))
    return GUTC_INVALID_ARGUMENT;
  result->kind=kind;result->id=id;
  return GUTC_OK;
}
gutc_status gutc_legacy_application_id(const gutc_object_entry *entries,
                                        size_t count,gutc_string_view *result) {
  size_t i;
  gutc_string_view selected={NULL,0};
  int found=0;
  if(!result || (!entries&&count))return GUTC_INVALID_ARGUMENT;
  for(i=0;i<count;i++) {
    gutc_string_view k=entries[i].key;
    int alias=vieweq(k,"app_id")||vieweq(k,"appId")||vieweq(k,"applicationId");
    if(!alias)continue;
    if(entries[i].value.kind!=GUTC_VALUE_STRING)return GUTC_INVALID_FORMAT;
    {gutc_string_view candidate=entries[i].value.as.string;
    if(!gutc_identity_id_valid(candidate.bytes,candidate.length))return GUTC_INVALID_FORMAT;
    if(found && (candidate.length!=selected.length ||
       memcmp(candidate.bytes,selected.bytes,selected.length)!=0))return GUTC_INVALID_FORMAT;
    selected=candidate;found=1;}
  }
  if(!found)return GUTC_INVALID_FORMAT;
  *result=selected;return GUTC_OK;
}
gutc_status gutc_invocation_budget_retry(const gutc_invocation_budget *budget,
                                          int64_t now_ms,unsigned *remaining_retries) {
  gutc_status s;
  if(!remaining_retries)return GUTC_INVALID_ARGUMENT;
  s=gutc_invocation_budget_check(budget,now_ms);
  if(s!=GUTC_OK)return s;
  if(budget->remaining_retries==0)return GUTC_BUDGET_EXHAUSTED;
  *remaining_retries=budget->remaining_retries-1;
  return GUTC_OK;
}
