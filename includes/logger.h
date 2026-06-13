#ifndef LOGGER_H
#define LOGGER_H

#include "simulation_config.h"

int logger_init(LogMode mode);
void logger_destroy(void);
void logger_event(const char *component, const char *format, ...);

#endif
