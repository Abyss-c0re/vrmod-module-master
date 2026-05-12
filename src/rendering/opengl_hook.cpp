#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <gmod/Interface.h>
#include <openvr/openvr.h>

#include "opengl_hook.h"
#include "common/vrmod_types.h"
#include "common/vrmod_state.h"
#include "common/vrmod_log.h"

void BuildCreateTextureHookPatch(void* CreateTextureHookAddr, uint8_t outPatch[HOOK_SIZE]) {
    uint64_t addr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(CreateTextureHookAddr));
    uint32_t low  = static_cast<uint32_t>(addr & 0xFFFFFFFF);
    uint32_t high = static_cast<uint32_t>((addr >> 32) & 0xFFFFFFFF);

    // push imm32
    outPatch[0] = 0x68;
    std::memcpy(&outPatch[1], &low, sizeof(low));

    // mov dword ptr [rsp+4], imm32
    outPatch[5] = 0xC7;
    outPatch[6] = 0x44;
    outPatch[7] = 0x24;
    outPatch[8] = 0x04;
    std::memcpy(&outPatch[9], &high, sizeof(high));

    // ret
    outPatch[13] = 0xC3;
}

void CreateTextureHook(GLsizei n, GLuint *textures) {
    memcpy((void*)g_createTexture, (void*)g_createTextureOrigBytes, 14);
    ((glGenTextures_t)g_createTexture)(n, textures);
    g_sharedTexture = textures[0];
}

bool RemoveTexturePatch(GarrysMod::Lua::ILuaBase* LUA) {
    if (!g_IsPatched) {
        LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
        LUA->GetField(-1, "print");
        LUA->PushString("VRMOD: Patch not applied, nothing to remove.");
        LUA->Call(1, 0);
        LUA->Pop(1);
        return true;
    }

    uintptr_t addr     = reinterpret_cast<uintptr_t>(g_createTexture);
    size_t    pageSize = getpagesize();
    uintptr_t start    = addr & ~(pageSize - 1);
    uintptr_t end      = (addr + HOOK_SIZE + pageSize - 1) & ~(pageSize - 1);
    size_t    len      = end - start;

    if (mprotect((void*)start, len, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LUA->ThrowError("VRMOD: Failed to set memory protection to RWX for unpatch.");
        return false;
    }

    // Restore original bytes
    std::memcpy((void*)addr, g_createTextureOrigBytes, HOOK_SIZE);

    // Verify restoration
    if (std::memcmp((void*)addr, g_createTextureOrigBytes, HOOK_SIZE) != 0) {
        LUA->ThrowError("VRMOD: Failed to verify unpatch — bytes mismatch.");
        // Still try to set protection back
        mprotect((void*)start, len, PROT_READ | PROT_EXEC);
        return false;
    }

    // Reset memory protection
    if (mprotect((void*)start, len, PROT_READ | PROT_EXEC) != 0) {
        LUA->ThrowError("VRMOD: Failed to reset memory protection after unpatch.");
        return false;
    }

    g_IsPatched = false;
    return true;
}

LUA_FUNCTION(ShareTextureBegin) {
    // Tear down previous
    if (glIsTexture(g_sharedTexture)) {
        glDeleteTextures(1, &g_sharedTexture);
        g_sharedTexture = 0;
        memset(&g_textureBoundsLeft,  0, sizeof(g_textureBoundsLeft));
        memset(&g_textureBoundsRight, 0, sizeof(g_textureBoundsRight));
        g_vrTexture = { nullptr, vr::TextureType_Invalid, vr::ColorSpace_Auto };
        glFlush();
    }

    // Generate & bind new texture
    glGenTextures(1, &g_sharedTexture);
    glBindTexture(GL_TEXTURE_2D, g_sharedTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Allocate storage (RGBA8) – contents undefined until we clear
    glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    g_recommendedWidth * 2,
    g_recommendedHeight,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    nullptr
);

    // Clamp to transparent border + linear filtering
    GLfloat borderColor[4] = {0,0,0,0};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, g_sharedTexture);

    // Hook‑patch logic unchanged
    if (g_IsPatched)
        return 0;

    uint8_t patch[HOOK_SIZE];
    void*    hookAddr = reinterpret_cast<void*>(
                           reinterpret_cast<uintptr_t>(CreateTextureHook));
    BuildCreateTextureHookPatch(hookAddr, patch);

    uintptr_t addr     = reinterpret_cast<uintptr_t>(g_createTexture);
    size_t    pageSize = getpagesize();
    uintptr_t startPg  = addr & ~(pageSize - 1);
    uintptr_t endPg    = (addr + HOOK_SIZE + pageSize - 1) & ~(pageSize - 1);
    size_t    length   = endPg - startPg;

    if (mprotect(reinterpret_cast<void*>(startPg),
                 length,
                 PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
        LUA->ThrowError("VRMOD: mprotect RWX failed");
        return 0;
    }

    memcpy(g_createTextureOrigBytes,
           reinterpret_cast<void*>(addr),
           HOOK_SIZE);
    memcpy(reinterpret_cast<void*>(addr),
           patch,
           HOOK_SIZE);

    g_IsPatched = true;
    return 0;
}

LUA_FUNCTION(ShareTextureFinish) {
    if (g_sharedTexture == 0 || !glIsTexture(g_sharedTexture)) {
        LUA->ThrowError("VRMOD: Failed to generate shared texture.");
        return 0;
    }
    g_vrTexture.handle = (void*)(uintptr_t)g_sharedTexture;
    g_vrTexture.eType = vr::TextureType_OpenGL;
    g_vrTexture.eColorSpace = vr::ColorSpace_Gamma;

    if (!RemoveTexturePatch(LUA))
    {
        LUA->ThrowError("VRMOD: Failed to remove the texture path.");
    }

    return 0;
}
