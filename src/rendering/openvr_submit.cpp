#include <string>

#include <gmod/Interface.h>
#include <openvr/openvr.h>

#include "openvr_submit.h"
#include "common/vrmod_types.h"
#include "common/vrmod_state.h"
#include "common/vrmod_log.h"
#include "lua/lua_interface.h"

void UpdateRecommendedSize() {
    g_pSystem->GetRecommendedRenderTargetSize(&g_recommendedWidth, &g_recommendedHeight);

    const int maxTexSize = 4096;

    uint32_t eyeWidth = g_recommendedWidth;
    uint32_t eyeHeight = g_recommendedHeight;

    uint32_t totalWidth = eyeWidth * 2;

    float wScale = (float)maxTexSize / totalWidth;
    float hScale = (float)maxTexSize / eyeHeight;

    float scaleFactor = std::min(1.0f, std::min(wScale, hScale));

    g_recommendedWidth = (uint32_t)(eyeWidth * scaleFactor);
    g_recommendedHeight = (uint32_t)(eyeHeight * scaleFactor);
}

LUA_FUNCTION(GetDisplayInfo) {
    UpdateRecommendedSize();
    float fNearZ = (float)LUA->CheckNumber(1);
    float fFarZ = (float)LUA->CheckNumber(2);
    vr::HmdMatrix44_t projLeft = g_pSystem->GetProjectionMatrix(vr::Hmd_Eye::Eye_Left, fNearZ, fFarZ);
    vr::HmdMatrix44_t projRight = g_pSystem->GetProjectionMatrix(vr::Hmd_Eye::Eye_Right, fNearZ, fFarZ);
    vr::HmdMatrix34_t transformLeft = g_pSystem->GetEyeToHeadTransform(vr::Eye_Left);
    vr::HmdMatrix34_t transformRight = g_pSystem->GetEyeToHeadTransform(vr::Eye_Right);
    LUA->CreateTable();
    PushMatrixAsTable(LUA, (float*)&projLeft, 4, 4);
    LUA->SetField(-2, "ProjectionLeft");
    PushMatrixAsTable(LUA, (float*)&projRight, 4, 4);
    LUA->SetField(-2, "ProjectionRight");
    PushMatrixAsTable(LUA, (float*)&transformLeft, 3, 4);
    LUA->SetField(-2, "TransformLeft");
    PushMatrixAsTable(LUA, (float*)&transformRight, 3, 4);
    LUA->SetField(-2, "TransformRight");
    LUA->PushNumber(g_recommendedWidth);
    LUA->SetField(-2, "RecommendedWidth");
    LUA->PushNumber(g_recommendedHeight);
    LUA->SetField(-2, "RecommendedHeight");
    return 1;
}

LUA_FUNCTION(SetSubmitTextureBounds) {
    g_textureBoundsLeft.uMin  = (float)LUA->CheckNumber(1);
    g_textureBoundsLeft.vMin  = (float)LUA->CheckNumber(2);
    g_textureBoundsLeft.uMax  = (float)LUA->CheckNumber(3);
    g_textureBoundsLeft.vMax  = (float)LUA->CheckNumber(4);

    g_textureBoundsRight.uMin = (float)LUA->CheckNumber(5);
    g_textureBoundsRight.vMin = (float)LUA->CheckNumber(6);
    g_textureBoundsRight.uMax = (float)LUA->CheckNumber(7);
    g_textureBoundsRight.vMax = (float)LUA->CheckNumber(8);

    return 0;
}

LUA_FUNCTION(SubmitSharedTexture)
{
    vr::EVRCompositorError errLeft =
        g_compositor->Submit(vr::Eye_Left, &g_vrTexture, &g_textureBoundsLeft, vr::Submit_Default);

    vr::EVRCompositorError errRight =
        g_compositor->Submit(vr::Eye_Right, &g_vrTexture, &g_textureBoundsRight, vr::Submit_Default);

    if (errLeft != vr::VRCompositorError_None || errRight != vr::VRCompositorError_None)
    {
        std::string errMsg =
            "VRMOD: OpenVR Submit failed: Left: " + std::to_string(errLeft) +
            ", Right: " + std::to_string(errRight);
        LuaPrint(LUA, errMsg.c_str());
    }
    g_compositor->PostPresentHandoff();
    return 0;
}
