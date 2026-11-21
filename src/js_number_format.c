#include "gutc/js_number_format.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int add_char(char *out,size_t *pos,size_t cap,char value){
 if(*pos>=cap-1)return 0;
 out[(*pos)++]=value;
 out[*pos]='\0';return 1;
}
static int add_text(char *out,size_t *pos,size_t cap,const char *s,size_t n){
 if(*pos>=cap||n>=cap-*pos)return 0;
 if(n)memcpy(out+*pos,s,n);
 *pos+=n;out[*pos]='\0';return 1;
}
gutc_status gutc_js_number_format(double number,
  char *output,size_t capacity,size_t *needed){
 char raw[96],digits[24],rendered[384],exptxt[16];
 const char *exp_begin;
 char *end=NULL;
 double magnitude,parsed;
 long exponent=0;
 size_t count=0,pos=0,i,exn;
 int precision,found=0;
 if(!needed||(!output&&capacity))return GUTC_INVALID_ARGUMENT;
 if(!isfinite(number)||number==0){
  const char *text=isfinite(number)?"0":"null";
  size_t n=strlen(text);
  *needed=n;
  if(!output)return GUTC_OK;
  if(capacity<=n)return GUTC_NO_SPACE;
  memcpy(output,text,n+1);
  return GUTC_OK;
 }
 magnitude=fabs(number);
 raw[0]='\0';rendered[0]='\0';
 for(precision=1;precision<=17;precision++){
  int n=snprintf(raw,sizeof(raw),"%.*e",precision-1,magnitude);
  if(n<0||(size_t)n>=sizeof(raw))return GUTC_OVERFLOW;
  /* snprintf/strtod are locale-sensitive: never emit non-C decimal punctuation. */
  if(strchr(raw,','))return GUTC_UNSUPPORTED;
  parsed=strtod(raw,&end);
  if(end==raw||*end!='\0')return GUTC_UNSUPPORTED;
  if(parsed==magnitude){found=1;break;}
 }
 if(!found)return GUTC_UNSUPPORTED;
 exp_begin=strchr(raw,'e');
 if(!exp_begin)return GUTC_UNSUPPORTED;
 exponent=strtol(exp_begin+1,&end,10);
 if(end==exp_begin+1||*end!='\0'||exponent < -400||exponent > 400)
  return GUTC_UNSUPPORTED;
 for(i=0;&raw[i]<exp_begin;i++){
  char ch=raw[i];
  if(ch>='0'&&ch<='9'){
   if(count>=sizeof(digits)-1)return GUTC_OVERFLOW;
   digits[count++]=ch;
  }else if(ch!='.')return GUTC_UNSUPPORTED;
 }
 if(!count)return GUTC_UNSUPPORTED;
 digits[count]='\0';
 if(number<0&&!add_char(rendered,&pos,sizeof(rendered),'-'))
  return GUTC_OVERFLOW;
 if(exponent>=-6&&exponent<21){
  long decimal_places=exponent+1;
  if(decimal_places<=0){
   if(!add_text(rendered,&pos,sizeof(rendered),"0.",2))
    return GUTC_OVERFLOW;
   for(i=0;i<(size_t)(-decimal_places);i++)
    if(!add_char(rendered,&pos,sizeof(rendered),'0'))return GUTC_OVERFLOW;
   if(!add_text(rendered,&pos,sizeof(rendered),digits,count))
    return GUTC_OVERFLOW;
  }else if((size_t)decimal_places>=count){
   if(!add_text(rendered,&pos,sizeof(rendered),digits,count))
    return GUTC_OVERFLOW;
   for(i=count;i<(size_t)decimal_places;i++)
    if(!add_char(rendered,&pos,sizeof(rendered),'0'))return GUTC_OVERFLOW;
  }else{
   if(!add_text(rendered,&pos,sizeof(rendered),digits,(size_t)decimal_places)||
      !add_char(rendered,&pos,sizeof(rendered),'.')||
      !add_text(rendered,&pos,sizeof(rendered),
                digits+(size_t)decimal_places,count-(size_t)decimal_places))
    return GUTC_OVERFLOW;
  }
 }else {
  if(!add_char(rendered,&pos,sizeof(rendered),digits[0]))return GUTC_OVERFLOW;
  if(count>1){
   if(!add_char(rendered,&pos,sizeof(rendered),'.')||
      !add_text(rendered,&pos,sizeof(rendered),digits+1,count-1))
    return GUTC_OVERFLOW;
  }
  if(!add_char(rendered,&pos,sizeof(rendered),'e')||
     !add_char(rendered,&pos,sizeof(rendered),exponent<0?'-':'+'))
   return GUTC_OVERFLOW;
  {
   unsigned abs_exp=(unsigned)(exponent<0?-exponent:exponent);
   int n=snprintf(exptxt,sizeof(exptxt),"%u",abs_exp);
   if(n<0||(size_t)n>=sizeof(exptxt))return GUTC_OVERFLOW;
   exn=(size_t)n;
   if(!add_text(rendered,&pos,sizeof(rendered),exptxt,exn))
    return GUTC_OVERFLOW;
  }
 }
 *needed=pos;
 if(!output)return GUTC_OK;
 if(capacity<=pos)return GUTC_NO_SPACE;
 memcpy(output,rendered,pos+1);
 return GUTC_OK;
}
