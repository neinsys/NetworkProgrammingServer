//
// Created by nein on 18. 6. 7.
//

#include "connection_queue.h"
#include <pthread.h>
#include <mysql.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdbool.h>



const char* db_host = "domjudge.ceyzmi2ecctb.ap-northeast-2.rds.amazonaws.com";
const char* db_user = "ssulegram";
const char* db_password = "nein7961!";
const char* db_name = "ssulegram";


MYSQL** Queue;

int N;
int start;
int end;

pthread_mutex_t push_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pop_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t sem;

void Queue_Init(int n){
    N=n;
    Queue=(MYSQL**)malloc(sizeof(MYSQL*)*N);
    for(int i=0;i<N;i++) {
        Queue[i]=(MYSQL*)malloc(sizeof(MYSQL));
        mysql_init(Queue[i]);
        unsigned int timeout = 3000000;
        bool auto_reconnect=true;
        mysql_options(Queue[i],MYSQL_OPT_CONNECT_TIMEOUT,(unsigned int *)&timeout);
        mysql_options(Queue[i],MYSQL_OPT_RECONNECT,(bool*)&auto_reconnect);
        if(!mysql_real_connect(Queue[i], db_host,
                           db_user, db_password,
                           db_name, 3306,
                           (char *)NULL, 0)){
            fprintf(stderr, "Mysql connection error : %s\n", mysql_error(Queue[i]));
        }
        mysql_options(Queue[i],MYSQL_OPT_CONNECT_TIMEOUT,(unsigned int *)&timeout);
        mysql_options(Queue[i],MYSQL_OPT_RECONNECT,(bool*)&auto_reconnect);
    }
    start=0;
    end=0;

    sem_init(&sem,0,N);
}


MYSQL* connection_pop(){
   /* pthread_mutex_lock(&push_mutex);
    sem_wait(&sem);
    MYSQL* ret = Queue[start];
    start=(start+1)%N;
    pthread_mutex_unlock(&push_mutex);
    if(mysql_ping(ret)){
        printf("mysql reconnect\n");
        mysql_close(ret);
        mysql_init(ret);
        unsigned int timeout = 3000000;
        bool auto_reconnect=true;
        mysql_options(ret,MYSQL_OPT_CONNECT_TIMEOUT,(unsigned int *)&timeout);
        mysql_options(ret,MYSQL_OPT_RECONNECT,(bool*)&auto_reconnect);
        if(!mysql_real_connect(ret, db_host,
                               db_user, db_password,
                               db_name, 3306,
                               (char *)NULL, 0)){
            fprintf(stderr, "Mysql connection error : %s\n", mysql_error(ret));
        }
        mysql_options(ret,MYSQL_OPT_CONNECT_TIMEOUT,(unsigned int *)&timeout);
        mysql_options(ret,MYSQL_OPT_RECONNECT,(bool*)&auto_reconnect);
    }*/
    MYSQL* ret;
    mysql_init(ret);
    unsigned int timeout = 3000000;
    bool auto_reconnect=true;
    mysql_options(ret,MYSQL_OPT_CONNECT_TIMEOUT,(unsigned int *)&timeout);
    mysql_options(ret,MYSQL_OPT_RECONNECT,(bool*)&auto_reconnect);
    if(!mysql_real_connect(ret, db_host,
                           db_user, db_password,
                           db_name, 3306,
                           (char *)NULL, 0)){
        fprintf(stderr, "Mysql connection error : %s\n", mysql_error(ret));
    }
    mysql_options(ret,MYSQL_OPT_CONNECT_TIMEOUT,(unsigned int *)&timeout);
    mysql_options(ret,MYSQL_OPT_RECONNECT,(bool*)&auto_reconnect);
    return ret;
}

void connection_push(MYSQL* mysql){
    mysql_close(mysql);
    /*pthread_mutex_lock(&pop_mutex);
    Queue[end]=mysql;
    end=(end+1)%N;
    sem_post(&sem);
    pthread_mutex_unlock(&pop_mutex);*/
}


