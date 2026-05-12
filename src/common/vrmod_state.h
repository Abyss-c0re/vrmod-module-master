#ifndef VRMOD_STATE_H
#define VRMOD_STATE_H

#include "vrmod_types.h"

// VR system interfaces
extern vr::IVRSystem*          g_pSystem;
extern vr::IVRInput*           g_pInput;
extern vr::IVRCompositor*      g_compositor;

// Pose tracking
extern vr::TrackedDevicePose_t g_poses[vr::k_unMaxTrackedDeviceCount];

// Action system
extern actionSet               g_actionSets[MAX_ACTIONSETS];
extern int                     g_actionSetCount;
extern vr::VRActiveActionSet_t g_activeActionSets[MAX_ACTIONSETS];
extern int                     g_activeActionSetCount;
extern action                  g_actions[MAX_ACTIONS];
extern int                     g_actionCount;

// General
extern char                    g_errorString[MAX_STR_LEN];

// Rendering - OpenGL hook
extern char                    g_createTextureOrigBytes[14];
extern void*                   g_createTexture;
extern unsigned int             g_sharedTexture;
extern COpenGLEntryPoints*     g_GL;
extern bool                    g_IsPatched;

// Rendering - OpenVR submission
extern vr::VRTextureBounds_t   g_textureBoundsLeft;
extern vr::VRTextureBounds_t   g_textureBoundsRight;
extern vr::Texture_t           g_vrTexture;
extern uint32_t                g_recommendedWidth;
extern uint32_t                g_recommendedHeight;

// Lua state
extern int                     g_luaRefs[LuaRefIndex_Max];
extern int                     g_luaRefCount;
extern bool                    g_IsPaused;

#endif
