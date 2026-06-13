#include <stdio.h>

#include "scenario.h"

static SimulationConfig make_config(
    int producer_delay_min_ms,
    int producer_delay_max_ms,
    int consumer_delay_min_ms,
    int consumer_delay_max_ms
) {
    SimulationConfig config = {
        .buffer_capacity = 5,
        .producer_count = 2,
        .consumer_count = 2,
        .items_per_producer = 10,
        .producer_delay_min_ms = producer_delay_min_ms,
        .producer_delay_max_ms = producer_delay_max_ms,
        .consumer_delay_min_ms = consumer_delay_min_ms,
        .consumer_delay_max_ms = consumer_delay_max_ms,
        .log_mode = LOG_MODE_DETAILED
    };
    return config;
}

SimulationConfig scenario_create(ScenarioType type) {
    switch (type) {
        case SCENARIO_BALANCED:
            return make_config(200, 400, 200, 400);
        case SCENARIO_FAST_PRODUCER:
            return make_config(50, 100, 400, 600);
        case SCENARIO_FAST_CONSUMER:
            return make_config(400, 600, 50, 100);
        default: {
            SimulationConfig invalid = {0};
            return invalid;
        }
    }
}

const char *scenario_get_name(ScenarioType type) {
    switch (type) {
        case SCENARIO_BALANCED:
            return "Balanced";
        case SCENARIO_FAST_PRODUCER:
            return "Fast Producer";
        case SCENARIO_FAST_CONSUMER:
            return "Fast Consumer";
        default:
            return "Unknown";
    }
}

void scenario_print(ScenarioType type, const SimulationConfig *config) {
    if (config == NULL) {
        return;
    }

    printf("\nScenario: %s\n", scenario_get_name(type));
    printf("----------------------------------------\n");
    printf("Buffer capacity:      %d\n", config->buffer_capacity);
    printf("Number of producers:  %d\n", config->producer_count);
    printf("Number of consumers:  %d\n", config->consumer_count);
    printf("Items per producer:   %d\n", config->items_per_producer);
    printf("Producer delay:       %d-%d ms\n",
           config->producer_delay_min_ms,
           config->producer_delay_max_ms);
    printf("Consumer delay:       %d-%d ms\n",
           config->consumer_delay_min_ms,
           config->consumer_delay_max_ms);
    printf("Logging mode:         %s\n",
           config->log_mode == LOG_MODE_DETAILED ? "Detailed" : "Summary");
}
