#include "vr_input.h"
#include "vrmod/types.h"
#include "vrmod/globals.h"
#include "vrmod/logging.h"
#include <cstdio>
#include <cstring>

bool VRInput_SetActionManifest(const char* fullPath, char* errorOut, size_t errorLen) {
    if (g_pInput->SetActionManifestPath(fullPath) != vr::VRInputError_None) {
        snprintf(errorOut, errorLen, "VRMOD: SetActionManifestPath failed");
        return false;
    }

    FILE* file = fopen(fullPath, "r");
    if (file == NULL) {
        snprintf(errorOut, errorLen, "VRMOD: failed to open action manifest");
        return false;
    }

    VRMOD_LOG_INFO("Parsing action manifest: %s", fullPath);

    memset(g_actions, 0, sizeof(g_actions));
    g_actionCount = 0;

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
            for (int i = 0; typeStr[i]; i++)
                g_actions[g_actionCount].type += typeStr[i];
        }
        if (g_actions[g_actionCount].fullname[0] && g_actions[g_actionCount].type) {
            VRMOD_LOG_DEBUG("Parsed action[%d]: %s (type=%d)", g_actionCount, g_actions[g_actionCount].fullname, g_actions[g_actionCount].type);
            g_actionCount++;
            if (g_actionCount == MAX_ACTIONS)
                break;
        }
    }
    fclose(file);
    VRMOD_LOG_INFO("Total actions parsed: %d", g_actionCount);
    return true;
}

void VRInput_SetActiveActionSets(const char** names, int count) {
    g_activeActionSetCount = 0;
    for (int i = 0; i < count && i < MAX_ACTIONSETS; i++) {
        int actionSetIndex = -1;
        for (int j = 0; j < g_actionSetCount; j++) {
            if (strcmp(names[i], g_actionSets[j].name) == 0) {
                actionSetIndex = j;
                break;
            }
        }
        if (actionSetIndex == -1) {
            g_pInput->GetActionSetHandle(names[i], &g_actionSets[g_actionSetCount].handle);
            memcpy(g_actionSets[g_actionSetCount].name, names[i], strlen(names[i]));
            actionSetIndex = g_actionSetCount;
            g_actionSetCount++;
        }
        g_activeActionSets[g_activeActionSetCount].ulActionSet = g_actionSets[actionSetIndex].handle;
        g_activeActionSetCount++;
    }
    VRMOD_LOG_DEBUG("Active action sets: %d", g_activeActionSetCount);
}

void VRInput_UpdatePosesAndActions() {
    g_compositor->WaitGetPoses(g_poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
    g_pInput->UpdateActionState(g_activeActionSets, sizeof(vr::VRActiveActionSet_t), g_activeActionSetCount);
}

void VRInput_TriggerHaptic(const char* actionName, float delay, float duration, float frequency, float amplitude) {
    for (int i = 0; i < g_actionCount; i++) {
        if (strcmp(g_actions[i].name, actionName) == 0) {
            g_pInput->TriggerHapticVibrationAction(g_actions[i].handle, delay, duration, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
            VRMOD_LOG_DEBUG("Haptic: %s (d=%.2f dur=%.2f f=%.1f a=%.2f)", actionName, delay, duration, frequency, amplitude);
            break;
        }
    }
}

int VRInput_GetTrackedDeviceNames(char outNames[][256], int maxNames) {
    int count = 0;
    char name[MAX_STR_LEN];
    for (int i = 0; i < (int)vr::k_unMaxTrackedDeviceCount && count < maxNames; i++) {
        if (g_pSystem->GetStringTrackedDeviceProperty(i, vr::Prop_ControllerType_String, name, MAX_STR_LEN) > 1) {
            memcpy(outNames[count], name, MAX_STR_LEN);
            count++;
        }
    }
    return count;
}
