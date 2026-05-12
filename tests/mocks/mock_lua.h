#ifndef VRMOD_MOCK_LUA_H
#define VRMOD_MOCK_LUA_H

#include <gmod/Interface.h>
#include <vector>
#include <string>
#include <map>

// Recording mock of GarrysMod::Lua::ILuaBase
// Tracks all method calls for verification in tests.
class MockLuaBase : public GarrysMod::Lua::ILuaBase {
public:
    struct Call {
        std::string method;
        std::string detail;
    };

    std::vector<Call> calls;
    std::vector<double> pushed_numbers;
    std::vector<std::string> pushed_strings;
    std::vector<bool> pushed_bools;
    std::map<std::string, double> fields_set_number;
    int next_ref = 100;
    double check_number_value = 0.0;
    const char* check_string_value = "";
    int get_type_return = GarrysMod::Lua::Type::NONE;

    void record(const char* method, const char* detail = "") {
        calls.push_back({method, detail});
    }

    void reset() {
        calls.clear();
        pushed_numbers.clear();
        pushed_strings.clear();
        pushed_bools.clear();
        fields_set_number.clear();
        next_ref = 100;
    }

    int countCalls(const char* method) const {
        int c = 0;
        for (auto& call : calls) if (call.method == method) c++;
        return c;
    }

    // --- ILuaBase pure virtuals ---
    int         Top() override { return 0; }
    void        Push(int) override { record("Push"); }
    void        Pop(int iAmt) override { record("Pop"); }
    void        GetTable(int) override { record("GetTable"); }
    void        GetField(int, const char* s) override { record("GetField", s); }
    void        SetField(int, const char* s) override { record("SetField", s); }
    void        CreateTable() override { record("CreateTable"); }
    void        SetTable(int) override { record("SetTable"); }
    void        SetMetaTable(int) override {}
    bool        GetMetaTable(int) override { return false; }
    void        Call(int, int) override { record("Call"); }
    int         PCall(int, int, int) override { return 0; }
    int         Equal(int, int) override { return 0; }
    int         RawEqual(int, int) override { return 0; }
    void        Insert(int) override {}
    void        Remove(int) override {}
    int         Next(int) override { return 0; }
    void*       NewUserdata(unsigned int) override { return nullptr; }
    void        ThrowError(const char* s) override { record("ThrowError", s); }
    void        CheckType(int, int) override {}
    void        ArgError(int, const char*) override {}
    void        RawGet(int) override {}
    void        RawSet(int) override {}
    const char* GetString(int, unsigned int*) override { return ""; }
    double      GetNumber(int) override { return 0; }
    bool        GetBool(int) override { return false; }
    GarrysMod::Lua::CFunc GetCFunction(int) override { return nullptr; }
    void*       GetUserdata(int) override { return nullptr; }
    void        PushNil() override { record("PushNil"); }

    void        PushString(const char* val, unsigned int) override {
        record("PushString", val);
        pushed_strings.push_back(val);
    }

    void        PushNumber(double val) override {
        record("PushNumber");
        pushed_numbers.push_back(val);
    }

    void        PushBool(bool val) override {
        record("PushBool");
        pushed_bools.push_back(val);
    }

    void        PushCFunction(GarrysMod::Lua::CFunc) override { record("PushCFunction"); }
    void        PushCClosure(GarrysMod::Lua::CFunc, int) override {}
    void        PushUserdata(void*) override {}

    int         ReferenceCreate() override {
        record("ReferenceCreate");
        return next_ref++;
    }
    void        ReferenceFree(int) override { record("ReferenceFree"); }
    void        ReferencePush(int) override { record("ReferencePush"); }
    void        PushSpecial(int) override { record("PushSpecial"); }

    bool        IsType(int, int iType) override {
        return iType == GarrysMod::Lua::Type::TABLE;
    }

    int         GetType(int) override { return get_type_return; }
    const char* GetTypeName(int) override { return ""; }
    void        CreateMetaTableType(const char*, int) override {}

    const char* CheckString(int) override {
        record("CheckString");
        return check_string_value;
    }

    double      CheckNumber(int) override {
        record("CheckNumber");
        return check_number_value;
    }

    int         ObjLen(int) override { return 0; }

    const QAngle& GetAngle(int) override {
        static QAngle a;
        return a;
    }

    const Vector& GetVector(int) override {
        static Vector v;
        return v;
    }

    void        PushAngle(const QAngle&) override { record("PushAngle"); }
    void        PushVector(const Vector&) override { record("PushVector"); }
    void        SetState(lua_State*) override {}
    int         CreateMetaTable(const char*) override { return 0; }
    bool        PushMetaTable(int) override { return false; }
    void        PushUserType(void*, int) override {}
    void        SetUserType(int, void*) override {}
};

// Helper: create a mock lua_State pointing to a MockLuaBase
inline lua_State* CreateMockLuaState(MockLuaBase* mock) {
    static lua_State state;
    memset(&state, 0, sizeof(state));
    state.luabase = mock;
    return &state;
}

#endif
