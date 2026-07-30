#include "gl_hooks.h"
#include "core/vrmod_log.h"

#include <cstring>
#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

// BlackCube → LizardTech / OpenVR compositor texture OUT
//
// Law: engine RT is IN (Source ToGL). Submit surface is OUT (module RGBA8).
// Never Submit raw eng id (compositor: format=43 dimensions=0x0 → 105).
// Never gate blit on glGetTexLevel of eng — ToGL returns 0x0 while RT is live.
// Always blit at known SBS size from ShareTextureBegin (Lua RT matches).

char                g_createTextureOrigBytes[14];
void*               g_createTexture = NULL;
GLuint              g_sharedTexture = 0;
GLuint              g_engineTexture = 0;
GLuint              g_submitTexture = 0;
uint32_t            g_submitTexW = 0;
uint32_t            g_submitTexH = 0;
COpenGLEntryPoints* g_GL = NULL;
bool                g_glIsPatched = false;

static bool g_captureArmed = false;
static bool g_blitOk = false; // last PrepareSubmitTexture blit succeeded
static bool g_everBlittedOk = false; // at least one good eng→OUT transfer this session
static GLuint g_fboRead = 0;
static GLuint g_fboDraw = 0;
static int s_log = 0;
// Actual allocated OUT size (Begin may overwrite g_submitTexW/H before Finish reallocs)
static uint32_t g_submitAllocW = 0;
static uint32_t g_submitAllocH = 0;
// Defer delete of previous OUT until after next Submit (compositor may still hold it)
static GLuint g_pendingDeleteTex = 0;

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_READ_FRAMEBUFFER_BINDING
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif

typedef void (*PFNGLBLITFRAMEBUFFER)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef void (*PFNGLBINDFRAMEBUFFER)(GLenum, GLuint);
typedef void (*PFNGLGENFRAMEBUFFERS)(GLsizei, GLuint*);
typedef void (*PFNGLDELETEFRAMEBUFFERS)(GLsizei, GLuint*);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2D)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (*PFNGLCHECKFRAMEBUFFERSTATUS)(GLenum);

static PFNGLBLITFRAMEBUFFER p_blit = nullptr;
static PFNGLBINDFRAMEBUFFER p_bindFbo = nullptr;
static PFNGLGENFRAMEBUFFERS p_genFbo = nullptr;
static PFNGLDELETEFRAMEBUFFERS p_delFbo = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2D p_fboTex = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUS p_checkFbo = nullptr;
static bool g_ext = false;

static void* LoadGL(const char* name) {
    void* p = (void*)glXGetProcAddress((const GLubyte*)name);
    if (!p) p = (void*)glXGetProcAddressARB((const GLubyte*)name);
    return p;
}

static void ResolveExt() {
    if (g_ext) return;
    g_ext = true;
    p_blit = (PFNGLBLITFRAMEBUFFER)LoadGL("glBlitFramebuffer");
    if (!p_blit) p_blit = (PFNGLBLITFRAMEBUFFER)LoadGL("glBlitFramebufferEXT");
    p_bindFbo = (PFNGLBINDFRAMEBUFFER)LoadGL("glBindFramebuffer");
    if (!p_bindFbo) p_bindFbo = (PFNGLBINDFRAMEBUFFER)LoadGL("glBindFramebufferEXT");
    p_genFbo = (PFNGLGENFRAMEBUFFERS)LoadGL("glGenFramebuffers");
    if (!p_genFbo) p_genFbo = (PFNGLGENFRAMEBUFFERS)LoadGL("glGenFramebuffersEXT");
    p_delFbo = (PFNGLDELETEFRAMEBUFFERS)LoadGL("glDeleteFramebuffers");
    if (!p_delFbo) p_delFbo = (PFNGLDELETEFRAMEBUFFERS)LoadGL("glDeleteFramebuffersEXT");
    p_fboTex = (PFNGLFRAMEBUFFERTEXTURE2D)LoadGL("glFramebufferTexture2D");
    if (!p_fboTex) p_fboTex = (PFNGLFRAMEBUFFERTEXTURE2D)LoadGL("glFramebufferTexture2DEXT");
    p_checkFbo = (PFNGLCHECKFRAMEBUFFERSTATUS)LoadGL("glCheckFramebufferStatus");
    if (!p_checkFbo) p_checkFbo = (PFNGLCHECKFRAMEBUFFERSTATUS)LoadGL("glCheckFramebufferStatusEXT");
    VRMOD_LOG_INFO("GL FBO ext blit=%d bind=%d gen=%d", p_blit ? 1 : 0, p_bindFbo ? 1 : 0, p_genFbo ? 1 : 0);
}

void BuildCreateTextureHookPatch(void* hookFn, uint8_t outPatch[HOOK_SIZE]) {
    uint64_t addr = (uint64_t)(uintptr_t)hookFn;
    uint32_t low = (uint32_t)(addr & 0xffffffffu);
    uint32_t high = (uint32_t)(addr >> 32);
    outPatch[0] = 0x68;
    memcpy(outPatch + 1, &low, 4);
    outPatch[5] = 0xC7;
    outPatch[6] = 0x44;
    outPatch[7] = 0x24;
    outPatch[8] = 0x04;
    memcpy(outPatch + 9, &high, 4);
    outPatch[13] = 0xC3;
}

static bool SetProt(uintptr_t addr, size_t len, int prot) {
    size_t page = getpagesize();
    uintptr_t start = addr & ~(page - 1);
    uintptr_t end = (addr + len + page - 1) & ~(page - 1);
    return mprotect((void*)start, end - start, prot) == 0;
}

static bool InstallPatch(ErrorFunc errFunc) {
    if (g_glIsPatched) return true;
    if (!g_createTexture) {
        if (errFunc) errFunc("VRMOD: g_createTexture null");
        return false;
    }
    uint8_t patch[HOOK_SIZE];
    BuildCreateTextureHookPatch((void*)(uintptr_t)CreateTextureHook, patch);
    uintptr_t addr = (uintptr_t)g_createTexture;
    if (!SetProt(addr, HOOK_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) &&
        !SetProt(addr, HOOK_SIZE, PROT_READ | PROT_WRITE)) {
        if (errFunc) errFunc("VRMOD: mprotect failed");
        return false;
    }
    memcpy(g_createTextureOrigBytes, (void*)addr, HOOK_SIZE);
    memcpy((void*)addr, patch, HOOK_SIZE);
    // Keep RWX while armed (CreateTextureHook restores bytes in-place)
    g_glIsPatched = true;
    return true;
}

bool RemoveTexturePatch(ErrorFunc errFunc) {
    if (!g_glIsPatched || !g_createTexture) {
        g_glIsPatched = false;
        return true;
    }
    uintptr_t addr = (uintptr_t)g_createTexture;
    if (!SetProt(addr, HOOK_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) &&
        !SetProt(addr, HOOK_SIZE, PROT_READ | PROT_WRITE)) {
        if (errFunc) errFunc("VRMOD: mprotect unpatch failed");
        return false;
    }
    memcpy((void*)addr, g_createTextureOrigBytes, HOOK_SIZE);
    SetProt(addr, HOOK_SIZE, PROT_READ | PROT_EXEC);
    g_glIsPatched = false;
    return true;
}

void CreateTextureHook(GLsizei n, GLuint* textures) {
    memcpy(g_createTexture, g_createTextureOrigBytes, 14);
    g_glIsPatched = false;
    ((glGenTextures_t)g_createTexture)(n, textures);
    if (!g_captureArmed || n <= 0 || !textures) return;
    g_engineTexture = textures[0];
    g_captureArmed = false;
    VRMOD_LOG_INFO("Captured eng RT id=%u (IN)", (unsigned)g_engineTexture);
}

static GLuint AllocRGBA8(uint32_t w, uint32_t h) {
    if (w < 16) w = 1024;
    if (h < 16) h = 1024;
    // Match UpdateRecommendedSize / Lua Linux clamp (4096 SBS max)
    if (w > 4096) w = 4096;
    if (h > 4096) h = 4096;

    if (g_glIsPatched) RemoveTexturePatch(nullptr);

    GLuint tex = 0;
    // ToGL glGenTextures — same GL context namespace as Source (native often returns 0)
    if (g_createTexture)
        ((glGenTextures_t)g_createTexture)(1, &tex);
    if (tex == 0)
        glGenTextures(1, &tex);
    if (tex == 0) {
        VRMOD_LOG_WARN("AllocRGBA8 GenTex failed");
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    while (glGetError() != GL_NO_ERROR) {}
    std::vector<unsigned char> zeros((size_t)w * (size_t)h * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Establish FBO-completeness of submit surface once (helps compositor size probe)
    ResolveExt();
    if (p_genFbo && p_bindFbo && p_fboTex && p_checkFbo) {
        GLuint fbo = 0;
        p_genFbo(1, &fbo);
        GLint prev = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);
        p_bindFbo(GL_FRAMEBUFFER, fbo);
        p_fboTex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        GLenum st = p_checkFbo(GL_FRAMEBUFFER);
        p_fboTex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        p_bindFbo(GL_FRAMEBUFFER, (GLuint)prev);
        if (p_delFbo) p_delFbo(1, &fbo);
        VRMOD_LOG_INFO("AllocRGBA8 id=%u %ux%u fboStatus=0x%x (OUT)", (unsigned)tex, w, h, (unsigned)st);
    } else {
        VRMOD_LOG_INFO("AllocRGBA8 id=%u %ux%u (OUT, no FBO probe)", (unsigned)tex, w, h);
    }

    g_submitTexW = w;
    g_submitTexH = h;
    g_submitAllocW = w;
    g_submitAllocH = h;
    return tex;
}

int ShareTextureBegin(uint32_t eyeW, uint32_t eyeH, ErrorFunc errFunc) {
    ResolveExt();
    if (eyeW == 0) eyeW = 1024;
    if (eyeH == 0) eyeH = 1024;
    uint32_t sbsW = eyeW * 2;
    uint32_t sbsH = eyeH;
    if (sbsW > 4096) {
        float scale = 4096.f / (float)sbsW;
        sbsW = 4096;
        sbsH = (uint32_t)((float)sbsH * scale);
        if (sbsH < 16) sbsH = 16;
    }
    if (sbsH > 4096) {
        float scale = 4096.f / (float)sbsH;
        sbsH = 4096;
        sbsW = (uint32_t)((float)sbsW * scale);
        if (sbsW < 16) sbsW = 16;
    }
    g_submitTexW = sbsW;
    g_submitTexH = sbsH;

    g_captureArmed = true;
    if (!g_createTexture) {
        if (errFunc) errFunc("VRMOD: g_createTexture null");
        return -1;
    }
    if (!InstallPatch(errFunc))
        return -1;
    VRMOD_LOG_INFO("ShareTextureBegin armed SBS %ux%u (IN hook + OUT dual)", sbsW, sbsH);
    return 0;
}

bool ShareTextureFinish(ErrorFunc errFunc) {
    RemoveTexturePatch(errFunc);

    const uint32_t wantW = g_submitTexW ? g_submitTexW : 4096;
    const uint32_t wantH = g_submitTexH ? g_submitTexH : 2048;

    // Dual OUT is mandatory — never fall back to eng Submit (105 UnsupportedFormat).
    // Realloc on size change; defer glDelete until after Submit so compositor is safe.
    if (g_submitTexture && g_submitTexture != g_engineTexture &&
        (g_submitAllocW != wantW || g_submitAllocH != wantH)) {
        VRMOD_LOG_INFO("ShareTextureFinish: size change %ux%u → %ux%u, realloc OUT",
                       g_submitAllocW, g_submitAllocH, wantW, wantH);
        if (g_pendingDeleteTex && g_pendingDeleteTex != g_submitTexture &&
            g_pendingDeleteTex != g_engineTexture) {
            glDeleteTextures(1, &g_pendingDeleteTex);
        }
        g_pendingDeleteTex = g_submitTexture;
        g_submitTexture = 0;
        g_submitAllocW = g_submitAllocH = 0;
        g_everBlittedOk = false; // new surface needs a fresh blit before Submit
    }

    if (!g_submitTexture || g_submitTexture == g_engineTexture) {
        g_submitTexture = AllocRGBA8(wantW, wantH);
    }
    if (!g_submitTexture) {
        if (errFunc) errFunc("VRMOD: dual RGBA8 OUT alloc failed");
        return false;
    }
    // Keep logical size in sync with allocation (Begin may have set want; Alloc clamps)
    g_submitTexW = g_submitAllocW ? g_submitAllocW : wantW;
    g_submitTexH = g_submitAllocH ? g_submitAllocH : wantH;
    g_sharedTexture = g_submitTexture;
    if (!g_engineTexture)
        VRMOD_LOG_WARN("ShareTextureFinish: eng IN not captured (hook miss) — blit will fail until recapture");
    VRMOD_LOG_INFO("ShareTextureFinish engIN=%u subOUT=%u %ux%u",
                   (unsigned)g_engineTexture, (unsigned)g_submitTexture,
                   g_submitTexW, g_submitTexH);
    return true;
}

bool ConsumeBlitReady() {
    bool r = g_blitOk;
    // do not clear here — SubmitFrames reads then we clear after Submit
    return r;
}

void ClearBlitReady() {
    g_blitOk = false;
}

bool PrepareSubmitTexture() {
    g_blitOk = false;
    if (!g_submitTexture || g_submitTexture == g_engineTexture)
        return false;
    if (!g_engineTexture)
        return false;
    if (g_submitTexW < 16 || g_submitTexH < 16)
        return false;

    ResolveExt();
    if (!p_bindFbo || !p_fboTex || !p_blit || !p_genFbo || !p_checkFbo) {
        if (s_log++ < 3)
            VRMOD_LOG_ERROR("FBO extensions missing — cannot transfer eng IN → submit OUT");
        return false;
    }

    GLint prevDraw = 0, prevRead = 0, prevTex = 0, vp[4] = {0,0,0,0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevDraw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_VIEWPORT, vp);

    if (!g_fboRead) p_genFbo(1, &g_fboRead);
    if (!g_fboDraw) p_genFbo(1, &g_fboDraw);

    // Known SBS size from Begin/Lua — NEVER use GetTexLevel on eng (ToGL lies 0x0)
    const GLsizei w = (GLsizei)g_submitTexW;
    const GLsizei h = (GLsizei)g_submitTexH;

    p_bindFbo(GL_READ_FRAMEBUFFER, g_fboRead);
    p_fboTex(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_engineTexture, 0);
    GLenum rs = p_checkFbo(GL_READ_FRAMEBUFFER);

    p_bindFbo(GL_DRAW_FRAMEBUFFER, g_fboDraw);
    p_fboTex(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_submitTexture, 0);
    GLenum ds = p_checkFbo(GL_DRAW_FRAMEBUFFER);

    if (rs == GL_FRAMEBUFFER_COMPLETE && ds == GL_FRAMEBUFFER_COMPLETE) {
        p_blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        g_blitOk = true;
        g_everBlittedOk = true;
        if (s_log++ < 10)
            VRMOD_LOG_INFO("FLOW engIN=%u → subOUT=%u %dx%d ok",
                           (unsigned)g_engineTexture, (unsigned)g_submitTexture, (int)w, (int)h);
    } else {
        if (s_log++ < 20)
            VRMOD_LOG_WARN("FLOW blit FAIL rs=0x%x ds=0x%x eng=%u sub=%u %dx%d",
                           (unsigned)rs, (unsigned)ds,
                           (unsigned)g_engineTexture, (unsigned)g_submitTexture,
                           (int)w, (int)h);
    }

    // Detach so textures are free for compositor / Source
    p_bindFbo(GL_READ_FRAMEBUFFER, g_fboRead);
    p_fboTex(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    p_bindFbo(GL_DRAW_FRAMEBUFFER, g_fboDraw);
    p_fboTex(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

    p_bindFbo(GL_READ_FRAMEBUFFER, (GLuint)prevRead);
    p_bindFbo(GL_DRAW_FRAMEBUFFER, (GLuint)prevDraw);
    p_bindFbo(GL_FRAMEBUFFER, (GLuint)prevDraw);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    // No glFinish — crashes Source under ToGL
    glFlush();
    return g_blitOk;
}

void ShareTextureReset() {
    RemoveTexturePatch(nullptr);
    if (g_fboRead && p_delFbo) { p_delFbo(1, &g_fboRead); g_fboRead = 0; }
    if (g_fboDraw && p_delFbo) { p_delFbo(1, &g_fboDraw); g_fboDraw = 0; }
    if (g_pendingDeleteTex && g_pendingDeleteTex != g_engineTexture &&
        g_pendingDeleteTex != g_submitTexture) {
        glDeleteTextures(1, &g_pendingDeleteTex);
    }
    g_pendingDeleteTex = 0;
    if (g_submitTexture && g_submitTexture != g_engineTexture)
        glDeleteTextures(1, &g_submitTexture);
    g_submitTexture = 0;
    g_sharedTexture = 0;
    g_engineTexture = 0;
    g_submitTexW = g_submitTexH = 0;
    g_submitAllocW = g_submitAllocH = 0;
    g_captureArmed = false;
    g_blitOk = false;
    g_everBlittedOk = false;
    s_log = 0;
}

bool ShareTextureHasGoodFrame() {
    return g_everBlittedOk && g_submitTexture != 0 && g_submitTexture != g_engineTexture
        && g_submitTexW >= 16 && g_submitTexH >= 16;
}

void ShareTextureRetirePending() {
    if (!g_pendingDeleteTex) return;
    if (g_pendingDeleteTex != g_engineTexture && g_pendingDeleteTex != g_submitTexture)
        glDeleteTextures(1, &g_pendingDeleteTex);
    g_pendingDeleteTex = 0;
}
