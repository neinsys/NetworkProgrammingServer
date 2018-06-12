//
// Created by nein on 18. 6. 12.
//

#include "string_util.h"


#include <string.h>

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
