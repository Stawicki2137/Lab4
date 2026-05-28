#include "l8_common.h"

#define SPELL_TYPES 3
const char* spell_names[SPELL_TYPES] = {"Divination", "Summon Elemental", "Fireball"};
#define BOARD_SIZE 8
#define BACKLOG 16

#define MAX_QUEUE 10
#define THREAD_COUNT 3
#define FAMILIAR_DELAY 100

#define MAX_CLIENTS 2
#define MAX_NAME_LENGTH 14

#define MAX_MESSAGES 4

typedef struct __attribute__((__packed__)) packed
{
    char c1; //1bajt
    int i1; //4bajty
    char c2; //1bajt
    int i2; //4bajty
} packed_t;
typedef struct not_packed
{
    char c1;
    int i1;
    char c2;
    int i2;
}notpacked_t;

//przykladowa se napisalem
typedef struct {
    int test;
    char char1;
    char char2;
} __attribute__((__packed__)) message_t;


typedef struct{
    uint16_t spell_index;
    uint16_t x;
    uint16_t y;
}cast_t;
typedef struct{
    cast_t cast_queue[MAX_QUEUE]; // tablica z zakleciami

    int head; //skad zdejmujemy
    int tail; //gdzie dokladamy

    pthread_mutex_t mutex;

    sem_t slots_sem; //liczba wolnych miejsc
    sem_t casts_sem; //liczba zaklec w kolejce

}queue_fifo_t; 

void init_queue(queue_fifo_t*queue){
    queue->head = 0;
    queue->tail = 0;

    if(pthread_mutex_init(&queue->mutex,NULL)!=0)
    ERR("pthread_mutex_init");
    
    if(sem_init(&queue->slots_sem,0,MAX_QUEUE)<0)
    ERR("sem_init: slots_sem");

    if(sem_init(&queue->casts_sem,0,0)<0)
    ERR("sem_init: slots_sem");
}

void pop_queue(queue_fifo_t* queue, cast_t* out_cast){
    while(sem_wait(&queue->casts_sem)==-1){
        if(errno==EINTR)
        continue;
        else {
            ERR("sem_wait:casts_sem");
        }
    }

    pthread_mutex_lock(&queue->mutex);
    *out_cast = queue->cast_queue[queue->head];
    queue->head = (queue->head+1)%MAX_QUEUE;
    pthread_mutex_unlock(&queue->mutex);

    if(sem_post(&queue->slots_sem)<0)
    ERR("sem_post casts");
    
}


int push_queue(queue_fifo_t* queue, cast_t cast){
    
    while(sem_trywait(&queue->slots_sem)==-1){
        if(errno==EINTR)
        continue;

        if(errno==EAGAIN){
            return -1; //kolejka pelna to zwrocilo mi EAGAIN
        }

        ERR("sem_trywait:slots");
    }

    pthread_mutex_lock(&queue->mutex);
    queue->cast_queue[queue->tail]=cast;
    queue->tail = (queue->tail+1) % MAX_QUEUE;
    pthread_mutex_unlock(&queue->mutex);

    if(sem_post(&queue->casts_sem)<0)
    ERR("sem_post casts");

    return 0;
}
void usage(char* name)
{
    printf("%s <in_port>\n", name);
    printf("  in_port - port that accepts messages\n");
    exit(EXIT_FAILURE);
}
int validate(int spell_index, int x, int y){
    if(spell_index<0 || spell_index>=SPELL_TYPES)
    return -1;
    if(x<0 || x>=BOARD_SIZE)
    return -1;
    if(y<0 || y>=BOARD_SIZE)
    return -1;

    return 0;
    
}
void *worker_routine(void*arg){
    queue_fifo_t* queue = arg;
    cast_t out;
    while(1){

    pop_queue(queue,&out);
    ms_sleep(FAMILIAR_DELAY);

    //wypisac i elo
    printf("[Cast] %s onto %d %d\n",spell_names[out.spell_index],out.x,out.y);
    }
   
    return NULL;
}
void doServer(int server_fd){
    char recvbuff[MAX_NAME_LENGTH+2];

    queue_fifo_t queue;
    init_queue(&queue);

    pthread_t threads[THREAD_COUNT];
    for(int i=0; i<THREAD_COUNT; i++){
        if(pthread_create(&threads[i],NULL,worker_routine,&queue)!=0)
        ERR("pthread_create");
    }

    while(1){

        memset(recvbuff,0,sizeof(recvbuff));

        int received = recv(server_fd,recvbuff,sizeof(recvbuff),0);
        if(received<0)
        ERR("recv");

        char type;
        type = recvbuff[0];
        
        /*
        inny parser z chata 
        uint16_t net_spell, net_x, net_y;

memcpy(&net_spell, recvbuff + 2, sizeof(uint16_t));
memcpy(&net_x,     recvbuff + 4, sizeof(uint16_t));
memcpy(&net_y,     recvbuff + 6, sizeof(uint16_t));

uint16_t spell_index = ntohs(net_spell);
uint16_t x_target = ntohs(net_x);
uint16_t y_target = ntohs(net_y);
        */
        uint16_t spell_index = -1;
        uint16_t x_target = -1;
        uint16_t y_target = -1;
        switch(type){
            case 'l':
            printf("[Login] Welcome, %s\n",recvbuff+sizeof(char)*2);
            break;

            case 'c':
            {
            int ret = sscanf(recvbuff+sizeof(char)*2,"%hu %hu %hu",&spell_index,&x_target,&y_target);
            if(ret!=3){
                printf("Podaj poprawne zaklecie: spell_index x_target y_target\n");
                continue;
            }
            if(validate(spell_index,x_target,y_target)<0){
                printf("Podaj poprawne zaklecie: spell_index x_target y_target\n");
                continue;
            }


            //teraz wrzucam na kolejke 
            cast_t cast;
            cast.spell_index = spell_index;
            cast.x = x_target;
            cast.y = y_target;

            if(push_queue(&queue,cast)<0)
            printf("Brak miejsca w kolejce\n");

            
            break;

        }

            case 'q':
            printf("[Quit] Someone quit. Goodbye!\n");
            break;

            default:
            printf("Wybierz poprawny typ: l,c lub q\n");
            continue;
            break;
        }

        
    }
     for(int i=0; i<THREAD_COUNT; i++){
        pthread_join(threads[i],NULL);
    }
}
int main(int argc, char** argv)
{
    /*
    printf("sizeof(struct packed) == %d\n", sizeof(struct packed));
    printf("sizeof(struct packed) moja == %d\n", sizeof(message_t));
    printf("sizeof(struct not_packed) == %d\n", sizeof(struct not_packed));
    */
   if(argc!=2){
    usage(argv[0]);
   }
   int server_fd = bind_inet_socket((uint16_t)atoi(argv[1]),SOCK_DGRAM,BACKLOG);

   doServer(server_fd);
   if(TEMP_FAILURE_RETRY(close(server_fd))<0)
   ERR("close:server_fd");

   return EXIT_SUCCESS;
}
