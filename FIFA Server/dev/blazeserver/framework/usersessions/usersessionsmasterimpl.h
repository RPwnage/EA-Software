/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_SESSIONS_MASTER_IMPL_H
#define BLAZE_USER_SESSIONS_MASTER_IMPL_H

/*** Include files *******************************************************************************/
#include "framework/tdf/userdefines.h"
#include "framework/usersessions/groupmanager.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionsubscriber.h"
#include "framework/usersessions/usersessionsmetrics.h"
#include "framework/usersessions/userinfodb.h"
#include "framework/rpc/usersessionsmaster_stub.h"
#include "framework/rpc/blazecontrollerslave_stub.h"
#include "framework/tdf/userextendeddatatypes_server.h"
#include "framework/controller/controller.h"
#include "framework/storage/storagetable.h"
#include "framework/redis/rediskeymap.h"
#include "framework/util/locales.h"
#include "framework/system/fiberjobqueue.h"
#include "EASTL/hash_map.h"
#include "EASTL/map.h"
#include "EASTL/vector_set.h"
#include "EASTL/vector_map.h"
#include "EASTL/vector_multimap.h"

namespace Blaze
{

typedef eastl::vector_set<UserSessionId> UserSessionIdVectorSet;
typedef eastl::hash_set<UserSessionId> UserSessionIdHashSet;
typedef eastl::hash_map<UserSessionId, UserSessionMasterPtr> UserSessionMasterPtrByIdMap;

namespace NucleusIdentity
{
    class IpGeoLocation;
}
namespace NucleusConnect
{
    class GetAccessTokenResponse;
}

/*! *****************************************************************************/
/*! \class
    \brief Master implementation for user sessions.
*********************************************************************************/
class UserSessionsMasterImpl :
    public UserSessionsMasterStub
{
public:
    static const uint32_t AUX_USER_ASSIGN_BATCH_MAX = 1000;
    static const uint32_t AUX_USER_ASSIGN_DELAY_MSEC = 1000;
    
    UserSessionsMasterImpl();
    ~UserSessionsMasterImpl() override;

    UserSessionMasterPtr getLocalSession(UserSessionId userSessionId);
    UserSessionMasterPtr getLocalSessionBySessionKey(const char8_t* sessionKey);

    StorageTable& getUserSessionStorageTable() { return mUserSessionTable; }

    const UserSessionMasterPtrByIdMap& getOwnedUserSessionsMap() const { return mUserSessionMasterPtrByIdMap; }

    BlazeRpcError updateClientState(UserSessionId userSessionId, const ClientState& request);

    bool isLocalUserSessionUnresponsive(UserSessionId userSessionId);

    BlazeRpcError createNormalUserSession(const UserInfoData& userData, PeerInfo& connection, ClientType clientType, int32_t userIndex, const char8_t* productName,
        bool skipPeriodicEntitlementCheck, const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded,
        UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool dbUpsertCompleted, bool updateCrossPlatformOpt, const char8_t* deviceId = "", DeviceLocality deviceLocality = DEVICELOCALITY_UNKNOWN,
        const NucleusConnect::GetAccessTokenResponse* accessTokenResponse = nullptr, const char8_t* ipAddr = "");
    BlazeRpcError createTrustedUserSession(const UserInfoData& userData, PeerInfo& connection, ClientType clientType, int32_t userIndex, const char8_t* productName,
        const char8_t* accessScope, const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded);
    BlazeRpcError createWalUserSession(const UserSessionMaster& parentUserSession, UserSessionMasterPtr& childWalUserSession, bool& geoIpSucceeded);
    BlazeRpcError createGuestUserSession(const UserSessionMaster& parentUserSession, UserSessionMasterPtr& childGuestUserSession, PeerInfo& connection, uint32_t userIndex);
    BlazeRpcError destroyUserSession(UserSessionId userSessionId, UserSessionDisconnectType disconnectType, int32_t socketError, UserSessionForcedLogoutType forceLogoutType);

    void generateLoginEvent(BlazeRpcError err, UserSessionId sessionId, UserSessionType sessionType, const PlatformInfo& platformInfo, const char8_t* deviceId, DeviceLocality deviceLocality,
        PersonaId personaId, const char8_t* personaName, Locale connLocale, uint32_t connCountry, const char8_t* sdkVersion, ClientType clientType,
        const char8_t* ipaddress, const char8_t* serviceName, const char8_t* productName, const GeoLocationData& geoLocData,
        const char8_t* projectId, Locale accountLocale = LOCALE_BLANK, uint32_t accountCountry = 0,
        const char8_t* clientVersion = "", bool includePINErrorEvent = false, bool isUnderage = false);

    void sendLoginPINError(BlazeRpcError err, UserSessionId sessionId, const PlatformInfo& platformInfo,
        PersonaId personaId, Locale connLocale, uint32_t connCountry, const char8_t* sdkVersion, ClientType clientType,
        const char8_t* ipaddress, const char8_t* serviceName, const char8_t* productName,
        const char8_t* clientVersion, bool isUnderage = false);

    void addLocalUserSessionSubscriber(LocalUserSessionSubscriber& subscriber) { mDispatcher.addDispatchee(subscriber); }
    void removeLocalUserSessionSubscriber(LocalUserSessionSubscriber& subscriber) { mDispatcher.removeDispatchee(subscriber); }

    BlazeRpcError getConnectionGroupUserIds(const ConnectionGroupObjectId& connGroupId, UserIdentificationList& results);
    BlazeRpcError getConnectionGroupSessionIds(const ConnectionGroupObjectId& connGroupId, UserSessionIdList& result);
    BlazeRpcError getConnectionGroupUserBlazeIds(const ConnectionGroupObjectId& connGroupId, BlazeIdList& result);

    bool resolveAccessGroup(ClientType clientType, const PeerInfo& peerInfo, const char8_t* accessScope, AccessGroupName& accessGroupName);
    // Local Authentication component sets this. This information is only used in the login/logout events which are local to this
    // instance so we can rely on local authentication component to set this. Ideally, the user sessions component does not generate
    // this event but it is perhaps best dealt with when we perform a surgery on user sessions. 
    void setProductInfoMaps(const ProductTrialContentIdMap& productTrialContentIdMap, const ProductProjectIdMap& productProjectIdMap)
    {
        productTrialContentIdMap.copyInto(mProductTrialContentIdMap);
        productProjectIdMap.copyInto(mProductProjectIdMap);
    }

private:

    static const char8_t PUBLIC_USERSESSION_FIELD[];
    static const char8_t PRIVATE_USERSESSION_FIELD[];
    static const char8_t PRIVATE_CUSTOM_STATE_FIELD[];
    static const char8_t PRIVATE_CUSTOM_STATE_FIELD_FMT[];
    static const char8_t* VALID_USERSESSION_FIELD_PREFIXES[];

    //Setup and initialization
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onResolve() override;
    bool onActivate() override;
    void onShutdown() override;

    bool onValidateConfig(UserSessionsConfig& config, const UserSessionsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onPrepareForReconfigure(const UserSessionsConfig& newConfig) override;

    void onSlaveSessionRemoved(SlaveSession& session) override;

    void addLocalUserSession(UserSessionMasterPtr localUserSession, OwnedSliver& ownedSliver);
    void removeLocalUserSession(UserSessionMasterPtr localUserSession);

    BlazeRpcError createNewUserSessionInternal(
        const UserInfoData& userData,
        PeerInfo& connection,
        ClientType clientType,
        int32_t userIndex,
        const char8_t* productName,
        const char8_t* deviceId,
        DeviceLocality deviceLocality,
        const char8_t* accessScope,
        const SessionFlags& sessionFlags,
        bool isTrustedLogin,
        const NucleusIdentity::IpGeoLocation& geoLoc,
        bool& geoIpSucceeded,
        const NucleusConnect::GetAccessTokenResponse* accessTokenResponse, 
        const char8_t* ipAddr,
        UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults,
        bool dbUpsertCompleted,
        bool updateCrossPlatformOpt);

    BlazeRpcError finalizeAndInstallNewUserSession(UserSessionMasterPtr newUserSession, OwnedSliver& ownedSliver, PeerInfo* peerInfo, 
        const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded, const NucleusConnect::GetAccessTokenResponse* accessTokenResponse,const char8_t* ipAddr);

    void onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot);
    void onExportStorageRecord(StorageRecordId recordId);
    void upsertUserExistenceData(UserSessionExistenceData& userExistenceData);
    void upsertUserSessionData(UserSessionData& userSessionData);

    //Master commands
    GetCensusDataError::Error processGetCensusData(Blaze::UserManagerCensusData &response, const Message* message) override;
    UpdateGeoDataMasterError::Error processUpdateGeoDataMaster(const GeoLocationData& data,  const Message* message) override;
    GetUserExtendedDataError::Error processGetUserExtendedData(const Blaze::GetUserExtendedDataRequest& request, Blaze::GetUserExtendedDataResponse &response, const Message* message) override;
    GetUserInfoDataError::Error processGetUserInfoData(const Blaze::GetUserInfoDataRequest& request, Blaze::GetUserInfoDataResponse &response, const Message* message) override;
    GetUserSessionDataError::Error processGetUserSessionData(const Blaze::GetUserInfoDataRequest& request, Blaze::GetUserSessionDataResponse &response, const Message* message) override;
    UpdateExtendedDataMasterError::Error processUpdateExtendedDataMaster(const Blaze::UpdateExtendedDataRequest& request, const Message* message) override;
    UpdateBlazeObjectIdListError::Error processUpdateBlazeObjectIdList(const Blaze::UpdateBlazeObjectIdListRequest& request, const Message* message) override;
public:
    UpdateNetworkInfoMasterError::Error processUpdateNetworkInfoMaster(const Blaze::UpdateNetworkInfoMasterRequest& request, const Message* message) override;
private:
    RefreshQosPingSiteMapMasterError::Error processRefreshQosPingSiteMapMaster(const Blaze::RefreshQosPingSiteMapRequest& request, const Message* message) override;
    UpdateUserSessionServerAttributeMasterError::Error processUpdateUserSessionServerAttributeMaster(const Blaze::UpdateUserSessionAttributeMasterRequest& request, const Message* message) override;
    UpdateHardwareFlagsMasterError::Error processUpdateHardwareFlagsMaster(const Blaze::UpdateHardwareFlagsMasterRequest& request, const Message* message) override;
    //UpdateUserInfoMasterError::Error processUpdateUserInfoMaster(const UserInfoData &data, const Message* message);
    UpdateUserSessionClientDataMasterError::Error processUpdateUserSessionClientDataMaster(const Blaze::UpdateUserSessionClientDataMasterRequest& request, const Message* message) override;
    //UpdateUserInfoAttributeMasterError::Error processUpdateUserInfoAttributeMaster(const UpdateUserInfoAttributeRequest& request, const Message* message);
    InsertUniqueDeviceIdMasterError::Error processInsertUniqueDeviceIdMaster(const Blaze::InsertUniqueDeviceIdMasterRequest& request, const Message* message) override;
    GetUserIpAddressError::Error processGetUserIpAddress(const Blaze::GetUserIpAddressRequest & request, Blaze::GetUserIpAddressResponse &response, const Message* message) override;
    UpdateDirtySockUserIndexMasterError::Error processUpdateDirtySockUserIndexMaster(const UpdateDirtySockUserIndexMasterRequest& request, const Message* message) override;

    AddUserSessionSubscriberError::Error processAddUserSessionSubscriber(const Blaze::UserSessionSubscriberRequest& request, const Message* message) override;
    RemoveUserSessionSubscriberError::Error processRemoveUserSessionSubscriber(const Blaze::UserSessionSubscriberRequest& request, const Message* message) override;
    FetchUserSessionsError::Error processFetchUserSessions(const Blaze::FetchUserSessionsRequest& request, Blaze::FetchUserSessionsResponse& response, const Message* message) override;
    ValidateUserExistenceError::Error processValidateUserExistence(const Blaze::ValidateUserExistenceRequest& request, const Message* message) override;
    ForceSessionLogoutMasterError::Error processForceSessionLogoutMaster(const Blaze::ForceSessionLogoutRequest& request, const Message* message) override;

    void handleExtendedDataChange(UserSessionMaster& session);
    UpdateLocalUserGroupError::Error updateLocalUserGroup(const UpdateLocalUserGroupRequest& request, UpdateLocalUserGroupResponse& response, const Message* message);
    BlazeRpcError updateSlaveMapping(const UpdateSlaveMappingRequest& request);

    //Internal data helpers
    void doExtendedDataUpdate(UserSessionMaster& session, uint16_t componentId, const Blaze::UpdateExtendedDataRequest::ExtendedDataUpdate& update, bool removeKeys);
    void removeAuxSlave(InstanceId auxInstanceId);
    void assignSessionsToAuxSlaves(uint8_t slaveTypeIndex, InstanceId coreInstanceId);
    InstanceId addSessionToAuxSlave(uint8_t slaveTypeIndex, InstanceId coreInstanceId);
    void removeSessionFromAuxSlave(InstanceId auxInstanceId);

    void updateUserInfoInLocalUserSessions(BlazeId blazeId);
    
    bool isImmediatePartialLocalUserReplication(InstanceId instanceId);

    BlazeRpcError doUserSessionAttributeUpdate(const Blaze::UpdateUserSessionAttributeRequest& request, Blaze::BlazeId blazeId);


    void updateLocalUserSessionCountsAddOrRemoveSession(const UserSessionMaster& session, bool addSession);

    void performBlockingUserSessionCleanupOperations(UserSessionId userSessionId, BlazeId blazeId, AccountId accountId, ClientPlatformType platform, ClientType clientType);

    void generateLogoutEvent(const UserSessionMasterPtr userSession, UserSessionDisconnectType disconnectType, int32_t socketError);

    uint64_t allocateUniqueSliverKey(SliverId sliverId, uint16_t idType);
    UserSessionId allocateUniqueUserSessionId(SliverId sliverId);
    ConnectionGroupId allocateUniqueConnectionGroupId(SliverId sliverId);

    bool getCrossplayOptInForPin(const UserSessionMasterPtr userSession, ClientPlatformType clientPlatform) const;
private:
    friend class UserSessionManager;
    friend class UserSessionMaster;

    struct AuxSlaveInfo
    {
        AuxSlaveInfo(int32_t count = 0, uint8_t index = 0, bool immediatePartialLocalUserReplicationEnabled = false) :
            sessionCount(count), typeIndex(index), immediatePartialLocalUserReplication(immediatePartialLocalUserReplicationEnabled) {}

        int32_t sessionCount;
        uint8_t typeIndex;
        bool partialUserReplication;
        bool immediatePartialLocalUserReplication;
    };

    struct BlazeIdUserSessionId
    {
        BlazeId blazeId;
        UserSessionId sessionId;
        bool operator<(const BlazeIdUserSessionId& other) const
        {
            if (blazeId != other.blazeId)
                return (blazeId < other.blazeId);

            return (sessionId < other.sessionId);
        }
    };

    typedef eastl::map<BlazeIdUserSessionId, UserSessionMaster*> UserSessionsByBlazeIdMap;
    typedef eastl::vector_multimap<InstanceId, InstanceId> AuxSlaveIdByCoreSlaveIdMap;
    typedef eastl::vector<AuxSlaveIdByCoreSlaveIdMap> AuxSlaveIdByCoreSlaveIdMapVector;
    typedef eastl::pair<AuxSlaveIdByCoreSlaveIdMap::iterator, AuxSlaveIdByCoreSlaveIdMap::iterator> AuxSlaveIdByCoreSlaveIdMapItPair;
    typedef eastl::vector_map<InstanceId, AuxSlaveInfo> AuxSlaveInfoMap;
    typedef eastl::vector_map<uint64_t, const UserSessionClientUEDConfig*> UEDClientConfigMap;
    typedef eastl::hash_map<InstanceId, TimerId> CleanupTimerByInstanceIdMap;
    typedef eastl::hash_map<UserSessionId, eastl::string> DeviceIdByUserSessionIdMap;

    struct ConnectionGroupData
    {
        ConnectionGroupData() : mNotificationJobQueue("ConnectionGroupData::NotificationJobQueue") {}
        ~ConnectionGroupData()
        {
            EA_ASSERT(!mNotificationJobQueue.hasPendingWork());
        }
        UserSessionIdVectorSet mUserSessionIdSet;
        NotificationJobQueue mNotificationJobQueue;
    };
    typedef eastl::hash_map<ConnectionGroupId, ConnectionGroupData> ConnGroupUserSetMap;

    UserSessionsByBlazeIdMap mUserSessionsByBlazeId;
    AuxSlaveIdByCoreSlaveIdMapVector mAuxIdByCoreIdMap;
    AuxSlaveInfoMap mAuxInfoMap;
    UEDClientConfigMap mUEDClientConfigMap;
    ConnGroupUserSetMap mConnGroupUserSetMap;
    CleanupTimerByInstanceIdMap mCleanupTimerByInstanceIdMap;

    StorageTable mUserSessionTable;

    UserSessionMasterPtrByIdMap mUserSessionMasterPtrByIdMap;

    typedef eastl::pair<BlazeId, UserSessionId> SingleLoginIdentity;
    RedisKeyMap<eastl::string, SingleLoginIdentity> mSingleLoginIds;

    typedef eastl::hash_map<ConnectionGroupId, ConnectionGroupList> SessionByConnGroupIdMap;
    SessionByConnGroupIdMap mLocalSessionByConnGroupIdMap;

    Metrics::TaggedGauge<Metrics::ProductName, ClientType, ClientPlatformType, ClientState::Status, ClientState::Mode, Metrics::PingSite> mLocalUserSessionCounts;

    QosMetricsByGeoIPData mLocalQosMetrics[CLIENT_TYPE_INVALID + 1];

    uint64_t mNextNoDbUniqueUserSessionIdBase;

    FiberJobQueue mUserSessionCleanupJobQueue;
    static RedisScript msDeleteSingleLoginKeyScript;

    void makeSingleLoginKey(StringBuilder& singleLoginKey, AccountId accountId, ClientPlatformType platform)
    {
        singleLoginKey << "singleLogin.accountId." << accountId;
        if (!getConfig().getSingleLoginCheckPerAccount())
            singleLoginKey << ".platform." << ClientPlatformTypeToString(platform);
    }

    Dispatcher<LocalUserSessionSubscriber> mDispatcher;

    bool mQosSettingsChangedOnLastReconfigure;

    EA::TDF::TimeValue mCensusCacheExpiry;
    UserManagerCensusData mCensusCache;
    ProductTrialContentIdMap mProductTrialContentIdMap;
    ProductProjectIdMap mProductProjectIdMap;
};

// Implementation of the UserSessionsMaster component. This is available on coreSlaves only.
extern EA_THREAD_LOCAL UserSessionsMasterImpl* gUserSessionMaster;

}

#endif //BLAZE_USER_SESSIONS_MASTER_IMPL_H
