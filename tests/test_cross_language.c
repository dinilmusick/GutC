#include "gutc/gutc.h"
#include "gutc/collections.h"
#include "gutc/safe_path.h"
#include "gut_string_predicates.h"
#include "gut_truncate_words.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run(char *op,char *in,char *arg,const char *expected){
  char output[4096]={0};
  size_t needed=0,i=0,n=0;
  int value=0;
  double result=0,lo,hi;
  if(strcmp(op,"identity")==0){
    snprintf(output,sizeof output,"%d",gutc_identity_id_valid(in,strlen(in)));
  } else if(strcmp(op,"blank")==0){
    value=gut_is_blank_utf8(in);assert(value>=0);
    snprintf(output,sizeof output,"%d",value);
  } else if(strcmp(op,"starts")==0||strcmp(op,"ends")==0){
    const char *a[]={arg};
    value=strcmp(op,"starts")==0 ?
      gut_starts_with_any_utf8(in,a,1) : gut_ends_with_any_utf8(in,a,1);
    assert(value>=0);snprintf(output,sizeof output,"%d",value);
  } else if(strcmp(op,"join")==0){
    const char *a[]={arg};
    assert(gutc_join_url(in,a,1,output,sizeof output,&needed)==GUTC_OK);
  } else if(strcmp(op,"safePath")==0){
    assert(gutc_encode_safe_path_segments(in,output,sizeof output,&needed)==GUTC_OK);
  } else if(strcmp(op,"words")==0){
    n=(size_t)strtoul(arg,NULL,10);
    assert(gut_truncate_words_utf8_default(in,n,output,sizeof output,&needed)==GUT_TRUNCATE_OK);
  } else if(strcmp(op,"inRange")==0||strcmp(op,"clamp")==0){
    assert(sscanf(arg,"%lf:%lf",&lo,&hi)==2);
    result=strtod(in,NULL);
    if(strcmp(op,"inRange")==0) {
      assert(gutc_number_in_range(result,lo,hi,&value)==GUTC_OK);
      snprintf(output,sizeof output,"%d",value);
    } else {
      assert(gutc_number_clamp(result,lo,hi,&result)==GUTC_OK);
      snprintf(output,sizeof output,"%.15g",result);
    }
  } else if(strcmp(op,"sum")==0||strcmp(op,"unique")==0){
    double nums[32];
    gutc_value vals[32],unique[32];
    const char *p=in,*next;
    do{
      if(i>=32)abort();
      nums[i]=strtod(p,(char**)&next);
      assert(next!=p);
      vals[i].kind=GUTC_VALUE_NUMBER;vals[i].as.number=nums[i];
      i++; p=next;
      if(*p==',')p++;
    }while(*p);
    if(strcmp(op,"sum")==0){
      assert(gutc_number_sum(nums,i,&result)==GUTC_OK);
      snprintf(output,sizeof output,"%.15g",result);
    } else {
      assert(gutc_array_unique(vals,i,unique,32,&needed)==GUTC_OK);
      n=0;
      for(size_t j=0;j<needed;j++) {
        int written=snprintf(output+n,sizeof(output)-n,"%s%.15g",j?",":"",unique[j].as.number);
        assert(written>=0 && (size_t)written<sizeof(output)-n);
        n+=(size_t)written;
      }
    }
  } else abort();
  if(strcmp(output,expected)!=0){
    fprintf(stderr,"Golden fixture mismatch %s [%s] [%s] expected [%s] got [%s]\n",
      op,in,arg,expected,output);
    abort();
  }
}
int main(void){
  FILE *f=fopen(GUTC_GOLDEN_FIXTURE_PATH,"r");
  char line[4096];size_t passed=0;
  assert(f);
  while(fgets(line,sizeof line,f)){
    char *a,*b,*c,*end;
    if(line[0]=='#'||line[0]=='\n')continue;
    end=strchr(line,'\n');if(end)*end='\0';
    a=strchr(line,'\t');assert(a);*a++='\0';
    b=strchr(a,'\t');assert(b);*b++='\0';
    c=strchr(b,'\t');assert(c);*c++='\0';
    run(line,a,b,c);passed++;
  }
  fclose(f);assert(passed>=20);
  printf("GutC cross-language golden cases passed: %zu\n",passed);
  return 0;
}
