/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DebugSystem

    This class provides the ability to control RPC error code returned and RPC delay on a global
    basis or individual RPCs if debug mode is enabled.

*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/system/debugsys.h"
#include "framework/component/blazerpc.h"

namespace Blaze
{

DebugSystem::DebugSystem() : 
    mComponentDelayMap(BlazeStlAllocator("DebugSystem::mComponentDelayMap")),
    mComponentErrorMap(BlazeStlAllocator("DebugSystem::mComponentErrorMap")),
    mEnabled(false), mOverrideBeforeExecution(false), mDelayPerCommand(0), mDelayIgnoreRate(0), 
    mDelayIgnoreCount(0), mErrorSuccessRate(0), mErrorSuccessCount(0), mErrorOverride(ERR_OK)
{

}

DebugSystem::~DebugSystem()
{
    clearLists();
}

bool DebugSystem::getRpcErrorOverride(uint16_t componentId, uint16_t commandId, const ComponentRpcIntMap &componentRpcIntMap, int32_t *outValue)
{
    const char8_t * componentName = BlazeRpcComponentDb::getComponentNameById(componentId);
    if (!componentName)
        return false;

    const char8_t * rpcName = BlazeRpcComponentDb::getCommandNameById(componentId, commandId);
    if (!rpcName)
        return false;

    // check to see if this component/rpc has a delay override
    ComponentRpcIntMap::const_iterator it = componentRpcIntMap.find(componentName);
    if (it != componentRpcIntMap.end())
    {
        const RpcIntMap *rpcIntMap = &((*it).second);
        RpcIntMap::const_iterator it2 = rpcIntMap->find(rpcName);
        if (it2 != rpcIntMap->end())
        {
            int32_t val = (*it2).second;
            if (outValue)
            {
                *outValue = val;
            }
            return true;
        }
    }
    return false;
}

bool DebugSystem::getRpcDelayOverride( uint16_t componentId, uint16_t commandId, ComponentRpcTimeMap *componentRpcTimeMap, TimeValue *outValue)
{
    const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(componentId);
    if (!componentName)
        return false;

    const char8_t * rpcName = BlazeRpcComponentDb::getCommandNameById(componentId, commandId);
    if (!rpcName)
        return false;

    // check to see if this component/rpc has a delay override
    ComponentRpcTimeMap::const_iterator it = componentRpcTimeMap->find(componentName);
    if (it != componentRpcTimeMap->end())
    {
        const RpcTimeMap *rpcTimeMap = &((*it).second);
        // found it!
        RpcTimeMap::const_iterator it2 = rpcTimeMap->find(rpcName);
        if (it2 != rpcTimeMap->end())
        {
            TimeValue val = (*it2).second;
            if (outValue)
            {
                *outValue = val;
            }
            return true;
        }
    }
    return false;
}

TimeValue DebugSystem::getDelayPerCommand( uint16_t componentId, uint16_t commandId) 
{
    if (!mEnabled)
        return 0; // bail if the debug system is disabled
    
    TimeValue delay;
    if (getRpcDelayOverride( componentId, commandId, &mComponentDelayMap, &delay))
    {
        BLAZE_WARN_LOG(Log::USER, "[DebugSystem].getDebugDelayPerCommand: Adding rpc specific delay of [" << delay.getMillis() <<
            "] to [" << BlazeRpcComponentDb::getComponentNameById(componentId) << ":" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "].");
        return delay ;
    }

    // no override found - just use the global delay override
    if (mDelayIgnoreCount >= mDelayIgnoreRate)
    {
        BLAZE_WARN_LOG(Log::USER, "[DebugSystem].getDebugDelayPerCommand: Adding global delay of [" << mDelayPerCommand.getMillis() <<
            "] to [" << BlazeRpcComponentDb::getComponentNameById(componentId) << ":" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "].");
        mDelayIgnoreCount = 0;
        return mDelayPerCommand;
    }

    mDelayIgnoreCount++;
    return 0; 
}

BlazeRpcError DebugSystem::getForcedError(uint16_t componentId, uint16_t commandId)
{
    if (!mEnabled)
        return ERR_OK ;

    int32_t err = 0 ;
    if (getRpcErrorOverride(componentId, commandId, mComponentErrorMap, &err))
    {
        BLAZE_WARN_LOG(Log::USER, "[DebugSystem].getDebugDelayPerCommand: Forcing rpc specific error of [" << err << "] to [" << 
            BlazeRpcComponentDb::getComponentNameById(componentId) << ":" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "].");
        return static_cast<BlazeRpcError>(err);
    }
    if( mErrorSuccessCount >= mErrorSuccessRate )
    {
        mErrorSuccessCount = 0 ;
        BLAZE_WARN_LOG(Log::USER, "[DebugSystem].getDebugDelayPerCommand: Forcing global error of [" << mErrorOverride << "].");
        return mErrorOverride;
    }
    
    mErrorSuccessCount++;
    return ERR_OK ;
    
}

void DebugSystem::parseComponentRpcIntMapConfig( const DebugSettingsConfig::ComponentRpcIntMap &errorOverridesList, ComponentRpcIntMap *componentRpcIntMap)
{
    if (!componentRpcIntMap)
        return;

    DebugSettingsConfig::ComponentRpcIntMap::const_iterator itComRpc = errorOverridesList.begin();
    for (; itComRpc != errorOverridesList.end(); ++itComRpc)
    {
        const char8_t* componentName = itComRpc->first.c_str();
        RpcIntMap *rpcIntMap = &(*componentRpcIntMap)[componentName];

        const DebugSettingsConfig::RpcIntMap *rpcList = itComRpc->second;
        DebugSettingsConfig::RpcIntMap::const_iterator itRpcInt = rpcList->begin();

        for (; itRpcInt != rpcList->end(); ++itRpcInt)
        {
            const char8_t* rpcName = itRpcInt->first.c_str();
            int32_t error = itRpcInt->second;
            (*rpcIntMap)[rpcName] = error;

            BLAZE_TRACE_LOG(Log::USER, "[DebugSystem].parseComponentRpcIntMapConfig: RPC override: [" << rpcName << "][" << error << "].");
        }
    }
    
}

void DebugSystem::parseComponentRpcTimeMapConfig( const DebugSettingsConfig::ComponentRpcTimeMap &delayOverridesList, ComponentRpcTimeMap *componentRpcTimeMap)
{
    if (!componentRpcTimeMap)
        return;

    DebugSettingsConfig::ComponentRpcTimeMap::const_iterator itComRpc = delayOverridesList.begin();
    for (; itComRpc != delayOverridesList.end(); ++itComRpc)
    {
        const char8_t* componentName = itComRpc->first.c_str();
        RpcTimeMap *rpcTimeMap = &(*componentRpcTimeMap)[componentName];

        const DebugSettingsConfig::RpcTimeMap *rpcList = itComRpc->second;
        DebugSettingsConfig::RpcTimeMap::const_iterator itRpcTime = rpcList->begin();

        for (; itRpcTime != rpcList->end(); ++itRpcTime)
        {
            const char8_t* rpcName = itRpcTime->first.c_str();
            TimeValue delay = itRpcTime->second;
            (*rpcTimeMap)[rpcName] = delay;

            BLAZE_TRACE_LOG(Log::USER, "[DebugSystem].parseComponentRpcTimeMapConfig: RPC override: [" << rpcName << "][" << delay.getMillis() << "].");
        }
    }
    
}

void DebugSystem::parseDebugConfig()
{  
    // reset our internal counts when we read in a new config file
    mDelayIgnoreCount = 0 ;
    mErrorSuccessCount = 0;

    const DebugSettingsConfig& config = gController->getFrameworkConfigTdf().getDebugSettingsConfig();

    mEnabled = config.getDebugEnabled();
    if (!mEnabled)
    {
        return;
    }
    BLAZE_INFO_LOG(Log::USER, "[DebugSystem].configure: DEBUG SYSTEM ENABLED!.");

    mOverrideBeforeExecution = config.getOverrideBeforeExecution();

    mDelayPerCommand = config.getDelayPerCommand();

    mErrorOverride = static_cast<BlazeRpcError>(config.getForceError());

    mDelayIgnoreRate = config.getDelayIgnoreRate();
    bool usingDefaultDelayIgnoreRate = !config.isDelayIgnoreRateSet();

    mErrorSuccessRate = config.getErrorSuccessRate();
    bool usingDefaultErrorSuccessRate = !config.isErrorSuccessRateSet();
    
    BLAZE_WARN_LOG(Log::USER, "[DebugSystem].configure: Global delay set [" << mDelayPerCommand.getMillis() << "] delayIgnoreRate [" << mDelayIgnoreRate << "]" << (usingDefaultDelayIgnoreRate? " (default)" : "") << ".");
    BLAZE_WARN_LOG(Log::USER, "[DebugSystem].configure: Global error set [" << mDelayPerCommand.getMillis() << "] errorSuccessRate [" << mErrorSuccessRate << "]" << (usingDefaultErrorSuccessRate? " (default)" : "") << ".");
    
    parseComponentRpcTimeMapConfig(config.getDelayOverrides(), &mComponentDelayMap);
    parseComponentRpcIntMapConfig(config.getErrorOverrides(), &mComponentErrorMap);
}

void DebugSystem::clearComponentRpcIntMap( ComponentRpcIntMap *componentRpcIntMap)
{
    if (!componentRpcIntMap)
        return;

    // clear out all of the rpcMaps first
    ComponentRpcIntMap::iterator it = componentRpcIntMap->begin();
    for (; it != componentRpcIntMap->end() ; ++it)
    {
        RpcIntMap* rpcMap = &((*it).second);
        rpcMap->clear();
    }
    componentRpcIntMap->clear();
}

void DebugSystem::clearComponentRpcTimeMap(ComponentRpcTimeMap *componentRpcTimeMap)
{
    if (!componentRpcTimeMap)
        return;

    // clear out all of the rpcMaps first
    ComponentRpcTimeMap::iterator it = componentRpcTimeMap->begin();
    for (; it != componentRpcTimeMap->end() ; ++it)
    {
        RpcTimeMap* rpcMap = &((*it).second);
        rpcMap->clear();
    }
    componentRpcTimeMap->clear();
}

void DebugSystem::clearLists()
{
    clearComponentRpcTimeMap(&mComponentDelayMap);
    clearComponentRpcIntMap(&mComponentErrorMap);
}

} //  namespace Blaze


