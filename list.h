#ifndef __LIST_H__
#define __LIST_H__



typedef struct _list{
    struct _node* head,*tail;
}header_list;
typedef struct _node{
    char* key;
    char* value;
    struct _node* next;
}node;


void add_header(header_list* list,char* key,char* value);

void clear_list(header_list* list);

char* find_value(header_list* list,const char* key);

#endif