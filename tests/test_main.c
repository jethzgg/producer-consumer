#include <stdio.h>

#include "test_support.h"

void test_scenarios(void);
void test_bounded_buffer(void);
void test_statistics(void);
void test_logger(void);
void test_simulation(void);

int main(void) {
    run_test("scenarios", test_scenarios);
    run_test("bounded buffer", test_bounded_buffer);
    run_test("statistics", test_statistics);
    run_test("logger", test_logger);
    run_test("simulation", test_simulation);
    puts("All tests passed.");
    return 0;
}
