#ifndef SCENARIO_H
#define SCENARIO_H

#include "simulation_config.h"

typedef enum {
    SCENARIO_BALANCED = 1,
    SCENARIO_FAST_PRODUCER,
    SCENARIO_FAST_CONSUMER
} ScenarioType;

SimulationConfig scenario_create(ScenarioType type);
const char *scenario_get_name(ScenarioType type);
void scenario_print(ScenarioType type, const SimulationConfig *config);

#endif
