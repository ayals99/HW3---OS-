#include "segel.h"
#include "request.h"
#include "Queue.h"
#include <pthread.h>

#define NUMBER_OF_SERVER_ARGUMENTS 5
#define ZERO 0

void* threadRequestHandler(void* argument);

void overloadHandler(int connfd, char* scheduleAlgorithm);

// size of Queue_working:
int number_of_working_threads = ZERO;

Queue waitingQueue = NULL;

// TODO: Create three arrays that will act as counters:
requestCounterArray DynamicRequests;
requestCounterArray StaticRequests;
requestCounterArray OverallRequests;

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

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    int numberOfThreads, maxQueueSize;
    char* scheduleAlgorithm = NULL;

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

    // TODO: Initialize all three arrays to 0.


    listenfd = Open_listenfd(port);


    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*)&clientlen);

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //

        requestHandle(connfd, DynamicRequests, StaticRequests, OverallRequests);

        Close(connfd);
    }

}


    


 
