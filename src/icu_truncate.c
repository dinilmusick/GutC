#include "gutc/icu_truncate.h"
#include <unicode/ubrk.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

/* ICU BreakIterator on UTF-8 UText returns native byte offsets.
 * UText may substitute malformed bytes, so validate strictly first. */
static int grapheme_info(const char *value,size_t prefix_count,
                         size_t *count_out,size_t *prefix_bytes) {
  if(!value||!count_out||!prefix_bytes)return 0;
  const size_t byte_count=strlen(value);
  if(byte_count>INT32_MAX)return 0;
  UErrorCode status=U_ZERO_ERROR;
  int32_t utf16_needed=0;
  u_strFromUTF8(NULL,0,&utf16_needed,value,(int32_t)byte_count,&status);
  if(status!=U_BUFFER_OVERFLOW_ERROR&&U_FAILURE(status))return 0;
  status=U_ZERO_ERROR;
  UBreakIterator *iter=ubrk_open(UBRK_CHARACTER,"root",NULL,0,&status);
  if(U_FAILURE(status)||!iter){if(iter)ubrk_close(iter);return 0;}
  UText *text=utext_openUTF8(NULL,value,(int64_t)byte_count,&status);
  if(U_FAILURE(status)||!text){
    if(text)utext_close(text);
    ubrk_close(iter);return 0;
  }
  ubrk_setUText(iter,text,&status);
  if(U_FAILURE(status)){utext_close(text);ubrk_close(iter);return 0;}
  size_t count=0,prefix=0;
  (void)ubrk_first(iter);
  for(int32_t end=ubrk_next(iter);end!=UBRK_DONE;end=ubrk_next(iter)){
    if(end<0||(size_t)end>byte_count||count==SIZE_MAX){
      utext_close(text);ubrk_close(iter);return 0;
    }
    ++count;
    if(count==prefix_count)prefix=(size_t)end;
  }
  if(prefix_count>=count)prefix=byte_count;
  *count_out=count;*prefix_bytes=prefix;
  utext_close(text);ubrk_close(iter);
  return 1;
}

gut_truncate_status gut_truncate_graphemes_icu(const char *input,
    size_t max_length,const char *suffix,char *output,
    size_t capacity,size_t *required){
  size_t input_count=0,suffix_count=0,ignored=0;
  size_t prefix=0,suffix_bytes=0;
  if(!grapheme_info(input,0,&input_count,&ignored)||
     !grapheme_info(suffix,0,&suffix_count,&ignored))return GUT_TRUNCATE_INVALID;
  if(input_count<=max_length){
    prefix=strlen(input);
  }else if(suffix_count>=max_length){
    prefix=0;
    suffix_bytes=strlen(suffix);
  }else{
    size_t check=0;
    if(!grapheme_info(input,max_length-suffix_count,&check,&prefix))
      return GUT_TRUNCATE_INVALID;
    suffix_bytes=strlen(suffix);
  }
  if(prefix>SIZE_MAX-suffix_bytes-1)return GUT_TRUNCATE_INVALID;
  const size_t needed=prefix+suffix_bytes+1;
  if(required)*required=needed;
  if(!output||capacity<needed)return GUT_TRUNCATE_NOSPACE;
  if(prefix)memcpy(output,input,prefix);
  if(suffix_bytes)memcpy(output+prefix,suffix,suffix_bytes);
  output[prefix+suffix_bytes]='\0';
  return GUT_TRUNCATE_OK;
}

gut_truncate_status gut_truncate_graphemes_icu_default(const char *input,
    size_t max_length,char *output,size_t capacity,size_t *required){
  return gut_truncate_graphemes_icu(input,max_length,"\xE2\x80\xA6",
                                    output,capacity,required);
}
