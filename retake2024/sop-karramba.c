#define _GNU_SOURCE
#include "board_utils.h"
#include "common.h"

#define BOARD_FILE "board"
#define FIFO_NAME "fifo"
#define STEP_COUNT 500
#define WAIT_N 10

#define PORT 12345
#define EPOLL_MAX_EVENTS 10
#define BOARD_MIN_SIZE 22

void usage(char* program_name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\t%s n m\n", program_name);
    fprintf(
        stderr,
        "\t  n, m - board width and height, respectively\n"
    );

    exit(EXIT_FAILURE);
}

void lock_mutex(pthread_mutex_t* mutex)
{
    int error = pthread_mutex_lock(mutex);

    if (error == EOWNERDEAD)
    {
        error = pthread_mutex_consistent(mutex);

        if (error != 0)
        {
            errno = error;
            ERR("pthread_mutex_consistent");
        }
    }
    else if (error != 0)
    {
        errno = error;
        ERR("pthread_mutex_lock");
    }
}

void unlock_mutex(pthread_mutex_t* mutex)
{
    int error = pthread_mutex_unlock(mutex);

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutex_unlock");
    }
}

void don_pedro_work(
    char* board,
    int n,
    int m,
    int pedro_x,
    int pedro_y,
    pthread_mutex_t* mutex
)
{
    ms_sleep(WAIT_N * 100);

    for (int i = 0; i < STEP_COUNT; ++i)
    {
        lock_mutex(mutex);

        if (has_trail(board, pedro_x, pedro_y, n, m))
        {
            set_char(
                board,
                pedro_x,
                pedro_y,
                n,
                m,
                EMPTY_CHAR
            );
        }
        else
        {
            set_char(
                board,
                pedro_x,
                pedro_y,
                n,
                m,
                KARRAMBA_CHAR
            );

            printf("Carramba!\n");
            fflush(stdout);
        }

        char move = get_trail_move(
            board,
            pedro_x,
            pedro_y,
            n,
            m
        );

        move_pos(
            board,
            move,
            n,
            m,
            &pedro_x,
            &pedro_y
        );

        unlock_mutex(mutex);

        ms_sleep(100);
    }
}

pid_t create_don_pedro(
    char* board,
    int n,
    int m,
    int start_x,
    int start_y,
    pthread_mutex_t* mutex,
    ssize_t board_size
)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        ERR("fork");
    }

    if (pid == 0)
    {
        srand((unsigned int)(time(NULL) ^ getpid()));

        don_pedro_work(
            board,
            n,
            m,
            start_x,
            start_y,
            mutex
        );

        if (munmap(board, board_size) == -1)
        {
            ERR("munmap board child");
        }

        if (munmap(mutex, sizeof(pthread_mutex_t)) == -1)
        {
            ERR("munmap mutex child");
        }

        _exit(EXIT_SUCCESS);
    }

    return pid;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if (n <= BOARD_MIN_SIZE || m <= BOARD_MIN_SIZE)
    {
        usage(argv[0]);
    }

    ssize_t board_size = (ssize_t)m * (n + 1);

    int board_fd = open(
        BOARD_FILE,
        O_CREAT | O_RDWR | O_TRUNC,
        0666
    );

    if (board_fd == -1)
    {
        ERR("open");
    }

    if (ftruncate(board_fd, board_size) == -1)
    {
        ERR("ftruncate");
    }

    char* board = mmap(
        NULL,
        board_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        board_fd,
        0
    );

    if (board == MAP_FAILED)
    {
        ERR("mmap board");
    }

    if (close(board_fd) == -1)
    {
        ERR("close");
    }

    fill_board(board, n, m);

    pthread_mutex_t* mutex = mmap(
        NULL,
        sizeof(pthread_mutex_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (mutex == MAP_FAILED)
    {
        ERR("mmap mutex");
    }

    pthread_mutexattr_t mutex_attr;

    int error = pthread_mutexattr_init(&mutex_attr);

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutexattr_init");
    }

    error = pthread_mutexattr_setpshared(
        &mutex_attr,
        PTHREAD_PROCESS_SHARED
    );

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutexattr_setpshared");
    }

    error = pthread_mutexattr_setrobust(
        &mutex_attr,
        PTHREAD_MUTEX_ROBUST
    );

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutexattr_setrobust");
    }

    error = pthread_mutex_init(mutex, &mutex_attr);

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutex_init");
    }

    error = pthread_mutexattr_destroy(&mutex_attr);

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutexattr_destroy");
    }

    srand((unsigned int)time(NULL));

    int expedition_x = rand() % n;
    int expedition_y = rand() % m;

    set_char(
        board,
        expedition_x,
        expedition_y,
        n,
        m,
        EXPEDITION_CHAR
    );

    pid_t pedro_pid = create_don_pedro(
        board,
        n,
        m,
        expedition_x,
        expedition_y,
        mutex,
        board_size
    );

    for (int i = 0; i < STEP_COUNT; ++i)
    {
        lock_mutex(mutex);

        char random_move = get_random_move(
            board,
            expedition_x,
            expedition_y,
            n,
            m
        );

        set_char(
            board,
            expedition_x,
            expedition_y,
            n,
            m,
            TRAIL_CHAR
        );

        move_pos(
            board,
            random_move,
            n,
            m,
            &expedition_x,
            &expedition_y
        );

        set_char(
            board,
            expedition_x,
            expedition_y,
            n,
            m,
            EXPEDITION_CHAR
        );

        unlock_mutex(mutex);

        ms_sleep(100);
    }

    /*
     * Najpierw czekamy, aż Don Pedro wykona wszystkie ruchy.
     */
    if (waitpid(pedro_pid, NULL, 0) == -1)
    {
        ERR("waitpid");
    }

    /*
     * Po waitpid dziecko już niczego nie wypisze.
     */
    printf("Smok-Expedition completed!\n");

    error = pthread_mutex_destroy(mutex);

    if (error != 0)
    {
        errno = error;
        ERR("pthread_mutex_destroy");
    }

    if (munmap(mutex, sizeof(pthread_mutex_t)) == -1)
    {
        ERR("munmap mutex");
    }

    if (munmap(board, board_size) == -1)
    {
        ERR("munmap board");
    }

    return EXIT_SUCCESS;
}
