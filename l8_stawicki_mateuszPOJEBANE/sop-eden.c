#include "l8_common.h"

#define MSG_MAX 64
#define USERS 10
#define THREADS 8
#define LOGIN_LEN 16
#define COMMAND_LEN 8
#define COMMANDS_NUM 6

#define COMMANDS_COUNT 6
#define MAX_PARAMS 10
#define HEADER_LEN (LOGIN_LEN + COMMAND_LEN)
#define RESPONSE 16

static char* LOGINS[USERS] = {"kaczmarskik", "jelowickif", "hermant",  "turs",     "krasowskip",
                              "larysaz",     "zygulas",    "homendaw", "galazkap", "jastrzebskaaaaaa"};
static char* COMMANDS[COMMANDS_NUM] = {"RUN", "EXIT", "PAUSE", "COMPUTE", "LIST", "GATHER"};

void usage(char* name)
{
    printf("%s <in_port>\n", name);
    printf("  in_port - port that accepts messages\n");
    exit(EXIT_FAILURE);
}

typedef struct
{
    char login[LOGIN_LEN];        // 16b
    char command[COMMAND_LEN];    // 8b
    uint32_t params[MAX_PARAMS];  // 40b
    int params_count;
} Message_t;

typedef enum
{
    RUN,
    EXIT,
    PAUSE,
    COMPUTE,
    LIST,
    GATHER
} Command_enum;

int is_logged_in(const char login[])
{
    for (int i = 0; i < USERS; i++)
    {
        if (strncmp(LOGINS[i], login, LOGIN_LEN) == 0)
            return i;
    }
    return -1;
}

int get_command_number(const char command[])
{
    for (int i = 0; i < COMMANDS_COUNT; i++)
    {
        if (strncmp(command, COMMANDS[i], COMMAND_LEN) == 0)
            return i;
    }
    return -1;
}

typedef struct ListNode
{
    uint32_t count;
    uint32_t seed;
    int owner_index;

    struct ListNode* next_node;
} ListNode_t;

typedef struct
{
    ListNode_t* head;  // tu mamy najstarsze zadanie HEAD -> next -> next -> TAIL
    ListNode_t* tail;  // tu mamy miejsce na dołączenie do kolejki nowych zadań
} List_t;

void init_list(List_t* list)
{
    list->head = NULL;
    list->tail = NULL;
}

ListNode_t* create_node(int owner_index, uint32_t count, uint32_t seed)
{
    ListNode_t* node = malloc(sizeof(ListNode_t));
    if (node == NULL)
        ERR("malloc");

    node->count = count;
    node->seed = seed;
    node->owner_index = owner_index;
    node->next_node = NULL;

    return node;
}

void add_to_list(List_t* list, ListNode_t* node)
{
    node->next_node = NULL;
    if (list->head == NULL)
    {
        list->head = node;
        list->tail = node;
        return;
    }
    list->tail->next_node = node;
    list->tail = node;
}

ListNode_t* remove_from_list(List_t* list)
{
    if (list->head == NULL)
        return NULL;  // pusta lista

    ListNode_t* ans = list->head;
    list->head = list->head->next_node;
    // a co jak usunelismy ostatni element ?
    if (list->head == NULL)
    {
        list->tail = NULL;
    }

    // tak no i odepnijmy jeszcze
    ans->next_node = NULL;
    return ans;
}
void destroy_list(List_t* list)
{
    ListNode_t* node;
    while ((node = remove_from_list(list)) != NULL)
    {
        free(node);
    }
}
void send_tasks2(
    int server_fd,
    int user_index,
    const struct sockaddr_in* addr,
    const List_t* list)
{
    struct sockaddr_in response_addr = *addr;
    response_addr.sin_port = htons(4001);

    char response[MSG_MAX];
    size_t used = 0;

    const ListNode_t* curr = list->head;

    while (curr != NULL)
    {
        if (curr->owner_index == user_index)
        {
            char task_text[64];

            int task_length = snprintf(
                task_text,
                sizeof(task_text),
                "count=%u seed=%u\n",
                curr->count,
                curr->seed
            );

            if (task_length < 0)
                ERR("snprintf");

            /*
             * Jeżeli kolejne zadanie nie mieści się w aktualnym
             * datagramie, wysyłamy dotychczasową zawartość.
             */
            if (used + (size_t)task_length > MSG_MAX)
            {
                ssize_t sent_bytes = sendto(
                    server_fd,
                    response,
                    used,
                    0,
                    (struct sockaddr*)&response_addr,
                    sizeof(response_addr)
                );

                if (sent_bytes < 0)
                    ERR("sendto");

                used = 0;
            }

            memcpy(response + used, task_text, task_length);
            used += task_length;
        }

        curr = curr->next_node;
    }

    if (used > 0)
    {
        ssize_t sent_bytes = sendto(
            server_fd,
            response,
            used,
            0,
            (struct sockaddr*)&response_addr,
            sizeof(response_addr)
        );

        if (sent_bytes < 0)
            ERR("sendto");
    }
}
void add_tasks(const Message_t* message, List_t* list, int user_index)
{
    for (int i = 0; i < message->params_count; i += 2)
    {
        uint32_t count = message->params[i];
        uint32_t seed = message->params[i + 1];

        if (count > 10000000)
        {
            printf("error: Zadanie pominiete za duzy count %u", count);
            continue;
        }
        ListNode_t* node = create_node(user_index, count, seed);
        add_to_list(list, node);
    }
}

void send_tasks(int server_fd, int user_index, const struct sockaddr_in* addr, const List_t* list)
{
    // dla user o tym indexie znajduje wszystkie taski na liscie i wysyłam mu na port o 1 wiekszy niz dostałem

    struct sockaddr_in response_addr = *addr;
    uint16_t requests_port = ntohs(response_addr.sin_port);
    response_addr.sin_port = htons(4001);

    uint32_t response[RESPONSE];
    int values_count = 0;

    ListNode_t* curr = list->head;

    while (curr != NULL)
    {
        if (curr->owner_index == user_index)
        {
            if (values_count > RESPONSE - 2)
            {
                // wysyłam paczke
                ssize_t sent_bytes = sendto(server_fd, response, sizeof(uint32_t) * values_count, 0,
                                            (struct sockaddr*)&response_addr, sizeof(response_addr));
                values_count = 0;
            }
            response[values_count] = htonl(curr->count);
            values_count++;
            response[values_count] = htonl(curr->seed);
            values_count++;
        }
        curr = curr->next_node;
    }

    // wysyłam paczke jesli sie cala nie uzbierala to daje tyle ile jest
    if (values_count > 0)
    {
        char buff[MSG_MAX];
        memcpy(buff,response,sizeof(uint32_t)*values_count);
        printf("wyslalelm %.64s\n",buff);
        ssize_t sent_bytes = sendto(server_fd, response, sizeof(uint32_t) * values_count, 0,
                                    (struct sockaddr*)&response_addr, sizeof(response_addr));
        values_count = 0;
    }
}
void doServer(int server_fd)
{
    List_t list;
    init_list(&list);

    Message_t message;
    int doWork = 1;
    while (doWork)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        ssize_t read_bytes = recvfrom(server_fd, &message, MSG_MAX + 1, 0, (struct sockaddr*)(&addr), &len);
        if (read_bytes < 0)
        {
            if (errno == EINTR)
                continue;

            ERR("recvfrom");
        }

        if (read_bytes < HEADER_LEN || read_bytes > MSG_MAX)
        {
            printf("Bad message length=%ld\n", read_bytes);
            continue;
        }

        const int user_index = is_logged_in(message.login);
        if (user_index < 0)
        {
            printf("Wrong login: %.16s\n", message.login);
            continue;
        }

        const int command_index = get_command_number(message.command);

        switch (command_index)
        {
            case -1:
            {
                printf("Wrong command %.8s\n", message.command);
                break;
            }
            case EXIT:
            {
                if ((read_bytes - HEADER_LEN) != 0)
                {
                    printf("Wrong: command: %.8s cannot have params\n", message.command);
                    break;
                }
                printf("Login: %.16s Command: %.8s\n", message.login, message.command);
                doWork = 0;
                break;
            }
            case COMPUTE:
            {
                if ((read_bytes - HEADER_LEN) == 0)
                {
                    printf("Wrong: COMPUTE has to have minimum 2 params\n");
                    break;
                }
                if (((read_bytes - HEADER_LEN) % (2 * sizeof(uint32_t))) != 0)
                {
                    printf("Wrong COMPUTE params length: %ld\n", read_bytes - HEADER_LEN);
                    break;
                }
                printf("Login: %.16s Command: %.8s Params:", message.login, message.command);
                message.params_count = (read_bytes - HEADER_LEN) / sizeof(uint32_t);
                for (int i = 0; i < message.params_count; i++)
                {
                    message.params[i] = ntohl(message.params[i]);

                    printf(" %u", message.params[i]);
                }
                putchar('\n');

                /// dobra wypisałem a teraz wrzucam se zadania na kolejke
                add_tasks(&message, &list, user_index);

                break;
            }
            case LIST:
            {
                if ((read_bytes - HEADER_LEN) != 0)
                {
                    printf("Wrong: command: %.8s cannot have params\n", message.command);
                    break;
                }
                send_tasks2(server_fd, user_index, &addr, &list);
                break;
            }
            default:
            {
                if ((read_bytes - HEADER_LEN) != 0)
                {
                    printf("Wrong: command: %.8s cannot have params\n", message.command);
                    break;
                }
                printf("Login: %.16s Command: %.8s\n", message.login, message.command);
                break;
            }
        }
    }
    destroy_list(&list);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        usage(argv[0]);
    }
    int server_fd = bind_inet_socket((uint16_t)(atoi(argv[1])), SOCK_DGRAM, -1);

    doServer(server_fd);

    if (close(server_fd) < 0)
    {
        ERR("close");
    }

    printf("Server has terminated\n");
    return 0;
}
