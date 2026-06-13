#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bounded_buffer.h"
#include "common.h"

#ifndef __APPLE__
static int semaphore_wait(sem_t *semaphore, int *waited) {
    int result;

    if (waited != NULL) {
        *waited = 0;
    }

    do {
        result = sem_trywait(semaphore);
    } while (result != 0 && errno == EINTR);

    if (result == 0) {
        return APP_SUCCESS;
    }
    if (errno != EAGAIN) {
        return APP_ERROR;
    }

    if (waited != NULL) {
        *waited = 1;
    }
    do {
        result = sem_wait(semaphore);
    } while (result != 0 && errno == EINTR);

    return result == 0 ? APP_SUCCESS : APP_ERROR;
}

static int semaphore_post(sem_t *semaphore) {
    return sem_post(semaphore) == 0 ? APP_SUCCESS : APP_ERROR;
}
#else
static int semaphore_wait(
    BoundedBuffer *buffer,
    int wait_for_empty,
    int *waited
) {
    unsigned int *count = wait_for_empty != 0
        ? &buffer->empty_count
        : &buffer->full_count;
    pthread_cond_t *condition = wait_for_empty != 0
        ? &buffer->empty_available
        : &buffer->full_available;

    if (waited != NULL) {
        *waited = 0;
    }
    if (pthread_mutex_lock(&buffer->semaphore_mutex) != 0) {
        return APP_ERROR;
    }

    while (*count == 0U) {
        if (waited != NULL) {
            *waited = 1;
        }
        if (pthread_cond_wait(condition, &buffer->semaphore_mutex) != 0) {
            (void)pthread_mutex_unlock(&buffer->semaphore_mutex);
            return APP_ERROR;
        }
    }
    (*count)--;

    return pthread_mutex_unlock(&buffer->semaphore_mutex) == 0
        ? APP_SUCCESS
        : APP_ERROR;
}

static int semaphore_post(BoundedBuffer *buffer, int post_empty) {
    unsigned int *count = post_empty != 0
        ? &buffer->empty_count
        : &buffer->full_count;
    pthread_cond_t *condition = post_empty != 0
        ? &buffer->empty_available
        : &buffer->full_available;
    int signal_result;
    int unlock_result;

    if (pthread_mutex_lock(&buffer->semaphore_mutex) != 0) {
        return APP_ERROR;
    }
    (*count)++;
    signal_result = pthread_cond_signal(condition);
    unlock_result = pthread_mutex_unlock(&buffer->semaphore_mutex);
    return signal_result == 0 && unlock_result == 0
        ? APP_SUCCESS
        : APP_ERROR;
}
#endif

int buffer_init(BoundedBuffer *buffer, int capacity) {
    if (buffer == NULL || capacity <= 0) {
        return APP_ERROR;
    }

    memset(buffer, 0, sizeof(*buffer));
    buffer->items = calloc((size_t)capacity, sizeof(*buffer->items));
    if (buffer->items == NULL) {
        return APP_ERROR;
    }
    buffer->capacity = capacity;

    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        free(buffer->items);
        buffer->items = NULL;
        return APP_ERROR;
    }

#ifndef __APPLE__
    if (sem_init(&buffer->empty, 0, (unsigned int)capacity) != 0) {
        (void)pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        buffer->items = NULL;
        return APP_ERROR;
    }
    if (sem_init(&buffer->full, 0, 0) != 0) {
        (void)sem_destroy(&buffer->empty);
        (void)pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        buffer->items = NULL;
        return APP_ERROR;
    }
#else
    if (pthread_mutex_init(&buffer->semaphore_mutex, NULL) != 0) {
        (void)pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        buffer->items = NULL;
        return APP_ERROR;
    }
    if (pthread_cond_init(&buffer->empty_available, NULL) != 0) {
        (void)pthread_mutex_destroy(&buffer->semaphore_mutex);
        (void)pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        buffer->items = NULL;
        return APP_ERROR;
    }
    if (pthread_cond_init(&buffer->full_available, NULL) != 0) {
        (void)pthread_cond_destroy(&buffer->empty_available);
        (void)pthread_mutex_destroy(&buffer->semaphore_mutex);
        (void)pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        buffer->items = NULL;
        return APP_ERROR;
    }
    buffer->empty_count = (unsigned int)capacity;
    buffer->full_count = 0U;
#endif

    return APP_SUCCESS;
}

void buffer_destroy(BoundedBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }

#ifndef __APPLE__
    (void)sem_destroy(&buffer->full);
    (void)sem_destroy(&buffer->empty);
#else
    (void)pthread_cond_destroy(&buffer->full_available);
    (void)pthread_cond_destroy(&buffer->empty_available);
    (void)pthread_mutex_destroy(&buffer->semaphore_mutex);
#endif
    (void)pthread_mutex_destroy(&buffer->mutex);
    free(buffer->items);
    buffer->items = NULL;
}

int buffer_put(
    BoundedBuffer *buffer,
    int item,
    int *waited,
    int *count_after
) {
    int unlock_result;
    int post_result;

    if (buffer == NULL || buffer->items == NULL) {
        return APP_ERROR;
    }

#ifndef __APPLE__
    if (semaphore_wait(&buffer->empty, waited) != APP_SUCCESS) {
        return APP_ERROR;
    }
#else
    if (semaphore_wait(buffer, 1, waited) != APP_SUCCESS) {
        return APP_ERROR;
    }
#endif

    if (pthread_mutex_lock(&buffer->mutex) != 0) {
#ifndef __APPLE__
        (void)semaphore_post(&buffer->empty);
#else
        (void)semaphore_post(buffer, 1);
#endif
        return APP_ERROR;
    }

    buffer->items[buffer->tail] = item;
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    buffer->count++;
    assert(buffer->count >= 0);
    assert(buffer->count <= buffer->capacity);
    if (count_after != NULL) {
        *count_after = buffer->count;
    }
    unlock_result = pthread_mutex_unlock(&buffer->mutex);

#ifndef __APPLE__
    post_result = semaphore_post(&buffer->full);
#else
    post_result = semaphore_post(buffer, 0);
#endif
    return unlock_result == 0 && post_result == APP_SUCCESS
        ? APP_SUCCESS
        : APP_ERROR;
}

int buffer_get(
    BoundedBuffer *buffer,
    int *item,
    int *waited,
    int *count_after
) {
    int unlock_result;
    int post_result;

    if (buffer == NULL || buffer->items == NULL || item == NULL) {
        return APP_ERROR;
    }

#ifndef __APPLE__
    if (semaphore_wait(&buffer->full, waited) != APP_SUCCESS) {
        return APP_ERROR;
    }
#else
    if (semaphore_wait(buffer, 0, waited) != APP_SUCCESS) {
        return APP_ERROR;
    }
#endif

    if (pthread_mutex_lock(&buffer->mutex) != 0) {
#ifndef __APPLE__
        (void)semaphore_post(&buffer->full);
#else
        (void)semaphore_post(buffer, 0);
#endif
        return APP_ERROR;
    }

    *item = buffer->items[buffer->head];
    buffer->head = (buffer->head + 1) % buffer->capacity;
    buffer->count--;
    assert(buffer->count >= 0);
    assert(buffer->count <= buffer->capacity);
    if (count_after != NULL) {
        *count_after = buffer->count;
    }
    unlock_result = pthread_mutex_unlock(&buffer->mutex);

#ifndef __APPLE__
    post_result = semaphore_post(&buffer->empty);
#else
    post_result = semaphore_post(buffer, 1);
#endif
    return unlock_result == 0 && post_result == APP_SUCCESS
        ? APP_SUCCESS
        : APP_ERROR;
}
