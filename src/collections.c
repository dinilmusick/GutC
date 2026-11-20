#include "gutc/collections.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
static int valid_string(gutc_string_view x) {return x.bytes!=NULL||x.length==0;}
static int eqstr(gutc_string_view a,gutc_string_view b) {
  return a.length==b.length &&
   (a.length==0 || (valid_string(a)&&valid_string(b)&&memcmp(a.bytes,b.bytes,a.length)==0));
}
static int valid_value(const gutc_value *x) {
  if(!x)return 0;
  switch(x->kind) {
    case GUTC_VALUE_UNDEFINED:case GUTC_VALUE_NULL:case GUTC_VALUE_BOOL:
    case GUTC_VALUE_NUMBER:return 1;
    case GUTC_VALUE_STRING:return valid_string(x->as.string);
    case GUTC_VALUE_OBJECT:return x->as.object_identity!=NULL;
    case GUTC_VALUE_ARRAY:return x->as.array.identity!=NULL &&
       (x->as.array.items!=NULL || x->as.array.length==0);
    default:return 0;
  }
}
int gutc_value_truthy(const gutc_value *v) {
  if(!valid_value(v))return 0;
  switch(v->kind) {
    case GUTC_VALUE_UNDEFINED:case GUTC_VALUE_NULL:return 0;
    case GUTC_VALUE_BOOL:return v->as.boolean!=0;
    case GUTC_VALUE_NUMBER:return v->as.number!=0.0 && !isnan(v->as.number);
    case GUTC_VALUE_STRING:return v->as.string.length!=0;
    case GUTC_VALUE_ARRAY:case GUTC_VALUE_OBJECT:return 1;
    default:return 0;
  }
}
int gutc_value_same_value_zero(const gutc_value *a,const gutc_value *b) {
  if(!valid_value(a)||!valid_value(b)||a->kind!=b->kind)return 0;
  switch(a->kind) {
    case GUTC_VALUE_UNDEFINED:case GUTC_VALUE_NULL:return 1;
    case GUTC_VALUE_BOOL:return (a->as.boolean!=0)==(b->as.boolean!=0);
    case GUTC_VALUE_NUMBER:return a->as.number==b->as.number ||
      (isnan(a->as.number)&&isnan(b->as.number));
    case GUTC_VALUE_STRING:return eqstr(a->as.string,b->as.string);
    case GUTC_VALUE_OBJECT:return a->as.object_identity==b->as.object_identity;
    case GUTC_VALUE_ARRAY:return a->as.array.identity==b->as.array.identity;
    default:return 0;
  }
}
static gutc_status inputs_ok(const gutc_value *in,size_t count) {
  size_t i;
  if(!in&&count)return GUTC_INVALID_ARGUMENT;
  for(i=0;i<count;i++)if(!valid_value(in+i))return GUTC_INVALID_ARGUMENT;
  return GUTC_OK;
}
static gutc_status capacity_ok(void *out,size_t cap,size_t required) {
  if(!out)return cap==0 ? GUTC_OK : GUTC_INVALID_ARGUMENT;
  return cap>=required?GUTC_OK:GUTC_NO_SPACE;
}
gutc_status gutc_array_compact(const gutc_value *in,size_t count,
                                gutc_value *out,size_t cap,size_t *needed) {
  size_t i,n=0,j=0;gutc_status s;
  if(!needed)return GUTC_INVALID_ARGUMENT;
  s=inputs_ok(in,count);if(s!=GUTC_OK)return s;
  for(i=0;i<count;i++)if(gutc_value_truthy(in+i))n++;
  *needed=n;s=capacity_ok(out,cap,n);
  if(s!=GUTC_OK||!out)return s;
  for(i=0;i<count;i++)if(gutc_value_truthy(in+i))out[j++]=in[i];
  return GUTC_OK;
}
gutc_status gutc_array_unique(const gutc_value *in,size_t count,
                               gutc_value *out,size_t cap,size_t *needed) {
  size_t i,j,n=0,k=0;int duplicate;gutc_status s;
  if(!needed)return GUTC_INVALID_ARGUMENT;
  s=inputs_ok(in,count);if(s!=GUTC_OK)return s;
  for(i=0;i<count;i++) {
    duplicate=0;
    for(j=0;j<i;j++)if(gutc_value_same_value_zero(in+i,in+j)){duplicate=1;break;}
    if(!duplicate)n++;
  }
  *needed=n;s=capacity_ok(out,cap,n);
  if(s!=GUTC_OK||!out)return s;
  for(i=0;i<count;i++) {
    duplicate=0;
    for(j=0;j<i;j++)if(gutc_value_same_value_zero(in+i,in+j)){duplicate=1;break;}
    if(!duplicate)out[k++]=in[i];
  }
  return GUTC_OK;
}
gutc_status gutc_array_flatten_once(const gutc_value *in,size_t count,
                                    gutc_value *out,size_t cap,size_t *needed) {
  size_t i,j,n=0,k=0;gutc_status s;
  if(!needed)return GUTC_INVALID_ARGUMENT;
  s=inputs_ok(in,count);if(s!=GUTC_OK)return s;
  for(i=0;i<count;i++) {
    if(in[i].kind==GUTC_VALUE_ARRAY) {
      size_t len=in[i].as.array.length;
      s=inputs_ok(in[i].as.array.items,len);if(s!=GUTC_OK)return s;
      if(len>SIZE_MAX-n)return GUTC_OVERFLOW;
      n+=len;
    } else if(n==SIZE_MAX)return GUTC_OVERFLOW;
    else n++;
  }
  *needed=n;s=capacity_ok(out,cap,n);
  if(s!=GUTC_OK||!out)return s;
  for(i=0;i<count;i++) {
    if(in[i].kind==GUTC_VALUE_ARRAY) {
      for(j=0;j<in[i].as.array.length;j++)out[k++]=in[i].as.array.items[j];
    } else out[k++]=in[i];
  }
  return GUTC_OK;
}
gutc_status gutc_array_chunks(const gutc_value *in,size_t count,size_t size,
                               gutc_chunk_view *out,size_t cap,size_t *needed) {
  size_t i,n,pos=0;gutc_status s;
  if(!needed)return GUTC_INVALID_ARGUMENT;
  s=inputs_ok(in,count);if(s!=GUTC_OK)return s;
  if(size==0)return GUTC_RANGE_ERROR;
  n=count/size+(count%size!=0);
  *needed=n;s=capacity_ok(out,cap,n);
  if(s!=GUTC_OK||!out)return s;
  for(i=0;i<n;i++) {
    size_t remaining=count-pos;
    out[i].items=in+pos;
    out[i].count=remaining<size?remaining:size;
    pos+=out[i].count;
  }
  return GUTC_OK;
}
gutc_status gutc_array_last(const gutc_value *in,size_t count,gutc_value *result,int *found) {
  gutc_status s=inputs_ok(in,count);
  if(s!=GUTC_OK)return s;
  if(!found)return GUTC_INVALID_ARGUMENT;
  *found=count>0;
  if(count>0) {
    if(!result)return GUTC_INVALID_ARGUMENT;
    *result=in[count-1];
  }
  return GUTC_OK;
}
static int valid_object(const gutc_object_entry *in,size_t count) {
  size_t i,j;
  if(!in&&count)return 0;
  for(i=0;i<count;i++) {
    if(!valid_string(in[i].key) || !valid_value(&in[i].value))return 0;
    for(j=0;j<i;j++)if(eqstr(in[i].key,in[j].key))return 0;
  }
  return 1;
}
static int selected(gutc_string_view k,const gutc_string_view *keys,size_t n) {
  size_t j;
  for(j=0;j<n;j++)if(eqstr(k,keys[j]))return 1;
  return 0;
}
static gutc_status selection(const gutc_object_entry *in,size_t count,
                             const gutc_string_view *keys,size_t keys_count,
                             gutc_object_entry *out,size_t cap,size_t *needed,int pick) {
  size_t i,n=0,j=0;gutc_status s;
  if(!needed || !valid_object(in,count) || (!keys&&keys_count))return GUTC_INVALID_ARGUMENT;
  for(i=0;i<keys_count;i++)if(!valid_string(keys[i]))return GUTC_INVALID_ARGUMENT;
  for(i=0;i<count;i++)if(selected(in[i].key,keys,keys_count)==pick)n++;
  *needed=n;s=capacity_ok(out,cap,n);
  if(s!=GUTC_OK||!out)return s;
  for(i=0;i<count;i++)if(selected(in[i].key,keys,keys_count)==pick)out[j++]=in[i];
  return GUTC_OK;
}
gutc_status gutc_object_has_direct(const gutc_object_entry *in,size_t count,
                                   gutc_string_view key,int *found) {
  if(!found || !valid_string(key) || !valid_object(in,count))return GUTC_INVALID_ARGUMENT;
  *found=selected(key,(const gutc_string_view*)0,0);
  {size_t i;for(i=0;i<count;i++)if(eqstr(key,in[i].key)){*found=1;break;}}
  return GUTC_OK;
}
gutc_status gutc_object_pick_direct(const gutc_object_entry *in,size_t count,
                                    const gutc_string_view *keys,size_t keys_count,
                                    gutc_object_entry *out,size_t cap,size_t *needed) {
  return selection(in,count,keys,keys_count,out,cap,needed,1);
}
gutc_status gutc_object_omit_direct(const gutc_object_entry *in,size_t count,
                                    const gutc_string_view *keys,size_t keys_count,
                                    gutc_object_entry *out,size_t cap,size_t *needed) {
  return selection(in,count,keys,keys_count,out,cap,needed,0);
}
int gutc_number_is_finite(double x){return isfinite(x)!=0;}
gutc_status gutc_number_round_to(double value,unsigned decimals,double *result) {
  static const double powers[]={1.,10.,100.,1000.,10000.,100000.,1000000.,10000000.,
  100000000.,1000000000.,10000000000.,100000000000.,1000000000000.,
  10000000000000.,100000000000000.,1000000000000000.};
  double scaled;
  if(!result || !isfinite(value))return GUTC_INVALID_ARGUMENT;
  if(decimals>15)return GUTC_RANGE_ERROR;
  scaled=value*powers[decimals];
  if(!isfinite(scaled))return GUTC_OVERFLOW;
  *result=round(scaled)/powers[decimals];
  return GUTC_OK;
}
