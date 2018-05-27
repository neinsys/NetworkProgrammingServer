
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
    dic_list* header;
    char* body;
    dic_list* parameter;
}request;

typedef struct _response{

}response;

int find_idx(const char* s,char ch){
    for(int i=0;s[i];i++){
        if(s[i]==ch)return i;
    }
    return -1;
}
int find_str(const char* s,const char* p){
    int len=strlen(p);
    for(int i=0;s[i];i++){
        if(i>=len-1 && strncmp(s+(i-len-1),p,len)==0){
            return i;
        }
    }
    return -1;

}
void parse_parameter(dic_list* list,char* params){
    int idx=0;
    if(params==NULL)return;
    int len=strlen(params);
    if(len==0)return;
    do{
        int next=find_idx(params+idx,'&');
        if(next==-1)next=len;
        int equal=find_idx(params+idx,'=')+idx;
        if(equal<next){
            char* key;
            char* value;
            int key_len=equal-idx;
            int value_len=next-equal-1;
            key=(char*)malloc(sizeof(char)*(key_len+1));
            value=(char*)malloc(sizeof(char)*(value_len+1));
            strncpy(key,params+idx,key_len);
            key[key_len]=0;
            strncpy(value,params+equal+1,value_len);
            value[value_len]=0;
            add_data(list,key,value);
        }
        idx=next+1;
    }while(idx<=len);
}
void parse_formdata(dic_list* list,char* body,const char* boundary){
    int idx=0;
    if(body==NULL)return;
    int len=strlen(body);
    if(len==0)return;
    int bound_len=strlen(boundary);
    do{
        printf("????");
        fflush(stdout);
        //boudnary
        int next=find_str(body+idx,boundary);
        if(strncmp(body+next+1,"--",2)==0)break;
        next+=2;
        idx=next+1;
        
        //paramter header?
        char* key;
        char* filename=NULL;
        do{
        printf("????");
        fflush(stdout);
            next=find_str(body+idx,"\r\n");
            if(next==idx+2){
                idx=next+1;
                break;
            }
            while(idx<next){
                int p=find_idx(body+idx,';');
                if(p==-1)p=next;
                if(strncmp(body+idx,"name",4)==0){
                    idx+=5;
                    key=(char*)malloc(sizeof(char)*(p-idx+1));
                    strncpy(key,body+idx,(p-idx));
                    key[p-idx]=0;
                }
                if(strncmp(body+idx,"filename",8)==0){
                    idx+=9;
                    filename=(char*)malloc(sizeof(char)*(p-idx+1));
                    strncpy(filename,body+idx,(p-idx));
                    filename[p-idx]=0;
                }
                idx=p+2;
            }
            idx=next+1;
        }while(1);
        next=find_str(body+idx,boundary)-bound_len;
        char* value=(char*)malloc(sizeof(char)*(next-idx-2+1));
        strncpy(value,body+idx,(next-idx-2));
        value[next-idx-2]=0;
        if(filename!=NULL){
            char* tmp=(char*)malloc(sizeof(char)*(30));
            int len=strlen(filename);
            sprintf(tmp,"%d",len);
            int llen=strlen(tmp);
            tmp=(char*)realloc(tmp,sizeof(char)*(len+llen+2+strlen(value)));
            sprintf(tmp,"%d|%s|%s",len,filename,value);
        }
        add_data(list,key,value);
        idx=next+1;
    }while(idx<=len);
}
request parse_request(int sock_fds){
    stream s;
    stream_init(&s,sock_fds);
    char* line;
    request req;
    memset(&req,0,sizeof(req));
    
    line=get_line(&s,"\r\n");
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

    req.header=(dic_list*)malloc(sizeof(dic_list));
    req.header->head=req.header->tail=NULL;
    req.parameter=(dic_list*)malloc(sizeof(dic_list));
    req.parameter->head=req.parameter->tail=NULL;

    free(line);
    //headerline
    while((line=get_line(&s,"\r\n"))!=NULL){
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
            add_data(req.header,key,value);
        }
        free(line);
    }
    const char* content_length=find_value(req.header,"content-length");
    if(content_length!=NULL){
        int len=atoi(content_length);
        req.body=read_sz(&s,len);
    }
    if(strcmp(req.method,"get")==0 || strcmp(req.method,"GET")==0){
        int idx=find_idx(req.path,'?');
        if(idx>=0){
            parse_parameter(req.parameter,req.path+idx+1);
        }
    }
    if(strcmp(req.method,"post")==0 || strcmp(req.method,"POST")==0){
        const char* contentType = find_value(req.header,"Content-Type");
        if(strcmp(contentType,"application/x-www-form-urlencoded")==0){
            parse_parameter(req.parameter,req.body);
        }
        else if (strncmp(contentType,"multipart/form-data",19)==0){
            int equal_idx=find_idx(contentType,'=');
            int len=strlen(contentType);
            int boundary_len=len-(equal_idx+1);
            char* boundary =(char*)malloc(sizeof(char)*(boundary_len+1+2));
            boundary[0]=boundary[1]='-';
            strncpy(boundary+2,contentType+equal_idx+1,boundary_len);
            parse_formdata(req.parameter,req.body,boundary);

        }
    }
    return req;
}
void print_request_info(request req){
    printf("%s|%s|%s\n",req.method,req.path,req.version);   
    for(node* it=req.header->head;it!=NULL;it=it->next){
        printf("%s|%s\n",it->key,it->value);
    }
    if(req.body!=NULL){
        printf("%s\n",req.body);
    }
    for(node* it=req.parameter->head;it!=NULL;it=it->next){
        printf("%s|%s\n",it->key,it->value);
    }
    fflush(stdout);
}

void* http_thread(void* data){
    int clnt_sock=*((int*)data);
    request req=parse_request(clnt_sock);
    print_request_info(req);
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