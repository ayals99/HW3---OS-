#ifndef WEBSERVER_FILES_QUEUE_H
#define WEBSERVER_FILES_QUEUE_H

#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>

#define EMPTY_QUEUE (-1)
#define OUT_OF_QUEUE_BOUNDS (-2)
#define INITIAL_QUEUE_SIZE 0

/** Typdefs **/
typedef struct node* Node;
typedef struct queue* Queue;

/** Constructors and Destructors **/
Queue queueConstructor(int max_size);
void queueDestructor(Queue queue);

/** Queue implementation **/
void enqueue(Queue queue, int fd, struct timeval time_of_arrival);
int dequeue(Queue queue);
int dequeueByNumberInLine(Queue queue, int numberInLine);
struct timeval getHeadsArrivalTime(Queue queue);
int getQueueSize(Queue queue);
bool empty(Queue queue);
bool full(Queue queue);

#endif //WEBSERVER_FILES_QUEUE_H
