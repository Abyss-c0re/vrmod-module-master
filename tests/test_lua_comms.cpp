#include "test_framework.h"
#include "mocks/mock_lua.h"
#include "lua/lua_interface.h"
#include <cstring>

//---------------------------------------------------------------------------
// PushMatrixAsTable tests -- verify Lua stack operations
//---------------------------------------------------------------------------

void test_push_matrix_1x1() {
    MockLuaBase mock;
    float mtx[1] = {42.0f};

    PushMatrixAsTable(&mock, mtx, 1, 1);

    // Expected sequence: CreateTable (outer), PushNumber(1), CreateTable (row),
    // PushNumber(1), PushNumber(42), SetTable(-3), SetTable(-3)
    TEST_ASSERT(mock.countCalls("CreateTable") == 2);
    TEST_ASSERT(mock.countCalls("SetTable") == 2);
    // 3 PushNumber calls: row index (1), col index (1), value (42)
    TEST_ASSERT(mock.countCalls("PushNumber") == 3);
    TEST_ASSERT_FLOAT_EQ(mock.pushed_numbers[2], 42.0f);
}

void test_push_matrix_2x2() {
    MockLuaBase mock;
    float mtx[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    PushMatrixAsTable(&mock, mtx, 2, 2);

    // 1 outer table + 2 row tables = 3 CreateTable
    TEST_ASSERT(mock.countCalls("CreateTable") == 3);
    // 2 row-level SetTable + 4 cell-level SetTable = 6
    TEST_ASSERT(mock.countCalls("SetTable") == 6);
    // 2 row indices + 2*2 col indices + 4 values = 10 PushNumber
    TEST_ASSERT(mock.countCalls("PushNumber") == 10);
}

void test_push_matrix_3x4() {
    MockLuaBase mock;
    float mtx[12] = {0};

    PushMatrixAsTable(&mock, mtx, 3, 4);

    // 1 outer + 3 rows = 4 CreateTable
    TEST_ASSERT(mock.countCalls("CreateTable") == 4);
    // 3 row SetTable + 12 cell SetTable = 15
    TEST_ASSERT(mock.countCalls("SetTable") == 15);
    // 3 row indices + 3*4 col indices + 12 values = 27 PushNumber
    TEST_ASSERT(mock.countCalls("PushNumber") == 27);
}

void test_push_matrix_values_order() {
    MockLuaBase mock;
    float mtx[4] = {10.0f, 20.0f, 30.0f, 40.0f};

    PushMatrixAsTable(&mock, mtx, 2, 2);

    // Values pushed (among index pushes):
    // row1: idx=1, {col_idx=1, val=10, col_idx=2, val=20}, row_set
    // row2: idx=2, {col_idx=1, val=30, col_idx=2, val=40}, row_set
    // pushed_numbers order: 1, 1, 10, 2, 20, 2, 1, 30, 2, 40
    TEST_ASSERT(mock.pushed_numbers.size() == 10);
    TEST_ASSERT_FLOAT_EQ(mock.pushed_numbers[0], 1.0);  // row 1 index
    TEST_ASSERT_FLOAT_EQ(mock.pushed_numbers[2], 10.0);  // first value
    TEST_ASSERT_FLOAT_EQ(mock.pushed_numbers[4], 20.0);  // second value
    TEST_ASSERT_FLOAT_EQ(mock.pushed_numbers[7], 30.0);  // third value
    TEST_ASSERT_FLOAT_EQ(mock.pushed_numbers[9], 40.0);  // fourth value
}

//---------------------------------------------------------------------------
// LuaPrint tests -- verify Lua communication pattern
//---------------------------------------------------------------------------

void test_lua_print_call_sequence() {
    MockLuaBase mock;

    LuaPrint(&mock, "Hello VR");

    // Should call: PushSpecial, GetField("print"), PushString("Hello VR"), Call, Pop
    TEST_ASSERT(mock.countCalls("PushSpecial") == 1);
    TEST_ASSERT(mock.countCalls("GetField") == 1);
    TEST_ASSERT(mock.countCalls("PushString") == 1);
    TEST_ASSERT(mock.countCalls("Call") == 1);
    TEST_ASSERT(mock.countCalls("Pop") == 1);

    // Verify the string pushed
    TEST_ASSERT(mock.pushed_strings.size() == 1);
    TEST_ASSERT_STR_EQ(mock.pushed_strings[0].c_str(), "Hello VR");
}

void test_lua_print_empty_message() {
    MockLuaBase mock;

    LuaPrint(&mock, "");

    TEST_ASSERT(mock.countCalls("PushString") == 1);
    TEST_ASSERT_STR_EQ(mock.pushed_strings[0].c_str(), "");
}

void test_lua_print_get_field_targets_print() {
    MockLuaBase mock;

    LuaPrint(&mock, "test");

    // Verify GetField was called with "print"
    bool found = false;
    for (auto& call : mock.calls) {
        if (call.method == "GetField" && call.detail == "print")
            found = true;
    }
    TEST_ASSERT(found);
}

//---------------------------------------------------------------------------
// Registration function
//---------------------------------------------------------------------------

void run_lua_comms_tests() {
    TEST_SUITE("Lua Comms: PushMatrixAsTable");
    RUN_TEST(test_push_matrix_1x1);
    RUN_TEST(test_push_matrix_2x2);
    RUN_TEST(test_push_matrix_3x4);
    RUN_TEST(test_push_matrix_values_order);

    TEST_SUITE("Lua Comms: LuaPrint");
    RUN_TEST(test_lua_print_call_sequence);
    RUN_TEST(test_lua_print_empty_message);
    RUN_TEST(test_lua_print_get_field_targets_print);
}
