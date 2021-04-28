#ifndef _LUACOMMONTYPES_H
#define _LUACOMMONTYPES_H


#include "tools/stress/lua/luathread.h"
//#include "tools/stress/lua/luavalue.h"
//class LuaThread;

namespace Blaze
{
namespace Stress
{
    LuaTable convertToLua_Blaze_Object_Type(LuaThread *lua, const EA::TDF::ObjectType &value);
    EA::TDF::ObjectType convertFromLua_Blaze_Object_Type(LuaThread *lua, LuaTable table);

    LuaTable convertToLua_Blaze_Object_Id(LuaThread *lua, const EA::TDF::ObjectId &value);
    EA::TDF::ObjectId convertFromLua_Blaze_Object_Id(LuaThread *lua, LuaTable table);

} //Stress
} //Blaze

#endif //_LUACOMMONTYPES_H
