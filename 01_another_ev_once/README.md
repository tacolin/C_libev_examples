# Anothter Implementation of Disposable ev_once()

Although libev has the API ev_once(...), it supports only ev_io and ev_timer.

When you trigger a lot of ev_once(loop, -1, EV_TIMER, 0.0, callback, argument) at the same time, the callback function wiil not be executed in order.

I use the ev_async to implement my "ev_once" for some logical purpose, like state machine events.
