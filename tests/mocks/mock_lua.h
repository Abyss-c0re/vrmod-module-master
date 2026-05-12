#pragma once
#include <gmod/Interface.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

// Records calls made to the Lua interface for verification
struct LuaCall {
    std::string method;
    std::string strArg;
    double numArg;
    int intArg;
    bool boolArg;
};

class MockLuaBase : public GarrysMod::Lua::ILuaBase {
public:
    std::vector<LuaCall> calls;
    int stackTop = 0;
    int nextRef = 1;

    // Track pushed values for verification
    std::vector<std::string> pushedStrings;
    std::vector<double> pushedNumbers;
    std::vector<bool> pushedBools;
    std::vector<std::string> setFields;

    // CheckString/CheckNumber return values (indexed by stack pos)
    std::string checkStringValues[16];
    double checkNumberValues[16] = {};
    int checkStringCount = 0;
    int checkNumberCount = 0;

    // Error tracking
    bool throwErrorCalled = false;
    std::string lastError;

    void reset() {
        calls.clear();
        stackTop = 0;
        nextRef = 1;
        pushedStrings.clear();
        pushedNumbers.clear();
        pushedBools.clear();
        setFields.clear();
        throwErrorCalled = false;
        lastError.clear();
        checkStringCount = 0;
        checkNumberCount = 0;
    }

    // --- ILuaBase implementation ---
    int Top() override { return stackTop; }
    void Push(int) override { stackTop++; }
    void Pop(int n = 1) override { stackTop -= n; }
    void GetTable(int) override {}
    void GetField(int iStackPos, const char* name) override {
        calls.push_back({"GetField", name ? name : "", 0, iStackPos, false});
        stackTop++;
    }
    void SetField(int iStackPos, const char* name) override {
        calls.push_back({"SetField", name ? name : "", 0, iStackPos, false});
        if (name) setFields.push_back(name);
        stackTop--;
    }
    void CreateTable() override {
        calls.push_back({"CreateTable", "", 0, 0, false});
        stackTop++;
    }
    void SetTable(int) override { stackTop -= 2; }
    void SetMetaTable(int) override { stackTop--; }
    bool GetMetaTable(int) override { return false; }
    void Call(int args, int results) override {
        stackTop -= args;
        stackTop += results;
    }
    int PCall(int, int, int) override { return 0; }
    int Equal(int, int) override { return 0; }
    int RawEqual(int, int) override { return 0; }
    void Insert(int) override {}
    void Remove(int) override { stackTop--; }
    int Next(int) override { return 0; }
    void* NewUserdata(unsigned int) override { return nullptr; }
    void ThrowError(const char* err) override {
        throwErrorCalled = true;
        lastError = err ? err : "";
        calls.push_back({"ThrowError", lastError, 0, 0, false});
    }
    void CheckType(int, int) override {}
    void ArgError(int, const char*) override {}
    void RawGet(int) override {}
    void RawSet(int) override {}
    const char* GetString(int iStackPos = -1, unsigned int* len = NULL) override {
        if (len) *len = 0;
        return "";
    }
    double GetNumber(int = -1) override { return 0; }
    bool GetBool(int = -1) override { return false; }
    GarrysMod::Lua::CFunc GetCFunction(int = -1) override { return nullptr; }
    void* GetUserdata(int = -1) override { return nullptr; }
    void PushNil() override { stackTop++; }
    void PushString(const char* val, unsigned int = 0) override {
        pushedStrings.push_back(val ? val : "");
        calls.push_back({"PushString", val ? val : "", 0, 0, false});
        stackTop++;
    }
    void PushNumber(double val) override {
        pushedNumbers.push_back(val);
        calls.push_back({"PushNumber", "", val, 0, false});
        stackTop++;
    }
    void PushBool(bool val) override {
        pushedBools.push_back(val);
        calls.push_back({"PushBool", "", 0, 0, val});
        stackTop++;
    }
    void PushCFunction(GarrysMod::Lua::CFunc) override {
        calls.push_back({"PushCFunction", "", 0, 0, false});
        stackTop++;
    }
    void PushCClosure(GarrysMod::Lua::CFunc, int) override { stackTop++; }
    void PushUserdata(void*) override { stackTop++; }
    int ReferenceCreate() override {
        calls.push_back({"ReferenceCreate", "", 0, nextRef, false});
        stackTop--;
        return nextRef++;
    }
    void ReferenceFree(int ref) override {
        calls.push_back({"ReferenceFree", "", 0, ref, false});
    }
    void ReferencePush(int ref) override {
        calls.push_back({"ReferencePush", "", 0, ref, false});
        stackTop++;
    }
    void PushSpecial(int type) override {
        calls.push_back({"PushSpecial", "", 0, type, false});
        stackTop++;
    }
    bool IsType(int, int iType) override {
        return iType == GarrysMod::Lua::Type::TABLE;
    }
    int GetType(int iStackPos) override {
        // Return STRING for positions where we've set checkString values
        if (iStackPos > 0 && iStackPos <= checkStringCount)
            return GarrysMod::Lua::Type::STRING;
        return GarrysMod::Lua::Type::NONE;
    }
    const char* GetTypeName(int) override { return "unknown"; }
    void CreateMetaTableType(const char*, int) override {}
    const char* CheckString(int iStackPos = -1) override {
        int idx = (iStackPos > 0) ? iStackPos - 1 : 0;
        if (idx < 16 && checkStringValues[idx].length() > 0)
            return checkStringValues[idx].c_str();
        return "";
    }
    double CheckNumber(int iStackPos = -1) override {
        int idx = (iStackPos > 0) ? iStackPos - 1 : 0;
        if (idx < 16) return checkNumberValues[idx];
        return 0;
    }
    int ObjLen(int = -1) override { return 0; }
    const QAngle& GetAngle(int = -1) override { static QAngle a; return a; }
    const Vector& GetVector(int = -1) override { static Vector v; return v; }
    void PushAngle(const QAngle& val) override {
        calls.push_back({"PushAngle", "", 0, 0, false});
        stackTop++;
    }
    void PushVector(const Vector& val) override {
        calls.push_back({"PushVector", "", 0, 0, false});
        stackTop++;
    }
    void SetState(lua_State*) override {}
    int CreateMetaTable(const char*) override { return 0; }
    bool PushMetaTable(int) override { return false; }
    void PushUserType(void*, int) override { stackTop++; }
    void SetUserType(int, void*) override {}
};
