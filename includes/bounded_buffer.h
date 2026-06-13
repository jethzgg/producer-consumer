#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int *items;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
#ifdef __APPLE__
    pthread_mutex_t semaphore_mutex;
    pthread_cond_t empty_available;
    pthread_cond_t full_available;
    unsigned int empty_count;
    unsigned int full_count;
#endif
} BoundedBuffer;

int buffer_init(BoundedBuffer *buffer, int capacity);
void buffer_destroy(BoundedBuffer *buffer);
int buffer_put(
    BoundedBuffer *buffer,
    int item,
    int *waited,
    int *count_after
);
int buffer_get(
    BoundedBuffer *buffer,
    int *item,
    int *waited,
    int *count_after
);

#endif
