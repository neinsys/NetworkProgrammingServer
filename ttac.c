
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
#include <regex.h>
#include "stream.h"
#include "list.h"
#include "http.h"
#include "connection_queue.h"

void error_handling(char *message);

const char* login_pattern = "^/login";
regex_t login_r;

const char* signup_pattern = "^/signup";
regex_t signup_r;

const char* create_group_pattern = "^/create_group";
regex_t create_group_r;

const char* group_list_pattern = "^/group_list";
regex_t group_list_r;

const char* join_group_pattern = "^/join_group";
regex_t join_group_r;


char idx(int p){
    if(p>=0 && p<26){
        return 'a'+p;
    }
    else if(p>=26 && p<52){
        return 'A'+p-26;
    }
    return '0'+p-52;
}

void create_token(char* str){
    for(int i=0;i<50;i++){
        str[i]=idx(rand()%62);
    }
}
void server_errer(int clnt_sock,request req,MYSQL* connection){
    fprintf(stderr, "Mysql query error : %s\n", mysql_error(connection));
    response(clnt_sock,500,"Internal Server Error",req.version,NULL,"server error");
}

int getUserIdx(MYSQL* conn,const char* token){
    MYSQL_RES   *sql_result;
    char query[256];
    MYSQL_ROW   sql_row;
    int       query_stat;
    sprintf(query,"select idx from user join token on user.idx = token.user_idx where token.token = '%s'",token);
    query_stat = mysql_query(conn,query);
    if (query_stat != 0)
    {
        return -1;
    }
    sql_result = mysql_store_result(conn);
    sql_row=mysql_fetch_row(sql_result);

    int idx=atoi(sql_row[0]);

    mysql_free_result(sql_result);
    return idx;
}


void signup(int clnt_sock,request req){
    if(find_value(req.parameter,"ID")==NULL || find_value(req.parameter,"password")==NULL || find_value(req.parameter,"name")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];
    char status[51]="";
    char body[256];

    connection = connection_pop();

    sprintf(query,"insert into user (name,ID,password) values ('%s','%s','%s')",find_value(req.parameter,"name"),find_value(req.parameter,"ID"),find_value(req.parameter,"password"));
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        sprintf(body,"{\"status\":\"%s\"}","ERROR");
        response(clnt_sock,200,"OK",req.version,NULL,body);
        return;
    }
    sql_result = mysql_store_result(connection);

    mysql_free_result(sql_result);
    mysql_commit(connection);

    connection_push(connection);

    sprintf(body,"{\"status\":\"%s\"}","OK");
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void login(int clnt_sock,request req){
    if(find_value(req.parameter,"ID")==NULL || find_value(req.parameter,"password")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];
    char token[51]="";
    char body[256];
    char name[26]="";
    char status[6]="ERROR";

    connection = connection_pop();

    sprintf(query,"select password,name,idx from user where ID='%s'",find_value(req.parameter,"ID"));
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result = mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);

    if(strcmp(sql_row[0],find_value(req.parameter,"password"))==0){
        create_token(token);
        int idx= atoi(sql_row[2]);
        sprintf(query,"insert into token (token,user_idx) values('%s',%d)",token,idx);
        query_stat=mysql_query(connection,query);
        if (query_stat != 0)
        {
            server_errer(clnt_sock,req,connection);
            return;
        }
        strcpy(name,sql_row[1]);
        strcpy(status,"OK");
    }

    mysql_free_result(sql_result);

    connection_push(connection);

    sprintf(body,"{"
                 "\"status\":\"%s\","
                 "\"token\":\"%s\","
                 "\"name\":\"%s\""
                 "}",status,token,name);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void create_group(int clnt_sock,request req){
    if(find_value(req.parameter,"name")==NULL || find_value(req.parameter,"token")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }

    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];
    char token[51]="";
    char body[256];
    char name[26]="";
    char status[6]="OK";

    connection = connection_pop();

    int idx=getUserIdx(connection,find_value(req.parameter,"token"));
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }

    sprintf(query,"insert into project_group (name,owner) values ('%s',%d)",find_value(req.parameter,"name"),idx);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    query_stat = mysql_query(connection,"select LAST_INSERT_ID()");
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result = mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);

    int group_idx=atoi(sql_row[0]);

    mysql_free_result(sql_result);


    sprintf(query,"insert into join_group (user_idx,group_idx) values (%d,%d)",idx,group_idx);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }


    mysql_commit(connection);

    connection_push(connection);

    sprintf(body,"{"
                 "\"status\":\"%s\""
                 "}",status);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void group_list(int clnt_sock,request req){
    if(find_value(req.parameter,"token")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }

    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];

    connection = connection_pop();

    int idx=getUserIdx(connection,find_value(req.parameter,"token"));
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }

    sprintf(query,"select project_group.idx,project_group.name,user.name from project_group join join_group on project_group.idx = join_group.group_idx join user on owner = user.idx where user_idx = %d",idx);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result=mysql_store_result(connection);
    stream s;
    stream_write_init(&s);
    write_stream(&s,"[");
    int flag=0;
    while((sql_row=mysql_fetch_row(sql_result))!=NULL){
        char tmp[100];
        if(flag)write_stream(&s,",");
        flag=1;
        sprintf(tmp,"{"
                    "\"id\":%s,"
                    "\"name\":\"%s\","
                    "\"owner\":\"%s\""
                    "}",sql_row[0],sql_row[1],sql_row[2]);
        write_stream(&s,tmp);
        printf("%s\n",s.buf);
    }
    write_stream(&s,"]");

    mysql_free_result(sql_result);

    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,s.buf);
    stream_destory(&s);
}

void join_group(int clnt_sock,request req){
    if(find_value(req.parameter,"token")==NULL || find_value(req.parameter,"group_id")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];
    char status[6]="OK";
    connection = connection_pop();

    int idx=getUserIdx(connection,find_value(req.parameter,"token"));
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }

    sprintf(query,"select group_idx from join_group where user_idx = %d and group_idx = %s",idx,find_value(req.parameter,"group_id"));
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result = mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);
    int create=0;
    if(sql_row!=NULL){
        strcpy(status,"ERROR");
    }
    else{
        create=1;
    }
    mysql_free_result(sql_result);
    if(create){
        sprintf(query,"insert into join_group (user_idx,group_idx) values (%d,%s)",idx,find_value(req.parameter,"group_id"));
        query_stat = mysql_query(connection,query);
        if (query_stat != 0)
        {
            server_errer(clnt_sock,req,connection);
            return;
        }
    }
    char body[50];
    sprintf(body,"{"
                 "\"status\":\"%s\""
                 "}",status);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void* http_thread(void* data){
    int clnt_sock=*((int*)data);
    request req=parse_request(clnt_sock);
    print_request_info(req);
    if(regexec(&login_r,req.path,0,NULL,0)==0){
        login(clnt_sock,req);
    }
    else if(regexec(&signup_r,req.path,0,NULL,0)==0){
        signup(clnt_sock,req);
    }
    else if(regexec(&create_group_r,req.path,0,NULL,0)==0){
        create_group(clnt_sock,req);
    }
    else if(regexec(&group_list_r,req.path,0,NULL,0)==0){
        group_list(clnt_sock,req);
    }
    else if(regexec(&join_group_r,req.path,0,NULL,0)==0){
        join_group(clnt_sock,req);
    }
    else response(clnt_sock,404,"Not Found",req.version,NULL,"404 Not Found");
    clear_requset(req);
    close(clnt_sock);
}

void regex_compile(){
    regcomp(&login_r,login_pattern,REG_EXTENDED);
    regcomp(&signup_r,signup_pattern,REG_EXTENDED);
    regcomp(&create_group_r,create_group_pattern,REG_EXTENDED);
    regcomp(&group_list_r,group_list_pattern,REG_EXTENDED);
    regcomp(&join_group_r,join_group_pattern,REG_EXTENDED);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    regex_compile();
    Queue_Init(5);
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
        pthread_detach(thread);

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
