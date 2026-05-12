#include "test_framework.h"
#include "input/vr_input.h"
#include "common/vrmod_types.h"
#include <cmath>

//---------------------------------------------------------------------------
// ExtractPoseData tests -- mock VR pose matrices, verify game coordinates
//---------------------------------------------------------------------------

void test_extract_pose_identity() {
    // Identity-like matrix: no rotation, position at origin
    float mat[3][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0}
    };
    float vel[3] = {0, 0, 0};
    float angvel[3] = {0, 0, 0};

    PoseResult r = ExtractPoseData(mat, vel, angvel);

    // pos: x = -mat[2][3], y = -mat[0][3], z = mat[1][3]
    TEST_ASSERT_FLOAT_EQ(r.pos[0], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.pos[1], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.pos[2], 0.0f);

    // angles from identity should be 0
    TEST_ASSERT_FLOAT_EQ(r.ang[0], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.ang[1], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.ang[2], 0.0f);

    TEST_ASSERT_FLOAT_EQ(r.vel[0], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.vel[1], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.vel[2], 0.0f);

    TEST_ASSERT_FLOAT_EQ(r.angvel[0], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.angvel[1], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.angvel[2], 0.0f);
}

void test_extract_pose_translation() {
    // Position at (1, 2, 3) in VR space
    float mat[3][4] = {
        {1, 0, 0, 2.0f},   // mat[0][3] = 2
        {0, 1, 0, 3.0f},   // mat[1][3] = 3
        {0, 0, 1, 1.0f}    // mat[2][3] = 1
    };
    float vel[3] = {0, 0, 0};
    float angvel[3] = {0, 0, 0};

    PoseResult r = ExtractPoseData(mat, vel, angvel);

    // Game coords: x = -mat[2][3] = -1, y = -mat[0][3] = -2, z = mat[1][3] = 3
    TEST_ASSERT_FLOAT_EQ(r.pos[0], -1.0f);
    TEST_ASSERT_FLOAT_EQ(r.pos[1], -2.0f);
    TEST_ASSERT_FLOAT_EQ(r.pos[2],  3.0f);
}

void test_extract_pose_velocity() {
    float mat[3][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    // VR velocity (1.0, 2.0, 3.0)
    float vel[3] = {1.0f, 2.0f, 3.0f};
    float angvel[3] = {0, 0, 0};

    PoseResult r = ExtractPoseData(mat, vel, angvel);

    // Game velocity: x = -vel[2] = -3, y = -vel[0] = -1, z = vel[1] = 2
    TEST_ASSERT_FLOAT_EQ(r.vel[0], -3.0f);
    TEST_ASSERT_FLOAT_EQ(r.vel[1], -1.0f);
    TEST_ASSERT_FLOAT_EQ(r.vel[2],  2.0f);
}

void test_extract_pose_angular_velocity() {
    float mat[3][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    float vel[3] = {0, 0, 0};
    // VR angular velocity in radians/s: (PI, 0, 0) around X
    float angvel[3] = {PI_F, 0.0f, 0.0f};

    PoseResult r = ExtractPoseData(mat, vel, angvel);

    // angvel game: x = -angvel[2]*deg, y = -angvel[0]*deg, z = angvel[1]*deg
    TEST_ASSERT_FLOAT_EQ(r.angvel[0], 0.0f);
    TEST_ASSERT_FLOAT_EQ(r.angvel[1], -180.0f);
    TEST_ASSERT_FLOAT_EQ(r.angvel[2], 0.0f);
}

void test_extract_pose_90deg_yaw() {
    // 90 degree rotation around VR Y-axis
    // This means mat[0][2] = 1, mat[2][2] = 0, mat[1][2] = 0
    float mat[3][4] = {
        {0, 0, 1, 0},  // row 0
        {0, 1, 0, 0},  // row 1
        {-1,0, 0, 0}   // row 2
    };
    float vel[3] = {0, 0, 0};
    float angvel[3] = {0, 0, 0};

    PoseResult r = ExtractPoseData(mat, vel, angvel);

    // pitch = asin(mat[1][2]) = asin(0) = 0
    TEST_ASSERT_FLOAT_EQ(r.ang[0], 0.0f);
    // yaw = atan2(mat[0][2], mat[2][2]) = atan2(1, 0) = 90
    TEST_ASSERT_FLOAT_EQ(r.ang[1], 90.0f);
    // roll = atan2(-mat[1][0], mat[1][1]) = atan2(0, 1) = 0
    TEST_ASSERT_FLOAT_EQ(r.ang[2], 0.0f);
}

//---------------------------------------------------------------------------
// ComputeActionTypeFromString tests -- mock VR action type strings
//---------------------------------------------------------------------------

void test_action_type_boolean() {
    TEST_ASSERT(ComputeActionTypeFromString("boolean") == ActionType_Boolean);
}

void test_action_type_vector1() {
    TEST_ASSERT(ComputeActionTypeFromString("vector1") == ActionType_Vector1);
}

void test_action_type_vector2() {
    TEST_ASSERT(ComputeActionTypeFromString("vector2") == ActionType_Vector2);
}

void test_action_type_skeleton() {
    TEST_ASSERT(ComputeActionTypeFromString("skeleton") == ActionType_Skeleton);
}

void test_action_type_vibration() {
    TEST_ASSERT(ComputeActionTypeFromString("vibration") == ActionType_Vibration);
}

void test_action_type_pose() {
    TEST_ASSERT(ComputeActionTypeFromString("pose") == ActionType_Pose);
}

void test_action_type_empty() {
    TEST_ASSERT(ComputeActionTypeFromString("") == 0);
}

void test_action_type_deterministic() {
    // Same input always produces same output
    int a = ComputeActionTypeFromString("boolean");
    int b = ComputeActionTypeFromString("boolean");
    TEST_ASSERT(a == b);
}

//---------------------------------------------------------------------------
// Registration function
//---------------------------------------------------------------------------

void run_input_tests() {
    TEST_SUITE("Input: Pose Extraction (mocked VR data)");
    RUN_TEST(test_extract_pose_identity);
    RUN_TEST(test_extract_pose_translation);
    RUN_TEST(test_extract_pose_velocity);
    RUN_TEST(test_extract_pose_angular_velocity);
    RUN_TEST(test_extract_pose_90deg_yaw);

    TEST_SUITE("Input: Action Type Parsing (mocked VR action types)");
    RUN_TEST(test_action_type_boolean);
    RUN_TEST(test_action_type_vector1);
    RUN_TEST(test_action_type_vector2);
    RUN_TEST(test_action_type_skeleton);
    RUN_TEST(test_action_type_vibration);
    RUN_TEST(test_action_type_pose);
    RUN_TEST(test_action_type_empty);
    RUN_TEST(test_action_type_deterministic);
}
