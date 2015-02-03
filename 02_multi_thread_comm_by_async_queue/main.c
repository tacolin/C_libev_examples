#include "basic.h"
#include "queue.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct myasync
{
    ev_async ev;
    struct ev_loop* loop;
    myqueue queue;

} myasync;

////////////////////////////////////////////////////////////////////////////////

static myqueue _queue;

////////////////////////////////////////////////////////////////////////////////

static void _sigIntCallback(struct ev_loop* loop, ev_signal* w, int revents)
{
    dprint("SIGINT happens, break the libev ev loop");
    ev_signal_stop(loop, w);
    ev_break(loop, EVBREAK_ALL);
}

static void _asyncCallback(struct ev_loop* loop, ev_async* w, int revents)
{
    char* data = NULL;
    myasync* my = (myasync*)w;

    while (data = (char*)myqueue_get(&(my->queue)))
    {
        dprint("data : %s", data);
        free(data);
        data = NULL;
    }
    return;
}

static void* _clientRoutine(void* arg)
{
    myasync *my = (myasync*)arg;

    dprint("enter, sleep 2 sec...");
    sleep(2);
    dprint("wake up");

    char* data;
    int i;
    for (i=0; i<1000; i++)
    {
        asprintf(&data, "data, count = %d", i);
        myqueue_put(&(my->queue), data);
        if (ev_async_pending(&(my->ev)) == 0)
        {
            ev_async_send(my->loop, &(my->ev));
            dprint("send i = %d", i);
        }
    }

    sleep(1);
    dprint("over");

    pthread_exit(0);
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    struct ev_loop* loop = EV_DEFAULT;

    myasync my;
    ev_async_init(&(my.ev), _asyncCallback);
    my.loop = loop;
    myqueue_init(&(my.queue), 500, NULL, QUEUE_PUT_SUSPEND, QUEUE_GET_UNSUSPEND);
    ev_async_start(loop, &(my.ev));

    ev_signal sig;
    ev_signal_init(&sig, _sigIntCallback, SIGINT);
    ev_signal_start(loop, &sig);

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, _clientRoutine, &my);

    ev_run(loop, 0);
    ev_loop_destroy(loop);

    pthread_join(client_thread, NULL);
    myqueue_clean(&(my.queue));

    dprint("test over");
    return 0;
}

