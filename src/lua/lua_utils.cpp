#include <gmod/Interface.h>
#include "lua_interface.h"

void LuaPrint(GarrysMod::Lua::ILuaBase* LUA, const char* msg) {
    LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    LUA->PushString(msg);
    LUA->Call(1, 0);
    LUA->Pop(1);
}

void PushMatrixAsTable(GarrysMod::Lua::ILuaBase* LUA, float* mtx, unsigned int rows, unsigned int cols) {
    LUA->CreateTable();
    for (unsigned int row = 0; row < rows; row++) {
        LUA->PushNumber(row + 1);
        LUA->CreateTable();
        for (unsigned int col = 0; col < cols; col++) {
            LUA->PushNumber(col+1);
            LUA->PushNumber(mtx[row * cols + col]);
            LUA->SetTable(-3);
        }
        LUA->SetTable(-3);
    }
}
