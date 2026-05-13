#pragma once

#include <openvr/openvr.h>
#include <cstring>
#include <vector>
#include <string>

namespace mock {

// Minimal IVRInput mock — stubs all pure virtuals, implements only what we test
class MockVRInput : public vr::IVRInput {
public:
    // Configurable test data
    struct ActionHandleEntry { std::string name; vr::VRActionHandle_t handle; };
    struct ActionSetEntry   { std::string name; vr::VRActionSetHandle_t handle; };

    std::vector<ActionHandleEntry> actionHandles;
    std::vector<ActionSetEntry>    actionSetHandles;
    vr::EVRInputError manifestError = vr::VRInputError_None;
    std::string lastManifestPath;
    int updateActionStateCalls = 0;
    int triggerHapticCalls = 0;
    vr::VRActionHandle_t lastHapticAction = 0;
    float lastHapticAmplitude = 0;

    vr::EVRInputError SetActionManifestPath(const char *p) override {
        lastManifestPath = p;
        return manifestError;
    }
    vr::EVRInputError GetActionSetHandle(const char *name, vr::VRActionSetHandle_t *h) override {
        for (auto& e : actionSetHandles) {
            if (e.name == name) { *h = e.handle; return vr::VRInputError_None; }
        }
        static vr::VRActionSetHandle_t nextHandle = 100;
        *h = nextHandle++;
        actionSetHandles.push_back({name, *h});
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetActionHandle(const char *name, vr::VRActionHandle_t *h) override {
        for (auto& e : actionHandles) {
            if (e.name == name) { *h = e.handle; return vr::VRInputError_None; }
        }
        static vr::VRActionHandle_t nextHandle = 200;
        *h = nextHandle++;
        actionHandles.push_back({name, *h});
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetInputSourceHandle(const char*, vr::VRInputValueHandle_t *h) override { *h = 0; return vr::VRInputError_None; }
    vr::EVRInputError UpdateActionState(vr::VRActiveActionSet_t*, uint32_t, uint32_t) override { updateActionStateCalls++; return vr::VRInputError_None; }
    vr::EVRInputError GetDigitalActionData(vr::VRActionHandle_t, vr::InputDigitalActionData_t *d, uint32_t, vr::VRInputValueHandle_t) override { memset(d, 0, sizeof(*d)); return vr::VRInputError_None; }
    vr::EVRInputError GetAnalogActionData(vr::VRActionHandle_t, vr::InputAnalogActionData_t *d, uint32_t, vr::VRInputValueHandle_t) override { memset(d, 0, sizeof(*d)); return vr::VRInputError_None; }
    vr::EVRInputError GetPoseActionDataRelativeToNow(vr::VRActionHandle_t, vr::ETrackingUniverseOrigin, float, vr::InputPoseActionData_t *d, uint32_t, vr::VRInputValueHandle_t) override { memset(d, 0, sizeof(*d)); return vr::VRInputError_None; }
    vr::EVRInputError GetPoseActionDataForNextFrame(vr::VRActionHandle_t, vr::ETrackingUniverseOrigin, vr::InputPoseActionData_t *d, uint32_t, vr::VRInputValueHandle_t) override { memset(d, 0, sizeof(*d)); return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalActionData(vr::VRActionHandle_t, vr::InputSkeletalActionData_t *d, uint32_t) override { memset(d, 0, sizeof(*d)); return vr::VRInputError_None; }
    vr::EVRInputError GetDominantHand(vr::ETrackedControllerRole *r) override { *r = vr::TrackedControllerRole_RightHand; return vr::VRInputError_None; }
    vr::EVRInputError SetDominantHand(vr::ETrackedControllerRole) override { return vr::VRInputError_None; }
    vr::EVRInputError GetBoneCount(vr::VRActionHandle_t, uint32_t *c) override { *c = 0; return vr::VRInputError_None; }
    vr::EVRInputError GetBoneHierarchy(vr::VRActionHandle_t, vr::BoneIndex_t*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetBoneName(vr::VRActionHandle_t, vr::BoneIndex_t, char*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalReferenceTransforms(vr::VRActionHandle_t, vr::EVRSkeletalTransformSpace, vr::EVRSkeletalReferencePose, vr::VRBoneTransform_t*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalTrackingLevel(vr::VRActionHandle_t, vr::EVRSkeletalTrackingLevel *l) override { *l = vr::VRSkeletalTracking_Full; return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalBoneData(vr::VRActionHandle_t, vr::EVRSkeletalTransformSpace, vr::EVRSkeletalMotionRange, vr::VRBoneTransform_t*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalSummaryData(vr::VRActionHandle_t, vr::EVRSummaryType, vr::VRSkeletalSummaryData_t *d) override { memset(d, 0, sizeof(*d)); return vr::VRInputError_None; }
    vr::EVRInputError GetSkeletalBoneDataCompressed(vr::VRActionHandle_t, vr::EVRSkeletalMotionRange, void*, uint32_t, uint32_t*) override { return vr::VRInputError_None; }
    vr::EVRInputError DecompressSkeletalBoneData(const void*, uint32_t, vr::EVRSkeletalTransformSpace, vr::VRBoneTransform_t*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError TriggerHapticVibrationAction(vr::VRActionHandle_t a, float, float, float, float amp, vr::VRInputValueHandle_t) override {
        triggerHapticCalls++;
        lastHapticAction = a;
        lastHapticAmplitude = amp;
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetActionOrigins(vr::VRActionSetHandle_t, vr::VRActionHandle_t, vr::VRInputValueHandle_t*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetOriginLocalizedName(vr::VRInputValueHandle_t, char*, uint32_t, int32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetOriginTrackedDeviceInfo(vr::VRInputValueHandle_t, vr::InputOriginInfo_t*, uint32_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetActionBindingInfo(vr::VRActionHandle_t, vr::InputBindingInfo_t*, uint32_t, uint32_t, uint32_t*) override { return vr::VRInputError_None; }
    vr::EVRInputError ShowActionOrigins(vr::VRActionSetHandle_t, vr::VRActionHandle_t) override { return vr::VRInputError_None; }
    vr::EVRInputError ShowBindingsForActionSet(vr::VRActiveActionSet_t*, uint32_t, uint32_t, vr::VRInputValueHandle_t) override { return vr::VRInputError_None; }
    vr::EVRInputError GetComponentStateForBinding(const char*, const char*, const vr::InputBindingInfo_t*, uint32_t, uint32_t, vr::RenderModel_ComponentState_t*) override { return vr::VRInputError_None; }
    bool IsUsingLegacyInput() override { return false; }
    vr::EVRInputError OpenBindingUI(const char*, vr::VRActionSetHandle_t, vr::VRInputValueHandle_t, bool) override { return vr::VRInputError_None; }
    vr::EVRInputError GetBindingVariant(vr::VRInputValueHandle_t, char*, uint32_t) override { return vr::VRInputError_None; }
};

// Helper: build a TrackedDevicePose_t with known values for testing
inline vr::TrackedDevicePose_t MakePose(
    float tx, float ty, float tz,   // translation in raw HmdMatrix34 space
    float vx, float vy, float vz,   // velocity
    float avx, float avy, float avz, // angular velocity (rad/s)
    bool valid = true)
{
    vr::TrackedDevicePose_t p;
    memset(&p, 0, sizeof(p));
    p.bPoseIsValid = valid;
    p.bDeviceIsConnected = valid;
    p.eTrackingResult = valid ? vr::TrackingResult_Running_OK : vr::TrackingResult_Uninitialized;

    // Identity rotation
    p.mDeviceToAbsoluteTracking.m[0][0] = 1; p.mDeviceToAbsoluteTracking.m[0][1] = 0; p.mDeviceToAbsoluteTracking.m[0][2] = 0;
    p.mDeviceToAbsoluteTracking.m[1][0] = 0; p.mDeviceToAbsoluteTracking.m[1][1] = 1; p.mDeviceToAbsoluteTracking.m[1][2] = 0;
    p.mDeviceToAbsoluteTracking.m[2][0] = 0; p.mDeviceToAbsoluteTracking.m[2][1] = 0; p.mDeviceToAbsoluteTracking.m[2][2] = 1;

    p.mDeviceToAbsoluteTracking.m[0][3] = tx;
    p.mDeviceToAbsoluteTracking.m[1][3] = ty;
    p.mDeviceToAbsoluteTracking.m[2][3] = tz;

    p.vVelocity.v[0] = vx; p.vVelocity.v[1] = vy; p.vVelocity.v[2] = vz;
    p.vAngularVelocity.v[0] = avx; p.vAngularVelocity.v[1] = avy; p.vAngularVelocity.v[2] = avz;
    return p;
}

} // namespace mock
