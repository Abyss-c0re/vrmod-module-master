#pragma once

#include "core/vrmod_common.h"

// ── Global OpenVR render state ──
extern vr::IVRCompositor*      g_compositor;
extern vr::VRTextureBounds_t   g_textureBoundsLeft;
extern vr::VRTextureBounds_t   g_textureBoundsRight;
extern vr::Texture_t           g_vrTexture;
extern uint32_t                recommendedWidth;
extern uint32_t                recommendedHeight;

// ── Display ──
void UpdateRecommendedSize();

// ── Backend quality ladder (deep-research-8) — internal only, no new Lua API ──
// Tick after WaitGetPoses / before Submit: reduce-work, drops → interleaved reprojection.
void TickRenderQualityLadder();
// Predicted seconds for GetPoseActionDataRelativeToNow (0 if unavailable).
float GetPosePredictionSeconds();
// Optional HMD position micro-smooth (in-module; does not change Lua export surface).
void SmoothHmdPoseIfEnabled(PoseResult& pr);

// ── Submit error info ──
struct SubmitResult {
    vr::EVRCompositorError errLeft;
    vr::EVRCompositorError errRight;
    bool ok() const {
        return errLeft == vr::VRCompositorError_None &&
               errRight == vr::VRCompositorError_None;
    }
};

SubmitResult SubmitFrames();
