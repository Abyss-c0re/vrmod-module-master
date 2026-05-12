#include "vrmod_state.h"

// VR system interfaces
vr::IVRSystem*          g_pSystem = NULL;
vr::IVRInput*           g_pInput = NULL;
vr::IVRCompositor*      g_compositor = nullptr;

// Pose tracking
vr::TrackedDevicePose_t g_poses[vr::k_unMaxTrackedDeviceCount];

// Action system
actionSet               g_actionSets[MAX_ACTIONSETS];
int                     g_actionSetCount = 0;
vr::VRActiveActionSet_t g_activeActionSets[MAX_ACTIONSETS];
int                     g_activeActionSetCount = 0;
action                  g_actions[MAX_ACTIONS];
int                     g_actionCount = 0;

// General
char                    g_errorString[MAX_STR_LEN];

// Rendering - OpenGL hook
char                    g_createTextureOrigBytes[14];
void*                   g_createTexture = NULL;
unsigned int            g_sharedTexture = 0;
COpenGLEntryPoints*     g_GL = NULL;
bool                    g_IsPatched = false;

// Rendering - OpenVR submission
vr::VRTextureBounds_t   g_textureBoundsLeft;
vr::VRTextureBounds_t   g_textureBoundsRight;
vr::Texture_t           g_vrTexture;
uint32_t                g_recommendedWidth = 0;
uint32_t                g_recommendedHeight = 0;

// Lua state
int                     g_luaRefs[LuaRefIndex_Max];
int                     g_luaRefCount = 0;
bool                    g_IsPaused = false;
