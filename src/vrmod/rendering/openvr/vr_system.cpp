#include "vr_system.h"
#include "vrmod/types.h"
#include "vrmod/globals.h"
#include "vrmod/logging.h"

#include <openvr/openvr.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>

bool VR_InitSystem(char* errorOut, size_t errorLen) {
    vr::HmdError error = vr::VRInitError_None;
    g_pSystem = vr::VR_Init(&error, vr::VRApplication_Scene);
    if (error != vr::VRInitError_None) {
        snprintf(errorOut, errorLen, "%s", vr::VR_GetVRInitErrorAsEnglishDescription(error));
        return false;
    }

    g_compositor = vr::VRCompositor();
    g_pInput = vr::VRInput();

    VRMOD_LOG_INFO("OpenVR system initialized");
    return true;
}

void VR_UpdateRecommendedSize() {
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

    VRMOD_LOG_DEBUG("Recommended size: %ux%u", recommendedWidth, recommendedHeight);
}

void VR_SubmitSharedTexture(char* errorOut, size_t errorLen) {
    errorOut[0] = '\0';

    vr::EVRCompositorError errLeft =
        g_compositor->Submit(vr::Eye_Left, &g_vrTexture, &g_textureBoundsLeft, vr::Submit_Default);

    vr::EVRCompositorError errRight =
        g_compositor->Submit(vr::Eye_Right, &g_vrTexture, &g_textureBoundsRight, vr::Submit_Default);

    if (errLeft != vr::VRCompositorError_None || errRight != vr::VRCompositorError_None) {
        snprintf(errorOut, errorLen,
            "VRMOD: OpenVR Submit failed: Left: %d, Right: %d",
            (int)errLeft, (int)errRight);
        VRMOD_LOG_ERROR("%s", errorOut);
    }
    g_compositor->PostPresentHandoff();
}

void VR_ShutdownCompositor() {
    if (g_compositor) {
        g_compositor->ClearLastSubmittedFrame();
        VRMOD_LOG_INFO("Compositor shutdown");
    }
}
