#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "basic.h"
#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////

#define QUEUE_OK   OK
#define QUEUE_FAIL FAIL

#define QUEUE_PUT_SUSPEND   TRUE
#define QUEUE_PUT_UNSUSPEND FALSE

#define QUEUE_GET_SUSPEND   TRUE
#define QUEUE_GET_UNSUSPEND FALSE

////////////////////////////////////////////////////////////////////////////////

typedef struct myqueue_obj
{
    void* content;
    struct myqueue_obj* next_obj;

} myqueue_obj;

typedef void (*myqueue_obj_cleanfn)(void* obj_content);

typedef struct myqueue
{
    int max_obj_num;
    int curr_obj_num;

    myqueue_obj *head;
    myqueue_obj *tail;

    sem_t obj_sem;
    sem_t empty_sem;

    int is_put_suspend;
    int is_get_suspend;

    pthread_mutex_t lock;

    myqueue_obj_cleanfn cleanfn;

} myqueue;

////////////////////////////////////////////////////////////////////////////////

int myqueue_init(myqueue *queue, int max_queue_depth,
                 myqueue_obj_cleanfn clean_func,
                 int is_put_suspend, int is_get_suspend);

void myqueue_clean(myqueue *queue);

int   myqueue_put(myqueue* queue, void* content);
void* myqueue_get(myqueue* queue);

#endif //_QUEUE_H_
