/*************************************************************************************************/
/*!
    \file   luathread.h

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luathread.h#2 $
    $Change: 44737 $
    $DateTime: 2009/06/04 14:33:39 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

#ifndef _LUATHREAD_H
#define _LUATHREAD_H

struct lua_State;

#include "luainterpreter.h"
#include "luavalue.h"

#include <stdlib.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

namespace Blaze
{
namespace Stress
{

/*************************************************************************************************/
/*!
    \class LuaThread

    Wraps an individual Lua thread.  Note that a Lua thread is not a thread in the OS sense, but more
    akin to a Windows (and Blaze) lightweight fiber.  As such it works very well with Blaze's fibers
    and we provide one LuaThread per connection.

    Note that for each LuaThread instance, we create a table for lua state data to be associated
    with this lua 'fiber' and insert an element 'data' that points back to this C++ instance
    of LuaThread.  We then pass this instance to all of our RPCs like so -

     function MyNewFunction(instance)
      local fid = GetGMFranchiseIdForUser(instance)
      GetRegularSchedule(instance, fid)
      GetPlayoffSchedule(instance, fid)
     end

     function Main(instance)
 
      local fid = GetGMFranchiseIdForUser(instance)
      MyNewFunction(instance)
      ..
     end

    In the above case, the Execute method of LuaThread would be called with "Main" as the function
    to execute.

*/
/*************************************************************************************************/
   
class LuaThread
{
public:

    LuaThread(LuaInterpreter &interpreter);
    ~LuaThread();

    void        PrintStackSize(); // for debugging, get rid of it?

    //! Set/GetContainer() - sets/gets the opaque pointer to the instance of
    /// the module (in this case FranchiseInstance) in the stress tester that
    /// is using this LuaThread.
    void        SetContainer(void *container);
    void       *GetContainer();

    //! Execute() - execute a lua function by name, passing in a table
    /// with a light user data member 'data' pointing to this LuaThread
    int         Execute(const char *function);

    lua_State  *GetLuaState() { return mState; }

    LuaTable    CreateLuaTable();
    int         GetLuaTableSize(LuaTable table);
    void        ReleaseLuaTable(LuaTable table);

    LuaValue    GetLuaProperty(LuaTable table, const char *key);
    void        SetLuaProperty(LuaTable table, const char *key, const LuaValue &value);
    
    void        Append(LuaTable table, const LuaValue &value);

    void        SetPair(LuaTable table, const LuaValue &key, const LuaValue &value);
    void        SetPair(LuaTable table, const char *key, const LuaValue &value);
    void        SetPair(LuaTable table, int key, const LuaValue &value);

    void        Collect();

    int         GetTableId(){ return mTableId; }

    //! GetInstanceArgument() - to be called in a Lua c++ wrapper to get the
    /// LuaInstance from the Lua 'instance' table that's passed around in
    /// the stress tester
    static LuaThread *GetInstanceArgument(lua_State *state);
   
private:

    int             mTableId,
                    mThreadId;
    lua_State      *mState;
    bool            mResume;
    int             mYieldSize;
    void           *mContainer;
    LuaInterpreter &mInterpreter;

    LuaThread &operator=(const LuaThread &);
};

// this is declared in generated code
void registerLuaFunctions(lua_State *lua);

} // Stress
} // Blaze

#endif // _LUATHREAD_H
