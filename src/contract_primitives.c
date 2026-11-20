#include "gutc/contract_primitives.h"
#include "gutc/gutc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const gutc_json_dom *gutc_contract_record(const gutc_json_dom *value){
  return value&&value->kind==GUTC_JSON_OBJECT?value:NULL;
}
static int blank(uint32_t cp) {
  return (cp>=9&&cp<=13)||cp==32||cp==0xA0||cp==0x1680||
    (cp>=0x2000&&cp<=0x200A)||cp==0x2028||cp==0x2029||
    cp==0x202F||cp==0x205F||cp==0x3000||cp==0xFEFF;
}
static int next(const unsigned char *s,size_t n,size_t *offset,uint32_t *cp) {
  size_t i=*offset,k,j;
  uint32_t value;
  unsigned char first;
  if(i>=n)return 0;
  first=s[i];
  if(first<128){k=1;value=first;}
  else if(first>=0xC2&&first<=0xDF){k=2;value=first&31u;}
  else if(first>=0xE0&&first<=0xEF){k=3;value=first&15u;}
  else if(first>=0xF0&&first<=0xF4){k=4;value=first&7u;}
  else return 0;
  if(k>n-i)return 0;
  for(j=1;j<k;j++){
    unsigned char c=s[i+j];
    if((c&0xC0u)!=0x80u)return 0;
    value=(value<<6)|(uint32_t)(c&63u);
  }
  if((k==2&&value<0x80u)||(k==3&&value<0x800u)||
    (k==4&&value<0x10000u)||value>0x10FFFFu||
    (value>=0xD800u&&value<=0xDFFFu))return 0;
  *cp=value;*offset=i+k;return 1;
}
int gutc_contract_id(const gutc_json_dom *value) {
  size_t i=0,units=0;
  uint32_t cp;
  gutc_json_text str;
  if(!value||value->kind!=GUTC_JSON_STRING)return 0;
  str=value->as.string;
  if(!str.bytes||!str.length)return 0;
  while(i<str.length){
    if(!next((const unsigned char*)str.bytes,str.length,&i,&cp))return 0;
    if(cp=='\\'||blank(cp))return 0;
    units+=cp>0xFFFFu?2u:1u;
    if(units>256u)return 0;
  }
  return units>0;
}
static int alpha(unsigned char c) {
  return (c>='A'&&c<='Z')||(c>='a'&&c<='z');
}
static int alnum(unsigned char c) {
  return alpha(c)||(c>='0'&&c<='9');
}
int gutc_contract_app_id(const gutc_json_dom *value){
  const char *s;
  size_t i,n;
  int may_be_scheme=0;
  if(!value||value->kind!=GUTC_JSON_STRING)return 0;
  s=value->as.string.bytes;n=value->as.string.length;
  if(!gutc_identity_id_valid(s,n))return 0;
  may_be_scheme=alpha((unsigned char)s[0]);
  for(i=1;may_be_scheme&&i<n;i++){
    unsigned char c=(unsigned char)s[i];
    if(c==':')return 0;
    if(!alnum(c)&&c!='+'&&c!='.'&&c!='-')may_be_scheme=0;
  }
  return 1;
}
gutc_status gutc_contract_exact_keys(const gutc_json_dom *value,
                                      const gutc_json_text *allowed,
                                      size_t allowed_count,int *result) {
  size_t i,j;
  if(!result||(!allowed&&allowed_count))
    return GUTC_INVALID_ARGUMENT;
  *result=0;
  if(!value)return GUTC_INVALID_FORMAT;
  for(i=0;i<allowed_count;i++)
    if(!allowed[i].bytes&&allowed[i].length)return GUTC_INVALID_ARGUMENT;
  if(value->kind==GUTC_JSON_ARRAY) {
    /* Object.keys(array) yields enumerable numeric element names. */
    for(i=0;i<value->as.array.count;i++) {
      char name[32];
      int digits=snprintf(name,sizeof(name),"%zu",i);
      int found=0;
      if(digits<=0||(size_t)digits>=sizeof(name))return GUTC_OVERFLOW;
      for(j=0;j<allowed_count;j++) {
        if(allowed[j].length==(size_t)digits&&
           !memcmp(allowed[j].bytes,name,(size_t)digits)){found=1;break;}
      }
      if(!found)return GUTC_OK;
    }
    *result=1;return GUTC_OK;
  }
  if(value->kind!=GUTC_JSON_OBJECT)return GUTC_UNSUPPORTED;
  for(i=0;i<value->as.object.count;i++){
    const gutc_json_text key=value->as.object.members[i].key;
    int found=0;
    for(j=0;j<allowed_count;j++){
      if(key.length==allowed[j].length&&(!key.length||
         (key.bytes&&allowed[j].bytes&&
          !memcmp(key.bytes,allowed[j].bytes,key.length)))){found=1;break;}
    }
    if(!found)return GUTC_OK;
  }
  *result=1;return GUTC_OK;
}
unsigned gutc_app_contract_version(void){return 1u;}
