#include "common.h"
#include "statistics.h"
#include "test_support.h"

void test_statistics(void) {
    Statistics statistics;
    StatisticsSnapshot snapshot;

    ASSERT_EQ_LONG(APP_ERROR, statistics_init(NULL));
    ASSERT_EQ_LONG(APP_SUCCESS, statistics_init(&statistics));
    ASSERT_EQ_LONG(APP_ERROR, statistics_record_produced(NULL, 0));
    ASSERT_EQ_LONG(APP_ERROR, statistics_record_consumed(NULL, 0));
    ASSERT_EQ_LONG(APP_ERROR, statistics_snapshot(NULL, &snapshot));
    ASSERT_EQ_LONG(APP_ERROR, statistics_snapshot(&statistics, NULL));
    statistics_record_produced(&statistics, 0);
    statistics_record_produced(&statistics, 1);
    statistics_record_consumed(&statistics, 1);
    statistics_snapshot(&statistics, &snapshot);

    ASSERT_EQ_LONG(2, snapshot.total_produced);
    ASSERT_EQ_LONG(1, snapshot.total_consumed);
    ASSERT_EQ_LONG(1, snapshot.producer_wait_count);
    ASSERT_EQ_LONG(1, snapshot.consumer_wait_count);

    statistics_destroy(&statistics);
    statistics_destroy(NULL);
}
