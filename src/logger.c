#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "logger.h"

static pthread_mutex_t logger_mutex;
static struct timespec logger_start;
static LogMode logger_mode = LOG_MODE_SUMMARY;
static int logger_initialized = 0;

static long elapsed_milliseconds(struct timespec now) {
    long seconds = now.tv_sec - logger_start.tv_sec;
    long nanoseconds = now.tv_nsec - logger_start.tv_nsec;
    return seconds * 1000L + nanoseconds / 1000000L;
}

int logger_init(LogMode mode) {
    if (mode != LOG_MODE_SUMMARY && mode != LOG_MODE_DETAILED) {
        return APP_ERROR;
    }
    if (pthread_mutex_init(&logger_mutex, NULL) != 0) {
        return APP_ERROR;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &logger_start) != 0) {
        (void)pthread_mutex_destroy(&logger_mutex);
        return APP_ERROR;
    }

    logger_mode = mode;
    logger_initialized = 1;
    return APP_SUCCESS;
}

void logger_destroy(void) {
    if (logger_initialized != 0) {
        logger_initialized = 0;
        (void)pthread_mutex_destroy(&logger_mutex);
    }
}

void logger_event(const char *component, const char *format, ...) {
    struct timespec now;
    va_list arguments;

    if (logger_initialized == 0 || logger_mode != LOG_MODE_DETAILED
        || component == NULL || format == NULL) {
        return;
    }
    if (pthread_mutex_lock(&logger_mutex) != 0) {
        return;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        (void)pthread_mutex_unlock(&logger_mutex);
        return;
    }

    printf("[%06ld ms] [%-12s] ", elapsed_milliseconds(now), component);
    va_start(arguments, format);
    (void)vprintf(format, arguments);
    va_end(arguments);
    putchar('\n');
    fflush(stdout);

    (void)pthread_mutex_unlock(&logger_mutex);
}
