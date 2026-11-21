#include "gutc/json_dom.h"
#include "gutc/json.h"
#include "gutc/js_number_format.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct parser {
  const unsigned char *s;
  size_t n, pos, nodes, limit;
  unsigned depth_limit;
  gutc_status error;
} parser;
static void white(parser *p) {
  while(p->pos<p->n && (p->s[p->pos]==' ' || p->s[p->pos]=='\t' ||
                        p->s[p->pos]=='\r' || p->s[p->pos]=='\n')) p->pos++;
}
static int hex(unsigned char c) {
  if(c>='0' && c<='9') return (int)(c-'0');
  if(c>='a' && c<='f') return (int)(c-'a')+10;
  if(c>='A' && c<='F') return (int)(c-'A')+10;
  return -1;
}
void gutc_json_dom_free(gutc_json_dom *v) {
  size_t i;
  if(!v) return;
  if(v->kind==GUTC_JSON_STRING) free(v->as.string.bytes);
  if(v->kind==GUTC_JSON_ARRAY) {
    for(i=0;i<v->as.array.count;i++) gutc_json_dom_free(v->as.array.values[i]);
    free(v->as.array.values);
  }
  if(v->kind==GUTC_JSON_OBJECT) {
    for(i=0;i<v->as.object.count;i++) {
      free(v->as.object.members[i].key.bytes);
      gutc_json_dom_free(v->as.object.members[i].value);
    }
    free(v->as.object.members);
  }
  free(v);
}
static gutc_json_dom *node(parser *p,gutc_json_kind kind) {
  gutc_json_dom *v;
  if(p->nodes>=p->limit) {p->error=GUTC_RANGE_ERROR;return NULL;}
  v=(gutc_json_dom*)calloc(1,sizeof(*v));
  if(!v) {p->error=GUTC_NO_MEMORY;return NULL;}
  v->kind=kind;p->nodes++;
  return v;
}
static void utf8(char *buf,size_t *at,unsigned cp) {
  size_t i=*at;
  if(cp<0x80u) buf[i++]=(char)cp;
  else if(cp<0x800u) {
    buf[i++]=(char)(0xC0u|(cp>>6));
    buf[i++]=(char)(0x80u|(cp&63u));
  } else if(cp<0x10000u) {
    buf[i++]=(char)(0xE0u|(cp>>12));
    buf[i++]=(char)(0x80u|((cp>>6)&63u));
    buf[i++]=(char)(0x80u|(cp&63u));
  } else {
    buf[i++]=(char)(0xF0u|(cp>>18));
    buf[i++]=(char)(0x80u|((cp>>12)&63u));
    buf[i++]=(char)(0x80u|((cp>>6)&63u));
    buf[i++]=(char)(0x80u|(cp&63u));
  }
  *at=i;
}
static gutc_status string(parser *p,gutc_json_text *out) {
  size_t begin,end,at,j=0;
  char *buf;
  if(p->pos>=p->n || p->s[p->pos]!='"')return GUTC_INVALID_FORMAT;
  begin=++p->pos;end=begin;
  while(end<p->n) {
    if(p->s[end]=='\\') {
      if(p->n-end<2)return GUTC_INVALID_FORMAT;
      end+=2;
    } else if(p->s[end]=='"')break;
    else end++;
  }
  if(end==p->n)return GUTC_INVALID_FORMAT;
  if(end-begin==SIZE_MAX)return GUTC_OVERFLOW;
  buf=(char*)malloc(end-begin+1);
  if(!buf)return GUTC_NO_MEMORY;
  at=begin;
  while(at<end) {
    unsigned char c=p->s[at++];
    if(c!='\\'){buf[j++]=(char)c;continue;}
    if(at>=end){free(buf);return GUTC_INVALID_FORMAT;}
    c=p->s[at++];
    switch(c) {
      case '"':buf[j++]='"';break;
      case '\\':buf[j++]='\\';break;
      case '/':buf[j++]='/';break;
      case 'b':buf[j++]='\b';break;
      case 'f':buf[j++]='\f';break;
      case 'n':buf[j++]='\n';break;
      case 'r':buf[j++]='\r';break;
      case 't':buf[j++]='\t';break;
      case 'u': {
        unsigned cp=0,lo=0;
        size_t k;
        if(end-at<4){free(buf);return GUTC_INVALID_FORMAT;}
        for(k=0;k<4;k++) {
          int h=hex(p->s[at+k]);
          if(h<0){free(buf);return GUTC_INVALID_FORMAT;}
          cp=(cp<<4)|(unsigned)h;
        }
        at+=4;
        if(cp>=0xD800u && cp<=0xDBFFu) {
          if(end-at<6 || p->s[at]!='\\' || p->s[at+1]!='u'){
            free(buf);return GUTC_INVALID_ENCODING;
          }
          for(k=2;k<6;k++) {
            int h=hex(p->s[at+k]);
            if(h<0){free(buf);return GUTC_INVALID_FORMAT;}
            lo=(lo<<4)|(unsigned)h;
          }
          if(lo<0xDC00u || lo>0xDFFFu){free(buf);return GUTC_INVALID_ENCODING;}
          cp=0x10000u+((cp-0xD800u)<<10)+(lo-0xDC00u);
          at+=6;
        } else if(cp>=0xDC00u && cp<=0xDFFFu) {
          free(buf);return GUTC_INVALID_ENCODING;
        }
        utf8(buf,&j,cp);
        break;
      }
      default:free(buf);return GUTC_INVALID_FORMAT;
    }
  }
  buf[j]='\0';
  out->bytes=buf;out->length=j;
  p->pos=end+1;
  return GUTC_OK;
}
static gutc_json_dom *value(parser *p,unsigned depth);
static int push_array(parser *p,gutc_json_dom *parent,gutc_json_dom *child) {
  size_t n=parent->as.array.count;
  gutc_json_dom **new_items;
  if(n>=p->limit || n>=SIZE_MAX/sizeof(*new_items)) {
    p->error=GUTC_OVERFLOW;return 0;
  }
  new_items=(gutc_json_dom**)realloc(parent->as.array.values,(n+1)*sizeof(*new_items));
  if(!new_items){p->error=GUTC_NO_MEMORY;return 0;}
  new_items[n]=child;
  parent->as.array.values=new_items;parent->as.array.count=n+1;
  return 1;
}
static int push_object(parser *p,gutc_json_dom *parent,
                       gutc_json_text key,gutc_json_dom *child) {
  size_t i,n=parent->as.object.count;
  gutc_json_member *new_items;
  for(i=0;i<n;i++) {
    gutc_json_text old=parent->as.object.members[i].key;
    if(old.length==key.length &&
       (!old.length || memcmp(old.bytes,key.bytes,key.length)==0)) {
      /* JSON.parse replaces a duplicate property's value without moving key. */
      gutc_json_dom_free(parent->as.object.members[i].value);
      parent->as.object.members[i].value=child;
      free(key.bytes);
      return 1;
    }
  }
  if(n>=p->limit || n>=SIZE_MAX/sizeof(*new_items)) {
    p->error=GUTC_OVERFLOW;return 0;
  }
  new_items=(gutc_json_member*)realloc(parent->as.object.members,(n+1)*sizeof(*new_items));
  if(!new_items){p->error=GUTC_NO_MEMORY;return 0;}
  new_items[n].key=key;new_items[n].value=child;
  parent->as.object.members=new_items;parent->as.object.count=n+1;
  return 1;
}
static gutc_json_dom *array(parser *p,unsigned depth) {
  gutc_json_dom *v,*child;
  if(depth>=p->depth_limit){p->error=GUTC_RANGE_ERROR;return NULL;}
  v=node(p,GUTC_JSON_ARRAY);
  if(!v)return NULL;
  p->pos++;white(p);
  if(p->pos<p->n && p->s[p->pos]==']'){p->pos++;return v;}
  while(p->pos<p->n) {
    child=value(p,depth+1);
    if(!child)goto fail;
    if(!push_array(p,v,child)){gutc_json_dom_free(child);goto fail;}
    white(p);
    if(p->pos<p->n && p->s[p->pos]==']'){p->pos++;return v;}
    if(p->pos==p->n || p->s[p->pos]!=',')break;
    p->pos++;
  }
  p->error=GUTC_INVALID_FORMAT;
fail:
  gutc_json_dom_free(v);return NULL;
}
static gutc_json_dom *object(parser *p,unsigned depth) {
  gutc_json_dom *v,*child;
  gutc_json_text key={NULL,0};
  gutc_status rc;
  if(depth>=p->depth_limit){p->error=GUTC_RANGE_ERROR;return NULL;}
  v=node(p,GUTC_JSON_OBJECT);
  if(!v)return NULL;
  p->pos++;white(p);
  if(p->pos<p->n && p->s[p->pos]=='}'){p->pos++;return v;}
  while(p->pos<p->n && p->s[p->pos]=='"') {
    rc=string(p,&key);
    if(rc!=GUTC_OK){p->error=rc;goto fail;}
    white(p);
    if(p->pos==p->n || p->s[p->pos]!=':')break;
    p->pos++;
    child=value(p,depth+1);
    if(!child)goto fail;
    if(!push_object(p,v,key,child)){gutc_json_dom_free(child);goto fail;}
    key.bytes=NULL;key.length=0;
    white(p);
    if(p->pos<p->n && p->s[p->pos]=='}'){p->pos++;return v;}
    if(p->pos==p->n || p->s[p->pos]!=',')break;
    p->pos++;white(p);
  }
  p->error=GUTC_INVALID_FORMAT;
fail:
  free(key.bytes);gutc_json_dom_free(v);return NULL;
}
static gutc_json_dom *value(parser *p,unsigned depth) {
  gutc_json_dom *v;
  size_t begin,n;
  char *copy,*end;
  white(p);
  if(p->pos>=p->n){p->error=GUTC_INVALID_FORMAT;return NULL;}
  if(p->s[p->pos]=='{')return object(p,depth);
  if(p->s[p->pos]=='[')return array(p,depth);
  if(p->s[p->pos]=='"') {
    v=node(p,GUTC_JSON_STRING);if(!v)return NULL;
    p->error=string(p,&v->as.string);
    if(p->error!=GUTC_OK){gutc_json_dom_free(v);return NULL;}
    return v;
  }
  if(p->s[p->pos]=='n') {
    v=node(p,GUTC_JSON_NULL);if(v)p->pos+=4;return v;
  }
  if(p->s[p->pos]=='t' || p->s[p->pos]=='f') {
    v=node(p,GUTC_JSON_BOOLEAN);if(!v)return NULL;
    v->as.boolean=p->s[p->pos]=='t';p->pos+=v->as.boolean?4u:5u;
    return v;
  }
  begin=p->pos;
  if(p->s[p->pos]=='-')p->pos++;
  if(p->pos<p->n && p->s[p->pos]=='0')p->pos++;
  else while(p->pos<p->n && p->s[p->pos]>='0' && p->s[p->pos]<='9')p->pos++;
  if(p->pos<p->n && p->s[p->pos]=='.') {
    p->pos++;
    while(p->pos<p->n && p->s[p->pos]>='0' && p->s[p->pos]<='9')p->pos++;
  }
  if(p->pos<p->n && (p->s[p->pos]=='e' || p->s[p->pos]=='E')) {
    p->pos++;
    if(p->pos<p->n && (p->s[p->pos]=='+' || p->s[p->pos]=='-'))p->pos++;
    while(p->pos<p->n && p->s[p->pos]>='0' && p->s[p->pos]<='9')p->pos++;
  }
  n=p->pos-begin;
  if(n==0 || n==SIZE_MAX){p->error=GUTC_INVALID_FORMAT;return NULL;}
  copy=(char*)malloc(n+1);
  if(!copy){p->error=GUTC_NO_MEMORY;return NULL;}
  memcpy(copy,p->s+begin,n);copy[n]='\0';
  v=node(p,GUTC_JSON_NUMBER);
  if(!v){free(copy);return NULL;}
  errno=0;
  v->as.number=strtod(copy,&end);
  if(end!=copy+n){
    gutc_json_dom_free(v);p->error=GUTC_INVALID_FORMAT;v=NULL;
  }
  free(copy);
  return v;
}
gutc_status gutc_json_dom_parse(const unsigned char *text,size_t len,
                                unsigned depth,size_t max_nodes,
                                gutc_json_dom **output,size_t *error_at) {
  gutc_status rc;
  parser p;
  size_t at=0;
  gutc_json_dom *v;
  if(!output)return GUTC_INVALID_ARGUMENT;
  *output=NULL;
  if((!text&&len)||depth<1||depth>128||max_nodes<1||max_nodes>100000)
    return GUTC_INVALID_ARGUMENT;
  rc=gutc_json_validate(text,len,depth,&at);
  if(rc!=GUTC_OK){if(error_at)*error_at=at;return rc;}
  p.s=text;p.n=len;p.pos=0;p.nodes=0;p.limit=max_nodes;p.depth_limit=depth;p.error=GUTC_OK;
  v=value(&p,0);
  if(!v){if(error_at)*error_at=p.pos;return p.error;}
  white(&p);
  if(p.pos!=len) {
    gutc_json_dom_free(v);
    if(error_at)*error_at=p.pos;
    return GUTC_INVALID_FORMAT;
  }
  *output=v;
  if(error_at)*error_at=len;
  return GUTC_OK;
}
typedef struct writer {char *out;size_t pos;int failed;} writer;
static void emit(writer *w,const char *bytes,size_t n) {
  if(w->failed)return;
  if(n>SIZE_MAX-w->pos){w->failed=1;return;}
  if(w->out && n)memcpy(w->out+w->pos,bytes,n);
  w->pos+=n;
}
static void quoted(writer *w,const char *s,size_t n) {
  static const char digits[]="0123456789ABCDEF";
  size_t i;
  emit(w,"\"",1);
  for(i=0;i<n;i++) {
    unsigned char c=(unsigned char)s[i];
    if(c=='"'||c=='\\') {
      const char esc[2]={'\\',(char)c};emit(w,esc,2);
    } else if(c=='\b'||c=='\f'||c=='\n'||c=='\r'||c=='\t') {
      const char esc[2]={'\\', c=='\b'?'b':c=='\f'?'f':c=='\n'?'n':c=='\r'?'r':'t'};
      emit(w,esc,2);
    } else if(c<0x20u) {
      const char esc[6]={'\\','u','0','0',digits[c>>4],digits[c&15u]};
      emit(w,esc,6);
    } else emit(w,s+i,1);
  }
  emit(w,"\"",1);
}
static gutc_status serialize(const gutc_json_dom *v,writer *w,unsigned depth) {
  size_t i,points=0;
  gutc_status rc;
  if(!v)return GUTC_INVALID_ARGUMENT;
  if(depth>128)return GUTC_RANGE_ERROR;
  switch(v->kind) {
    case GUTC_JSON_NULL:emit(w,"null",4);return GUTC_OK;
    case GUTC_JSON_BOOLEAN:
      emit(w,v->as.boolean?"true":"false",v->as.boolean?4:5);return GUTC_OK;
    case GUTC_JSON_NUMBER:
      {
        char json_number[384];
        size_t number_length=0;
        rc=gutc_js_number_format(v->as.number,json_number,sizeof json_number,
                                 &number_length);
        if(rc!=GUTC_OK)return rc;
        emit(w,json_number,number_length);
        return GUTC_OK;
      }
    case GUTC_JSON_STRING:
      rc=gutc_utf8_validate((const uint8_t*)v->as.string.bytes,v->as.string.length,&points);
      if(rc!=GUTC_OK)return rc;
      quoted(w,v->as.string.bytes,v->as.string.length);return GUTC_OK;
    case GUTC_JSON_ARRAY:
      if(v->as.array.count&&!v->as.array.values)return GUTC_INVALID_ARGUMENT;
      emit(w,"[",1);
      for(i=0;i<v->as.array.count;i++) {
        if(i)emit(w,",",1);
        rc=serialize(v->as.array.values[i],w,depth+1);
        if(rc!=GUTC_OK)return rc;
      }
      emit(w,"]",1);return GUTC_OK;
    case GUTC_JSON_OBJECT:
      if(v->as.object.count&&!v->as.object.members)return GUTC_INVALID_ARGUMENT;
      emit(w,"{",1);
      for(i=0;i<v->as.object.count;i++) {
        gutc_json_member m=v->as.object.members[i];
        if(i)emit(w,",",1);
        rc=gutc_utf8_validate((const uint8_t*)m.key.bytes,m.key.length,&points);
        if(rc!=GUTC_OK)return rc;
        quoted(w,m.key.bytes,m.key.length);emit(w,":",1);
        rc=serialize(m.value,w,depth+1);
        if(rc!=GUTC_OK)return rc;
      }
      emit(w,"}",1);return GUTC_OK;
    default:return GUTC_INVALID_ARGUMENT;
  }
}
gutc_status gutc_json_dom_stringify(const gutc_json_dom *v,char *out,
                                    size_t cap,size_t *needed) {
  writer w={NULL,0,0};
  gutc_status rc;
  if(!needed||(!out&&cap))return GUTC_INVALID_ARGUMENT;
  rc=serialize(v,&w,0);
  if(rc!=GUTC_OK)return rc;
  if(w.failed||w.pos==SIZE_MAX)return GUTC_OVERFLOW;
  *needed=w.pos;
  if(!out)return GUTC_OK;
  if(cap<w.pos+1)return GUTC_NO_SPACE;
  w.out=out;w.pos=0;w.failed=0;
  rc=serialize(v,&w,0);
  if(rc!=GUTC_OK)return rc;
  if(w.failed)return GUTC_OVERFLOW;
  out[w.pos]='\0';return GUTC_OK;
}
const gutc_json_dom *gutc_json_dom_get(const gutc_json_dom *v,
                                      const char *key,size_t len) {
  size_t i;
  if(!v||v->kind!=GUTC_JSON_OBJECT||(!key&&len))return NULL;
  for(i=0;i<v->as.object.count;i++) {
    gutc_json_member m=v->as.object.members[i];
    if(m.key.length==len && (!len||memcmp(m.key.bytes,key,len)==0))return m.value;
  }
  return NULL;
}
static int json_value(const gutc_json_dom *v,unsigned depth) {
  size_t i,points;
  if(!v||depth>128)return 0;
  switch(v->kind) {
    case GUTC_JSON_NULL:case GUTC_JSON_BOOLEAN:return 1;
    case GUTC_JSON_NUMBER:return isfinite(v->as.number)!=0;
    case GUTC_JSON_STRING:
      return gutc_utf8_validate((const uint8_t*)v->as.string.bytes,v->as.string.length,&points)==GUTC_OK;
    case GUTC_JSON_ARRAY:
      if(v->as.array.count&&!v->as.array.values)return 0;
      for(i=0;i<v->as.array.count;i++)if(!json_value(v->as.array.values[i],depth+1))return 0;
      return 1;
    case GUTC_JSON_OBJECT:
      if(v->as.object.count&&!v->as.object.members)return 0;
      for(i=0;i<v->as.object.count;i++) {
        gutc_json_member m=v->as.object.members[i];
        if(gutc_utf8_validate((const uint8_t*)m.key.bytes,m.key.length,&points)!=GUTC_OK||
           !json_value(m.value,depth+1))return 0;
      }
      return 1;
    default:return 0;
  }
}
int gutc_json_dom_is_json_value(const gutc_json_dom *v) {return json_value(v,0);}
