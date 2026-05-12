#include "lua_helpers.h"
#include "vrmod/types.h"
#include "vrmod/globals.h"
#include "vrmod/logging.h"
#include "vrmod/input/vr_input.h"
#include "vrmod/rendering/opengl/gl_texture.h"
#include "vrmod/rendering/openvr/vr_system.h"

#include <cstring>
#include <cstdio>
#include <climits>
#include <cmath>
#include <unistd.h>
#include <string>

#include <openvr/openvr.h>
#include <gmod/Interface.h>

/******************************************************************************
 * LUA_FUNCTION bindings
 *****************************************************************************/

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
            return 0;
        }
    }

    char err[MAX_STR_LEN];
    if (!VR_InitSystem(err, sizeof(err)))
        LUA->ThrowError(err);

    memset(g_luaRefs, 0, sizeof(g_luaRefs));
    for (int i = 0; i < LuaRefIndex_Max; i++) {
        LUA->CreateTable();
        g_luaRefs[i] = LUA->ReferenceCreate();
        g_luaRefCount++;
    }

    if (!GL_Init(err, sizeof(err)))
        LUA->ThrowError(err);

    VRMOD_LOG_INFO("VRMOD Init complete");
    return 0;
}

LUA_FUNCTION(SetActionManifest) {
    const char* fileName = LUA->CheckString(1);
    char path[PATH_MAX];
    char currentDir[PATH_MAX];
    if(getcwd(currentDir, PATH_MAX) == NULL)
        LUA->ThrowError("VRMOD: getcwd failed");
    if (snprintf(path, PATH_MAX, "%s/garrysmod/data/%s", currentDir, fileName) >= PATH_MAX)
        LUA->ThrowError("VRMOD: SetActionManifest path too long");

    char err[MAX_STR_LEN];
    if (!VRInput_SetActionManifest(path, err, sizeof(err)))
        LUA->ThrowError(err);

    // Create Lua reference tables for each parsed action
    for (int i = 0; i < g_actionCount; i++) {
        for(int j = 0; j < 2; j++){
            LUA->CreateTable();
            g_actions[i].luaRefs[j] = LUA->ReferenceCreate();
        }
    }
    return 0;
}

LUA_FUNCTION(SetActiveActionSets) {
    const char* names[MAX_ACTIONSETS];
    int count = 0;
    for (int i = 0; i < MAX_ACTIONSETS; i++) {
        if (LUA->GetType(i + 1) == GarrysMod::Lua::Type::STRING) {
            names[count++] = LUA->CheckString(i + 1);
        } else {
            break;
        }
    }
    VRInput_SetActiveActionSets(names, count);
    return 0;
}

LUA_FUNCTION(GetDisplayInfo) {
    VR_UpdateRecommendedSize();
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
    VRInput_UpdatePosesAndActions();
    return 0;
}

LUA_FUNCTION(GetPoses) {
    vr::InputPoseActionData_t poseActionData;
    vr::TrackedDevicePose_t pose = g_poses[0];
    char* poseName = (char*)"hmd";
    int poseRef = g_luaRefs[LuaRefIndex_HmdPose];
    LUA->ReferencePush(g_luaRefs[LuaRefIndex_PoseTable]);
    for (int i = -1; i < g_actionCount; i++) {
        if (i != -1){
            if (g_actions[i].type == ActionType_Pose) {
                g_pInput-> GetPoseActionDataRelativeToNow(g_actions[i].handle, vr::TrackingUniverseStanding, 0, &poseActionData, sizeof(poseActionData), vr::k_ulInvalidInputValueHandle);
                pose = poseActionData.pose;
                poseName = g_actions[i].name;
                poseRef = g_actions[i].luaRefs[0];
            } else continue;
        }
        if (pose.bPoseIsValid) {
            vr::HmdMatrix34_t mat = pose.mDeviceToAbsoluteTracking;
            Vector pos;
            Vector vel;
            QAngle ang;
            QAngle angvel;
            pos.x = -mat.m[2][3];
            pos.y = -mat.m[0][3];
            pos.z = mat.m[1][3];
            ang.x = asinf(mat.m[1][2]) * (180.0f / PI_F);
            ang.y = atan2f(mat.m[0][2], mat.m[2][2]) * (180.0f / PI_F);
            ang.z = atan2f(-mat.m[1][0], mat.m[1][1]) * (180.0f / PI_F);
            vel.x = -pose.vVelocity.v[2];
            vel.y = -pose.vVelocity.v[0];
            vel.z = pose.vVelocity.v[1];
            angvel.x = -pose.vAngularVelocity.v[2] * (180.0f / PI_F);
            angvel.y = -pose.vAngularVelocity.v[0] * (180.0f / PI_F);
            angvel.z = pose.vAngularVelocity.v[1] * (180.0f / PI_F);
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
            if(digitalActionData.bChanged){
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
            g_pInput->GetSkeletalSummaryData(g_actions[i].handle, static_cast<vr::EVRSummaryType>(1) , &skeletalSummaryData);
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
    if (changedActionCount == 0){
        LUA->ReferencePush(g_luaRefs[LuaRefIndex_EmptyTable]);
    }else{
        LUA->CreateTable();
        for(int i = 0; i < changedActionCount; i++){
            LUA->PushBool(changedActionStates[i]);
            LUA->SetField(-2,changedActionNames[i]);
        }
    }
    return 2;
}

LUA_FUNCTION(ShareTextureBegin) {
    char err[MAX_STR_LEN];
    if (!GL_ShareTextureBegin(recommendedWidth, recommendedHeight, err, sizeof(err))) {
        LUA->ThrowError(err);
    }
    return 0;
}

LUA_FUNCTION(ShareTextureFinish) {
    char err[MAX_STR_LEN];
    if (!GL_ShareTextureFinish(err, sizeof(err))) {
        LUA->ThrowError(err);
    }
    return 0;
}

LUA_FUNCTION(SetSubmitTextureBounds) {
    GL_SetSubmitTextureBounds(
        (float)LUA->CheckNumber(1), (float)LUA->CheckNumber(2),
        (float)LUA->CheckNumber(3), (float)LUA->CheckNumber(4),
        (float)LUA->CheckNumber(5), (float)LUA->CheckNumber(6),
        (float)LUA->CheckNumber(7), (float)LUA->CheckNumber(8)
    );
    return 0;
}

LUA_FUNCTION(SubmitSharedTexture) {
    char err[MAX_STR_LEN];
    VR_SubmitSharedTexture(err, sizeof(err));
    if (err[0] != '\0') {
        LuaPrint(LUA, err);
    }
    return 0;
}

LUA_FUNCTION(Shutdown) {
    if(!g_compositor)
        return 0;
    if(g_IsPaused)
        return 0;
    if (g_compositor) {
        VR_ShutdownCompositor();
        GL_FlushAndFinish();

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
        g_luaRefCount =  LuaRefIndex_Max;
        g_actionCount = 0;
        memset(g_actions, 0, sizeof(g_actions));
        g_actionSetCount = 0;
        g_activeActionSetCount = 0;
        g_IsPaused = true;
    }
    return 0;
}

LUA_FUNCTION(TriggerHaptic) {
    const char* actionName = LUA->CheckString(1);
    VRInput_TriggerHaptic(actionName,
        (float)LUA->CheckNumber(2), (float)LUA->CheckNumber(3),
        (float)LUA->CheckNumber(4), (float)LUA->CheckNumber(5));
    return 0;
}

LUA_FUNCTION(GetTrackedDeviceNames) {
    char names[vr::k_unMaxTrackedDeviceCount][256];
    int count = VRInput_GetTrackedDeviceNames(names, vr::k_unMaxTrackedDeviceCount);

    LUA->CreateTable();
    for (int i = 0; i < count; i++) {
        LUA->PushNumber(i + 1);
        LUA->PushString(names[i]);
        LUA->SetTable(-3);
    }
    return 1;
}

/******************************************************************************
 * Module entry points
 *****************************************************************************/

GMOD_MODULE_OPEN(){
    VRMOD_LOG_INFO("VRMOD module loading");

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
    return 0;
}

GMOD_MODULE_CLOSE(){
    VRMOD_LOG_INFO("VRMOD module unloading");
    vrmod_log_shutdown();
    return 0;
}
