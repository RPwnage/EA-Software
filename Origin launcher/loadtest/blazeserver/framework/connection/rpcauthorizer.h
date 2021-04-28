/*************************************************************************************************/
/*!
    \file rpcauthorizer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RPCAUTHORIZER_H
#define BLAZE_RPCAUTHORIZER_H

/*** Include files *******************************************************************************/

#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/metrics/metrics.h"
#include "EATDF/tdfobjectid.h"
#include "framework/blazedefines.h"
#include "framework/component/componentstub.h" // for ClientPlatformSet

namespace Blaze
{
// EndpointConfig is the full config
// EndpointConfig::Endpoint is the legacy endpoint
// EndpointConfig::GrpcEndpointConfig is the grpc endpoint

class RpcAuthorizer
{
    NON_COPYABLE(RpcAuthorizer);

public:

    RpcAuthorizer(Metrics::MetricsCollection& metricsCollection, const char8_t* metricName);
    ~RpcAuthorizer();

    void incrementTotalRpcAuthorizationFailureCount() { mRpcAuthorizationFailureCount.increment(); }
    uint64_t getTotalRpcAuthorizationFailureCount() const { return mRpcAuthorizationFailureCount.getTotal(); }

    bool isRpcAuthorized(ClientPlatformType platform, EA::TDF::ComponentId cmpId, CommandId cmdId) const;

    void initializeRpcAuthorizationList(const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);

    typedef eastl::hash_set<CommandId> CommandIdSet;
    typedef eastl::hash_map<EA::TDF::ComponentId, CommandIdSet> CommandIdsByComponentIdMap;
    typedef eastl::hash_map<eastl::string, CommandIdsByComponentIdMap> CommandIdsByComponentIdByControlListMap;
    typedef eastl::hash_map<eastl::string, eastl::string> ValidationErrorByWhitelistNameMap;
    typedef eastl::hash_map<eastl::string, ValidationErrorByWhitelistNameMap> RpcControlListComboMap;

    static void validateRpcControlLists(const EndpointConfig& config, const char8_t* endpointName, const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, CommandIdsByComponentIdByControlListMap& controlListMap, RpcControlListComboMap& controlListComboMap, ConfigureValidationErrors& validationErrors);
    static bool validatePlatformSpecificRpcControlLists(const EndpointConfig& config, ConfigureValidationErrors& validationErrors);

private:
    typedef enum
    {
        LIST_EMPTY,
        LIST_VALID,
        LIST_INVALID
    } ControlListValidationResult;

    // Helper methods for Rpc control list validation
    static ControlListValidationResult validateRpcControlList(const EndpointConfig& config, const char8_t* endpointName, const char8_t* listName, bool isBlacklist, CommandIdsByComponentIdByControlListMap& controlListMap, ConfigureValidationErrors& validationErrors);
    static void validateRpcControlListCombination(const EndpointConfig& config, const char8_t* endpointName, const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, const CommandIdsByComponentIdByControlListMap& controlListMap, RpcControlListComboMap& controlListComboMap, ConfigureValidationErrors& validationErrors);
    static const char8_t* getValidationErrorForControlListCombination(const EndpointConfig& config, const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, const CommandIdsByComponentIdByControlListMap& controlListMap, RpcControlListComboMap& controlListComboMap);
    static const char8_t* getControlListForPlatform(ClientPlatformType platform, const EndpointConfig::PlatformsByControlListMap& platformsMap);

    // Helper methods for Rpc control list initialization
    typedef eastl::hash_map<eastl::string, ClientPlatformSet> PlatformSetByWhitelistMap;
    static void insertWhitelistNameForPlatforms(const ClientPlatformSet& blacklistPlatforms, const char8_t* whitelistName, PlatformSetByWhitelistMap& whitelistMap, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);
    static void insertAuthorizedCommands(EA::TDF::ComponentId cmpId, CommandId cmdId, CommandIdsByComponentIdMap& authorizedCommands, const CommandIdsByComponentIdMap& blacklistCommands);

    Metrics::MetricsCollection& mMetricsCollection;
    
    typedef eastl::vector<CommandIdsByComponentIdMap> AuthorizedCommandsList;
    AuthorizedCommandsList mAuthorizedCommandsList;
    typedef eastl::map<ClientPlatformType, uint32_t> AuthorizedCommandsByPlatformMap;
    AuthorizedCommandsByPlatformMap mAuthorizedCommandsByPlatform;

    Metrics::Counter mRpcAuthorizationFailureCount;  // The total number of requests that failed due to being unauthorized (not whitelisted or blacklisted)

};
} // Blaze

#endif // BLAZE_RPCAUTHORIZER_H
