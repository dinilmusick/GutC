#include <gutc/icu_truncate.h>
#include <string.h>
int main(void) {
  char result[80];size_t need=0;
  if(gut_truncate_graphemes_icu_default("A👨‍👩‍👦🇮🇳é",3,result,sizeof(result),&need)!=GUT_TRUNCATE_OK)return 1;
  if(strcmp(result,"A👨‍👩‍👦…")!=0)return 2;
  if(need!=strlen(result)+1)return 3;
  if(gut_truncate_lodash_icu_default("शिक्षा",3,result,sizeof(result),&need)!=GUT_TRUNCATE_OK)return 4;
  if(strcmp(result,"शि…")!=0)return 5;
  if(need!=strlen(result)+1)return 6;
  return 0;
}
