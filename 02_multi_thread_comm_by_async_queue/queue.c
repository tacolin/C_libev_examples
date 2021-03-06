#include "queue.h"

////////////////////////////////////////////////////////////////////////////////

static void _lock(myqueue* queue)
{
    pthread_mutex_lock(&(queue->lock));
}

static void _unlock(myqueue* queue)
{
    pthread_mutex_unlock(&(queue->lock));
}

////////////////////////////////////////////////////////////////////////////////

int myqueue_init(myqueue *queue, int max_queue_depth,
                 myqueue_obj_cleanfn clean_func,
                 int is_put_suspend, int is_get_suspend)
{
    check_if(queue == NULL, return QUEUE_FAIL, "queue is null");
    check_if(max_queue_depth <= 0, return QUEUE_FAIL,
             "max_queue_depth is %d", max_queue_depth);

    queue->max_obj_num    = max_queue_depth;
    queue->curr_obj_num   = 0;
    queue->head           = NULL;
    queue->tail           = NULL;
    queue->cleanfn        = clean_func;
    queue->is_put_suspend = is_put_suspend;
    queue->is_get_suspend = is_get_suspend;

    sem_init(&(queue->obj_sem), 0, 0);
    sem_init(&(queue->empty_sem), 0, max_queue_depth);

    int chk = pthread_mutex_init(&(queue->lock), NULL);
    check_if(chk != 0, return QUEUE_FAIL, "pthread_mutext_init failed");

    return QUEUE_OK;
}

void myqueue_clean(myqueue *queue)
{
    check_if(queue == NULL, return, "queue is null");

    _lock(queue);

    myqueue_obj *obj      = queue->head;
    myqueue_obj *next_obj = NULL;

    while (obj)
    {
        next_obj = obj->next_obj;

        if (queue->cleanfn)
        {
            queue->cleanfn(obj->content);
        }

        free(obj);

        obj = next_obj;
    }

    queue->curr_obj_num = 0;
    queue->head         = NULL;
    queue->tail         = NULL;

    sem_init(&(queue->obj_sem), 0, 0);
    sem_init(&(queue->empty_sem), 0, queue->max_obj_num);

    _unlock(queue);

    return;
}

int myqueue_put(myqueue* queue, void* content)
{
    check_if(queue == NULL, return QUEUE_FAIL, "queue is null");
    check_if(content == NULL, return QUEUE_FAIL, "content is null");

    if (queue->is_put_suspend == QUEUE_PUT_SUSPEND)
    {
        sem_wait(&(queue->empty_sem));
    }
    else
    {
        if (queue->curr_obj_num >= queue->max_obj_num)
        {
            // queue is full
            return QUEUE_FAIL;
        }
    }

    myqueue_obj *new_obj;
    new_obj           = calloc(sizeof(myqueue_obj), 1);
    new_obj->content  = content;
    new_obj->next_obj = NULL;

    _lock(queue);

    if (queue->tail)
    {
        queue->tail->next_obj = new_obj;
        queue->tail = new_obj;
    }
    else
    {
        queue->tail = new_obj;
        queue->head = new_obj;
    }
    queue->curr_obj_num++;

    if (queue->is_get_suspend == QUEUE_GET_SUSPEND)
    {
        sem_post(&(queue->obj_sem));
    }

    _unlock(queue);

    return QUEUE_OK;
}

void* myqueue_get(myqueue* queue)
{
    check_if(queue == NULL, return NULL, "queue is null");

    if (queue->is_get_suspend == QUEUE_GET_SUSPEND)
    {
        sem_wait(&(queue->obj_sem));
    }
    else
    {
        if (queue->curr_obj_num <= 0)
        {
            // queue is empty
            check_if(queue->head, return NULL,
                     "queue is empty but pHead exists");
            check_if(queue->tail, return NULL,
                     "queue is empty but pTail exists");

            return NULL;
        }
    }

    check_if(queue->head == NULL, return NULL,
             "currentObjNum = %d, but pHead is NULL", queue->curr_obj_num);
    check_if(queue->tail == NULL, return NULL,
             "currentObjNum = %d, but pTail is NULL", queue->curr_obj_num);

    _lock(queue);

    myqueue_obj *obj = queue->head;
    if (obj->next_obj)
    {
        queue->head = obj->next_obj;
    }
    else
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    queue->curr_obj_num--;

    if (queue->is_put_suspend == QUEUE_PUT_SUSPEND)
    {
        sem_post(&(queue->empty_sem));
    }

    _unlock(queue);

    void* content = obj->content;
    free(obj);

    return content;
}

