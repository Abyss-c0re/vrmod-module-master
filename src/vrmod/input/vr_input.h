#pragma once
#include <cstddef>

// Parse action manifest file at the given absolute path.
// Populates g_actions/g_actionCount using g_pInput for handle registration.
// Does NOT create Lua references (caller must do that separately).
// Returns true on success, false on error (message in errorOut).
bool VRInput_SetActionManifest(const char* fullPath, char* errorOut, size_t errorLen);

// Set active action sets by name.
// names is an array of count null-terminated strings.
void VRInput_SetActiveActionSets(const char** names, int count);

// Call compositor WaitGetPoses and input UpdateActionState.
void VRInput_UpdatePosesAndActions();

// Trigger haptic vibration on the named action.
void VRInput_TriggerHaptic(const char* actionName, float delay, float duration, float frequency, float amplitude);

// Get tracked device names into outNames. Returns count found.
int VRInput_GetTrackedDeviceNames(char outNames[][256], int maxNames);
