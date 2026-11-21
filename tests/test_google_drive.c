#include "gutc/google_drive.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct fixture {
  const char *method,*path,*mime;
  int status,calls,releases,fail_transport;
} fixture;
static void release_bytes(void *ctx,uint8_t *body,size_t n){
  fixture *f=(fixture*)ctx;
  assert(n==2);f->releases++;free(body);
}
static gutc_status execute(void *ctx,const gutc_drive_http_request *r,
                           gutc_drive_response *out){
  fixture *f=(fixture*)ctx;
  assert(!strcmp(r->base_url,"https://www.googleapis.com"));
  assert(!strcmp(r->method,f->method));
  assert(!strcmp(r->path,f->path));
  assert((!r->content_type&&!f->mime)||
         (r->content_type&&f->mime&&!strcmp(r->content_type,f->mime)));
  assert(r->bearer_length==11&&!memcmp(r->bearer,"valid_token",11));
  f->calls++;
  out->http_status=f->status;
  out->body=(uint8_t*)malloc(2);
  assert(out->body);
  out->body[0]='{';out->body[1]='}';out->body_length=2;
  out->release_context=f;out->release_body=release_bytes;
  return f->fail_transport?GUTC_UNSUPPORTED:GUTC_OK;
}
static void expect(gutc_drive_client *client,fixture *f,
                   gutc_drive_request *request,const char *method,
                   const char *path,const char *mime,int status) {
  gutc_drive_response result={0};
  const int previous=f->calls,previous_release=f->releases;
  f->method=method;f->path=path;f->mime=mime;f->status=status;
  assert(gutc_drive_execute(client,request,&result)==GUTC_OK);
  assert(f->calls==previous+1);
  assert(result.http_status==status && result.body_length==2);
  gutc_drive_response_clear(&result);
  assert(!result.body&&result.http_status==0&&f->releases==previous_release+1);
}
int main(void){
  fixture f={0};
  gutc_drive_host host={&f,execute};
  gutc_drive_client c={0},unconfigured={0};
  gutc_drive_response out={0};
  const uint8_t data[]={0,255,1};
  static const uint8_t meta[]="{\"name\":\"x\"}";
  gutc_drive_request r={0};
  assert(gutc_drive_client_init(NULL,&host)==GUTC_INVALID_ARGUMENT);
  assert(gutc_drive_client_init(&unconfigured,NULL)==GUTC_OK);
  r.access_token="valid_token";r.access_token_length=11;
  r.operation=GUTC_DRIVE_LIST_FILES;
  assert(gutc_drive_execute(&unconfigured,&r,&out)==GUTC_UNSUPPORTED);
  assert(gutc_drive_client_init(&c,&host)==GUTC_OK);
  r.filter="'parent' in parents";r.filter_length=strlen(r.filter);
  r.page_token="a/b +";r.page_token_length=strlen(r.page_token);
  r.page_size=100;
  expect(&c,&f,&r,"GET",
    "/drive/v3/files?q=%27parent%27%20in%20parents&pageToken=a%2Fb%20%2B&pageSize=100",
    NULL,200);
  r.filter=NULL;r.filter_length=0;r.page_token=NULL;r.page_token_length=0;
  r.page_size=0;r.file_id="abc_DEF-123";r.file_id_length=strlen(r.file_id);
  r.operation=GUTC_DRIVE_GET_METADATA;
  expect(&c,&f,&r,"GET","/drive/v3/files/abc_DEF-123",NULL,200);
  r.operation=GUTC_DRIVE_GET_MEDIA;
  expect(&c,&f,&r,"GET","/drive/v3/files/abc_DEF-123?alt=media",NULL,206);
  r.file_id=NULL;r.file_id_length=0;
  r.body=meta;r.body_length=sizeof(meta)-1;
  r.operation=GUTC_DRIVE_CREATE_METADATA;
  expect(&c,&f,&r,"POST","/drive/v3/files","application/json",201);
  r.operation=GUTC_DRIVE_UPLOAD_MEDIA;
  r.body=data;r.body_length=sizeof data;r.content_type="image/png";
  expect(&c,&f,&r,"POST","/upload/drive/v3/files?uploadType=media","image/png",200);
  r.file_id="abc_DEF-123";r.file_id_length=strlen(r.file_id);
  r.operation=GUTC_DRIVE_UPDATE_MEDIA;
  expect(&c,&f,&r,"PATCH","/upload/drive/v3/files/abc_DEF-123?uploadType=media","image/png",200);
  r.operation=GUTC_DRIVE_UPDATE_METADATA;
  r.body=meta;r.body_length=sizeof(meta)-1;
  expect(&c,&f,&r,"PATCH","/drive/v3/files/abc_DEF-123","application/json",200);
  r.operation=GUTC_DRIVE_DELETE_FILE;r.body=NULL;r.body_length=0;
  expect(&c,&f,&r,"DELETE","/drive/v3/files/abc_DEF-123",NULL,204);
  expect(&c,&f,&r,"DELETE","/drive/v3/files/abc_DEF-123",NULL,401);
  const int previous=f.releases;
  f.fail_transport=1;
  f.method="DELETE";f.path="/drive/v3/files/abc_DEF-123";f.mime=NULL;f.status=502;
  assert(gutc_drive_execute(&c,&r,&out)==GUTC_UNSUPPORTED);
  assert(f.releases==previous+1 && !out.body);
  f.fail_transport=0;
  r.file_id="../etc";r.file_id_length=6;
  assert(gutc_drive_execute(&c,&r,&out)==GUTC_INVALID_FORMAT);
  r.file_id="a";r.file_id_length=1;
  r.access_token="bad\nbearer";r.access_token_length=10;
  assert(gutc_drive_execute(&c,&r,&out)==GUTC_INVALID_ARGUMENT);
  r.access_token="valid_token";r.access_token_length=11;
  r.operation=GUTC_DRIVE_LIST_FILES;r.file_id=NULL;r.file_id_length=0;
  r.filter="\xC0\xAF";r.filter_length=2;
  assert(gutc_drive_execute(&c,&r,&out)==GUTC_INVALID_ENCODING);
  r.filter=NULL;r.filter_length=0;
  r.page_size=1001;
  assert(gutc_drive_execute(&c,&r,&out)==GUTC_INVALID_ARGUMENT);
  r.page_size=0;r.operation=(gutc_drive_operation)999;
  assert(gutc_drive_execute(&c,&r,&out)==GUTC_INVALID_ARGUMENT);
  puts("GutC Drive v3 real URL contracts, binary ownership, HTTP errors, OAuth safety and host failure verified");
  return 0;
}
