#include "Queue.h"


struct node{
    int m_data;
    struct timeval m_time_of_arrival;
    struct timeval m_dispatch;
    Node m_next;
};


struct queue{
    Node m_head;
    Node m_tail;
    int m_current_size;
    int m_max_size;
};



