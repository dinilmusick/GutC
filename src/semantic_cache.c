#include "gutc/semantic_cache.h"
#include "gutc/gutc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GUTC_CACHE_MAX_ENTRIES 4096u
#define GUTC_CACHE_MAX_KEY_BYTES 8192u
static int valid_key(const char *key,size_t n){
  size_t cp=0;
  if((!key&&n)||n>GUTC_CACHE_MAX_KEY_BYTES)return 0;
  return gutc_utf8_validate((const uint8_t*)(n?key:""),n,&cp)==GUTC_OK;
}
static size_t locate(const gutc_semantic_cache *cache,const char *key,size_t n){
  size_t i;
  for(i=0;i<cache->size;i++) {
    const gutc_semantic_cache_entry *slot=cache->entries+i;
    if(slot->length==n&&(!n||memcmp(slot->key,key,n)==0))return i;
  }
  return cache->size;
}
gutc_status gutc_semantic_cache_init(gutc_semantic_cache *cache,size_t max_entries){
  gutc_semantic_cache_entry *slots;
  if(!cache||max_entries>GUTC_CACHE_MAX_ENTRIES)return GUTC_INVALID_ARGUMENT;
  cache->entries=NULL;cache->size=0;cache->capacity=0;
  if(!max_entries)return GUTC_OK;
  slots=(gutc_semantic_cache_entry*)calloc(max_entries,sizeof(*slots));
  if(!slots)return GUTC_NO_MEMORY;
  cache->entries=slots;cache->capacity=max_entries;
  return GUTC_OK;
}
void gutc_semantic_cache_clear(gutc_semantic_cache *cache){
  size_t i;
  if(!cache||(!cache->entries&&cache->size))return;
  for(i=0;i<cache->size;i++){
    free(cache->entries[i].key);
    gutc_json_dom_free(cache->entries[i].value);
    memset(cache->entries+i,0,sizeof(*cache->entries));
  }
  cache->size=0;
}
void gutc_semantic_cache_destroy(gutc_semantic_cache *cache){
  if(!cache)return;
  gutc_semantic_cache_clear(cache);
  free(cache->entries);
  cache->entries=NULL;cache->capacity=0;cache->size=0;
}
size_t gutc_semantic_cache_size(const gutc_semantic_cache *cache){
  return cache?cache->size:0;
}
gutc_status gutc_semantic_cache_set(gutc_semantic_cache *cache,
  const char *key,size_t length,gutc_json_dom *owned_value){
  size_t at;
  char *copy;
  gutc_semantic_cache_entry slot;
  if(!cache||(!cache->entries&&cache->capacity)||!valid_key(key,length)||
     !owned_value||!gutc_json_dom_is_json_value(owned_value))
    return GUTC_INVALID_ARGUMENT;
  if(cache->size>cache->capacity)return GUTC_INVALID_FORMAT;
  {size_t i;
    for(i=0;i<cache->size;i++)
      if(cache->entries[i].value==owned_value)
        return GUTC_INVALID_ARGUMENT; /* prevent duplicate ownership */
  }
  if(!cache->capacity){
    gutc_json_dom_free(owned_value);
    return GUTC_OK;
  }
  at=locate(cache,key,length);
  if(at<cache->size){
    slot=cache->entries[at];
    gutc_json_dom_free(slot.value);
    slot.value=owned_value;
    if(at+1<cache->size)
      memmove(cache->entries+at,cache->entries+at+1,
              (cache->size-at-1)*sizeof(slot));
    cache->entries[cache->size-1]=slot;
    return GUTC_OK;
  }
  if(length==SIZE_MAX)return GUTC_OVERFLOW;
  copy=(char*)malloc(length+1);
  if(!copy)return GUTC_NO_MEMORY;
  if(length)memcpy(copy,key,length);
  copy[length]='\0';
  if(cache->size==cache->capacity){
    free(cache->entries[0].key);
    gutc_json_dom_free(cache->entries[0].value);
    if(cache->size>1)
      memmove(cache->entries,cache->entries+1,(cache->size-1)*sizeof(slot));
    cache->size--;
  }
  slot.key=copy;slot.length=length;slot.value=owned_value;
  cache->entries[cache->size++]=slot;
  return GUTC_OK;
}
gutc_status gutc_semantic_cache_get(gutc_semantic_cache *cache,
  const char *key,size_t length,const gutc_json_dom **borrowed,int *found){
  size_t at;
  gutc_semantic_cache_entry slot;
  if(!cache||!borrowed||!found||(!cache->entries&&cache->capacity)||
     !valid_key(key,length))
    return GUTC_INVALID_ARGUMENT;
  *borrowed=NULL;*found=0;
  if(cache->size>cache->capacity)return GUTC_INVALID_FORMAT;
  at=locate(cache,key,length);
  if(at==cache->size)return GUTC_OK;
  slot=cache->entries[at];
  if(at+1<cache->size)
    memmove(cache->entries+at,cache->entries+at+1,
            (cache->size-at-1)*sizeof(slot));
  cache->entries[cache->size-1]=slot;
  *borrowed=slot.value;*found=1;
  return GUTC_OK;
}
