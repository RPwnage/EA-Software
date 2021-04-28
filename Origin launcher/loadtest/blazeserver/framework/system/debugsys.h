/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DEBUGSYS_H
#define BLAZE_DEBUGSYS_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DebugSystem
{
public:
    DebugSystem() ;
    virtual ~DebugSystem() ;

    /*! \brief Returns the amount of delay we want to add for each command */
    EA::TDF::TimeValue getDelayPerCommand(uint16_t componentId, uint16_t commandId);

    /*! \brief Returns ERR_OK if we shouldn't interfere with the error reporting for an rpc, otherwise returns a valid ERROR_CODE to return to the client */
    BlazeRpcError getForcedError(uint16_t componentId, uint16_t commandId);
      
    // reads the debug configuration
    void parseDebugConfig();

    bool isEnabled() { return mEnabled; }

    bool isOverrideBeforeExecution() { return mOverrideBeforeExecution; }

private:
    typedef eastl::hash_map<eastl::string, int32_t> RpcIntMap;
    typedef eastl::hash_map<eastl::string, RpcIntMap > ComponentRpcIntMap;
    typedef eastl::hash_map<eastl::string, EA::TDF::TimeValue> RpcTimeMap;
    typedef eastl::hash_map<eastl::string, RpcTimeMap > ComponentRpcTimeMap;

    void clearLists();
    void clearComponentRpcIntMap(ComponentRpcIntMap *componentRpcIntMap);
    void parseComponentRpcIntMapConfig(const DebugSettingsConfig::ComponentRpcIntMap &errorOverridesList, ComponentRpcIntMap *componentRpcIntMap);
    bool getRpcErrorOverride(uint16_t componentId, uint16_t commandId, const ComponentRpcIntMap &componentRpcIntMap, int32_t *outValue);

    void clearComponentRpcTimeMap(ComponentRpcTimeMap *componentRpcTimeMap);
    void parseComponentRpcTimeMapConfig(const DebugSettingsConfig::ComponentRpcTimeMap &delayOverridesList, ComponentRpcTimeMap *componentRpcTimeMap);
    bool getRpcDelayOverride(uint16_t componentId, uint16_t commandId, ComponentRpcTimeMap *componentRpcTimeMap, EA::TDF::TimeValue *outValue);

private:
    ComponentRpcTimeMap mComponentDelayMap;
    ComponentRpcIntMap mComponentErrorMap;

    bool    mEnabled;
    bool    mOverrideBeforeExecution;
    EA::TDF::TimeValue mDelayPerCommand; 
    uint32_t mDelayIgnoreRate;
    uint32_t mDelayIgnoreCount;
    uint32_t mErrorSuccessRate;
    uint32_t mErrorSuccessCount;
    BlazeRpcError mErrorOverride;
};


}
/******************************************************************************/

#endif  //  BLAZE_DEBUGSYS_H

