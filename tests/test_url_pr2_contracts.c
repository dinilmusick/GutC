#include "gutc/gutc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int is_hex(unsigned c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}
static void verify_binary_percent_roundtrip(void) {
  uint8_t bytes[256], decoded[257];
  char escaped[769], untouched[769];
  size_t encoded_length=0, decoded_length=0;
  for(size_t i=0; i<256; ++i) bytes[i]=(uint8_t)i;
  assert(gutc_percent_encode(bytes,256,NULL,0,&encoded_length)==GUTC_OK);
  assert(encoded_length < sizeof escaped);
  memset(untouched,'Q',sizeof untouched);
  assert(gutc_percent_encode(bytes,256,untouched,encoded_length,&decoded_length)==GUTC_NO_SPACE);
  assert(untouched[0]=='Q' && untouched[encoded_length]=='Q');
  assert(gutc_percent_encode(bytes,256,escaped,sizeof escaped,&decoded_length)==GUTC_OK);
  assert(decoded_length==encoded_length && escaped[encoded_length]=='\0');
  assert(gutc_percent_decode(escaped,encoded_length,NULL,0,&decoded_length)==GUTC_OK);
  assert(decoded_length==256);
  memset(decoded,0xA5,sizeof decoded);
  assert(gutc_percent_decode(escaped,encoded_length,decoded,255,&decoded_length)==GUTC_NO_SPACE);
  assert(decoded[0]==0xA5 && decoded[256]==0xA5);
  assert(gutc_percent_decode(escaped,encoded_length,decoded,256,&decoded_length)==GUTC_OK);
  assert(decoded_length==256 && !memcmp(decoded,bytes,256) && decoded[256]==0xA5);
}
static void verify_all_percent_pair_decoding(void) {
  char escape[3]={'%',0,0};
  uint8_t result[2];
  size_t needed=999;
  for(unsigned hi=0;hi<256;hi++)for(unsigned lo=0;lo<256;lo++) {
    escape[1]=(char)hi;escape[2]=(char)lo;
    result[0]=0xA5;result[1]=0xA5;
    gutc_status status=gutc_percent_decode(escape,3,result,1,&needed);
    if(is_hex(hi)&&is_hex(lo)) {
      assert(status==GUTC_OK && needed==1 && result[1]==0xA5);
    } else {
      assert(status==GUTC_INVALID_ENCODING && result[0]==0xA5 && result[1]==0xA5);
    }
  }
}
static void verify_join_buffer_contract(void) {
  const char *parts[]={"/users/","//42//","///","?x=1"};
  char output[128];size_t needed=0;
  assert(gutc_join_url("https://example.com///",parts,4,NULL,0,&needed)==GUTC_OK);
  assert(needed==strlen("https://example.com/users/42/?x=1"));
  memset(output,'X',sizeof output);
  assert(gutc_join_url("https://example.com///",parts,4,output,needed,&needed)==GUTC_NO_SPACE);
  assert(output[0]=='X' && output[needed]=='X');
  assert(gutc_join_url("https://example.com///",parts,4,output,sizeof output,&needed)==GUTC_OK);
  assert(!strcmp(output,"https://example.com/users/42/?x=1"));
  assert(gutc_join_url("///",NULL,0,output,sizeof output,&needed)==GUTC_OK);
  assert(needed==0 && output[0]=='\0');
}
int main(void) {
  char plus[2]={0,0};
  uint8_t decoded[4]={0,0,0,0};
  size_t needed=0;
  verify_binary_percent_roundtrip();
  verify_all_percent_pair_decoding();
  verify_join_buffer_contract();
  assert(gutc_percent_decode("+",1,decoded,1,&needed)==GUTC_OK);
  assert(needed==1 && decoded[0]=='+');
  assert(gutc_percent_decode("%00%FF",6,decoded,2,&needed)==GUTC_OK);
  assert(needed==2 && decoded[0]==0 && decoded[1]==255);
  assert(gutc_percent_decode("%GG",3,decoded,sizeof decoded,&needed)==GUTC_INVALID_ENCODING);
  assert(gutc_percent_encode(NULL,0,plus,sizeof plus,&needed)==GUTC_OK);
  assert(needed==0 && plus[0]=='\0');
  puts("GutC original PR #2 URL primitives: 256-byte roundtrip, 65536 escapes, no-partial-write and joinUrl passed");
  return 0;
}
