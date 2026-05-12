#pragma once
#include "types.h"

extern vr::IVRSystem*          g_pSystem;
extern vr::IVRInput*           g_pInput;
extern vr::IVRCompositor*      g_compositor;
extern vr::TrackedDevicePose_t g_poses[vr::k_unMaxTrackedDeviceCount];
extern actionSet               g_actionSets[MAX_ACTIONSETS];
extern int                     g_actionSetCount;
extern vr::VRActiveActionSet_t g_activeActionSets[MAX_ACTIONSETS];
extern int                     g_activeActionSetCount;
extern action                  g_actions[MAX_ACTIONS];
extern int                     g_actionCount;
extern char                    g_errorString[MAX_STR_LEN];
extern vr::VRTextureBounds_t   g_textureBoundsLeft;
extern vr::VRTextureBounds_t   g_textureBoundsRight;
extern vr::Texture_t           g_vrTexture;
extern uint32_t                recommendedWidth;
extern uint32_t                recommendedHeight;
extern int                     g_luaRefs[LuaRefIndex_Max];
extern int                     g_luaRefCount;
extern bool                    g_IsPaused;
