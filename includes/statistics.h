#ifndef STATISTICS_H
#define STATISTICS_H

#include <pthread.h>

typedef struct {
    long total_produced;
    long total_consumed;
    long producer_wait_count;
    long consumer_wait_count;
    pthread_mutex_t mutex;
} Statistics;

typedef struct {
    long total_produced;
    long total_consumed;
    long producer_wait_count;
    long consumer_wait_count;
} StatisticsSnapshot;

int statistics_init(Statistics *statistics);
void statistics_destroy(Statistics *statistics);
int statistics_record_produced(Statistics *statistics, int waited);
int statistics_record_consumed(Statistics *statistics, int waited);
int statistics_snapshot(
    Statistics *statistics,
    StatisticsSnapshot *snapshot
);

#endif
