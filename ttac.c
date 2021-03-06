
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
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
#include "string_util.h"

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

const char* todolist_pattern = "^/todolist";
regex_t todolist_r;

const char* progess_pattern = "^/progress";
regex_t progress_r;

const char* chat_pattern = "^/chat$";
regex_t chat_r;

const char* chatlist_pattern = "^/chatlist";
regex_t chatlist_r;

const char* location_pattern = "^/location";
regex_t location_r;

char idx(int p){
    if(p>=0 && p<26){
        return 'a'+p;
    }
    else if(p>=26 && p<52){
        return 'A'+p-26;
    }
    return '0'+p-52;
}

char* getNow(){
    char* now =(char*)malloc(sizeof(char)*200);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(now,"%d-%d-%d %d:%d:%d",
           tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
           tm.tm_hour, tm.tm_min, tm.tm_sec);

    return now;


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

int getUserIdxByID(MYSQL* conn,const char* ID){
    MYSQL_RES   *sql_result;
    char query[256];
    MYSQL_ROW   sql_row;
    int       query_stat;
    sprintf(query,"select idx from user where ID = '%s'",ID);
    query_stat = mysql_query(conn,query);
    if (query_stat != 0)
    {
        return -1;
    }
    sql_result = mysql_store_result(conn);
    sql_row=mysql_fetch_row(sql_result);
    if(sql_row==NULL){

        return -1;
    }
    int idx=atoi(sql_row[0]);

    mysql_free_result(sql_result);
    return idx;
}

void attendance_check(MYSQL* conn,int user_idx,int group_idx){
    char query[256];
    int       query_stat;
    sprintf(query,"insert into attendance (user_idx,group_idx,attendance_date) values (%d,%d,NOW())",user_idx,group_idx);
    query_stat = mysql_query(conn,query);

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
    const char* token = find_value(req.parameter,"token");
    const char* group_id=find_value(req.parameter,"group_id");
    const char* ID=find_value(req.parameter,"ID");
    if(token==NULL || group_id==NULL || ID==NULL){
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
    int idx=getUserIdx(connection,token);
    int useridx=getUserIdxByID(connection,ID);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }
    printf("%d\n",useridx);
    if(useridx==-1) {
        strcpy(status,"NONE");
    }
    else {
        sprintf(query, "select idx from project_group where owner = %d and idx = %s", idx, group_id);
        query_stat = mysql_query(connection, query);
        if (query_stat != 0) {
            server_errer(clnt_sock, req, connection);
            return;
        }
        sql_result = mysql_store_result(connection);
        sql_row = mysql_fetch_row(sql_result);
        int create = 0;
        if (sql_row == NULL) {
            strcpy(status, "ERROR");
        } else {
            create = 1;
        }
        mysql_free_result(sql_result);
        if (create) {
            sprintf(query, "insert into join_group (user_idx,group_idx) values (%d,%s)", useridx, group_id);
            query_stat = mysql_query(connection, query);
            if (query_stat != 0) {
                server_errer(clnt_sock, req, connection);
                return;
            }
        }
    }
    char body[50];
    sprintf(body,"{"
                 "\"status\":\"%s\""
                 "}",status);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void get_todolist(int clnt_sock,request req){
    const char* token = find_value(req.parameter,"token");
    if(token==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }

    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];

    connection = connection_pop();

    int idx=getUserIdx(connection,token);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }

    sprintf(query,"select deadline,detail from todo where user_idx = %d",idx);
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
                    "\"deadline\":%s,"
                    "\"detail\":\"%s\""
                    "}",sql_row[0],sql_row[1]);
        write_stream(&s,tmp);

    }
    write_stream(&s,"]");

    mysql_free_result(sql_result);

    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,s.buf);
    stream_destory(&s);
}
void post_todolist(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    const char* deadline =find_value(req.parameter,"deadline");
    const char* detail=find_value(req.parameter,"detail");
    if(token==NULL || deadline==NULL || detail==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }

    MYSQL       *connection=NULL;
    int       query_stat;
    char query[256];
    char status[6]="OK";

    connection = connection_pop();

    int idx=getUserIdx(connection,token);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }

    sprintf(query,"insert into todo (user_idx,deadline,detail) values (%d,'%s','%s')",idx,deadline,detail);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {

        fprintf(stderr, "Mysql query error : %s\n", mysql_error(connection));
        strcpy(status,"ERROR");
    }

    char body[256];
    sprintf(body,"{"
                 "\"status\":\"%s\""
                 "}",status);

    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void get_progress(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    if(token==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }


    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[256];


    connection = connection_pop();

    int idx=getUserIdx(connection,token);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }
    sprintf(query,"select number from progress where user_idx = %d",idx);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result=mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);
    char body[256];
    if(sql_row!=NULL) {
        sprintf(body, "{"
                      "\"number\":%s"
                      "}", sql_row[0]);
    }
    else{
        sprintf(body, "{"
                      "\"number\":%s"
                      "}", "0");
    }


    mysql_free_result(sql_result);

    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,body);

}
void post_progress(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    const char* number = find_value(req.parameter,"number");
    if(token==NULL || number ==NULL){
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

    int idx=getUserIdx(connection,token);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }
    sprintf(query,"select number from progress where user_idx = %d",idx);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result=mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);

    char body[256];
    if(sql_row!=NULL) {
        sprintf(query,"update progress set number = %s where user_idx = %d",number,idx);
    }
    else{
        sprintf(query,"insert into progress (user_idx,number) values(%d,%s)",idx,number);
    }

    mysql_free_result(sql_result);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        strcpy(status,"ERROR");
    }

    sprintf(body,"{"
                 "\"status\":\"%s\""
                 "}",status);
    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,body);

}

void create_chat(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    const char* group_id = find_value(req.parameter,"group_id");
    const char* content = find_value(req.parameter,"content");
    if(token==NULL || group_id ==NULL || content==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }


    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[2000];
    char status[6]="OK";

    connection = connection_pop();

    int idx=getUserIdx(connection,token);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }
    attendance_check(connection,idx,atoi(group_id));

    sprintf(query,"select MAX(idx) from chat where group_idx = %s",group_id);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    sql_result=mysql_store_result(connection);
    sql_row=mysql_fetch_row(sql_result);

    int latest=-1;
    if(sql_row[0]!=NULL){
        latest=atoi(sql_row[0]);
    }
    mysql_free_result(sql_result);

    char* now=getNow();
    sprintf(query,"insert into chat (user_idx,group_idx,chat_type,content,chat_time) values (%d,%s,\"%s\",\"%s\",\"%s\")",idx,group_id,"TEXT",content,now);
    free(now);
    query_stat = mysql_query(connection,query);
    if (query_stat != 0)
    {
        server_errer(clnt_sock,req,connection);
        return;
    }
    char body[256];

    sprintf(body,"{"
                 "\"status\":\"%s\","
                 "\"latest\":%d"
                 "}",status,latest);
    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}


void get_chatlist(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    const char* group_id = find_value(req.parameter,"group_id");
    const char* latest = find_value(req.parameter,"latest");

    if(token==NULL || group_id ==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }


    MYSQL       *connection=NULL;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    int       query_stat;
    char query[2000];
    char status[6]="OK";

    connection = connection_pop();

    int idx=getUserIdx(connection,token);
    if(idx==-1){
        server_errer(clnt_sock,req,connection);
        return;
    }
    attendance_check(connection,idx,atoi(group_id));

    if(latest==NULL)sprintf(query,"select chat.idx,chat_type,content,chat_time,user.ID from chat join user on chat.user_idx = user.idx where group_idx = %s",group_id);
    else sprintf(query,"select chat.idx,chat_type,content,chat_time,user.ID from chat join user on chat.user_idx = user.idx where group_idx = %s and chat.idx>%s",group_id,latest);
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
                    "\"user_ID\":\"%s\","
                    "\"chat_type\":\"%s\","
                    "\"content\":\"%s\","
                    "\"chat_time\":\"%s\""
                    "}",sql_row[0],sql_row[4],sql_row[1],sql_row[2],sql_row[3]);
        write_stream(&s,tmp);

    }
    write_stream(&s,"]");

    mysql_free_result(sql_result);

    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,s.buf);
    stream_destory(&s);
}
dic_list location_list;
pthread_mutex_t list_mutex =PTHREAD_MUTEX_INITIALIZER;

void push_location(const char* user_id,const char* latitude,const char* longitute){
    pthread_mutex_lock(&list_mutex);
    for(node* it = location_list.head;it!=NULL;it=it->next){
        if(strcmp(user_id,it->key)==0){
            free(it->value);
            int len = strlen(latitude)+strlen(longitute)+1;
            it->value=(char*)malloc(sizeof(char)*(len+1));
            sprintf(it->value,"%s|%s",latitude,longitute);

            pthread_mutex_unlock(&list_mutex);
            return;
        }
    }
    char* key=(char*)malloc(sizeof(char)*(strlen(user_id)+1));
    strcpy(key,user_id);
    int len = strlen(latitude)+strlen(longitute)+1;
    char* value=(char*)malloc(sizeof(char)*(len+1));
    sprintf(value,"%s|%s",latitude,longitute);
    add_data(&location_list,key,value);
    pthread_mutex_unlock(&list_mutex);
}

const char* pop_location(const char* user_id){
    pthread_mutex_lock(&list_mutex);
    for(node* it = location_list.head;it!=NULL;it=it->next){
        if(strcmp(user_id,it->key)==0){
            pthread_mutex_unlock(&list_mutex);
            return it->value;
        }
    }
    pthread_mutex_unlock(&list_mutex);
    return NULL;
}
void post_location(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    const char* latitude = find_value(req.parameter,"latitude");
    const char* longitude = find_value(req.parameter,"longitude");

    if(token==NULL || latitude ==NULL || longitude==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    MYSQL       *connection=NULL;
    connection = connection_pop();
    int idx=getUserIdx(connection,token);
    char num[30];
    sprintf(num,"%d",idx);
    push_location(num,latitude,longitude);

    char body[50];
    sprintf(body,"{"
                 "\"status\":\"OK\""
                 "}");
    connection_push(connection);
    response(clnt_sock,200,"OK",req.version,NULL,body);
}

void get_location(int clnt_sock,request req){
    const char* token =find_value(req.parameter,"token");
    const char* user_id = find_value(req.parameter,"user_ID");

    if(token==NULL || user_id==NULL){
        response(clnt_sock,500,"Internal Server Error",req.version,NULL,"missing parameters");
        return;
    }
    char body[256];
    const char* location = pop_location(user_id);
    if(location==NULL){
        sprintf(body,"{"
                    "\"latitude\":0.0,"
                    "\"longitude\":0.0"
                    "}");

    }
    else {
        int len = strlen(location);
        int p = find_idx(location, '|');
        printf("%d\n", p);
        int alen = p;
        int blen = len - (p + 1);
        char *latitude = (char *) malloc(sizeof(char) * (alen + 1));
        char *longtitude = (char *) malloc(sizeof(char) * (blen + 1));
        strncpy(latitude, location, alen);
        latitude[alen] = 0;
        strncpy(longtitude, location + p + 1, blen);
        longtitude[blen] = 0;
        sprintf(body, "{"
                     "\"latitude\":%s,"
                     "\"longitude\":%s"
                     "}", latitude, longtitude);
        free(latitude);
        free(longtitude);
    }
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
    else if(regexec(&todolist_r,req.path,0,NULL,0)==0){
        if(strcmp(req.method,"get")==0 || strcmp(req.method,"GET")==0){
            get_todolist(clnt_sock,req);
        }
        else if(strcmp(req.method,"post")==0 || strcmp(req.method,"POST")==0){
            post_todolist(clnt_sock,req);
        }
    }
    else if(regexec(&progress_r,req.path,0,NULL,0)==0){
        if(strcmp(req.method,"get")==0 || strcmp(req.method,"GET")==0){
            get_progress(clnt_sock,req);
        }
        else if(strcmp(req.method,"post")==0 || strcmp(req.method,"POST")==0){
            post_progress(clnt_sock,req);
        }
    }
    else if(regexec(&chat_r,req.path,0,NULL,0)==0){
        create_chat(clnt_sock,req);
    }
    else if(regexec(&chatlist_r,req.path,0,NULL,0)==0){
        get_chatlist(clnt_sock,req);
    }
    else if(regexec(&location_r,req.path,0,NULL,0)==0){
        if(strcmp(req.method,"get")==0 || strcmp(req.method,"GET")==0){
            get_location(clnt_sock,req);
        }
        else if(strcmp(req.method,"post")==0 || strcmp(req.method,"POST")==0){
            post_location(clnt_sock,req);
        }
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
    regcomp(&todolist_r,todolist_pattern,REG_EXTENDED);
    regcomp(&progress_r,progess_pattern,REG_EXTENDED);
    regcomp(&chat_r,chat_pattern,REG_EXTENDED);
    regcomp(&chatlist_r,chatlist_pattern,REG_EXTENDED);
    regcomp(&location_r,location_pattern,REG_EXTENDED);
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
