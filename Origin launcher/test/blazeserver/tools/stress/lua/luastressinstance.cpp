/*************************************************************************************************/
/*!
    \file

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luastressinstance.cpp#60 $
    $Change: 46633 $
    $DateTime: 2009/06/19 13:41:13 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LuaStressInstance

    Stress tests component.
    ---------------------------------------
    TODO.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/util/random.h"
#include "blazerpcerrors.h"
#include "framework/connection/selector.h"
#include "framework/util/shared/blazestring.h"

#include "luastressinstance.h"
#include "luathread.h"

#include "stressinstance.h"
#include "stressmodule.h"
#include "stressconnection.h"
#include "loginmanager.h"
#include "stress.h"


namespace Blaze
{
namespace Stress
{
/*************************************************************************************************/
/*!
    \class LuaStressInstance

    An implementation of the blaze stress tester's StressInstance class that provides an interface
    to the Lua/C++ bindings for calling RPCs automatically generated from the component's RPC and TDF
    files.
    
    For more detailed information see:
    http://tibwiki.tib.ad.ea.com/main/index.php?title=Lua_Support_for_the_Blaze_Stress_Tester
    
    The LuaStressInstance instance also maintains a map of proxies that are used directly by the
    Lua code to call the RPCs.  For example, for Madden Online Franchise, the proxy is an instance of
    FranchiseSlave and in the proxies.cpp file that instance is associated with the component name
    "Franchise".  This mechanism will allow multiple components to be invoked using the same
    instance of LuaStressInstance.  This is important for the multicomponent stress test modifications
    coming later.

*/
/*************************************************************************************************/

LuaStressInstance::LuaStressInstance(LuaStressModule *owner, StressConnection *connection, Login *login, int32_t id) :
    StressInstance(owner, connection, login, Log::SYSTEM),
    mOwner(owner),
    mName("None"),
    mRpcDelay(500),
    mId(id),
    mLuaThread(0)
{
}

void LuaStressInstance::setProxy(Component* proxy)
{
    mProxies[proxy->getId()] = proxy;
}

Component* LuaStressInstance::getProxy(ComponentId id)
{
    return mProxies[id];
}


int32_t LuaStressInstance::getId() const
{
    return mId;
}

void LuaStressInstance::SetDelay(int32_t time)
{
    mRpcDelay = time;
}

void LuaStressInstance::Delay(int32_t time)
{
    if (gSelector->sleep(time * 1000) != ERR_OK) // arg is in microsecs
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[" << mId << "] Error sleeping lua instance");
    }
}

LuaStressInstance::~LuaStressInstance()
{
//  delete mProxy;
}

void LuaStressInstance::start()
{
    StressInstance::start();
}

const char8_t *LuaStressInstance::getName() const
{
    return "";
}

BlazeRpcError LuaStressInstance::execute()
{   
    mRpcDelay = mOwner->getDelay();
    mOwner->incNumUsers();

//  BLAZE_INFO_LOG (Log::SYSTEM, "[" << mId << "] --------- START --------------------------- bzid=" << mBlazeId << ", tid=" << mTeamId);

    LuaInterpreter *interpreter = mOwner->getLuaInterpreter();
    // determine if using lua or actions
    if (interpreter != nullptr && interpreter->GetState()) // if a state is created, then we're using lua
    {
        if (mLuaThread == nullptr)
        {
            mLuaThread = interpreter->CreateThread(this);
        }
        mLuaThread->Execute("Main"); // for now, we're just executing Main, but I can see this being customizable with some stress tester architecture changes
        if (gSelector->sleep(mRpcDelay * 1000) != ERR_OK)  // arg is in microsecs
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[" << mId << "] Error sleeping lua instance");
        }
        mOwner->decNumUsers();
    }
   
    mOwner->decNumUsers();
    return ERR_OK;
}

// This will get called from the <component>luarpcs.cpp at time of initialization to register the
// component specific registration of Async Notification handler.
void LuaStressInstance::registerAsyncHandlers(uint16_t component, uint16_t type, const AsyncNotifyCb& cb)
{
    getConnection()->addAsyncHandler(component, type, this);
    mAsyncNotificationCbMap[(component << 16)| type] = cb;
}

// This will get called from the StressConnection when it receives onAsync notification from blaze server.
void LuaStressInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    // call OnAsync Function from <component>luarpc.cpp file
    AsyncNotificationCbMap::const_iterator cit = mAsyncNotificationCbMap.find((component << 16) | type);
    if (cit != mAsyncNotificationCbMap.end())
    {
        const AsyncNotifyCb &cb = (*cit).second;        
        cb(this, component, type, payload);
    }
}
} // Stress
} // Blaze

