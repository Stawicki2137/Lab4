#include "common_l8.h"
#include <arpa/inet.h>

volatile sig_atomic_t run = 1;

void sigint_handler(int signo){
    run = 0;
}

int main(int argc, char**argv){
    printf("Server has started\n");

    if(sethandler(sigint_handler,SIGINT)<0)
    ERR("sethandler");

    int fd_serv = socket(AF_INET,SOCK_DGRAM,0);
    if(fd_serv<0)
    ERR("socket");
    struct sockaddr_in serv_addr = make_address("localhost","2137");

    if(bind(fd_serv,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    ERR("bind");

   
    while(1){
        if(run==0)
        break;

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char buf[100];

        ssize_t received = recvfrom(fd_serv,buf,sizeof(buf)-1,0,(struct sockaddr*)&client_addr,&client_len);
        if(received<0)
        {
            if(errno==EINTR){
                if(run==0)
                break;
            }
            else{
             ERR("recvfrom");

            }
        }

        buf[received]='\0';

        char client_addr_text[INET_ADDRSTRLEN];

        if(inet_ntop(AF_INET,&client_addr.sin_addr,client_addr_text,sizeof(client_addr_text)) == NULL)
        ERR("inet_ntop");

        int client_port = ntohs(client_addr.sin_port);

        printf("[%s:%d]: %s \n",client_addr_text,client_port,buf);
    }

    if(close(fd_serv)<0)
    ERR("close");

    printf("Server has terminated\n");
    return EXIT_SUCCESS;
}