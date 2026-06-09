#include "w7-common.h"

#define ELECTORS 7
#define WELCOME_MSG "Welcome, elector!"

static char* ELECTOR_PRIN[] = {"Moguncja","Trewir","Kolonia","Czechy","Pallatynat","Saksonia","Brandenburgia"};
typedef struct {
    int fd;
    int vote;
} Elector;
void usage(char* name)
{
    fprintf(stderr, "USAGE: %s port\n", name);
    exit(EXIT_FAILURE);
}
void add_to_epoll(int epoll_fd, int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
        ERR("epoll_ctl");
}
void new_client(int server_fd, int epoll_fd)
{
    int client_fd = add_new_client(server_fd);
    add_to_epoll(epoll_fd, client_fd);

    if (bulk_write(client_fd, WELCOME_MSG, sizeof(WELCOME_MSG)) < 0)
    {
        if (errno == EPIPE)
        {
            close(client_fd);
        }
        ERR("bulk_write");
    }
}
void old_client(int client_fd,Elector electors[])
{
    char c;
    int ret = bulk_read(client_fd, &c, 1);
    if (ret < 0)
    {
        ERR("bulk_read");
    }
    else if (ret == 0)
    {
        close(client_fd);
    }
    int num = c-'0';
    if(c<1||c>ELECTORS)
    {
     close(client_fd);   
    }

    int elector_idx = num -1;
    if(electors[elector_idx].fd==-1)
    {
        electors[elector_idx].fd = client_fd;
        write_msg(client_fd,WELCOME_MSG);
        write_msg(client_fd,ELECTOR_PRIN[elector_idx]);
    }
    putchar(c);
    putchar('\n');
}
void server(uint16_t port,Elector electors[])
{
    int server_fd = bind_tcp_socket(port, ELECTORS);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        ERR("epoll_create1");

    add_to_epoll(epoll_fd, server_fd);

    struct epoll_event event;
    while (1)
    {
        if (epoll_wait(epoll_fd, &event, 1, -1) < 0)
            ERR("epoll_wait");

        if (event.data.fd == server_fd)
        {
            new_client(server_fd, epoll_fd);
        }
        else
        {
            old_client(event.data.fd);
        }
    }

    close(server_fd);
}

int main(int argc, char** argv)
{
    if (argc != 2)
        usage(argv[0]);

    uint16_t port = atoi(argv[1]);
    if (port <= 0)
        usage(argv[0]);
    Elector electors[ELECTORS];

    for(int i=0; i<ELECTORS; i++){
        electors[i].fd = -1;
    }
    server(port,electors);

    return 0;
}
