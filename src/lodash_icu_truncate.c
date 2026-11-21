#include "gutc/icu_truncate.h"
#include <unicode/uregex.h>
#include <unicode/ustring.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Lodash-es v4.18.1 Unicode symbol regexp represented in ICU regex's scalar
 * syntax. The upstream JavaScript uses surrogate-pair patterns; our strict
 * UTF-8 => UTF-16 conversion translates those ranges to supplementary code
 * points. This is NOT the UAX29 grapheme algorithm in icu_truncate.c.
 * Lodash's 4.18.1 license is MIT; preserve source provenance in docs. */
#define LODASH_COMBO "[\\u0300-\\u036f\\ufe20-\\ufe2f\\u20d0-\\u20ff]"
#define LODASH_ASTRAL "[\\U00010000-\\U0010FFFF]"
#define LODASH_NONASTRAL "[^\\U00010000-\\U0010FFFF]"
#define LODASH_REGIONAL "(?:[\\U0001F1E6-\\U0001F1FF]){2}"
#define LODASH_FITZ "[\\U0001F3FB-\\U0001F3FF]"
#define LODASH_SYMBOL "(?:" LODASH_NONASTRAL LODASH_COMBO "?|" LODASH_COMBO "|" LODASH_REGIONAL "|" LODASH_ASTRAL ")"
#define LODASH_MODIFIER "(?:" LODASH_COMBO "|" LODASH_FITZ ")?"
#define LODASH_SEQUENCE "[\\ufe0e\\ufe0f]?" LODASH_MODIFIER "(?:\\u200d(?:" LODASH_NONASTRAL "|" LODASH_REGIONAL "|" LODASH_ASTRAL ")[\\ufe0e\\ufe0f]?" LODASH_MODIFIER ")*"
#define LODASH_PATTERN "(?:" LODASH_FITZ "(?=" LODASH_FITZ ")|" LODASH_SYMBOL LODASH_SEQUENCE ")"

/* Return UTF-8 prefix length for N Lodash Unicode symbols, verified against
 * the actual pinned TS source. No JS engine/Node runtime is needed at runtime.
 * Invalid UTF-8 is rejected, including truncated/overlong/surrogate inputs.
 * Bounds cap allocations and cost for untrusted strings. */
static int lodash_info(const char *input,size_t prefix_count,
                       size_t *count_out,size_t *byte_prefix){
  size_t byte_len,count=0,prefix=0;
  int32_t units=0,previous=0;
  UChar *utf=NULL;
  URegularExpression *matcher=NULL;
  UErrorCode error=U_ZERO_ERROR;
  UParseError parse;
  int valid=0;
  if(!input||!count_out||!byte_prefix)return 0;
  byte_len=strlen(input);
  if(byte_len>32768u||byte_len>INT32_MAX)return 0;
  u_strFromUTF8(NULL,0,&units,input,(int32_t)byte_len,&error);
  if(error!=U_BUFFER_OVERFLOW_ERROR&&U_FAILURE(error))return 0;
  error=U_ZERO_ERROR;
  utf=(UChar*)calloc((size_t)units+1u,sizeof(UChar));
  if(!utf)return 0;
  u_strFromUTF8(utf,units+1,NULL,input,(int32_t)byte_len,&error);
  if(U_FAILURE(error))goto done;
  matcher=uregex_openC(LODASH_PATTERN,0,&parse,&error);
  if(U_FAILURE(error)||!matcher)goto done;
  uregex_setText(matcher,utf,units,&error);
  if(U_FAILURE(error))goto done;
  while(uregex_findNext(matcher,&error)){
    const int32_t begin=uregex_start(matcher,0,&error);
    const int32_t end=uregex_end(matcher,0,&error);
    if(U_FAILURE(error)||begin!=previous||end<=begin||
       end>units||count==SIZE_MAX)goto done;
    previous=end;
    ++count;
    if(count==prefix_count){
      int32_t bytes=0;
      UErrorCode length_error=U_ZERO_ERROR;
      if(end){
        u_strToUTF8(NULL,0,&bytes,utf,end,&length_error);
        if(length_error!=U_BUFFER_OVERFLOW_ERROR&&U_FAILURE(length_error))
          goto done;
      }
      if(bytes<0||(size_t)bytes>byte_len)goto done;
      prefix=(size_t)bytes;
    }
  }
  if(U_FAILURE(error)||previous!=units)goto done;
  if(prefix_count>=count)prefix=byte_len;
  *count_out=count;*byte_prefix=prefix;
  valid=1;
done:
  if(matcher)uregex_close(matcher);
  free(utf);
  return valid;
}

gut_truncate_status gut_truncate_lodash_icu(const char *input,
    size_t max_length,const char *suffix,char *output,
    size_t capacity,size_t *required){
  size_t input_count=0,suffix_count=0,ignored=0;
  size_t prefix=0,suffix_bytes=0;
  if(!lodash_info(input,0,&input_count,&ignored)||
     !lodash_info(suffix,0,&suffix_count,&ignored))return GUT_TRUNCATE_INVALID;
  if(input_count<=max_length){
    prefix=strlen(input);
  }else if(suffix_count>=max_length){
    suffix_bytes=strlen(suffix);
  }else{
    size_t checked=0;
    if(!lodash_info(input,max_length-suffix_count,&checked,&prefix))
      return GUT_TRUNCATE_INVALID;
    suffix_bytes=strlen(suffix);
  }
  if(prefix>SIZE_MAX-suffix_bytes-1u)return GUT_TRUNCATE_INVALID;
  const size_t needed=prefix+suffix_bytes+1u;
  if(required)*required=needed;
  if(!output||capacity<needed)return GUT_TRUNCATE_NOSPACE;
  if(prefix)memcpy(output,input,prefix);
  if(suffix_bytes)memcpy(output+prefix,suffix,suffix_bytes);
  output[prefix+suffix_bytes]='\0';
  return GUT_TRUNCATE_OK;
}

gut_truncate_status gut_truncate_lodash_icu_default(const char *input,
    size_t max_length,char *output,size_t capacity,size_t *required){
  return gut_truncate_lodash_icu(input,max_length,"\xE2\x80\xA6",
                                  output,capacity,required);
}
