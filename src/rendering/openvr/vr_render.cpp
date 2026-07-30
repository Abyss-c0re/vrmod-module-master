#include "vr_render.h"
#include "input/vr_input.h"
#include "core/vrmod_log.h"
#include "rendering/opengl/gl_hooks.h"

#include <algorithm>
#include <cmath>

vr::IVRCompositor*      g_compositor = nullptr;
vr::VRTextureBounds_t   g_textureBoundsLeft{};
vr::VRTextureBounds_t   g_textureBoundsRight{};
vr::Texture_t           g_vrTexture{};
uint32_t                recommendedWidth = 0;
uint32_t                recommendedHeight = 0;

void UpdateRecommendedSize() {
    g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
    // Cap SBS for shared-image memory (4096 → 0x0 alloc fail)
    const int maxTexSize = 2048;
    uint32_t eyeWidth = recommendedWidth;
    uint32_t eyeHeight = recommendedHeight;
    uint32_t totalWidth = eyeWidth * 2;
    float wScale = (float)maxTexSize / (float)totalWidth;
    float hScale = (float)maxTexSize / (float)eyeHeight;
    float scaleFactor = std::min(1.0f, std::min(wScale, hScale));
    recommendedWidth = (uint32_t)(eyeWidth * scaleFactor);
    recommendedHeight = (uint32_t)(eyeHeight * scaleFactor);
    if (recommendedWidth < 16) recommendedWidth = 512;
    if (recommendedHeight < 16) recommendedHeight = 512;
    VRMOD_LOG_DEBUG("UpdateRecommendedSize: %u x %u", recommendedWidth, recommendedHeight);
}

static void ClampBounds(vr::VRTextureBounds_t& b, bool left) {
    auto cl = [](float v, float lo, float hi) {
        if (v != v) return lo;
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    };
    if (left) {
        b.uMin = cl(b.uMin, 0.f, 0.49f);
        b.uMax = cl(b.uMax, 0.01f, 0.5f);
        if (b.uMax - b.uMin < 0.05f) { b.uMin = 0.003f; b.uMax = 0.5f; }
    } else {
        b.uMin = cl(b.uMin, 0.5f, 0.99f);
        b.uMax = cl(b.uMax, 0.51f, 1.f);
        if (b.uMax - b.uMin < 0.05f) { b.uMin = 0.5f; b.uMax = 0.997f; }
    }
    b.vMin = cl(b.vMin, 0.f, 1.f);
    b.vMax = cl(b.vMax, 0.f, 1.f);
    if (std::fabs(b.vMax - b.vMin) < 0.05f) {
        b.vMin = 0.997f;
        b.vMax = 0.003f;
    }
}

static void EnsureDefaultBounds() {
    if (g_textureBoundsLeft.uMin == 0.f && g_textureBoundsLeft.uMax == 0.f &&
        g_textureBoundsLeft.vMin == 0.f && g_textureBoundsLeft.vMax == 0.f) {
        g_textureBoundsLeft  = {0.003f, 0.997f, 0.5f, 0.003f};
        g_textureBoundsRight = {0.5f, 0.997f, 0.997f, 0.003f};
    }
    ClampBounds(g_textureBoundsLeft, true);
    ClampBounds(g_textureBoundsRight, false);
}

SubmitResult SubmitFrames() {
    SubmitResult res{};
    res.errLeft = vr::VRCompositorError_RequestFailed;
    res.errRight = vr::VRCompositorError_RequestFailed;
    if (!g_compositor) return res;

    // Transfer eng IN → dual OUT (always at known size; never trust ToGL GetTexLevel)
    bool blitOk = PrepareSubmitTexture();

    GLuint outTex = g_submitTexture;
    if (outTex == 0 || outTex == g_engineTexture) {
        static int n = 0;
        if (++n <= 5 || (n % 180) == 0)
            VRMOD_LOG_ERROR("No dual OUT texture — refuse eng Submit (would 105)");
        ClearBlitReady();
        return res; // RequestFailed — visible, not hidden
    }

    if (!blitOk) {
        static int n = 0;
        if (++n <= 8 || (n % 180) == 0)
            VRMOD_LOG_WARN("Blit eng→OUT failed this frame — still Submit last OUT (truthful)");
        // Fall through: last good dual frame may still be valid size for LizardTech
    }

    g_vrTexture.handle = (void*)(uintptr_t)outTex;
    g_vrTexture.eType = vr::TextureType_OpenGL;
    g_vrTexture.eColorSpace = vr::ColorSpace_Gamma;
    EnsureDefaultBounds();

    res.errLeft = g_compositor->Submit(vr::Eye_Left, &g_vrTexture, &g_textureBoundsLeft, vr::Submit_Default);
    res.errRight = g_compositor->Submit(vr::Eye_Right, &g_vrTexture, &g_textureBoundsRight, vr::Submit_Default);

    static int fail = 0, ok = 0;
    if (!res.ok()) {
        if (++fail <= 8 || (fail % 120) == 0)
            VRMOD_LOG_WARN("Submit OUT L=%d R=%d streak=%d tex=%u %ux%u blit=%d",
                           (int)res.errLeft, (int)res.errRight, fail,
                           (unsigned)outTex, g_submitTexW, g_submitTexH, blitOk ? 1 : 0);
    } else {
        fail = 0;
        if (++ok <= 8 || (ok % 300) == 0)
            VRMOD_LOG_INFO("Submit OUT ok #%d tex=%u blit=%d", ok, (unsigned)outTex, blitOk ? 1 : 0);
    }

    g_compositor->PostPresentHandoff();
    ClearBlitReady();
    return res;
}
