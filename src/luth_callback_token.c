#include "gutc/luth_callback_token.h"
#include "gutc/runtime_app_url.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LIMIT 8192u
typedef struct writer {char *buf;size_t length,capacity;int overflow;} writer;
static int bounded(const char *s,size_t *out){
  size_t i;
  if(!s){*out=0;return 1;}
  for(i=0;i<LIMIT;i++)if(!s[i]){*out=i;return 1;}
  return 0;
}
static int hex(char c){
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}
static gutc_status decode(const char *text,size_t n,char *out,size_t *used){
  size_t i,j=0,points=0;
  for(i=0;i<n;i++) {
    unsigned char c=(unsigned char)text[i];
    if(c=='%'){
      int hi,lo;
      if(n-i<3||(hi=hex(text[i+1]))<0||(lo=hex(text[i+2]))<0)
        return GUTC_UNSUPPORTED;
      c=(unsigned char)((hi<<4)|lo);
      i+=2;
    }else if(c=='+')c=' ';
    if(c==0)return GUTC_UNSUPPORTED;
    out[j++]=(char)c;
  }
  out[j]='\0';
  if(gutc_utf8_validate((const uint8_t*)out,j,&points)!=GUTC_OK)
    return GUTC_UNSUPPORTED;
  *used=j;
  return GUTC_OK;
}
static void emit(writer *w,const char *s,size_t n){
  if(w->overflow)return;
  if(n>w->capacity-w->length){w->overflow=1;return;}
  if(n)memcpy(w->buf+w->length,s,n);
  w->length+=n;
}
static void ch(writer *w,char c){emit(w,&c,1);}
static void encode_form(writer *w,const char *s,size_t n){
  static const char hexchars[]="0123456789ABCDEF";
  size_t i;
  for(i=0;i<n;i++){
    const unsigned char c=(unsigned char)s[i];
    if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
       (c>='0'&&c<='9')||c=='*'||c=='-'||c=='.'||c=='_')
      ch(w,(char)c);
    else if(c==' ')ch(w,'+');
    else{
      char bytes[3]={'%',hexchars[c>>4],hexchars[c&15]};
      emit(w,bytes,3);
    }
  }
}
static int token_key(const char *s,size_t n){
  return n==5&&!memcmp(s,"token",5);
}
gutc_status gutc_luth_hydrate_callback_token(
  const char *pathname,const char *search,const char *hash,
  const gutc_luth_callback_host *host,int *hydrated){
  size_t pn=0,qn=0,hn=0,begin=0,end,rawk,rawv,decodedk=0,decodedv=0;
  size_t token_len=0,remaining=0,relative_cap;
  char *key=NULL,*value=NULL,*token=NULL,*relative=NULL;
  writer w;
  gutc_status rc=GUTC_OK;
  int found=0,written=0;
  if(!hydrated)return GUTC_INVALID_ARGUMENT;
  *hydrated=0;
  if(!bounded(search,&qn)||!bounded(pathname,&pn)||!bounded(hash,&hn))
    return GUTC_RANGE_ERROR;
  if(!search||!qn)return GUTC_OK;
  if(search[0]!='?'||(hash&&hn&&hash[0]!='#')||
     (pathname&&pn&&pathname[0]!='/'))return GUTC_UNSUPPORTED;
  if(!host||!host->store_token||!host->replace_history)return GUTC_UNSUPPORTED;
  key=(char*)malloc(qn+1);value=(char*)malloc(qn+1);
  token=(char*)malloc(qn+1);
  if(!key||!value||!token){rc=GUTC_NO_MEMORY;goto done;}
  begin=1;
  while(begin<qn){
    size_t k_end;
    end=begin;
    while(end<qn&&search[end]!='&')end++;
    if(end==begin){begin=end+1;continue;}
    k_end=begin;
    while(k_end<end&&search[k_end]!='=')k_end++;
    rawk=k_end-begin;rawv=k_end<end?k_end+1:end;
    rc=decode(search+begin,rawk,key,&decodedk);
    if(rc!=GUTC_OK)goto done;
    rc=decode(search+rawv,end-rawv,value,&decodedv);
    if(rc!=GUTC_OK)goto done;
    if(!found&&token_key(key,decodedk)){
      found=1;
      rc=gutc_resolve_runtime_app_url(value,NULL,0,&token_len);
      if(rc!=GUTC_OK)goto done;
      if(token_len){
        rc=gutc_resolve_runtime_app_url(value,token,qn+1,&remaining);
        if(rc!=GUTC_OK)goto done;
        token_len=remaining;
      }
    }
    begin=end+1;
  }
  if(!found||!token_len){rc=GUTC_OK;goto done;}
  /* Each query byte expands by at most three when reserialized. */
  if(pn>SIZE_MAX-hn-8 || qn>(SIZE_MAX-pn-hn-8)/3){
    rc=GUTC_OVERFLOW;goto done;
  }
  relative_cap=pn+hn+qn*3+8;
  relative=(char*)malloc(relative_cap);
  if(!relative){rc=GUTC_NO_MEMORY;goto done;}
  w.buf=relative;w.length=0;w.capacity=relative_cap-1;w.overflow=0;
  if(pathname&&pn)emit(&w,pathname,pn);
  else ch(&w,'/');
  written=0;
  begin=1;
  while(begin<qn){
    size_t k_end;
    end=begin;
    while(end<qn&&search[end]!='&')end++;
    if(end==begin){begin=end+1;continue;}
    k_end=begin;
    while(k_end<end&&search[k_end]!='=')k_end++;
    rawk=k_end-begin;rawv=k_end<end?k_end+1:end;
    rc=decode(search+begin,rawk,key,&decodedk);
    if(rc!=GUTC_OK)goto done;
    rc=decode(search+rawv,end-rawv,value,&decodedv);
    if(rc!=GUTC_OK)goto done;
    if(!token_key(key,decodedk)){
      ch(&w,written?'&':'?');
      encode_form(&w,key,decodedk);
      ch(&w,'=');
      encode_form(&w,value,decodedv);
      written=1;
    }
    begin=end+1;
  }
  if(hash&&hn)emit(&w,hash,hn);
  if(w.overflow){rc=GUTC_OVERFLOW;goto done;}
  relative[w.length]='\0';
  rc=host->store_token(host->context,token,token_len);
  if(rc!=GUTC_OK)goto done;
  rc=host->replace_history(host->context,relative,w.length);
  if(rc!=GUTC_OK)goto done;
  *hydrated=1;
done:
  free(key);free(value);free(token);free(relative);
  return rc;
}
