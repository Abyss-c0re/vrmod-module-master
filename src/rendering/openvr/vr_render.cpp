#include "vr_render.h"
#include "input/vr_input.h"
#include "core/vrmod_log.h"
#include "rendering/opengl/gl_hooks.h"

#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstring>

vr::IVRCompositor*      g_compositor = nullptr;
vr::VRTextureBounds_t   g_textureBoundsLeft{};
vr::VRTextureBounds_t   g_textureBoundsRight{};
vr::Texture_t           g_vrTexture{};
uint32_t                recommendedWidth = 0;
uint32_t                recommendedHeight = 0;

// ── Backend quality ladder (no Lua surface change) ──
// deep-research-8: reduce-work + drops → compositor interleaved reprojection;
//                  glFlush after Submit; light HMD pos smooth; pose prediction.
static bool  s_reduceWork = false;
static bool  s_interleavedOn = false;
static float s_posePredictSec = 0.f;
static int   s_dropStreak = 0;
static int   s_okStreak = 0;

// HMD position EMA (Source space after ConvertPose)
static bool  s_hmdSmoothInit = false;
static float s_hmdSmoothPos[3] = {0, 0, 0};
// ~0.12 equivalent: blend = 1 - smooth; lower smooth = snappier
static constexpr float kHmdSmoothAmt = 0.12f;
static constexpr float kHmdSmoothCutoffSpeed = 0.35f; // m/s in Source units after convert

void UpdateRecommendedSize() {
    g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
    // Return *raw* HMD recommended per-eye size so Lua can supersample first,
    // then clamp SBS to 4096 once. Pre-crushing here made SS a no-op (potato).
    if (recommendedWidth < 16) recommendedWidth = 512;
    if (recommendedHeight < 16) recommendedHeight = 512;
    // Sanity ceiling only (absurd driver values)
    if (recommendedWidth > 8192) recommendedWidth = 8192;
    if (recommendedHeight > 8192) recommendedHeight = 8192;
    VRMOD_LOG_INFO("UpdateRecommendedSize: raw HMD eye %ux%u (Lua applies SS + 4096 SBS clamp)",
                   recommendedWidth, recommendedHeight);
}

void TickRenderQualityLadder() {
    if (!g_compositor)
        return;

    // 1) System reduce-work flag (IVRSystem)
    s_reduceWork = false;
    if (g_pSystem)
        s_reduceWork = g_pSystem->ShouldApplicationReduceRenderingWork();

    // 2) Frame timing: dropped frames / long wait → stress
    vr::Compositor_FrameTiming ft{};
    ft.m_nSize = sizeof(ft);
    bool stress = s_reduceWork;
    if (g_compositor->GetFrameTiming(&ft, 0)) {
        if (ft.m_nNumDroppedFrames > 0)
            s_dropStreak = std::min(s_dropStreak + (int)ft.m_nNumDroppedFrames, 30);
        else
            s_dropStreak = std::max(0, s_dropStreak - 1);

        if (ft.m_flWaitForPresentCpuMs > 2.0f || ft.m_flClientFrameIntervalMs > 16.0f)
            stress = true;
        if (s_dropStreak >= 2)
            stress = true;

        // Pose prediction toward photons (clamped)
        float remain = g_compositor->GetFrameTimeRemaining();
        if (remain > 0.f && remain < 0.05f)
            s_posePredictSec = remain;
        else if (ft.m_flClientFrameIntervalMs > 1.f && ft.m_flClientFrameIntervalMs < 40.f)
            s_posePredictSec = (ft.m_flClientFrameIntervalMs * 0.001f) * 0.5f;
        else
            s_posePredictSec = 0.011f; // ~half frame @90Hz fallback
    } else {
        s_posePredictSec = 0.011f;
    }

    if (stress)
        s_okStreak = 0;
    else
        s_okStreak = std::min(s_okStreak + 1, 60);

    // 3) Interleaved reprojection under load (compositor motion smoothing assist)
    //    Clear only after sustained good frames so we don't thrash the flag.
    bool wantInterleaved = stress || s_dropStreak >= 2;
    if (wantInterleaved && !s_interleavedOn) {
        g_compositor->ForceInterleavedReprojectionOn(true);
        s_interleavedOn = true;
        static int n = 0;
        if (++n <= 5 || (n % 300) == 0)
            VRMOD_LOG_INFO("Quality ladder: interleaved reprojection ON (reduce=%d drops=%d)",
                           s_reduceWork ? 1 : 0, s_dropStreak);
    } else if (!wantInterleaved && s_interleavedOn && s_okStreak >= 45) {
        g_compositor->ForceInterleavedReprojectionOn(false);
        s_interleavedOn = false;
        VRMOD_LOG_INFO("Quality ladder: interleaved reprojection OFF (stable)");
    }
}

float GetPosePredictionSeconds() {
    return s_posePredictSec;
}

void SmoothHmdPoseIfEnabled(PoseResult& pr) {
    if (!pr.valid)
        return;

    float speed = std::sqrt(pr.vel[0] * pr.vel[0] + pr.vel[1] * pr.vel[1] + pr.vel[2] * pr.vel[2]);
    if (speed >= kHmdSmoothCutoffSpeed) {
        s_hmdSmoothPos[0] = pr.pos[0];
        s_hmdSmoothPos[1] = pr.pos[1];
        s_hmdSmoothPos[2] = pr.pos[2];
        s_hmdSmoothInit = true;
        return;
    }

    if (!s_hmdSmoothInit) {
        s_hmdSmoothPos[0] = pr.pos[0];
        s_hmdSmoothPos[1] = pr.pos[1];
        s_hmdSmoothPos[2] = pr.pos[2];
        s_hmdSmoothInit = true;
        return;
    }

    // Exponential blend toward raw (higher amt → more lag / smoother)
    const float a = std::clamp(1.f - kHmdSmoothAmt, 0.05f, 1.f);
    s_hmdSmoothPos[0] += (pr.pos[0] - s_hmdSmoothPos[0]) * a;
    s_hmdSmoothPos[1] += (pr.pos[1] - s_hmdSmoothPos[1]) * a;
    s_hmdSmoothPos[2] += (pr.pos[2] - s_hmdSmoothPos[2]) * a;
    pr.pos[0] = s_hmdSmoothPos[0];
    pr.pos[1] = s_hmdSmoothPos[1];
    pr.pos[2] = s_hmdSmoothPos[2];
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
    // OpenVR OpenGL contract: flush after dual Submit so the compositor can
    // acquire the shared texture without missing the frame (deep-research-8).
    glFlush();
    ClearBlitReady();
    return res;
}
