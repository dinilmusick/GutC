#include "gutc/engineering_types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void match(const char *group,const char *value,size_t index){
 const char *actual=NULL;
 size_t n=strlen(value);
 if(!strcmp(group,"provider")){
  gutc_eng_provider_kind kind=GUTC_ENG_PROVIDER_COUNT;
  assert(gutc_eng_provider_parse(value,n,&kind));
  assert((size_t)kind==index);
  actual=gutc_eng_provider_name(kind);
 }else if(!strcmp(group,"capability")){
  gutc_eng_capability capability=GUTC_ENG_CAPABILITY_COUNT;
  assert(gutc_eng_capability_parse(value,n,&capability));
  assert((size_t)capability==index);
  actual=gutc_eng_capability_name(capability);
 }else if(!strcmp(group,"network")){
  actual=gutc_eng_network_name((gutc_eng_network)index);
 }else if(!strcmp(group,"execution-status")){
  actual=gutc_eng_execution_status_name((gutc_eng_execution_status)index);
 }else if(!strcmp(group,"check-status")){
  actual=gutc_eng_check_status_name((gutc_eng_check_status)index);
 }else if(!strcmp(group,"report-status")){
  actual=gutc_eng_report_status_name((gutc_eng_report_status)index);
 }else abort();
 if(!actual||strcmp(actual,value))fprintf(stderr,"Engineering kind %s index %zu expected %s\n",group,index,value);
 assert(actual&&!strcmp(actual,value));
}
static void compile_structural_contracts(void){
 const gutc_eng_text text={"hello",5};
 const gutc_eng_optional_text optional={1,{"world",5}};
 gutc_eng_repository_ref repository={0};
 gutc_eng_workspace_ref workspace={0};
 gutc_eng_execution_request request={0};
 gutc_eng_execution_result result={0};
 gutc_eng_verification_check check={0};
 gutc_eng_verification_report report={0};
 gutc_eng_artifact_ref artifact={0};
 gutc_eng_capability_grant grant={0};
 gutc_eng_operation_context context={0};
 gutc_eng_provenance_record provenance={0};
 gutc_eng_environment_pair env={{"K",1},{"V",1}};
 gutc_eng_resources resources={0};
 repository.id=text;repository.provider=GUTC_ENG_GITHUB;
 repository.owner=optional;repository.name=text;
 repository.default_branch=optional;repository.remote=optional;
 workspace.id=text;workspace.root=text;workspace.repositories=&repository;
 workspace.repository_count=1;workspace.snapshot_id=optional;
 resources.cpu.present=1;resources.cpu.value=2.0;
 resources.memory_mb.present=1;resources.memory_mb.value=1024.0;
 resources.disk_mb.present=1;resources.disk_mb.value=2048.0;
 request.workspace_id=workspace.id;request.command=text;
 request.cwd=optional;request.environment=&env;request.environment_count=1;
 request.timeout_ms.present=1;request.timeout_ms.value=1000.0;
 request.network=GUTC_ENG_NETWORK_RESTRICTED;request.has_network=1;
 request.resources=resources;request.has_resources=1;
 result.execution_id=text;result.status=GUTC_ENG_EXECUTION_SUCCEEDED;
 result.exit_code.present=1;result.exit_code.value=0;
 result.stdout_text=text;result.stderr_text=text;
 result.artifact_ids.values=&text;result.artifact_ids.count=1;
 check.id=text;check.name=text;check.status=GUTC_ENG_CHECK_PASSED;
 check.duration_ms.present=1;check.duration_ms.value=12.0;
 check.execution_id=optional;check.details=optional;
 report.id=text;report.status=GUTC_ENG_REPORT_PASSED;
 report.checks=&check;report.check_count=1;
 report.artifact_ids=result.artifact_ids;
 artifact.id=text;artifact.digest=text;artifact.type=text;
 artifact.uri=optional;artifact.source_execution_id=optional;
 grant.capability=GUTC_ENG_REPOSITORY_READ;
 grant.resource=optional;grant.expires_at=optional;
 context.task_id=text;context.trace_id=text;context.workspace_id=optional;
 context.grants=&grant;context.grant_count=1;
 provenance.operation_id=text;provenance.actor_id=text;
 provenance.task_id=text;provenance.trace_id=text;provenance.timestamp=text;
 provenance.has_source=1;provenance.source=repository;
 provenance.commit=optional;provenance.artifacts=result.artifact_ids;
 assert(workspace.repository_count==1&&workspace.repositories->provider==GUTC_ENG_GITHUB);
 assert(request.has_network&&request.environment[0].key.length==1);
 assert(request.resources.memory_mb.value==1024.0);
 assert(result.artifact_ids.count==1&&report.checks->duration_ms.value==12.0);
 assert(artifact.uri.present&&context.grant_count==1);
 assert(provenance.has_source&&provenance.source.name.length==5);
 assert(gutc_eng_provider_name((gutc_eng_provider_kind)999)==NULL);
 assert(gutc_eng_capability_name((gutc_eng_capability)999)==NULL);
 assert(gutc_eng_network_name((gutc_eng_network)999)==NULL);
 assert(gutc_eng_execution_status_name((gutc_eng_execution_status)999)==NULL);
 assert(gutc_eng_check_status_name((gutc_eng_check_status)999)==NULL);
 assert(gutc_eng_report_status_name((gutc_eng_report_status)999)==NULL);
 {
  gutc_eng_provider_kind k=GUTC_ENG_GITHUB;
  gutc_eng_capability c=GUTC_ENG_REPOSITORY_READ;
  assert(!gutc_eng_provider_parse("evil",4,&k)&&k==GUTC_ENG_GITHUB);
  assert(!gutc_eng_capability_parse("root.access",11,&c)&&c==GUTC_ENG_REPOSITORY_READ);
  assert(!gutc_eng_provider_parse(NULL,1,&k));
  assert(!gutc_eng_capability_parse(NULL,1,&c));
  assert(!gutc_eng_provider_parse("git",3,NULL));
 }
}
int main(void){
 FILE *f=fopen(GUTC_ENGINEERING_KINDS_PATH,"r");
 char line[4096];size_t rows=0;
 assert(f);
 while(fgets(line,sizeof line,f)){
  char *a,*b,*end;
  size_t n;
  if(line[0]=='#'||line[0]=='\n')continue;
  end=strchr(line,'\n');if(end)*end='\0';
  a=strchr(line,'\t');assert(a);*a++='\0';
  b=strchr(a,'\t');assert(b);*b++='\0';
  n=(size_t)strtoul(b,NULL,10);
  match(line,a,n);rows++;
 }
 fclose(f);assert(rows==34);
 compile_structural_contracts();
 puts("GutC source-pinned engineering type and tagged-union contracts passed");
 return 0;
}
