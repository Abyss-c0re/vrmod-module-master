#include "test_framework.h"
#include "mocks/mock_openvr.h"
#include "input/vr_input.h"
#include "core/vrmod_common.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ─── Pose conversion tests ───

TEST(ConvertPose_Identity_Translation) {
    // Mat row0=[1,0,0, tx=2.0], row1=[0,1,0, ty=3.0], row2=[0,0,1, tz=5.0]
    // Expected: pos.x = -tz = -5.0, pos.y = -tx = -2.0, pos.z = ty = 3.0
    auto pose = mock::MakePose(2.0f, 3.0f, 5.0f,  0,0,0,  0,0,0);
    PoseResult r = ConvertPose(pose);
    ASSERT_TRUE(r.valid);
    ASSERT_NEAR(r.pos[0], -5.0f, 0.001f);
    ASSERT_NEAR(r.pos[1], -2.0f, 0.001f);
    ASSERT_NEAR(r.pos[2],  3.0f, 0.001f);
}

TEST(ConvertPose_Velocity) {
    // vVelocity = (vx=1.0, vy=2.0, vz=3.0)
    // Expected: vel.x = -vz = -3.0, vel.y = -vx = -1.0, vel.z = vy = 2.0
    auto pose = mock::MakePose(0,0,0,  1.0f, 2.0f, 3.0f,  0,0,0);
    PoseResult r = ConvertPose(pose);
    ASSERT_TRUE(r.valid);
    ASSERT_NEAR(r.vel[0], -3.0f, 0.001f);
    ASSERT_NEAR(r.vel[1], -1.0f, 0.001f);
    ASSERT_NEAR(r.vel[2],  2.0f, 0.001f);
}

TEST(ConvertPose_AngularVelocity) {
    // vAngularVelocity = (avx=0.5, avy=1.0, avz=1.5) in rad/s
    // Expected: angvel.x = -avz * (180/PI), angvel.y = -avx * (180/PI), angvel.z = avy * (180/PI)
    auto pose = mock::MakePose(0,0,0, 0,0,0,  0.5f, 1.0f, 1.5f);
    PoseResult r = ConvertPose(pose);
    ASSERT_TRUE(r.valid);
    float toDeg = 180.0f / PI_F;
    ASSERT_NEAR(r.angvel[0], -1.5f * toDeg, 0.01f);
    ASSERT_NEAR(r.angvel[1], -0.5f * toDeg, 0.01f);
    ASSERT_NEAR(r.angvel[2],  1.0f * toDeg, 0.01f);
}

TEST(ConvertPose_Identity_Angles) {
    // Identity rotation matrix should give all-zero angles
    auto pose = mock::MakePose(0,0,0, 0,0,0, 0,0,0);
    PoseResult r = ConvertPose(pose);
    ASSERT_TRUE(r.valid);
    ASSERT_NEAR(r.ang[0], 0.0f, 0.001f);
    ASSERT_NEAR(r.ang[1], 0.0f, 0.001f);
    ASSERT_NEAR(r.ang[2], 0.0f, 0.001f);
}

TEST(ConvertPose_Invalid) {
    auto pose = mock::MakePose(1,2,3, 4,5,6, 7,8,9, false);
    PoseResult r = ConvertPose(pose);
    ASSERT_FALSE(r.valid);
}

// ─── Action manifest parsing tests ───

TEST(ParseActionManifest_ValidFile) {
    // Create a temporary action manifest file
    const char* tmpPath = "/tmp/vrmod_test_actions.json";
    FILE* f = fopen(tmpPath, "w");
    ASSERT_TRUE(f != nullptr);
    fprintf(f, "{\n");
    fprintf(f, "  \"actions\": [\n");
    fprintf(f, "    { \"name\": \"/actions/main/in/trigger\", \"type\": \"boolean\" },\n");
    fprintf(f, "    { \"name\": \"/actions/main/in/trackpad\", \"type\": \"vector2\" },\n");
    fprintf(f, "    { \"name\": \"/actions/main/in/hand_left\", \"type\": \"pose\" }\n");
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    mock::MockVRInput mockInput;
    action actions[MAX_ACTIONS];
    int count = ParseActionManifest(tmpPath, actions, MAX_ACTIONS, &mockInput);

    ASSERT_EQ(count, 3);

    // Verify names were parsed correctly (name points to after last '/')
    ASSERT_STREQ(actions[0].name, "trigger");
    ASSERT_STREQ(actions[1].name, "trackpad");
    ASSERT_STREQ(actions[2].name, "hand_left");

    // Verify fullnames
    ASSERT_STREQ(actions[0].fullname, "/actions/main/in/trigger");

    // Verify types are non-zero (sum of chars in type string)
    ASSERT_TRUE(actions[0].type > 0);
    ASSERT_TRUE(actions[1].type > 0);
    ASSERT_TRUE(actions[2].type > 0);

    // Verify handles were requested from mock
    ASSERT_EQ((int)mockInput.actionHandles.size(), 3);

    remove(tmpPath);
}

TEST(ParseActionManifest_EmptyFile) {
    const char* tmpPath = "/tmp/vrmod_test_empty.json";
    FILE* f = fopen(tmpPath, "w");
    fprintf(f, "{}\n");
    fclose(f);

    mock::MockVRInput mockInput;
    action actions[MAX_ACTIONS];
    int count = ParseActionManifest(tmpPath, actions, MAX_ACTIONS, &mockInput);
    ASSERT_EQ(count, 0);

    remove(tmpPath);
}

TEST(ParseActionManifest_ManifestPathError) {
    mock::MockVRInput mockInput;
    mockInput.manifestError = vr::VRInputError_InvalidParam;

    action actions[MAX_ACTIONS];
    int count = ParseActionManifest("/tmp/nonexistent_for_test.json", actions, MAX_ACTIONS, &mockInput);
    ASSERT_EQ(count, -1);
}

TEST(ParseActionManifest_FileNotFound) {
    mock::MockVRInput mockInput;
    action actions[MAX_ACTIONS];
    int count = ParseActionManifest("/tmp/this_file_does_not_exist_12345.json", actions, MAX_ACTIONS, &mockInput);
    ASSERT_EQ(count, -2);
}

// ─── Action set management tests ───

TEST(FindOrCreateActionSet_New) {
    mock::MockVRInput mockInput;
    actionSet sets[MAX_ACTIONSETS];
    memset(sets, 0, sizeof(sets));
    int count = 0;

    int idx = FindOrCreateActionSet("/actions/main", sets, &count, &mockInput);
    ASSERT_EQ(idx, 0);
    ASSERT_EQ(count, 1);
    ASSERT_STREQ(sets[0].name, "/actions/main");
}

TEST(FindOrCreateActionSet_Existing) {
    mock::MockVRInput mockInput;
    actionSet sets[MAX_ACTIONSETS];
    memset(sets, 0, sizeof(sets));
    int count = 0;

    int idx1 = FindOrCreateActionSet("/actions/main", sets, &count, &mockInput);
    int idx2 = FindOrCreateActionSet("/actions/main", sets, &count, &mockInput);
    ASSERT_EQ(idx1, idx2);
    ASSERT_EQ(count, 1);
}

TEST(FindOrCreateActionSet_Multiple) {
    mock::MockVRInput mockInput;
    actionSet sets[MAX_ACTIONSETS];
    memset(sets, 0, sizeof(sets));
    int count = 0;

    int idx1 = FindOrCreateActionSet("/actions/main", sets, &count, &mockInput);
    int idx2 = FindOrCreateActionSet("/actions/driving", sets, &count, &mockInput);
    ASSERT_EQ(idx1, 0);
    ASSERT_EQ(idx2, 1);
    ASSERT_EQ(count, 2);
}

// ─── Haptic lookup tests ───

TEST(FindActionHandleByName_Found) {
    action actions[3];
    memset(actions, 0, sizeof(actions));
    strcpy(actions[0].fullname, "/actions/main/in/haptic_left");
    actions[0].name = actions[0].fullname + 17; // "haptic_left"
    actions[0].handle = 42;
    strcpy(actions[1].fullname, "/actions/main/in/haptic_right");
    actions[1].name = actions[1].fullname + 17; // "haptic_right"
    actions[1].handle = 43;

    vr::VRActionHandle_t h = FindActionHandleByName("haptic_left", actions, 2);
    ASSERT_EQ(h, (vr::VRActionHandle_t)42);
}

TEST(FindActionHandleByName_NotFound) {
    action actions[1];
    memset(actions, 0, sizeof(actions));
    strcpy(actions[0].fullname, "/actions/main/in/trigger");
    actions[0].name = actions[0].fullname + 17;
    actions[0].handle = 10;

    vr::VRActionHandle_t h = FindActionHandleByName("nonexistent", actions, 1);
    ASSERT_EQ(h, vr::k_ulInvalidActionHandle);
}

// ─── Boolean action type hash test ───

TEST(ActionType_BooleanHash) {
    // The original code sums char values of the type string to get the type enum
    // "boolean" should hash to ActionType_Boolean = 736
    const char* typeStr = "boolean";
    int hash = 0;
    for (int i = 0; typeStr[i]; i++) hash += typeStr[i];
    ASSERT_EQ(hash, ActionType_Boolean);
}

TEST(ActionType_PoseHash) {
    const char* typeStr = "pose";
    int hash = 0;
    for (int i = 0; typeStr[i]; i++) hash += typeStr[i];
    ASSERT_EQ(hash, ActionType_Pose);
}

TEST(ActionType_Vector1Hash) {
    const char* typeStr = "vector1";
    int hash = 0;
    for (int i = 0; typeStr[i]; i++) hash += typeStr[i];
    ASSERT_EQ(hash, ActionType_Vector1);
}

TEST(ActionType_Vector2Hash) {
    const char* typeStr = "vector2";
    int hash = 0;
    for (int i = 0; typeStr[i]; i++) hash += typeStr[i];
    ASSERT_EQ(hash, ActionType_Vector2);
}
