//
// request.c: Does the bulk of the work for the web server.
// 

#include "segel.h"
#include "request.h"
#include <stdbool.h>

#define SKIP_SUFFIX ".skip"

extern pthread_mutex_t lock;
extern pthread_cond_t conditionQueueNotEmpty;
extern pthread_cond_t conditionBlockFlush;
extern pthread_cond_t conditionBufferAvailable;

// Also remove the suffix from the filename
bool suffixIsDotSkip(const char* filename) {
    const char dot = '.';
    if (filename == NULL) {
        return false;
    }

    char* suffix = strrchr(filename, dot);
    if (suffix == NULL) {
        return false;
    }
    if (strcmp(suffix, SKIP_SUFFIX) == 0){
        // remove the suffix by changing the memory contents to '\0' instead of '.'
        *suffix = '\0';
        return true;
    }
    return false;
}


// requestError(      fd,    filename,        "404",    "Not found", "OS-HW3 Server could not find this file");
void requestError(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg,
                            struct timeval arrivalTime, struct timeval dispatch,
                            int* StaticArray, int* DynamicArray, int* OverallArray, int threadID) {
   char buf[MAXLINE], body[MAXBUF];

   // Create the body of the error message
   sprintf(body, "<html><title>OS-HW3 Error</title>");
   sprintf(body, "%s<body bgcolor=""fffff"">\r\n", body);
   sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
   sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
   sprintf(body, "%s<hr>OS-HW3 Web Server\r\n", body);

   //Write out the header information for this response
   sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
   Rio_writen(fd, buf, strlen(buf));
   printf("%s", buf);

   sprintf(buf, "Content-Type: text/html\r\n");
   Rio_writen(fd, buf, strlen(buf));
   printf("%s", buf);

   sprintf(buf, "Content-Length: %lu\r\n", strlen(body));

   sprintf(buf, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buf, arrivalTime.tv_sec, arrivalTime.tv_usec);

   sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf, dispatch.tv_sec, dispatch.tv_usec);

   sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, threadID);

   sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf, OverallArray[threadID]);

   sprintf(buf, "%sStat-Thread-Static:: %d\r\n", buf, StaticArray[threadID]);

   sprintf(buf, "%sStat-Thread-Dynamic:: %d\r\n\r\n", buf, DynamicArray[threadID]);

   // Write out the content
   Rio_writen(fd, buf, strlen(buf));
   printf("%s", buf);
   Rio_writen(fd, body, strlen(body));
   printf("%s", body);
}


//
// Reads and discards everything up to an empty text line
//
void requestReadhdrs(rio_t *rp)
{
    char buf[MAXLINE];

   Rio_readlineb(rp, buf, MAXLINE);
   while (strcmp(buf, "\r\n")) {
      Rio_readlineb(rp, buf, MAXLINE);
   }
   return;
}

//
// Return 1 if static, 0 if dynamic content
// Calculates filename (and cgiargs, for dynamic) from uri
//
int requestParseURI(char *uri, char *filename, char *cgiargs) 
{
   char *ptr;

   if (strstr(uri, "..")) {
      sprintf(filename, "./public/home.html");
      return 1;
   }

   if (!strstr(uri, "cgi")) {
      // static
      strcpy(cgiargs, "");
      sprintf(filename, "./public/%s", uri);
      if (uri[strlen(uri)-1] == '/') {
         strcat(filename, "home.html");
      }
      return 1;
   } else {
      // dynamic
      ptr = index(uri, '?');
      if (ptr) {
         strcpy(cgiargs, ptr+1);
         *ptr = '\0';
      } else {
         strcpy(cgiargs, "");
      }
      sprintf(filename, "./public/%s", uri);
      return 0;
   }
}

//
// Fills in the filetype given the filename
//
void requestGetFiletype(char *filename, char *filetype)
{
   if (strstr(filename, ".html")) 
      strcpy(filetype, "text/html");
   else if (strstr(filename, ".gif")) 
      strcpy(filetype, "image/gif");
   else if (strstr(filename, ".jpg")) 
      strcpy(filetype, "image/jpeg");
   else 
      strcpy(filetype, "text/plain");
}

void requestServeDynamic(int fd, char *filename, char *cgiargs,
                         struct timeval arrivalTime, struct timeval dispatch,
                         int* DynamicArray, int* StaticArray, int* OverallArray, int threadID)
{
   char buf[MAXLINE], *emptylist[] = {NULL};

   // The server does only a little bit of the header.
   // The CGI script has to finish writing out the header.
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: OS-HW3 Web Server\r\n", buf);

    sprintf(buf, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buf, arrivalTime.tv_sec, arrivalTime.tv_usec);

    sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf, dispatch.tv_sec, dispatch.tv_usec);

    sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, threadID);

    sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf, OverallArray[threadID]);

    sprintf(buf, "%sStat-Thread-Static:: %d\r\n", buf, StaticArray[threadID]);

    sprintf(buf, "%sStat-Thread-Dynamic:: %d\r\n", buf, DynamicArray[threadID]);

    Rio_writen(fd, buf, strlen(buf));
    int pid = 0;
    if ((pid = Fork()) == 0) {
      /* Child process */
      Setenv("QUERY_STRING", cgiargs, 1);
      /* When the CGI process writes to stdout, it will instead go to the socket */
      Dup2(fd, STDOUT_FILENO);
      Execve(filename, emptylist, environ);
    }
    WaitPid(pid, NULL, WUNTRACED);
}


void requestServeStatic(int fd, char *filename, int filesize,
                        struct timeval arrival, struct timeval dispatch,
                        int* DynamicArray, int* StaticArray,  int* OverallArray, int threadID)
{
   int srcfd;
   char *srcp, filetype[MAXLINE], buf[MAXBUF];

   requestGetFiletype(filename, filetype);

   srcfd = Open(filename, O_RDONLY, 0);

   // Rather than call read() to read the file into memory,
   // which would require that we allocate a buffer, we memory-map the file
   srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
   Close(srcfd);

   // put together response
   sprintf(buf, "HTTP/1.0 200 OK\r\n");
   sprintf(buf, "%sServer: OS-HW3 Web Server\r\n", buf);
   sprintf(buf, "%sContent-Length: %d\r\n", buf, filesize);
   sprintf(buf, "%sContent-Type: %s\r\n", buf, filetype);
   sprintf(buf, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buf, arrival.tv_sec, arrival.tv_usec);

   sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf, dispatch.tv_sec, dispatch.tv_usec);

   sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, threadID);

   sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf, OverallArray[threadID]);

   sprintf(buf, "%sStat-Thread-Static:: %d\r\n", buf, StaticArray[threadID]);

   sprintf(buf, "%sStat-Thread-Dynamic:: %d\r\n\r\n", buf, DynamicArray[threadID]);

   Rio_writen(fd, buf, strlen(buf));

   //  Writes out to the client socket the memory-mapped file
   Rio_writen(fd, srcp, filesize);
   Munmap(srcp, filesize);

}


// handle a request
void requestHandle(int fd, struct timeval timeOfArrival,
                            struct timeval timeOfHandling,
                            requestCounterArray DynamicArray,
                            requestCounterArray StaticArray,
                            requestCounterArray OverallArray,
                            int threadID, Queue queue, int* activeThreadCount) {
    /** Increment the total number of requests: */
    OverallArray[threadID] += 1;

    /** calculate dispatch time: */
    struct timeval dispatch;
    timersub(&timeOfHandling, &timeOfArrival, &dispatch);

    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

   printf("%d\n", fd);
   Rio_readinitb(&rio, fd);
   Rio_readlineb(&rio, buf, MAXLINE);
   sscanf(buf, "%s %s %s", method, uri, version);

   printf("%s %s %s\n", method, uri, version);


   if (strcasecmp(method, "GET")) {
       requestError(fd, method, "501", "Not Implemented", "OS-HW3 Server does not implement this method",
                    timeOfArrival, dispatch,
                    StaticArray, DynamicArray, OverallArray, threadID);
       return;
   }

   requestReadhdrs(&rio);

   is_static = requestParseURI(uri, filename, cgiargs);

   bool skip = false;
   struct timeval latestArrivalTime;
   int latestFD;
   if (suffixIsDotSkip(filename)) { // if the suffix is .skip this function will remove the suffix


       pthread_mutex_lock(&lock);
       skip = true;

       latestArrivalTime = getTailsArrivalTime(queue);
       latestFD = dequeueLatest(queue);
       if (latestFD == EMPTY_QUEUE) {
           // Q@459 in piazza says that if the queue is empty then disregard the skip
           skip = false;
       }
       else{
           // the skipped request needs to be counted as part of the queue
           // and since we dequeue it, we need to increment the active thread count to compensate:
           (*activeThreadCount)++;
       }
       pthread_mutex_unlock(&lock);
   }

   if (stat(filename, &sbuf) < 0) {
       requestError(fd, filename, "404", "Not found", "OS-HW3 Server could not find this file",
                    timeOfArrival, dispatch,
                    StaticArray, DynamicArray, OverallArray, threadID);
       return;
   }

   if (is_static) {
      if ( !(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
         requestError(fd, filename, "403", "Forbidden", "OS-HW3 Server could not read this file",
                      timeOfArrival, dispatch,
                      StaticArray, DynamicArray, OverallArray, threadID);
         return;
      }
      StaticArray[threadID] += 1;
      requestServeStatic(fd, filename, sbuf.st_size, timeOfArrival, dispatch,
                         DynamicArray, StaticArray, OverallArray, threadID);
   }
   else {
      if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
         requestError(fd, filename, "403", "Forbidden", "OS-HW3 Server could not run this CGI program",
                      timeOfArrival, dispatch,
                      StaticArray, DynamicArray, OverallArray, threadID);
         return;
      }
      DynamicArray[threadID] += 1;
      requestServeDynamic(fd, filename, cgiargs, timeOfArrival, dispatch,
                          DynamicArray, StaticArray, OverallArray, threadID);
   }

   // we now handle the skip:
   if (skip){
       struct timeval timeOfHandlingSkip;
       gettimeofday(&timeOfHandlingSkip, NULL);

       requestHandle(latestFD, latestArrivalTime, timeOfHandling,
                     DynamicArray, StaticArray, OverallArray, threadID,
                     queue, activeThreadCount);
       Close(latestFD);

       pthread_mutex_lock(&lock);
       // the request has been handled,
       // decrement the active thread count to balance back the total number of requests in the queue
       (*activeThreadCount)--;
       pthread_mutex_unlock(&lock);

       // signal that the buffer is available
       pthread_cond_signal(&conditionBufferAvailable);
   }
}