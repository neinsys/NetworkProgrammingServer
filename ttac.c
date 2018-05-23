
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "stream.h"
#include "list.h"
void error_handling(char *message);

char* res="HTTP/1.1 200 OK\r\n"
"Date: Mon, 23 May 2005 22:38:34 GMT\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Content-Encoding: UTF-8\r\n"
"Content-Length: 138\r\n"
"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\n"
"Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\r\n"
"ETag: \"3f80f-1b6-3e1cb03b\"\r\n"
"Accept-Ranges: bytes\r\n"
"Connection: close\r\n"
"\r\n"
"<html>"
"<head>"
"  <title>An Example Page</title>"
"</head>"
"<body>"
"  Hello World, this is a very simple HTML document."
"</body>"
"</html>";

typedef struct _request{
    char* method;
    char* path;
    char* version;
    header_list* header;
    char* body;
}request;

typedef struct _response{

}response;

int find_idx(char* s,char ch){
    for(int i=0;s[i];i++){
        if(s[i]==ch)return i;
    }
    return -1;
}

request parse_request(int sock_fds){
    stream s;
    stream_init(&s,sock_fds);
    char* line;
    request req;
    memset(&req,0,sizeof(req));
    
    line=get_line(&s);
    int first=find_idx(line,' ');
    int second=find_idx(line+first+1,' ')+first+1;
    int len = strlen(line);
    int method_len=first;
    int path_len=second-first-1;
    int version_len=len-second-1;
    req.method=(char*)malloc(sizeof(char)*(method_len+1));
    req.path=(char*)malloc(sizeof(char)*(path_len+1));
    req.version=(char*)malloc(sizeof(char)*(version_len+1));
    strncpy(req.method,line,method_len);
    req.method[method_len]=0;
    strncpy(req.path,line+first+1,path_len);
    req.path[path_len]=0;
    strncpy(req.version,line+second+1,version_len);
    req.version[version_len]=0;

    req.header=(header_list*)malloc(sizeof(header_list));
    req.header->head=req.header->tail=NULL;

    free(line);
    //headerline
    while((line=get_line(&s))!=NULL){
        char *key,*value;
        int idx=find_idx(line,':');
        if(idx!=-1){
            int key_len=idx;
            int value_len=strlen(line)-idx-1;
            key=(char*)malloc(sizeof(char)*(key_len+1));
            value=(char*)malloc(sizeof(char)*(value_len+1));
            strncpy(key,line,key_len);
            key[key_len]=0;
            strncpy(value,line+idx+2,value_len);
            value[value_len]=0;
            add_header(req.header,key,value);
        }
        free(line);
    }
    const char* content_length=find_value(req.header,"content-length");
    if(content_length!=NULL){
        int len=atoi(content_length);
        req.body=read_sz(&s,len);
    }
    return req;
}

void* http_thread(void* data){
    int clnt_sock=*((int*)data);
    request req=parse_request(clnt_sock);
    printf("%s|%s|%s\n",req.method,req.path,req.version);   
    fflush(stdout);
    for(node* it=req.header->head;it!=NULL;it=it->next){
        printf("%s|%s\n",it->key,it->value);
    }
    printf("%s\n",req.body);
    write(clnt_sock,res,strlen(res));
    close(clnt_sock);
}
int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int fds[2];

    socklen_t adr_sz;
    int str_len, state;
    char buf[BUF_SIZE];
    if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    int optval =1;
    setsockopt(serv_sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));

    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");


    int i;
    for(i=0;i<100;i++)
    {
        adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        if(clnt_sock==-1)
            continue;
        else
            puts("new client connected...");

        pthread_t thread;
        pthread_create(&thread,NULL,http_thread ,(void*)&clnt_sock);

    }

    close(serv_sock);
    printf("stop connecting new client\n");
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}