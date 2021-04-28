/*************************************************************************************************/
/*!
    \file luastressinstance.h

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luastressinstance.h#37 $
    $Change: 46575 $
    $DateTime: 2009/06/19 09:46:10 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_LUASTRESSINSTANCE_H
#define BLAZE_STRESS_LUASTRESSINSTANCE_H

/*** Include files *******************************************************************************/

#include "luastressmodule.h"
#include "tools/stress/stressinstance.h"
#include "tools/stress/stressconnection.h"
#ifdef TARGET_util
#include "util/rpc/utilslave.h"
#include "util/tdf/utiltypes.h"
#endif
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stress
{
class StressModule;
class StressConnection;
class Login;
class LuaStressModule;

/*************************************************************************************************/
/*!
    \class LuaStressInstance

    An implementation of the blaze stress tester's StressInstance class that provides an interface
    to the Lua/C++ bindings for calling RPCs automatically generated from the component's RPC and TDF
    files.  A LuaStressInstance usually corresponds to a connection to the server to be load tested.
    
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

class LuaStressInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(LuaStressInstance);
    friend class LuaStressModule;

public:
    
    ~LuaStressInstance() override;

    // This is the entry point for a stress instance to start running
    void                 start() override;

    void                         setProxy(Component* proxy);
    Component*                   getProxy(ComponentId id);

    int32_t                      getId() const;

    void                         SetDelay(int32_t time);
    void                         Delay(int32_t time);
    
    LuaThread*                   GetLuaThread() { return mLuaThread; }
    
    //  The callback should know what notification TDF to use based on the notification type.  Type-cast accordingly     
    typedef Functor4<LuaStressInstance*, uint16_t, uint16_t, RawBuffer*> AsyncNotifyCb;
    void registerAsyncHandlers(uint16_t component, uint16_t type, const AsyncNotifyCb& cb);

protected:

    //Override this method to do your task
    BlazeRpcError        execute() override;
    const char8_t       *getName() const override;
    
    // Override these methods to track connection/disconnection events
     void                 onDisconnected() override {}
    void                 onLogin(BlazeRpcError result) override {}    
    void                 onLogout(BlazeRpcError result) override { }

    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

//  int32_t                      getSeed() const { return mTrialIndex + StressInstance::getIdent(); }
        
    LuaStressInstance(LuaStressModule *owner, StressConnection *connection, Login *login, int32_t id);

private:
    typedef eastl::vector_map<uint32_t, AsyncNotifyCb> AsyncNotificationCbMap;
    AsyncNotificationCbMap mAsyncNotificationCbMap;

    eastl::map<ComponentId, Component*>  mProxies;

    LuaStressModule                        *mOwner;
    const char8_t                          *mName;
    int32_t                                 mRpcDelay;
    int32_t                                 mId;
    LuaThread                              *mLuaThread;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_LUASTRESSINSTANCE_H

