#include "gutc/gutc.h"
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <string.h>

static int add_size(size_t *number, size_t amount) {
  if (amount > SIZE_MAX - *number) return 0;
  *number += amount;
  return 1;
}
static int is_cont(uint8_t c) { return (c & 0xC0u) == 0x80u; }
static int is_unreserved(uint8_t c) {
  return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||
         (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~';
}
static int hex_digit(char c) {
  if(c>='0'&&c<='9') return c-'0';
  if(c>='a'&&c<='f') return c-'a'+10;
  if(c>='A'&&c<='F') return c-'A'+10;
  return -1;
}
static int string_length(const char *s, size_t *length) {
  size_t n;
  if(!s || !length) return 0;
  for(n=0;n<1048576u;++n) if(s[n]=='\0') { *length=n; return 1; }
  return 0;
}
static gutc_status output_check(void *output,size_t capacity,size_t needed,int terminator) {
  size_t required=needed;
  if (terminator && !add_size(&required,1)) return GUTC_OVERFLOW;
  if (output==NULL) return capacity==0 ? GUTC_OK : GUTC_INVALID_ARGUMENT;
  return capacity>=required ? GUTC_OK : GUTC_NO_SPACE;
}
const char *gutc_status_name(gutc_status s) {
  static const char *const names[]={
    "OK","INVALID_ARGUMENT","INVALID_ENCODING","INVALID_FORMAT","RANGE_ERROR",
    "OVERFLOW","NO_SPACE","DEADLINE_EXCEEDED","MAX_DEPTH","CYCLE","BUDGET_EXHAUSTED"
  };
  return (unsigned)s < sizeof(names)/sizeof(names[0]) ? names[s] : "UNKNOWN_STATUS";
}
gutc_status gutc_utf8_validate(const uint8_t *data,size_t len,size_t *points) {
  size_t i=0,n=0,k;
  uint8_t c,b;
  if(!points || (!data && len)) return GUTC_INVALID_ARGUMENT;
  while(i<len) {
    c=data[i];
    if(c<0x80u) k=1;
    else if(c>=0xC2u && c<=0xDFu) k=2;
    else if(c>=0xE0u && c<=0xEFu) k=3;
    else if(c>=0xF0u && c<=0xF4u) k=4;
    else return GUTC_INVALID_ENCODING;
    if(k>len-i) return GUTC_INVALID_ENCODING;
    if(k>1) {
      b=data[i+1];
      if(!is_cont(b)) return GUTC_INVALID_ENCODING;
      if((c==0xE0u && b<0xA0u) || (c==0xEDu && b>=0xA0u) ||
         (c==0xF0u && b<0x90u) || (c==0xF4u && b>=0x90u))
        return GUTC_INVALID_ENCODING;
      if(k>2 && !is_cont(data[i+2])) return GUTC_INVALID_ENCODING;
      if(k>3 && !is_cont(data[i+3])) return GUTC_INVALID_ENCODING;
    }
    i+=k; n++;
  }
  *points=n;
  return GUTC_OK;
}
gutc_status gutc_percent_encode(const uint8_t *data,size_t len,
                                 char *out,size_t cap,size_t *needed) {
  static const char digits[]="0123456789ABCDEF";
  size_t i,n=0,j=0;
  gutc_status status;
  if(!needed || (!data && len)) return GUTC_INVALID_ARGUMENT;
  for(i=0;i<len;i++) if(!add_size(&n,is_unreserved(data[i])?1u:3u)) return GUTC_OVERFLOW;
  *needed=n;
  status=output_check(out,cap,n,1);
  if(status!=GUTC_OK || !out) return status;
  for(i=0;i<len;i++) {
    uint8_t c=data[i];
    if(is_unreserved(c)) out[j++]=(char)c;
    else {out[j++]='%';out[j++]=digits[c>>4];out[j++]=digits[c&15u];}
  }
  out[j]='\0';
  return GUTC_OK;
}
gutc_status gutc_percent_decode(const char *text,size_t len,
                                 uint8_t *out,size_t cap,size_t *needed) {
  size_t i,n=0,j=0;
  int a,b;
  gutc_status status;
  if(!needed || (!text && len)) return GUTC_INVALID_ARGUMENT;
  for(i=0;i<len;i++) {
    if(text[i]=='%') {
      if(len-i<3 || (a=hex_digit(text[i+1]))<0 ||
          (b=hex_digit(text[i+2]))<0) return GUTC_INVALID_ENCODING;
      i+=2;
    }
    if(!add_size(&n,1)) return GUTC_OVERFLOW;
  }
  *needed=n;
  status=output_check(out,cap,n,0);
  if(status!=GUTC_OK || !out) return status;
  for(i=0;i<len;i++) {
    if(text[i]=='%') {
      a=hex_digit(text[i+1]); b=hex_digit(text[i+2]);
      out[j++]=(uint8_t)((a<<4)|b);i+=2;
    } else out[j++]=(uint8_t)text[i];
  }
  return GUTC_OK;
}
gutc_status gutc_join_url(const char *base,const char *const *parts,size_t count,
                           char *out,size_t cap,size_t *needed) {
  size_t length,start,end,total=0,i,j=0;
  int any=0;
  gutc_status status;
  if(!needed || !string_length(base,&length) || (count && !parts)) return GUTC_INVALID_ARGUMENT;
  end=length;
  while(end && base[end-1]=='/') --end;
  if(end) {total=end;any=1;}
  for(i=0;i<count;i++) {
    if(!string_length(parts[i],&length)) return GUTC_INVALID_ARGUMENT;
    start=0;end=length;
    while(start<end && parts[i][start]=='/') ++start;
    while(end>start && parts[i][end-1]=='/') --end;
    if(end>start) {
      if(!add_size(&total,(size_t)any+(end-start))) return GUTC_OVERFLOW;
      any=1;
    }
  }
  *needed=total;
  status=output_check(out,cap,total,1);
  if(status!=GUTC_OK || !out) return status;
  end=0;
  if(!string_length(base,&length)) return GUTC_INVALID_ARGUMENT;
  end=length;
  while(end && base[end-1]=='/') --end;
  if(end) {memcpy(out,base,end);j=end;}
  for(i=0;i<count;i++) {
    (void)string_length(parts[i],&length);
    start=0;end=length;
    while(start<end && parts[i][start]=='/') ++start;
    while(end>start && parts[i][end-1]=='/') --end;
    if(end>start) {
      if(j) out[j++]='/';
      memcpy(out+j,parts[i]+start,end-start); j+=end-start;
    }
  }
  out[j]='\0';
  return GUTC_OK;
}
int gutc_identity_kind_valid(const char *kind) {
  static const char *const kinds[]={"application","deployment","oauth_client",
                                    "user","workspace","ui_session"};
  size_t i;
  if(!kind) return 0;
  for(i=0;i<sizeof(kinds)/sizeof(kinds[0]);i++)
    if(strcmp(kind,kinds[i])==0) return 1;
  return 0;
}
int gutc_identity_id_valid(const char *value,size_t length) {
  size_t i;
  unsigned char c;
  if(!value || length==0 || length>256) return 0;
  for(i=0;i<length;i++) {
    c=(unsigned char)value[i];
    if((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9')) continue;
    if(i>0 && (c=='.'||c=='_'||c==':'||c=='-')) continue;
    return 0;
  }
  return 1;
}
gutc_status gutc_string_blank_ascii(const char *text,size_t len,int *blank) {
  size_t i;
  if(!blank || (!text&&len)) return GUTC_INVALID_ARGUMENT;
  for(i=0;i<len;i++) {
    unsigned char c=(unsigned char)text[i];
    if(c!=' '&&c!='\t'&&c!='\n'&&c!='\r'&&c!='\v'&&c!='\f') {
      *blank=0; return GUTC_OK;
    }
  }
  *blank=1;return GUTC_OK;
}
static gutc_status starts_or_ends(const char *text,size_t len,
                            const char *const *values,size_t count,int *matched,int at_end) {
  size_t i,n;
  if(!matched || (!text&&len) || (count&&!values)) return GUTC_INVALID_ARGUMENT;
  for(i=0;i<count;i++) {
    if(!string_length(values[i],&n)) return GUTC_INVALID_ARGUMENT;
    if(n<=len && memcmp(text+(at_end ? len-n : 0),values[i],n)==0) {
      *matched=1;return GUTC_OK;
    }
  }
  *matched=0;return GUTC_OK;
}
gutc_status gutc_string_starts_with_any(const char *s,size_t n,
                            const char *const *v,size_t c,int *r) {
  return starts_or_ends(s,n,v,c,r,0);
}
gutc_status gutc_string_ends_with_any(const char *s,size_t n,
                            const char *const *v,size_t c,int *r) {
  return starts_or_ends(s,n,v,c,r,1);
}
gutc_status gutc_number_clamp(double v,double lo,double hi,double *result) {
  if(!result || !isfinite(v) || !isfinite(lo) || !isfinite(hi)) return GUTC_INVALID_ARGUMENT;
  if(lo>hi) return GUTC_RANGE_ERROR;
  *result = v<lo ? lo : (v>hi ? hi : v);
  return GUTC_OK;
}
gutc_status gutc_number_in_range(double v,double lo,double hi,int *result) {
  if(!result || !isfinite(v) || !isfinite(lo) || !isfinite(hi)) return GUTC_INVALID_ARGUMENT;
  if(lo>hi) return GUTC_RANGE_ERROR;
  *result = v>=lo && v<hi;
  return GUTC_OK;
}
gutc_status gutc_number_sum(const double *values,size_t count,double *result) {
  double sum=0.0;
  size_t i;
  if(!result || (!values&&count)) return GUTC_INVALID_ARGUMENT;
  for(i=0;i<count;i++) {
    if(!isfinite(values[i])) return GUTC_INVALID_ARGUMENT;
    sum+=values[i];
    if(!isfinite(sum)) return GUTC_OVERFLOW;
  }
  *result=sum;return GUTC_OK;
}
gutc_status gutc_array_chunk_count(size_t count,size_t size,size_t *chunks) {
  if(!chunks) return GUTC_INVALID_ARGUMENT;
  if(!size) return GUTC_RANGE_ERROR;
  *chunks=count/size+(count%size!=0u);return GUTC_OK;
}
gutc_status gutc_invocation_budget_check(const gutc_invocation_budget *b,int64_t now) {
  size_t i;
  if(!b || !b->caller_app_id || !b->target_app_id || !b->audience_app_id ||
     !b->active_app_chain || !b->chain_length || b->chain_length>16 ||
     b->max_depth==0 || b->max_depth>16 || b->remaining_calls>1000 ||
     b->remaining_retries>20) return GUTC_INVALID_ARGUMENT;
  if(!gutc_identity_id_valid(b->caller_app_id,strlen(b->caller_app_id)) ||
     !gutc_identity_id_valid(b->target_app_id,strlen(b->target_app_id)) ||
     !gutc_identity_id_valid(b->audience_app_id,strlen(b->audience_app_id)) ||
     strcmp(b->audience_app_id,b->target_app_id)!=0) return GUTC_INVALID_ARGUMENT;
  for(i=0;i<b->chain_length;i++)
    if(!b->active_app_chain[i] ||
       !gutc_identity_id_valid(b->active_app_chain[i],strlen(b->active_app_chain[i])))
      return GUTC_INVALID_ARGUMENT;
  if(strcmp(b->active_app_chain[b->chain_length-1],b->caller_app_id)!=0)
    return GUTC_INVALID_ARGUMENT;
  if(now>=b->deadline_at_ms) return GUTC_DEADLINE_EXCEEDED;
  if(b->chain_length>=b->max_depth) return GUTC_MAX_DEPTH;
  for(i=0;i<b->chain_length;i++)
    if(strcmp(b->active_app_chain[i],b->target_app_id)==0) return GUTC_CYCLE;
  if(b->remaining_calls==0) return GUTC_BUDGET_EXHAUSTED;
  return GUTC_OK;
}
gutc_status gutc_invocation_budget_advance(const gutc_invocation_budget *b,int64_t now,
                                            unsigned *remaining) {
  gutc_status s;
  if(!remaining) return GUTC_INVALID_ARGUMENT;
  s=gutc_invocation_budget_check(b,now);
  if(s==GUTC_OK) *remaining=b->remaining_calls-1;
  return s;
}
