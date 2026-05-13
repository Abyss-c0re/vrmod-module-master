#include <vector>
#include <string>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>

#include <GL/gl.h>

#include <gmod/Interface.h>
#include <openvr/openvr.h>

#include "core/vrmod_common.h"
#include "core/vrmod_log.h"
#include "input/vr_input.h"
#include "rendering/opengl/gl_hooks.h"
#include "rendering/openvr/vr_render.h"

// ── Lua-side state ──
static char g_errorString[MAX_STR_LEN];
static int  g_luaRefs[LuaRefIndex_Max];
static int  g_luaRefCount = 0;
static bool g_IsPaused = false;

// ── Helpers ──

static void LuaPrint(GarrysMod::Lua::ILuaBase* LUA, const char* msg) {
    LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    LUA->PushString(msg);
    LUA->Call(1, 0);
    LUA->Pop(1);
}

static void LuaThrowError(GarrysMod::Lua::ILuaBase* LUA, const char* msg) {
    LUA->ThrowError(msg);
}

static void PushMatrixAsTable(GarrysMod::Lua::ILuaBase* LUA, float* mtx, unsigned int rows, unsigned int cols) {
    LUA->CreateTable();
    for (unsigned int row = 0; row < rows; row++) {
        LUA->PushNumber(row + 1);
        LUA->CreateTable();
        for (unsigned int col = 0; col < cols; col++) {
            LUA->PushNumber(col + 1);
            LUA->PushNumber(mtx[row * cols + col]);
            LUA->SetTable(-3);
        }
        LUA->SetTable(-3);
    }
}

// ── LUA_FUNCTIONs ──

LUA_FUNCTION(GetVersion) {
    LUA->PushNumber(23);
    return 1;
}

LUA_FUNCTION(IsHMDPresent) {
    LUA->PushBool(vr::VR_IsHmdPresent());
    return 1;
}

LUA_FUNCTION(Init) {
    if (g_pSystem != nullptr) {
        if (g_IsPaused) {
            LUA->CreateTable();
            g_luaRefs[LuaRefIndex_PoseTable] = LUA->ReferenceCreate();

            LUA->CreateTable();
            g_luaRefs[LuaRefIndex_EmptyTable] = LUA->ReferenceCreate();

            LUA->CreateTable();
            g_luaRefs[LuaRefIndex_ActionTable] = LUA->ReferenceCreate();

            LUA->CreateTable();
            g_luaRefs[LuaRefIndex_HmdPose] = LUA->ReferenceCreate();
            g_IsPaused = false;
            VRMOD_LOG_INFO("Resumed from paused state.");
            return 0;
        }
    }

    vr::HmdError error = vr::VRInitError_None;
    g_pSystem = vr::VR_Init(&error, vr::VRApplication_Scene);
    if (error != vr::VRInitError_None)
        LUA->ThrowError(vr::VR_GetVRInitErrorAsEnglishDescription(error));

    memset(g_luaRefs, 0, sizeof(g_luaRefs));
    for (int i = 0; i < LuaRefIndex_Max; i++) {
        LUA->CreateTable();
        g_luaRefs[i] = LUA->ReferenceCreate();
        g_luaRefCount++;
    }

    void* lib = dlopen("libtogl_client.so", RTLD_NOW | RTLD_NOLOAD);
    if (!lib) LUA->ThrowError("VRMOD: dlopen failed");

    auto GetOpenGLEntryPoints = reinterpret_cast<GetOpenGLEntryPoints_t>(dlsym(lib, "GetOpenGLEntryPoints"));
    if (!GetOpenGLEntryPoints) LUA->ThrowError("VRMOD: dlsym failed");

    g_GL = GetOpenGLEntryPoints(nullptr);
    dlclose(lib);

    g_createTexture = *((void**)&g_GL->firstFunc + 50);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char buf[128];
        snprintf(buf, sizeof(buf), "VRMOD: OpenGL error: %u", err);
        LUA->ThrowError(buf);
        return 0;
    }

    g_compositor = vr::VRCompositor();
    g_pInput = vr::VRInput();

    VRMOD_LOG_INFO("VR initialized successfully.");
    return 0;
}

LUA_FUNCTION(SetActionManifest) {
    const char* fileName = LUA->CheckString(1);
    char path[PATH_MAX];
    char currentDir[PATH_MAX];
    if (getcwd(currentDir, PATH_MAX) == NULL)
        LUA->ThrowError("VRMOD: getcwd failed");
    if (snprintf(path, PATH_MAX, "%s/garrysmod/data/%s", currentDir, fileName) >= PATH_MAX)
        LUA->ThrowError("VRMOD: SetActionManifest path too long");

    int result = ParseActionManifest(path, g_actions, MAX_ACTIONS, g_pInput);
    if (result == -1)
        LUA->ThrowError("VRMOD: SetActionManifestPath failed");
    if (result == -2)
        LUA->ThrowError("VRMOD: failed to open action manifest");

    g_actionCount = result;

    // Create lua refs for actions that need them
    for (int i = 0; i < g_actionCount; i++) {
        for (int j = 0; j < 2; j++) {
            LUA->CreateTable();
            g_actions[i].luaRefs[j] = LUA->ReferenceCreate();
        }
    }
    return 0;
}

LUA_FUNCTION(SetActiveActionSets) {
    g_activeActionSetCount = 0;
    for (int i = 0; i < MAX_ACTIONSETS; i++) {
        if (LUA->GetType(i + 1) == GarrysMod::Lua::Type::STRING) {
            const char* actionSetName = LUA->CheckString(i + 1);
            int actionSetIndex = FindOrCreateActionSet(
                actionSetName, g_actionSets, &g_actionSetCount, g_pInput);
            g_activeActionSets[g_activeActionSetCount].ulActionSet = g_actionSets[actionSetIndex].handle;
            g_activeActionSetCount++;
        } else {
            break;
        }
    }
    return 0;
}

LUA_FUNCTION(GetDisplayInfo) {
    UpdateRecommendedSize();
    float fNearZ = (float)LUA->CheckNumber(1);
    float fFarZ = (float)LUA->CheckNumber(2);
    vr::HmdMatrix44_t projLeft = g_pSystem->GetProjectionMatrix(vr::Hmd_Eye::Eye_Left, fNearZ, fFarZ);
    vr::HmdMatrix44_t projRight = g_pSystem->GetProjectionMatrix(vr::Hmd_Eye::Eye_Right, fNearZ, fFarZ);
    vr::HmdMatrix34_t transformLeft = g_pSystem->GetEyeToHeadTransform(vr::Eye_Left);
    vr::HmdMatrix34_t transformRight = g_pSystem->GetEyeToHeadTransform(vr::Eye_Right);
    LUA->CreateTable();
    PushMatrixAsTable(LUA, (float*)&projLeft, 4, 4);
    LUA->SetField(-2, "ProjectionLeft");
    PushMatrixAsTable(LUA, (float*)&projRight, 4, 4);
    LUA->SetField(-2, "ProjectionRight");
    PushMatrixAsTable(LUA, (float*)&transformLeft, 3, 4);
    LUA->SetField(-2, "TransformLeft");
    PushMatrixAsTable(LUA, (float*)&transformRight, 3, 4);
    LUA->SetField(-2, "TransformRight");
    LUA->PushNumber(recommendedWidth);
    LUA->SetField(-2, "RecommendedWidth");
    LUA->PushNumber(recommendedHeight);
    LUA->SetField(-2, "RecommendedHeight");
    return 1;
}

LUA_FUNCTION(UpdatePosesAndActions) {
    g_compositor->WaitGetPoses(g_poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
    g_pInput->UpdateActionState(g_activeActionSets, sizeof(vr::VRActiveActionSet_t), g_activeActionSetCount);
    return 0;
}

LUA_FUNCTION(GetPoses) {
    vr::InputPoseActionData_t poseActionData;
    vr::TrackedDevicePose_t pose = g_poses[0];
    char* poseName = (char*)"hmd";
    int poseRef = g_luaRefs[LuaRefIndex_HmdPose];
    LUA->ReferencePush(g_luaRefs[LuaRefIndex_PoseTable]);
    for (int i = -1; i < g_actionCount; i++) {
        if (i != -1) {
            if (g_actions[i].type == ActionType_Pose) {
                g_pInput->GetPoseActionDataRelativeToNow(g_actions[i].handle, vr::TrackingUniverseStanding, 0, &poseActionData, sizeof(poseActionData), vr::k_ulInvalidInputValueHandle);
                pose = poseActionData.pose;
                poseName = g_actions[i].name;
                poseRef = g_actions[i].luaRefs[0];
            } else continue;
        }
        PoseResult pr = ConvertPose(pose);
        if (pr.valid) {
            Vector pos; pos.x = pr.pos[0]; pos.y = pr.pos[1]; pos.z = pr.pos[2];
            Vector vel; vel.x = pr.vel[0]; vel.y = pr.vel[1]; vel.z = pr.vel[2];
            QAngle ang; ang.x = pr.ang[0]; ang.y = pr.ang[1]; ang.z = pr.ang[2];
            QAngle angvel; angvel.x = pr.angvel[0]; angvel.y = pr.angvel[1]; angvel.z = pr.angvel[2];
            LUA->ReferencePush(poseRef);
            LUA->PushVector(pos);
            LUA->SetField(-2, "pos");
            LUA->PushVector(vel);
            LUA->SetField(-2, "vel");
            LUA->PushAngle(ang);
            LUA->SetField(-2, "ang");
            LUA->PushAngle(angvel);
            LUA->SetField(-2, "angvel");
            LUA->SetField(-2, poseName);
        }
    }
    return 1;
}

LUA_FUNCTION(GetActions) {
    vr::InputDigitalActionData_t digitalActionData;
    vr::InputAnalogActionData_t analogActionData;
    vr::VRSkeletalSummaryData_t skeletalSummaryData;
    char* changedActionNames[MAX_ACTIONS];
    bool changedActionStates[MAX_ACTIONS];
    int changedActionCount = 0;
    LUA->ReferencePush(g_luaRefs[LuaRefIndex_ActionTable]);
    for (int i = 0; i < g_actionCount; i++) {
        if (g_actions[i].type == ActionType_Boolean) {
            LUA->PushBool((g_pInput->GetDigitalActionData(g_actions[i].handle, &digitalActionData, sizeof(digitalActionData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && digitalActionData.bState));
            LUA->SetField(-2, g_actions[i].name);
            if (digitalActionData.bChanged) {
                changedActionNames[changedActionCount] = g_actions[i].name;
                changedActionStates[changedActionCount] = digitalActionData.bState;
                changedActionCount++;
            }
        }
        else if (g_actions[i].type == ActionType_Vector1) {
            g_pInput->GetAnalogActionData(g_actions[i].handle, &analogActionData, sizeof(analogActionData), vr::k_ulInvalidInputValueHandle);
            LUA->PushNumber(analogActionData.x);
            LUA->SetField(-2, g_actions[i].name);
        }
        else if (g_actions[i].type == ActionType_Vector2) {
            LUA->ReferencePush(g_actions[i].luaRefs[0]);
            g_pInput->GetAnalogActionData(g_actions[i].handle, &analogActionData, sizeof(analogActionData), vr::k_ulInvalidInputValueHandle);
            LUA->PushNumber(analogActionData.x);
            LUA->SetField(-2, "x");
            LUA->PushNumber(analogActionData.y);
            LUA->SetField(-2, "y");
            LUA->SetField(-2, g_actions[i].name);
        }
        else if (g_actions[i].type == ActionType_Skeleton) {
            g_pInput->GetSkeletalSummaryData(g_actions[i].handle, static_cast<vr::EVRSummaryType>(1), &skeletalSummaryData);
            LUA->ReferencePush(g_actions[i].luaRefs[0]);
            LUA->ReferencePush(g_actions[i].luaRefs[1]);
            for (int j = 0; j < 5; j++) {
                LUA->PushNumber(j + 1);
                LUA->PushNumber(skeletalSummaryData.flFingerCurl[j]);
                LUA->SetTable(-3);
            }
            LUA->SetField(-2, "fingerCurls");
            LUA->SetField(-2, g_actions[i].name);
        }
    }
    if (changedActionCount == 0) {
        LUA->ReferencePush(g_luaRefs[LuaRefIndex_EmptyTable]);
    } else {
        LUA->CreateTable();
        for (int i = 0; i < changedActionCount; i++) {
            LUA->PushBool(changedActionStates[i]);
            LUA->SetField(-2, changedActionNames[i]);
        }
    }
    return 2;
}

LUA_FUNCTION(ShareTextureBegin) {
    // Clear previous texture bounds and vr texture when tearing down
    if (glIsTexture(g_sharedTexture)) {
        memset(&g_textureBoundsLeft,  0, sizeof(g_textureBoundsLeft));
        memset(&g_textureBoundsRight, 0, sizeof(g_textureBoundsRight));
        g_vrTexture = { nullptr, vr::TextureType_Invalid, vr::ColorSpace_Auto };
    }

    // Error bridge: capture LUA pointer for ThrowError
    GarrysMod::Lua::ILuaBase* luaPtr = LUA;
    auto errBridge = [](const char* msg) {
        // In release, errors are posted via lua; logging is a noop
        VRMOD_LOG_ERROR("%s", msg);
    };

    int rc = ShareTextureBegin(recommendedWidth, recommendedHeight, errBridge);
    if (rc != 0) {
        LUA->ThrowError("VRMOD: mprotect RWX failed");
    }
    return 0;
}

LUA_FUNCTION(ShareTextureFinish) {
    if (g_sharedTexture == 0 || !glIsTexture(g_sharedTexture)) {
        LUA->ThrowError("VRMOD: Failed to generate shared texture.");
        return 0;
    }
    g_vrTexture.handle = (void*)(uintptr_t)g_sharedTexture;
    g_vrTexture.eType = vr::TextureType_OpenGL;
    g_vrTexture.eColorSpace = vr::ColorSpace_Gamma;

    auto errBridge = [](const char* msg) {
        VRMOD_LOG_ERROR("%s", msg);
    };

    if (!RemoveTexturePatch(errBridge)) {
        LUA->ThrowError("VRMOD: Failed to remove the texture path.");
    }

    return 0;
}

LUA_FUNCTION(SetSubmitTextureBounds) {
    g_textureBoundsLeft.uMin  = (float)LUA->CheckNumber(1);
    g_textureBoundsLeft.vMin  = (float)LUA->CheckNumber(2);
    g_textureBoundsLeft.uMax  = (float)LUA->CheckNumber(3);
    g_textureBoundsLeft.vMax  = (float)LUA->CheckNumber(4);

    g_textureBoundsRight.uMin = (float)LUA->CheckNumber(5);
    g_textureBoundsRight.vMin = (float)LUA->CheckNumber(6);
    g_textureBoundsRight.uMax = (float)LUA->CheckNumber(7);
    g_textureBoundsRight.vMax = (float)LUA->CheckNumber(8);

    return 0;
}

LUA_FUNCTION(SubmitSharedTexture) {
    SubmitResult res = SubmitFrames();
    if (!res.ok()) {
        std::string errMsg =
            "VRMOD: OpenVR Submit failed: Left: " + std::to_string(res.errLeft) +
            ", Right: " + std::to_string(res.errRight);
        LuaPrint(LUA, errMsg.c_str());
    }
    return 0;
}

LUA_FUNCTION(Shutdown) {
    if (!g_compositor)
        return 0;
    if (g_IsPaused)
        return 0;
    if (g_compositor) {
        g_compositor->ClearLastSubmittedFrame();
        glFlush();
        glFinish();

        for (int i = 0; i < g_luaRefCount; i++) {
            if (g_luaRefs[i] != 0) {
                LUA->ReferenceFree(g_luaRefs[i]);
                g_luaRefs[i] = 0;
            }
        }
        for (int i = 0; i < g_actionCount; i++) {
            for (int j = 0; j < 2; j++) {
                if (g_actions[i].luaRefs[j] != 0) {
                    LUA->ReferenceFree(g_actions[i].luaRefs[j]);
                    g_actions[i].luaRefs[j] = 0;
                }
            }
        }
        g_luaRefCount = LuaRefIndex_Max;
        g_actionCount = 0;
        memset(g_actions, 0, sizeof(g_actions));
        g_actionSetCount = 0;
        g_activeActionSetCount = 0;
        g_IsPaused = true;

        VRMOD_LOG_INFO("VR shutdown (paused).");
    }
    return 0;
}

LUA_FUNCTION(TriggerHaptic) {
    const char* actionName = LUA->CheckString(1);
    vr::VRActionHandle_t handle = FindActionHandleByName(actionName, g_actions, g_actionCount);
    if (handle != vr::k_ulInvalidActionHandle) {
        g_pInput->TriggerHapticVibrationAction(handle, (float)LUA->CheckNumber(2), (float)LUA->CheckNumber(3), (float)LUA->CheckNumber(4), (float)LUA->CheckNumber(5), vr::k_ulInvalidInputValueHandle);
    }
    return 0;
}

LUA_FUNCTION(GetTrackedDeviceNames) {
    LUA->CreateTable();
    int tableIndex = 1;
    char name[MAX_STR_LEN];
    for (int i = 0; i < (int)vr::k_unMaxTrackedDeviceCount; i++) {
        if (g_pSystem->GetStringTrackedDeviceProperty(i, vr::Prop_ControllerType_String, name, MAX_STR_LEN) > 1) {
            LUA->PushNumber(tableIndex);
            LUA->PushString(name);
            LUA->SetTable(-3);
            tableIndex++;
        }
    }
    return 1;
}

// ── Module entry points ──

GMOD_MODULE_OPEN() {
    VRMOD_LOG_INIT("vrmod_debug.log");
    VRMOD_LOG_INFO("Module loading...");

    LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    LUA->GetField(-1, "vrmod");
    if (!LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
        LUA->Pop(1);
        LUA->CreateTable();
    }
    LUA->PushCFunction(GetVersion);
    LUA->SetField(-2, "GetVersion");
    LUA->PushCFunction(IsHMDPresent);
    LUA->SetField(-2, "IsHMDPresent");
    LUA->PushCFunction(Init);
    LUA->SetField(-2, "Init");
    LUA->PushCFunction(SetActionManifest);
    LUA->SetField(-2, "SetActionManifest");
    LUA->PushCFunction(SetActiveActionSets);
    LUA->SetField(-2, "SetActiveActionSets");
    LUA->PushCFunction(GetDisplayInfo);
    LUA->SetField(-2, "GetDisplayInfo");
    LUA->PushCFunction(UpdatePosesAndActions);
    LUA->SetField(-2, "UpdatePosesAndActions");
    LUA->PushCFunction(GetPoses);
    LUA->SetField(-2, "GetPoses");
    LUA->PushCFunction(GetActions);
    LUA->SetField(-2, "GetActions");
    LUA->PushCFunction(ShareTextureBegin);
    LUA->SetField(-2, "ShareTextureBegin");
    LUA->PushCFunction(ShareTextureFinish);
    LUA->SetField(-2, "ShareTextureFinish");
    LUA->PushCFunction(SetSubmitTextureBounds);
    LUA->SetField(-2, "SetSubmitTextureBounds");
    LUA->PushCFunction(SubmitSharedTexture);
    LUA->SetField(-2, "SubmitSharedTexture");
    LUA->PushCFunction(Shutdown);
    LUA->SetField(-2, "Shutdown");
    LUA->PushCFunction(TriggerHaptic);
    LUA->SetField(-2, "TriggerHaptic");
    LUA->PushCFunction(GetTrackedDeviceNames);
    LUA->SetField(-2, "GetTrackedDeviceNames");
    LUA->SetField(-2, "vrmod");

    VRMOD_LOG_INFO("Module loaded.");
    return 0;
}

GMOD_MODULE_CLOSE() {
    VRMOD_LOG_INFO("Module closing.");
    VRMOD_LOG_CLOSE();
    return 0;
}
