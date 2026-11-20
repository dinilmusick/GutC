#include "gutc/auth_types.h"
#include "gutc/gutc.h"
#include <string.h>

static const char *const roles[GUTC_AUTH_IDENTITY_KIND_COUNT]={
 "application","deployment","oauth_client","user","workspace","ui_session"
};
static const char *const parameters[GUTC_AUTH_PARAM_KIND_COUNT]={
 "string","number","integer","boolean","object","array"
};
static const char *const capability_kinds[GUTC_AUTH_CAPABILITY_KIND_COUNT]={
 "backend","frontend","workflow"
};
static const char *const permissions[GUTC_AUTH_SESSION_PERMISSION_COUNT]={
 "inspect","act"
};
static const char *const environments[]={
 "production","staging","preview","development"
};
static const gutc_luth_environment environment_values[]={
 GUTC_LUTH_PRODUCTION,GUTC_LUTH_STAGING,
 GUTC_LUTH_PREVIEW,GUTC_LUTH_DEVELOPMENT
};
static const char *const platforms[]={"web","ios","android","desktop"};
static int parse(const char *const *names,size_t count,
                 const char *text,size_t len,size_t *index){
 size_t i;
 if(!index||(!text&&len))return 0;
 for(i=0;i<count;i++){
  size_t n=strlen(names[i]);
  if(n==len&&(!len||memcmp(names[i],text,len)==0)){
   *index=i;return 1;
  }
 }
 return 0;
}
const char *gutc_auth_identity_kind_name(gutc_auth_identity_kind kind){
 if((unsigned)kind>=GUTC_AUTH_IDENTITY_KIND_COUNT)return NULL;
 return roles[(size_t)kind];
}
int gutc_auth_identity_kind_parse(const char *value,size_t n,gutc_auth_identity_kind *out){
 size_t index=0;
 if(!out||!parse(roles,GUTC_AUTH_IDENTITY_KIND_COUNT,value,n,&index))return 0;
 *out=(gutc_auth_identity_kind)index;
 return 1;
}
int gutc_auth_identity_ref_valid(const gutc_auth_identity_ref *reference){
 if(!reference||!gutc_auth_identity_kind_name(reference->kind))return 0;
 return gutc_identity_id_valid(reference->id.value.bytes,
                               reference->id.value.length);
}
const char *gutc_auth_parameter_kind_name(gutc_auth_parameter_kind kind){
 if((unsigned)kind>=GUTC_AUTH_PARAM_KIND_COUNT)return NULL;
 return parameters[(size_t)kind];
}
int gutc_auth_parameter_kind_parse(const char *value,size_t n,gutc_auth_parameter_kind *out){
 size_t index=0;
 if(!out||!parse(parameters,GUTC_AUTH_PARAM_KIND_COUNT,value,n,&index))return 0;
 *out=(gutc_auth_parameter_kind)index;
 return 1;
}
const char *gutc_auth_capability_kind_name(gutc_auth_capability_kind kind){
 if((unsigned)kind>=GUTC_AUTH_CAPABILITY_KIND_COUNT)return NULL;
 return capability_kinds[(size_t)kind];
}
const char *gutc_auth_session_permission_name(gutc_auth_session_permission permission){
 if((unsigned)permission>=GUTC_AUTH_SESSION_PERMISSION_COUNT)return NULL;
 return permissions[(size_t)permission];
}
const char *gutc_auth_oauth_platform_name(gutc_oauth_platform platform){
 if((unsigned)platform>=4u)return NULL;
 return platforms[(size_t)platform];
}
int gutc_auth_deployment_environment_parse(const char *value,size_t n,
                                            gutc_luth_environment *out){
 size_t index=0;
 if(!out||!parse(environments,4,value,n,&index))return 0;
 *out=environment_values[index];
 return 1;
}
