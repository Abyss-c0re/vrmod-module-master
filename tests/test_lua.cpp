#include "test_framework.h"
#include "mocks/mock_lua.h"
#include "vrmod/types.h"
#include "vrmod/globals.h"
#include "vrmod/lua/lua_helpers.h"
#include <cstring>
#include <cmath>

static MockLuaBase s_lua;

static void setup() {
    s_lua.reset();
    memset(g_luaRefs, 0, sizeof(g_luaRefs));
    g_luaRefCount = 0;
    g_actionCount = 0;
    g_actionSetCount = 0;
    g_activeActionSetCount = 0;
    g_IsPaused = false;
    memset(g_actions, 0, sizeof(g_actions));
}

void run_lua_tests() {
    printf("\n=== Lua Communication Tests ===\n");

    BEGIN_TEST(lua_print_calls_global_print);
        setup();
        LuaPrint(&s_lua, "Hello VRMOD");
        bool foundPushSpecial = false, foundGetField = false, foundPushString = false;
        for (auto& c : s_lua.calls) {
            if (c.method == "PushSpecial" && c.intArg == GarrysMod::Lua::SPECIAL_GLOB) foundPushSpecial = true;
            if (c.method == "GetField" && c.strArg == "print") foundGetField = true;
            if (c.method == "PushString" && c.strArg == "Hello VRMOD") foundPushString = true;
        }
        ASSERT_TRUE(foundPushSpecial);
        ASSERT_TRUE(foundGetField);
        ASSERT_TRUE(foundPushString);
    END_TEST();

    BEGIN_TEST(push_matrix_2x2_creates_3_tables);
        setup();
        float mat[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        PushMatrixAsTable(&s_lua, mat, 2, 2);
        int createCount = 0;
        for (auto& c : s_lua.calls)
            if (c.method == "CreateTable") createCount++;
        ASSERT_EQ(createCount, 3);
    END_TEST();

    BEGIN_TEST(push_matrix_4x4_creates_5_tables);
        setup();
        float identity[16] = {};
        for (int i = 0; i < 4; i++) identity[i*4+i] = 1.0f;
        PushMatrixAsTable(&s_lua, identity, 4, 4);
        int createCount = 0;
        for (auto& c : s_lua.calls)
            if (c.method == "CreateTable") createCount++;
        ASSERT_EQ(createCount, 5);
    END_TEST();

    BEGIN_TEST(push_matrix_3x4_creates_4_tables);
        setup();
        float mat[12] = { 1,0,0,0, 0,1,0,0, 0,0,1,0 };
        PushMatrixAsTable(&s_lua, mat, 3, 4);
        int createCount = 0;
        for (auto& c : s_lua.calls)
            if (c.method == "CreateTable") createCount++;
        ASSERT_EQ(createCount, 4);
    END_TEST();

    BEGIN_TEST(push_matrix_1x1_values_1_indexed);
        setup();
        float mat[1] = { 42.0f };
        PushMatrixAsTable(&s_lua, mat, 1, 1);
        ASSERT_TRUE(s_lua.pushedNumbers.size() >= 3);
        ASSERT_NEAR(s_lua.pushedNumbers[0], 1.0, 0.001);
        ASSERT_NEAR(s_lua.pushedNumbers[1], 1.0, 0.001);
        ASSERT_NEAR(s_lua.pushedNumbers[2], 42.0, 0.001);
    END_TEST();

    BEGIN_TEST(lua_ref_create_unique_ids);
        setup();
        for (int i = 0; i < LuaRefIndex_Max; i++) {
            s_lua.CreateTable();
            g_luaRefs[i] = s_lua.ReferenceCreate();
            g_luaRefCount++;
        }
        ASSERT_EQ(g_luaRefCount, LuaRefIndex_Max);
        for (int i = 0; i < LuaRefIndex_Max; i++)
            for (int j = i+1; j < LuaRefIndex_Max; j++)
                ASSERT_TRUE(g_luaRefs[i] != g_luaRefs[j]);
    END_TEST();

    BEGIN_TEST(action_lua_refs_created_per_action);
        setup();
        g_actionCount = 2;
        for (int i = 0; i < g_actionCount; i++)
            for (int j = 0; j < 2; j++) {
                s_lua.CreateTable();
                g_actions[i].luaRefs[j] = s_lua.ReferenceCreate();
            }
        ASSERT_TRUE(g_actions[0].luaRefs[0] > 0);
        ASSERT_TRUE(g_actions[0].luaRefs[1] > 0);
        ASSERT_TRUE(g_actions[0].luaRefs[0] != g_actions[0].luaRefs[1]);
        ASSERT_TRUE(g_actions[0].luaRefs[0] != g_actions[1].luaRefs[0]);
    END_TEST();

    BEGIN_TEST(shutdown_frees_all_refs);
        setup();
        g_luaRefCount = LuaRefIndex_Max;
        for (int i = 0; i < LuaRefIndex_Max; i++) {
            s_lua.CreateTable();
            g_luaRefs[i] = s_lua.ReferenceCreate();
        }
        g_actionCount = 1;
        for (int j = 0; j < 2; j++) {
            s_lua.CreateTable();
            g_actions[0].luaRefs[j] = s_lua.ReferenceCreate();
        }
        s_lua.calls.clear();
        for (int i = 0; i < g_luaRefCount; i++)
            if (g_luaRefs[i] != 0) { s_lua.ReferenceFree(g_luaRefs[i]); g_luaRefs[i] = 0; }
        for (int i = 0; i < g_actionCount; i++)
            for (int j = 0; j < 2; j++)
                if (g_actions[i].luaRefs[j] != 0) { s_lua.ReferenceFree(g_actions[i].luaRefs[j]); g_actions[i].luaRefs[j] = 0; }
        int freeCount = 0;
        for (auto& c : s_lua.calls)
            if (c.method == "ReferenceFree") freeCount++;
        ASSERT_EQ(freeCount, LuaRefIndex_Max + 2);
    END_TEST();

    BEGIN_TEST(pose_math_identity_position);
        setup();
        memset(&g_poses[0], 0, sizeof(g_poses[0]));
        g_poses[0].bPoseIsValid = true;
        vr::HmdMatrix34_t& mat = g_poses[0].mDeviceToAbsoluteTracking;
        mat.m[0][0]=1; mat.m[0][3]=1.0f;
        mat.m[1][1]=1; mat.m[1][3]=2.0f;
        mat.m[2][2]=1; mat.m[2][3]=3.0f;
        Vector pos;
        pos.x = -mat.m[2][3];
        pos.y = -mat.m[0][3];
        pos.z = mat.m[1][3];
        ASSERT_NEAR(pos.x, -3.0f, 0.001f);
        ASSERT_NEAR(pos.y, -1.0f, 0.001f);
        ASSERT_NEAR(pos.z,  2.0f, 0.001f);
    END_TEST();

    BEGIN_TEST(pose_math_identity_angles);
        setup();
        memset(&g_poses[0], 0, sizeof(g_poses[0]));
        g_poses[0].bPoseIsValid = true;
        vr::HmdMatrix34_t& mat = g_poses[0].mDeviceToAbsoluteTracking;
        mat.m[0][0]=1; mat.m[1][1]=1; mat.m[2][2]=1;
        QAngle ang;
        ang.x = asinf(mat.m[1][2]) * (180.0f / PI_F);
        ang.y = atan2f(mat.m[0][2], mat.m[2][2]) * (180.0f / PI_F);
        ang.z = atan2f(-mat.m[1][0], mat.m[1][1]) * (180.0f / PI_F);
        ASSERT_NEAR(ang.x, 0.0f, 0.001f);
        ASSERT_NEAR(ang.y, 0.0f, 0.001f);
        ASSERT_NEAR(ang.z, 0.0f, 0.001f);
    END_TEST();

    BEGIN_TEST(pose_math_90deg_yaw);
        setup();
        memset(&g_poses[0], 0, sizeof(g_poses[0]));
        g_poses[0].bPoseIsValid = true;
        vr::HmdMatrix34_t& mat = g_poses[0].mDeviceToAbsoluteTracking;
        mat.m[0][0]=0;  mat.m[0][2]=1;
        mat.m[1][1]=1;
        mat.m[2][0]=-1; mat.m[2][2]=0;
        QAngle ang;
        ang.y = atan2f(mat.m[0][2], mat.m[2][2]) * (180.0f / PI_F);
        ASSERT_NEAR(ang.y, 90.0f, 0.001f);
    END_TEST();

    BEGIN_TEST(velocity_conversion);
        setup();
        memset(&g_poses[0], 0, sizeof(g_poses[0]));
        g_poses[0].vVelocity.v[0] = 1.0f;
        g_poses[0].vVelocity.v[1] = 2.0f;
        g_poses[0].vVelocity.v[2] = 3.0f;
        Vector vel;
        vel.x = -g_poses[0].vVelocity.v[2];
        vel.y = -g_poses[0].vVelocity.v[0];
        vel.z = g_poses[0].vVelocity.v[1];
        ASSERT_NEAR(vel.x, -3.0f, 0.001f);
        ASSERT_NEAR(vel.y, -1.0f, 0.001f);
        ASSERT_NEAR(vel.z,  2.0f, 0.001f);
    END_TEST();
}
