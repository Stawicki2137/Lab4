#include "w7-common.h"

#define ELECTORS 7
#define WELCOME_MSG "Welcome, elector!"

#define MAX_MSG 8

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s port\n", name);
    exit(EXIT_FAILURE);
}

void add_client_to_epoll(int epoll_fd, int client_fd){

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&event)<0)
    ERR("epoll_ctl");
}
void close_client(int epoll_fd,int client_fd){

    fprintf(stdout,"Client  fd=%d disconected\n",client_fd);

    if(epoll_ctl(epoll_fd,EPOLL_CTL_DEL,client_fd,NULL)<0)
    ERR("epoll_ctl");

    if(close(client_fd)<0)
    ERR("close:client_fd");

}
void send_message_to_client(int epoll_fd,int client_fd){

    ssize_t sent_bytes =  bulk_write(client_fd,WELCOME_MSG,strlen(WELCOME_MSG));
    if(sent_bytes<0){
        if(errno==EPIPE){
            //klient sie rozlaczyl
            close_client(epoll_fd,client_fd);
            return;
        }
        else {
            ERR("bulk_write");
        }

    }
}

void receive_message_from_client(int epoll_fd,int client_fd){

    char buff[MAX_MSG+1];

    ssize_t read_bytes = read(client_fd,buff,MAX_MSG);

    if(read_bytes<0)
    {
        if(errno==EINTR)
        return;

        ERR("read");
    }

    if(read_bytes==0){
        close_client(epoll_fd,client_fd);
        return;
    }

    buff[read_bytes] = '\0';
    printf("Message from clientd_fd: %d message: %s\n",client_fd,buff);

}

void doServer(int server_fd){
    int epoll_fd = epoll_create1(0);
    if(epoll_fd<0)
    ERR("epoll_create1");

    struct epoll_event event, events[ELECTORS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&event)<0)
    ERR("epoll_ctl");


    while(1){
        int n = epoll_wait(epoll_fd,events,ELECTORS,-1);

        if(n<0){
            if(errno==EINTR)
            continue;
            ERR("epoll_wait");
        }

        for(int i=0;i<n;i++){
            if(events[i].data.fd==server_fd){
                // jest klient do accepta do obslugi
                int client_fd = add_new_client(server_fd);
                if(client_fd<0)
                ERR("client_fd");

                add_client_to_epoll(epoll_fd,client_fd);

                send_message_to_client(epoll_fd,client_fd);


            }
            else{
                // w innym wypadku to jakis klient sie zglosil ze jest cos do odczytu u niego albo sie rozlaczyl

                receive_message_from_client(epoll_fd,events[i].data.fd);
                
            }
        }
    }

    if(close(epoll_fd)<0)
    ERR("close:epoll_fd"); // na razie unreachable ale pozniej moze sie przyda

}
int main(int argc, char** argv)
{
    signal(SIGPIPE,SIG_IGN);

    if (argc != 2)
        usage(argv[0]);

    printf("Czekam na połączenie na porcie: %d\n",atoi(argv[1]));

    int server_fd = bind_tcp_socket((uint16_t)(atoi(argv[1])),ELECTORS);

    doServer(server_fd);

    if(close(server_fd)<0)
    ERR("close:server_fd");

    return 0;
}
