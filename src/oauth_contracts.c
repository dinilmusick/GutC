#include "gutc/oauth_contracts.h"
#include "gutc/gutc.h"
#include <stdint.h>
#include <string.h>
static gutc_status invalid(const char **reason,const char *message) {
  if(reason)*reason=message;
  return GUTC_INVALID_FORMAT;
}
static int eq(gutc_json_text a,gutc_json_text b) {
  return a.length==b.length &&
   (!a.length||(a.bytes&&b.bytes&&!memcmp(a.bytes,b.bytes,a.length)));
}
static int lit(gutc_json_text value,const char *text) {
  size_t n=strlen(text);
  return value.length==n && (!n || (value.bytes&&!memcmp(value.bytes,text,n)));
}
static const gutc_json_dom *get(const gutc_json_dom *o,const char *key) {
  return gutc_json_dom_get(o,key,strlen(key));
}
static int string_is(const gutc_json_dom *v,const char *s) {
  return v && v->kind==GUTC_JSON_STRING && lit(v->as.string,s);
}
static int valid_id(const gutc_json_dom *v) {
  return v && v->kind==GUTC_JSON_STRING &&
    gutc_identity_id_valid(v->as.string.bytes,v->as.string.length);
}
static int valid_app(const gutc_json_dom *v) {
  size_t i;int scheme;
  gutc_json_text s;
  if(!valid_id(v))return 0;
  s=v->as.string;
  scheme=(s.bytes[0]>='a'&&s.bytes[0]<='z') ||
         (s.bytes[0]>='A'&&s.bytes[0]<='Z');
  for(i=1;scheme&&i<s.length;i++){
    char c=s.bytes[i];
    if(c==':')return 0;
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
         (c>='0'&&c<='9')||c=='+'||c=='-'||c=='.'))scheme=0;
  }
  return 1;
}
static int whitelist(const gutc_json_dom *v,const char *const *keys,size_t n) {
  size_t i,j;
  if(!v||v->kind!=GUTC_JSON_OBJECT)return 0;
  for(i=0;i<v->as.object.count;i++){
    int found=0;
    for(j=0;j<n;j++)if(lit(v->as.object.members[i].key,keys[j])){found=1;break;}
    if(!found)return 0;
  }
  return 1;
}
static int good_host(gutc_json_text t) {
  size_t i;
  if(!t.bytes||!t.length||t.bytes[0]=='.')return 0;
  for(i=0;i<t.length;i++) {
    unsigned char c=(unsigned char)t.bytes[i];
    if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='-'))return 0;
    if(c=='.' && i && t.bytes[i-1]=='.')return 0;
  }
  return 1;
}
static int verified_hosts(const gutc_json_dom *v) {
  size_t i;
  if(!v)return 1;
  if(v->kind!=GUTC_JSON_ARRAY)return 0;
  for(i=0;i<v->as.array.count;i++){
    const gutc_json_dom *x=v->as.array.values[i];
    if(!x||x->kind!=GUTC_JSON_STRING||!good_host(x->as.string))return 0;
  }
  return 1;
}
static int host_allowed(gutc_json_text name,const gutc_json_dom *verified) {
  size_t i;
  if(!verified||verified->kind!=GUTC_JSON_ARRAY)return 0;
  for(i=0;i<verified->as.array.count;i++) {
    const gutc_json_dom *h=verified->as.array.values[i];
    if(h&&h->kind==GUTC_JSON_STRING && eq(name,h->as.string))return 1;
  }
  return 0;
}
static int parse_port(const char *s,size_t n,unsigned *result) {
  size_t i;unsigned value=0;
  if(!n||n>5)return 0;
  for(i=0;i<n;i++) {
    if(s[i]<'0'||s[i]>'9')return 0;
    value=value*10u+(unsigned)(s[i]-'0');
    if(value>65535u)return 0;
  }
  if(value==0)return 0;
  *result=value;return 1;
}
static int hex(char c) {
  return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
}
gutc_status gutc_validate_oauth_redirect_v1(const gutc_json_dom *uri,
                                              gutc_oauth_platform platform,
                                              const gutc_json_dom *verified,
                                              const char **reason) {
  gutc_json_text t,host;
  size_t i,begin,end,port_begin=0,path_begin;
  unsigned port=0;
  int http=0,https=0,ipv6=0;
  if(reason)*reason=NULL;
  if(!uri||uri->kind!=GUTC_JSON_STRING)
    return invalid(reason,"INVALID_REDIRECT_URI");
  t=uri->as.string;
  if(!t.bytes||t.length<8||t.length>2048)
    return invalid(reason,"INVALID_REDIRECT_URI");
  for(i=0;i<t.length;i++) {
    unsigned char c=(unsigned char)t.bytes[i];
    if(c<=32u||c>=127u||c=='*'||c=='#'||c=='\\')
      return invalid(reason,"INVALID_REDIRECT_URI");
  }
  if(t.length>=8 && !memcmp(t.bytes,"https://",8)){https=1;begin=8;}
  else if(t.length>=7 && !memcmp(t.bytes,"http://",7)){http=1;begin=7;}
  else return invalid(reason,"INVALID_REDIRECT_URI");
  end=begin;
  while(end<t.length && t.bytes[end]!='/'&&t.bytes[end]!='?'&&t.bytes[end]!='#')end++;
  if(end==begin)return invalid(reason,"INVALID_REDIRECT_URI");
  if(t.bytes[begin]=='[') {
    size_t close=begin;
    while(close<end&&t.bytes[close]!=']')close++;
    if(close==end||close==begin+1)return invalid(reason,"INVALID_REDIRECT_URI");
    host.bytes=t.bytes+begin;host.length=close-begin+1;
    ipv6=1;
    if(close+1<end) {
      if(t.bytes[close+1]!=':')return invalid(reason,"INVALID_REDIRECT_URI");
      port_begin=close+2;
    }
  } else {
    size_t colon=begin;
    while(colon<end&&t.bytes[colon]!=':')colon++;
    host.bytes=t.bytes+begin;host.length=colon-begin;
    if(colon<end)port_begin=colon+1;
    if(!good_host(host))return invalid(reason,"NON_CANONICAL_REDIRECT_URI");
  }
  if(port_begin && !parse_port(t.bytes+port_begin,end-port_begin,&port))
    return invalid(reason,"INVALID_REDIRECT_URI");
  if((http&&port==80u)||(https&&port==443u))
    return invalid(reason,"NON_CANONICAL_REDIRECT_URI");
  if(end==t.length||t.bytes[end]!='/'||
     (end+1<t.length && t.bytes[end+1]=='/'))
    return invalid(reason,"NON_CANONICAL_REDIRECT_URI");
  path_begin=end;
  for(i=path_begin;i<t.length;i++) {
    unsigned char c=(unsigned char)t.bytes[i];
    if(c=='?')return invalid(reason,"NON_CANONICAL_REDIRECT_URI");
    if(c=='%') {
      if(t.length-i<3||!hex(t.bytes[i+1])||!hex(t.bytes[i+2]))
        return invalid(reason,"NON_CANONICAL_REDIRECT_URI");
      i+=2;
    }
  }
  for(i=path_begin;i<t.length;) {
    size_t next=i+1,n;
    while(next<t.length&&t.bytes[next]!='/')next++;
    n=next-i-1;
    if((n==1&&t.bytes[i+1]=='.')||
       (n==2&&t.bytes[i+1]=='.'&&t.bytes[i+2]=='.'))
      return invalid(reason,"NON_CANONICAL_REDIRECT_URI");
    i=next;
  }
  if(platform==GUTC_OAUTH_DESKTOP) {
    if(!http||!port||t.length-path_begin<=1||
       !(lit(host,"127.0.0.1")||lit(host,"[::1]")))
      return invalid(reason,"DESKTOP_REQUIRES_EXACT_LOOPBACK_CALLBACK");
    return GUTC_OK;
  }
  if(!https||ipv6||lit(host,"localhost")||
     (host.length>=10 &&
      !memcmp(host.bytes+host.length-10,".localhost",10)))
    return invalid(reason,"WEB_OR_MOBILE_REQUIRES_HTTPS");
  if((platform==GUTC_OAUTH_IOS||platform==GUTC_OAUTH_ANDROID)&&
      !host_allowed(host,verified))
    return invalid(reason,"MOBILE_HOST_NOT_VERIFIED");
  if(platform!=GUTC_OAUTH_WEB&&platform!=GUTC_OAUTH_IOS&&
     platform!=GUTC_OAUTH_ANDROID)
    return invalid(reason,"INVALID_REDIRECT_URI");
  return GUTC_OK;
}
gutc_status gutc_validate_oauth_registration_v1(const gutc_json_dom *o,
                                                  const char **reason) {
  static const char *const fields[]={"schemaVersion","appId","clientId",
    "platform","publicClient","pkceMethod","redirectUris","verifiedHosts"};
  const gutc_json_dom *platform,*verified,*redirects,*version,*pub;
  gutc_oauth_platform code;
  size_t i,j;
  gutc_status rc;
  if(reason)*reason=NULL;
  if(!whitelist(o,fields,sizeof(fields)/sizeof(fields[0])))
    return invalid(reason,"INVALID_CLIENT_FIELDS");
  platform=get(o,"platform");verified=get(o,"verifiedHosts");
  redirects=get(o,"redirectUris");pub=get(o,"publicClient");
  version=get(o,"schemaVersion");
  if(!version||version->kind!=GUTC_JSON_NUMBER||version->as.number!=1||
     !valid_app(get(o,"appId"))||!valid_id(get(o,"clientId"))||
     !pub||pub->kind!=GUTC_JSON_BOOLEAN||!pub->as.boolean||
     !string_is(get(o,"pkceMethod"),"S256"))
    return invalid(reason,"INVALID_CLIENT_IDENTITY_OR_PKCE");
  if(string_is(platform,"web"))code=GUTC_OAUTH_WEB;
  else if(string_is(platform,"ios"))code=GUTC_OAUTH_IOS;
  else if(string_is(platform,"android"))code=GUTC_OAUTH_ANDROID;
  else if(string_is(platform,"desktop"))code=GUTC_OAUTH_DESKTOP;
  else return invalid(reason,"INVALID_CLIENT_IDENTITY_OR_PKCE");
  if(!verified_hosts(verified))return invalid(reason,"INVALID_VERIFIED_HOSTS");
  if(!redirects||redirects->kind!=GUTC_JSON_ARRAY||
     redirects->as.array.count==0||redirects->as.array.count>32)
    return invalid(reason,"INVALID_REDIRECT_SET");
  for(i=0;i<redirects->as.array.count;i++) {
    const gutc_json_dom *uri=redirects->as.array.values[i];
    for(j=0;j<i;j++) {
      const gutc_json_dom *prev=redirects->as.array.values[j];
      if(uri&&uri->kind==GUTC_JSON_STRING&&prev&&prev->kind==GUTC_JSON_STRING &&
         eq(uri->as.string,prev->as.string))
        return invalid(reason,"INVALID_REDIRECT_SET");
    }
  }
  for(i=0;i<redirects->as.array.count;i++) {
    rc=gutc_validate_oauth_redirect_v1(redirects->as.array.values[i],code,verified,reason);
    if(rc!=GUTC_OK)return rc;
  }
  return GUTC_OK;
}
