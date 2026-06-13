#include <string.h>

#include "scenario.h"
#include "test_support.h"

static void assert_config(
    SimulationConfig config,
    int producer_min,
    int producer_max,
    int consumer_min,
    int consumer_max
) {
    ASSERT_EQ_LONG(5, config.buffer_capacity);
    ASSERT_EQ_LONG(2, config.producer_count);
    ASSERT_EQ_LONG(2, config.consumer_count);
    ASSERT_EQ_LONG(10, config.items_per_producer);
    ASSERT_EQ_LONG(producer_min, config.producer_delay_min_ms);
    ASSERT_EQ_LONG(producer_max, config.producer_delay_max_ms);
    ASSERT_EQ_LONG(consumer_min, config.consumer_delay_min_ms);
    ASSERT_EQ_LONG(consumer_max, config.consumer_delay_max_ms);
    ASSERT_EQ_LONG(LOG_MODE_DETAILED, config.log_mode);
}

void test_scenarios(void) {
    SimulationConfig invalid;

    assert_config(scenario_create(SCENARIO_BALANCED), 200, 400, 200, 400);
    assert_config(scenario_create(SCENARIO_FAST_PRODUCER), 50, 100, 400, 600);
    assert_config(scenario_create(SCENARIO_FAST_CONSUMER), 400, 600, 50, 100);

    ASSERT_TRUE(strcmp(scenario_get_name(SCENARIO_BALANCED), "Balanced") == 0);
    ASSERT_TRUE(strcmp(scenario_get_name(SCENARIO_FAST_PRODUCER),
                       "Fast Producer") == 0);
    ASSERT_TRUE(strcmp(scenario_get_name(SCENARIO_FAST_CONSUMER),
                       "Fast Consumer") == 0);
    ASSERT_TRUE(strcmp(scenario_get_name((ScenarioType)99), "Unknown") == 0);

    invalid = scenario_create((ScenarioType)99);
    ASSERT_EQ_LONG(0, invalid.buffer_capacity);
    scenario_print(SCENARIO_BALANCED, NULL);
}
