#include "gutc/identity.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static gutc_string_view str(const char *s) {gutc_string_view v={s,strlen(s)};return v;}
static gutc_value val(const char *s) {
  gutc_value v={0};
  v.kind=GUTC_VALUE_STRING;v.as.string=str(s);return v;
}
int main(void) {
  gutc_identity_ref ref={{0},{0}};
  gutc_object_entry legacy[2];
  gutc_string_view resolved={0};
  const char *chain[]={"a"};
  gutc_invocation_budget b={"a","b","b",chain,1,200,3,3,2};
  unsigned retry=99;
  assert(gutc_identity_ref_parse(str("application"),str("hello-1"),&ref)==GUTC_OK);
  assert(ref.kind.length==11&&ref.id.length==7);
  assert(gutc_identity_ref_parse(str("invalid"),str("hello"),&ref)==GUTC_INVALID_ARGUMENT);
  assert(gutc_identity_ref_parse(str("user"),str("/bad"),&ref)==GUTC_INVALID_ARGUMENT);
  legacy[0]=(gutc_object_entry){.key={"app_id",6},.value=val("app-1")};
  legacy[1]=(gutc_object_entry){.key={"appId",5},.value=val("app-1")};
  assert(gutc_legacy_application_id(legacy,2,&resolved)==GUTC_OK&&resolved.length==5);
  legacy[1].value=val("other");
  assert(gutc_legacy_application_id(legacy,2,&resolved)==GUTC_INVALID_FORMAT);
  assert(gutc_invocation_budget_retry(&b,100,&retry)==GUTC_OK&&retry==1);
  b.remaining_retries=0;
  assert(gutc_invocation_budget_retry(&b,100,&retry)==GUTC_BUDGET_EXHAUSTED&&retry==1);
  assert(gutc_invocation_budget_retry(&b,201,&retry)==GUTC_DEADLINE_EXCEEDED);
  puts("GutC identity and retry tests passed");
  return 0;
}
