#ifndef SIMULATION_CONFIG_H
#define SIMULATION_CONFIG_H

typedef enum {
    LOG_MODE_SUMMARY = 0,
    LOG_MODE_DETAILED = 1
} LogMode;

typedef struct {
    int buffer_capacity;
    int producer_count;
    int consumer_count;
    int items_per_producer;
    int producer_delay_min_ms;
    int producer_delay_max_ms;
    int consumer_delay_min_ms;
    int consumer_delay_max_ms;
    LogMode log_mode;
} SimulationConfig;

#endif
