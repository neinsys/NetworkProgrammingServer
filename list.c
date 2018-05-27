

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void add_data(dic_list* list,char* key,char* value){
    node* p=(node*)malloc(sizeof(node));
    p->key=key;p->value=value;p->next=NULL;
    if(list->head==NULL && list->tail==NULL){
        list->head=list->tail=p;
    }
    else{
        list->tail->next=p;
        list->tail=p;
    }
}

void clear_list(dic_list* list){
    for(node* it=list->head;it!=NULL;){
        node* tmp=it;
        it=it->next;
        free(tmp->key);
        free(tmp->value);
        free(tmp);
    }
}

const char* find_value(dic_list* list,const char* key){
    for(node* it=list->head;it!=NULL;it=it->next){
        if(strcmp(it->key,key)==0)return it->value;
    }
    return NULL;
}