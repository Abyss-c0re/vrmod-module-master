#ifndef VRMOD_TEST_FRAMEWORK_H
#define VRMOD_TEST_FRAMEWORK_H

#include <cstdio>
#include <cmath>
#include <cstring>

namespace vrmod_test {
    inline int g_pass = 0;
    inline int g_fail = 0;
    inline int g_test_count = 0;
}

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        vrmod_test::g_fail++; \
    } else { \
        vrmod_test::g_pass++; \
    } \
} while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b) do { \
    float _a = (a), _b = (b); \
    if (fabsf(_a - _b) > 1e-4f) { \
        fprintf(stderr, "    FAIL %s:%d: %.6f != %.6f\n", __FILE__, __LINE__, _a, _b); \
        vrmod_test::g_fail++; \
    } else { \
        vrmod_test::g_pass++; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        fprintf(stderr, "    FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
        vrmod_test::g_fail++; \
    } else { \
        vrmod_test::g_pass++; \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    vrmod_test::g_test_count++; \
    int _before_fail = vrmod_test::g_fail; \
    fprintf(stderr, "  [%d] %s ... ", vrmod_test::g_test_count, #fn); \
    fn(); \
    if (vrmod_test::g_fail == _before_fail) fprintf(stderr, "OK\n"); \
    else fprintf(stderr, "\n"); \
} while(0)

#define TEST_SUITE(name) fprintf(stderr, "\n=== %s ===\n", name)

#define TEST_SUMMARY() do { \
    fprintf(stderr, "\n--- Results: %d passed, %d failed, %d tests ---\n", \
            vrmod_test::g_pass, vrmod_test::g_fail, vrmod_test::g_test_count); \
    return vrmod_test::g_fail > 0 ? 1 : 0; \
} while(0)

#endif
