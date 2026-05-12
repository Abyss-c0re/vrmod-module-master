#include "test_framework.h"
#include "mocks/mock_openvr.h"
#include "vrmod/types.h"
#include "vrmod/globals.h"
#include "vrmod/input/vr_input.h"
#include <cstring>

static vr::MockIVRInput      s_mockInput;
static vr::MockIVRSystem     s_mockSystem;
static vr::MockIVRCompositor s_mockCompositor;

static void setup() {
    g_pInput = &s_mockInput;
    g_pSystem = &s_mockSystem;
    g_compositor = &s_mockCompositor;
    g_actionCount = 0;
    g_actionSetCount = 0;
    g_activeActionSetCount = 0;
    memset(g_actions, 0, sizeof(g_actions));
    memset(g_actionSets, 0, sizeof(g_actionSets));
    s_mockInput.mockHandleCounter = 100;
    s_mockInput.mockSetHandleCounter = 200;
    s_mockInput.hapticTriggered = false;
    s_mockSystem.mockDeviceCount = 0;
}

void run_input_tests() {
    printf("\n=== Input Module Tests ===\n");

    BEGIN_TEST(parse_action_manifest_loads_actions);
        setup();
        char err[256] = {};
        bool ok = VRInput_SetActionManifest("tests/data/test_actions.json", err, sizeof(err));
        ASSERT_TRUE(ok);
        ASSERT_EQ(g_actionCount, 5);
    END_TEST();

    BEGIN_TEST(parse_action_manifest_short_names);
        setup();
        char err[256] = {};
        VRInput_SetActionManifest("tests/data/test_actions.json", err, sizeof(err));
        ASSERT_STREQ(g_actions[0].name, "grab");
        ASSERT_STREQ(g_actions[1].name, "trigger");
        ASSERT_STREQ(g_actions[2].name, "trackpad");
        ASSERT_STREQ(g_actions[3].name, "left_hand");
        ASSERT_STREQ(g_actions[4].name, "haptic");
    END_TEST();

    BEGIN_TEST(parse_action_manifest_fullnames);
        setup();
        char err[256] = {};
        VRInput_SetActionManifest("tests/data/test_actions.json", err, sizeof(err));
        ASSERT_STREQ(g_actions[0].fullname, "/actions/main/in/grab");
        ASSERT_STREQ(g_actions[4].fullname, "/actions/main/out/haptic");
    END_TEST();

    BEGIN_TEST(parse_action_types_match_enums);
        setup();
        char err[256] = {};
        VRInput_SetActionManifest("tests/data/test_actions.json", err, sizeof(err));
        ASSERT_EQ(g_actions[0].type, ActionType_Boolean);
        ASSERT_EQ(g_actions[1].type, ActionType_Vector1);
        ASSERT_EQ(g_actions[2].type, ActionType_Vector2);
        ASSERT_EQ(g_actions[3].type, ActionType_Pose);
        ASSERT_EQ(g_actions[4].type, ActionType_Vibration);
    END_TEST();

    BEGIN_TEST(parse_action_handles_assigned);
        setup();
        char err[256] = {};
        VRInput_SetActionManifest("tests/data/test_actions.json", err, sizeof(err));
        ASSERT_EQ(g_actions[0].handle, (vr::VRActionHandle_t)101);
        ASSERT_EQ(g_actions[1].handle, (vr::VRActionHandle_t)102);
        ASSERT_EQ(g_actions[4].handle, (vr::VRActionHandle_t)105);
    END_TEST();

    BEGIN_TEST(parse_action_manifest_missing_file);
        setup();
        char err[256] = {};
        bool ok = VRInput_SetActionManifest("/nonexistent/path.json", err, sizeof(err));
        ASSERT_FALSE(ok);
        ASSERT_TRUE(strlen(err) > 0);
    END_TEST();

    BEGIN_TEST(set_active_action_sets);
        setup();
        const char* sets[] = { "/actions/main", "/actions/secondary" };
        VRInput_SetActiveActionSets(sets, 2);
        ASSERT_EQ(g_activeActionSetCount, 2);
        ASSERT_EQ(g_actionSetCount, 2);
        ASSERT_STREQ(g_actionSets[0].name, "/actions/main");
        ASSERT_STREQ(g_actionSets[1].name, "/actions/secondary");
    END_TEST();

    BEGIN_TEST(set_active_action_sets_reuses_existing);
        setup();
        const char* sets1[] = { "/actions/main" };
        VRInput_SetActiveActionSets(sets1, 1);
        ASSERT_EQ(g_actionSetCount, 1);
        VRInput_SetActiveActionSets(sets1, 1);
        ASSERT_EQ(g_actionSetCount, 1);
    END_TEST();

    BEGIN_TEST(trigger_haptic_fires);
        setup();
        char err[256] = {};
        VRInput_SetActionManifest("tests/data/test_actions.json", err, sizeof(err));
        s_mockInput.hapticTriggered = false;
        VRInput_TriggerHaptic("haptic", 0.0f, 0.5f, 100.0f, 0.8f);
        ASSERT_TRUE(s_mockInput.hapticTriggered);
        ASSERT_NEAR(s_mockInput.lastHapticAmplitude, 0.8f, 0.001f);
    END_TEST();

    BEGIN_TEST(trigger_haptic_nonexistent_noop);
        setup();
        s_mockInput.hapticTriggered = false;
        VRInput_TriggerHaptic("nonexistent", 0, 0, 0, 0);
        ASSERT_FALSE(s_mockInput.hapticTriggered);
    END_TEST();

    BEGIN_TEST(get_tracked_device_names);
        setup();
        strcpy(s_mockSystem.mockDeviceNames[0], "knuckles_left");
        strcpy(s_mockSystem.mockDeviceNames[1], "knuckles_right");
        s_mockSystem.mockDeviceCount = 2;
        char names[16][256] = {};
        int count = VRInput_GetTrackedDeviceNames(names, 16);
        ASSERT_EQ(count, 2);
        ASSERT_STREQ(names[0], "knuckles_left");
        ASSERT_STREQ(names[1], "knuckles_right");
    END_TEST();

    BEGIN_TEST(update_poses_and_actions_no_crash);
        setup();
        VRInput_UpdatePosesAndActions();
        ASSERT_TRUE(true);
    END_TEST();
}
