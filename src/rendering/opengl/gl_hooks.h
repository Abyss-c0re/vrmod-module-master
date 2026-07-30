#pragma once

#include "core/vrmod_common.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

typedef struct {
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

extern char            g_createTextureOrigBytes[14];
extern void*           g_createTexture;
extern GLuint          g_sharedTexture;   // alias of submit OUT
extern GLuint          g_engineTexture;   // Source RT IN — never delete
extern GLuint          g_submitTexture;   // RGBA8 OUT to OpenVR / LizardTech
extern uint32_t        g_submitTexW;
extern uint32_t        g_submitTexH;
extern COpenGLEntryPoints* g_GL;
extern bool            g_glIsPatched;

typedef void (*ErrorFunc)(const char* msg);

void BuildCreateTextureHookPatch(void* CreateTextureHook, uint8_t outPatch[HOOK_SIZE]);
void CreateTextureHook(GLsizei n, GLuint *textures);
bool RemoveTexturePatch(ErrorFunc errFunc);

int  ShareTextureBegin(uint32_t eyeWidth, uint32_t eyeHeight, ErrorFunc errFunc);
bool ShareTextureFinish(ErrorFunc errFunc);
bool PrepareSubmitTexture(); // eng IN → submit OUT blit; returns blit ok
bool ConsumeBlitReady();
void ClearBlitReady();
void ShareTextureReset();
