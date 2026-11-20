#include "gutc/string_case.h"
#include <stdint.h>
#include <string.h>
static int upper(unsigned char c){return c>='A'&&c<='Z';}
static int lower(unsigned char c){return c>='a'&&c<='z';}
static int letter(unsigned char c){return upper(c)||lower(c);}
static int digit(unsigned char c){return c>='0'&&c<='9';}
static int alpha_num(unsigned char c){return letter(c)||digit(c);}
static unsigned char to_lower(unsigned char c) {
  return upper(c)?(unsigned char)(c+('a'-'A')):c;
}
static unsigned char to_upper(unsigned char c) {
  return lower(c)?(unsigned char)(c-('a'-'A')):c;
}
static gutc_status write_transform(const char *s,size_t n,gutc_string_case kind,
                                   char *out,size_t *size) {
  size_t i,j=0;
  int prev_word=0,any_word=0;
  unsigned char previous=0;
  if(kind==GUTC_CASE_CAPITALIZE){
    for(i=0;i<n;i++){
      unsigned char c=(unsigned char)s[i];
      if(c>=128u)return GUTC_UNSUPPORTED;
      if(c==0)return GUTC_INVALID_ARGUMENT;
      if(out)out[j]=(char)(i==0?to_upper(c):to_lower(c));
      j++;
    }
    *size=j;return GUTC_OK;
  }
  for(i=0;i<n;i++){
    unsigned char c=(unsigned char)s[i],next=i+1<n?(unsigned char)s[i+1]:0;
    int first,transition;
    if(c>=128u)return GUTC_UNSUPPORTED;
    if(c==0)return GUTC_INVALID_ARGUMENT;
    if(!alpha_num(c)){prev_word=0;previous=0;continue;}
    transition=prev_word &&
      ((upper(c)&&(lower(previous)||digit(previous))) ||
       (upper(c)&&upper(previous)&&lower(next)));
    first=!prev_word||transition;
    if(first&&any_word&&(kind==GUTC_CASE_KEBAB||kind==GUTC_CASE_SNAKE)) {
      if(out)out[j]=(kind==GUTC_CASE_KEBAB?'-':'_');
      j++;
    }
    if(first&&kind==GUTC_CASE_CAMEL&&any_word)c=to_upper(c);
    else c=to_lower(c);
    if(out)out[j]=(char)c;
    j++;
    any_word=1;prev_word=1;previous=(unsigned char)s[i];
  }
  *size=j;return GUTC_OK;
}
gutc_status gutc_string_transform_ascii(const char *s,size_t n,gutc_string_case kind,
                                         char *out,size_t cap,size_t *needed) {
  size_t length=0,actual=0;
  gutc_status rc;
  if(!needed||(!s&&n)||(!out&&cap)||kind<GUTC_CASE_CAPITALIZE||
     kind>GUTC_CASE_SNAKE)return GUTC_INVALID_ARGUMENT;
  rc=write_transform(s,n,kind,NULL,&length);
  if(rc!=GUTC_OK)return rc;
  *needed=length;
  if(!out)return GUTC_OK;
  if(length==SIZE_MAX)return GUTC_OVERFLOW;
  if(cap<length+1)return GUTC_NO_SPACE;
  rc=write_transform(s,n,kind,out,&actual);
  if(rc!=GUTC_OK)return rc;
  out[actual]='\0';
  return GUTC_OK;
}
static int allowed_local(unsigned char c){
  return alpha_num(c)||c=='.'||c=='!'||c=='#'||c=='$'||
    c=='%'||c=='&'||c=='\''||c=='*'||c=='+'||c=='-'||
    c=='/'||c=='='||c=='?'||c=='^'||c=='_'||c==0x60u||
    c=='{'||c=='|'||c=='}'||c=='~';
}
gutc_status gutc_is_email_ascii(const char *s,size_t n,int *result) {
  size_t i,at=n,local_len,label_len=0,label_start=0,tld_len=0;
  int found_dot=0,tld_has_letter=0;
  if(!result||(!s&&n))return GUTC_INVALID_ARGUMENT;
  *result=0;
  for(i=0;i<n;i++){
    unsigned char c=(unsigned char)s[i];
    if(c>=128u)return GUTC_UNSUPPORTED;
    if(c==0)return GUTC_INVALID_ARGUMENT;
    if(c=='"'||c=='\\')return GUTC_UNSUPPORTED;
    if(c=='@'){
      if(at!=n)return GUTC_OK;
      at=i;
    }
  }
  if(at==n||n>254||at==0||at>64||at+1>=n)return GUTC_OK;
  local_len=at;
  if(s[0]=='.'||s[local_len-1]=='.')return GUTC_OK;
  for(i=0;i<local_len;i++){
    unsigned char c=(unsigned char)s[i];
    if(!allowed_local(c)||(c=='.'&&i&&s[i-1]=='.'))return GUTC_OK;
  }
  label_start=at+1;
  for(i=at+1;i<=n;i++) {
    unsigned char c=i<n?(unsigned char)s[i]:'.';
    if(c=='.') {
      if(i==label_start||label_len>63)return GUTC_OK;
      if(s[label_start]=='-'||s[i-1]=='-')return GUTC_OK;
      if(i!=n)found_dot=1;
      if(i==n)tld_len=label_len;
      label_start=i+1;label_len=0;
    } else {
      if(!alpha_num(c)&&c!='-')return GUTC_OK;
      label_len++;
    }
  }
  if(!found_dot||tld_len<2)return GUTC_OK;
  for(i=n-tld_len;i<n;i++)if(letter((unsigned char)s[i]))tld_has_letter=1;
  *result=tld_has_letter;
  return GUTC_OK;
}
