#include "segel.h"
#include "request.h"
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

void* threadRequestHandler(void* threadID);

/** Mutex and condition variables: */
pthread_mutex_t lock;

// Condition variables:
pthread_cond_t conditionQueueNotEmpty; // for the threads in the pool to wait until there is a request in the queue
pthread_cond_t conditionBlockFlush; // for the main thread to wait until the queue is empty and all threads are idle (for block_flush)
pthread_cond_t conditionBufferAvailable; // for the main thread to wait until a buffer becomes available (for block)

// A queue that will hold the requests that are waiting to be handled:
Queue waitingQueue = NULL;

int* DynamicRequests = NULL;
int* StaticRequests = NULL;
int* OverallRequests = NULL;

int threadsAtWorkCounter = ZERO;

// server.c: A very, very simple web server

// To run:
//  ./server <portnum (above 2000)>

// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c

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
             int* maxQueueSize, char** scheduleAlgorithmPointer){
    // according to Question 434 in piazza, we can assume that all arguments are valid
    if (argc < NUMBER_OF_SERVER_ARGUMENTS) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *numberOfThreads = atoi(argv[2]);
    *maxQueueSize = atoi(argv[3]);
    *scheduleAlgorithmPointer = argv[4];
}

void* threadRequestHandler(void* threadID){
    // we receive the threadID as a void pointer, so we need to cast it to an int pointer:
    int* threadIDPointer = (int*)threadID;
    int threadIDValue = *threadIDPointer;

    while (1){
        // Lock the mutex in order to check if there are requests in the queue:
        pthread_mutex_lock(&lock);

        // If the queue is empty, wait until there is a request in the queue:
        while (empty(waitingQueue)){
            pthread_cond_wait(&conditionQueueNotEmpty, &lock);
        }

        // Increment the number of threads that are currently working:
        threadsAtWorkCounter++;

        // Dequeue the request from the queue:
        struct timeval timeOfArrival = getHeadsArrivalTime(waitingQueue);
        int connfd = dequeue(waitingQueue);

        // Record the time of handling before handling the request:
        struct timeval timeOfHandling;
        gettimeofday(&timeOfHandling, NULL);

        // Unlock the mutex:
        pthread_mutex_unlock(&lock);


        // TODO: check if need to lock the mutex before calling requestHandle
        // Handle the request:
        requestHandle(connfd, timeOfArrival, timeOfHandling,
                      DynamicRequests, StaticRequests, OverallRequests,
                      threadIDValue, waitingQueue, &threadsAtWorkCounter);
        Close(connfd);

        // Lock the mutex in order to update the number of requests handled by the thread:
        pthread_mutex_lock(&lock);

        threadsAtWorkCounter--;

        // If the queue is empty and all threads are idle, signal the main thread:
        if (empty(waitingQueue) && (threadsAtWorkCounter == ZERO) ){
            pthread_cond_signal(&conditionBlockFlush);
        }

        // signal the main thread that a buffer is available, in case it's waiting on "block" algorithm:
        pthread_cond_signal(&conditionBufferAvailable);

        // Unlock the mutex:
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    int numberOfThreads, maxRequestsAllowed;
    char* scheduleAlgorithm = NULL;

    getargs(&port, argc, argv, &numberOfThreads,
            &maxRequestsAllowed, &scheduleAlgorithm);

    waitingQueue = queueConstructor();

    // Initialize the lock and the condition variables:
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&conditionQueueNotEmpty, NULL);
    pthread_cond_init(&conditionBlockFlush, NULL);
    pthread_cond_init(&conditionBufferAvailable, NULL);

    pthread_t* threadPool = malloc(sizeof(pthread_t)*numberOfThreads);

    for(int i = 0; i < numberOfThreads; i++){
        int* threadID = malloc(sizeof(int));

        // if malloc for threadID failed:
        if (threadID == NULL) {
            perror("Error: malloc for threadID failed\n");
            exit(1);
        }

        *threadID = i;
        pthread_create(&threadPool[i], NULL, threadRequestHandler, (void*)threadID);

        // error check for pthread_create:
        if (threadPool[i] < 0) { // 0 is success
            perror("Error: pthread_create failed\n");
            exit(1);
        }
    }

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
 
    //  Initialize all three arrays to 0.
    for (int i = 0; i < numberOfThreads; i++){
        DynamicRequests[i] = ZERO;
        StaticRequests[i] = ZERO;
        OverallRequests[i] = ZERO;
    }

    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*)&clientlen);

        // record the time of arrival before locking the mutex: (TA said to put this first)
        struct timeval timeOfArrival;
        gettimeofday(&timeOfArrival, NULL);

        // Lock the mutex in order to send the request to be handled and in the buffer is full
        pthread_mutex_lock(&lock);

        // If all threads are busy and the queue is full, we need to handle the overload:
        bool needToEnqueue = true; 
        bool addedRequestToQueue = true;

        while (threadsAtWorkCounter + getQueueSize(waitingQueue) == maxRequestsAllowed){
            // save the status that is returned from handleOverload in case of drop_tail
            // (because then we don't need to enqueue the request)
            if (strcmp(scheduleAlgorithm, BLOCK_ALGORITHM) == IDENTICAL){
                while(threadsAtWorkCounter + getQueueSize(waitingQueue) == maxRequestsAllowed){
                    // block until a buffer becomes available
                    pthread_cond_wait(&conditionBufferAvailable, &lock);
                }
            }
            else if (strcmp(scheduleAlgorithm, DROP_TAIL_ALGORITHM) == IDENTICAL){
                // drop_tail: drop new request by closing the socket and continue listening for new requests
                Close(connfd);
                addedRequestToQueue = false;
                needToEnqueue = false; // new request will NOT be added to the queue
                break;
            }
            else if (strcmp(scheduleAlgorithm, DROP_HEAD_ALGORITHM) == IDENTICAL){ // drop_head
                if (empty(waitingQueue)){
                    Close(connfd);
                    addedRequestToQueue = false;
                    needToEnqueue = false; // new request will NOT be added to the queue
                    break;
                }
                // drop the oldest request in the queue that is not currently being processed by a thread:
                int headFD = dequeue(waitingQueue);
                Close(headFD);
                addedRequestToQueue = false;
            }
            else if (strcmp(scheduleAlgorithm, BLOCK_FLUSH_ALGORITHM) == IDENTICAL){ // block_flush
                pthread_cond_wait(&conditionBlockFlush, &lock);
                // and then drop the request:
                Close(connfd);

                addedRequestToQueue = false;
                needToEnqueue = false; // new request will NOT be added to the queue
                break;
            }
            else { // (strcmp(scheduleAlgorithm, DROP_RANDOM_ALGORITHM) == IDENTICAL) // drop_random
                if (empty(waitingQueue)){
                    needToEnqueue = false; // new request will NOT be added to the queue
                    addedRequestToQueue = false;
                    Close(connfd);
                    break;
                }

                // Calculate 50% of the requests in the queue:
                // Question 409 in piazza says to round upwards,
                // so we will add 1 to the division before dividing by 2.
                int numberOfRequestsToDrop = (getQueueSize(waitingQueue) + 1) / 2;

                // drop the requests, in a for loop:
                for (int i = 0; i < numberOfRequestsToDrop; i++){
                    if (empty(waitingQueue)){
                        break;
                    }

                    // generate a random number between 0 and the size of the queue - 1
                    // notice that rand() return a number between 0 and RAND_MAX, so
                    // we need to use the modulo operator to get a number between 0 and
                    // the size of the queue - 1
                    int randomIndexInQueue = rand() % getQueueSize(waitingQueue);

                    // drop the request in index randomNumberInRange
                    // by de-queueing it from the waiting queue
                    int toClose = dequeueByNumberInLine(waitingQueue, randomIndexInQueue);
                    Close(toClose);
                }
                needToEnqueue = true;
                addedRequestToQueue = true;
                break;
            }
        }

        // add the request to the queue (if we dropped the request, we don't need to enqueue):
        if (needToEnqueue) {
            enqueue(waitingQueue, connfd, timeOfArrival);
        }

        // In drop_head we don't want to signal the threads to start working
        // because we're not adding a new request to the queue.
        if (addedRequestToQueue) {
            // send a signal that there is now request waiting in the queue:
            pthread_cond_signal(&conditionQueueNotEmpty);
        }

        // unlock the mutex:
        pthread_mutex_unlock(&lock);
    }

    return 0;
}