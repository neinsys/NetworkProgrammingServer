//
// Created by nein on 18. 6. 7.
//

#ifndef TTAC_CONNECTION_QUEUE_H
#define TTAC_CONNECTION_QUEUE_H

#include<mysql.h>

void Queue_Init(int n);
void connection_push(MYSQL* mysql);
MYSQL* connection_pop();

#endif //TTAC_CONNECTION_QUEUE_H
