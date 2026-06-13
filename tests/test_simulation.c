#include "common.h"
#include "scenario.h"
#include "simulation.h"
#include "test_support.h"
#include "worker.h"

static SimulationConfig make_config(
    int capacity,
    int producers,
    int consumers,
    int items
) {
    SimulationConfig config = {
        .buffer_capacity = capacity,
        .producer_count = producers,
        .consumer_count = consumers,
        .items_per_producer = items,
        .producer_delay_min_ms = 0,
        .producer_delay_max_ms = 0,
        .consumer_delay_min_ms = 0,
        .consumer_delay_max_ms = 0,
        .log_mode = LOG_MODE_SUMMARY
    };
    return config;
}

void test_simulation(void) {
    int scenario_type;
    SimulationConfig invalid = make_config(0, 1, 1, 1);
    ASSERT_TRUE(producer_run(NULL) != NULL);
    ASSERT_TRUE(consumer_run(NULL) != NULL);
    ASSERT_EQ_LONG(APP_ERROR, simulation_run(NULL));
    ASSERT_EQ_LONG(APP_ERROR, simulation_run(&invalid));

    SimulationConfig single_slot = make_config(1, 2, 2, 25);
    ASSERT_EQ_LONG(APP_SUCCESS, simulation_run(&single_slot));

    for (scenario_type = SCENARIO_BALANCED;
         scenario_type <= SCENARIO_FAST_CONSUMER;
         scenario_type++) {
        SimulationConfig scenario = scenario_create(
            (ScenarioType)scenario_type
        );
        scenario.items_per_producer = 5;
        scenario.log_mode = LOG_MODE_SUMMARY;
        ASSERT_EQ_LONG(APP_SUCCESS, simulation_run(&scenario));
    }

    SimulationConfig stress = make_config(8, 10, 10, 1000);
    ASSERT_EQ_LONG(APP_SUCCESS, simulation_run(&stress));
}
