#ifndef VRMOD_TYPES_H
#define VRMOD_TYPES_H

#include <cstdint>
#include <openvr/openvr.h>

#define MAX_STR_LEN     256
#define MAX_ACTIONS     64
#define MAX_ACTIONSETS  16
#define PI_F            3.141592654f

constexpr size_t HOOK_SIZE = 14;

enum EActionType{
    ActionType_Pose         = 439,
    ActionType_Vector1      = 708,
    ActionType_Vector2      = 709,
    ActionType_Boolean      = 736,
    ActionType_Skeleton     = 869,
    ActionType_Vibration    = 974,
};

enum ELuaRefIndex{
    LuaRefIndex_EmptyTable,
    LuaRefIndex_PoseTable,
    LuaRefIndex_HmdPose,
    LuaRefIndex_ActionTable,
    LuaRefIndex_Max,
};

typedef struct {
    vr::VRActionHandle_t handle;
    char fullname[MAX_STR_LEN];
    int luaRefs[2];
    char* name;
    int type;
} action;

typedef struct {
    vr::VRActionSetHandle_t handle;
    char name[MAX_STR_LEN];
} actionSet;

typedef struct{
    void ClearEntryPoints();
    uint64_t m_nTotalGLCycles, m_nTotalGLCalls;
    int unknown1;
    int unknown2;
    int m_nOpenGLVersionMajor;
    int m_nOpenGLVersionMinor;
    int m_nOpenGLVersionPatch;
    bool m_bHave_OpenGL;
    char *m_pGLDriverStrings[4];
    int m_nDriverProvider;
    void *firstFunc;
} COpenGLEntryPoints;

typedef void *(*GL_GetProcAddressCallbackFunc_t)(const char *, bool &, const bool, void *);
typedef COpenGLEntryPoints*(*GetOpenGLEntryPoints_t)(GL_GetProcAddressCallbackFunc_t callback);

#endif
