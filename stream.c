#include "stream.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<stdio.h>
//private
int read_fds(stream* s){
   s->len=read(s->fds,s->buf,BUF_SIZE);
   s->cnt=0;
   if(s->len<=0)return -1;
   return 0;
}

char get_char(stream* s){
    if(s->cnt==s->len){
        if(read_fds(s)){
            return -1;
        }
    }
    return s->buf[s->cnt++];
}


//public
void stream_read_init(stream* s,int fds){
    memset(s,0,sizeof(s));
    s->fds=fds;
    s->cnt=0;
    s->len=0;
    s->buf=(char*)malloc(sizeof(char)*BUF_SIZE);
}

void stream_write_init(stream* s){
    memset(s,0,sizeof(s));
    s->cnt=0;
    s->len=10;
    s->buf=(char*)malloc(sizeof(char)*10);
}

void stream_destory(stream* s){
    free(s->buf);
}

//read
char* get_line(stream* s,const char* delimiter){
    char* line = (char*)malloc(sizeof(char)*10);
    int sz=10;
    int len=0;
    int del_len=strlen(delimiter);
    for(;len<del_len || strncmp(line+(len-2),delimiter,del_len)!=0;len++){
        if(len+1==sz){
            sz=(sz*3)/2;
            line=(char*)realloc(line,sizeof(char)*sz);
        }
        line[len]=get_char(s);
        if(line[len]==-1)break;
    }
    if(strncmp(line+(len-2),delimiter,del_len)==0)len-=del_len;
    line[len]=0;
    if(len == 0 || strcmp(line,delimiter)==0){
        free(line);
        return NULL;
    }
    return line;
}


char* read_sz(stream* s,int sz){
    char* buf=(char*)malloc(sizeof(char)*(sz+1));
    for(int i=0;i<sz;i++){
        buf[i]=get_char(s);
    }    
    buf[sz]=0;
    return buf;
}

//write
void write_stream(stream* s,const char* str){
    size_t len = strlen(str);
    while(s->cnt+len+1>s->len){
        s->len=(s->len*3)/2;
        s->buf=(char*)realloc(s->buf,sizeof(char)*s->len);
    }
    strncpy(s->buf+s->cnt,str,len);
    s->cnt+=len;
    s->buf[s->cnt]=0;
}
