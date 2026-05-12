#ifndef VRMOD_VR_INPUT_H
#define VRMOD_VR_INPUT_H

struct lua_State;

// LUA_FUNCTION wrappers (callable across translation units)
int SetActionManifest(lua_State* L);
int SetActiveActionSets(lua_State* L);
int UpdatePosesAndActions(lua_State* L);
int GetPoses(lua_State* L);
int GetActions(lua_State* L);
int TriggerHaptic(lua_State* L);
int GetTrackedDeviceNames(lua_State* L);

// Extracted testable helpers (pure functions, no external deps)
struct PoseResult {
    float pos[3];    // x, y, z in game coordinates
    float vel[3];    // velocity in game coordinates
    float ang[3];    // pitch, yaw, roll in degrees
    float angvel[3]; // angular velocity in degrees/s
};

PoseResult ExtractPoseData(const float mat[3][4],
                           const float velocity[3],
                           const float angularVelocity[3]);

int ComputeActionTypeFromString(const char* typeStr);

#endif
