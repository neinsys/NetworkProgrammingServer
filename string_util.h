//
// Created by nein on 18. 6. 12.
//

#ifndef TTAC_STRING_UTIL_H
#define TTAC_STRING_UTIL_H


int find_idx(const char* s,char ch);
int find_str(const char* s,const char* p);

int min(int a,int b) ;
int prefixcmp(const char* a,const char* b);


int isHex(char ch);

int toHex(char ch);

void toLower(char* str);
#endif //TTAC_STRING_UTIL_H
