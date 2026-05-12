#pragma once
#include <gmod/Interface.h>

// Print a message to the Garry's Mod console via Lua.
void LuaPrint(GarrysMod::Lua::ILuaBase* LUA, const char* msg);

// Push a flat float array as a nested Lua table (1-indexed rows and cols).
void PushMatrixAsTable(GarrysMod::Lua::ILuaBase* LUA, float* mtx, unsigned int rows, unsigned int cols);
