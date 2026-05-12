#include <cstring>
#include <dlfcn.h>

#include <GL/gl.h>

#include <gmod/Interface.h>
#include <openvr/openvr.h>

#include "lua_interface.h"
#include "common/vrmod_types.h"
#include "common/vrmod_state.h"
#include "common/vrmod_log.h"
#include "input/vr_input.h"
#include "rendering/opengl_hook.h"
#include "rendering/openvr_submit.h"

//---------------------------------------------------------------------------
// Lua utilities
//---------------------------------------------------------------------------

// LuaPrint and PushMatrixAsTable are defined in lua_utils.cpp
// (shared between main library and test builds)

//---------------------------------------------------------------------------
// Lifecycle LUA_FUNCTIONs
//---------------------------------------------------------------------------

LUA_FUNCTION(GetVersion) {
    LUA->PushNumber(23);
    return 1;
}

LUA_FUNCTION(IsHMDPresent) {
    LUA->PushBool(vr::VR_IsHmdPresent());
    return 1;
}

LUA_FUNCTION(Init) {
        if (g_pSystem != nullptr) {
            if (g_IsPaused)
                {
                LUA->CreateTable();
                g_luaRefs[LuaRefIndex_PoseTable] = LUA->ReferenceCreate();

                LUA->CreateTable();
                g_luaRefs[LuaRefIndex_EmptyTable] = LUA->ReferenceCreate();

                LUA->CreateTable();
                g_luaRefs[LuaRefIndex_ActionTable] = LUA->ReferenceCreate();

                LUA->CreateTable();
                g_luaRefs[LuaRefIndex_HmdPose] = LUA->ReferenceCreate();
                g_IsPaused = false;
                VRMOD_LOG("VR resumed from pause");
                return 0;
            }

        }
    vr::HmdError error = vr::VRInitError_None;
    g_pSystem = vr::VR_Init(&error, vr::VRApplication_Scene);
    if (error != vr::VRInitError_None)
        LUA->ThrowError(vr::VR_GetVRInitErrorAsEnglishDescription(error));

    VRMOD_LOG("VR system initialized");

    memset(g_luaRefs, 0, sizeof(g_luaRefs));
    for (int i = 0; i < LuaRefIndex_Max; i++) {
        LUA->CreateTable();
        g_luaRefs[i] = LUA->ReferenceCreate();
        g_luaRefCount++;
    }

    void* lib = dlopen("libtogl_client.so", RTLD_NOW | RTLD_NOLOAD);
    if (!lib) LUA->ThrowError("VRMOD: dlopen failed");

    auto GetOpenGLEntryPoints = reinterpret_cast<GetOpenGLEntryPoints_t>(dlsym(lib, "GetOpenGLEntryPoints"));
    if (!GetOpenGLEntryPoints) LUA->ThrowError("VRMOD: dlsym failed");

    g_GL = GetOpenGLEntryPoints(nullptr);
    dlclose(lib);

    g_createTexture = *((void**)&g_GL->firstFunc + 50);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char buf[128];
        snprintf(buf, sizeof(buf), "VRMOD: OpenGL error: %u", err);
        LUA->ThrowError(buf);
        return 0;
    }

    g_compositor = vr::VRCompositor();
    g_pInput = vr::VRInput();

    VRMOD_LOG("VR compositor and input initialized");
    VRMOD_LOG_INIT("/tmp/vrmod_dev.log");
    return 0;
}

LUA_FUNCTION(Shutdown) {
    if(!g_compositor)
        return 0;
    if(g_IsPaused)
        return 0;
    if (g_compositor) {
        g_compositor->ClearLastSubmittedFrame();
        glFlush();
        glFinish();

        for (int i = 0; i < g_luaRefCount; i++) {
            if (g_luaRefs[i] != 0) {
                LUA->ReferenceFree(g_luaRefs[i]);
                g_luaRefs[i] = 0;
            }
        }
        for (int i = 0; i < g_actionCount; i++) {
            for (int j = 0; j < 2; j++) {
                if (g_actions[i].luaRefs[j] != 0) {
                    LUA->ReferenceFree(g_actions[i].luaRefs[j]);
                    g_actions[i].luaRefs[j] = 0;
                }
            }
        }
        g_luaRefCount =  LuaRefIndex_Max;
        g_actionCount = 0;
        memset(g_actions, 0, sizeof(g_actions));
        g_actionSetCount = 0;
        g_activeActionSetCount = 0;
        g_IsPaused = true;
        VRMOD_LOG("VR shutdown (paused)");
        VRMOD_LOG_SHUTDOWN();
    }
    return 0;
}

//---------------------------------------------------------------------------
// Module registration
//---------------------------------------------------------------------------

GMOD_MODULE_OPEN(){
    LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    LUA->GetField(-1, "vrmod");
    if (!LUA->IsType(-1, GarrysMod::Lua::Type::TABLE)) {
        LUA->Pop(1);
        LUA->CreateTable();
    }
    LUA->PushCFunction(GetVersion);
    LUA->SetField(-2, "GetVersion");
    LUA->PushCFunction(IsHMDPresent);
    LUA->SetField(-2, "IsHMDPresent");
    LUA->PushCFunction(Init);
    LUA->SetField(-2, "Init");
    LUA->PushCFunction(SetActionManifest);
    LUA->SetField(-2, "SetActionManifest");
    LUA->PushCFunction(SetActiveActionSets);
    LUA->SetField(-2, "SetActiveActionSets");
    LUA->PushCFunction(GetDisplayInfo);
    LUA->SetField(-2, "GetDisplayInfo");
    LUA->PushCFunction(UpdatePosesAndActions);
    LUA->SetField(-2, "UpdatePosesAndActions");
    LUA->PushCFunction(GetPoses);
    LUA->SetField(-2, "GetPoses");
    LUA->PushCFunction(GetActions);
    LUA->SetField(-2, "GetActions");
    LUA->PushCFunction(ShareTextureBegin);
    LUA->SetField(-2, "ShareTextureBegin");
    LUA->PushCFunction(ShareTextureFinish);
    LUA->SetField(-2, "ShareTextureFinish");
    LUA->PushCFunction(SetSubmitTextureBounds);
    LUA->SetField(-2, "SetSubmitTextureBounds");
    LUA->PushCFunction(SubmitSharedTexture);
    LUA->SetField(-2, "SubmitSharedTexture");
    LUA->PushCFunction(Shutdown);
    LUA->SetField(-2, "Shutdown");
    LUA->PushCFunction(TriggerHaptic);
    LUA->SetField(-2, "TriggerHaptic");
    LUA->PushCFunction(GetTrackedDeviceNames);
    LUA->SetField(-2, "GetTrackedDeviceNames");
    LUA->SetField(-2, "vrmod");
    return 0;
}

GMOD_MODULE_CLOSE(){
    return 0;
}
