#include "../include/gut_truncate_words.h"
#include <stdint.h>
#include <string.h>
static int next_cp(const unsigned char *s, size_t len, size_t *at, uint32_t *cp) {
 size_t i=*at,k; unsigned char a; uint32_t v;
 if(i>=len)return 0;
 a=s[i];
 if(a<128){v=a;k=1;}
 else if(a>=0xC2&&a<=0xDF){v=a&31;k=2;}
 else if(a>=0xE0&&a<=0xEF){v=a&15;k=3;}
 else if(a>=0xF0&&a<=0xF4){v=a&7;k=4;}
 else return -1;
 if(k>len-i)return -1;
 for(size_t j=1;j<k;j++){
  if((s[i+j]&0xC0)!=0x80)return -1;
  v=(v<<6)|(s[i+j]&63);
 }
 if((k==2&&v<128)||(k==3&&v<2048)||(k==4&&v<65536)||
    v>0x10FFFF||(v>=0xD800&&v<=0xDFFF))return -1;
 *cp=v;*at=i+k;return 1;
}
static int ws(uint32_t c){
 return (c>=9&&c<=13)||c==32||c==0xA0||c==0x1680||
 (c>=0x2000&&c<=0x200A)||c==0x2028||c==0x2029||
 c==0x202F||c==0x205F||c==0x3000||c==0xFEFF;
}
static int scan(const char *s,size_t *words,size_t *bytes){
 size_t i=0,len;int in_word=0;uint32_t cp;
 if(!s)return 0;
 len=strlen(s);*words=0;*bytes=len;
 while(i<len){
  int r=next_cp((const unsigned char*)s,len,&i,&cp);
  if(r<0)return 0;
  if(ws(cp))in_word=0;
  else if(!in_word){if(*words==SIZE_MAX)return 0; ++*words;in_word=1;}
 }
 return 1;
}
gut_truncate_status gut_truncate_words_utf8(const char *input,size_t max_words,
 const char *suffix,char *output,size_t capacity,size_t *required){
 size_t words,len,suffix_words,suffix_len,total=0,i=0,kept=0;
 int in_word=0,needs_truncation;
 uint32_t cp;
 if(!scan(input,&words,&len)||!scan(suffix,&suffix_words,&suffix_len))return GUT_TRUNCATE_INVALID;
 (void)suffix_words;
 /* Empty input returns empty, and a single empty split token counts as one. */
 needs_truncation=(len!=0 && (words?words:1)>max_words);
 if(!needs_truncation){
  if(len==SIZE_MAX)return GUT_TRUNCATE_INVALID;
  if(required)*required=len+1;
  if(!output||capacity<=len)return GUT_TRUNCATE_NOSPACE;
  memcpy(output,input,len+1);return GUT_TRUNCATE_OK;
 }
 /* First pass computes exact bytes; second pass writes only after capacity check. */
 for(int pass=0;pass<2;pass++){
  i=0;kept=0;in_word=0;total=0;
  while(i<len){
   size_t pos=i;
   if(next_cp((const unsigned char*)input,len,&i,&cp)<0)return GUT_TRUNCATE_INVALID;
   if(ws(cp))in_word=0;
   else if(!in_word){
    ++kept;in_word=1;
    if(kept>max_words)break;
    if(kept>1){
     if(total==SIZE_MAX)return GUT_TRUNCATE_INVALID;
     if(pass==1)output[total]=' ';
     ++total;
    }
   }
   if(in_word&&kept<=max_words){
    if(i-pos>SIZE_MAX-total)return GUT_TRUNCATE_INVALID;
    /* Copy each scalar exactly once. */
    if(pass==1)memcpy(output+total,input+pos,i-pos);
    total+=i-pos;
   }
  }
  if(total>SIZE_MAX-suffix_len-1)return GUT_TRUNCATE_INVALID;
  if(pass==0){
   if(required)*required=total+suffix_len+1;
   if(!output||capacity<=total+suffix_len)return GUT_TRUNCATE_NOSPACE;
  }else{
   memcpy(output+total,suffix,suffix_len);
   output[total+suffix_len]=0;
  }
 }
 return GUT_TRUNCATE_OK;
}
gut_truncate_status gut_truncate_words_utf8_default(const char *input,size_t max_words,
 char *output,size_t capacity,size_t *required){
 return gut_truncate_words_utf8(input,max_words,"\xE2\x80\xA6",output,capacity,required);
}
