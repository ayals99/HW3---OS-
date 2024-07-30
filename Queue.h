#ifndef WEBSERVER_FILES_QUEUE_H
#define WEBSERVER_FILES_QUEUE_H
#include "segel.h"


/** Typdefs **/
typedef struct node* Node;
typedef struct queue* Queue;

/** Constructors and Destructors **/
Queue queueConstructor(int max_size);
void queueDestructor(Queue queue);



#endif //WEBSERVER_FILES_QUEUE_H
