#include "../include/gut_truncate.h"
#include <stdint.h>
#include <string.h>
static int scan(const char *s,size_t *count,size_t *bytes) {
 size_t n=0,i=0,len; const unsigned char *p=(const unsigned char *)s;
 if(!s)return 0;
 len=strlen(s);
 while(i<len) {
  uint32_t cp;size_t k;unsigned char a=p[i];
  if(a<0x80){cp=a;k=1;}
  else if(a>=0xC2&&a<=0xDF){cp=a&31;k=2;}
  else if(a>=0xE0&&a<=0xEF){cp=a&15;k=3;}
  else if(a>=0xF0&&a<=0xF4){cp=a&7;k=4;}
  else return 0;
  if(k>len-i)return 0;
  for(size_t j=1;j<k;j++){if((p[i+j]&0xC0)!=0x80)return 0;cp=(cp<<6)|(p[i+j]&63);}
  if((k==2&&cp<128)||(k==3&&cp<2048)||(k==4&&cp<65536)||cp>0x10FFFF||(cp>=0xD800&&cp<=0xDFFF))return 0;
  if(i>SIZE_MAX-k||n==SIZE_MAX)return 0;
  i+=k;n++;
 }
 *count=n;*bytes=i;return 1;
}
static size_t prefix_bytes(const char *s,size_t count){
 size_t i=0;const unsigned char *p=(const unsigned char *)s;
 while(count--){unsigned char c=p[i];i+=c<128?1:c<224?2:c<240?3:4;}
 return i;
}
gut_truncate_status gut_truncate_utf8(const char *input,size_t max_length,const char *suffix,char *output,size_t capacity,size_t *required){
 size_t in_count,in_bytes,suf_count,suf_bytes,head_bytes,total;
 if(!scan(input,&in_count,&in_bytes)||!scan(suffix,&suf_count,&suf_bytes))return GUT_TRUNCATE_INVALID;
 if(in_count<=max_length){head_bytes=in_bytes;suf_bytes=0;}
 else if(suf_count>=max_length){head_bytes=0;/* Lodash returns the entire omission when it cannot fit. */}
 else head_bytes=prefix_bytes(input,max_length-suf_count);
 if(head_bytes>SIZE_MAX-suf_bytes-1)return GUT_TRUNCATE_INVALID;
 total=head_bytes+suf_bytes;
 if(required)*required=total+1;
 if(!output||capacity<=total)return GUT_TRUNCATE_NOSPACE;
 memcpy(output,input,head_bytes);
 if(suf_bytes)memcpy(output+head_bytes,suffix,suf_bytes);
 output[total]=0;return GUT_TRUNCATE_OK;
}

/* GutTS uses U+2026 when omission is omitted. */
gut_truncate_status gut_truncate_utf8_default(const char *input,size_t max_length,char *output,size_t capacity,size_t *required){
 return gut_truncate_utf8(input,max_length,"\xE2\x80\xA6",output,capacity,required);
}

/* Bounded UTF-8 truncation for non-NUL-terminated spans. Reject embedded NUL
 * and malformed sequences before touching output. Input/output must not overlap. */
static int scan_span(const char *s, size_t len, size_t *count) {
 if (!s) return 0;
 const unsigned char *p = (const unsigned char *)s;
 size_t i = 0, n = 0;
 while (i < len) {
  unsigned char a = p[i];
  uint32_t cp; size_t k;
  if (a == 0) return 0;
  if (a < 0x80) { cp = a; k = 1; }
  else if (a >= 0xC2 && a <= 0xDF) { cp = a & 31; k = 2; }
  else if (a >= 0xE0 && a <= 0xEF) { cp = a & 15; k = 3; }
  else if (a >= 0xF0 && a <= 0xF4) { cp = a & 7; k = 4; }
  else return 0;
  if (k > len - i) return 0;
  for (size_t j = 1; j < k; ++j) {
   if ((p[i+j] & 0xC0) != 0x80) return 0;
   cp = (cp << 6) | (p[i+j] & 63);
  }
  if ((k == 2 && cp < 128) || (k == 3 && cp < 2048) ||
      (k == 4 && cp < 65536) || cp > 0x10FFFF ||
      (cp >= 0xD800 && cp <= 0xDFFF) || n == SIZE_MAX) return 0;
  i += k; ++n;
 }
 *count = n; return 1;
}
gut_truncate_status gut_truncate_utf8_n(const char *input, size_t input_bytes,
 size_t max_length, const char *suffix, size_t suffix_bytes,
 char *output, size_t capacity, size_t *required) {
 size_t in_count, suf_count, head, total;
 if (!scan_span(input, input_bytes, &in_count) ||
     !scan_span(suffix, suffix_bytes, &suf_count)) return GUT_TRUNCATE_INVALID;
 if (in_count <= max_length) { head = input_bytes; suffix_bytes = 0; }
 else if (suf_count >= max_length) head = 0;
 else head = prefix_bytes(input, max_length - suf_count);
 if (head > SIZE_MAX - suffix_bytes - 1) return GUT_TRUNCATE_INVALID;
 total = head + suffix_bytes;
 if (required) *required = total + 1;
 if (!output || capacity <= total) return GUT_TRUNCATE_NOSPACE;
 if (head) memcpy(output, input, head);
 if (suffix_bytes) memcpy(output + head, suffix, suffix_bytes);
 output[total] = 0;
 return GUT_TRUNCATE_OK;
}
