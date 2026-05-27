#include "l8_common.h"
#define MAXBUF 576

volatile sig_atomic_t last_signal =0;

void sigalrm_handler(int sig) {last_signal=sig;}

void usage(char* name){
    printf("USAGE: %s adres port nazwa_pliku\n",name);
    exit(EXIT_FAILURE);
}

int make_socket(int domain, int type){
    int fd = socket(domain,type,0);
    if(fd<0)
    ERR("socket");

    return fd;
}
void send_and_confirm(int client_fd, char* sendbuff,char*recvbuff,ssize_t size,struct sockaddr_in addr){

    struct itimerval ts;
    if(TEMP_FAILURE_RETRY(sendto(client_fd,sendbuff,size,0,(struct sockaddr*)&addr,sizeof(addr)))<0)
    ERR("sendto");
    memset(&ts,0,sizeof(struct itimerval));
    ts.it_value.tv_usec=500000;
    setitimer(ITIMER_REAL,&ts,NULL);
    last_signal = 0;

    while(recv(client_fd,recvbuff,MAXBUF,0)<0){
        if(errno!=EINTR){
            ERR("recv");
        }
        if(SIGALRM==last_signal)
        break;
    }
    
}
void doClient(int client_fd, struct sockaddr_in addr,int file_fd){
    char sendbuff[MAXBUF];
    char recvbuff[MAXBUF];
    int offset = 2*sizeof(int32_t);
    int32_t chunkNo = 0;
    ssize_t size;
    int counter;

    do {
    memset(sendbuff,0,MAXBUF);
    memset(recvbuff,0,MAXBUF);

    size = bulk_read(file_fd,sendbuff + offset,MAXBUF-offset);
    if(size<0)
    ERR("bulk_read");


    *((int32_t*)sendbuff)=htonl(++chunkNo); // numer chunka ustawiony

    int32_t last = 0;
    if(size<MAXBUF-offset){
        last = 1;
    }
    int32_t net_last = htonl(last);
    memcpy(sendbuff+sizeof(int32_t),&net_last,sizeof(net_last)); // isLast message 

    counter = 0;
    do{
        counter++;
        send_and_confirm(client_fd,sendbuff,recvbuff,size+offset,addr);
    }while((int32_t)htonl(chunkNo)!=*((int32_t*)recvbuff)&&counter<=5);

    if((int32_t)htonl(chunkNo)!=*((int32_t*)recvbuff) && counter>5)
    break; //konce dzialanie bo za duzo prob juz 

    }while(size==MAXBUF-offset);
    
}

int main(int argc, char**argv){

    if(argc!=4){
        usage(argv[0]);
    }

    if(sethandler(sigalrm_handler,SIGALRM)<0)
    ERR("setting SIGALRM");

    int file_fd;
    file_fd=TEMP_FAILURE_RETRY(open(argv[3],O_RDONLY));
    if(file_fd<0)
    ERR("open");

    int client_fd = make_socket(AF_INET,SOCK_DGRAM);

    struct sockaddr_in addr;
    addr = make_address(argv[1],argv[2]); // na ten adres wysylam

    doClient(client_fd,addr,file_fd);

    if(TEMP_FAILURE_RETRY(close(file_fd))<0)
    ERR("close: file_fd");

    if(TEMP_FAILURE_RETRY(close(client_fd))<0)
    ERR("close: client_fd");

    return EXIT_SUCCESS;
}