#ifndef __REQUEST_H__

#include "Queue.h"

#define requestCounterArray int*


void requestHandle(int fd, struct timeval timeOfArrival,
                   struct timeval timeOfHandling,
                   requestCounterArray DynamicArray,
                   requestCounterArray StaticArray,
                   requestCounterArray OverallArray,
                   int threadID, Queue queue, int* activeThreads);

#endif
