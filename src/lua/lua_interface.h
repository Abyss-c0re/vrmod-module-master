#ifndef VRMOD_LUA_INTERFACE_H
#define VRMOD_LUA_INTERFACE_H

struct lua_State;
namespace GarrysMod { namespace Lua { class ILuaBase; } }

// Lua utility functions (used by other modules)
void LuaPrint(GarrysMod::Lua::ILuaBase* LUA, const char* msg);
void PushMatrixAsTable(GarrysMod::Lua::ILuaBase* LUA, float* mtx, unsigned int rows, unsigned int cols);

// LUA_FUNCTION wrappers
int GetVersion(lua_State* L);
int IsHMDPresent(lua_State* L);
int Init(lua_State* L);
int Shutdown(lua_State* L);

#endif
