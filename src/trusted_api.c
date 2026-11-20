#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
/* 0 success, -1 invalid, -2 insufficient capacity. No partial writes. */
static int hex(unsigned char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}
static int bad(unsigned char c) { return c<32||c==127||c=='\\'||c=='#'; }
static int segment(const char *s,size_t n) {
  char a[4096],b[4096]; size_t i,k; int depth,decoded;
  if(!n||n>=sizeof(a))return 0;
  memcpy(a,s,n);
  for(depth=0;depth<8;depth++) {
    k=0;decoded=0;
    for(i=0;i<n;i++){
      unsigned char c=(unsigned char)a[i];
      if(c=='%'){
        int h,l;
        if(i+2>=n||(h=hex((unsigned char)a[i+1]))<0||(l=hex((unsigned char)a[i+2]))<0)return 0;
        c=(unsigned char)(h*16+l);i+=2;decoded=1;
      }
      if(bad(c)||c=='?'||c==0)return 0;
      b[k++]=(char)c;
    }
    for(i=0;i<k;){
      size_t st=i;
      while(i<k&&b[i]!='/')i++;
      if((i-st==1&&b[st]=='.')||(i-st==2&&b[st]=='.'&&b[st+1]=='.'))return 0;
      i++;
    }
    memcpy(a,b,k);n=k;
    if(!decoded)return 1;
  }
  return 0;
}
static int origin_ok(const char *s,size_t *length){
  const char *p,*port=0;size_t n;
  if(!s||strncmp(s,"https://",8))return 0;
  n=strlen(s);if(n<=8)return 0;
  for(p=s+8;*p;p++)
    if((unsigned char)*p<=32||(unsigned char)*p>=127||strchr("@/?#\\%",*p))return 0;
  p=s+8;
  if(*p=='['){
    const char *end=strchr(p,']'),*q;
    if(!end||end==p+1)return 0;
    for(q=p+1;q<end;q++)if(!isxdigit((unsigned char)*q)&&*q!=':'&&*q!='.')return 0;
    if(end[1]&&end[1]!=':')return 0;
    if(end[1]==':')port=end+2;
  }else{
    const char *start=p;
    for(;*p&&*p!=':';p++)if(!isalnum((unsigned char)*p)&&*p!='.'&&*p!='-')return 0;
    if(p==start||*start=='.'||*start=='-'||p[-1]=='.'||p[-1]=='-')return 0;
    if(*p==':')port=p+1;
  }
  if(port){
    unsigned long value=0;
    if(!*port)return 0;
    for(;*port;port++){
      if(!isdigit((unsigned char)*port))return 0;
      value=value*10+(unsigned)(*port-'0');
      if(value>65535)return 0;
    }
    if(!value)return 0;
  }
  *length=n;return 1;
}
int gutc_resolve_trusted_api_url(const char *origin,const char *path,
                                char *output,size_t capacity,size_t *required){
  size_t olen,plen,pathlen,start,i,total;
  if(!origin_ok(origin,&olen)||!path||path[0]!='/'||path[1]=='/'||
     strchr(path,'#')||strchr(path,'\\'))return -1;
  plen=strlen(path);pathlen=strcspn(path,"?");
  for(i=0;i<plen;i++)if(bad((unsigned char)path[i]))return -1;
  for(start=1;start<pathlen;){
    size_t end=start;
    while(end<pathlen&&path[end]!='/')end++;
    if(end>start&&!segment(path+start,end-start))return -1;
    start=end+1;
  }
  if(olen>SIZE_MAX-plen-1)return -1;
  total=olen+plen+1;
  if(required)*required=total;
  if(!output||capacity<total)return -2;
  memcpy(output,origin,olen);memcpy(output+olen,path,plen+1);
  return 0;
}
