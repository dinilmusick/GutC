#include "gutc/engineering_types.h"
#include <string.h>
static const char *const providers[GUTC_ENG_PROVIDER_COUNT]={
 "github","gitlab","bitbucket","git","local","container",
 "webcontainer","remote-sandbox","ci"
};
static const char *const capabilities[GUTC_ENG_CAPABILITY_COUNT]={
 "repository.read","repository.write","git.write","workspace.create",
 "execution.run","browser.run","verification.run","artifact.write",
 "package.publish","deployment.write","promotion.write"
};
static const char *const networks[GUTC_ENG_NETWORK_COUNT]={
 "none","restricted","full"
};
static const char *const execution_statuses[GUTC_ENG_EXECUTION_STATUS_COUNT]={
 "succeeded","failed","cancelled","timed-out"
};
static const char *const check_statuses[GUTC_ENG_CHECK_STATUS_COUNT]={
 "passed","failed","skipped","blocked"
};
static const char *const report_statuses[GUTC_ENG_REPORT_STATUS_COUNT]={
 "passed","failed","blocked"
};
static int match(const char *const *values,size_t count,
                 const char *text,size_t length,size_t *index){
 size_t i;
 if(!index||(!text&&length))return 0;
 for(i=0;i<count;i++){
  size_t n=strlen(values[i]);
  if(n==length&&(!n||memcmp(text,values[i],n)==0)){
   *index=i;return 1;
  }
 }
 return 0;
}
const char *gutc_eng_provider_name(gutc_eng_provider_kind kind){
 if((unsigned)kind>=GUTC_ENG_PROVIDER_COUNT)return NULL;
 return providers[(size_t)kind];
}
int gutc_eng_provider_parse(const char *value,size_t length,gutc_eng_provider_kind *out){
 size_t index=0;
 if(!out||!match(providers,GUTC_ENG_PROVIDER_COUNT,value,length,&index))return 0;
 *out=(gutc_eng_provider_kind)index;
 return 1;
}
const char *gutc_eng_capability_name(gutc_eng_capability kind){
 if((unsigned)kind>=GUTC_ENG_CAPABILITY_COUNT)return NULL;
 return capabilities[(size_t)kind];
}
int gutc_eng_capability_parse(const char *value,size_t length,gutc_eng_capability *out){
 size_t index=0;
 if(!out||!match(capabilities,GUTC_ENG_CAPABILITY_COUNT,value,length,&index))return 0;
 *out=(gutc_eng_capability)index;
 return 1;
}
const char *gutc_eng_network_name(gutc_eng_network value){
 if((unsigned)value>=GUTC_ENG_NETWORK_COUNT)return NULL;
 return networks[(size_t)value];
}
const char *gutc_eng_execution_status_name(gutc_eng_execution_status value){
 if((unsigned)value>=GUTC_ENG_EXECUTION_STATUS_COUNT)return NULL;
 return execution_statuses[(size_t)value];
}
const char *gutc_eng_check_status_name(gutc_eng_check_status value){
 if((unsigned)value>=GUTC_ENG_CHECK_STATUS_COUNT)return NULL;
 return check_statuses[(size_t)value];
}
const char *gutc_eng_report_status_name(gutc_eng_report_status value){
 if((unsigned)value>=GUTC_ENG_REPORT_STATUS_COUNT)return NULL;
 return report_statuses[(size_t)value];
}
