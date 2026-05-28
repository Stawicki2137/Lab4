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
/*
typedef struct{
    cast_t
}queue_fifo_t; */
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
void doServer(int server_fd){
    int counter = 0;
    char recvbuff[MAX_NAME_LENGTH+2];
    while(1){
        if(counter>=MAX_MESSAGES)
        break;

        memset(recvbuff,0,sizeof(recvbuff));

        int received = recv(server_fd,recvbuff,sizeof(recvbuff),0);
        if(received<0)
        ERR("recv");

        char type;
        type = recvbuff[0];
        
        uint16_t spell_index = -1;
        uint16_t x_target = -1;
        uint16_t y_target = -1;
        switch(type){
            case 'l':
            printf("[Login] Welcome, %s\n",recvbuff+sizeof(char)*2);
            break;

            case 'c':
            int ret = sscanf(recvbuff+sizeof(char)*2,"%hu %hu %hu",&spell_index,&x_target,&y_target);
            if(ret!=3){
                printf("Podaj poprawne zaklecie: spell_index x_target y_target\n");
                continue;
            }
            if(validate(spell_index,x_target,y_target)<0){
                printf("Podaj poprawne zaklecie: spell_index x_target y_target\n");
                continue;
            }

            // dobra cos tam zvalidowane to teraz komunikat:

            printf("[Cast] %s onto %d %d\n",spell_names[spell_index],x_target,y_target);
            break;

            case 'q':
            printf("[Quit] Someone quit. Goodbye!\n");
            break;

            default:
            printf("Wybierz poprawny typ: l,c lub q\n");
            continue;
            break;
        }

        counter++;
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
