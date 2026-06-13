#include <pthread.h>
#include <time.h>

#include "bounded_buffer.h"
#include "common.h"
#include "test_support.h"

typedef struct {
    BoundedBuffer *buffer;
    int item;
    int waited;
    int result;
} BufferThreadArgs;

static void sleep_briefly(void) {
    const struct timespec delay = {0, 20000000L};
    (void)nanosleep(&delay, NULL);
}

static void *put_item(void *argument) {
    BufferThreadArgs *args = argument;
    args->result = buffer_put(
        args->buffer,
        args->item,
        &args->waited,
        NULL
    );
    return NULL;
}

static void *get_item(void *argument) {
    BufferThreadArgs *args = argument;
    args->result = buffer_get(
        args->buffer,
        &args->item,
        &args->waited,
        NULL
    );
    return NULL;
}

void test_bounded_buffer(void) {
    BoundedBuffer buffer;
    int item = 0;
    int count = 0;
    int waited = -1;

    ASSERT_EQ_LONG(APP_ERROR, buffer_init(NULL, 2));
    ASSERT_EQ_LONG(APP_ERROR, buffer_init(&buffer, 0));
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_init(&buffer, 2));
    ASSERT_EQ_LONG(APP_ERROR, buffer_put(NULL, 1, NULL, NULL));
    ASSERT_EQ_LONG(APP_ERROR, buffer_get(NULL, &item, NULL, NULL));
    ASSERT_EQ_LONG(APP_ERROR, buffer_get(&buffer, NULL, NULL, NULL));

    ASSERT_EQ_LONG(APP_SUCCESS, buffer_put(&buffer, 10, &waited, &count));
    ASSERT_EQ_LONG(0, waited);
    ASSERT_EQ_LONG(1, count);
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_put(&buffer, 20, NULL, &count));
    ASSERT_EQ_LONG(2, count);
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_get(&buffer, &item, NULL, &count));
    ASSERT_EQ_LONG(10, item);
    ASSERT_EQ_LONG(1, count);
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_put(&buffer, 30, NULL, NULL));
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_get(&buffer, &item, NULL, NULL));
    ASSERT_EQ_LONG(20, item);
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_get(&buffer, &item, NULL, NULL));
    ASSERT_EQ_LONG(30, item);

    BufferThreadArgs consumer = {&buffer, 0, 0, APP_ERROR};
    pthread_t consumer_thread;
    ASSERT_EQ_LONG(0, pthread_create(
        &consumer_thread, NULL, get_item, &consumer
    ));
    sleep_briefly();
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_put(&buffer, 40, NULL, NULL));
    ASSERT_EQ_LONG(0, pthread_join(consumer_thread, NULL));
    ASSERT_EQ_LONG(APP_SUCCESS, consumer.result);
    ASSERT_EQ_LONG(1, consumer.waited);
    ASSERT_EQ_LONG(40, consumer.item);

    ASSERT_EQ_LONG(APP_SUCCESS, buffer_put(&buffer, 50, NULL, NULL));
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_put(&buffer, 60, NULL, NULL));
    BufferThreadArgs producer = {&buffer, 70, 0, APP_ERROR};
    pthread_t producer_thread;
    ASSERT_EQ_LONG(0, pthread_create(
        &producer_thread, NULL, put_item, &producer
    ));
    sleep_briefly();
    ASSERT_EQ_LONG(APP_SUCCESS, buffer_get(&buffer, &item, NULL, NULL));
    ASSERT_EQ_LONG(0, pthread_join(producer_thread, NULL));
    ASSERT_EQ_LONG(APP_SUCCESS, producer.result);
    ASSERT_EQ_LONG(1, producer.waited);

    buffer_destroy(&buffer);
    buffer_destroy(NULL);
}
