#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "logger.h"
#include "worker.h"

#define WORKER_FAILURE ((void *)(uintptr_t)1U)

static unsigned int next_random(unsigned int *state) {
    unsigned int value = *state;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static int random_delay(
    int minimum_ms,
    int maximum_ms,
    unsigned int *state
) {
    unsigned int range;

    if (maximum_ms <= minimum_ms) {
        return minimum_ms;
    }
    range = (unsigned int)(maximum_ms - minimum_ms + 1);
    return minimum_ms + (int)(next_random(state) % range);
}

static int sleep_milliseconds(int milliseconds) {
    struct timespec remaining = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (long)(milliseconds % 1000) * 1000000L
    };

    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno != EINTR) {
            return APP_ERROR;
        }
    }
    return APP_SUCCESS;
}

static unsigned int make_seed(int id) {
    struct timespec now;
    unsigned int seed = (unsigned int)id * 2654435761U;

    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
        seed ^= (unsigned int)now.tv_nsec;
        seed ^= (unsigned int)now.tv_sec;
    }
    return seed == 0U ? 1U : seed;
}

void *producer_run(void *argument) {
    ProducerArgs *args = argument;
    unsigned int random_state;
    char component[24];
    int sequence;

    if (args == NULL || args->buffer == NULL || args->statistics == NULL
        || args->config == NULL) {
        return WORKER_FAILURE;
    }

    random_state = make_seed(args->id);
    (void)snprintf(component, sizeof(component), "Producer %d", args->id);

    for (sequence = 1; sequence <= args->config->items_per_producer;
         sequence++) {
        int item;
        int waited;
        int count_after;
        int delay = random_delay(
            args->config->producer_delay_min_ms,
            args->config->producer_delay_max_ms,
            &random_state
        );

        if (sleep_milliseconds(delay) != APP_SUCCESS) {
            return WORKER_FAILURE;
        }

        item = args->id * 100000 + sequence;
        if (buffer_put(
                args->buffer,
                item,
                &waited,
                &count_after
            ) != APP_SUCCESS) {
            return WORKER_FAILURE;
        }
        if (statistics_record_produced(
                args->statistics,
                waited
            ) != APP_SUCCESS) {
            return WORKER_FAILURE;
        }
        logger_event(
            component,
            "Produced item %d | Buffer: %d/%d%s",
            item,
            count_after,
            args->config->buffer_capacity,
            waited != 0 ? " | waited" : ""
        );
    }

    return NULL;
}

void *consumer_run(void *argument) {
    ConsumerArgs *args = argument;
    unsigned int random_state;
    char component[24];

    if (args == NULL || args->buffer == NULL || args->statistics == NULL
        || args->config == NULL) {
        return WORKER_FAILURE;
    }

    random_state = make_seed(args->id + 1000);
    (void)snprintf(component, sizeof(component), "Consumer %d", args->id);

    for (;;) {
        int item;
        int waited;
        int count_after;
        int delay;

        if (buffer_get(
                args->buffer,
                &item,
                &waited,
                &count_after
            ) != APP_SUCCESS) {
            return WORKER_FAILURE;
        }
        if (item == TERMINATION_ITEM) {
            return NULL;
        }
        if (statistics_record_consumed(
                args->statistics,
                waited
            ) != APP_SUCCESS) {
            return WORKER_FAILURE;
        }
        logger_event(
            component,
            "Consumed item %d | Buffer: %d/%d%s",
            item,
            count_after,
            args->config->buffer_capacity,
            waited != 0 ? " | waited" : ""
        );

        delay = random_delay(
            args->config->consumer_delay_min_ms,
            args->config->consumer_delay_max_ms,
            &random_state
        );
        if (sleep_milliseconds(delay) != APP_SUCCESS) {
            return WORKER_FAILURE;
        }
    }
}
