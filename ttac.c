
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
void error_handling(char *message);

const char* db_host = "domjudge.ceyzmi2ecctb.ap-northeast-2.rds.amazonaws.com";
const char* db_user = "ssulegram";
const char* db_password = "nein7961!";
const char* db_name = "ssulegram";

const char* login_pattern = "^/login";
regex_t login_r;

const char* signup_pattern = "^/signup";
regex_t signup_r;

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

void signup(int clnt_sock,request req){
    if(find_value(req.parameter,"ID")==NULL || find_value(req.parameter,"password")==NULL || find_value(req.parameter,"name")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    MYSQL       *connection=NULL, conn;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];
    char status[51]="";
    char body[256];
    mysql_init(&conn);

    connection = mysql_real_connect(&conn, db_host,
                                    db_user, db_password,
                                    db_name, 3306,
                                    (char *)NULL, 0);
    if (connection == NULL)
    {
        fprintf(stderr, "Mysql connection error : %s\n", mysql_error(&conn));
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"server error");
        return;
    }
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

    sprintf(body,"{\"status\":\"%s\"}","OK");
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void login(int clnt_sock,request req){
    if(find_value(req.parameter,"ID")==NULL || find_value(req.parameter,"password")==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    MYSQL       *connection=NULL, conn;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];
    char token[51]="";
    char body[256];
    char name[26]="";
    char status[6]="ERROR";
    mysql_init(&conn);

    connection = mysql_real_connect(&conn, db_host,
                                    db_user, db_password,
                                    db_name, 3306,
                                    (char *)NULL, 0);
    if (connection == NULL)
    {
        fprintf(stderr, "Mysql connection error : %s\n", mysql_error(&conn));
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"server error");
        return;
    }
    sprintf(query,"select password,name from user where ID='%s'",find_value(req.parameter,"ID"));
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        fprintf(stderr, "Mysql query error : %s\n", mysql_error(&conn));
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"server error");
        return;
    }
    sql_result = mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);

    if(strcmp(sql_row[0],find_value(req.parameter,"password"))==0){
        create_token(token);
        strcpy(name,sql_row[1]);
        strcpy(status,"OK");
    }

    mysql_free_result(sql_result);

    sprintf(body,"{"
                 "\"status\":\"%s\","
                 "\"token\":\"%s\","
                 "\"name\":\"%s\""
                 "}",status,token,name);
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
    else response(clnt_sock,404,"Not Found",req.version,NULL,"404 Not Found");
    clear_requset(req);
    close(clnt_sock);
}

void regex_compile(){
    regcomp(&login_r,login_pattern,REG_EXTENDED);
    regcomp(&signup_r,signup_pattern,REG_EXTENDED);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    regex_compile();
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
