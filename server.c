#include "segel.h"
#include "request.h"
#include "Queue.h"
#include <pthread.h>

#define NUMBER_OF_SERVER_ARGUMENTS 5
#define ZERO 0
#define IDENTICAL 0

// Scheduling algorithms: Must be one of "block", "dt", "dh", "bf" or "random".
#define BLOCK_ALGORITHM "block"
#define DROP_TAIL_ALGORITHM "dt"
#define DROP_HEAD_ALGORITHM "dh"
#define BLOCK_FLUSH_ALGORITHM "bf"
#define DROP_RANDOM_ALGORITHM "random"

void* threadRequestHandler(void* argument);

bool handleOverload(char* scheduleAlgorithm, int connfd);

/** Mutex and condition variables: */
pthread_mutex_t lock;
pthread_cond_t conditionBufferAvailable;
pthread_cond_t conditionQueueEmpty;

// A queue that will hold the requests that are waiting to be handled:
Queue waitingQueue = NULL;

// TODO: Create three arrays that will act as counters:
int* DynamicRequests = NULL;
int* StaticRequests = NULL;
int* OverallRequests = NULL;

int threadsAtWorkCounter = 0;

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

/**
Format for command line:
    ./server [portnum] [threads] [queue_size] [schedalg]

The command line arguments to your web server are to be interpreted as follows:
    portnum:
            the port number that the web server should listen on; the basic web
            server already handles this argument.
    threads:
            the number of worker threads that should be created within the web
            server. Must be a positive integer.
    queue_size:
        the number of request connections that can be accepted at one time.
        Must be a positive integer.
        Note that it is not an error for more or less threads to be created than buffers.
    schedalg:
        the scheduling algorithm to be performed. Must be one of "block", "dt",
        "dh", "bf" or "random".
**/


void getargs(int* port, int argc, char *argv[],int* numberOfThreads,
             int* maxQueueSize, char* scheduleAlgorithm)
{
    if (argc < NUMBER_OF_SERVER_ARGUMENTS) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *numberOfThreads = atoi(argv[2]);
    *maxQueueSize = atoi(argv[3]);
    scheduleAlgorithm = argv[4];
}


pthread_t* createThreads(int numberOfThreads){
    //      Function that will create the threads.
    //      The threads should be created in a for loop.
    //      The number of threads created should be equal to the number of threads
    //      specified in the command line arguments.
    pthread_t* threadsArray = malloc(sizeof(pthread_t) * numberOfThreads);
    // if malloc failed:
    if (threadsArray == NULL) {
        perror("Error: malloc for threadsArray failed\n");
        exit(1);
    }
    for(int i = 0; i < numberOfThreads; i++){
        // TODO: add function instead of FUNCTION
        int* threadID = malloc(sizeof(int));
        // if malloc for threadID failed:
        if (threadID == NULL) {
            perror("Error: malloc for threadID failed\n");
            exit(1);
        }
        *threadID = i;
        pthread_create(&threadsArray[i], NULL, threadRequestHandler, (void*)threadID);
    }
    return threadsArray;
}

    //TODO: check if we need to wait for all threads to finish (by using pthread_join)
    // and then free the array

    //void destroyThreads(pthread_t* threadsArray, int numberOfThreads){
    //    for(int i = 0; i < numberOfThreads; i++){
    //        pthread_join(threadsArray[i], NULL);
    //    }
    //    free(threadsArray);
    //}


// From the homework instructions:
// block : your code for the listening (main) thread should block (not busy wait!) until a
//          buffer becomes available and then handle the request.
// drop_tail : your code should drop the new request immediately by closing the socket
//              and continue listening for new requests.
// drop_head : your code should drop the oldest request in the queue that is not
//            currently being processed by a thread and add the new request to the end of the
//            queue.
// block_flush : your code for the listening (main) thread should block (not busy wait!)
//               until the queue is empty and none of the threads handles a request and then drop
//               the request.
// Bonus - drop_random : when the queue is full and a new request arrives, drop
//                      50% of the requests in the queue (that are not handled
//                      by a thread) randomly.
//                      You can use the rand() function to choose which to drop.
//                      An example of this function is in output.c.
//                      Once old requests have been dropped,
//                      you can add the new request to the queue.

/** return values:
 * // false if the new request was dropped
 * // true if the new request needs to be enqueued */
bool handleOverload(char* scheduleAlgorithm, int connfd){
    if (strcmp(scheduleAlgorithm, BLOCK_ALGORITHM) == IDENTICAL){
        // block until a buffer becomes available

        // TODO: write the block implementation
    }
    else if (strcmp(scheduleAlgorithm, DROP_TAIL_ALGORITHM) == IDENTICAL){
        // drop_tail: drop new request by closing the socket and continue listening for new requests

        // TODO: drop_tail
        return false;
    }
    else if (strcmp(scheduleAlgorithm, DROP_HEAD_ALGORITHM) == IDENTICAL){
        // drop_head:
        // TODO: drop_head

//        // drop the oldest request in the queue that is not currently being processed by a thread
//        dequeue(queue);

        return true;

    }
    else if (strcmp(scheduleAlgorithm, BLOCK_FLUSH_ALGORITHM) == IDENTICAL){
        // block_flush
        // TODO: block_flush
    }
    else if (strcmp(scheduleAlgorithm, DROP_RANDOM_ALGORITHM) == IDENTICAL){
        // drop_random

        // Calculate 50% of the requests in the queue:
        // Question 409 in piazza says to round upwards,
        // so we will add 1 to the division
        // before dividing by 2.
        int numberOfRequestsToDrop = (getQueueSize(waitingQueue ) + 1) / 2 ;

        // drop the requests, in a for loop:
        for (int i = 0; i < numberOfRequestsToDrop; i++){
            // generate a random number between 0 and the size of the queue - 1
            // notice that rand() return a number between 0 and RAND_MAX, so
            // we need to use the modulo operator to get a number between 0 and
            // the size of the queue - 1
            int randomIndexInQueue = rand() % getQueueSize(waitingQueue);

            // drop the request in index randomNumberInRange
            // by de-queueing it from the waiting queue
            dequeueByNumberInLine(waitingQueue, randomIndexInQueue);
        }
    }
    else {
        perror("Error: Invalid scheduling algorithm\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    int numberOfThreads, maxQueueSize;
    char* scheduleAlgorithm = NULL;

    // A counter for the amount of threads that area working at each given time:
    int number_of_working_threads = ZERO;


    getargs(&port, argc, argv, &numberOfThreads,
            &maxQueueSize, scheduleAlgorithm);


    pthread_t* threadPool = createThreads(numberOfThreads);
    // TODO: make sure that all threads are blocked when created
    // so they won't start working before a request comes in.
    // might want to create a function that will block the threads.
    // we can tell the amount of threads that need to be blocked by: number of threads - number of requests.




    /** Explanation:
    // Three arrays that will act as counters.
    // each index in the array represents a thread, and the value at that index
    // represents the number of requests that thread has handled.
    // We will need one array for Dynamic, one array for Static and one for
    // Overall requests handled.
    **/


    DynamicRequests = malloc(sizeof(*DynamicRequests) * numberOfThreads);
    StaticRequests = malloc(sizeof(*StaticRequests) * numberOfThreads);
    OverallRequests = malloc(sizeof(*OverallRequests) * numberOfThreads);

    // if malloc failed:
    if (DynamicRequests == NULL || StaticRequests == NULL || OverallRequests == NULL) {
        perror("Error: malloc for request counter array failed\n");
        exit(1);
    }

    // TODO: Initialize all three arrays to 0.
    for (int i = 0; i < numberOfThreads; i++){
        DynamicRequests[i] = 0;
        StaticRequests[i] = 0;
        OverallRequests[i] = 0;
    }


    listenfd = Open_listenfd(port);


    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*)&clientlen);


        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.


//        // TODO: These two lines need to be in the threadRequestHandler function
//        requestHandle(connfd, timeOfArrival, timeOfHandling, DynamicRequests, StaticRequests, OverallRequests);
//        Close(connfd);

        // record the time of arrival before locking the mutex:
        struct timeval timeOfArrival;
        gettimeofday(&timeOfArrival, NULL);

        // Lock the mutex in order to send the request to be handled.
        // We also need the lock for the changes we might make to the queue if the buffer is full
        pthread_mutex_lock(&lock);

        // If all threads are busy and the queue is full, we need to handle the overload:
        bool needToEnqueue = true;
        if (threadsAtWorkCounter == numberOfThreads && full(waitingQueue)){
            // get a return status handleOverload function in case of drop tail (because then we don't need to enque)
            needToEnqueue = handleOverload(scheduleAlgorithm, connfd);
        }

        // add the request to the queue (if we dropped the request, we don't need to enqueue):
        if (needToEnqueue) {
            enqueue(waitingQueue, connfd, timeOfArrival);
        }

        // TODO: make sure this is the right signal to send
        // signal the condition variable that a buffer is available:
        pthread_cond_signal(&conditionBufferAvailable);

        // unlock the mutex:
        pthread_mutex_unlock(&lock);
    }
}


    


 
