#include "common_l8.h"

int main(int argc, char**argv){
    
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0)
    ERR("socket");

    char buf[100];
    struct sockaddr_in addr = make_address("localhost","2137");

    for(int i=0; i<10; i++){
        /*strcpy(buf,"Chuj");
        snprintf(buf+4,sizeof(buf)-strlen(buf),"%d\n",i); */
        snprintf(buf,sizeof(buf),"Chuj_%d\n",i);
        ssize_t sent = sendto(sock_fd,buf,strlen(buf),0, (struct sockaddr*) &addr,sizeof(addr));
        if(sent<0)
        ERR("sendto");
    }

    if(close(sock_fd)<0)
    ERR("close");

    printf("Client has terminated\n");

    return EXIT_SUCCESS;
}