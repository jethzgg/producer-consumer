#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bounded_buffer.h"
#include "common.h"
#include "logger.h"
#include "simulation.h"
#include "statistics.h"
#include "worker.h"

static int validate_config(const SimulationConfig *config) {
    if (config == NULL
        || config->buffer_capacity <= 0
        || config->producer_count <= 0
        || config->consumer_count <= 0
        || config->items_per_producer <= 0
        || config->producer_delay_min_ms < 0
        || config->producer_delay_max_ms < config->producer_delay_min_ms
        || config->consumer_delay_min_ms < 0
        || config->consumer_delay_max_ms < config->consumer_delay_min_ms
        || (config->log_mode != LOG_MODE_SUMMARY
            && config->log_mode != LOG_MODE_DETAILED)) {
        return APP_ERROR;
    }

    if ((long)config->producer_count
            > LONG_MAX / (long)config->items_per_producer
        || config->items_per_producer >= 100000
        || config->producer_count
            > (INT_MAX - config->items_per_producer) / 100000
        || (size_t)config->producer_count
            > SIZE_MAX / sizeof(pthread_t)
        || (size_t)config->consumer_count
            > SIZE_MAX / sizeof(pthread_t)) {
        return APP_ERROR;
    }
    return APP_SUCCESS;
}

static double elapsed_seconds(
    const struct timespec *start,
    const struct timespec *finish
) {
    double seconds = (double)(finish->tv_sec - start->tv_sec);
    double nanoseconds = (double)(finish->tv_nsec - start->tv_nsec);
    return seconds + nanoseconds / 1000000000.0;
}

static void print_results(
    const SimulationConfig *config,
    const StatisticsSnapshot *snapshot,
    int remaining,
    double duration,
    int correct
) {
    long expected = (long)config->producer_count
        * (long)config->items_per_producer;
    double throughput = duration > 0.0
        ? (double)snapshot->total_consumed / duration
        : 0.0;

    printf("\nSimulation Results\n");
    printf("========================================\n");
    printf("Expected data items:  %ld\n", expected);
    printf("Total items produced: %ld\n", snapshot->total_produced);
    printf("Total items consumed: %ld\n", snapshot->total_consumed);
    printf("Items remaining:      %d\n", remaining);
    printf("Producer wait events: %ld\n", snapshot->producer_wait_count);
    printf("Consumer wait events: %ld\n", snapshot->consumer_wait_count);
    printf("Execution time:       %.3f seconds\n", duration);
    printf("Throughput:           %.2f items/second\n", throughput);
    printf("Correctness checks:   %s\n", correct != 0 ? "PASS" : "FAIL");
}

static int join_threads(pthread_t *threads, int count) {
    int status = APP_SUCCESS;
    int index;

    for (index = 0; index < count; index++) {
        void *worker_result = NULL;
        if (pthread_join(threads[index], &worker_result) != 0
            || worker_result != NULL) {
            status = APP_ERROR;
        }
    }
    return status;
}

static int send_termination_items(BoundedBuffer *buffer, int count) {
    int index;

    for (index = 0; index < count; index++) {
        if (buffer_put(
                buffer,
                TERMINATION_ITEM,
                NULL,
                NULL
            ) != APP_SUCCESS) {
            return APP_ERROR;
        }
    }
    return APP_SUCCESS;
}

int simulation_run(const SimulationConfig *config) {
    BoundedBuffer buffer;
    Statistics statistics;
    StatisticsSnapshot snapshot = {0};
    pthread_t *producer_threads = NULL;
    pthread_t *consumer_threads = NULL;
    ProducerArgs *producer_args = NULL;
    ConsumerArgs *consumer_args = NULL;
    struct timespec start;
    struct timespec finish;
    int buffer_initialized = 0;
    int statistics_initialized = 0;
    int logger_initialized = 0;
    int producers_created = 0;
    int consumers_created = 0;
    int status = APP_SUCCESS;
    int correct = 0;
    int index;

    if (validate_config(config) != APP_SUCCESS) {
        fputs("Invalid simulation configuration.\n", stderr);
        return APP_ERROR;
    }
    if (buffer_init(&buffer, config->buffer_capacity) != APP_SUCCESS) {
        fputs("Failed to initialize bounded buffer.\n", stderr);
        return APP_ERROR;
    }
    buffer_initialized = 1;

    if (statistics_init(&statistics) != APP_SUCCESS) {
        fputs("Failed to initialize statistics.\n", stderr);
        status = APP_ERROR;
        goto cleanup;
    }
    statistics_initialized = 1;

    if (logger_init(config->log_mode) != APP_SUCCESS) {
        fputs("Failed to initialize logger.\n", stderr);
        status = APP_ERROR;
        goto cleanup;
    }
    logger_initialized = 1;

    producer_threads = calloc(
        (size_t)config->producer_count,
        sizeof(*producer_threads)
    );
    consumer_threads = calloc(
        (size_t)config->consumer_count,
        sizeof(*consumer_threads)
    );
    producer_args = calloc(
        (size_t)config->producer_count,
        sizeof(*producer_args)
    );
    consumer_args = calloc(
        (size_t)config->consumer_count,
        sizeof(*consumer_args)
    );
    if (producer_threads == NULL || consumer_threads == NULL
        || producer_args == NULL || consumer_args == NULL) {
        fputs("Failed to allocate thread resources.\n", stderr);
        status = APP_ERROR;
        goto cleanup;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        fputs("Failed to start simulation timer.\n", stderr);
        status = APP_ERROR;
        goto cleanup;
    }

    for (index = 0; index < config->consumer_count; index++) {
        consumer_args[index].id = index + 1;
        consumer_args[index].buffer = &buffer;
        consumer_args[index].statistics = &statistics;
        consumer_args[index].config = config;
        if (pthread_create(
                &consumer_threads[index],
                NULL,
                consumer_run,
                &consumer_args[index]
            ) != 0) {
            fputs("Failed to create consumer thread.\n", stderr);
            status = APP_ERROR;
            break;
        }
        consumers_created++;
    }

    if (consumers_created == config->consumer_count) {
        for (index = 0; index < config->producer_count; index++) {
            producer_args[index].id = index + 1;
            producer_args[index].buffer = &buffer;
            producer_args[index].statistics = &statistics;
            producer_args[index].config = config;
            if (pthread_create(
                    &producer_threads[index],
                    NULL,
                    producer_run,
                    &producer_args[index]
                ) != 0) {
                fputs("Failed to create producer thread.\n", stderr);
                status = APP_ERROR;
                break;
            }
            producers_created++;
        }
    }

    if (join_threads(
            producer_threads,
            producers_created
        ) != APP_SUCCESS) {
        status = APP_ERROR;
    }
    if (send_termination_items(
            &buffer,
            consumers_created
        ) != APP_SUCCESS) {
        status = APP_ERROR;
    }
    if (join_threads(
            consumer_threads,
            consumers_created
        ) != APP_SUCCESS) {
        status = APP_ERROR;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &finish) != 0
        || statistics_snapshot(&statistics, &snapshot) != APP_SUCCESS) {
        fputs("Failed to collect simulation results.\n", stderr);
        status = APP_ERROR;
        goto cleanup;
    }

    correct = snapshot.total_produced
            == (long)config->producer_count * config->items_per_producer
        && snapshot.total_consumed == snapshot.total_produced
        && buffer.count == 0;
    print_results(
        config,
        &snapshot,
        buffer.count,
        elapsed_seconds(&start, &finish),
        correct
    );
    if (correct == 0) {
        status = APP_ERROR;
    }

cleanup:
    free(consumer_args);
    free(producer_args);
    free(consumer_threads);
    free(producer_threads);
    if (logger_initialized != 0) {
        logger_destroy();
    }
    if (statistics_initialized != 0) {
        statistics_destroy(&statistics);
    }
    if (buffer_initialized != 0) {
        buffer_destroy(&buffer);
    }
    return status;
}
