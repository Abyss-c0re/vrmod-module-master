#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>
#include <unistd.h>

#include <gmod/Interface.h>
#include <openvr/openvr.h>

#include "vr_input.h"
#include "common/vrmod_types.h"
#include "common/vrmod_state.h"
#include "common/vrmod_log.h"

//---------------------------------------------------------------------------
// Extracted testable helpers
//---------------------------------------------------------------------------

PoseResult ExtractPoseData(const float mat[3][4],
                           const float velocity[3],
                           const float angularVelocity[3])
{
    PoseResult r;
    r.pos[0] = -mat[2][3];
    r.pos[1] = -mat[0][3];
    r.pos[2] = mat[1][3];

    r.ang[0] = asinf(mat[1][2]) * (180.0f / PI_F);
    r.ang[1] = atan2f(mat[0][2], mat[2][2]) * (180.0f / PI_F);
    r.ang[2] = atan2f(-mat[1][0], mat[1][1]) * (180.0f / PI_F);

    r.vel[0] = -velocity[2];
    r.vel[1] = -velocity[0];
    r.vel[2] = velocity[1];

    r.angvel[0] = -angularVelocity[2] * (180.0f / PI_F);
    r.angvel[1] = -angularVelocity[0] * (180.0f / PI_F);
    r.angvel[2] = angularVelocity[1] * (180.0f / PI_F);

    return r;
}

int ComputeActionTypeFromString(const char* typeStr) {
    int type = 0;
    for (int i = 0; typeStr[i]; i++)
        type += typeStr[i];
    return type;
}

//---------------------------------------------------------------------------
// LUA_FUNCTION implementations
//---------------------------------------------------------------------------

LUA_FUNCTION(SetActionManifest) {
    const char* fileName = LUA->CheckString(1);
    char path[PATH_MAX];
    char currentDir[PATH_MAX];
    if(getcwd(currentDir, PATH_MAX) == NULL)
        LUA->ThrowError("VRMOD: getcwd failed");
    if (snprintf(path, PATH_MAX, "%s/garrysmod/data/%s", currentDir, fileName) >= PATH_MAX)
        LUA->ThrowError("VRMOD: SetActionManifest path too long");

    if (g_pInput->SetActionManifestPath(path) != vr::VRInputError_None)
        LUA->ThrowError("VRMOD: SetActionManifestPath failed");
    FILE* file = fopen(path, "r");
    if (file == NULL)
        LUA->ThrowError("VRMOD: failed to open action manifest");
    memset(g_actions, 0, sizeof(g_actions));
    char word[MAX_STR_LEN];
    char fmt1[MAX_STR_LEN], fmt2[MAX_STR_LEN];
    snprintf(fmt1, MAX_STR_LEN, "%%*[^\"]\"%%%i[^\"]\"", MAX_STR_LEN-1);
    snprintf(fmt2, MAX_STR_LEN, "%%%i[^\"]\"", MAX_STR_LEN-1);
    while (fscanf(file, fmt1, word) == 1 && strcmp(word, "actions") != 0);
    while (fscanf(file, fmt2, word) == 1) {
        if (strchr(word, ']') != nullptr)
            break;
        if (strcmp(word, "name") == 0) {
            if (fscanf(file, fmt1, g_actions[g_actionCount].fullname) != 1)
                break;
            g_actions[g_actionCount].name = g_actions[g_actionCount].fullname;
            for (unsigned int i = 0; i < strlen(g_actions[g_actionCount].fullname); i++) {
                if (g_actions[g_actionCount].fullname[i] == '/')
                    g_actions[g_actionCount].name = g_actions[g_actionCount].fullname + i + 1;
            }
            g_pInput->GetActionHandle(g_actions[g_actionCount].fullname, &(g_actions[g_actionCount].handle));
        }
        if (strcmp(word, "type") == 0) {
            char typeStr[MAX_STR_LEN] = {0};
            if (fscanf(file, fmt1, typeStr) != 1)
                break;
            g_actions[g_actionCount].type = ComputeActionTypeFromString(typeStr);
        }
        if (g_actions[g_actionCount].fullname[0] && g_actions[g_actionCount].type) {
            for(int i = 0; i < 2; i++){
                LUA->CreateTable();
                g_actions[g_actionCount].luaRefs[i] = LUA->ReferenceCreate();
            }
            VRMOD_LOG("Registered action: %s (type=%d)", g_actions[g_actionCount].fullname, g_actions[g_actionCount].type);
            g_actionCount++;
            if (g_actionCount == MAX_ACTIONS)
                break;
        }
    }
    fclose(file);
    VRMOD_LOG("Action manifest loaded: %d actions from %s", g_actionCount, path);
    return 0;
}

LUA_FUNCTION(SetActiveActionSets) {
    g_activeActionSetCount = 0;
    for (int i = 0; i < MAX_ACTIONSETS; i++) {
        if (LUA->GetType(i + 1) == GarrysMod::Lua::Type::STRING) {
            const char* actionSetName = LUA->CheckString(i + 1);
            int actionSetIndex = -1;
            for (int j = 0; j < g_actionSetCount; j++) {
                if (strcmp(actionSetName, g_actionSets[j].name) == 0) {
                    actionSetIndex = j;
                    break;
                }
            }
            if (actionSetIndex == -1) {
                g_pInput->GetActionSetHandle(actionSetName, &g_actionSets[g_actionSetCount].handle);
                memcpy(g_actionSets[g_actionSetCount].name, actionSetName, strlen(actionSetName));
                actionSetIndex = g_actionSetCount;
                g_actionSetCount++;
            }
            g_activeActionSets[g_activeActionSetCount].ulActionSet = g_actionSets[actionSetIndex].handle;
            g_activeActionSetCount++;
        }
        else {
            break;
        }
    }
    VRMOD_LOG("Active action sets updated: %d sets", g_activeActionSetCount);
    return 0;
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
            PoseResult pr = ExtractPoseData(
                mat.m,
                pose.vVelocity.v,
                pose.vAngularVelocity.v
            );
            Vector pos;
            pos.x = pr.pos[0]; pos.y = pr.pos[1]; pos.z = pr.pos[2];
            Vector vel;
            vel.x = pr.vel[0]; vel.y = pr.vel[1]; vel.z = pr.vel[2];
            QAngle ang;
            ang.x = pr.ang[0]; ang.y = pr.ang[1]; ang.z = pr.ang[2];
            QAngle angvel;
            angvel.x = pr.angvel[0]; angvel.y = pr.angvel[1]; angvel.z = pr.angvel[2];
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

LUA_FUNCTION(TriggerHaptic) {
    const char* actionName = LUA->CheckString(1);
    for (int i = 0; i < g_actionCount; i++) {
        if (strcmp(g_actions[i].name, actionName) == 0) {
            g_pInput->TriggerHapticVibrationAction(g_actions[i].handle, (float)LUA->CheckNumber(2), (float)LUA->CheckNumber(3), (float)LUA->CheckNumber(4), (float)LUA->CheckNumber(5), vr::k_ulInvalidInputValueHandle);
            break;
        }
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
