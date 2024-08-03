#include "Queue.h"


/** NODE */
struct node{
    int m_fd;
    struct timeval m_timeOfArrival;
    Node m_next;
    Node m_previous;
};
/** Node implementation */

Node createNode(int fd, struct timeval time_of_arrival){
    Node newNode = (Node)malloc(sizeof(*newNode));
    newNode->m_fd = fd;
    newNode->m_timeOfArrival = time_of_arrival;
    newNode->m_next = NULL;
    newNode->m_previous = NULL;

    return newNode;
}

/** QUEUE */
struct queue{
    Node m_head;
    Node m_tail;
    int m_current_size;
};

/** Constructors and Destructors **/
Queue queueConstructor(){
    Queue newQueue = (Queue)malloc(sizeof(*newQueue));
    newQueue->m_head = NULL;
    newQueue->m_tail = NULL;
    newQueue->m_current_size = INITIAL_QUEUE_SIZE;

    return newQueue;
}

void queueDestructor(Queue queue){
    Node current = queue->m_head;
    while(current != NULL){
        Node next = current->m_next;
        free(current);
        current = next;
    }
    free(queue);
}


/** Queue implementation **/

// adds a new node to the end of the queue
void enqueue(Queue queue, int fd, struct timeval time_of_arrival){
    if (full(queue)){
        return;
    }
    Node newNode = createNode(fd, time_of_arrival);
    if (empty(queue)){
        // both head and tail should point to the new node:
        queue->m_head = newNode;
        queue->m_tail = newNode;
    }
    else {
        // add the new node after the current tail node

        queue->m_tail->m_next = newNode;
        newNode->m_previous = queue->m_tail;

        // assign the new node to be the tail node
        queue->m_tail = newNode;
    }
    queue->m_current_size++;
}

// returns the file descriptor of the head of the queue.
// make sure not to lose the arrival time of the head
// because the head will be deleted after this function is called
int dequeue(Queue queue){
    if (empty(queue)){
        return EMPTY_QUEUE;
    }
    // save the head node in order to free it later
    Node originalHead = queue->m_head;

    // move the head to the next node
    queue->m_head = queue->m_head->m_next;

    // need to update the previous pointer of the new head to be NULL
    if (queue->m_head != NULL){
        queue->m_head->m_previous = NULL;
    }

    queue->m_current_size--;

    if (queue->m_current_size == 0 || queue->m_current_size == 1){
        queue->m_tail = queue->m_head;
    }

    int fd = originalHead->m_fd;
    free(originalHead);

    return fd;
}


// this function assumes that the number in line is valid
// and is between 0 and the size of the queue - 1
// the function returns the file descriptor of the node

// notice that we're dropping the requests and therefore don't need the arrival time
// of the node we're deleting
int dequeueByNumberInLine(Queue queue, int numberInLine){
    if (empty(queue)){
        return EMPTY_QUEUE;
    }
    if (numberInLine >= queue->m_current_size || numberInLine < 0){
        return OUT_OF_QUEUE_BOUNDS;
    }

    Node current = queue->m_head;
    Node previous = NULL;

    for (int i = 0; i < numberInLine; i++){
        previous = current;
        current = current->m_next;
    }

    // if the node we want to delete is first in line
    if (numberInLine == 0){
        Node newHead = current->m_next;
        queue->m_head = newHead;
        if (newHead != NULL){
            newHead->m_previous = NULL;
        }
    }
    else { // if the node we want to delete is not first in line, i.e numberInLine > 0
        previous->m_next = current->m_next;
        if(current->m_next != NULL){
            current->m_next->m_previous = previous;
        }
    }

    int fd = current->m_fd;
    free(current);

    queue->m_current_size--;

    if (queue->m_current_size == 0 || queue->m_current_size == 1){
        queue->m_tail = queue->m_head;
    }

    return fd;
}

// This function assumes that the queue is not empty:
struct timeval getHeadsArrivalTime(Queue queue){
    return queue->m_head->m_timeOfArrival;
}

struct timeval getTailsArrivalTime(Queue queue){
    if (empty(queue)){
        struct timeval emptyTime;
        emptyTime.tv_sec = -1;
        emptyTime.tv_usec = -1;
        return emptyTime;
    }
    return queue->m_tail->m_timeOfArrival;
}

int getQueueSize(Queue queue){
    return queue->m_current_size;
}

bool empty(Queue queue){
    return queue->m_current_size == 0;
}

int dequeueLatest(Queue queue){
    if (empty(queue)){
        return EMPTY_QUEUE;
    }
    Node originalTail = queue->m_tail;
    Node newTail = originalTail->m_previous;

    if (newTail == NULL){
        // if the queue has only one node
        queue->m_head = NULL;
        queue->m_tail = NULL;
    }
    else {
        newTail->m_next = NULL;
        queue->m_tail = newTail;
    }

    int fd = originalTail->m_fd;
    free(originalTail);

    queue->m_current_size--;

    return fd;
}