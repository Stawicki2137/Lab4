#include "l8_common.h"
#define MAX_QUEUE 10
typedef struct{
    uint16_t spell_index;
    uint16_t x;
    uint16_t y;
}cast_t;
typedef struct {
    cast_t cast_queue[MAX_QUEUE];

    int head;   // skąd zdejmujemy
    int tail;   // gdzie dokładamy
    int count;  // ile elementów jest aktualnie w kolejce

    pthread_mutex_t mutex;
    pthread_cond_t not_empty; // workerzy czekają, aż pojawi się cast
} queue_fifo_t;

/*Nie potrzebujesz tutaj not_full, bo w Twoim zadaniu producent ma nie czekać, gdy kolejka jest pełna, tylko odrzucić zaklęcie. */

void init_queue(queue_fifo_t *queue)
{
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    int err;

    err = pthread_mutex_init(&queue->mutex, NULL);
    if (err != 0) {
        errno = err;
        ERR("pthread_mutex_init");
    }

    err = pthread_cond_init(&queue->not_empty, NULL);
    if (err != 0) {
        errno = err;
        ERR("pthread_cond_init");
    }
}

int push_queue(queue_fifo_t *queue, cast_t cast)
{
    pthread_mutex_lock(&queue->mutex);

    if (queue->count == MAX_QUEUE) {
        pthread_mutex_unlock(&queue->mutex);
        return -1; // kolejka pełna
    }

    queue->cast_queue[queue->tail] = cast;
    queue->tail = (queue->tail + 1) % MAX_QUEUE;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);

    /*
    pthread_cond_signal()    -> obudź jeden czekający wątek
pthread_cond_broadcast() -> obudź wszystkie czekające wątki
    */

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

void pop_queue(queue_fifo_t *queue, cast_t *out_cast)
{
    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    *out_cast = queue->cast_queue[queue->head];
    queue->head = (queue->head + 1) % MAX_QUEUE;
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);
}