#include <stdio.h>
#include "Queue.h"
#include <stdbool.h>

bool queueConstructorTest(){
    Queue queue = queueConstructor(10);
    if (queue == NULL){
        return false;
    }
    queueDestructor(queue);
    return true;
}

bool enqueueTest(){

    Queue queue = queueConstructor(10);
    struct timeval time_of_arrival;

    gettimeofday(&time_of_arrival, NULL);


    enqueue(queue, 1, time_of_arrival);
    if (getHeadsArrivalTime(queue).tv_sec != time_of_arrival.tv_sec){
        return false;
    }
    if (getHeadsArrivalTime(queue).tv_usec != time_of_arrival.tv_usec){
        return false;
    }
    if (getQueueSize(queue) != 1){
        return false;
    }

    if (empty(queue)){
        return false;
    }

    queueDestructor(queue);
    return true;

}

bool dequeueTest(){
    Queue queue = queueConstructor(10);
    struct timeval time_of_arrival;

    gettimeofday(&time_of_arrival, NULL);

    enqueue(queue, 1, time_of_arrival);
    if (getQueueSize(queue) != 1){
        return false;
    }

    if (dequeue(queue) != 1){
        return false;
    }

    if (getQueueSize(queue) != 0){
        return false;
    }

    if (!empty(queue)){
        return false;
    }

    queueDestructor(queue);
    return true;
}

bool fullQueueTest(){
    Queue queue = queueConstructor(1);
    struct timeval time_of_arrival;

    gettimeofday(&time_of_arrival, NULL);

    enqueue(queue, 1, time_of_arrival);
    if (!full(queue)){
        return false;
    }

    queueDestructor(queue);
    return true;
}

bool dequeueByNumberInLineTest(){
    Queue queue = queueConstructor(10);
    struct timeval time_of_arrival_1;
    struct timeval time_of_arrival_2;
    struct timeval time_of_arrival_3;

    gettimeofday(&time_of_arrival_1, NULL);
    enqueue(queue, 1, time_of_arrival_1);

    gettimeofday(&time_of_arrival_2, NULL);
    enqueue(queue, 2, time_of_arrival_2);

    gettimeofday(&time_of_arrival_3, NULL);
    enqueue(queue, 3, time_of_arrival_3);

    if (dequeueByNumberInLine(queue, 1) != 2){
        return false;
    }

    if (getHeadsArrivalTime(queue).tv_sec != time_of_arrival_1.tv_sec){
        return false;
    }

    if (getHeadsArrivalTime(queue).tv_usec != time_of_arrival_1.tv_usec){
        return false;
    }

    if (getQueueSize(queue) != 2){
        return false;
    }

    if (dequeueByNumberInLine(queue, 1) != 3){
        return false;
    }

    if (getQueueSize(queue) != 1){
        return false;
    }

    if (getHeadsArrivalTime(queue).tv_sec != time_of_arrival_1.tv_sec){
        return false;
    }
    if (getHeadsArrivalTime(queue).tv_usec != time_of_arrival_1.tv_usec){
        return false;
    }


    if (dequeueByNumberInLine(queue, 0) != 1){
        return false;
    }

    if (getQueueSize(queue) != 0){
        return false;
    }
    if (!empty(queue)){
        return false;
    }

    queueDestructor(queue);
    return true;

}

int main(void) {
    if (queueConstructorTest()){
        printf("queueConstructorTest passed\n");
    }
    else {
        printf("queueConstructorTest failed\n");
    }

    if (enqueueTest()){
        printf("enqueueTest passed\n");
    }
    else {
        printf("enqueueTest failed\n");
    }

    if (dequeueTest()){
        printf("dequeueTest passed\n");
    }
    else {
        printf("dequeueTest failed\n");
    }

    if (fullQueueTest()){
        printf("fullQueueTest passed\n");
    }
    else {
        printf("fullQueueTest failed\n");
    }

    if (dequeueByNumberInLineTest()){
        printf("dequeueByNumberInLineTest passed\n");
    }
    else {
        printf("dequeueByNumberInLineTest failed\n");
    }

    return 0;
}
