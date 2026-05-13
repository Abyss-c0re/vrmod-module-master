#include "vr_render.h"
#include "input/vr_input.h"
#include "core/vrmod_log.h"

#include <algorithm>

// ── Global OpenVR render state definitions ──
vr::IVRCompositor*      g_compositor = nullptr;
vr::VRTextureBounds_t   g_textureBoundsLeft;
vr::VRTextureBounds_t   g_textureBoundsRight;
vr::Texture_t           g_vrTexture;
uint32_t                recommendedWidth = 0;
uint32_t                recommendedHeight = 0;

void UpdateRecommendedSize() {
    g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);

    const int maxTexSize = 4096;

    uint32_t eyeWidth = recommendedWidth;
    uint32_t eyeHeight = recommendedHeight;

    uint32_t totalWidth = eyeWidth * 2;

    float wScale = (float)maxTexSize / totalWidth;
    float hScale = (float)maxTexSize / eyeHeight;

    float scaleFactor = std::min(1.0f, std::min(wScale, hScale));

    recommendedWidth = (uint32_t)(eyeWidth * scaleFactor);
    recommendedHeight = (uint32_t)(eyeHeight * scaleFactor);

    VRMOD_LOG_DEBUG("UpdateRecommendedSize: %u x %u", recommendedWidth, recommendedHeight);
}

SubmitResult SubmitFrames() {
    SubmitResult res;
    res.errLeft = g_compositor->Submit(
        vr::Eye_Left, &g_vrTexture, &g_textureBoundsLeft, vr::Submit_Default);
    res.errRight = g_compositor->Submit(
        vr::Eye_Right, &g_vrTexture, &g_textureBoundsRight, vr::Submit_Default);
    g_compositor->PostPresentHandoff();

    if (!res.ok()) {
        VRMOD_LOG_WARN("Submit failed: Left=%d Right=%d", res.errLeft, res.errRight);
    }
    return res;
}
