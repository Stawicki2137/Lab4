#include "common_l8.h"

#define ODDZIAL_LEN 128

int make_socket(int domain, int type)
{
    int sock;
    sock = socket(domain, type, 0);
    if (sock < 0)
        ERR("socket");
    return sock;
}

int bind_inet_socket(uint16_t port, int type)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;
    socketfd = make_socket(PF_INET, type);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");

    return socketfd;
}

void usage(char* name){
    printf("USAGE: %s port\n",name);
    exit(EXIT_FAILURE);
}

int validate(int x){
    if(x<0||x>99)
    return -1;
    return 0;
}
void print_oddzial(int x, int y, int p, char*oddzial_name){
    if(p==1){
        printf("Nasz oddział %s był widziany na pozycji (%d,%d)\n",oddzial_name,x,y);
    }
    else if(p==0){
        printf("Wrogi oddział %s był widziany na pozycji (%d,%d)\n",oddzial_name,x,y);
    }
}
void doServer(int server_fd){

    ssize_t buff_size = ODDZIAL_LEN + sizeof(int)*3;
    char*recvbuff = malloc(sizeof(char)*buff_size);
    if(recvbuff==NULL)
    ERR("malloc");

    while(1){
        int x = -1;
        int y = -1;
        int p = -1;

        memset(recvbuff,0,buff_size);
        ssize_t received;
        received=TEMP_FAILURE_RETRY(recv(server_fd,recvbuff,buff_size,0));
        if(received<0)
        ERR("recv");

        char oddzial_name[ODDZIAL_LEN+1];
        int ret = sscanf(recvbuff,"%d %d %d %s",&x,&y,&p,oddzial_name);
        if(ret!=4)
        {
            printf("Zły format danych: <X> <Y> <P> <nazwa oddziału> \n");
            continue;
        }
        if(validate(x) || validate(y) || (p!=0 && p!=1))
        {
            printf("Zły format danych: <X> <Y> <P> <nazwa oddziału>\n(x,y) 0-99 p 0 lub 1\n");
            continue;
        }
        
        print_oddzial(x,y,p,oddzial_name);

    }
    free(recvbuff);
}

int main(int argc, char**argv){
    if(argc!=2){
        usage(argv[0]);
    }
    printf("Server has started\n");

    int server_fd = bind_inet_socket((uint16_t)atoi(argv[1]),SOCK_DGRAM);

    doServer(server_fd);

    if(TEMP_FAILURE_RETRY(close(server_fd))<0)
    ERR("close");

    printf("Server has terminated\n");
    return EXIT_SUCCESS;
}