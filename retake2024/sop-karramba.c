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
    fprintf(stderr, "Usage: \n");

    fprintf(stderr, "\t%s n m\n", program_name);
    fprintf(stderr, "\t  n, m - board width and height, respectively\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if ((n <= BOARD_MIN_SIZE) || (m <= BOARD_MIN_SIZE))
    {
        usage(argv[0]);
    }
    printf("Wczytano n=%d,m=%d\n", n, m);

    int board_fd;
    if ((board_fd = open(BOARD_FILE, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1)
        ERR("open");
    if (ftruncate(board_fd, m * (n + 1)))
        ERR("ftruncate");

    char* board;
    if ((board = (char*)mmap(NULL, m * (n + 1), PROT_WRITE | PROT_READ, MAP_SHARED, board_fd, 0)) == MAP_FAILED)
        ERR("mmap");
    if (close(board_fd))
        ERR("close");

    fill_board(board, n, m);

    srand(time(NULL));

    int losowy_x = rand() % n;
    int losowy_y = rand() % m;
    set_char(board, losowy_x, losowy_y, n, m, 'S');

    for (int i = 0; i < STEP_COUNT; i++)
    {
        char random_move = get_random_move(board, losowy_x, losowy_y, n, m);

        // Stara pozycja staje się śladem.
        set_char(board, losowy_x, losowy_y, n, m, TRAIL_CHAR);

        // Funkcja aktualizuje losowy_x i losowy_y.
        move_pos(board, random_move, n, m, &losowy_x, &losowy_y);

        // Oznaczamy nową pozycję ekspedycji.
        set_char(board, losowy_x, losowy_y, n, m, EXPEDITION_CHAR);

        ms_sleep(100);
    }

    printf("Smok-Expedition completed!\n");
    if (munmap(board, m * (n + 1)))
        ERR("munmap");

    return EXIT_SUCCESS;
}
