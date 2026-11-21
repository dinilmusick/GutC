#include "gutc/icu_truncate.h"
#include <unicode/ubrk.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct { UChar *data; int32_t len; } utf16_text;
static int valid_utf16(const char *value,utf16_text *result) {
 size_t raw;
 int32_t units=0;
 UErrorCode error=U_ZERO_ERROR;
 if(!value||!result)return 0;
 raw=strlen(value);
 if(raw>INT32_MAX)return 0;
 u_strFromUTF8(NULL,0,&units,value,(int32_t)raw,&error);
 if(error!=U_BUFFER_OVERFLOW_ERROR&&U_FAILURE(error))return 0;
 if(units<0||units>=INT32_MAX)return 0;
 result->data=(UChar *)malloc(((size_t)units+1)*sizeof(UChar));
 if(!result->data)return 0;
 error=U_ZERO_ERROR;
 u_strFromUTF8(result->data,units+1,&result->len,value,(int32_t)raw,&error);
 if(U_FAILURE(error)){free(result->data);result->data=NULL;result->len=0;return 0;}
 return 1;
}
static int count_clusters(const utf16_text *s,UBreakIterator **iter,size_t *count) {
 UErrorCode status=U_ZERO_ERROR;
 int32_t boundary;
 *iter=ubrk_open(UBRK_CHARACTER,"en",s->data,s->len,&status);
 if(U_FAILURE(status)||!*iter)return 0;
 *count=0;
 (void)ubrk_first(*iter);
 while((boundary=ubrk_next(*iter))!=UBRK_DONE) {
  if(*count==SIZE_MAX)return 0;
  ++*count;
 }
 return 1;
}
static int prefix_utf8_bytes(const utf16_text *input,int32_t boundary,size_t *needed) {
 UErrorCode status=U_ZERO_ERROR;
 int32_t n=0;
 if(boundary<0||boundary>input->len)return 0;
 u_strToUTF8(NULL,0,&n,input->data,boundary,&status);
 if(status!=U_BUFFER_OVERFLOW_ERROR&&U_FAILURE(status))return 0;
 if(n<0)return 0;
 *needed=(size_t)n;
 return 1;
}
gut_truncate_status gut_truncate_graphemes_icu(const char *input,size_t max_length,
 const char *suffix,char *output,size_t capacity,size_t *required) {
 utf16_text text={NULL,0},omission={NULL,0};
 UBreakIterator *input_it=NULL,*suffix_it=NULL;
 size_t graphemes=0,suffix_clusters=0,head_bytes=0,suffix_bytes,total;
 gut_truncate_status rc=GUT_TRUNCATE_INVALID;
 if(!input||!suffix)return rc;
 if(!valid_utf16(input,&text)||!valid_utf16(suffix,&omission))goto cleanup;
 if(!count_clusters(&text,&input_it,&graphemes)||
    !count_clusters(&omission,&suffix_it,&suffix_clusters))goto cleanup;
 suffix_bytes=strlen(suffix);
 if(graphemes<=max_length) {
  head_bytes=strlen(input);
  suffix_bytes=0;
 } else if(suffix_clusters>=max_length) {
  head_bytes=0; /* Lodash emits full omission even when it cannot fit. */
 } else {
  size_t prefix_count=max_length-suffix_clusters;
  int32_t end=ubrk_first(input_it);
  for(size_t i=0;i<prefix_count;++i) {
   end=ubrk_next(input_it);
   if(end==UBRK_DONE)goto cleanup;
  }
  if(!prefix_utf8_bytes(&text,end,&head_bytes))goto cleanup;
 }
 if(suffix_bytes>SIZE_MAX-1||head_bytes>SIZE_MAX-suffix_bytes-1)goto cleanup;
 total=head_bytes+suffix_bytes;
 if(required)*required=total+1;
 if(!output||capacity<=total){rc=GUT_TRUNCATE_NOSPACE;goto cleanup;}
 if(head_bytes)memcpy(output,input,head_bytes);
 if(suffix_bytes)memcpy(output+head_bytes,suffix,suffix_bytes);
 output[total]=0;
 rc=GUT_TRUNCATE_OK;
cleanup:
 if(input_it)ubrk_close(input_it);
 if(suffix_it)ubrk_close(suffix_it);
 free(text.data);
 free(omission.data);
 return rc;
}
gut_truncate_status gut_truncate_graphemes_icu_default(const char *input,
 size_t max_length,char *output,size_t capacity,size_t *required){
 return gut_truncate_graphemes_icu(input,max_length,"\xE2\x80\xA6",
                                  output,capacity,required);
}
