#include "gutc/safe_path.h"
#include "gutc_path.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static int bounded_len(const char *text,size_t *len) {
  size_t i;
  if(!text||!len)return 0;
  for(i=0;i<1048576u;i++) if(text[i]=='\0') {*len=i;return 1;}
  return 0;
}
static int unreserved_js(unsigned char c) {
  return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||
         (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~'||
         c=='!'||c=='\''||c=='('||c==')'||c=='*';
}
static int hex(char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}
static int has_escape(const char *s,size_t n) {
  size_t i;
  for(i=0;i+2<n;i++) if(s[i]=='%'&&hex(s[i+1])>=0&&hex(s[i+2])>=0)return 1;
  return 0;
}
/* Reject invalid Unicode in every decoding layer of a storage component.
 * URL decoding is byte-oriented, but JS decodeURIComponent is UTF-8 aware. */
static int valid_layers(const char *segment,size_t len) {
  char *a,*b;
  size_t points,need=0,i,n=len;
  int good=0;
  a=(char*)malloc(len+1);
  b=(char*)malloc(len+1);
  if(!a||!b)goto done;
  memcpy(a,segment,len);
  a[len]='\0';
  for(i=0;i<8;i++) {
    if(gutc_utf8_validate((const uint8_t*)a,n,&points)!=GUTC_OK)goto done;
    if(!has_escape(a,n)){good=1;break;}
    if(gutc_percent_decode(a,n,(uint8_t*)b,len,&need)!=GUTC_OK)goto done;
    if(memchr(b,0,need)!=NULL)goto done;
    n=need;
    {char *tmp=a;a=b;b=tmp;}
  }
  if(i==8 && !has_escape(a,n) &&
     gutc_utf8_validate((const uint8_t*)a,n,&points)==GUTC_OK) good=1;
done:
  free(a);free(b);
  return good;
}
gutc_status gutc_encode_safe_path_segments(const char *in,char *out,
                                          size_t capacity,size_t *needed) {
  static const char hexchars[]="0123456789ABCDEF";
  size_t in_len=0,n=0,points=0,i,j,required=0,at=0;
  char *normalized=NULL;
  gutc_status status=GUTC_INVALID_ARGUMENT;
  if(!needed || !bounded_len(in,&in_len) || (out==NULL&&capacity!=0))return status;
  if(in_len==SIZE_MAX)return GUTC_OVERFLOW;
  normalized=(char*)malloc(in_len+1);
  if(!normalized)return GUTC_OVERFLOW;
  if(gutc_normalize_storage_path(in,normalized,in_len+1)!=GUTC_PATH_OK) {
    status=GUTC_INVALID_FORMAT; goto done;
  }
  n=strlen(normalized);
  if(!n) {status=GUTC_INVALID_FORMAT;goto done;}
  if(gutc_utf8_validate((const uint8_t*)normalized,n,&points)!=GUTC_OK) {
    status=GUTC_INVALID_ENCODING;goto done;
  }
  for(i=0;i<n;) {
    size_t start=i;
    while(i<n&&normalized[i]!='/')i++;
    if(!valid_layers(normalized+start,i-start)){
      status=GUTC_INVALID_ENCODING;goto done;
    }
    if(i<n)i++;
  }
  for(i=0;i<n;i++) {
    unsigned char c=(unsigned char)normalized[i];
    size_t step=(unreserved_js(c)||c=='/')?1u:3u;
    if(step>SIZE_MAX-required){status=GUTC_OVERFLOW;goto done;}
    required+=step;
  }
  *needed=required;
  if(!out){status=GUTC_OK;goto done;}
  if(required==SIZE_MAX){status=GUTC_OVERFLOW;goto done;}
  if(capacity<required+1){status=GUTC_NO_SPACE;goto done;}
  for(j=0;j<n;j++) {
    unsigned char c=(unsigned char)normalized[j];
    if(unreserved_js(c)||c=='/')out[at++]=(char)c;
    else{
      out[at++]='%';
      out[at++]=hexchars[c>>4];
      out[at++]=hexchars[c&15u];
    }
  }
  out[at]='\0';
  status=GUTC_OK;
done:
  free(normalized);
  return status;
}
