#pragma once
#include <cstdio>
#include <cstring>
#include <cmath>

extern int g_testsPassed;
extern int g_testsFailed;
extern int g_currentTestFailed;

#define BEGIN_TEST(name) \
    do { \
        printf("  [TEST] %-44s ", #name); \
        g_currentTestFailed = 0;

#define END_TEST() \
        if (!g_currentTestFailed) { printf("PASS\n"); g_testsPassed++; } \
    } while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        if (!g_currentTestFailed) printf("FAIL\n"); \
        printf("    Assert: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        g_testsFailed++; g_currentTestFailed = 1; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        if (!g_currentTestFailed) printf("FAIL\n"); \
        printf("    %s != %s at %s:%d\n", #a, #b, __FILE__, __LINE__); \
        g_testsFailed++; g_currentTestFailed = 1; \
    } \
} while(0)

#define ASSERT_STREQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        if (!g_currentTestFailed) printf("FAIL\n"); \
        printf("    \"%s\" != \"%s\" at %s:%d\n", (a), (b), __FILE__, __LINE__); \
        g_testsFailed++; g_currentTestFailed = 1; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, eps) do { \
    if (fabs((double)(a) - (double)(b)) > (eps)) { \
        if (!g_currentTestFailed) printf("FAIL\n"); \
        printf("    |%g - %g| > %g at %s:%d\n", (double)(a), (double)(b), (double)(eps), __FILE__, __LINE__); \
        g_testsFailed++; g_currentTestFailed = 1; \
    } \
} while(0)

inline int test_report() {
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");
    return g_testsFailed > 0 ? 1 : 0;
}
