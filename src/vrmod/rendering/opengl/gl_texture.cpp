#include "gl_texture.h"
#include "vrmod/types.h"
#include "vrmod/globals.h"
#include "vrmod/logging.h"

#include <cstring>
#include <cstdio>
#include <sys/mman.h>
#include <dlfcn.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

/******************************************************************************
 * GL-module-local types and globals
 *****************************************************************************/

typedef struct{
    void ClearEntryPoints();
    uint64_t m_nTotalGLCycles, m_nTotalGLCalls;
    int unknown1;
    int unknown2;
    int m_nOpenGLVersionMajor;
    int m_nOpenGLVersionMinor;
    int m_nOpenGLVersionPatch;
    bool m_bHave_OpenGL;
    char *m_pGLDriverStrings[4];
    int m_nDriverProvider;
    void *firstFunc;
} COpenGLEntryPoints;

typedef void *(*GL_GetProcAddressCallbackFunc_t)(const char *, bool &, const bool, void *);
typedef COpenGLEntryPoints*(*GetOpenGLEntryPoints_t)(GL_GetProcAddressCallbackFunc_t callback);
typedef void (*glGenTextures_t)(GLsizei n, GLuint *textures);

static char             g_createTextureOrigBytes[14];
static void*            g_createTexture = NULL;
static GLuint           g_sharedTexture = 0;
static COpenGLEntryPoints* g_GL = NULL;
static bool             s_IsPatched = false;

/******************************************************************************
 * Hook patch building - RENDERING LOGIC PRESERVED EXACTLY
 *****************************************************************************/

static void BuildCreateTextureHookPatch(void* CreateTextureHook, uint8_t outPatch[HOOK_SIZE]) {
    uint64_t addr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(CreateTextureHook));
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

/******************************************************************************
 * Texture hook callback - RENDERING LOGIC PRESERVED EXACTLY
 *****************************************************************************/

static void CreateTextureHook(GLsizei n, GLuint *textures) {
    memcpy((void*)g_createTexture, (void*)g_createTextureOrigBytes, 14);
    ((glGenTextures_t)g_createTexture)(n, textures);
    g_sharedTexture = textures[0];
}

/******************************************************************************
 * Remove texture patch - RENDERING LOGIC PRESERVED EXACTLY
 * Returns false on error, writes message to errorOut.
 *****************************************************************************/

static bool RemoveTexturePatch(char* errorOut, size_t errorLen) {
    if (!s_IsPatched) {
        VRMOD_LOG_INFO("Patch not applied, nothing to remove.");
        return true;
    }

    uintptr_t addr     = reinterpret_cast<uintptr_t>(g_createTexture);
    size_t    pageSize = getpagesize();
    uintptr_t start    = addr & ~(pageSize - 1);
    uintptr_t end      = (addr + HOOK_SIZE + pageSize - 1) & ~(pageSize - 1);
    size_t    len      = end - start;

    if (mprotect((void*)start, len, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        snprintf(errorOut, errorLen, "VRMOD: Failed to set memory protection to RWX for unpatch.");
        return false;
    }

    // Restore original bytes
    std::memcpy((void*)addr, g_createTextureOrigBytes, HOOK_SIZE);

    // Verify restoration
    if (std::memcmp((void*)addr, g_createTextureOrigBytes, HOOK_SIZE) != 0) {
        snprintf(errorOut, errorLen, "VRMOD: Failed to verify unpatch — bytes mismatch.");
        mprotect((void*)start, len, PROT_READ | PROT_EXEC);
        return false;
    }

    // Reset memory protection
    if (mprotect((void*)start, len, PROT_READ | PROT_EXEC) != 0) {
        snprintf(errorOut, errorLen, "VRMOD: Failed to reset memory protection after unpatch.");
        return false;
    }

    s_IsPatched = false;
    return true;
}

/******************************************************************************
 * Public API
 *****************************************************************************/

bool GL_Init(char* errorOut, size_t errorLen) {
    void* lib = dlopen("libtogl_client.so", RTLD_NOW | RTLD_NOLOAD);
    if (!lib) {
        snprintf(errorOut, errorLen, "VRMOD: dlopen failed");
        return false;
    }

    auto GetOpenGLEntryPoints = reinterpret_cast<GetOpenGLEntryPoints_t>(dlsym(lib, "GetOpenGLEntryPoints"));
    if (!GetOpenGLEntryPoints) {
        dlclose(lib);
        snprintf(errorOut, errorLen, "VRMOD: dlsym failed");
        return false;
    }

    g_GL = GetOpenGLEntryPoints(nullptr);
    dlclose(lib);

    g_createTexture = *((void**)&g_GL->firstFunc + 50);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        snprintf(errorOut, errorLen, "VRMOD: OpenGL error: %u", err);
        return false;
    }

    VRMOD_LOG_INFO("GL initialized, createTexture addr: %p", g_createTexture);
    return true;
}

/******************************************************************************
 * ShareTextureBegin - RENDERING LOGIC PRESERVED EXACTLY
 *****************************************************************************/

bool GL_ShareTextureBegin(uint32_t width, uint32_t height, char* errorOut, size_t errorLen) {
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

    // Allocate storage (RGBA8)
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width * 2,
        height,
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

    // Hook-patch logic
    if (s_IsPatched)
        return true;

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
        snprintf(errorOut, errorLen, "VRMOD: mprotect RWX failed");
        return false;
    }

    memcpy(g_createTextureOrigBytes,
           reinterpret_cast<void*>(addr),
           HOOK_SIZE);
    memcpy(reinterpret_cast<void*>(addr),
           patch,
           HOOK_SIZE);

    s_IsPatched = true;
    VRMOD_LOG_INFO("Texture hook patch applied");
    return true;
}

/******************************************************************************
 * ShareTextureFinish - RENDERING LOGIC PRESERVED EXACTLY
 *****************************************************************************/

bool GL_ShareTextureFinish(char* errorOut, size_t errorLen) {
    if (g_sharedTexture == 0 || !glIsTexture(g_sharedTexture)) {
        snprintf(errorOut, errorLen, "VRMOD: Failed to generate shared texture.");
        return false;
    }
    g_vrTexture.handle = (void*)(uintptr_t)g_sharedTexture;
    g_vrTexture.eType = vr::TextureType_OpenGL;
    g_vrTexture.eColorSpace = vr::ColorSpace_Gamma;

    if (!RemoveTexturePatch(errorOut, errorLen)) {
        return false;
    }

    VRMOD_LOG_INFO("Shared texture finalized: %u", g_sharedTexture);
    return true;
}

void GL_SetSubmitTextureBounds(float luMin, float lvMin, float luMax, float lvMax,
                               float ruMin, float rvMin, float ruMax, float rvMax) {
    g_textureBoundsLeft.uMin  = luMin;
    g_textureBoundsLeft.vMin  = lvMin;
    g_textureBoundsLeft.uMax  = luMax;
    g_textureBoundsLeft.vMax  = lvMax;

    g_textureBoundsRight.uMin = ruMin;
    g_textureBoundsRight.vMin = rvMin;
    g_textureBoundsRight.uMax = ruMax;
    g_textureBoundsRight.vMax = rvMax;
}

void GL_FlushAndFinish() {
    glFlush();
    glFinish();
}
