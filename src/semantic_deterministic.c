#include "gutc/semantic_deterministic.h"
#include "gutc/json.h"
#include "gut_string_predicates.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static int eq(const gutc_json_dom *v,const char *name) {
  size_t n=strlen(name);
  return v && v->kind==GUTC_JSON_STRING && v->as.string.length==n &&
    (n==0||memcmp(v->as.string.bytes,name,n)==0);
}
static const gutc_json_dom *field(const gutc_json_dom *o,const char *name) {
  return gutc_json_dom_get(o,name,strlen(name));
}
static int ascii_lower(int c) {return c>='A'&&c<='Z'?c+('a'-'A'):c;}
static int ci_equal(const char *s,size_t n,const char *wanted) {
  size_t i,len=strlen(wanted);
  if(n!=len)return 0;
  for(i=0;i<n;i++)if(ascii_lower((unsigned char)s[i])!=ascii_lower((unsigned char)wanted[i]))return 0;
  return 1;
}
static int size_cp(unsigned char c) {
  if(c<0x80u)return 1;
  if(c>=0xC2u&&c<=0xDFu)return 2;
  if(c>=0xE0u&&c<=0xEFu)return 3;
  if(c>=0xF0u&&c<=0xF4u)return 4;
  return 0;
}
static gutc_status trim(const char *input,size_t n,size_t *start,size_t *end) {
  size_t off=0,begin=SIZE_MAX,last=0,points=0;
  gutc_status rc;
  rc=gutc_utf8_validate((const uint8_t*)input,n,&points);
  if(rc!=GUTC_OK)return rc;
  while(off<n){
    int width=size_cp((unsigned char)input[off]);
    int blank;
    if(width==0||(size_t)width>n-off)return GUTC_INVALID_ENCODING;
    blank=gut_is_blank_utf8_n(input+off,(size_t)width);
    if(blank<0)return GUTC_INVALID_ARGUMENT;
    if(!blank) {
      if(begin==SIZE_MAX)begin=off;
      last=off+(size_t)width;
    }
    off+=(size_t)width;
  }
  *start=begin==SIZE_MAX?n:begin;
  *end=last;
  return GUTC_OK;
}
static int email_format(const char *s,size_t n) {
  size_t i,at=n,dot=n;
  for(i=0;i<n;){
    int width=size_cp((unsigned char)s[i]);
    if(width==0||(size_t)width>n-i)return 0;
    if(gut_is_blank_utf8_n(s+i,(size_t)width)!=0)return 0;
    if(width==1&&s[i]=='@'){
      if(at!=n)return 0;
      at=i;
    }
    i+=(size_t)width;
  }
  if(at==0||at==n||at+1==n)return 0;
  for(i=at+1;i<n;i++)if(s[i]=='.')dot=i;
  return dot>at+1 && dot<n-1;
}
static int uri_format(const char *s,size_t n) {
  size_t i,req;
  const char *http="http://",*https="https://";
  if(n>=7){
    req=7;
    for(i=0;i<req;i++)if(ascii_lower((unsigned char)s[i])!=http[i])break;
    if(i==req)return 1;
  }
  if(n>=8){
    req=8;
    for(i=0;i<req;i++)if(ascii_lower((unsigned char)s[i])!=https[i])break;
    if(i==req)return 1;
  }
  return 0;
}
static int digit(char c){return c>='0'&&c<='9';}
static int date_prefix(const char *s,size_t n) {
  size_t i;
  if(n<10)return 0;
  for(i=0;i<10;i++){
    if(i==4||i==7){if(s[i]!='-')return 0;}
    else if(!digit(s[i]))return 0;
  }
  return 1;
}
static int parse_number(const char *s,size_t n,double *value) {
  size_t i=0;
  char copy[8193],*end;
  double out=0;
  int base=10,any=0;
  if(!n||n>8192)return 0;
  if(n>2&&s[0]=='0'&&(s[1]=='x'||s[1]=='X'||s[1]=='b'||s[1]=='B'||s[1]=='o'||s[1]=='O')) {
    base=s[1]=='x'||s[1]=='X'?16:(s[1]=='b'||s[1]=='B'?2:8);
    for(i=2;i<n;i++){
      unsigned char c=(unsigned char)s[i];
      int d=c>='0'&&c<='9'?c-'0':
        (c>='a'&&c<='f'?c-'a'+10:(c>='A'&&c<='F'?c-'A'+10:-1));
      if(d<0||d>=base)return 0;
      out=out*(double)base+(double)d;
      if(!isfinite(out))return 0;
      any=1;
    }
    if(!any)return 0;
    *value=out;return 1;
  }
  if(s[i]=='+'||s[i]=='-')i++;
  while(i<n&&digit(s[i])){i++;any=1;}
  if(i<n&&s[i]=='.'){
    i++;
    while(i<n&&digit(s[i])){i++;any=1;}
  }
  if(!any)return 0;
  if(i<n&&(s[i]=='e'||s[i]=='E')){
    int exp_digits=0;
    i++;
    if(i<n&&(s[i]=='+'||s[i]=='-'))i++;
    while(i<n&&digit(s[i])){i++;exp_digits=1;}
    if(!exp_digits)return 0;
  }
  if(i!=n)return 0;
  memcpy(copy,s,n);copy[n]='\0';
  out=strtod(copy,&end);
  if(end!=copy+n||!isfinite(out))return 0;
  *value=out;return 1;
}
static gutc_status make_result(const char *literal,size_t n,gutc_json_dom **output) {
  static const char p[]="{\"status\":\"resolved\",\"value\":";
  static const char middle[]=",\"candidates\":[],\"modelOutput\":";
  static const char end[]="}";
  const size_t fixed=sizeof(p)-1+sizeof(middle)-1+sizeof(end)-1;
  size_t total,at=0;
  char *buf;
  gutc_status rc;
  if(n>(SIZE_MAX-fixed)/2)return GUTC_OVERFLOW;
  total=fixed+2*n;
  buf=(char*)malloc(total+1);
  if(!buf)return GUTC_NO_MEMORY;
  memcpy(buf+at,p,sizeof(p)-1);at+=sizeof(p)-1;
  memcpy(buf+at,literal,n);at+=n;
  memcpy(buf+at,middle,sizeof(middle)-1);at+=sizeof(middle)-1;
  memcpy(buf+at,literal,n);at+=n;
  memcpy(buf+at,end,sizeof(end)-1);at+=sizeof(end)-1;
  buf[at]='\0';
  rc=gutc_json_dom_parse((const uint8_t*)buf,at,16,1000,output,NULL);
  free(buf);
  return rc;
}
gutc_status gutc_resolve_deterministic_semantic(const char *input,size_t n,
                                                const gutc_json_dom *schema,
                                                gutc_json_dom **output,int *resolved) {
  const gutc_json_dom *type,*enumeration,*format,*pattern;
  size_t start=0,end=0,m,i,quoted_size=0;
  const char *str;
  char numeric[90];
  char *escaped=NULL;
  const char *literal=NULL;
  gutc_status rc;
  double value=0;
  if(!output||!resolved)return GUTC_INVALID_ARGUMENT;
  *output=NULL;*resolved=0;
  if((!input&&n)||n>8192||!schema||schema->kind!=GUTC_JSON_OBJECT)return GUTC_INVALID_ARGUMENT;
  rc=trim(input,n,&start,&end);
  if(rc!=GUTC_OK)return rc;
  if(start>=end)return GUTC_OK;
  str=input+start;m=end-start;
  type=field(schema,"type");
  enumeration=field(schema,"enum");
  if(enumeration&&enumeration->kind==GUTC_JSON_ARRAY){
    for(i=0;i<enumeration->as.array.count;i++){
      const gutc_json_dom *entry=enumeration->as.array.values[i];
      if(entry&&entry->kind==GUTC_JSON_STRING&&entry->as.string.length==m&&
        !memcmp(entry->as.string.bytes,str,m)) {
        goto resolved_string;
      }
    }
  }
  if(eq(type,"string")){
    format=field(schema,"format");pattern=field(schema,"pattern");
    if(eq(format,"email")&&email_format(str,m))goto resolved_string;
    if(eq(format,"uri")&&uri_format(str,m))goto resolved_string;
    if(eq(format,"date")&&m==10&&date_prefix(str,m))goto resolved_string;
    if(eq(format,"date-time")&&m>=11&&date_prefix(str,m)&&str[10]=='T')goto resolved_string;
    if(pattern&&pattern->kind==GUTC_JSON_STRING)return GUTC_UNSUPPORTED;
    return GUTC_OK;
  }
  if(eq(type,"boolean")){
    if(ci_equal(str,m,"true"))literal="true";
    else if(ci_equal(str,m,"false"))literal="false";
    else return GUTC_OK;
    quoted_size=strlen(literal);
  } else if(eq(type,"number")||eq(type,"integer")){
    int k;
    if(!parse_number(str,m,&value))return GUTC_OK;
    if(eq(type,"integer")&&floor(value)!=value)return GUTC_OK;
    if(value==0)literal="0";
    else {
      k=snprintf(numeric,sizeof numeric,"%.17g",value);
      if(k<=0||(size_t)k>=sizeof numeric)return GUTC_OVERFLOW;
      literal=numeric;
    }
    quoted_size=strlen(literal);
  } else return GUTC_OK;
  rc=make_result(literal,quoted_size,output);
  if(rc==GUTC_OK)*resolved=1;
  return rc;
resolved_string:
  rc=gutc_json_quote_string((const uint8_t*)str,m,NULL,0,&quoted_size);
  if(rc!=GUTC_OK)return rc;
  if(quoted_size==SIZE_MAX)return GUTC_OVERFLOW;
  escaped=(char*)malloc(quoted_size+1);
  if(!escaped)return GUTC_NO_MEMORY;
  rc=gutc_json_quote_string((const uint8_t*)str,m,escaped,quoted_size+1,&quoted_size);
  if(rc==GUTC_OK)rc=make_result(escaped,quoted_size,output);
  free(escaped);
  if(rc==GUTC_OK)*resolved=1;
  return rc;
}
