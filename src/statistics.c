#include <string.h>

#include "common.h"
#include "statistics.h"

int statistics_init(Statistics *statistics) {
    if (statistics == NULL) {
        return APP_ERROR;
    }

    memset(statistics, 0, sizeof(*statistics));
    if (pthread_mutex_init(&statistics->mutex, NULL) != 0) {
        return APP_ERROR;
    }
    return APP_SUCCESS;
}

void statistics_destroy(Statistics *statistics) {
    if (statistics != NULL) {
        (void)pthread_mutex_destroy(&statistics->mutex);
    }
}

int statistics_record_produced(Statistics *statistics, int waited) {
    if (statistics == NULL || pthread_mutex_lock(&statistics->mutex) != 0) {
        return APP_ERROR;
    }

    statistics->total_produced++;
    if (waited != 0) {
        statistics->producer_wait_count++;
    }

    return pthread_mutex_unlock(&statistics->mutex) == 0
        ? APP_SUCCESS
        : APP_ERROR;
}

int statistics_record_consumed(Statistics *statistics, int waited) {
    if (statistics == NULL || pthread_mutex_lock(&statistics->mutex) != 0) {
        return APP_ERROR;
    }

    statistics->total_consumed++;
    if (waited != 0) {
        statistics->consumer_wait_count++;
    }

    return pthread_mutex_unlock(&statistics->mutex) == 0
        ? APP_SUCCESS
        : APP_ERROR;
}

int statistics_snapshot(
    Statistics *statistics,
    StatisticsSnapshot *snapshot
) {
    if (statistics == NULL || snapshot == NULL
        || pthread_mutex_lock(&statistics->mutex) != 0) {
        return APP_ERROR;
    }

    snapshot->total_produced = statistics->total_produced;
    snapshot->total_consumed = statistics->total_consumed;
    snapshot->producer_wait_count = statistics->producer_wait_count;
    snapshot->consumer_wait_count = statistics->consumer_wait_count;

    return pthread_mutex_unlock(&statistics->mutex) == 0
        ? APP_SUCCESS
        : APP_ERROR;
}
