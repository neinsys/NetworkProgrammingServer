#ifndef __STREAM_H__
#define __STREAM_H__


#define BUF_SIZE 1024
typedef struct _stream{
    int fds;
    char* buf;
    int cnt;
    int len;
}stream;

void stream_read_init(stream* s,int fds);
void stream_write_init(stream* s);
void stream_destory(stream* s);
char* get_line(stream* s,const char* delimiter);
char* read_sz(stream* s,int sz);
void write_stream(stream* s,const char* str);
#endif