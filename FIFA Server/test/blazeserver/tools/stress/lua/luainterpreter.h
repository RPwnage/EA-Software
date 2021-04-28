/*************************************************************************************************/
/*!
    \file   luainterpreter.h

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luainterpreter.h#2 $
    $Change: 44737 $
    $DateTime: 2009/06/04 14:33:39 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LuaInterpreter

    Wraps the Lua interpreter for the purposes of stress testing, and serves as a factory for
    Lua threads.  Note that there will be ONE instance of LuaInterpreter PER OS thread.  This is
    a requirement to maintain thread safety in Lua.

*/
/*************************************************************************************************/

#ifndef _LUAINTERPRETER_H
#define _LUAINTERPRETER_H

#include "luavalue.h"

struct lua_State;

namespace Blaze
{
namespace Stress
{

class LuaThread;
LuaTable convertFromTdfToLua(LuaThread* luaThread, EA::TDF::TdfPtr response);

class LuaInterpreter
{
public:

    LuaInterpreter(const char *rpcCallStatsFile, const char *rpcErrorStatsFile);
    ~LuaInterpreter();
    
    //! RunFile() initializes, runs the lua script and sets LuaStats::SetLuaFile(..)
    bool       RunFile(const char8_t *filename);

    //! CreateThread() creates a new lua 'fiber' and initializes it.
    LuaThread *CreateThread(void *userContainer);

    //! GetState() returns the underlying lua_State instance.
    lua_State *GetState();
    
private:

    enum { MAX_FILENAME_SIZE = 1024 };

    lua_State  *mState;

    static char8_t *AppendTimeToFilename(char8_t *fileName);
};

} // Stress
} // Blaze

#endif // _LUAINTERPRETER_H
