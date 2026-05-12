#include "test_framework.h"
#include "vrmod/logging.h"

int g_testsPassed = 0;
int g_testsFailed = 0;
int g_currentTestFailed = 0;

extern void run_input_tests();
extern void run_lua_tests();

int main() {
    printf("========================================\n");
    printf("  VRMOD Dev Build - Test Suite\n");
    printf("========================================\n");

    vrmod_log_init("vrmod_test.log");

    run_input_tests();
    run_lua_tests();

    vrmod_log_shutdown();

    return test_report();
}
