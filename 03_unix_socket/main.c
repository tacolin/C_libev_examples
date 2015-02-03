#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <ev.h>

////////////////////////////////////////////////////////////////////////////////

#define SOCK_PATH "unix_socket_server"
#define BUFFER_SIZE 100
#define MAX_SERVER_LISTEN_NUM 10

////////////////////////////////////////////////////////////////////////////////

static void _printUsage(void);
static int  _runServer(void);
static int  _runClient(void);

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        _printUsage();
        return -1;
    }

    if (strcasecmp(argv[1],"c") == 0) return _runClient();
    else if (strcasecmp(argv[1],"s") == 0) return _runServer();

    _printUsage();
    return -1;
}

////////////////////////////////////////////////////////////////////////////////

static void _printUsage(void)
{
    printf("\n");
    printf("\t./main s : run server mode\n");
    printf("\t./main c : run client mode\n");
    printf("\n");
}

////////////////////////////////////////////////////////////////////////////////

static int _runClient(void)
{
    printf("client start ...\n");

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket failed");
        return -1;
    }

    struct sockaddr_un remote = {.sun_family = AF_UNIX};
    int len = strlen(SOCK_PATH) + sizeof(remote.sun_family);

    strcpy(remote.sun_path, SOCK_PATH);

    if (connect(fd, (struct sockaddr*)&remote, len) < 0) {
        perror("connect failed");
        close(fd);
        return -1;
    }

    int  sendlen;
    char buf[BUFFER_SIZE] = {0};
    int  i;
    for(i=0; i<1000; i++) {
        sprintf(buf, "data : %d", i);
        sendlen = send(fd, buf, strlen(buf)+1, 0);
        if (sendlen <= 0) {
            perror("send failed");
            close(fd);
            return -1;
        }
        usleep(1);
    }

    close(fd);
    printf("client end ...\n");
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void _recvCallback(struct ev_loop* loop, ev_io *w, int revents)
{
    int  recvlen          = 0;
    char buf[BUFFER_SIZE] = {0};

    recvlen = recv(w->fd, buf, BUFFER_SIZE, 0);
    if (recvlen <= 0) {
        perror("recv failed");
        printf("recvlen = %d\n", recvlen);

        //////////////////////////////////////
        // recvlen < 0 : failed
        // recvlen = 0 : connection close
        // stop the ev io, close the recv fd
        // do forget to free this ev io
        //////////////////////////////////////
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }

    printf("recv : %s, recvlen = %d\n", buf, recvlen);
    return;
}

static void _acceptCallback(struct ev_loop* loop, ev_io *w, int revents)
{
    struct sockaddr_un remote = {};
    int len = sizeof(remote);
    int newfd;

    newfd = accept(w->fd, (struct sockaddr*)&remote, &len);
    if (newfd < 0) {
        perror("accept failed");
        ev_io_stop(loop, w);
        ev_break(loop, EVBREAK_ALL);
        return;
    }

    printf("accept ok\n");

    ///////////////////////////////////////////////////////
    // malloc a new ev io for recv data
    // it will be free when recv fail or connection closed
    ///////////////////////////////////////////////////////
    ev_io *recv_io = (ev_io*)calloc(sizeof(ev_io), 1);
    ev_io_init(recv_io, _recvCallback, newfd, EV_READ);
    ev_io_start(loop, recv_io);

    return;
}

static void _sigIntCallback(struct ev_loop* loop, ev_signal *w, int revents)
{
    printf("SIGINT happens, break the libev ev loop\n");
    ev_signal_stop(loop, w);
    ev_break(loop, EVBREAK_ALL);
}

static int _runServer(void)
{
    printf("server start ...\n");

    unlink(SOCK_PATH);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket failed");
        return -1;
    }

    struct sockaddr_un local = {.sun_family = AF_UNIX};
    int len = strlen(SOCK_PATH) + sizeof(local.sun_family);

    strcpy(local.sun_path, SOCK_PATH);

    if (bind(fd, (struct sockaddr*)&local, len) < 0) {
        perror("bind failed");
        close(fd);
        return -1;
    }

    if (listen(fd, MAX_SERVER_LISTEN_NUM) < 0) {
        perror("listen failed");
        close(fd);
        return -1;
    }

    struct ev_loop *loop = EV_DEFAULT;

    ev_io accept_io;
    ev_io_init(&accept_io, _acceptCallback, fd, EV_READ);
    ev_io_start(loop, &accept_io);

    ev_signal sig;
    ev_signal_init(&sig, _sigIntCallback, SIGINT);
    ev_signal_start(loop, &sig);

    ev_run(loop, 0);
    ev_loop_destroy(loop);

    unlink(SOCK_PATH);
    close(fd);

    printf("server end ...\n");
    return 0;
}
