#include "gutc/json.h"
#include <string.h>
#include <stdint.h>

typedef struct json_cursor {
  const uint8_t *s;
  size_t n, i;
  unsigned limit;
} json_cursor;
static int space(uint8_t c) { return c==' '||c=='\t'||c=='\r'||c=='\n'; }
static int digit(uint8_t c) { return c>='0'&&c<='9'; }
static int hd(uint8_t c) {
  if(c>='0'&&c<='9') return c-'0';
  if(c>='a'&&c<='f') return c-'a'+10;
  if(c>='A'&&c<='F') return c-'A'+10;
  return -1;
}
static void ws(json_cursor *p) { while(p->i<p->n && space(p->s[p->i])) p->i++; }
static gutc_status json_string(json_cursor *p) {
  size_t size=0,k;
  uint8_t c;
  if(p->i>=p->n || p->s[p->i]!='"') return GUTC_INVALID_FORMAT;
  ++p->i;
  while(p->i<p->n) {
    c=p->s[p->i++];
    if(c=='"') return GUTC_OK;
    if(c<0x20u) {p->i--;return GUTC_INVALID_FORMAT;}
    if(c=='\\') {
      if(p->i==p->n) return GUTC_INVALID_FORMAT;
      c=p->s[p->i++];
      if(c=='"'||c=='\\'||c=='/'||c=='b'||c=='f'||c=='n'||c=='r'||c=='t') continue;
      if(c!='u') {--p->i;return GUTC_INVALID_FORMAT;}
      if(p->n-p->i<4) return GUTC_INVALID_FORMAT;
      for(k=0;k<4;k++) if(hd(p->s[p->i+k])<0) return GUTC_INVALID_FORMAT;
      p->i+=4;
      continue;
    }
    if(c<0x80u) continue;
    if(c>=0xC2u&&c<=0xDFu) k=2;
    else if(c>=0xE0u&&c<=0xEFu) k=3;
    else if(c>=0xF0u&&c<=0xF4u) k=4;
    else {--p->i;return GUTC_INVALID_ENCODING;}
    --p->i;
    if(k>p->n-p->i) return GUTC_INVALID_ENCODING;
    if(gutc_utf8_validate(p->s+p->i,k,&size)!=GUTC_OK) return GUTC_INVALID_ENCODING;
    p->i+=k;
  }
  return GUTC_INVALID_FORMAT;
}
static gutc_status json_number(json_cursor *p) {
  size_t i=p->i;
  if(i<p->n && p->s[i]=='-') ++i;
  if(i==p->n) {p->i=i;return GUTC_INVALID_FORMAT;}
  if(p->s[i]=='0') ++i;
  else {
    if(p->s[i]<'1'||p->s[i]>'9') {p->i=i;return GUTC_INVALID_FORMAT;}
    do {++i;} while(i<p->n&&digit(p->s[i]));
  }
  if(i<p->n && p->s[i]=='.') {
    ++i;
    if(i==p->n || !digit(p->s[i])) {p->i=i;return GUTC_INVALID_FORMAT;}
    do {++i;} while(i<p->n&&digit(p->s[i]));
  }
  if(i<p->n && (p->s[i]=='e'||p->s[i]=='E')) {
    ++i;
    if(i<p->n && (p->s[i]=='+'||p->s[i]=='-')) ++i;
    if(i==p->n || !digit(p->s[i])) {p->i=i;return GUTC_INVALID_FORMAT;}
    do {++i;} while(i<p->n&&digit(p->s[i]));
  }
  p->i=i;
  return GUTC_OK;
}
static gutc_status json_value(json_cursor *p,unsigned depth) {
  gutc_status result;
  uint8_t c;
  ws(p);
  if(p->i==p->n) return GUTC_INVALID_FORMAT;
  c=p->s[p->i];
  if(c=='"') return json_string(p);
  if(c=='-'||digit(c)) return json_number(p);
  if(c=='t' && p->n-p->i>=4 && memcmp(p->s+p->i,"true",4)==0) {p->i+=4;return GUTC_OK;}
  if(c=='f' && p->n-p->i>=5 && memcmp(p->s+p->i,"false",5)==0) {p->i+=5;return GUTC_OK;}
  if(c=='n' && p->n-p->i>=4 && memcmp(p->s+p->i,"null",4)==0) {p->i+=4;return GUTC_OK;}
  if(c!='['&&c!='{') return GUTC_INVALID_FORMAT;
  if(depth>=p->limit) return GUTC_RANGE_ERROR;
  ++p->i;
  ws(p);
  if(p->i<p->n && p->s[p->i]==(c=='['?']':'}')) {++p->i;return GUTC_OK;}
  for(;;) {
    if(c=='{') {
      result=json_string(p);
      if(result!=GUTC_OK) return result;
      ws(p);
      if(p->i==p->n||p->s[p->i]!=':') return GUTC_INVALID_FORMAT;
      ++p->i;
    }
    result=json_value(p,depth+1);
    if(result!=GUTC_OK) return result;
    ws(p);
    if(p->i>=p->n) return GUTC_INVALID_FORMAT;
    if(p->s[p->i]==(c=='['?']':'}')) {++p->i;return GUTC_OK;}
    if(p->s[p->i]!=',') return GUTC_INVALID_FORMAT;
    ++p->i;
    ws(p);
  }
}
gutc_status gutc_json_validate(const uint8_t *text,size_t length,
                               unsigned max_depth,size_t *error_offset) {
  json_cursor p;
  gutc_status s;
  if((!text&&length) || !max_depth || max_depth>128) return GUTC_INVALID_ARGUMENT;
  p.s=text;p.n=length;p.i=0;p.limit=max_depth;
  s=json_value(&p,0);
  if(s==GUTC_OK) {
    ws(&p);
    if(p.i!=p.n) s=GUTC_INVALID_FORMAT;
  }
  if(error_offset) *error_offset=p.i;
  return s;
}
static int plus(size_t *n,size_t x) {
  if(x>SIZE_MAX-*n) return 0;
  *n+=x;return 1;
}
gutc_status gutc_json_quote_string(const uint8_t *text,size_t length,
                                   char *output,size_t capacity,size_t *needed) {
  static const char hex[]="0123456789ABCDEF";
  size_t i,n=2,j=0,cp=0;
  gutc_status st;
  if(!needed || (!text&&length) || (output==NULL&&capacity!=0)) return GUTC_INVALID_ARGUMENT;
  st=gutc_utf8_validate(text,length,&cp);
  if(st!=GUTC_OK) return st;
  for(i=0;i<length;i++) {
    uint8_t c=text[i];
    if(c=='"'||c=='\\'||c=='\b'||c=='\f'||c=='\n'||c=='\r'||c=='\t') {
      if(!plus(&n,2)) return GUTC_OVERFLOW;
    } else if(c<0x20u) {
      if(!plus(&n,6)) return GUTC_OVERFLOW;
    } else if(!plus(&n,1)) return GUTC_OVERFLOW;
  }
  *needed=n;
  if(!output) return GUTC_OK;
  if(n==SIZE_MAX) return GUTC_OVERFLOW;
  if(capacity<n+1) return GUTC_NO_SPACE;
  output[j++]='"';
  for(i=0;i<length;i++) {
    uint8_t c=text[i];
    if(c=='"'||c=='\\'||c=='\b'||c=='\f'||c=='\n'||c=='\r'||c=='\t') {
      output[j++]='\\';
      switch(c) {
        case '"':output[j++]='"';break;
        case '\\':output[j++]='\\';break;
        case '\b':output[j++]='b';break;
        case '\f':output[j++]='f';break;
        case '\n':output[j++]='n';break;
        case '\r':output[j++]='r';break;
        default:output[j++]='t';break;
      }
    } else if(c<0x20u) {
      output[j++]='\\';output[j++]='u';output[j++]='0';output[j++]='0';
      output[j++]=hex[c>>4];output[j++]=hex[c&15u];
    } else output[j++]=(char)c;
  }
  output[j++]='"';
  output[j]='\0';
  return GUTC_OK;
}
