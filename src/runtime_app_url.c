#include "gutc/runtime_app_url.h"
#include "gut_string_predicates.h"
#include <stdint.h>
#include <string.h>

#define MAX_RUNTIME_URL 8192u
static size_t scalar_width(unsigned char first) {
  if(first<128u)return 1u;
  if(first>=0xC2u&&first<=0xDFu)return 2u;
  if(first>=0xE0u&&first<=0xEFu)return 3u;
  if(first>=0xF0u&&first<=0xF4u)return 4u;
  return 0u;
}
static gutc_status trimmed(const char *input,char *output,size_t capacity,size_t *needed) {
  size_t length=0,i,first=0,last=0,codepoints=0,output_size;
  int found=0;
  if(!needed||(!output&&capacity))return GUTC_INVALID_ARGUMENT;
  if(!input){
    *needed=0;
    if(output){
      if(!capacity)return GUTC_NO_SPACE;
      output[0]='\0';
    }
    return GUTC_OK;
  }
  for(length=0;length<MAX_RUNTIME_URL;length++)
    if(!input[length])break;
  if(length==MAX_RUNTIME_URL)return GUTC_RANGE_ERROR;
  if(gutc_utf8_validate((const uint8_t*)input,length,&codepoints)!=GUTC_OK)
    return GUTC_INVALID_ENCODING;
  for(i=0;i<length;) {
    size_t width=scalar_width((unsigned char)input[i]);
    int blank;
    if(width==0||width>length-i)return GUTC_INVALID_ENCODING;
    blank=gut_is_blank_utf8_n(input+i,width);
    if(blank<0)return GUTC_INVALID_ENCODING;
    if(!blank){
      if(!found)first=i;
      last=i+width;
      found=1;
    }
    i+=width;
  }
  output_size=found?last-first:0;
  *needed=output_size;
  if(!output)return GUTC_OK;
  if(capacity<=output_size)return GUTC_NO_SPACE;
  if(output_size)memcpy(output,input+first,output_size);
  output[output_size]='\0';
  return GUTC_OK;
}
gutc_status gutc_resolve_runtime_app_url(const char *host_location_origin,
                                          char *output,size_t capacity,size_t *needed) {
  return trimmed(host_location_origin,output,capacity,needed);
}
gutc_status gutc_resolve_server_runtime_app_url(const char *host_app_url,
                                                 char *output,size_t capacity,size_t *needed) {
  return trimmed(host_app_url,output,capacity,needed);
}
