#include "gutc/model_runtime.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void gutc_model_residency_init(gutc_model_residency *r){
 if(r)memset(r,0,sizeof(*r));
}
int gutc_model_residency_is_loaded(const gutc_model_residency *r){
 return r&&r->model!=NULL;
}
size_t gutc_model_residency_users(const gutc_model_residency *r){
 return r?r->active_users:0;
}
static int same_host(const gutc_model_runtime_host *a,const gutc_model_runtime_host *b){
 return a->context==b->context && a->load==b->load &&
        a->unload==b->unload&&a->invoke==b->invoke;
}
static void clear_model(gutc_model_residency *r){
 r->model=NULL;r->cleanup_pending=0;
 memset(&r->host,0,sizeof(r->host));
}
gutc_status gutc_model_residency_acquire(gutc_model_residency *r,
 const gutc_model_runtime_host *host,const gutc_json_dom *metadata,
 gutc_model_lease *lease){
 gutc_status rc;
 void *loaded=NULL;
 if(!r||!lease||!host||!metadata||metadata->kind!=GUTC_JSON_OBJECT)
   return GUTC_INVALID_ARGUMENT;
 memset(lease,0,sizeof(*lease));
 if(!host->load||!host->unload||!host->invoke)return GUTC_UNSUPPORTED;
 if(r->cleanup_pending)return GUTC_UNSUPPORTED;
 if(r->active_users==SIZE_MAX)return GUTC_OVERFLOW;
 if(!r->model){
  if(r->active_users)return GUTC_INVALID_FORMAT;
  if(host->load_context.is_cancelled &&
     host->load_context.is_cancelled(host->load_context.context))
    return GUTC_UNSUPPORTED;
  rc=host->load(host->context,metadata,&host->load_context,&loaded);
  if(rc!=GUTC_OK)return rc;
  if(!loaded)return GUTC_INVALID_FORMAT;
  r->model=loaded;
  r->host=*host;
 }else if(!same_host(&r->host,host))return GUTC_INVALID_ARGUMENT;
 r->active_users++;
 lease->model=r->model;lease->owner=r;lease->active=1;
 return GUTC_OK;
}
gutc_status gutc_model_lease_release(gutc_model_lease *lease){
 gutc_model_residency *r;
 gutc_status rc;
 if(!lease||!lease->active||!lease->owner||!lease->model)
  return GUTC_INVALID_ARGUMENT;
 r=lease->owner;
 if(r->active_users==0||r->model!=lease->model)return GUTC_INVALID_FORMAT;
 lease->active=0;lease->model=NULL;lease->owner=NULL;
 r->active_users--;
 if(r->active_users)return GUTC_OK;
 if(!r->host.unload){r->cleanup_pending=1;return GUTC_UNSUPPORTED;}
 rc=r->host.unload(r->host.context,r->model);
 if(rc!=GUTC_OK){
  r->cleanup_pending=1;return rc;
 }
 clear_model(r);
 return GUTC_OK;
}
gutc_status gutc_model_residency_cleanup(gutc_model_residency *r){
 gutc_status rc;
 if(!r)return GUTC_INVALID_ARGUMENT;
 if(r->active_users)return GUTC_INVALID_ARGUMENT;
 if(!r->model){clear_model(r);return GUTC_OK;}
 if(!r->host.unload)return GUTC_UNSUPPORTED;
 rc=r->host.unload(r->host.context,r->model);
 if(rc==GUTC_OK)clear_model(r);
 else r->cleanup_pending=1;
 return rc;
}
gutc_status gutc_model_library_init(gutc_model_library *library,
 const char *id,size_t id_length,const gutc_model_runtime_host *host){
 int found=0;
 gutc_status rc;
 if(!library||(!id&&id_length))return GUTC_INVALID_ARGUMENT;
 memset(library,0,sizeof(*library));
 rc=gutc_model_registry_lookup(id,id_length,&library->metadata,&found);
 if(rc!=GUTC_OK)return rc;
 if(!found)return GUTC_UNSUPPORTED;
 if(host)library->runtime=*host;
 gutc_model_residency_init(&library->residency);
 return GUTC_OK;
}
gutc_status gutc_model_library_invoke(gutc_model_library *library,
 const char *operation,const gutc_json_dom *input,gutc_json_dom **output){
 gutc_model_lease lease={NULL,NULL,0};
 gutc_json_dom *result=NULL;
 gutc_status rc,release_rc;
 if(!library||!output||!operation||!operation[0]||!input)
  return GUTC_INVALID_ARGUMENT;
 *output=NULL;
 if(!library->metadata)return GUTC_INVALID_ARGUMENT;
 if(!gutc_json_dom_is_json_value(input))return GUTC_INVALID_FORMAT;
 if(!library->runtime.load||!library->runtime.unload||!library->runtime.invoke)
  return GUTC_UNSUPPORTED;
 rc=gutc_model_residency_acquire(&library->residency,
                                &library->runtime,library->metadata,&lease);
 if(rc!=GUTC_OK)return rc;
 rc=library->runtime.invoke(library->runtime.context,lease.model,
                            operation,input,&result);
 if(rc==GUTC_OK&&(!result||!gutc_json_dom_is_json_value(result)))
  rc=GUTC_INVALID_FORMAT;
 release_rc=gutc_model_lease_release(&lease);
 if(rc==GUTC_OK&&release_rc!=GUTC_OK)rc=release_rc;
 if(rc!=GUTC_OK){gutc_json_dom_free(result);return rc;}
 *output=result;
 return GUTC_OK;
}
gutc_status gutc_model_library_destroy(gutc_model_library *library){
 gutc_status rc;
 if(!library)return GUTC_INVALID_ARGUMENT;
 rc=gutc_model_residency_cleanup(&library->residency);
 if(rc!=GUTC_OK)return rc;
 gutc_json_dom_free(library->metadata);
 memset(library,0,sizeof(*library));
 return GUTC_OK;
}
