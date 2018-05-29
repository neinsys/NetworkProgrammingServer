//
// Created by nein on 18. 5. 29.
//

#ifndef TTAC_HTTP_H
#define TTAC_HTTP_H
#include "list.h"

typedef struct _request{
    char* method;
    char* path;
    char* version;
    dic_list* header;
    char* body;
    dic_list* parameter;
}request;

request parse_request(int sock_fds);
void clear_requset(request req);
void print_request_info(request req);
void response(int sock,int status_code,const char* status_msg,const char* http_version,const dic_list* header,const char* body);


#endif //TTAC_HTTP_H
