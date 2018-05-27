#ifndef __LIST_H__
#define __LIST_H__



typedef struct _list{
    struct _node* head,*tail;
}dic_list;
typedef struct _node{
    char* key;
    char* value;
    struct _node* next;
}node;


void add_data(dic_list* list,char* key,char* value);

void clear_list(dic_list* list);

const char* find_value(dic_list* list,const char* key);

#endif