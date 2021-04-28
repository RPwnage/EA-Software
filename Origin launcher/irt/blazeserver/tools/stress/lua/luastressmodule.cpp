/*************************************************************************************************/
/*!
    \file

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luastressmodule.cpp#60 $
    $Change: 46633 $
    $DateTime: 2009/06/19 13:41:13 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LuaStressModule

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
#include "framework/protocol/shared/heat2decoder.h"

#include "luastressmodule.h"


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

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const int32_t DEFAULT_RPC_DELAY = 500;

/*** LuaStressInstance Implementation ***************************************************************/


/*** LuaStressModule Methods ******************************************************************************/

// static
StressModule *LuaStressModule::create()
{
    return BLAZE_NEW LuaStressModule();
}

LuaStressModule::LuaStressModule()
: mFutex(), mLuaScript(nullptr)
{
    mRpcDelay = DEFAULT_RPC_DELAY; // default delay between RPC calls, in ms
    mNumConnections = 100;
    mNumUsers = 0;
    mMaxUsers = 0;
}

LuaStressModule::~LuaStressModule()
{
}

LuaInterpreter *LuaStressModule::getLuaInterpreter()
{
    EA::Thread::AutoFutex lock(mFutex);
    if (mLuaInterpreterTls.GetValue() == nullptr)
    {
        mLuaInterpreterTls.SetValue(createLuaInterpreter());
    }
    return (LuaInterpreter *)mLuaInterpreterTls.GetValue();
}

//  LuaStress Stress Test Configuration
//      Defines fields passed to LuaStress RPCs invoked during a stress test.
//      Each Test uses a subset of the listed fields.
//
bool LuaStressModule::parseConfig(const ConfigMap& config)
{
    mRpcDelay          = config.getUInt32("delay",             DEFAULT_RPC_DELAY); // in ms
    mRpcCallStatsFile  = config.getString("rpcCallStatsFile",  "rpcstats.csv");
    mRpcErrorStatsFile = config.getString("rpcErrorStatsFile", "errorstats.csv");
    return true;
}

LuaInterpreter *LuaStressModule::createLuaInterpreter()
{
    LuaInterpreter *lua = nullptr;
    if (mLuaScript != nullptr)
    {
        lua = BLAZE_NEW LuaInterpreter(mRpcCallStatsFile.c_str(), mRpcErrorStatsFile.c_str());
        if (!lua->RunFile(mLuaScript))
        {
            delete lua;
            return nullptr;
        }
    }
    return lua;
}

bool LuaStressModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    BLAZE_INFO_LOG(Log::SYSTEM, "LuaStressModule : initialize - enter.");

    if (!StressModule::initialize(config, bootUtil))
    {
        return false;
    }

    if (!parseConfig(config))
    {
        return false;
    }

    mLuaScript = config.getString("luaScript", 0);
    BLAZE_INFO_LOG(Log::SYSTEM, "[LuaStressModule]:: luaScript=" << mLuaScript << ".");
    if (mLuaScript != nullptr)
    {
        getLuaInterpreter(); // prime the lua pump; this is necessary for linux
        return true;
    }

    return false;
}

// OKAY I see - This is some magic that lets you set *_onAsyncNotificationHandler handlers in lua for the varius notifications. 
// That makes sense. 


// Registered with the LuaStressInstance in the stress tester to handle onAsync notification.
void onAsyncNotificationHandler(LuaStressInstance *instance, uint16_t component, uint16_t type, RawBuffer* payload)
{
    // handle the notification here and call Lua function.
    // first check if there is async Lua function available.
    LuaThread* luaThread = instance->GetLuaThread();

    // Certain events trigger notifications before any Lua modules
    // and LuaThreads are constructed, such as the login done before
    // executing the Stress Module.   These we have to ignore since
    // there is no loaded Lua environment to receive them.
    if (luaThread == nullptr)
    {
        return;
    }

    lua_State *luaState = luaThread->GetLuaState();

    const ComponentData* compData = ComponentData::getComponentData((ComponentId)component);
    const NotificationInfo* notifyInfo = ComponentData::getNotificationInfo((ComponentId)component, (NotificationId)type);
    if (compData == nullptr || notifyInfo == nullptr)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "onAsyncNotificationHandler:: failed to find notification type " << type << " in component " << component);
        return;
    }
    eastl::string componentNameCapitalized = compData->loggingName;        // remove the (0x0000) at the end:
    componentNameCapitalized.erase(componentNameCapitalized.end() - 8, componentNameCapitalized.end());
    componentNameCapitalized.append("_onAsyncNotificationHandler");

    lua_getglobal(luaState, componentNameCapitalized.c_str());
    if (lua_isnoneornil(luaState, -1) || !lua_isfunction(luaState, -1))
    {
        // no need to call the Lua function as it does not exist
        lua_pop(luaState, 1);
        return;
    }


    LuaTable notificationParam = static_cast<int32_t>(0);
    Heat2Decoder decoder;
    EA::TDF::Tdf* notificationTdf = notifyInfo->payloadTypeDesc->createInstance(*Allocator::getAllocator(MEM_GROUP_FRAMEWORK_DEFAULT), "Lua Notification");
    if (decoder.decode(*payload, *notificationTdf) == ERR_OK)
    {
        notificationParam = convertFromTdfToLua(luaThread, notificationTdf);
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "onAsyncNotificationHandler:: Heat2Decoder failed to decode notification type " << notifyInfo->name << " in component " << compData->name);
    }

    if (notificationParam != 0)
    {
        // Now call the BlazeController_onAsyncNotificationHandler lua function
        // push 'instance', notificationData, component, type on lua stack
        // 'instance' can be used to get blazeId for this stressinstance and use it as index into other tables
        // for accessing this user specific notification data
        lua_rawgeti(luaState, LUA_REGISTRYINDEX, luaThread->GetTableId());
        lua_rawgeti(luaState, LUA_REGISTRYINDEX, static_cast<int32_t>(notificationParam));
        lua_pushinteger(luaState, component);
        lua_pushinteger(luaState, type);
        lua_call(luaState, 4, 0);
        luaThread->ReleaseLuaTable(notificationParam);
    }
}


void RegisterAsyncHandlers(LuaStressInstance* instance)
{
    // Add all of the RPCs mapped to components:
    size_t numComps = ComponentData::getComponentCount();
    for (size_t i = 0; i < numComps; ++i)
    {
        const ComponentData* compData = ComponentData::getComponentDataAtIndex(i);
        if (Component::isMasterComponentId(compData->id))
            continue;

        ComponentId componentId = compData->id;
        for (auto& notification : compData->notifications)
        {
            instance->registerAsyncHandlers(componentId, notification.first, onAsyncNotificationHandler);
        }
    }
}

StressInstance *LuaStressModule::createInstance(StressConnection* connection, Login* login)
{
    static int32_t instanceId = 0;  // simple counter to keep track of all instances created
    LuaStressInstance *instance = BLAZE_NEW LuaStressInstance(this, connection, login, instanceId++);
    // AddProxies(instance);
    size_t numComps = ComponentData::getComponentCount();
    for (size_t i = 0; i < numComps; ++i)
    {
        const ComponentData* compData = ComponentData::getComponentDataAtIndex(i);
        if (!Component::isMasterComponentId(compData->id))
        {
            // This allocates memory: 
            instance->setProxy(compData->createRemoteComponent(*instance->getConnection()));
        }
    }

    RegisterAsyncHandlers(instance);
    return instance;
}

} // Stress
} // Blaze
