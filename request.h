#ifndef __REQUEST_H__

#define requestCounterArray int*


void requestHandle(int fd, struct timeval timeOfArrival,
                   struct timeval timeOfHandling,
                   requestCounterArray DynamicArray,
                   requestCounterArray StaticArray,
                   requestCounterArray OverallArray);

#endif
