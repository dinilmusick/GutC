#include "gutc/url_query.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct url_span {
  size_t length, query_start, query_end, fragment_start;
  int has_query;
} url_span;
typedef struct writer {char *out;size_t used;int overflow;} writer;
static unsigned char lower(unsigned char c) {
  return c>='A'&&c<='Z'?(unsigned char)(c+('a'-'A')):c;
}
static int bounded(const char *s,size_t *n) {
  size_t i;
  if(!s)return 0;
  for(i=0;i<1048576u;i++)if(s[i]=='\0'){*n=i;return 1;}
  return 0;
}
static int valid_utf8(const char *s,size_t n) {
  size_t count=0;
  return gutc_utf8_validate((const uint8_t*)s,n,&count)==GUTC_OK;
}
static int valid_http(const char *url,url_span *sp) {
  size_t i,n,scheme_len=0,host_start,host_end,question;
  if(!bounded(url,&n))return 0;
  if(n>=7 &&
     lower((unsigned char)url[0])=='h'&&
     lower((unsigned char)url[1])=='t'&&
     lower((unsigned char)url[2])=='t'&&
     lower((unsigned char)url[3])=='p') {
    if(url[4]==':'&&url[5]=='/'&&url[6]=='/')scheme_len=7;
    else if(n>=8 && lower((unsigned char)url[4])=='s'&&
            url[5]==':'&&url[6]=='/'&&url[7]=='/')scheme_len=8;
  }
  if(!scheme_len)return 0;
  for(i=0;i<n;i++){
    unsigned char c=(unsigned char)url[i];
    if(c<0x21u||c>=0x7Fu||c=='\\')return 0;
  }
  host_start=scheme_len;host_end=host_start;
  while(host_end<n&&url[host_end]!='/'&&url[host_end]!='?'&&url[host_end]!='#')host_end++;
  if(host_end==host_start)return 0;
  /* Validate bracket balancing and reject an empty numeric port. */
  {int in_bracket=0;for(i=host_start;i<host_end;i++) {
     if(url[i]=='['){if(in_bracket)return 0;in_bracket=1;}
     if(url[i]==']'){if(!in_bracket)return 0;in_bracket=0;}
   }
   if(in_bracket)return 0;
  }
  sp->length=n;sp->fragment_start=n;
  for(i=host_end;i<n;i++)if(url[i]=='#'){sp->fragment_start=i;break;}
  question=sp->fragment_start;
  for(i=host_end;i<sp->fragment_start;i++)if(url[i]=='?'){question=i;break;}
  sp->has_query=question<sp->fragment_start;
  sp->query_start=sp->has_query?question+1:sp->fragment_start;
  sp->query_end=sp->fragment_start;
  return 1;
}
int gutc_url_is_http_url(const char *url) {
  url_span span;
  return valid_http(url,&span);
}
static int hex(char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}
static gutc_status decode_form(const char *s,size_t n,char *out,size_t *needed) {
  size_t i,j=0,cp=0;
  if(!out||!needed)return GUTC_INVALID_ARGUMENT;
  for(i=0;i<n;i++){
    unsigned char c=(unsigned char)s[i];
    if(c=='%'){
      int a,b;
      if(n-i<3||(a=hex(s[i+1]))<0||(b=hex(s[i+2]))<0)
        return GUTC_INVALID_ENCODING;
      c=(unsigned char)((a<<4)|b);i+=2;
    } else if(c=='+') c=' ';
    out[j++]=(char)c;
  }
  if(gutc_utf8_validate((const uint8_t*)out,j,&cp)!=GUTC_OK)
    return GUTC_INVALID_ENCODING;
  out[j]='\0';*needed=j;
  return GUTC_OK;
}
static void emit(writer *w,const char *src,size_t n) {
  if(w->overflow)return;
  if(n>SIZE_MAX-w->used){w->overflow=1;return;}
  if(w->out&&n)memcpy(w->out+w->used,src,n);
  w->used+=n;
}
static void literal(writer *w,char c) {emit(w,&c,1);}
static void encode_form(writer *w,const char *str,size_t n) {
  static const char hexchars[]="0123456789ABCDEF";
  size_t i;
  for(i=0;i<n;i++) {
    unsigned char c=(unsigned char)str[i];
    if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
       (c>='0'&&c<='9')||c=='*'||c=='-'||c=='.'||c=='_')
      literal(w,(char)c);
    else if(c==' ')literal(w,'+');
    else {
      char p[3]={'%',hexchars[c>>4],hexchars[c&15]};
      emit(w,p,3);
    }
  }
}
static void pair_bounds(const char *url,size_t begin,size_t end,
                        size_t *key_end,size_t *val_begin) {
  size_t i;
  *key_end=end;*val_begin=end;
  for(i=begin;i<end;i++)if(url[i]=='='){
    *key_end=i;*val_begin=i+1;break;
  }
}
gutc_status gutc_url_get_query_param(const char *url,const char *key,
                                     char *out,size_t cap,size_t *needed,int *found) {
  url_span sp;
  char *scratch;
  size_t pos,klen=0;
  gutc_status rc=GUTC_OK;
  if(!needed||!found||(!out&&cap)||!bounded(key,&klen)||
     !valid_utf8(key,klen)||!valid_http(url,&sp))return GUTC_INVALID_ARGUMENT;
  scratch=(char*)malloc(sp.length+1);
  if(!scratch)return GUTC_NO_MEMORY;
  *found=0;*needed=0;
  pos=sp.query_start;
  while(pos<sp.query_end) {
    size_t end=pos,kend,vbegin,n;
    while(end<sp.query_end&&url[end]!='&')end++;
    if(end>pos) {
      pair_bounds(url,pos,end,&kend,&vbegin);
      rc=decode_form(url+pos,kend-pos,scratch,&n);
      if(rc!=GUTC_OK)break;
      if(n==klen && (n==0||memcmp(scratch,key,n)==0)) {
        rc=decode_form(url+vbegin,end-vbegin,scratch,&n);
        if(rc!=GUTC_OK)break;
        *found=1;*needed=n;
        if(out) {
          if(cap<=n){rc=GUTC_NO_SPACE;break;}
          memcpy(out,scratch,n);out[n]='\0';
        }
        break;
      }
    }
    pos=end+1;
  }
  free(scratch);
  return rc;
}
static gutc_status build_set(const char *url,url_span sp,
                             const char *key,size_t keylen,
                             const char *new_value,size_t value_len,
                             char *scratch_key,char *scratch_value,
                             writer *w) {
  size_t before=sp.has_query?sp.query_start-1:sp.fragment_start;
  size_t pos=sp.query_start;
  int written=0,replaced=0;
  gutc_status rc;
  emit(w,url,before);literal(w,'?');
  while(pos<sp.query_end) {
    size_t end=pos,kend,vbegin,keylen_dec,valuelen_dec;
    while(end<sp.query_end&&url[end]!='&')end++;
    if(end>pos) {
      pair_bounds(url,pos,end,&kend,&vbegin);
      rc=decode_form(url+pos,kend-pos,scratch_key,&keylen_dec);
      if(rc!=GUTC_OK)return rc;
      rc=decode_form(url+vbegin,end-vbegin,scratch_value,&valuelen_dec);
      if(rc!=GUTC_OK)return rc;
      if(keylen_dec==keylen &&
         (!keylen||memcmp(scratch_key,key,keylen)==0)) {
        if(!replaced) {
          if(written)literal(w,'&');
          encode_form(w,key,keylen);
          literal(w,'=');
          encode_form(w,new_value,value_len);
          replaced=1;written=1;
        }
      } else {
        if(written)literal(w,'&');
        encode_form(w,scratch_key,keylen_dec);
        literal(w,'=');
        encode_form(w,scratch_value,valuelen_dec);
        written=1;
      }
    }
    pos=end+1;
  }
  if(!replaced) {
    if(written)literal(w,'&');
    encode_form(w,key,keylen);
    literal(w,'=');
    encode_form(w,new_value,value_len);
  }
  emit(w,url+sp.fragment_start,sp.length-sp.fragment_start);
  return w->overflow?GUTC_OVERFLOW:GUTC_OK;
}
gutc_status gutc_url_set_query_param(const char *url,const char *key,
                                     const char *new_value,char *out,
                                     size_t cap,size_t *needed) {
  url_span sp;
  size_t keylen=0,valuelen=0;
  char *scratch_key,*scratch_value;
  writer w={NULL,0,0};
  gutc_status rc;
  if(!needed||(!out&&cap)||!bounded(key,&keylen)||!bounded(new_value,&valuelen)||
     !valid_utf8(key,keylen)||!valid_utf8(new_value,valuelen)||
     !valid_http(url,&sp))return GUTC_INVALID_ARGUMENT;
  scratch_key=(char*)malloc(sp.length+1);
  scratch_value=(char*)malloc(sp.length+1);
  if(!scratch_key||!scratch_value) {
    free(scratch_key);free(scratch_value);return GUTC_NO_MEMORY;
  }
  rc=build_set(url,sp,key,keylen,new_value,valuelen,scratch_key,scratch_value,&w);
  if(rc==GUTC_OK) {
    *needed=w.used;
    if(out) {
      if(w.used==SIZE_MAX)rc=GUTC_OVERFLOW;
      else if(cap<w.used+1)rc=GUTC_NO_SPACE;
      else {
        w.used=0;w.out=out;w.overflow=0;
        rc=build_set(url,sp,key,keylen,new_value,valuelen,scratch_key,scratch_value,&w);
        if(rc==GUTC_OK)out[w.used]='\0';
      }
    }
  }
  free(scratch_key);free(scratch_value);
  return rc;
}
