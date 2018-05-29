
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <mysql.h>
#include "stream.h"
#include "list.h"
#include "http.h"
void error_handling(char *message);

/*char* res="HTTP/1.1 200 OK\r\n"
"Date: Mon, 23 May 2005 22:38:34 GMT\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Content-Encoding: UTF-8\r\n"
"Content-Length: 138\r\n"
"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\n"
"Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\r\n"
"ETag: \"3f80f-1b6-3e1cb03b\"\r\n"
"Accept-Ranges: bytes\r\n"
"Connection: close\r\n"
"\r\n"*/
char* res="<html>"
"<head>"
"  <title>An Example Page</title>"
"</head>"
"<body>"
"  Hello World, this is a very simple HTML document."
"</body>"
"</html>";



void* http_thread(void* data){
    int clnt_sock=*((int*)data);
    request req=parse_request(clnt_sock);
    print_request_info(req);
    response(clnt_sock,200,"OK",req.version,NULL,res);
    clear_requset(req);
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
    while(1)
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