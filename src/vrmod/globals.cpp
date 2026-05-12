#include "globals.h"

vr::IVRSystem*          g_pSystem = NULL;
vr::IVRInput*           g_pInput = NULL;
vr::IVRCompositor*      g_compositor = nullptr;
vr::TrackedDevicePose_t g_poses[vr::k_unMaxTrackedDeviceCount];
actionSet               g_actionSets[MAX_ACTIONSETS];
int                     g_actionSetCount = 0;
vr::VRActiveActionSet_t g_activeActionSets[MAX_ACTIONSETS];
int                     g_activeActionSetCount = 0;
action                  g_actions[MAX_ACTIONS];
int                     g_actionCount = 0;
char                    g_errorString[MAX_STR_LEN];
vr::VRTextureBounds_t   g_textureBoundsLeft;
vr::VRTextureBounds_t   g_textureBoundsRight;
vr::Texture_t           g_vrTexture;
uint32_t                recommendedWidth = 0;
uint32_t                recommendedHeight = 0;
int                     g_luaRefs[LuaRefIndex_Max];
int                     g_luaRefCount = 0;
bool                    g_IsPaused = false;
