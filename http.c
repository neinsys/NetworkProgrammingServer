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
#include "http.h"

int isHex(char ch){
    return (ch>='0' && ch<='9') || (ch>='a' && ch<='f') || (ch>='A' && ch<='F');
}

int toHex(char ch){
    if(ch>='0' && ch<='9'){
        return ch-'0';
    }
    else if(ch>='a' && ch<='f'){
        return ch-'a'+10;
    }
    else if(ch>='A' && ch<='F'){
        return ch-'A'+10;
    }
    return -1;
}

void toLower(char* str){
    for(int i=0;str[i];i++){
      if(str[i]>='A' && str[i]<='Z')str[i]+='a'-'A';
    }
}


char* url_decode(char* old){
    int len = strlen(old);
    char* new = (char*)malloc(sizeof(char)*(len+1));
    new[0]=0;
    for(int i=0,j=0;i<len;i++,j++){
        if(old[i]=='%' && isHex(old[i+1]) && isHex(old[i+2])) {
            new[j] = toHex(old[i + 1]) * 16 + toHex(old[i + 2]);
            i += 2;
        }
        else{
            new[j]=old[i];
        }
        new[j+1]=0;
    }
    return new;
}

int find_idx(const char* s,char ch){
    for(int i=0;s[i];i++){
        if(s[i]==ch)return i;
    }
    return -1;
}
int find_str(const char* s,const char* p){
    int len=strlen(p);
    for(int i=0;s[i];i++){
        if(i>=len-1 && strncmp(s+(i-len+1),p,len)==0){
            return i;
        }
    }
    return -1;

}

int min(int a,int b) {
    if(a<b)return a;
    return b;
}
int prefixcmp(const char* a,const char* b){
    int len = min(strlen(a),strlen(b));
    return strncmp(a,b,len);
}

void parse_parameter(dic_list* list,char* params){
    int idx=0;
    if(params==NULL)return;
    int len=strlen(params);
    if(len==0)return;
    printf("%s\n",params);
    do{
        int next=find_idx(params+idx,'&')+idx;
        if(next-idx==-1)next=len;
        int equal=find_idx(params+idx,'=')+idx;
        if(equal<next && equal!=-1){
            char* key;
            char* value;
            char* tmp;
            int key_len=equal-idx;
            int value_len=next-equal-1;
            key=(char*)malloc(sizeof(char)*(key_len+1));
            value=(char*)malloc(sizeof(char)*(value_len+1));
            strncpy(key,params+idx,key_len);
            key[key_len]=0;
            strncpy(value,params+equal+1,value_len);
            value[value_len]=0;

            tmp = url_decode(key);
            free(key);
            key=tmp;
            tmp = url_decode(value);
            free(value);
            value=tmp;
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
        //boudnary
        int next=find_str(body+idx,boundary)+idx;
        if(strncmp(body+next+1,"--",2)==0)break;
        next+=2;
        idx=next+1;

        //paramter header?
        char* key=NULL;
        char* filename=NULL;
        do{
            next=find_str(body+idx,"\r\n")+idx-1;
            if(next==idx){
                idx=next+2;
                break;
            }
            while(idx<next){
                int p=find_idx(body+idx,';')+idx;
                if(p-idx==-1 || p>next)p=next;
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
            idx=next+2;

        }while(idx<len);
        next=find_str(body+idx,boundary)+idx-bound_len;
        char* value=(char*)malloc(sizeof(char)*(next-idx-2+1));
        strncpy(value,body+idx,(next-idx-2));
        value[next-idx-2]=0;
        if(filename!=NULL) {
            char *tmp = (char *) malloc(sizeof(char) * (30));
            int len = strlen(filename);
            sprintf(tmp, "%d", len);
            int llen = strlen(tmp);
            tmp = (char *) realloc(tmp, sizeof(char) * (len + llen + 2 + strlen(value)));
            sprintf(tmp, "%d|%s|%s", len, filename, value);
            free(value);
            value = tmp;
        }
        else{
            char* tmp = url_decode(value);
            free(value);
            value=tmp;
        }
        //printf("%s|%s\n",key,value);
        add_data(list,key,value);
        idx=next+1;
    }while(idx<=len);
}
request parse_request(int sock_fds){
    stream s;
    stream_read_init(&s,sock_fds);
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
            int end=strlen(line);
            int value_len=end-idx-1;
            key=(char*)malloc(sizeof(char)*(key_len+1));
            value=(char*)malloc(sizeof(char)*(value_len+1));
            strncpy(key,line,key_len);
            key[key_len]=0;
            strncpy(value,line+idx+2,value_len);
            value[value_len]=0;
            toLower(key);
            add_data(req.header,key,value);
        }
        free(line);
    }
    const char* content_length=find_value(req.header,"content-length");
    if(content_length!=NULL){
        int len=atoi(content_length);
        req.body=read_sz(&s,len);
    }
    stream_destory(&s);

    if(strcmp(req.method,"get")==0 || strcmp(req.method,"GET")==0){
        int idx=find_idx(req.path,'?');
        if(idx>=0){
            parse_parameter(req.parameter,req.path+idx+1);
        }
    }
    if(strcmp(req.method,"post")==0 || strcmp(req.method,"POST")==0){
        const char* contentType = find_value(req.header,"content-type");
        if(prefixcmp(contentType,"application/x-www-form-urlencoded")==0){
            parse_parameter(req.parameter,req.body);
        }
        else if (prefixcmp(contentType,"multipart/form-data")==0){
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

void clear_requset(request req){
    free(req.body);
    free(req.method);
    free(req.path);
    free(req.version);
    clear_list(req.header);
    clear_list(req.parameter);
}
void print_request_info(request req){
    printf("%s|%s|%s\n",req.method,req.path,req.version);
    printf("header\n");
    for(node* it=req.header->head;it!=NULL;it=it->next){
        printf("%s|%s\n",it->key,it->value);
    }
    printf("parameter\n");
    for(node* it=req.parameter->head;it!=NULL;it=it->next){
        printf("%s|%s\n",it->key,it->value);
    }
    fflush(stdout);
}
void response(int sock,int status_code,const char* status_msg,const char* http_version,const dic_list* header,const char* body){
    dprintf(sock,"%s %d %s\r\n",http_version,status_code,status_msg);
    if(header!=NULL){
        for(node* it=header->head;it!=NULL;it=it->next){
            dprintf(sock,"%s: %s\r\n",it->key,it->value);
        }
    }
    dprintf(sock,"content-length: %lu\r\n",strlen(body));

    dprintf(sock,"\r\n");
    if(body!=NULL){
        dprintf(sock,"%s",body);
    }
}
