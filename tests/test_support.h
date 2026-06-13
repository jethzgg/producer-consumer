#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>

#define ASSERT_TRUE(condition)                                                \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "%s:%d: assertion failed: %s\n",                 \
                    __FILE__, __LINE__, #condition);                          \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

#define ASSERT_EQ_LONG(expected, actual)                                      \
    do {                                                                      \
        long expected_value = (long)(expected);                               \
        long actual_value = (long)(actual);                                   \
        if (expected_value != actual_value) {                                 \
            fprintf(stderr,                                                   \
                    "%s:%d: expected %ld, got %ld\n",                        \
                    __FILE__, __LINE__, expected_value, actual_value);         \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

typedef void (*TestFunction)(void);

static inline void run_test(const char *name, TestFunction function) {
    printf("[TEST] %s\n", name);
    function();
}

#endif
