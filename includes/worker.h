#ifndef WORKER_H
#define WORKER_H

#include "bounded_buffer.h"
#include "simulation_config.h"
#include "statistics.h"

typedef struct {
    int id;
    BoundedBuffer *buffer;
    Statistics *statistics;
    const SimulationConfig *config;
} ProducerArgs;

typedef struct {
    int id;
    BoundedBuffer *buffer;
    Statistics *statistics;
    const SimulationConfig *config;
} ConsumerArgs;

void *producer_run(void *argument);
void *consumer_run(void *argument);

#endif
