#include "w7-common.h"

#define ELECTORS 7
#define WELCOME_MSG "Welcome, elector!"

#define GREETING_MESSAGE 128

#define MAX_MSG 8

static char* ELECTOR_NAME[] = {"Moguncja","Trewir","Kolonia","Czechy","Pallatynat","Saksonia","Brandenburgia"};

typedef struct {
    int is_logged_in;
    int fd_elektora;
    int vote;
} elector_t;

typedef struct {
    int client_fd;
    struct sockaddr_in addr;
    int votes[3];

    pthread_mutex_t votes_mtx;
}worker_arg_t;

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s port_server_tcp port_client_udp\n", name);
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

int isLogged_in(int client_fd, elector_t*electors){
    for(int i=0; i<ELECTORS; i++){
        if(electors[i].fd_elektora==client_fd && electors[i].is_logged_in==1)
        return i;
    }
    return -1;
}
int czy_proba_wlamania(elector_t*electors, int elector_number){
    if(electors[elector_number-1].is_logged_in==1)
    return -1; // proba wlamania bo juz zalogowany
    return 1; //jeszcze nie zalogowany
}
void dissconect_loged_in_elector(elector_t*electors, int elector_index){
    electors[elector_index].is_logged_in = -1;
    electors[elector_index].fd_elektora = -1;
}
void zaloguj(elector_t*electors, int elector_number,int client_fd,int epoll_fd){
    electors[elector_number-1].fd_elektora = client_fd;
    electors[elector_number-1].is_logged_in = 1;

    char greeting_message[GREETING_MESSAGE];

    snprintf(greeting_message,GREETING_MESSAGE,"Welcome, elector of %s!\n",ELECTOR_NAME[elector_number-1]);
    ssize_t bytes_sent = bulk_write(client_fd,greeting_message,strlen(greeting_message));
    if(bytes_sent<0){
        if(errno == EPIPE){
            // zamknal sie chuj wiec go wylaczam
            dissconect_loged_in_elector(electors,elector_number-1);
            close_client(epoll_fd,client_fd);
        }
        else {
            ERR("write");
        }
    }
}


void handle_client(int client_fd, int epoll_fd,elector_t*electors,worker_arg_t* arg){


    int elector_index= isLogged_in(client_fd,electors);
    if(elector_index<0){
        // jesli mam niezalgoowanego
        char c;
        ssize_t read_bytes = read(client_fd,&c,sizeof(c)); // wczytuje jeden znak
        if(read_bytes==0){
            close_client(epoll_fd,client_fd); 
            printf("DEBUG: CLOSED 110\n");
            return;
        }
        else if(read_bytes<0){
        ERR("read");
        }
        int elector_number = c - '0';
        if(elector_number>=1&&elector_number<=7){
            // teraz sprawdzam czy juz jest taki
            if(czy_proba_wlamania(electors,elector_number)<0){
                close_client(epoll_fd,client_fd);
                return;
            }
            else {
                //poprawne lgoowanie 
                zaloguj(electors,elector_number,client_fd,epoll_fd);
                return;

            }
        }
        else {
            //podal zły znak podzcas logowania to go zamykam po prostu 
            close_client(epoll_fd,client_fd);
            return;
        }
    }
    else {
        // mam zalogowanego elektora i teraz on spami glosowaniem 
        // tutaj potencjalnie rozlaczam  ale moze tez sie chuj rozlaczyc
        char c;
        ssize_t read_bytes = read(client_fd,&c,1);
        if(read_bytes==0){
            dissconect_loged_in_elector(electors,elector_index);
            close_client(epoll_fd,client_fd);
            return;
        }
        else if(read_bytes<0){
            ERR("read");
        }

        int vote = c - '0';
        if(vote>=1 && vote<=3){
            if(electors[elector_index].vote==vote){
                //nic do zmiany ten sam glos
            }
            else if(electors[elector_index].vote==-1){
                //nowy głos na dana osobe
                printf("NOWY GLOS\n");
                pthread_mutex_lock(&arg->votes_mtx);
                arg->votes[vote-1]++;
                pthread_mutex_unlock(&arg->votes_mtx);

               
            }
            else{
                printf("zdymanie glosy\n");
                pthread_mutex_lock(&arg->votes_mtx);
                arg->votes[electors[elector_index].vote-1]--; //przerzucam poprzedni glos na nowy
                arg->votes[vote-1]++;
                pthread_mutex_unlock(&arg->votes_mtx);
            }
             electors[elector_index].vote = vote;
        }
        else {
            // skipuje to w pizdu
        }

    }
    
    
}

void doServer(int server_fd,elector_t*electors,worker_arg_t*arg){
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
                int client_fd = add_new_client(server_fd);
                if(client_fd<0)
                ERR("client_fd");

                add_client_to_epoll(epoll_fd,client_fd); // doddaje tego klienta do epolla

            }
            else{
                // teraz validuje co on mi dał i jednoczesnie nie blokuje tzn mam wiadomosc od klienta teraz (np powitalny numer)

                int client_fd = events[i].data.fd;
                handle_client(client_fd,epoll_fd,electors,arg);               
                
            }
        }
    }

    if(close(epoll_fd)<0)
    ERR("close:epoll_fd"); // na razie unreachable ale pozniej moze sie przyda

}



void *thread_work(void*arg_t){
    worker_arg_t* args = (worker_arg_t*)(arg_t);

    char message[1024];
    while(1){
    pthread_mutex_lock(&args->votes_mtx);
    snprintf(message,1024,"-----VOTES------\nFranciszek I: %d\nKarol V: %d\nHenryk VII: %d\n",args->votes[0],args->votes[1],args->votes[2]);
    pthread_mutex_unlock(&args->votes_mtx);

    ssize_t bytes_sent = sendto(args->client_fd,message,strlen(message),0,(struct sockaddr*)(&args->addr),sizeof(args->addr));
    if(bytes_sent<0)
    ERR("sendto");
    ms_sleep(2000);

    }
  

    return NULL;
}

int main(int argc, char** argv)
{
    signal(SIGPIPE,SIG_IGN);

    if (argc != 3)
        usage(argv[0]);

    
    int client_udp = socket(AF_INET,SOCK_DGRAM,0);
    if(client_udp<0)
    ERR("socket");


    pthread_t thread;

    worker_arg_t arg;
    arg.addr = make_address("localhost",argv[2]);
    arg.client_fd = client_udp;
    if(pthread_mutex_init(&arg.votes_mtx,NULL)!=0)
    ERR("pthread_mutex_init");

    for(int i=0; i<3; i++){
        arg.votes[i]=0;
    }
    if(pthread_create(&thread,NULL,thread_work,&arg)!=0)
    ERR("pthread_create");
    


    printf("Czekam na połączenie na porcie: %d\n",atoi(argv[1]));

    int server_fd = bind_tcp_socket((uint16_t)(atoi(argv[1])),ELECTORS);

    elector_t electors[ELECTORS];
    for(int i=0; i<ELECTORS; i++){
        electors[i].fd_elektora = -1;
        electors[i].is_logged_in = -1;
        electors[i].vote = -1;
    }

    doServer(server_fd,electors,&arg);


    if(close(server_fd)<0)
    ERR("close:server_fd");

    pthread_join(thread,NULL);
    if(pthread_mutex_destroy(&arg.votes_mtx)!=0)
    ERR("pthread_mutex_destroy");

    printf("Server has terminated\n");
    return 0;
}
