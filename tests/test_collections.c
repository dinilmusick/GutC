#include "gutc/collections.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static gutc_value num(double x) {
  gutc_value v={0};
  v.kind=GUTC_VALUE_NUMBER;v.as.number=x;return v;
}
static gutc_value str(const char *p) {
  gutc_value v={0};
  v.kind=GUTC_VALUE_STRING;v.as.string.bytes=p;v.as.string.length=strlen(p);return v;
}
static gutc_string_view key(const char *p) {gutc_string_view s={p,strlen(p)};return s;}
int main(void) {
  gutc_value in[8],out[10],nested[2],last={0};
  gutc_chunk_view views[5];
  gutc_object_entry obj[2],picked[2];
  gutc_string_view keys[1];
  size_t need=0;
  int found=0;double rounded;
  in[0]=num(0);in[1]=num(NAN);in[2]=num(3);in[3]=num(3);
  in[4]=str("");in[5]=str("hi");in[6]=str("hi");
  in[7]=(gutc_value){.kind=GUTC_VALUE_BOOL,.as.boolean=0};
  assert(gutc_array_compact(in,8,NULL,0,&need)==GUTC_OK&&need==4);
  out[0]=num(900);
  assert(gutc_array_compact(in,8,out,3,&need)==GUTC_NO_SPACE&&out[0].as.number==900);
  assert(gutc_array_compact(in,8,out,10,&need)==GUTC_OK&&need==4);
  assert(out[0].as.number==3&&out[3].kind==GUTC_VALUE_STRING);
  assert(gutc_array_unique(in,8,NULL,0,&need)==GUTC_OK&&need==6);
  assert(gutc_array_unique(in,8,out,10,&need)==GUTC_OK&&need==6);
  assert(isnan(out[1].as.number));
  nested[0]=num(12);nested[1]=str("x");
  in[0]=(gutc_value){.kind=GUTC_VALUE_ARRAY,.as.array={nested,2,nested}};
  in[1]=num(5);
  assert(gutc_array_flatten_once(in,2,out,10,&need)==GUTC_OK&&need==3);
  assert(out[0].as.number==12&&out[1].kind==GUTC_VALUE_STRING&&out[2].as.number==5);
  assert(gutc_array_chunks(out,3,2,views,5,&need)==GUTC_OK&&need==2);
  assert(views[0].count==2&&views[1].count==1);
  assert(gutc_array_last(out,3,&last,&found)==GUTC_OK&&found&&last.as.number==5);
  assert(gutc_array_last(NULL,0,&last,&found)==GUTC_OK&&!found);
  assert(gutc_array_chunks(out,3,0,views,5,&need)==GUTC_RANGE_ERROR);
  obj[0]=(gutc_object_entry){.key={"a",1},.value=num(1)};
  obj[1]=(gutc_object_entry){.key={"b",1},.value=num(2)};
  keys[0]=key("b");
  assert(gutc_object_has_direct(obj,2,key("a"),&found)==GUTC_OK&&found);
  assert(gutc_object_has_direct(obj,2,key("nested.x"),&found)==GUTC_OK&&!found);
  assert(gutc_object_pick_direct(obj,2,keys,1,picked,2,&need)==GUTC_OK&&need==1);
  assert(picked[0].value.as.number==2);
  assert(gutc_object_omit_direct(obj,2,keys,1,picked,2,&need)==GUTC_OK&&need==1);
  assert(picked[0].value.as.number==1);
  assert(gutc_number_round_to(12.345,2,&rounded)==GUTC_OK&&fabs(rounded-12.35)<1e-9);
  assert(gutc_number_round_to(1,16,&rounded)==GUTC_RANGE_ERROR);
  assert(!gutc_number_is_finite(NAN)&&gutc_number_is_finite(1.0));
  puts("GutC collection and numeric tests passed");
  return 0;
}
