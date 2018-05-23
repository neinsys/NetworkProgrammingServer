#ifndef __STREAM_H__
#define __STREAM_H__


#define BUF_SIZE 1024
typedef struct _stream{
    int fds;
    char* buf;
    int cnt;
    int len;
}stream;

void stream_init(stream* s,int fds);
char* get_line(stream* s);
char* read_sz(stream* s,int sz);

#endif