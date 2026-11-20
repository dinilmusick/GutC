#include "gutc/luth_origin.h"
#include <stdint.h>
#include <string.h>

#define MAX_ORIGIN_INPUT 8192u
#define MAX_HOST 2048u
typedef struct origin_details {
  char scheme[6];
  char host[MAX_HOST+1];
  size_t host_length;
  unsigned port;
  int has_port;
  int ipv6;
  int valid;
} origin_details;
static int lower(int c){return (c>='A'&&c<='Z')?c+('a'-'A'):c;}
static int ascii_blank(unsigned char c){
  return c==9||c==10||c==11||c==12||c==13||c==32;
}
static int digit(int c){return c>='0'&&c<='9';}
static int alpha(int c){c=lower(c);return c>='a'&&c<='z';}
static int starts(const char *s,size_t n,const char *wanted){
  size_t i,len=strlen(wanted);
  if(n<len)return 0;
  for(i=0;i<len;i++)if(lower((unsigned char)s[i])!=wanted[i])return 0;
  return 1;
}
static int contains(const char *s,size_t n,const char *needle){
  size_t i,len=strlen(needle);
  if(!len)return 1;
  if(len>n)return 0;
  for(i=0;i<=n-len;i++)if(!memcmp(s+i,needle,len))return 1;
  return 0;
}
static int ends(const char *s,size_t n,const char *ending){
  size_t len=strlen(ending);
  return n>=len&&!memcmp(s+n-len,ending,len);
}
static int strlen_bounded(const char *s,size_t *length){
  size_t i;
  if(!s)return 0;
  for(i=0;i<MAX_ORIGIN_INPUT;i++)if(!s[i]){*length=i;return 1;}
  return 0;
}
static void trim(const char *s,size_t n,size_t *first,size_t *last){
  size_t a=0,b=n;
  while(a<b && ascii_blank((unsigned char)s[a]))a++;
  while(b>a && ascii_blank((unsigned char)s[b-1]))b--;
  *first=a;*last=b;
}
static gutc_status parse_origin(const char *input,origin_details *o){
  size_t n=0,a=0,b=0,begin,authority_end,host_end,i,port_begin;
  int https;
  memset(o,0,sizeof(*o));
  if(!input)return GUTC_OK;
  if(!strlen_bounded(input,&n))return GUTC_RANGE_ERROR;
  trim(input,n,&a,&b);
  if(a==b)return GUTC_OK;
  if(b-a<7)return GUTC_OK;
  https=starts(input+a,b-a,"https://");
  if(https){
    memcpy(o->scheme,"https",6);begin=a+8;
  }else if(starts(input+a,b-a,"http://")){
    memcpy(o->scheme,"http",5);begin=a+7;
  }else {
    /* Non-special WHATWG URLs can have opaque/null origins. Reject instead
       of misrepresenting the browser URL constructor. */
    for(i=a;i<b;i++)if(input[i]==':')return GUTC_UNSUPPORTED;
    return GUTC_OK;
  }
  if(begin>=b)return GUTC_OK;
  authority_end=begin;
  while(authority_end<b&&input[authority_end]!='/'&&
        input[authority_end]!='?'&&input[authority_end]!='#'){
    unsigned char c=(unsigned char)input[authority_end];
    if(c=='@'||c=='\\'||c=='%')return GUTC_UNSUPPORTED;
    if(c<=32u||c>=127u)return GUTC_UNSUPPORTED;
    authority_end++;
  }
  if(authority_end==begin)return GUTC_OK;
  for(i=authority_end;i<b;i++){
    unsigned char c=(unsigned char)input[i];
    if(c=='\\'||c<=31u||c==127u)return GUTC_UNSUPPORTED;
  }
  host_end=authority_end;port_begin=authority_end;
  if(input[begin]=='['){
    size_t close=begin;
    while(close<authority_end && input[close]!=']')close++;
    if(close==authority_end)return GUTC_OK;
    /* JS URL canonicalizes IPv6 formats and compresses zeros.
       Support only the canonical loopback literal without guessing. */
    if(close-begin!=4||memcmp(input+begin,"[::1]",5))return GUTC_UNSUPPORTED;
    o->ipv6=1;host_end=close+1;
    if(host_end<authority_end&&input[host_end]!=':')return GUTC_OK;
    port_begin=host_end<authority_end?host_end+1:authority_end;
  }else{
    size_t colon=begin;
    while(colon<authority_end&&input[colon]!=':')colon++;
    host_end=colon;
    port_begin=colon<authority_end?colon+1:authority_end;
  }
  if(host_end==begin||host_end-begin>MAX_HOST)return GUTC_OK;
  o->host_length=host_end-begin;
  for(i=0;i<o->host_length;i++){
    unsigned char c=(unsigned char)input[begin+i];
    if(!o->ipv6 && !(alpha(c)||digit(c)||c=='.'||c=='-'||c=='_'))
      return GUTC_UNSUPPORTED;
    o->host[i]=(char)lower(c);
  }
  o->host[o->host_length]='\0';
  if(!o->ipv6){
    if(contains(o->host,o->host_length,"..")||o->host[0]=='.')return GUTC_OK;
    if(starts(o->host,o->host_length,"xn--"))return GUTC_UNSUPPORTED;
    /* WHATWG numeric host aliases/abbreviations are not plain DNS hosts. */
    if(starts(o->host,o->host_length,"0x"))return GUTC_UNSUPPORTED;
  }
  if(port_begin<authority_end){
    unsigned port=0;
    if(!digit((unsigned char)input[port_begin]))return GUTC_OK;
    for(i=port_begin;i<authority_end;i++){
      if(!digit((unsigned char)input[i]))return GUTC_OK;
      if(port>6553u)return GUTC_OK;
      port=port*10u+(unsigned)(input[i]-'0');
      if(port>65535u)return GUTC_OK;
    }
    o->has_port=1;o->port=port;
  }else if(!o->ipv6 && host_end<authority_end) {
    return GUTC_OK;
  }else if(o->ipv6 && host_end<authority_end) {
    return GUTC_OK;
  }
  if(o->has_port&&((https&&o->port==443u)||(!https&&o->port==80u)))
    o->has_port=0;
  o->valid=1;
  return GUTC_OK;
}
static gutc_status render_origin(const origin_details *o,char *output,
                                 size_t capacity,size_t *needed){
  char buffer[2120];
  size_t pos=0,i;
  unsigned port;
  char rev[6];size_t digits=0;
  if(!needed||(!output&&capacity))return GUTC_INVALID_ARGUMENT;
  if(o->valid){
    size_t n=strlen(o->scheme);
    memcpy(buffer+pos,o->scheme,n);pos+=n;
    memcpy(buffer+pos,"://",3);pos+=3;
    memcpy(buffer+pos,o->host,o->host_length);pos+=o->host_length;
    if(o->has_port){
      buffer[pos++]=':';
      port=o->port;
      do{rev[digits++]=(char)('0'+port%10u);port/=10u;}while(port);
      for(i=0;i<digits;i++)buffer[pos++]=rev[digits-i-1];
    }
  }
  *needed=pos;
  if(!output)return GUTC_OK;
  if(capacity<=pos)return GUTC_NO_SPACE;
  if(pos)memcpy(output,buffer,pos);
  output[pos]='\0';
  return GUTC_OK;
}
const char *gutc_luth_default_url(void){
  return "https://luth.blueprint-ninjastudios.workers.dev";
}
const char *gutc_luth_environment_name(gutc_luth_environment v){
  switch(v){
    case GUTC_LUTH_DEVELOPMENT:return "development";
    case GUTC_LUTH_STAGING:return "staging";
    case GUTC_LUTH_PREVIEW:return "preview";
    case GUTC_LUTH_PRODUCTION:return "production";
    default:return NULL;
  }
}
gutc_status gutc_luth_normalize_origin(const char *input,char *out,
                                       size_t capacity,size_t *needed){
  origin_details details;
  gutc_status rc;
  if(!needed||(!out&&capacity))return GUTC_INVALID_ARGUMENT;
  rc=parse_origin(input,&details);
  if(rc!=GUTC_OK)return rc;
  return render_origin(&details,out,capacity,needed);
}
gutc_status gutc_luth_resolve_client_origin(const char *explicit_origin,
                                            const char *runtime_origin,
                                            char *out,size_t capacity,size_t *needed){
  size_t n=0,a,b;
  if(explicit_origin){
    if(!strlen_bounded(explicit_origin,&n))return GUTC_RANGE_ERROR;
    trim(explicit_origin,n,&a,&b);
    if(a!=b)return gutc_luth_normalize_origin(explicit_origin,out,capacity,needed);
  }
  return gutc_luth_normalize_origin(runtime_origin,out,capacity,needed);
}
gutc_status gutc_luth_resolve_client_environment(const char *origin,
                                                  gutc_luth_environment *out){
  origin_details details;
  gutc_status rc;
  if(!out)return GUTC_INVALID_ARGUMENT;
  rc=parse_origin(origin,&details);
  if(rc!=GUTC_OK)return rc;
  if(!details.valid){*out=GUTC_LUTH_PRODUCTION;return GUTC_OK;}
  if(!strcmp(details.host,"localhost")||!strcmp(details.host,"127.0.0.1")||
     !strcmp(details.host,"[::1]")){
    *out=GUTC_LUTH_DEVELOPMENT;return GUTC_OK;
  }
  if(contains(details.host,details.host_length,"staging")){
    *out=GUTC_LUTH_STAGING;return GUTC_OK;
  }
  if(contains(details.host,details.host_length,"preview")||
     contains(details.host,details.host_length,"-git-")||
     ends(details.host,details.host_length,".workers.dev")){
    *out=GUTC_LUTH_PREVIEW;return GUTC_OK;
  }
  *out=GUTC_LUTH_PRODUCTION;
  return GUTC_OK;
}
