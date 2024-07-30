#ifndef __REQUEST_H__

typdef int* requestArray;

void requestHandle(int fd , requestArray DynamicArray,
                   requestArray StaticArray, requestArray OverallArray);

#endif
