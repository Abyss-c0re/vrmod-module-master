#ifndef VRMOD_OPENGL_HOOK_H
#define VRMOD_OPENGL_HOOK_H

#include <GL/gl.h>

struct lua_State;
namespace GarrysMod { namespace Lua { class ILuaBase; } }

typedef void (*glGenTextures_t)(GLsizei n, GLuint *textures);

void BuildCreateTextureHookPatch(void* CreateTextureHook, uint8_t outPatch[14]);
void CreateTextureHook(GLsizei n, GLuint *textures);
bool RemoveTexturePatch(GarrysMod::Lua::ILuaBase* LUA);

int ShareTextureBegin(lua_State* L);
int ShareTextureFinish(lua_State* L);

#endif
