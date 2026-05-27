#include "common_l8.h"

#define ODDZIAL_LEN 128
#define STACK_SIZE 16
#define MSG_SIZE 256
#define ADIUTANTS 4

typedef struct {
    char data[MSG_SIZE];
    ssize_t len;
}report_t;

typedef struct{
    report_t reports[STACK_SIZE];
    int top; // liczba elementow na stosie i od razu index nastepnego wolnego miejsca 

    pthread_mutex_t mutex;
    sem_t items_sem; // liczba elementow na stosie
    sem_t slots_sem; // liczba wolnych miejsc na stosie 

}report_stack_t;

/*
sem_wait(sem):
    jeśli sem > 0:
        sem--
        idziesz dalej
    jeśli sem == 0:
        wątek zasypia i czeka

sem_post(sem):
    sem++
    jeśli ktoś czekał na tym semaforze, to jeden wątek może zostać obudzony
*/
void stack_push(report_stack_t* stack,const char*data,ssize_t size){
    //jesli stos jest pelen to sem_t slots ma 0
    if(size>=MSG_SIZE)
        size = MSG_SIZE - 1;

    while (sem_wait(&stack->slots_sem) == -1) {
        if (errno != EINTR)
            ERR("sem_wait slots");
    }

    pthread_mutex_lock(&stack->mutex);

    memcpy(stack->reports[stack->top].data,data,size);
    stack->reports[stack->top].data[size]='\0';
    stack->reports[stack->top].len = size;
    stack->top++;

    pthread_mutex_unlock(&stack->mutex);

    sem_post(&stack->items_sem);
}
void stack_pop(report_stack_t* stack,report_t* out_report){

    while (sem_wait(&stack->items_sem) == -1) {
        if (errno != EINTR)
            ERR("sem_wait items");
    }

    pthread_mutex_lock(&stack->mutex);
    
    stack->top--;
    *out_report = stack->reports[stack->top];

    pthread_mutex_unlock(&stack->mutex);

    sem_post(&stack->slots_sem);
}

void stack_init(report_stack_t*stack){
    stack->top = 0;

    if(pthread_mutex_init(&stack->mutex,NULL)!=0)
    ERR("pthread_mutex_init");

    if(sem_init(&stack->items_sem,0,0)!=0)
    ERR("sem_init: items");

    if(sem_init(&stack->slots_sem,0,STACK_SIZE)!=0)
    ERR("sem_init: items");

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
void * adiutant_works(void* arg){
    report_stack_t* stack = arg;

    while(1){
        report_t report;
        stack_pop(stack,&report);

        int x = -1;
        int y = -1;
        int p = -1;
        char oddzial_name[ODDZIAL_LEN+1];

        memset(oddzial_name,0,sizeof(oddzial_name));

        int ret_sscanf = sscanf(report.data,"%d %d %d %128s",&x,&y,&p,oddzial_name);
        if (ret_sscanf != 4) {
            printf("Zły format danych: <X> <Y> <P> <nazwa oddziału>\n");
            continue;
        }

        if (validate(x) || validate(y) || (p != 0 && p != 1)) {
            printf("Zły format danych: <X> <Y> <P> <nazwa oddziału>\n");
            continue;
        }

        print_oddzial(x, y, p, oddzial_name);

    }
    return NULL;
}
void doServer(int server_fd){

    char recvbuff[MSG_SIZE];
    report_stack_t stack;
    stack_init(&stack);

    pthread_t threads[ADIUTANTS];

    for(int i=0; i<ADIUTANTS; i++){
        if(pthread_create(&threads[i],NULL,adiutant_works,&stack)!=0)
        ERR("pthread_create");
    }

    while(1){
       
        memset(recvbuff,0,MSG_SIZE);
        ssize_t received;
        received=TEMP_FAILURE_RETRY(recv(server_fd,recvbuff,MSG_SIZE-1,0));
        if(received<0)
        ERR("recv");

        // tutaj wrzucam na stos 
        recvbuff[received]='\0';
        stack_push(&stack,recvbuff,received);
    }

    for(int i=0; i<ADIUTANTS; i++){
        pthread_join(threads[i],NULL);
    }
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