#include <stdio.h>
#include <stdlib.h>
#include <ev.h>

static void _sigIntCb(struct ev_loop* loop, ev_signal* w, int revents)
{
    perror("SIGINT happens, break the libev ev loop");
    ev_signal_stop(loop, w);
    ev_break(loop, EVBREAK_ALL);
}

static void _disposableCb(struct ev_loop* loop, ev_async* w, int revents)
{
    long event_data = (long)(w->data);
    printf("disposable callback, event_data = %ld\n", event_data);

    /////////////////////////////
    // stop the event from loop
    // don't forget to free it
    /////////////////////////////
    ev_async_stop(loop, w);
    free(w);
    return;
}

static void _generateDisposableEvents(int revents, void* arg)
{
    struct ev_loop* loop = (struct ev_loop*)arg;

    printf("timeout, generate 1000 more events\n");

    long i;
    ev_async* disposable = NULL;
    for (i=0; i<1000; i++)
    {
        //////////////////////////////////////////////////////////////
        // NOTICE:
        // if you use timer event with time period 0.0 for disposable,
        // like : ev_once(loop, -1, EV_TIMER, 0.0, callbackf, arg)
        //
        // the callback executions will not be in order ...
        //
        // so we choose ev_async to implement once disposable event.
        //////////////////////////////////////////////////////////////

        /////////////////////////////////////////////
        // allocate new memory for disposable event.
        /////////////////////////////////////////////
        disposable = calloc(sizeof(ev_async), 1);

        ev_async_init(disposable, _disposableCb);
        ev_async_start(loop, disposable);
        disposable->data = (void*)i;
        ev_async_send(loop, disposable);
    }
}

int main(int argc, char const *argv[])
{
    struct ev_loop* loop = EV_DEFAULT;

    //////////////////////////////////////////////////////////////////
    // for safety, you could use CTRL+C to close your program safely.
    //////////////////////////////////////////////////////////////////
    ev_signal sig;
    ev_signal_init(&sig, _sigIntCb, SIGINT);

    //////////////////////////////////////////////////////////////////
    // trigger a timer event.
    // when timeout, it generates more events in callback function
    //////////////////////////////////////////////////////////////////
    ev_once(loop, -1, EV_TIMER, 1.0, _generateDisposableEvents, loop);

    ev_run(loop, 0);
    ev_loop_destroy(loop);

    printf("test over\n");
    return 0;
}
