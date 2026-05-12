#include "test_framework.h"

// Forward declarations of test suites
void run_input_tests();
void run_lua_comms_tests();

int main() {
    fprintf(stderr, "VRMOD Dev Build - Test Suite\n");
    fprintf(stderr, "============================\n");

    run_input_tests();
    run_lua_comms_tests();

    TEST_SUMMARY();
}
