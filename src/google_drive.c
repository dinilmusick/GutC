#include "gutc/google_drive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DRIVE_PATH_CAP 8192u
#define DRIVE_MAX_TOKEN 4096u

static gutc_status append(char *buf,size_t cap,size_t *n,const char *s,size_t len){
  if(!s&&len)return GUTC_INVALID_ARGUMENT;
  if(*n>=cap||len>=cap-*n)return GUTC_NO_SPACE;
  if(len)memcpy(buf+*n,s,len);
  *n+=len;buf[*n]='\0';return GUTC_OK;
}
static gutc_status literal(char *out,size_t cap,size_t *at,const char *s){
  return append(out,cap,at,s,strlen(s));
}
static gutc_status encoded(char *out,size_t cap,size_t *at,
                            const char *value,size_t length){
  size_t cp=0,needed=0;gutc_status rc;
  if((!value&&length)||length>4096)return GUTC_INVALID_ARGUMENT;
  if(length&&memchr(value,0,length))return GUTC_INVALID_FORMAT;
  rc=gutc_utf8_validate((const uint8_t*)(length?value:""),length,&cp);
  if(rc!=GUTC_OK)return rc;
  rc=gutc_percent_encode((const uint8_t*)(length?value:""),length,NULL,0,&needed);
  if(rc!=GUTC_OK)return rc;
  if(*at>=cap||needed>=cap-*at)return GUTC_NO_SPACE;
  rc=gutc_percent_encode((const uint8_t*)(length?value:""),length,
                         out+*at,cap-*at,&needed);
  if(rc==GUTC_OK)*at+=needed;
  return rc;
}
static int safe_id(const char *id,size_t n){
  size_t i;
  if(!id||!n||n>256)return 0;
  for(i=0;i<n;++i){
    const unsigned char c=(unsigned char)id[i];
    if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
         (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
  }
  return 1;
}
static int safe_token(const char *token,size_t n){
  size_t i;
  if(!token||!n||n>DRIVE_MAX_TOKEN)return 0;
  for(i=0;i<n;++i)
    if((unsigned char)token[i]<=0x20u||(unsigned char)token[i]>=0x7fu)return 0;
  return 1;
}
static int safe_mime(const char *type){
  size_t i=0;
  if(!type||!type[0])return 0;
  while(type[i]){
    const unsigned char c=(unsigned char)type[i];
    if(i++>=127||c<=0x20u||c>=0x7fu)return 0;
  }
  return 1;
}
static int requires_id(gutc_drive_operation op){
  return op==GUTC_DRIVE_GET_METADATA||op==GUTC_DRIVE_GET_MEDIA||
         op==GUTC_DRIVE_UPDATE_METADATA||op==GUTC_DRIVE_UPDATE_MEDIA||
         op==GUTC_DRIVE_DELETE_FILE;
}
static int accepts_body(gutc_drive_operation op){
  return op==GUTC_DRIVE_CREATE_METADATA||op==GUTC_DRIVE_UPLOAD_MEDIA||
         op==GUTC_DRIVE_UPDATE_METADATA||op==GUTC_DRIVE_UPDATE_MEDIA;
}
void gutc_drive_response_clear(gutc_drive_response *response){
  if(!response)return;
  if(response->body){
    if(response->release_body)
      response->release_body(response->release_context,
                             response->body,response->body_length);
    else free(response->body);
  }
  memset(response,0,sizeof(*response));
}
gutc_status gutc_drive_client_init(gutc_drive_client *client,
                                    const gutc_drive_host *host){
  if(!client)return GUTC_INVALID_ARGUMENT;
  memset(client,0,sizeof(*client));
  if(host)client->host=*host;
  return GUTC_OK;
}
gutc_status gutc_drive_execute(const gutc_drive_client *client,
  const gutc_drive_request *r,gutc_drive_response *out){
  char path[DRIVE_PATH_CAP]={0},count[24];size_t at=0;
  const char *method=NULL,*mime=NULL;
  int n=0,has_query=0;gutc_status rc;
  gutc_drive_http_request req;
  gutc_drive_response tmp={0};
  if(!out)return GUTC_INVALID_ARGUMENT;
  memset(out,0,sizeof(*out));
  if(!client||!r)return GUTC_INVALID_ARGUMENT;
  if(!client->host.execute)return GUTC_UNSUPPORTED;
  if(!safe_token(r->access_token,r->access_token_length))
    return GUTC_INVALID_ARGUMENT;
  if((!r->body&&r->body_length)||r->body_length>64u*1024u*1024u)
    return GUTC_INVALID_ARGUMENT;
  if(r->page_size>1000u||r->filter_length>2048u||r->page_token_length>2048u)
    return GUTC_INVALID_ARGUMENT;
  if(requires_id(r->operation)&&!safe_id(r->file_id,r->file_id_length))
    return GUTC_INVALID_FORMAT;
  if(!accepts_body(r->operation)&&r->body_length)
    return GUTC_INVALID_ARGUMENT;
  if(r->operation!=GUTC_DRIVE_LIST_FILES&&
    (r->page_size||r->filter_length||r->page_token_length))
    return GUTC_INVALID_ARGUMENT;
  switch(r->operation){
    case GUTC_DRIVE_LIST_FILES:
      method="GET";rc=literal(path,sizeof(path),&at,"/drive/v3/files");
      break;
    case GUTC_DRIVE_GET_METADATA:case GUTC_DRIVE_GET_MEDIA:
      method="GET";rc=literal(path,sizeof(path),&at,"/drive/v3/files/");
      break;
    case GUTC_DRIVE_CREATE_METADATA:
      method="POST";mime="application/json";
      rc=literal(path,sizeof(path),&at,"/drive/v3/files");
      break;
    case GUTC_DRIVE_UPLOAD_MEDIA:
      method="POST";mime=r->content_type?r->content_type:"application/octet-stream";
      rc=literal(path,sizeof(path),&at,"/upload/drive/v3/files?uploadType=media");
      break;
    case GUTC_DRIVE_UPDATE_METADATA:
      method="PATCH";mime="application/json";
      rc=literal(path,sizeof(path),&at,"/drive/v3/files/");
      break;
    case GUTC_DRIVE_UPDATE_MEDIA:
      method="PATCH";mime=r->content_type?r->content_type:"application/octet-stream";
      rc=literal(path,sizeof(path),&at,"/upload/drive/v3/files/");
      break;
    case GUTC_DRIVE_DELETE_FILE:
      method="DELETE";rc=literal(path,sizeof(path),&at,"/drive/v3/files/");
      break;
    default:return GUTC_INVALID_ARGUMENT;
  }
  if(rc!=GUTC_OK)return rc;
  if(requires_id(r->operation)){
    rc=append(path,sizeof(path),&at,r->file_id,r->file_id_length);
    if(rc!=GUTC_OK)return rc;
  }
  if(r->operation==GUTC_DRIVE_GET_MEDIA){
    rc=literal(path,sizeof(path),&at,"?alt=media");
    if(rc!=GUTC_OK)return rc;
  }
  if(r->operation==GUTC_DRIVE_UPDATE_MEDIA){
    rc=literal(path,sizeof(path),&at,"?uploadType=media");
    if(rc!=GUTC_OK)return rc;
  }
  if(r->operation==GUTC_DRIVE_LIST_FILES){
    if(r->filter_length){
      rc=literal(path,sizeof(path),&at,"?q=");
      if(rc!=GUTC_OK)return rc;
      rc=encoded(path,sizeof(path),&at,r->filter,r->filter_length);
      if(rc!=GUTC_OK)return rc;
      has_query=1;
    }
    if(r->page_token_length){
      rc=literal(path,sizeof(path),&at,has_query?"&pageToken=":"?pageToken=");
      if(rc!=GUTC_OK)return rc;
      rc=encoded(path,sizeof(path),&at,r->page_token,r->page_token_length);
      if(rc!=GUTC_OK)return rc;
      has_query=1;
    }
    if(r->page_size){
      n=snprintf(count,sizeof(count),"%u",r->page_size);
      if(n<=0||(size_t)n>=sizeof(count))return GUTC_OVERFLOW;
      rc=literal(path,sizeof(path),&at,has_query?"&pageSize=":"?pageSize=");
      if(rc!=GUTC_OK)return rc;
      rc=append(path,sizeof(path),&at,count,(size_t)n);
      if(rc!=GUTC_OK)return rc;
    }
  }
  if(mime&&!safe_mime(mime))return GUTC_INVALID_FORMAT;
  req.base_url="https://www.googleapis.com";
  req.method=method;req.path=path;
  req.bearer=r->access_token;req.bearer_length=r->access_token_length;
  req.content_type=mime;req.body=r->body;req.body_length=r->body_length;
  rc=client->host.execute(client->host.context,&req,&tmp);
  if(rc!=GUTC_OK){gutc_drive_response_clear(&tmp);return rc;}
  if(tmp.http_status<100||tmp.http_status>599||
     (!tmp.body&&tmp.body_length)){
    gutc_drive_response_clear(&tmp);return GUTC_INVALID_FORMAT;
  }
  *out=tmp;return GUTC_OK;
}
