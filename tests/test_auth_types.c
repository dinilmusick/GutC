#include "gutc/auth_types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check_literal(const char *group,const char *value,size_t index){
 const char *actual=NULL;
 if(!strcmp(group,"identity")){
  gutc_auth_identity_kind kind=GUTC_AUTH_IDENTITY_KIND_COUNT;
  assert(gutc_auth_identity_kind_parse(value,strlen(value),&kind));
  assert((size_t)kind==index);
  actual=gutc_auth_identity_kind_name(kind);
 }else if(!strcmp(group,"environment")){
  gutc_luth_environment environment=GUTC_LUTH_DEVELOPMENT;
  assert(gutc_auth_deployment_environment_parse(value,strlen(value),&environment));
  actual=gutc_luth_environment_name(environment);
 }else if(!strcmp(group,"oauth-platform")){
  actual=gutc_auth_oauth_platform_name((gutc_oauth_platform)index);
 }else if(!strcmp(group,"parameter")){
  gutc_auth_parameter_kind parameter=GUTC_AUTH_PARAM_KIND_COUNT;
  assert(gutc_auth_parameter_kind_parse(value,strlen(value),&parameter));
  assert((size_t)parameter==index);
  actual=gutc_auth_parameter_kind_name(parameter);
 }else if(!strcmp(group,"capability-kind")){
  actual=gutc_auth_capability_kind_name((gutc_auth_capability_kind)index);
 }else if(!strcmp(group,"permission")){
  actual=gutc_auth_session_permission_name((gutc_auth_session_permission)index);
 }else abort();
 assert(actual&&!strcmp(actual,value));
}
static void type_smoke(void){
 const gutc_auth_text t={"id",2};
 const gutc_auth_optional_text optional={1,{"present",7}};
 const gutc_auth_text_array values={&t,1};
 gutc_auth_identity_ref ref={GUTC_AUTH_USER,{{"user-1",6}}};
 gutc_auth_application_id application={{t}};
 gutc_auth_deployment_id deployment={{t}};
 gutc_auth_oauth_client_id oauth_client={{t}};
 gutc_auth_user_id user_id={{t}};
 gutc_auth_workspace_id workspace={{t}};
 gutc_auth_ui_session_id session_id={{t}};
 gutc_luth_deployment_record deploy={0};
 gutc_luth_auth_client_config config={0};
 gutc_luth_auth_token_store token_store={0};
 gutc_luth_registration_result registration={0};
 gutc_luth_user user={0};
 gutc_luth_session session={0};
 gutc_luth_session_result session_result={0};
 gutc_luth_email_sign_in signin={0};
 gutc_luth_email_sign_up signup={0};
 gutc_luth_google_sign_in google={0};
 gutc_luth_auth_error error={0};
 gutc_auth_oauth_registration_v1 oauth={0};
 gutc_auth_contract_check contract={0};
 gutc_auth_capability_parameter_v1 parameter={0};
 gutc_auth_capability_base_v1 base={0};
 gutc_auth_static_capability_v1 static_cap={0};
 gutc_auth_workflow_publication_v1 workflow={0};
 gutc_auth_capability_descriptor_v1 descriptor={0};
 assert(gutc_auth_identity_ref_valid(&ref));
 ref.id.value=(gutc_auth_text){"bad id",6};
 assert(!gutc_auth_identity_ref_valid(&ref));
 ref.id.value=t;
 assert(gutc_auth_identity_ref_valid(&ref));
 assert(application.id.value.length==2&&deployment.id.value.length==2);
 assert(oauth_client.id.value.length==2&&user_id.id.value.length==2);
 assert(workspace.id.value.length==2&&session_id.id.value.length==2);
 deploy.id=t;deploy.url=t;deploy.label=optional;deploy.has_environment=1;
 deploy.environment=GUTC_LUTH_PRODUCTION;deploy.is_active=1;
 deploy.is_primary=(gutc_auth_optional_boolean){1,1};
 deploy.priority=(gutc_auth_optional_number){1,5};
 config.ecosystem_id=t;config.application_id=t;config.luth_url=optional;
 config.luth_urls=values;config.origin=optional;config.has_environment=1;
 config.environment=GUTC_LUTH_STAGING;config.auto_register=(gutc_auth_optional_boolean){1,1};
 config.max_retries=(gutc_auth_optional_number){1,3};
 config.retry_delay_ms=(gutc_auth_optional_number){1,200};
 config.timeout_ms=(gutc_auth_optional_number){1,1000};
 config.token_store=&token_store;
 registration.registered=1;registration.ecosystem_id=t;
 registration.application_id=t;registration.origin=t;
 registration.environment=GUTC_LUTH_PRODUCTION;registration.authentication_enabled=1;
 user.id=t;user.name=t;user.email=t;user.image=(gutc_auth_nullable_text){1,1,{NULL,0}};
 user.email_verified=(gutc_auth_optional_boolean){1,1};
 session.id=t;session.user_id=t;session.expires_at=t;session.token=optional;
 session_result.authenticated=1;session_result.has_user=1;
 session_result.user=user;session_result.has_session=1;
 session_result.session=session;session_result.error=optional;
 signin.email=t;signin.password=t;
 signup.email=t;signup.password=t;signup.name=t;
 google.callback_url=optional;
 error.code=t;error.message=t;error.http_status=(gutc_auth_optional_number){1,401};
 oauth.schema_version=1;oauth.app_id=t;oauth.client_id=t;oauth.platform=GUTC_OAUTH_WEB;
 oauth.public_client=1;oauth.pkce_method=(gutc_auth_text){"S256",4};
 oauth.redirect_uris=values;oauth.verified_hosts=values;
 contract.ok=1;contract.borrowed_value=NULL;contract.reason=optional;
 parameter.name=t;parameter.kind=GUTC_AUTH_PARAM_STRING;parameter.required=1;
 base.schema_version=1;base.app_id=t;base.operation_id=t;base.revision=t;
 base.display_name=t;base.required_scopes=values;
 base.input_parameters=&parameter;base.input_parameter_count=1;
 base.output_parameters=&parameter;base.output_parameter_count=1;
 static_cap.base=base;static_cap.kind=GUTC_AUTH_BACKEND;
 static_cap.deployment_id=t;static_cap.has_session_permission=0;
 workflow.base=base;workflow.workflow_id=t;
 workflow.owner_user_id=t;workflow.source_workspace_id=t;workflow.published=1;
 descriptor.kind=GUTC_AUTH_WORKFLOW;descriptor.value.workflow=workflow;
 assert(deploy.has_environment&&config.luth_urls.count==1&&token_store.context==NULL);
 assert(registration.registered&&session_result.has_session&&session.token.present);
 assert(signin.password.length==2&&signup.name.length==2&&google.callback_url.present);
 assert(error.http_status.value==401&&oauth.redirect_uris.count==1);
 assert(contract.ok&&descriptor.value.workflow.base.schema_version==1&&static_cap.deployment_id.length==2);
 assert(gutc_auth_identity_kind_name((gutc_auth_identity_kind)999)==NULL);
 assert(gutc_auth_parameter_kind_name((gutc_auth_parameter_kind)999)==NULL);
 assert(gutc_auth_capability_kind_name((gutc_auth_capability_kind)999)==NULL);
 assert(gutc_auth_session_permission_name((gutc_auth_session_permission)999)==NULL);
 assert(gutc_auth_oauth_platform_name((gutc_oauth_platform)999)==NULL);
 {gutc_auth_identity_kind role=GUTC_AUTH_USER;gutc_auth_parameter_kind p=GUTC_AUTH_PARAM_STRING;
  gutc_luth_environment env=GUTC_LUTH_PRODUCTION;
  assert(!gutc_auth_identity_kind_parse("invalid",7,&role)&&role==GUTC_AUTH_USER);
  assert(!gutc_auth_parameter_kind_parse("unknown",7,&p)&&p==GUTC_AUTH_PARAM_STRING);
  assert(!gutc_auth_deployment_environment_parse("prod",4,&env)&&env==GUTC_LUTH_PRODUCTION);
  assert(!gutc_auth_identity_kind_parse(NULL,1,&role));
  assert(!gutc_auth_parameter_kind_parse(NULL,1,&p));}
}
int main(void){
 FILE *f=fopen(GUTC_AUTH_TYPES_PATH,"r");
 char line[2048];size_t cases=0;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *a,*b,*end;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  a=strchr(line,'\t');assert(a);*a++='\0';
  b=strchr(a,'\t');assert(b);*b++='\0';
  check_literal(line,a,(size_t)strtoul(b,NULL,10));cases++;
 }
 fclose(f);assert(cases==25);
 type_smoke();
 puts("GutC source-pinned auth type and tagged-union contracts passed");
 return 0;
}
