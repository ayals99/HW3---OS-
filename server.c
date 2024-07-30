#include "segel.h"
#include "request.h"
#define NUMBER_OF_SERVER_ARGUMENTS 5

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


// HW3: Parse the new arguments too
// TODO: add more parameters to the function
void getargs(int *port, int argc, char *argv[])
{
    if (argc < NUMBER_OF_SERVER_ARGUMENTS) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
 // TODO: add the other arguments to the function
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);

    // 
    // HW3: Create some threads...
    //

    // TODO: Create three arrays that will act as counters:

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
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);


        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //
        requestHandle(connfd);

        Close(connfd);
    }

}


    


 
