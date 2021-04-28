/*************************************************************************************************/
/*!
    \file   luathread.cpp

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luathread.cpp#4 $
    $Change: 44939 $
    $DateTime: 2009/06/05 14:42:23 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LuaThread

    Wraps an individual Lua thread.  Note that a Lua thread is not a thread in the OS sense, but more
    akin to a Windows (and Blaze) lightweight fiber.  As such it works very well with Blaze's fibers
    and we provide one LuaThread per connection.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "luathread.h"


namespace Blaze
{
namespace Stress
{

//! Constructor - create a table for lua state data to be associated with this
/// lua 'fiber' and insert an element 'data' that points back to this instance
/// of LuaThread.
LuaThread::LuaThread(LuaInterpreter &interpreter) : 
    mTableId(0), mThreadId(0), mResume(false),
    mYieldSize(0), mContainer(0), mInterpreter(interpreter)
{
    lua_State *main = interpreter.GetState();

    mState = lua_newthread(main);
    lua_pushvalue(main, -1);
    mThreadId = luaL_ref(main, LUA_REGISTRYINDEX);
    lua_rawseti(main, LUA_REGISTRYINDEX, mThreadId);

    lua_newtable(mState);
    lua_pushvalue(mState, -1);
    mTableId = luaL_ref(mState, LUA_REGISTRYINDEX);
    lua_rawseti(mState, LUA_REGISTRYINDEX, mTableId);
    lua_rawgeti(mState, LUA_REGISTRYINDEX, mTableId);
    lua_pushlightuserdata(mState, this);
    lua_setfield(mState, 1, "data");
    lua_pop(mState, 1);
}

LuaThread::~LuaThread()
{
    lua_State *main = mInterpreter.GetState();
    luaL_unref(main, LUA_REGISTRYINDEX, mThreadId);
    luaL_unref(mState, LUA_REGISTRYINDEX, mTableId);
}

//! Execute() - execute a lua function by name, passing in a table
/// with a light user data member 'data' pointing to this LuaThread
int32_t LuaThread::Execute(const char8_t *function)
{
    if (!mResume)
    {
        lua_getglobal(mState, function);
        lua_rawgeti(mState, LUA_REGISTRYINDEX, mTableId);
    }
    int32_t error = lua_resume(mState, nullptr, mResume ? mYieldSize : 1);
    EA_ASSERT_MSG(lua_gettop(mState) >= 0, "LUA POPPED TOO MUCH FROM THE STACK!!");
    mResume = (error == LUA_YIELD);
    if (!mResume && (error != 0))
    {
        EA_ASSERT_MSG((error == 0) || (error == LUA_YIELD), lua_tostring(mState,-1));
        lua_pop(mState, lua_gettop(mState)); // pop everything
        return error;
    }
    if (mResume)
    {
        mYieldSize = lua_gettop(mState);
    }

    lua_gc(mState, LUA_GCCOLLECT, 0);

    return error;
}

//! GetInstanceArgument() - to be called in a Lua c++ wrapper to get the LuaInstance from 
/// the Lua 'instance' table that's passed around in the stress tester
LuaThread *LuaThread::GetInstanceArgument(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TTABLE);
    lua_pushvalue(state, 1);
    lua_getfield(state, -1, "data");
    LuaThread *instance = (LuaThread *)lua_touserdata(state, -1);
    lua_pop(state, 2);
    return instance;
}

LuaTable LuaThread::CreateLuaTable()
{
    lua_newtable(mState);
    return (LuaTable)luaL_ref(mState, LUA_REGISTRYINDEX);
}

int32_t LuaThread::GetLuaTableSize(LuaTable table)
{
    lua_rawgeti(mState, LUA_REGISTRYINDEX, static_cast<int32_t>(table));
    int32_t size = static_cast<int32_t>(lua_objlen(mState, -1));
    lua_pop(mState, 1);
    return size;
}

void LuaThread::ReleaseLuaTable(LuaTable table)
{
    luaL_unref(mState, LUA_REGISTRYINDEX, static_cast<int32_t>(table));
}

void LuaThread::PrintStackSize()
{
    printf("stack top = %d\n", lua_gettop(mState));
}

LuaValue LuaThread::GetLuaProperty(LuaTable table, const char8_t *key)
{
    lua_rawgeti(mState, LUA_REGISTRYINDEX, static_cast<int32_t>(table));
    luaL_checktype(mState, -1, LUA_TTABLE);
    lua_getfield(mState, -1, key);
    LuaValue value(LuaValue::Pop(mState));
    lua_pop(mState, 1);
    return value;
}

void LuaThread::SetLuaProperty(LuaTable table, const char8_t *key, const LuaValue &value)
{
    SetPair(table, key, value);
}

void LuaThread::Append(LuaTable table, const LuaValue &value)
{
    lua_rawgeti(mState, LUA_REGISTRYINDEX, static_cast<int32_t>(table));
    luaL_checktype(mState, -1, LUA_TTABLE);
    value.Push(mState);
    lua_rawseti(mState, -2, static_cast<int32_t>(lua_objlen(mState, -2)) + 1);
    lua_pop(mState, 1);
}

void LuaThread::SetPair(LuaTable table, const LuaValue &key, const LuaValue &value)
{
    if (key.GetType() == LuaValue::LUATYPE_NUMBER)
    {
        SetPair(table, key.GetInt(), value);
    }
    else
    {
    eastl::string keyString(key.ToString(mState));
    SetPair(table, keyString.c_str(), value);
}
}

void LuaThread::SetPair(LuaTable table, const char8_t *key, const LuaValue &value)
{
    lua_rawgeti(mState, LUA_REGISTRYINDEX, static_cast<int32_t>(table));
    value.Push(mState);
    lua_setfield(mState, -2, key);
    lua_pop(mState, 1);
}

void LuaThread::SetPair(LuaTable table, int32_t key, const LuaValue &value)
{
    lua_rawgeti(mState, LUA_REGISTRYINDEX, static_cast<int32_t>(table));
    value.Push(mState);
    lua_rawseti(mState, -2, key);
    lua_pop(mState, 1);
}

void LuaThread::Collect()
{
    lua_gc(mState, LUA_GCCOLLECT, 0);
}

//! SetContainer() - sets the opaque pointer to the instance of
/// the module in the stress tester that is using this LuaThread.
void LuaThread::SetContainer(void *container)
{
    mContainer = container;
}

//! GetContainer() - returns the opaque pointer to the instance of
/// the module in the stress tester that is using this LuaThread.
void *LuaThread::GetContainer()
{
    return mContainer;
}

} // Stress
} // Blaze
