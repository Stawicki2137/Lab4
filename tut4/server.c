#include "l8_common.h"

#define BACKLOG 3
#define MAXADDR 5
#define MAXBUF 576

typedef struct {
    int free;
    int chunkNo;
    struct sockaddr_in addr;

}connections_t;

void usage (char* name){
    printf("USAGE: %s port\n",name);
    exit(EXIT_FAILURE);
}

int make_socket(int domain, int type){
    int fd = socket(domain,type,0);
    if(fd<0)
    ERR("socket");
    return fd;
}

int bind_inet_socket(uint16_t port,int type){
    struct sockaddr_in addr;
    int socket_fd = make_socket(AF_INET,type);
    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    int t = 1;

    if(setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&t,sizeof(t))<0)
    ERR("setsockopt");
    if(bind(socket_fd,(struct sockaddr*)&addr,sizeof(addr))<0)
    ERR("bind");

    if(SOCK_STREAM==type)
        if(listen(socket_fd,BACKLOG)<0)
        ERR("listen");

    return socket_fd;

}

int find_index(struct sockaddr_in addr,connections_t connections[MAXADDR]){
    int empty = -1, pos = -1;
    for(int i=0; i<MAXADDR; i++){
        if(connections[i].free==1)
        {
            empty = i;
        }
        else if(memcmp(&addr,&(connections[i].addr),/*liczba bajtow do porownania!!*/sizeof(struct sockaddr_in))==0){
            pos = i;
            break;
        }
    }
    if(pos<0 && empty >=0)
    {
        connections[empty].addr = addr;
        connections[empty].free = 0;
        connections[empty].chunkNo = 0;
        pos = empty;
    }
    return pos;
}

void doServer(int fd_server){
    struct sockaddr_in addr;
    connections_t connections[MAXADDR];
    char buff[MAXBUF+1];

    for(int i=0; i<MAXADDR; i++){
        connections[i].free=1;
    }

    // chunkNo, isLast, data 
    while(1){
        socklen_t size = sizeof(addr);
        ssize_t received = TEMP_FAILURE_RETRY(recvfrom(fd_server,buff,MAXBUF,0,(struct sockaddr*)&addr,&size));
        if(received<0)
        ERR("recvfrom");

        buff[received]='\0';

        int index = -1;
        index = find_index(addr,connections);
        if(index<0){
            //jest juz full i wiecej nie mozna wiec  klientow obsluzyc wiec ignoruje
        }
        else 
        {
            int32_t chunkNo =  ntohl(*((int32_t*)(buff)));
            if(chunkNo>connections[index].chunkNo+1){
                continue; // pominelismy jeden (zla kolejnosc datagramow)
            }
            else if(chunkNo==connections[index].chunkNo+1){
                //last message
                /*
                int32_t net_isLast;
                memcpy(&net_isLast, buff + sizeof(int32_t), sizeof(int32_t));
                int32_t isLast = ntohl(net_isLast);
                */
                if(ntohl(*((int32_t*)(buff+sizeof(int32_t))))==1){
                    //lsat message
                    printf("[%d]: Last Part: %d: %s\n",ntohs(connections[index].addr.sin_port),chunkNo,buff+2*sizeof(int32_t));
                    connections[index].free=1;
                }
                else{
                    printf("[%d]: Part: %d: %s\n",ntohs(connections[index].addr.sin_port),chunkNo,buff+2*sizeof(int32_t));
                }
                connections[index].chunkNo++;
            }
            else {
                //przyszedl duplikat 
            }

            if(TEMP_FAILURE_RETRY(sendto(fd_server,buff,received,0,(struct sockaddr*)&addr,sizeof(addr)))<0)
            ERR("sendto");

        }



    }

}

int main(int argc, char**argv){
    if(argc!=2){
        usage(argv[0]);
    }
    int fd_server = bind_inet_socket((uint16_t)atoi(argv[1]),SOCK_DGRAM);

    doServer(fd_server);

    if(TEMP_FAILURE_RETRY(close(fd_server))<0)
    ERR("close");

    fprintf(stderr,"Server has terminated.\n");
    return EXIT_SUCCESS;
}