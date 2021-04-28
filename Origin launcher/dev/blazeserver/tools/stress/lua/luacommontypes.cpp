
#include "framework/blaze.h"
#include "tools/stress/lua/luacommontypes.h"

namespace Blaze
{
namespace Stress
{

#define NULLABLE(type, expression) {LuaValue setValue = lua->GetLuaProperty(table, #type); if (setValue.GetType() != LuaValue::LUATYPE_NONE) {expression;} }

LuaTable convertToLua_Blaze_Object_Type(LuaThread *lua, const EA::TDF::ObjectType &value)
{
    LuaTable table = lua->CreateLuaTable();
    lua->SetLuaProperty(table, "Component", LuaValue::UInt(value.component));
    lua->SetLuaProperty(table, "Type", LuaValue::UInt(value.type));
    return table;
}

EA::TDF::ObjectType convertFromLua_Blaze_Object_Type(LuaThread *lua, LuaTable table)
{
    EA::TDF::ObjectType temp;
    NULLABLE(Component, temp.component = setValue.GetUInt16());
    NULLABLE(Type, temp.type = setValue.GetUInt16());
    return temp;
}

LuaTable convertToLua_Blaze_Object_Id(LuaThread *lua, const EA::TDF::ObjectId& value)
{
    LuaTable table = lua->CreateLuaTable();
    lua->SetLuaProperty(table, "Id", LuaValue::Int64(value.id));
    LuaTable table_type = convertToLua_Blaze_Object_Type(lua, value.type);
    lua->SetLuaProperty(table, "Type", LuaValue::Table(table_type));
    lua->ReleaseLuaTable(table_type);
    return table;
}

EA::TDF::ObjectId convertFromLua_Blaze_Object_Id(LuaThread *lua, LuaTable table)
{
    EA::TDF::ObjectId temp;
    NULLABLE(Id, temp.id = setValue.GetInt64());
    NULLABLE(Type, temp.type = convertFromLua_Blaze_Object_Type(lua, setValue.GetTable()));
    return temp;
}

} // namespace Stress
} // namespace Blaze
