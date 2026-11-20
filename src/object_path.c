#include "gutc/object_path.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX_BYTES 512u
#define PATH_MAX_PARTS 32u
#define PICK_MAX_ARRAY_INDEX 4095u
typedef struct path_part {char text[PATH_MAX_BYTES+1];size_t len;} path_part;
typedef struct parsed_path {path_part parts[PATH_MAX_PARTS];size_t count;} parsed_path;

static gutc_status invalid_path(void){return GUTC_UNSUPPORTED;}
static int name_equal(gutc_json_text a,const path_part *b) {
  return a.length==b->len && (!a.length ||
      (a.bytes && memcmp(a.bytes,b->text,a.length)==0));
}
static int forbidden(const path_part *p) {
  return (p->len==9&&!memcmp(p->text,"__proto__",9))||
         (p->len==9&&!memcmp(p->text,"prototype",9))||
         (p->len==11&&!memcmp(p->text,"constructor",11));
}
static gutc_status push(parsed_path *out,const char *s,size_t n) {
  path_part *p;size_t cp=0;
  if(n>PATH_MAX_BYTES||out->count>=PATH_MAX_PARTS)return GUTC_RANGE_ERROR;
  if(gutc_utf8_validate((const uint8_t*)s,n,&cp)!=GUTC_OK)return GUTC_INVALID_ENCODING;
  p=&out->parts[out->count];
  if(n)memcpy(p->text,s,n);
  p->text[n]='\0';p->len=n;
  if(forbidden(p))return GUTC_UNSUPPORTED;
  out->count++;
  return GUTC_OK;
}
static int array_index(const path_part *p,size_t *out) {
  size_t i,n=0;
  if(!p->len)return 0;
  if(p->len>1&&p->text[0]=='0')return 0;
  for(i=0;i<p->len;i++){
    unsigned char c=(unsigned char)p->text[i];
    if(c<'0'||c>'9')return 0;
    if(n>(SIZE_MAX-(size_t)(c-'0'))/10)return 0;
    n=n*10+(size_t)(c-'0');
  }
  *out=n;return 1;
}
static const gutc_json_dom *lookup(const gutc_json_dom *root,const path_part *p) {
  size_t index=0;
  if(!root)return NULL;
  if(root->kind==GUTC_JSON_OBJECT)
    return gutc_json_dom_get(root,p->text,p->len);
  if(root->kind==GUTC_JSON_ARRAY && array_index(p,&index) &&
      index<root->as.array.count)return root->as.array.values[index];
  return NULL;
}
static gutc_status parse(const gutc_json_dom *root,const char *text,size_t n,
                         parsed_path *out) {
  size_t i=0,cp=0;
  gutc_status rc;
  out->count=0;
  if((!text&&n)||n>PATH_MAX_BYTES)return GUTC_INVALID_ARGUMENT;
  if(gutc_utf8_validate((const uint8_t*)text,n,&cp)!=GUTC_OK)return GUTC_INVALID_ENCODING;
  if(root&&root->kind==GUTC_JSON_OBJECT &&
     gutc_json_dom_get(root,text,n)!=NULL)return push(out,text,n);
  if(n==0)return push(out,"",0);
  while(i<n){
    size_t start,write;
    char quoted[PATH_MAX_BYTES+1];
    if(text[i]=='.'){
      if(!out->count||i+1==n)return invalid_path();
      ++i;
      if(text[i]=='.'||text[i]=='[')return invalid_path();
    }
    if(text[i]=='['){
      ++i;
      if(i==n)return invalid_path();
      if(text[i]=='"'||text[i]==39){
        char quote=text[i++];
        write=0;
        while(i<n && text[i]!=quote){
          unsigned char c=(unsigned char)text[i++];
          if(c=='\\') {
            if(i>=n)return invalid_path();
            c=(unsigned char)text[i++];
            if(c!='\\'&&c!=(unsigned char)quote)return invalid_path();
          }
          if(write==PATH_MAX_BYTES)return GUTC_RANGE_ERROR;
          quoted[write++]=(char)c;
        }
        if(i>=n||text[i++]!=quote||i>=n||text[i++]!=']')return invalid_path();
        rc=push(out,quoted,write);
      } else {
        start=i;
        while(i<n&&text[i]!=']') {
          if(text[i]=='.'||text[i]=='['||text[i]=='"'||text[i]==39)return invalid_path();
          i++;
        }
        if(i==n||i==start)return invalid_path();
        rc=push(out,text+start,i-start);
        ++i;
      }
    } else {
      start=i;
      while(i<n&&text[i]!='.'&&text[i]!='['){
        if(text[i]==']'||text[i]==0)return invalid_path();
        ++i;
      }
      if(i==start)return invalid_path();
      rc=push(out,text+start,i-start);
    }
    if(rc!=GUTC_OK)return rc;
    if(i<n&&text[i]!='.'&&text[i]!='[')return invalid_path();
  }
  return out->count?GUTC_OK:invalid_path();
}
int gutc_json_is_plain_object(const gutc_json_dom *value) {
  return value!=NULL && value->kind==GUTC_JSON_OBJECT;
}
gutc_status gutc_json_path_has(const gutc_json_dom *root,const char *path,
                               size_t n,int *found) {
  parsed_path parsed;size_t i;const gutc_json_dom *cursor=root;
  gutc_status rc;
  if(!found)return GUTC_INVALID_ARGUMENT;
  *found=0;
  rc=parse(root,path,n,&parsed);
  if(rc!=GUTC_OK)return rc;
  for(i=0;i<parsed.count;i++){
    cursor=lookup(cursor,&parsed.parts[i]);
    if(!cursor)return GUTC_OK;
  }
  *found=1;return GUTC_OK;
}
static gutc_json_dom *new_node(gutc_json_kind kind) {
  gutc_json_dom *v=(gutc_json_dom*)calloc(1,sizeof(*v));
  if(v)v->kind=kind;
  return v;
}
/* Clone preserves value semantics using the validated JSON serializer.
 * Numeric shortest-form differences are limited to the text representation. */
static gutc_status clone(const gutc_json_dom *source,gutc_json_dom **out) {
  size_t len=0,at=0;
  gutc_status rc;
  char *buffer;
  *out=NULL;
  if(!gutc_json_dom_is_json_value(source))return GUTC_INVALID_ARGUMENT;
  rc=gutc_json_dom_stringify(source,NULL,0,&len);
  if(rc!=GUTC_OK)return rc;
  if(len==SIZE_MAX)return GUTC_OVERFLOW;
  buffer=(char*)malloc(len+1);
  if(!buffer)return GUTC_NO_MEMORY;
  rc=gutc_json_dom_stringify(source,buffer,len+1,&at);
  if(rc==GUTC_OK)rc=gutc_json_dom_parse((const uint8_t*)buffer,at,128,100000,out,NULL);
  free(buffer);
  return rc;
}
static gutc_json_dom **mutable_child(gutc_json_dom *root,const path_part *p) {
  size_t i,index;
  if(root->kind==GUTC_JSON_OBJECT){
    for(i=0;i<root->as.object.count;i++)
      if(name_equal(root->as.object.members[i].key,p))
        return &root->as.object.members[i].value;
  }
  if(root->kind==GUTC_JSON_ARRAY&&array_index(p,&index)&&
     index<root->as.array.count)return &root->as.array.values[index];
  return NULL;
}
static gutc_status insert(gutc_json_dom *root,const path_part *p,gutc_json_dom *value) {
  gutc_json_dom **existing=mutable_child(root,p);
  if(existing){gutc_json_dom_free(*existing);*existing=value;return GUTC_OK;}
  if(root->kind==GUTC_JSON_OBJECT){
    size_t n=root->as.object.count;
    gutc_json_member *members;
    char *key;
    if(n>=100000||n>=SIZE_MAX/sizeof(*members))return GUTC_RANGE_ERROR;
    key=(char*)malloc(p->len+1);
    if(!key)return GUTC_NO_MEMORY;
    if(p->len)memcpy(key,p->text,p->len);
    key[p->len]='\0';
    members=(gutc_json_member*)realloc(root->as.object.members,(n+1)*sizeof(*members));
    if(!members){free(key);return GUTC_NO_MEMORY;}
    members[n].key.bytes=key;members[n].key.length=p->len;
    members[n].value=value;root->as.object.members=members;root->as.object.count=n+1;
    return GUTC_OK;
  }
  if(root->kind==GUTC_JSON_ARRAY){
    size_t index,n,i;
    gutc_json_dom **items;
    if(!array_index(p,&index)||index>PICK_MAX_ARRAY_INDEX)return GUTC_UNSUPPORTED;
    n=root->as.array.count;
    if(index>=n){
      items=(gutc_json_dom**)realloc(root->as.array.values,(index+1)*sizeof(*items));
      if(!items)return GUTC_NO_MEMORY;
      root->as.array.values=items;
      for(i=n;i<=index;i++){
        items[i]=new_node(GUTC_JSON_NULL);
        if(!items[i]){
          root->as.array.count=i;
          return GUTC_NO_MEMORY;
        }
        root->as.array.count=i+1;
      }
    }
    gutc_json_dom_free(root->as.array.values[index]);
    root->as.array.values[index]=value;
    return GUTC_OK;
  }
  return GUTC_INVALID_FORMAT;
}
static gutc_status select_path(gutc_json_dom *out,const gutc_json_dom *source,
                               const parsed_path *path) {
  size_t i;
  gutc_json_dom *cursor=out,*owned;
  const gutc_json_dom *original=source,*next;
  gutc_status rc;
  for(i=0;i<path->count;i++){
    gutc_json_dom **destination;
    next=lookup(original,&path->parts[i]);
    if(!next)return GUTC_OK;
    if(i+1==path->count){
      rc=clone(next,&owned);
      if(rc!=GUTC_OK)return rc;
      rc=insert(cursor,&path->parts[i],owned);
      if(rc!=GUTC_OK)gutc_json_dom_free(owned);
      return rc;
    }
    destination=mutable_child(cursor,&path->parts[i]);
    if(!destination){
      gutc_json_kind kind=next->kind;
      if(kind!=GUTC_JSON_OBJECT&&kind!=GUTC_JSON_ARRAY)return GUTC_OK;
      owned=new_node(kind);
      if(!owned)return GUTC_NO_MEMORY;
      rc=insert(cursor,&path->parts[i],owned);
      if(rc!=GUTC_OK){gutc_json_dom_free(owned);return rc;}
      destination=mutable_child(cursor,&path->parts[i]);
    }
    if(!destination||!(*destination))return GUTC_INVALID_FORMAT;
    cursor=*destination;original=next;
  }
  return GUTC_OK;
}
static gutc_status remove_path(gutc_json_dom *root,const parsed_path *p) {
  size_t i,j;
  gutc_json_dom *cursor=root;
  for(i=0;i+1<p->count;i++){
    gutc_json_dom **next=mutable_child(cursor,&p->parts[i]);
    if(!next)return GUTC_OK;
    cursor=*next;
  }
  if(cursor->kind==GUTC_JSON_OBJECT){
    for(j=0;j<cursor->as.object.count;j++)
      if(name_equal(cursor->as.object.members[j].key,&p->parts[p->count-1])){
        free(cursor->as.object.members[j].key.bytes);
        gutc_json_dom_free(cursor->as.object.members[j].value);
        cursor->as.object.count--;
        if(j<cursor->as.object.count)
          memmove(cursor->as.object.members+j,cursor->as.object.members+j+1,
                  (cursor->as.object.count-j)*sizeof(gutc_json_member));
        return GUTC_OK;
      }
  } else if(cursor->kind==GUTC_JSON_ARRAY){
    size_t index;
    if(array_index(&p->parts[p->count-1],&index)&&index<cursor->as.array.count){
      gutc_json_dom *replacement=new_node(GUTC_JSON_NULL);
      if(!replacement)return GUTC_NO_MEMORY;
      gutc_json_dom_free(cursor->as.array.values[index]);
      cursor->as.array.values[index]=replacement;
    }
  }
  return GUTC_OK;
}
static gutc_status transform(const gutc_json_dom *root,const gutc_json_text *paths,
                             size_t count,gutc_json_dom **out,int pick) {
  size_t i;
  parsed_path p;
  gutc_status rc=GUTC_OK;
  if(!out)return GUTC_INVALID_ARGUMENT;
  *out=NULL;
  if(!root||(!paths&&count)||count>256)return GUTC_INVALID_ARGUMENT;
  if(!gutc_json_dom_is_json_value(root))return GUTC_INVALID_ARGUMENT;
  if(pick){
    *out=new_node(GUTC_JSON_OBJECT);
    if(!*out)return GUTC_NO_MEMORY;
  } else {
    rc=clone(root,out);
    if(rc!=GUTC_OK)return rc;
  }
  for(i=0;i<count;i++){
    rc=parse(root,paths[i].bytes,paths[i].length,&p);
    if(rc!=GUTC_OK)break;
    rc=pick?select_path(*out,root,&p):remove_path(*out,&p);
    if(rc!=GUTC_OK)break;
  }
  if(rc!=GUTC_OK) {
    gutc_json_dom_free(*out);
    *out=NULL;
  }
  return rc;
}
gutc_status gutc_json_pick_paths(const gutc_json_dom *root,const gutc_json_text *paths,
                                 size_t count,gutc_json_dom **out) {
  return transform(root,paths,count,out,1);
}
gutc_status gutc_json_omit_paths(const gutc_json_dom *root,const gutc_json_text *paths,
                                 size_t count,gutc_json_dom **out) {
  return transform(root,paths,count,out,0);
}
