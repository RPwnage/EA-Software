/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_SESSION_MANAGER_H
#define BLAZE_USER_SESSION_MANAGER_H

/*** Include files *******************************************************************************/
#include "framework/tdf/userdefines.h"
#include "framework/tdf/userextendeddatatypes_server.h"
#include "framework/tdf/networkaddress.h"
#include "framework/tdf/userpineventtypes_server.h"
#include "framework/usersessions/extendeddataprovider.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/userinfo.h"
#include "framework/userset/userset.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/usersessions/groupmanager.h"
#include "framework/usersessions/usersessionsmetrics.h"
#include "framework/usersessions/reputationserviceutilfactory.h"
#include "framework/usersessions/qosconfig.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/rpc/usersessionsslave_stub.h"
#include "framework/rpc/usersessionsmaster.h"
#include "framework/replication/remotereplicatedmap.h"
#include "framework/util/dispatcher.h"
#include "framework/util/intervalmap.h"
#include "framework/util/locales.h"
#include "framework/identity/identity.h"
#include "framework/controller/drainlistener.h"
#include "framework/controller/remoteinstancelistener.h"
#include "framework/storage/storagelistener.h"
#include "framework/system/fiberjobqueue.h"
#include "framework/usersessions/userinfodb.h"
#include "framework/util/riverpinutil.h"

#include "EASTL/hash_map.h"
#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "EASTL/vector_map.h"
#include "EASTL/vector_set.h"
#include "EASTL/string.h"
#include "EASTL/set.h"
#include "EASTL/hash_set.h"
#include "EASTL/deque.h"
#include "framework/component/censusdata.h" // for CensusDataManager & CensusDataProvider
#include "framework/system/fiberjobqueue.h"

namespace Blaze
{
typedef eastl::intrusive_ptr<OutboundHttpConnectionManager> OutboundHttpConnectionManagerPtr;

namespace Authorization
{
    class GroupManager;
}

namespace NucleusIdentity
{
    class IpGeoLocation;
}

// we use South Pole/Antarctica because it's the place farthest away from end-users with easily recognizable coordinates, ~2500 miles away from the southernmost tip of South America.
static const float GEO_IP_DEFAULT_LATITUDE = -90.0f;
static const float GEO_IP_DEFAULT_LONGITUDE = 0.0f;

class NameHandler;

const char8_t* ClientTypeToPinString(ClientType clientType);

/*! *****************************************************************************/
/*! \class 
    \brief Manager for all user instances.

    There is one master user manager 
*********************************************************************************/
class UserSessionManager :
    public UserSessionsSlaveStub,
    private IdentityProvider,
    private UserSetProvider,
    private ExtendedDataProvider,
    private RemoteServerInstanceListener,
    private CensusDataProvider
{
public:
    static const int32_t FREE_SESSION_ID_PERIOD_MS = 120000;

    UserSessionManager();
    ~UserSessionManager() override;

    // DB setup
    uint16_t getDbSchemaVersion() const override;
    // Special case that treats Gen4/5 as the same DB:
    const ClientPlatformSet& getUserInfoDbPlatforms() const;

    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }
    bool requiresDbForPlatform(ClientPlatformType platform) const override { return true; }

    BlazeRpcError DEFINE_ASYNC_RET(upsertUserInfo(const UserInfoData& userInfo, bool nullExternalId, bool updateTimeFields, bool updateCrossPlatformOpt, bool& isFirstLogin, bool& isFirstConsoleLogin, uint32_t& previousAccountCountry, bool broadcastUpdateNotification));
    BlazeRpcError DEFINE_ASYNC_RET(upsertUserAndAccountInfo(const UserInfoData& userInfo, UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool updateCrossPlatformOpt, bool broadcastUpdateNotification));
    BlazeRpcError DEFINE_ASYNC_RET(updateOriginPersonaName(AccountId nucleusAccountId, const char8_t* originPersonaName, bool broadcastUpdateNotification));
    BlazeRpcError DEFINE_ASYNC_RET(updateLastUsedLocaleAndError(const UserInfoData& userInfo));

    BlazeRpcError DEFINE_ASYNC_RET(updateExtendedData(UserSessionId userSessionId, uint16_t componentId, uint16_t key, UserExtendedDataValue value) const);
    BlazeRpcError DEFINE_ASYNC_RET(removeExtendedData(UserSessionId userSessionId, uint16_t componentId, uint16_t key) const);
    BlazeRpcError DEFINE_ASYNC_RET(updateExtendedData(UpdateExtendedDataRequest& request) const);
    BlazeRpcError DEFINE_ASYNC_RET(updateUserSessionServerAttribute(BlazeId blazeId, UpdateUserSessionAttributeRequest& request));

    BlazeRpcError DEFINE_ASYNC_RET(insertBlazeObjectIdForSession(UserSessionId sessionId, const EA::TDF::ObjectId& bobjId));
    BlazeRpcError DEFINE_ASYNC_RET(insertBlazeObjectIdForUser(BlazeId blazeId, const EA::TDF::ObjectId& bobjId));
    BlazeRpcError DEFINE_ASYNC_RET(removeBlazeObjectIdForSession(UserSessionId sessionId, const EA::TDF::ObjectId& bobjId));
    BlazeRpcError DEFINE_ASYNC_RET(removeBlazeObjectIdForUser(BlazeId blazeId, const EA::TDF::ObjectId& bobjId));
    BlazeRpcError DEFINE_ASYNC_RET(removeBlazeObjectTypeForSession(UserSessionId sessionId, const EA::TDF::ObjectType& bobjId));
    BlazeRpcError DEFINE_ASYNC_RET(removeBlazeObjectTypeForUser(BlazeId blazeId, const EA::TDF::ObjectType& bobjId));

    /*! ************************************************************************************************/
    /*!
        \brief get the componentId and UserExtendedDataKey using the UserExtendedDataName from the replicated
            map.  This map is loaded on config time via the component.
    
        \param[in] name - The name of the User Extended Data to get in the form component.data (ie. stats.dnf).
        \param[out] componentId - The component id that this extended data belongs to.
        \param[out] key - The UserExtendedDataKey that corresponds to the name.
        \return true if the name is valid and the key exists.
    *************************************************************************************************/
    bool getUserExtendedDataId(const UserExtendedDataName& name, uint16_t& componentId, uint16_t& dataId);
    
    /*! ************************************************************************************************/
    /*!
        \brief get the UserExtendedDataKey using the UserExtendedDataName from the replicated
            map.  This map is loaded on config time via the component.
    
        \param[in] name - The name of the User Extended Data to get in the form component.data (i.e. stats.dnf).
        \param[out] key - The UserExtendedDataKey that corresponds to the name (high 16 bits componentId, low 16 bits dataId).
        \return true if the name is valid and the key exists.
    *************************************************************************************************/
    bool getUserExtendedDataKey(const UserExtendedDataName& name, UserExtendedDataKey& key);

    /*! ************************************************************************************************/
    /*!
    \brief get the UserExtendedDataName from the replicated map using the UserExtendedDataKey. 
        This map is loaded on config time via the component. This can be a bit inefficient, as we map names to IDs but not the reverse.
        Iterative search, but expectations are that this map of a size in the tens, with over 100 as an extreme case.
    
        \param[in] key - The UserExtendedDataKey that corresponds to the name (high 16 bits componentId, low 16 bits dataId).
        \return nullptr if the key didn't exist in the map
    *************************************************************************************************/
    const char8_t* getUserExtendedDataName(UserExtendedDataKey key);

    /*! ************************************************************************************************/
    /*!
        \brief consistent way for components to add name/Id pairs to the UserSessionExdendedDataIdsMap.
            Will store the names in the format 'component.name' where component is derived from
            componentId.
    
        \param[out]  userExtendedDataIdMap - the userExtendedDataIdMap to add the name/Id pair to.
        \param[in] name - the name of the data to be added to the map.
        \param[in] componentId - The component id adding data to the user extended data.
        \param[in] dataId - The id of the data being added to the user extended data.
    *************************************************************************************************/
    static void addUserExtendedDataId(UserExtendedDataIdMap &userExtendedDataIdMap, const char8_t* name, uint16_t componentId, uint16_t dataId);

    const UserExtendedDataIdMap& getUserExtendedDataIdMap() const { return mUserExtendedDataIdMap; }

    void addUserRegionForCensus();
    void getOnlineClubsUsers(BlazeIdSet &blazeIds);
    void insertUniqueDeviceIdForUserSession(UserSessionId sessionId, const char8_t* uniqueDeviceid);
    void updateDsUserIndexForUserSession(UserSessionId sessionId, int32_t dirtySockUserIndex);

    BlazeRpcError DEFINE_ASYNC_RET(getIpAddressForUserSession(UserSessionId sessionId, IpAddress& ipAddress));

    //User lookup
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByBlazeIds(const BlazeIdVector& blazeIds, BlazeIdToUserInfoMap& results, bool ignoreStatus = false, bool isRepLagAcceptable = true, ClientPlatformType clientPlatformType = INVALID));
//    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByExternalIds(const ExternalIdVector& externalIds, ExternalIdToUserInfoMap& results, bool ignoreStatus = false, ClientPlatformType clientPlatformType = INVALID));  // DEPRECATED
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPlatformInfoVector(const PlatformInfoVector& platformInfoList, UserInfoPtrList& results, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByAccountIds(const AccountIdVector& accountIds, UserInfoPtrList& results, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByAccountIds(const AccountIdVector& accountIds, ClientPlatformType platform, UserInfoPtrList& results, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByOriginPersonaIds(const OriginPersonaIdVector& originPersonaIds, ClientPlatformType platform, UserInfoPtrList& results, bool ignoreStatus = false));

    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByBlazeId(BlazeId blazeId, UserInfoPtr& userInfo, bool ignoreStatus = false, bool isRepLagAcceptable = true, ClientPlatformType clientPlatformType = INVALID));
//    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByExternalId(ExternalId externalId, UserInfoPtr& userInfo, bool ignoreStatus = false, ClientPlatformType clientPlatformType = INVALID));  // DEPRECATED
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPlatformInfo(const PlatformInfo& platformInfo, UserInfoPtr& userInfo, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByAccountId(AccountId accountId, UserInfoPtrList& results, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByAccountId(AccountId accountId, ClientPlatformType platform, UserInfoPtr& userInfo, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByOriginPersonaId(OriginPersonaId originPersonaId, ClientPlatformType platform, UserInfoPtr& userInfo, bool ignoreStatus = false));

    // Single-namespace lookups
    // Note that these methods will not return results for users who don't use the requested namespace
    // (e.g. a user on platform 'xone' with xbl gamer tag 'xboxname' and Origin persona name 'originname' will not be found by a search for 'originname' in the Origin namespace, but he will be found by a search for 'xboxname' in the xbox namespace)
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNames(const PersonaNameVector& personaNames, PersonaNameToUserInfoMap& results, const char8_t* personaNamespace = nullptr, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNames(const PersonaNameVector& personaNames, ClientPlatformType platform, PersonaNameToUserInfoMap& results, const char8_t* personaNamespace = nullptr, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNamePrefix(const char8_t* personaNamePrefix, ClientPlatformType platform, const uint32_t maxResultCount, PersonaNameToUserInfoMap& results, const char8_t* personaNamespace = nullptr));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaName(const char8_t* personaName, ClientPlatformType platform, UserInfoPtr& userInfo, const char8_t* personaNamespace = nullptr, bool ignoreStatus = false));

    // Multi-namespace lookups
    // If 'primaryNamespaceOnly' is true, these methods will only return results for users on their primary namespace ('xbox' for xone users, 'ps3' for ps4 users, etc.)
    // If 'primaryNamespaceOnly' is false, these methods will include results for users whose Origin persona name is in the searchlist, even if they're on a platform that doesn't use the Origin namespace
    // If 'restrictCrossPlatformResults' is true, then gCurrentUserSession is required in order to determine which platforms and namespaces the caller is permitted to look up
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNamesMultiNamespace(const PersonaNameVector& personaNames, UserInfoListByPersonaNameByNamespaceMap& results, bool primaryNamespaceOnly, bool restrictCrossPlatformResults, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNamePrefixMultiNamespace(const char8_t* personaNamePrefix, const uint32_t maxResultCountPerPlatform, UserInfoListByPersonaNameByNamespaceMap& results, bool primaryNamespaceOnly, bool restrictCrossPlatformResults));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNameMultiNamespace(const char8_t* personaName, UserInfoListByPersonaNameByNamespaceMap& results, bool primaryNamespaceOnly, bool restrictCrossPlatformResults, bool ignoreStatus = false));

    // Origin name lookup
    // These methods find users across all hosted platforms, even those that don't use the Origin namespace
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByOriginPersonaNames(const PersonaNameVector& personaNames, const ClientPlatformSet& platformSet, UserInfoListByPersonaNameMap& results));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByOriginPersonaNamePrefix(const char8_t* personaNamePrefix, const ClientPlatformSet& platformSet, const uint32_t maxResultCountPerPlatform, UserInfoListByPersonaNameMap& results));

    // Backward compatible method for single-platform cluster
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaName(const char8_t* personaName, UserInfoPtr& userInfo, const char8_t* personaNamespace = nullptr, bool ignoreStatus = false));

    // If needed, re-implement these deprecated methods (but for single-platform deployments only)
    /*
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNamesMultiNamespace(const PersonaNameVector& personaNames, NamespaceToPersonaUserInfoMap& results, bool ignoreStatus = false));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNamePrefix(const char8_t* personaNamePrefix, const uint32_t maxResultCount, PersonaNameToUserInfoMap& results, const char8_t* personaNamespace = nullptr));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNamePrefixMultiNamespace(const char8_t* personaNamePrefix, const uint32_t maxResultCount, NamespaceToPersonaUserInfoMap& results));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByPersonaNameMultiNamespace(const char8_t* personaName, NamespaceToPersonaUserInfoMap& results, bool ignoreStatus = false));
     */
    

    BlazeRpcError DEFINE_ASYNC_RET(updateUserInfoAttribute(const BlazeIdVector &ids, UserInfoAttribute attribute, UserInfoAttributeMask mask, ClientPlatformType platform = ClientPlatformType::INVALID));

    BlazeRpcError DEFINE_ASYNC_RET(requestUserExtendedData(const UserInfoData& userData, UserSessionExtendedData& extendedData, bool copyFromExistingSession = true));
    BlazeRpcError DEFINE_ASYNC_RET(refreshUserExtendedData(const UserInfoData& userData, UserSessionExtendedData& extendedData));
    BlazeRpcError DEFINE_ASYNC_RET(getRemoteUserExtendedData(UserSessionId remoteSessionId, UserSessionExtendedData& extendedData, UserSessionId subscriberSessionId = INVALID_USER_SESSION_ID));
    BlazeRpcError DEFINE_ASYNC_RET(getRemoteUserInfo(UserSessionId remoteSessionId, UserInfoData& userInfo));

    BlazeRpcError DEFINE_ASYNC_RET(sessionOverrideGeoIPById(GeoLocationData& overrideData));
    BlazeRpcError DEFINE_ASYNC_RET(setUserGeoOptIn(BlazeId blazeId, ClientPlatformType platform, bool optInFlag));
    BlazeRpcError DEFINE_ASYNC_RET(fetchGeoIPDataById(GeoLocationData& data, bool checkOptIn = true));
    BlazeRpcError DEFINE_ASYNC_RET(sessionResetGeoIPById(BlazeId blazeId));
    BlazeRpcError DEFINE_ASYNC_RET(updateUserSessionExtendedIds(const UserExtendedDataIdMap &userExtendedDataIdMap));
    BlazeRpcError DEFINE_ASYNC_RET(waitForUserSessionExistence(UserSessionId userSessionId, const EA::TDF::TimeValue& relTimeout = EA::TDF::TimeValue()));

    BlazeRpcError DEFINE_ASYNC_RET(updateUserCrossPlatformOptIn(const OptInRequest& optInRequest, ClientPlatformType platform));
    BlazeRpcError DEFINE_ASYNC_RET(lookupUserInfoByIdentification(const UserIdentification& userId, UserInfoPtr& userInfo, bool restrictCrossPlatformResults = false));

    UpdateLocalUserGroupError::Error processUpdateLocalUserGroup(const UpdateLocalUserGroupRequest& request, UpdateLocalUserGroupResponse& response, const Message* message) override;

    //Immediate user lookup
    UserInfoPtr getUser(BlazeId blazeId) { return findResidentUserInfoByBlazeId(blazeId); }
    UserInfoPtr getUserByPersona(const char8_t* personaName, ClientPlatformType platform, const char8_t* personaNamespace = nullptr) { return findResidentUserInfoByPersonaName(personaName, platform, personaNamespace); }
//    UserInfoPtr getUserByExternalId(ExternalId externalId) { return findResidentUserInfoByExternalId(externalId); }
    UserInfoPtr getUserByPlatformInfo(const PlatformInfo& platformInfo) { return findResidentUserInfoByPlatformInfo(platformInfo); }
    UserInfoPtr getUserByOriginPersonaId(OriginPersonaId originPersonaId, ClientPlatformType platform) { return findResidentUserInfoByOriginPersonaId(originPersonaId, platform); }
    UserInfoPtr getUserByNucleusAccountId(AccountId accountId, ClientPlatformType platform) { return findResidentUserInfoByAccountId(accountId, platform); }

    UserSessionId getPrimarySessionId(BlazeId id);
    BlazeId getBlazeId(UserSessionId id);
    BlazeId getUserId(UserSessionId id) { return getBlazeId(id); } // DEPRECATED
    // ExternalId getExternalId(UserSessionId id);  // DEPRECATED
    const PlatformInfo& getPlatformInfo(UserSessionId id);
    AccountId getAccountId(UserSessionId id);
    const char8_t* getPersonaName(UserSessionId id);
    const char8_t* getPersonaNamespace(UserSessionId id);
    const char8_t* getUniqueDeviceId(UserSessionId id);
    DeviceLocality getDeviceLocality(UserSessionId id);
    uint32_t getClientAddress(UserSessionId id);
    uint32_t getAccountCountry(UserSessionId id);
    Locale getAccountLocale(UserSessionId id);
    Locale getSessionLocale(UserSessionId id);
    ClientType getClientType(UserSessionId id);
    const char8_t* getProtoTunnelVer(UserSessionId id);
    const char8_t* getClientVersion(UserSessionId id);
    void getServiceName(UserSessionId id, ServiceName& serviceName);
    void getProductName(UserSessionId id, ProductName& productName);
    bool isSessionAuthorized(UserSessionId id, const Authorization::Permission& permission, bool suppressWarnLog = false);
    bool getSessionExists(UserSessionId id);
    bool isUserOnline(BlazeId id);
    bool isAccountOnline(AccountId accountId);
    uint32_t getOnlineBlazeIdsByAccountId(AccountId accountId, BlazeIdSet& blazeIds);

    EA::TDF::TimeValue getSessionCleanupTimeout(ClientType clientType) const;

    // UserSessionId lookup
    UserSessionPtr getSession(UserSessionId userSessionId);
    UserSessionId getUserSessionIdByBlazeId(BlazeId blazeId) const;
    // UserSessionId getUserSessionIdByExternalId(ExternalId externalId) const;  // DEPRECATED
    UserSessionId getUserSessionIdByPlatformInfo(const PlatformInfo& platformInfo) const;
    UserSessionId getUserSessionIdByAccountId(AccountId accountId, ClientPlatformType platform) const;
    void getUserSessionIdsByAccountId(AccountId accountId, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const = nullptr, bool localOnly = false) const;
    void getUserSessionIdsByBlazeId(BlazeId blazeId, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const = nullptr, bool localOnly = false) const;
    // void getUserSessionIdsByExternalId(ExternalId externalId, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const = nullptr, bool localOnly = false) const;  // DEPRECATED
    void getUserSessionIdsByPlatformInfo(const PlatformInfo& platformInfo, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const = nullptr, bool localOnly = false) const;

    /*! \brief Subscribes a class for user session events. */
    void addSubscriber(UserSessionSubscriber& subscriber);
    /*! \brief Unsubscribes a class for user session events. */
    void removeSubscriber(UserSessionSubscriber& subscriber);
    /*! \brief registers a class as a user session extended data provider */
    bool registerExtendedDataProvider(ComponentId componentId, ExtendedDataProvider& provider);
    /*! \brief deregisters a class as a user session extended data provider */
    bool deregisterExtendedDataProvider(ComponentId componentId);

    UserSessionId getUserSessionIdFromSessionKey(const char8_t *sessionKey) const;

    typedef EA::TDF::Tdf* (*CustomDataFactoryFunc)();
    static void registerCustomDataFactory(CustomDataFactoryFunc function);

    // return the current number of user sessions logged into blaze with the specified client type
    //   if multiple logins are allowed, duplicate user logins are counted
    uint32_t getUserSessionCountByClientType(ClientType clientType, eastl::list<eastl::string>* productNames = nullptr) const;
    // Get the Normal/Limited Gameplay user session count:
    uint32_t getGameplayUserSessionCount(eastl::list<eastl::string>* productNames = nullptr) const;
    // return the current number of user local sessions logged into blaze instance with the specified client type
    //   if multiple logins are allowed, duplicate user logins are counted
    uint32_t getLocalUserSessionCountByClientType(ClientType clientType) const;
    // return the current number of local user sessions across all client types
    //   if multiple logins are allowed, duplicate user logins are counted
    uint32_t getLocalUserSessionCount() const;
    // return the current number of user sessions logged into this instance, 
    // mapped by client type and geoip data (isp, country and timezone)
    BlazeRpcError getLocalUserSessionCounts(Blaze::GetPSUResponse &response) const;

    const NameHandler& getNameHandler() const { return *mNameHandler; }

    //  return a list of user sessions by Id attached to the specified Blaze user.
    BlazeRpcError getSessionIds(BlazeId blazeId, UserSessionIdList& ids);

    /* \brief return server metrics 
        For a full description of the metrics for this health check, see
        https://docs.developer.ea.com/display/blaze/Blaze+Health+Check+Definitions */
    void getStatusInfo(ComponentStatus& status) const override;

    /* \brief Used by GameManager and UserSessionManager to gather metrics by latency.
              Returns the bucket in which the given latency belongs. */
    static uint16_t getLatencyIndex(uint32_t latency);

    // Identity provider interface
    BlazeRpcError getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results) override;
    BlazeRpcError getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& entityName, IdentityInfo& result) override;

    //get GeoIpData from geoIp lookup
    // \return false if geolocation data could not be looked up, true otherwise
    bool getGeoIpData(const InetAddress& connAddr, GeoLocationData& geoLocData);

    //get GeoIpData from geoIp lookup, but first check the database for any location overrides
    BlazeRpcError getGeoIpDataWithOverrides(BlazeId id, const InetAddress& connAddr, GeoLocationData& geoLocData);

    // extract GeoIpData from Nucleus info, but also check the database for any location overrides
    BlazeRpcError convertGeoLocationData(BlazeId id, const NucleusIdentity::IpGeoLocation& geoLoc, GeoLocationData& geoLocData);

    // access group related functions
    Authorization::GroupManager* getGroupManager()const { return mGroupManager;};

    // qos settings related function
    QosConfig& getQosConfig() { return mQosConfig;}
    UserSessionsManagerMetrics& getMetricsObj() { return mMetrics; };

    // User Data helpers
    BlazeRpcError DEFINE_ASYNC_RET(filloutUserData(const UserInfo& userInfo, Blaze::UserData& userData, bool includeNetworkAddress, ClientPlatformType callerPlatform));
    static void obfuscate1stPartyIds(ClientPlatformType callerPlatform, PlatformInfo& platformInfo);

    // for SHA256 result as string, 42 base64 digits represent result's 256 bits, plus null terminator
    static void createSHA256Sum(char8_t* buf, size_t bufLen, UserSessionId id, const Blaze::Int256Buffer& random256Bits, int64_t creationTime);

    bool shouldAuditSession(UserSessionId userSessionId);

    bool isGeoLocationEnabled() const { return getConfig().getUseGeoipData(); }
    bool getOnlyShowPrimaryPersonaAccounts() const { return getConfig().getOnlyShowPrimaryPersonaAccounts(); }

    uint32_t getConnectedUserCount() const { return (uint32_t)mExistenceByBlazeIdMap.size(); }
    ExtendedDataProvider* getLocalUserExtendedDataProvider(ComponentId componentId) const;

    static ConnectionGroupId makeConnectionGroupId(SliverId sliverId, const ConnectionId connId);

    static eastl::string formatAdminActionContext(const Message * message);

    //If blazeId is negative, it's a stateless user.
    static bool isStatelessUser(BlazeId blazeId)
    {
        return blazeId < 0;
    }

    typedef eastl::vector_map<uint64_t, const UserSessionClientUEDConfig*> UEDClientConfigMap;
    const UEDClientConfigMap& getUEDClientConfigMap() { return mUEDClientConfigMap; }

    bool isReputationDisabled();
    ReputationService::ReputationServiceUtilPtr getReputationServiceUtil();
    const ReputationService::ReputationServiceFactory& getReputationServiceUtilFactory() const { return mReputationServiceFactory; }
    BlazeId allocateNegativeId();

    bool DEFINE_ASYNC_RET(checkConnectivity(UserSessionId userSessionId, const EA::TDF::TimeValue& timeToWait));

    const ClientTypeFlags& getClientTypeDescription(ClientType clientType) const { return mClientTypeDescriptions[clientType]; }

    // sendPIN* methods intentionally not marked async, as they are fire and forget.
    void sendPINErrorEvent(const char8_t* errorId, RiverPoster::ErrorTypes errorType, const char8_t* serviceName = "", RiverPoster::PINEventHeaderCore* headerCore = nullptr);
    void sendPINErrorEventToUserSessionIdList(const char8_t* errorId, RiverPoster::ErrorTypes errorType, UserSessionIdList& userSessionIdList);
    void sendPINEvents(PINSubmissionPtr request);
    //sendPINEvents looks up the UserSessionExistenceData in order to fill out the PIN event header, so an alternate method is needed for failed logins
    //(where the required data is known, but there is no user session to look up)
    void sendPINLoginFailureEvent(RiverPoster::LoginEventPtr loginEvent, UserSessionId userSessionId, const UserSessionExistenceData& existenceData);

    const char8_t* getProductReleaseType(const char8_t* productName) const;

    void getProjectIdPlatforms(const char8_t* projectId, ClientPlatformTypeList& platforms) const;

    typedef eastl::hash_map<UserSessionId, UserSessionPtr> UserSessionPtrByIdMap;
    const UserSessionPtrByIdMap& getUserSessionPtrByIdMap() { return mUserSessionPtrByIdMap; }

    static RiverPoster::PINClientPlatform blazeClientPlatformTypeToPINClientPlatform(ClientPlatformType clientPlatformType);
    static const char8_t* blazeClientPlatformTypeToPINClientPlatformString(ClientPlatformType clientPlatformType);

    typedef eastl::map<ClientPlatformType, ClientPlatformSet> ClientPlatformRestrictionSet;
    const ClientPlatformSet& getUnrestrictedPlatforms(ClientPlatformType platform) const;
    const ClientPlatformSet& getNonCrossplayPlatformSet(ClientPlatformType platform) const;

    PreparedStatementId getUserInfoPreparedStatement(uint32_t dbId, UserInfoDbCalls::UserInfoCallType callType) const;

    static bool isCrossplayEnabled(const UserInfoData& data) { return data.getCrossPlatformOptIn(); }
    static bool isCrossplayEnabled(const UserSessionMaster& userSessionMaster) { return userSessionMaster.isCrossplayEnabled();  }

    static void getPlatformSetFromExternalIdentifiers(ClientPlatformSet& platformSet, const ExternalIdentifiers& externalIds);
    static BlazeRpcError setPlatformsAndNamespaceForCurrentUserSession(const ClientPlatformSet*& platformSet, eastl::string& targetNamespace, bool& crossPlatformOptIn, bool matchPrimaryNamespace = false);

    void obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const override;
    static void obfuscatePlatformInfo(ClientPlatformType platform, UserSessionLoginInfo& tdf);
    static void obfuscatePlatformInfo(ClientPlatformType platform, UserIdentification& tdf);
    static void obfuscatePlatformInfo(ClientPlatformType platform, UserIdentificationList& list);

    bool shouldSendNotifyEvent(const ClientPlatformSet& platformSet, const Notification& notification) const override;

protected:
    // UserSet interface
    BlazeRpcError getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) override;
    BlazeRpcError getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results) override;
    BlazeRpcError getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results) override;
    BlazeRpcError countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;
    BlazeRpcError countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;
    RoutingOptions getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId) override;

private:
    // Setup and initialization
    bool onActivate() override;
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onPrepareForReconfigure(const UserSessionsConfig& config) override;
    bool onResolve() override;
    void onShutdown() override;
    void validateAuthorizationConfig(AuthorizationConfig& config, ConfigureValidationErrors& validationErrors) const;
    void validatePINSettings(PINConfig& config, ConfigureValidationErrors& validationErrors) const;
    void validatePlatformRestrictions(UserSessionsConfig& config, ConfigureValidationErrors& validationErrors) const;
    bool onValidateConfig(UserSessionsConfig& config, const UserSessionsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    bool configureCommon(const UserSessionsConfig& map, bool reconfigure = false);
    void parseAuditPersonasConfig(const UserSessionsConfig& config);

    UserInfo* findResidentUserInfoByBlazeId(BlazeId blazeId);
    UserInfo* findResidentUserInfoByPersonaName(const char8_t* personaName, ClientPlatformType platform, const char8_t* personaNamespace = nullptr);
    //UserInfo* findResidentUserInfoByExternalId(ExternalId externalId);  // DEPRECATED
    UserInfo* findResidentUserInfoByPlatformInfo(const PlatformInfo& platformInfo);
    UserInfo* findResidentUserInfoByOriginPersonaId(OriginPersonaId originPersonaId, ClientPlatformType platform);
    UserInfo* findResidentUserInfoByAccountId(AccountId accountId, ClientPlatformType platform);

    void findUserInfoByOriginPersonaNamesHelper(const ClientPlatformSet& unrestrictedPlatforms, const PersonaNameVector& names, UserInfoListByPersonaNameMap& results, PlatformInfoVector& platformInfoList, PersonaNameVector& remainingNames);
    // Helper for lookupUserInfoByOriginPersonaName(Prefix)
    BlazeRpcError DEFINE_ASYNC_RET(getUserInfoFromBlazeIdMap(const BlazeIdsByPlatformMap& blazeIdMap, const ClientPlatformSet& includedPlatforms, const uint32_t maxResultCountPerPlatform, UserInfoListByPersonaNameMap& usersByOriginName));

    BlazeRpcError getCensusData(CensusDataByComponent& censusData) override;
    void addCensusResponse(const UserManagerCensusData& data, ConnectedCountsByPingSiteMap& connectedCountsByPingSiteMap);

    // Internal data helpers
    void addUserInfoToIndices(UserInfo* userInfo, bool cacheUserInfo);
    void removeUserInfoFromIndices(UserInfo* userInfo);
    void removeUserInfoFromIndicesByPersona(UserInfo* userInfo, const char8_t* personaName, const char8_t* personaNamespace);
    void updateUserInfoCache(BlazeId blazeId, AccountId accountId, bool broadcastUpdateNotification);

    void broadcastUserInfoUpdatedNotification(BlazeId blazeId, AccountId accountId = INVALID_ACCOUNT_ID);
    void onNotifyUserInfoUpdated(const UserInfoUpdated& data, UserSession*) override;

    //access control helper
    BlazeRpcError parseAuthorizationConfig(const UserSessionsConfig& userConfig, Authorization::GroupManager*& groupManager);

    // UserSessionsSlaveListener
    void onUserSubscriptionsUpdated(const NotifyUpdateUserSubscriptions& data, UserSession*) override;
    void onSessionSubscriptionsUpdated(const NotifyUpdateSessionSubscriptions& data, UserSession*) override;

    RegisterRemoteSlaveSessionError::Error processRegisterRemoteSlaveSession(const Message* message) override;
    RequestUserExtendedDataProviderRegistrationError::Error processRequestUserExtendedDataProviderRegistration(UpdateUserExtendedDataProviderRegistrationResponse &response, const Message* message) override;
    ValidateSessionKeyError::Error processValidateSessionKey(const ValidateSessionKeyRequest &request, SessionInfo &response, const Message *message) override;

    // Controller::RemoteServerInstanceListener
    void onRemoteInstanceChanged(const RemoteServerInstance& instance) override;

    void upsertUserSessionExistence(UserSessionExistenceData& existence);
    void onUpsertUserSessionExistence(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue);
    void onEraseUserSessionExistence(StorageRecordId recordId);

    void removeUserFromNegativeCache(const char8_t* personaNamespace, const char8_t* personaName);
    
    void handleExtendedDataChange(UserSession &wrapper);
    void updateSlaveMapping(InstanceId instanceId);

    void createClientTypeDescriptions();

    BlazeRpcError getUserExtendedDataInternal(UserSessionId userSessionId, UserSessionExtendedData& extendedData);

    // ExtendedDataProvider interface
    /* \brief The custom data for a user session is being loaded. */
    BlazeRpcError onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData) override;
    BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;

    void getRemoteUserExtendedDataProviderRegistration(InstanceId instanceId);
    void getRemoteUserExtendedDataProviderRegistrationFiber(InstanceId instanceId);
    void refreshRemoteUserExtendedDataProviderRegistrations();    

    // PIN events
    void doSendPINEvent(ClientPlatformType platform, RiverPoster::PINEventList& events);
    void filloutPINEventHeader(RiverPoster::PINEventHeaderCore& headerCore, UserSessionId userSessionId, const UserSessionExistenceData* existenceData) const;

private:
    // these methods are used while managing user session subscriptions
    friend class UserSessionsMasterImpl;
    friend class UserSession;
    friend class UserInfo;
    
    bool hasNonSiblingUserSubscriptions(BlazeId blazeId) const;

    static UserSessionId fetchUserSessionId(UserSessionId id) { return id; }
    template<typename InputIterator, typename SessionIdFetchFunction, typename SendToMasterFunction, typename Request>
    BlazeRpcError sendToMasters(InputIterator first, InputIterator last, SessionIdFetchFunction fetchFunction, SendToMasterFunction sendFunction, const Request& req) const;
    template<typename InputIterator, typename SessionIdFetchFunction, typename SendToMasterFunction, typename Request, typename Response>
    BlazeRpcError sendToMasters(InputIterator first, InputIterator last, SessionIdFetchFunction fetchFunction, SendToMasterFunction sendFunction, const Request& req, Response& resp) const;
    BlazeRpcError updateBlazeObjectIdList(UpdateBlazeObjectIdInfoPtr info, BlazeId blazeId);
    void doUpdateBlazeObjectIdListByUserSessionId(UpdateBlazeObjectIdInfoPtr updateInfo);
    void doUpdateBlazeObjectIdListByBlazeId(UpdateBlazeObjectIdInfoPtr updateInfo, BlazeId blazeId);

    void getStatusInfoGlobals(ComponentStatus& status) const;

    //All our index maps are done as intrusive nodes to avoid extra allocations per session.  When each session
    //is created, we create a session wrapper object and use that to insert into all the various indices.

    //A note that most of our maps and structures all use the intrusive eastl classes.  This leads to a few
    //template backflips: (a) because we have lots of maps each map has its own node to avoid template collision,
    //(b) we friend the eastl::use_intrusive_key class in order to prevent having to define an "mKey" member 
    //inside the special node structures.  This class is defined in a templated specialization and grabs the 
    //key through an accessor method at the end of this file. 

    // NOTE:  Most of these typedefs intentionally use UserInfo* (NOT UserInfoPtr) because on the LRU Cache and 
    //  UserSessionMaster are intended to hold actual ref counts of the User.  One the User leaves the LRU cache, it
    //  should call its destructor, which will clean it up. 

    //Blaze id - > user info object lookup
    typedef eastl::hash_map<BlazeId, UserInfo*> UserInfoByBlazeIdMap;
    UserInfoByBlazeIdMap mUsersByBlazeId;

    // these mappings should not contain users who have linked accounts, but are logged in from a different platform.
    typedef eastl::hash_map<ExternalPsnAccountId, UserInfo*> UserInfoByPsnAccountIdMap;
    UserInfoByPsnAccountIdMap mUsersByPsnAccountId;
    typedef eastl::hash_map<ExternalXblAccountId, UserInfo*> UserInfoByXblAccountIdMap;
    UserInfoByXblAccountIdMap mUsersByXblAccountId;
    typedef eastl::hash_map<ExternalSwitchId, UserInfo*, EA::TDF::TdfStringHash> UserInfoBySwitchIdMap;
    UserInfoBySwitchIdMap mUsersBySwitchId;
    typedef eastl::hash_map<ExternalSteamAccountId, UserInfo*> UserInfoBySteamAccountIdMap;
    UserInfoBySteamAccountIdMap mUsersBySteamAccountId;
    typedef eastl::hash_map<ExternalStadiaAccountId, UserInfo*> UserInfoByStadiaAccountIdMap;
    UserInfoByStadiaAccountIdMap mUsersByStadiaAccountId;

    // A single OriginPersonaId or NucleusAccountId can apply to multiple Users (one per platform).
    typedef eastl::hash_map<AccountId, UserInfo*> UserInfoByAccountIdMap;
    typedef eastl::map<ClientPlatformType, UserInfoByAccountIdMap> UserInfoByAccountIdByPlatformMap;
    UserInfoByAccountIdByPlatformMap mUsersByAccountId;

    typedef eastl::hash_map<OriginPersonaId, UserInfo*> UserInfoByOriginPersonaIdMap;
    typedef eastl::map<ClientPlatformType, UserInfoByOriginPersonaIdMap> UserInfoByOriginPersonaIdByPlatformMap;
    UserInfoByOriginPersonaIdByPlatformMap mUsersByOriginPersonaId;

    //Persona name -> user info object lookup.
    struct PersonaHash
    {
        size_t operator()(const eastl::string &name) const;
    };

    struct PersonaEqualTo
    {
        bool operator()(const eastl::string &name1, const eastl::string &name2) const;
    };
    
    typedef eastl::hash_map<eastl::string, UserInfoByPlatform, PersonaHash, PersonaEqualTo> UserInfoByPersonaMap;
    typedef eastl::hash_map<eastl::string, UserInfoByPersonaMap> PersonaMapByNamespaceMap;
    PersonaMapByNamespaceMap mPersonaMapByNamespaceMap;

    template < typename K, typename Hash = eastl::hash<K>, typename Predicate = eastl::equal_to<K> >
    class NegativeUserInfoLookupCache : public LruCache<K, uint32_t, Hash, Predicate>
    {
    public:
        NegativeUserInfoLookupCache() :
            LruCache<K, uint32_t, Hash, Predicate>(1, BlazeStlAllocator("UserSessionManager::NegativeUserInfoLookupCache", UserSessionManager::COMPONENT_MEMORY_GROUP)) {}
        ~NegativeUserInfoLookupCache() override {}

        BlazeRpcError shouldPerformLookup(const K& key)
        {
            uint32_t count = 0;
            if (this->get(key, count))
            {
                // If next lookup will overstep the limit, it doesn't use the negative cache (hit positive cache and/or db below)
                if (++count >= MAX_NEGATIVE_CACHE_HIT_COUNT)
                    this->remove(key);
                else
                    this->add(key, count);

                return USER_ERR_USER_NOT_FOUND;
            }

            return ERR_OK;
        }

        void reportLookupResult(const K& key, BlazeRpcError err)
        {
            if (err == USER_ERR_USER_NOT_FOUND)
                this->add(key, 0);
        }
    };

    //At max negative cache hits we give the positive cache and/or db a try instead (safety valve).
    static const uint32_t MAX_NEGATIVE_CACHE_HIT_COUNT = 30;

    // Negative lookup caches.  Used to track failed user lookup attempts so future lookups for users who
    // are known to not exist on this Blaze server can early out.
    
    typedef NegativeUserInfoLookupCache<eastl::string, PersonaHash, PersonaEqualTo> NegativePersonaNameCache;
    typedef eastl::hash_map<eastl::string, NegativePersonaNameCache> NegativePersonaNameCacheByNamespaceMap;
    NegativePersonaNameCacheByNamespaceMap mNegativePersonaNameCacheByNamespaceMap;

    // other containers
    typedef eastl::hash_map<BlazeId, BlazeIdExistenceList> ExistenceByBlazeIdMap;
    typedef eastl::hash_map<AccountId, AccountIdExistenceList> ExistenceByAccountIdMap;
    typedef eastl::hash_map<ExternalPsnAccountId, ExternalPsnAccountIdExistenceList> ExistenceByPsnAccountIdMap;
    typedef eastl::hash_map<ExternalXblAccountId, ExternalXblAccountIdExistenceList> ExistenceByXblAccountIdMap;
    typedef eastl::hash_map<ExternalSwitchId, ExternalSwitchIdExistenceList, EA::TDF::TdfStringHash> ExistenceBySwitchIdMap;
    typedef eastl::hash_map<ExternalSteamAccountId, ExternalSteamAccountIdExistenceList> ExistenceBySteamAccountIdMap;
    typedef eastl::hash_map<ExternalStadiaAccountId, ExternalStadiaAccountIdExistenceList> ExistenceByStadiaAccountIdMap;
    typedef eastl::hash_map<BlazeId, PrimarySessionExistenceList> PrimaryExistenceByBlazeIdMap;

    ExistenceByBlazeIdMap mExistenceByBlazeIdMap;
    ExistenceByAccountIdMap mExistenceByAccountIdMap;
    ExistenceByPsnAccountIdMap mExistenceByPsnIdMap;
    ExistenceByXblAccountIdMap mExistenceByXblIdMap;
    ExistenceBySteamAccountIdMap mExistenceBySteamIdMap;
    ExistenceBySwitchIdMap mExistenceBySwitchIdMap;
    ExistenceByStadiaAccountIdMap mExistenceByStadiaIdMap;
    PrimaryExistenceByBlazeIdMap mPrimaryExistenceByBlazeIdMap;

    typedef LruCache<BlazeId, UserInfoPtr> UserInfoLruCache;
    UserInfoLruCache mUserInfoLruCache;

    typedef eastl::hash_map<ComponentId, ExtendedDataProvider*> DataProvidersByComponent;
    DataProvidersByComponent mDataProviders;
    Dispatcher<ExtendedDataProvider> mExtendedDataDispatcher;

    typedef eastl::set<ComponentId> RemoteUserExtendedDataProviderSet;
    RemoteUserExtendedDataProviderSet mRemoteUEDProviders;

    UEDClientConfigMap mUEDClientConfigMap;

    UserExtendedDataIdMap mUserExtendedDataIdMap;

    ClientTypeFlags mClientTypeDescriptions[CLIENT_TYPE_INVALID + 1];
    // uint32_t mUserSessionCountByClientType[CLIENT_TYPE_INVALID + 1]; // current count of users logged in as the various client types
    UserSessionsGlobalMetrics mUserSessionsGlobalMetrics;

    Metrics::TaggedGauge<ClientType> mGaugeAuxLocalUserSessionCount;
    Metrics::TaggedGauge<Metrics::ProductName, ClientType, Metrics::BlazeSdkVersion> mGaugeUserSessionCount;
    Metrics::TaggedCounter<Metrics::ProductName, ClientType, Metrics::BlazeSdkVersion> mUserSessionsCreated;

    UserInfoDbCalls::PreparedStatementsByDbIdMap mUserInfoPreparedStatementsByDbId;

    Dispatcher<UserSessionSubscriber> mDispatcher;

    NameHandler* mNameHandler;

    Authorization::GroupManager* mGroupManager;
    Authorization::GroupManager* mTempGroupManager;

    ClientPlatformRestrictionSet mRestrictedPlatformSets;
    ClientPlatformRestrictionSet mNonCrossplayPlatformSets;

    ClientPlatformsByProjectIdMap mClientPlatformsByProjectIdMap;

    ClientPlatformSet mUserInfoDbPlatformSet;

    QosConfig mQosConfig;

    UserSessionsManagerMetrics mMetrics;

    InetAddress *mGeoWebAddress;

    typedef eastl::vector<Fiber::EventHandle> EventList;

    BlazeRpcError updateCurrentUserSessionsGeoIP(const GeoLocationData& overrideData);

    BlazeRpcError updateCurrentUserSessionsCrossPlatformOptIn(const OptInRequest& optInRequest);

    Blaze::ReputationService::ReputationServiceFactory mReputationServiceFactory;

    StorageListener::FieldHandler mExistenceFieldHandler;
    StorageListener::RecordHandler mExistenceRecordHandler;
    StorageListener mUserSessionExistenceListener;

    UserSessionPtrByIdMap mUserSessionPtrByIdMap;
    UserSessionPtrByIdMap mExtinctUserSessionPtrByIdMap; // This short lived cache enables sendPINEvents() to find the extinct session.

    typedef eastl::hash_map<UserSessionId, Fiber::WaitList> WaitListByUserSessionIdMap;
    WaitListByUserSessionIdMap mWaitListByUserSessionIdMap;

    static bool getUserSessionIdAndHashSumFromSessionKey(const char8_t *sessionKey, UserSessionId& sessionId, char8_t*& hashSum);

    FiberJobQueue mUpdateBlazeObjectIdJobQueue;

    void increasePINEventPayload();

    void reducePINEventPayload();

    RiverPinUtil mRiverPinUtil;
};

inline bool UserSessionManager::getSessionExists(UserSessionId id)
{
    return getBlazeId(id) != INVALID_BLAZE_ID;
}

// Implementation of the UserSessionsSlave component. This is available on each blaze instance (including coreSlaves). 
extern EA_THREAD_LOCAL UserSessionManager* gUserSessionManager; 
}

#endif //BLAZE_USER_SESSION_MANAGER_H

