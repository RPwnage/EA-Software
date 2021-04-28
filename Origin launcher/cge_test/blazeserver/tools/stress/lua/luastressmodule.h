/*************************************************************************************************/
/*!
    \file luastressmodule.h

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luastressmodule.h#37 $
    $Change: 46575 $
    $DateTime: 2009/06/19 09:46:10 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_LUASTRESSMODULE_H
#define BLAZE_STRESS_LUASTRESSMODULE_H

/*** Include files *******************************************************************************/

#include "tools/stress/stressmodule.h"
#include "tools/stress/stressinstance.h"
#include "EATDF/time.h"
#include <eathread/eathread_futex.h>

#include "luainterpreter.h"
#include "luathread.h"
#include "luastressinstance.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stress
{

class StressInstance;
class StressConnection;
class Login;

/*************************************************************************************************/
/*!
    \class LuaStressModule

    An implementation of the blaze stress tester's StressModule class that creates the LuaStressInstance
    which then provides an interface to the Lua/C++ bindings for calling RPCs automatically
    generated from the component's RPC and TDF files.
    
    For more detailed information see:
    http://tibwiki.tib.ad.ea.com/main/index.php?title=Lua_Support_for_the_Blaze_Stress_Tester
    
    LuaStressModule essentially serves as a factory for LuaStressInstances, each of which roughly
    corresponds to a connection in the stress tester.

*/
/*************************************************************************************************/

class LuaStressModule : public StressModule
{
    NON_COPYABLE(LuaStressModule);

public:

    static StressModule *create();
    
    const char8_t* getModuleName() override { return "LuaStress"; }

    uint32_t getRpcDelay()       const { return mRpcDelay; }
    uint32_t getNumConnections() const { return mNumConnections; }

    // Perform any necessary configuration prior to starting the stress test
    bool            initialize(const ConfigMap &config, const ConfigBootUtil* bootUtil) override;

    // Called by the core system to create stress instances for this module
    StressInstance *createInstance(StressConnection *connection, Login *login) override;
    
    LuaInterpreter         *getLuaInterpreter();

    ~LuaStressModule() override;

    void incNumUsers () { mNumUsers++; }
    void decNumUsers () { mNumUsers--; }

protected:
    
    LuaStressModule();

    bool parseConfig(const ConfigMap& config);

private:
    
    LuaInterpreter                        *createLuaInterpreter();

    uint32_t                               mRpcDelay;
    uint32_t                               mNumConnections;
    uint32_t                               mNumUsers;
    uint32_t                               mMaxUsers;
    eastl::string                          mRpcCallStatsFile;
    eastl::string                          mRpcErrorStatsFile;

    EA::Thread::Futex                      mFutex;
    const char                            *mLuaScript;
    EA::Thread::ThreadLocalStorage         mLuaInterpreterTls;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_LUASTRESSMODULE_H

