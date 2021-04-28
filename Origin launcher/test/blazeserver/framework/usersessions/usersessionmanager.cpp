/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/userinfodb.h"
#include "framework/usersessions/namehandler.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/socketutil.h"
#include "framework/usersessions/accountinfodb.h"

#include "framework/database/dbscheduler.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/component/componentmanager.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/tdf/userpineventtypes_server.h"
#include "framework/rpc/usersessionsmaster.h"
#include "framework/lobby/lobby/lobbynames.h"
#include "framework/util/random.h"
#include "framework/util/hashutil.h"
#include "framework/util/locales.h"
#include "framework/tdf/userevents_server.h"
#include "framework/event/eventmanager.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/system/debugsys.h"
#include "framework/oauth/accesstokenutil.h"

#include "framework/usersessions/reputationserviceutilfactory.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/tdf/userdefines.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/storage/storagemanager.h"
#include "framework/slivers/slivermanager.h"
#include "framework/util/riverpinutil.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "framework/util/shared/rawbufferistream.h"

#include "nucleusidentity/rpc/nucleusidentityslave.h"
#include "nucleusidentity/tdf/nucleusidentity.h" // for IpGeoLocation

#include <EAStdC/EAString.h>
#include <EASTL/scoped_array.h>
#include <EASTL/bitvector.h>
#include <openssl/sha.h> // for SHA256

#include "EAStdC/EAString.h"
#include "EAStdC/EAHashCRC.h"

namespace Blaze
{

// An internal admin group which is granted PERMISSION_ALL, this group 
// name should not be used in authorization.cfg !!
const char8_t* ADMIN_GROUP_NAME = "InternalAdminGroup";
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define CONVERT_FUNCTOR(X, TYPE) (*((TYPE *) ((FunctorBase *) &X))); 

/*** Public Methods ******************************************************************************/

size_t UserSessionManager::PersonaHash::operator()(const eastl::string &name) const
{
    return gUserSessionManager->getNameHandler().generateHashCode(name.c_str());
}

bool UserSessionManager::PersonaEqualTo::operator()(
        const eastl::string &name1, const eastl::string &name2) const
{
    return (gUserSessionManager->getNameHandler().compareNames(name1.c_str(), name2.c_str()) == 0);
}

// static
UserSessionsSlave* UserSessionsSlave::createImpl()
{
    return BLAZE_NEW_MGID(UserSessionManager::COMPONENT_MEMORY_GROUP, "Component") UserSessionManager();
}

UserSessionsManagerMetrics::UserSessionsManagerMetrics(Metrics::MetricsCollection& collection)
    : mUserLookupsByBlazeId(collection, "userLookupsByBlazeId")
    , mUserLookupsByPersonaName(collection, "userLookupsByPersonaName", Metrics::Tag::platform)
    , mUserLookupsByPersonaNameMultiNamespace(collection, "userLookupsByPersonaNameMultiNamespace")
    , mUserLookupsByExternalStringOrId(collection, "userLookupsByExternalStringOrId", Metrics::Tag::platform)
    , mUserLookupsByAccountId(collection, "userLookupsByAccountId", Metrics::Tag::platform)
    , mUserLookupsByOriginPersonaId(collection, "userLookupsByOriginPersonaId", Metrics::Tag::platform)
    , mUserLookupsByOriginPersonaName(collection, "userLookupsByOriginPersonaName")
    , mUserLookupsByBlazeIdDbHits(collection, "userLookupsByBlazeIdDbHits")
    , mUserLookupsByBlazeIdActualDbHits(collection, "userLookupsByBlazeIdDbHits", Metrics::Tag::platform)
    , mUserLookupsByPersonaNameDbHits(collection, "userLookupsByPersonaNameDbHits", Metrics::Tag::platform)
    , mUserLookupsByExternalStringOrIdDbHits(collection, "userLookupsByExternalStringOrIdDbHits", Metrics::Tag::platform)
    , mUserLookupsByAccountIdDbHits(collection, "userLookupsByAccountIdDbHits", Metrics::Tag::platform)
    , mUserLookupsByOriginPersonaIdDbHits(collection, "userLookupsByOriginPersonaIdDbHits", Metrics::Tag::platform)
    , mUserLookupsByOriginPersonaNameUserInfoDbHits(collection, "userLookupsByOriginPersonaNameUserDbHits", Metrics::Tag::platform)
    , mUserLookupsByOriginPersonaNameAccountInfoDbHits(collection, "userLookupsByOriginPersonaNameAccountDbHits")
    , mGaugeUserSessionSubscribers(collection, "gaugeUserSessionSubscribers")
    , mGaugeUserExtendedDataProviders(collection, "gaugeUserExtendedDataProviders")
    , mDisconnections(collection, "disconnections", Metrics::Tag::client_type, Metrics::Tag::disconnect_type, Metrics::Tag::client_status, Metrics::Tag::client_mode)
    , mBlazeObjectIdListUpdateCount(collection, "blazeObjectIdListUpdateCount")
    , mUEDMessagesCoallesced(collection, "UEDMessagesCoallesced")
    , mReputationLookups(collection, "reputationLookups")
    , mReputationLookupsWithValueChange(collection, "reputationLookupsWithValueChange")
    , mGeoIpLookupsFailed(collection, "geoIpLookupsFailed")
    , mPinEventsSent(collection, "PINEventsSent")
    , mPinEventBytesSent(collection, "PINEventBytesSent")
    , mGaugeUsers(collection, "gaugeUsers", []() { return gUserSessionManager->getConnectedUserCount(); })
    , mGaugeUserSessionsOverall(collection, "gaugeUserSessionsOverall", []() { return gUserSessionManager->getUserSessionPtrByIdMap().size(); })
    , mGaugeOwnedUserSessions(collection, "gaugeOwnedUserSessions", []() { return (gUserSessionMaster != nullptr ? gUserSessionMaster->getOwnedUserSessionsMap().size() : 0); })
{
}

void UserSessionsManagerMetrics::addPerPlatformStatusInfo(ComponentStatus::InfoMap& infoMap, const Metrics::TagPairList& tagList, const char8_t* metricName, const uint64_t value)
{
    char8_t keyBuf[128];
    char8_t valueBuf[64];
    blaze_snzprintf(keyBuf, sizeof(keyBuf), "%s_%s", metricName, tagList[0].second.c_str());
    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%" PRIu64, value);
    infoMap[keyBuf] = valueBuf;
}

UserSessionManager::UserSessionManager() :
    mUsersByAccountId(BlazeStlAllocator("UserSessionManager::mUsersByAccountId", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mUsersByOriginPersonaId(BlazeStlAllocator("UserSessionManager::mUsersByOriginPersonaId", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mPersonaMapByNamespaceMap(BlazeStlAllocator("UserSessionManager::mPersonaMapByNamespace", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mExistenceByBlazeIdMap(BlazeStlAllocator("UserSessionManager::mExistenceByBlazeIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mExistenceByAccountIdMap(BlazeStlAllocator("UserSessionManager::mExistenceByAccountIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mExistenceByPsnIdMap(BlazeStlAllocator("UserSessionManager::mExistenceByPsnIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mExistenceByXblIdMap(BlazeStlAllocator("UserSessionManager::mExistenceByXblIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mExistenceBySteamIdMap(BlazeStlAllocator("UserSessionManager::mExistenceBySteamIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mExistenceBySwitchIdMap(BlazeStlAllocator("UserSessionManager::mExistenceBySwitchIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mPrimaryExistenceByBlazeIdMap(BlazeStlAllocator("UserSessionManager::mPrimaryExistenceByBlazeIdMap", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mUserInfoLruCache(100, BlazeStlAllocator("UserSessionManager::mUserInfoLruCache", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mDataProviders(BlazeStlAllocator("UserSessionManager::mDataProviders", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mRemoteUEDProviders(BlazeStlAllocator("UserSessionManager::mRemoteUEDProviders", UserSessionManager::COMPONENT_MEMORY_GROUP)),
    mUserSessionsGlobalMetrics(getMetricsCollection()),
    mGaugeAuxLocalUserSessionCount(getMetricsCollection(), "gaugeAuxLocalUserSessionCount", Metrics::Tag::client_type),
    mGaugeUserSessionCount(getMetricsCollection(), "gaugeUserSessionCount", Metrics::Tag::product_name, Metrics::Tag::client_type, Metrics::Tag::blaze_sdk_version),
    mUserSessionsCreated(getMetricsCollection(), "userSessionsCreated", Metrics::Tag::product_name, Metrics::Tag::client_type, Metrics::Tag::blaze_sdk_version),
    mNameHandler(nullptr),
    mGroupManager(nullptr),
    mTempGroupManager(nullptr),
    mMetrics(getMetricsCollection()),
    mGeoWebAddress(nullptr),
    mUserSessionExistenceListener(UserSessionsMaster::COMPONENT_ID, UserSessionsMaster::COMPONENT_MEMORY_GROUP, UserSessionsMaster::LOGGING_CATEGORY),
    mUpdateBlazeObjectIdJobQueue("UserSessionManager::UpdateBlazeObjectIdJobQueue")
{
    mGaugeUserSessionCount.defineTagGroups({ { Metrics::Tag::client_type },
                                             { Metrics::Tag::product_name },
                                             { Metrics::Tag::product_name, Metrics::Tag::client_type },
                                             { Metrics::Tag::blaze_sdk_version },
                                             { Metrics::Tag::client_type, Metrics::Tag::blaze_sdk_version } 
        });

    mGaugeAuxLocalUserSessionCount.defineTagGroups({ { Metrics::Tag::client_type }
        });

    mUserSessionsCreated.defineTagGroups({ { Metrics::Tag::product_name },
                                           { Metrics::Tag::product_name, Metrics::Tag::client_type }
        });



    EA_ASSERT(gUserSessionManager == nullptr);
    gUserSessionManager = this;

    createClientTypeDescriptions();

    registerExtendedDataProvider(COMPONENT_ID, *this);

    for (Controller::NamespacesList::const_iterator it = gController->getAllSupportedNamespaces().begin(), end = gController->getAllSupportedNamespaces().end(); it != end; ++it)
    {
        mNegativePersonaNameCacheByNamespaceMap.insert(*it);
    }

    // initialize the storage field/record handlers
    mExistenceFieldHandler.fieldNamePrefix = UserSessionsMasterImpl::PUBLIC_USERSESSION_FIELD;
    mExistenceFieldHandler.upsertFieldCb = UpsertStorageFieldCb(this, &UserSessionManager::onUpsertUserSessionExistence);
    mExistenceFieldHandler.upsertFieldFiberContext = "UserSessionManager::onUpsertUserSessionExistence";

    mExistenceRecordHandler.eraseRecordCb = EraseStorageRecordCb(this, &UserSessionManager::onEraseUserSessionExistence);
    mExistenceRecordHandler.eraseRecordFiberContext = "UserSessionManager::onEraseUserSessionExistence";
    mUpdateBlazeObjectIdJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
}

UserSessionManager::~UserSessionManager()
{
    // clear self as notification listener
    removeNotificationListener(*this);

    //Delete all the cached users
    mUserInfoLruCache.reset();
    
    if (!mWaitListByUserSessionIdMap.empty())
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionMaster.~UserSessionManager: The mWaitListByUserSessionIdMap still contains " << mWaitListByUserSessionIdMap.size() << " entries, this may cause issues elsewhere." ); 
    }

    UserInfoDbCalls::shutdownUserInfoDbCalls();

    if (mNameHandler)
    {
        delete mNameHandler;
        mNameHandler=nullptr;
    }    

    if (mGroupManager)
    {
        delete mGroupManager;
        mGroupManager=nullptr;
    }

    //delete in case server shut down between PrepareForReconfigure and Reconfigure
    if (mTempGroupManager)
    {
        delete mTempGroupManager;
        mTempGroupManager=nullptr;
    }

    if (mGeoWebAddress)
    {
        delete mGeoWebAddress;
        mGeoWebAddress = nullptr;
    }

    gUserSessionManager = nullptr;
    //Any internal replicated data will be taken care of by the replicator
}

uint16_t UserSessionManager::getDbSchemaVersion() const
{
    return 26; // Matches largest 00X_usersessions.sql version
}

BlazeRpcError UserSessionManager::parseAuthorizationConfig(const UserSessionsConfig& userConfig, Authorization::GroupManager*& groupManager)
{
    groupManager = BLAZE_NEW_MGID(UserSessionManager::COMPONENT_MEMORY_GROUP, "GroupManager") Authorization::GroupManager();

    const AuthorizationConfig& config = userConfig.getAuthorization();

    const AuthorizationConfig::IpListMap& ipLists = config.getIpLists();
    BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() " << ipLists.size() << " IP lists defined in config.");
    if (!ipLists.empty())
    {
        //read each white list sequence
        AuthorizationConfig::IpListMap::const_iterator it = ipLists.begin();
        AuthorizationConfig::IpListMap::const_iterator end = ipLists.end();
        for (; it != end; ++it)
        {
            const char8_t* ipListName = it->first.c_str();
            BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() ip list(" << ipListName << ") added to groupmanager");
            groupManager->addIpWhiteList(ipListName, *(it->second));
        }
    }

    const AuthorizationConfig::AccessGroupMap& accessGroups = config.getAccessGroups();
    BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() " << accessGroups.size() << " Groups defined in config.");

    AuthorizationConfig::AccessGroupMap::const_iterator mapIt = accessGroups.begin();
    AuthorizationConfig::AccessGroupMap::const_iterator mapEnd = accessGroups.end();
    for (; mapIt != mapEnd; ++mapIt)
    {
        const char8_t *groupName = mapIt->first.c_str();
        BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() Adding Authorization Group(" << groupName << ")");
        Authorization::Group* group = groupManager->addGroup(groupName);

        const AuthorizationConfig::AccessGroupEntry* accessGroupEntry = mapIt->second;
        if (accessGroupEntry == nullptr)
        {
            BLAZE_WARN_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() No permission details defined for AccessGroup(" << groupName << ").");
            return ERR_SYSTEM;
        }

        group->setRequireTrustedClient(accessGroupEntry->getRequireTrustedClient());

        const AuthorizationConfig::AccessStringList& permissionList = accessGroupEntry->getPermissions();
        if (!permissionList.empty())
        {
            BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() " << permissionList.size() << " Permission entries for Group(" << groupName << ")");
            AuthorizationConfig::AccessStringList::const_iterator listIt = permissionList.begin();
            AuthorizationConfig::AccessStringList::const_iterator listEnd = permissionList.end();
            for (; listIt != listEnd; ++listIt)
            {
                const char8_t* permissionName = (*listIt).c_str();
                Authorization::Permission perm = ComponentBaseInfo::getPermissionIdByName(permissionName);
                if (perm != Authorization::PERMISSION_NONE)
                {
                    BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() Permission(" << permissionName << ") added for Group(" << groupName << ")");
                    group->grantPermission(perm, permissionName);
                }
                else
                {
                    BLAZE_INFO_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() Permission(" << permissionName << ") does not exist for any loaded components.");
                }
            }
        }

        const AuthorizationConfig::AccessStringList& ipWhiteList = accessGroupEntry->getIpWhiteList();
        if (!ipWhiteList.empty())
        {
            BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() " << ipWhiteList.size() << " ip white list entries defined for the Group(" << groupName << ")");
            AuthorizationConfig::AccessStringList::const_iterator listIt = ipWhiteList.begin();
            AuthorizationConfig::AccessStringList::const_iterator listEnd = ipWhiteList.end();
            for (; listIt != listEnd; ++listIt)
            {
                const char8_t* ipWhiteListName = (*listIt).c_str();
                BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() ip white list(" << ipWhiteListName << ") added to the Group(" << groupName << ")");
                group->grantIpPermission(ipWhiteListName);
            }
        }
    }

    //read the client type default for each client type
    const AuthorizationConfig::ClientTypeMap& clientTypeDefaults = config.getClientTypeDefaults();
    AuthorizationConfig::ClientTypeMap::const_iterator it = clientTypeDefaults.begin();
    AuthorizationConfig::ClientTypeMap::const_iterator itEnd = clientTypeDefaults.end();
    for (; it != itEnd; ++it)
    {
        ClientType clientType;
        if (ParseClientType(it->first.c_str(), clientType))
        {
            BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.parseAuthorizationConfig() ClientTypeDefaults mapped (" << it->first.c_str() << ":" << it->second.c_str() << ")");
            groupManager->setDefaultGroup(clientType, it->second.c_str());
        }
    }

    return ERR_OK;
}

void UserSessionManager::getRemoteUserExtendedDataProviderRegistration(InstanceId instanceId)
{
    // we're not going to pause the activate while these are in-flight, nor will we fail if the requests do
    gSelector->scheduleFiberCall(this, &UserSessionManager::getRemoteUserExtendedDataProviderRegistrationFiber, instanceId, "UserSessionManager::getRemoteUserExtendedDataProviderRegistrationFiber");
}

void UserSessionManager::getRemoteUserExtendedDataProviderRegistrationFiber(InstanceId instanceId)
{
    BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.getRemoteUserExtendedDataProviderRegistration() requestUserExtendedDataProviderRegistration attempt against Blaze instance ID (" << instanceId << ")");

    RpcCallOptions opts;
    opts.routeTo.setInstanceId(instanceId);

    UpdateUserExtendedDataProviderRegistrationResponse response;
    BlazeRpcError error = this->requestUserExtendedDataProviderRegistration(response, opts);

    if (error != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionManager.getRemoteUserExtendedDataProviderRegistration() requestUserExtendedDataProviderRegistration attempt against Blaze instance ID ("
            << instanceId << ") failed with error " << ErrorHelp::getErrorName(error) << ".");
        return;
    }

    ComponentIdList::const_iterator componentIter = response.getComponentIds().begin();
    ComponentIdList::const_iterator componentEnd = response.getComponentIds().end();
    // incomplete, but this is the proper handling of the response's data providers
    for (; componentIter != componentEnd; ++componentIter)
    {
        ComponentId key = *componentIter;
        if (mDataProviders.find((ComponentId)key) == mDataProviders.end())
        {
            // it's not local, so it must be remote
            mRemoteUEDProviders.insert((ComponentId)key);
        }
    }

    error = updateUserSessionExtendedIds(response.getDataIdMap());
    if (error != ERR_OK)
    {
        // log the error, but there's nothing we can do at this point
        BLAZE_ERR_LOG(Log::USER, "UserSessionManager.getRemoteUserExtendedDataProviderRegistration() updateUserSessionExtendedIds failed with error " 
            << ErrorHelp::getErrorName(error) << ".");
    }
}

void UserSessionManager::refreshRemoteUserExtendedDataProviderRegistrations()
{
    // iterate over all remote providers, remove components that no longer have active instances
    RemoteUserExtendedDataProviderSet::iterator iter = mRemoteUEDProviders.begin();
    RemoteUserExtendedDataProviderSet::iterator end = mRemoteUEDProviders.end();
    while (iter != end)
    {
        // need to validate that there aren't any remaining instances of this component first
        ComponentId componentId = *iter;
        Component *component = gController->getComponent(componentId, false /*reqLocal*/, true /*reqActive*/);
        if (component == nullptr)
        {
            BLAZE_TRACE_LOG(Log::USER, "UserSessionManager.refreshRemoteUserExtendedDataProviderRegistrations() no more active instances of component ("
                << componentId << "/" << BlazeRpcComponentDb::getComponentNameById(componentId) << ") removing from RemoteUserExtendedDataProviderSet.");
            iter = mRemoteUEDProviders.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

bool UserSessionManager::onActivate()
{
    // since we don't replicate the extended data providers or keys, all UserSessionManagers need to watch remote instances

    //First we need to scan any remote instances we already know about and pretend like they're new
    gController->syncRemoteInstanceListener(*this);

    // now register a listener to pick up other remote instances as they come up
    gController->addRemoteInstanceListener(*this);

    gIdentityManager->registerProvider(COMPONENT_ID, *this);
    gUserSetManager->registerProvider(COMPONENT_ID, *this);

    gCensusDataManager->registerProvider(COMPONENT_ID, *this);

    // Because the UserSessionsMaster component has no notifications, the automatic call to notificationSubscribe() is *not* made.
    // So, we need to call this manually here.  This is so that the UserSessionsMasterImpl::onSlaveSessionRemoved() callback is triggered.
    getMaster()->notificationSubscribe();

    // Kick off the river log reader thread.
    mRiverPinUtil.start();

    return true;
}

void UserSessionManager::validateAuthorizationConfig(AuthorizationConfig& config, ConfigureValidationErrors& validationErrors) const
{
    Authorization::GroupManager groupManager;

    const AuthorizationConfig::AccessGroupMap& accessGroups = config.getAccessGroups();
    if (accessGroups.empty())
    {
        eastl::string msg;
        msg.sprintf("[UserSessionManager].validateAuthorizationConfig(): No AccessGroups defined in Authorization.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig(): " << accessGroups.size() << " groups defined in config.");

        AuthorizationConfig::AccessGroupMap::const_iterator mapIt = accessGroups.begin();
        AuthorizationConfig::AccessGroupMap::const_iterator mapEnd = accessGroups.end();
        for (; mapIt != mapEnd; ++mapIt)
        {
            const char8_t *groupName = mapIt->first.c_str();
            const AuthorizationConfig::AccessGroupEntry* accessGroupEntry = mapIt->second;
            if (accessGroupEntry == nullptr)
            {
                BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig() No permission details defined for AccessGroup(" << groupName << ").");
                continue;
            }

            BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig() Adding Authorization Group(" << groupName << ")");
            Authorization::Group* group = groupManager.addGroup(groupName);

            group->setRequireTrustedClient(accessGroupEntry->getRequireTrustedClient());

            const AuthorizationConfig::AccessStringList& permissionList = accessGroupEntry->getPermissions();
            if (!permissionList.empty())
            {
                BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig() " << permissionList.size() << " Permission entries for Group(" << groupName << ")");
                AuthorizationConfig::AccessStringList::const_iterator listIt = permissionList.begin();
                AuthorizationConfig::AccessStringList::const_iterator listEnd = permissionList.end();
                for (; listIt != listEnd; ++listIt)
                {
                    const char8_t* permissionName = (*listIt).c_str();
                    Authorization::Permission perm = ComponentBaseInfo::getPermissionIdByName(permissionName);
                    if (perm != Authorization::PERMISSION_NONE)
                    {
                        BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager]validateAuthorizationConfig() Permission(" << permissionName << ") added for Group(" << groupName << ")");
                        group->grantPermission(perm, permissionName);
                    }
                    else
                    {
                        BLAZE_INFO_LOG(Log::USER, "[UserSessionManager]validateAuthorizationConfig() Permission(" << permissionName << ") does not exist for loaded components. Not added for Group(" << groupName << ")");
                    }
                }
            }

            const AuthorizationConfig::IpListMap& ipLists = config.getIpLists();
            const AuthorizationConfig::AccessStringList& ipWhiteList = accessGroupEntry->getIpWhiteList();
            if (!ipWhiteList.empty())
            {
                BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig() " << ipWhiteList.size() << " ip white list entries defined for the Group(" << groupName << ")");
                if (ipWhiteList.size() > 1)
                {
                    eastl::string msg;
                    msg.sprintf("[UserSessionManager].validateAuthorizationConfig(): More then 1 named whitelist in group(%s). Only the last whitelist entry is honoured", groupName);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                
                AuthorizationConfig::AccessStringList::const_iterator listIt = ipWhiteList.begin();
                AuthorizationConfig::AccessStringList::const_iterator listEnd = ipWhiteList.end();
                for (; listIt != listEnd; ++listIt)
                {
                    const char8_t* ipWhiteListName = (*listIt).c_str();
                    if (ipLists.find(ipWhiteListName) == ipLists.end())
                    {
                        eastl::string msg;
                        msg.sprintf("[UserSessionManager].validateAuthorizationConfig(): No IP whitelist with the name(%s) is defined in [Authorization.IpWhileList]", ipWhiteListName);
                        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                    else
                    {
                        BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig() ip white list(" << ipWhiteListName << ") added to the Group(" << groupName << ")");
                        group->grantIpPermission(ipWhiteListName);
                    }
                }
            }
        }
    }

    const AuthorizationConfig::ClientTypeMap& clientTypeDefaults = config.getClientTypeDefaults();
    if (clientTypeDefaults.empty())
    {
        eastl::string msg;
        msg.sprintf("[UserSessionManager].validateAuthorizationConfig(): ClientTypeDefaults must be defined in the config");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        AuthorizationConfig::ClientTypeMap::const_iterator it = clientTypeDefaults.begin();
        AuthorizationConfig::ClientTypeMap::const_iterator itEnd = clientTypeDefaults.end();
        for (; it != itEnd; ++it)
        {
            ClientType clientType;
            if (ParseClientType(it->first.c_str(), clientType))
            {
                BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].validateAuthorizationConfig() ClientTypeDefaults mapped (" << it->first.c_str() << ":" << it->second.c_str() << ")");
                groupManager.setDefaultGroup(clientType, it->second.c_str());
            }
        }
    }
}

void UserSessionManager::validatePINSettings(PINConfig& config, ConfigureValidationErrors& validationErrors) const
{
    for (const auto& platform : gController->getHostedPlatforms())
    {
        TitlePINInfoMap::const_iterator itr = config.getTitlePINInfo().find(platform);
        if (itr == config.getTitlePINInfo().end())
        {
            eastl::string msg;
            msg.sprintf("[UserSessionManager].validatePINSettings(): TitlePINInfo map is missing an entry for platform '%s'", ClientPlatformTypeToString(platform));
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            if (itr->second->getTitleIdAsTdfString().empty())
            {
                eastl::string msg;
                msg.sprintf("[UserSessionManager].validatePINSettings(): Title ID is not defined for platform '%s'", ClientPlatformTypeToString(platform));
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }

            if (itr->second->getTitleIdTypeAsTdfString().empty())
            {
                eastl::string msg;
                msg.sprintf("[UserSessionManager].validatePINSettings(): Title ID type is not defined for platform '%s'", ClientPlatformTypeToString(platform));
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }

    if (!config.getProductNamesPINInfo().empty())
    {
        for (const auto& pinInfo : config.getProductNamesPINInfo())
        {
            const char8_t* productName = pinInfo.first;
            if (pinInfo.second->getReleaseType()[0] == '\0')
            {
                eastl::string msg;
                msg.sprintf("[UserSessionManager].validatePINSettings(): Release type must be defined for product '%s' in usersessions.cfg", productName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }

    for (const auto& serviceNamePair : gController->getHostedServicesInfo())
    {
        const char8_t* productName = serviceNamePair.second->getProductName();
        if (productName[0] != '\0')
        {
            const auto& productNamePINInfoPair = getConfig().getPINSettings().getProductNamesPINInfo().find(productName);
            if (productNamePINInfoPair == getConfig().getPINSettings().getProductNamesPINInfo().end())
            {
                eastl::string msg;
                msg.sprintf("[UserSessionManager].validatePINSettings(): productNamesPINInfo must be defined for product '%s' in usersessions.cfg", productName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }
}

void UserSessionManager::validatePlatformRestrictions(UserSessionsConfig& config, ConfigureValidationErrors& validationErrors) const
{
    // Validate that the platform restrictions are correctly defined:
    const ClientPlatformSet& supportedPlatforms = gController->getHostedPlatforms();
    auto unrestrictedPlatforms = supportedPlatforms;
    
    eastl::map<ClientPlatformType, ClientPlatformSet> platformRestrictions;

    for (auto& platLists : config.getPlatformRestrictions())
    {
        // ignore platform restriction validation for the platforms that are not even hosted. 
        if (!gController->isPlatformHosted(platLists.first))
            continue;

        unrestrictedPlatforms.erase(platLists.first);

        // Always support self lookup:
        platformRestrictions[platLists.first].insert(platLists.first);

        for (auto& curPlatform : *platLists.second)
        {
            if (supportedPlatforms.find(curPlatform) == supportedPlatforms.end())
            {
                eastl::string msg;
                msg.sprintf("[Controller].onValidateConfig: Unsupported platform listed in Platform Restrictions list: %s.", ClientPlatformTypeToString(curPlatform));
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }

            platformRestrictions[platLists.first].insert(curPlatform);
        }
    }

    // Fill in any missing platforms with full support: 
    for (auto& curPlatform : unrestrictedPlatforms)
    {
        platformRestrictions[curPlatform].insert(unrestrictedPlatforms.begin(), unrestrictedPlatforms.end());
    }

    // Validate that all entries are bidirectional:
    for (auto& platSets : platformRestrictions)
    {
        for (auto& curPlatform : platSets.second)
        {
            auto platIter = platformRestrictions.find(curPlatform);
            if (platIter == platformRestrictions.end() || platIter->second.find(platSets.first) == platIter->second.end())
            {
                eastl::string msg;
                msg.sprintf("[Controller].onValidateConfig: Missing bidirectional entry for platforms: %s <- %s.", ClientPlatformTypeToString(platSets.first), ClientPlatformTypeToString(curPlatform));
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }

    // validate that no platform is in more than one non-crossplay platform set, and that all platforms in a non-crossplay set are allowed to crossplay with each other
    eastl::map<ClientPlatformType, NonCrossplayPlatformSetName> nonCrossplayPlatformSetMappings;
    for (auto& platSetIter : config.getNonCrossplayPlatformSets())
    {

        auto& platList = *platSetIter.second;
        if (platList.empty())
        {
            eastl::string msg;
            msg.sprintf("[Controller].onValidateConfig: Non-crossplay platform set: '%s' was empty.", platSetIter.first.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        for (auto& curPlatform : platList)
        {
            if (nonCrossplayPlatformSetMappings.find(curPlatform) != nonCrossplayPlatformSetMappings.end())
            {
                if (blaze_strcmp(platSetIter.first, nonCrossplayPlatformSetMappings.find(curPlatform)->second) != 0)
                { 
                    // platform is in 2 non-crossplay platform sets.
                    eastl::string msg;
                    msg.sprintf("[Controller].onValidateConfig: Platforms: %s is listed in more than one non-crossplay platform set: '%s' and '%s'.", ClientPlatformTypeToString(curPlatform), nonCrossplayPlatformSetMappings.find(curPlatform)->second.c_str(), platSetIter.first.c_str());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                else
                {
                    // platform is listed twice in the same set
                    eastl::string msg;
                    msg.sprintf("[Controller].onValidateConfig: Platforms: %s is duplicated in non-crossplay platform set: '%s'.", ClientPlatformTypeToString(curPlatform), platSetIter.first.c_str());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                
            }
            else
            {
                nonCrossplayPlatformSetMappings.insert(eastl::make_pair(curPlatform, platSetIter.first));
                // verify that the platforms in the list are allowed to engage in crossplay via the platform restrictions
                auto platRestrictionsIter = platformRestrictions.find(curPlatform);
                if (platRestrictionsIter != platformRestrictions.end())
                {
                    for (auto& noncrossplayPlatformIter : platList)
                    {

                        // make sure every member of this noncrossplay platform list is in the current platform's unrestricted list
                        if (platRestrictionsIter->second.find(noncrossplayPlatformIter) == platRestrictionsIter->second.end())
                        {
                            eastl::string msg;
                            msg.sprintf("[Controller].onValidateConfig: Platforms: %s and %s in nonCrossplayPlatformSet '%s' are restricted from crossplay.",
                                ClientPlatformTypeToString(curPlatform), ClientPlatformTypeToString(noncrossplayPlatformIter), platSetIter.first.c_str());
                            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                            str.set(msg.c_str());
                        }
                    }
                }
                else // if (platRestrictionsIter == platformRestrictions.end())
                {
                    eastl::string msg;
                    msg.sprintf("[Controller].onValidateConfig: Missing platform restrictions for platform: %s in nonCrossplayPlatformSet '%s.'", ClientPlatformTypeToString(curPlatform), platSetIter.first.c_str());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }
}

BlazeRpcError UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(const ClientPlatformSet*& platformSet, eastl::string& targetNamespace, bool& crossPlatformOptIn, bool matchPrimaryNamespace /*= false*/)
{
    platformSet = nullptr;
    crossPlatformOptIn = false;

    if (gCurrentUserSession == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionManager.setPlatformsAndNamespaceForCurrentUserSession: gCurrentUserSession is nullptr!");
        return ERR_SYSTEM;
    }

    // If the current context is authorized to make cross-platform user info lookups (usually things like
    // tools and game servers), then just go ahead and give the caller the hosted platforms.
    if (UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_USER_LOOKUP_CROSSPLATFORM, true))
    {
        platformSet = &gController->getHostedPlatforms();
        return ERR_OK;
    }

    // If the authorization check failed, then there's a number of conditions that have to be met.
    // The first one is that any client that hasn't specified a normal platform (e.g. xone, xbsx, ps5, ps4, stadia, etc.)
    // simply isn't allowed to do user info lookups on *any* platform.
    if ((gCurrentUserSession->getClientPlatform() == common) || (gCurrentUserSession->getClientPlatform() == INVALID))
    {
        return ERR_AUTHORIZATION_REQUIRED;
    }

    // Cross-platform opt-in is required when the caller's platform isn't 'common' and doesn't use the Origin namespace,
    // but the caller is explicitly searching for users whose primary namespace is Origin
    bool requireCrossPlatformOptIn = false;

    if (targetNamespace.empty())
        targetNamespace.assign(gCurrentUserSession->getPersonaNamespace());
    else if (blaze_strcmp(targetNamespace.c_str(), NAMESPACE_ORIGIN) == 0)
        requireCrossPlatformOptIn = matchPrimaryNamespace && blaze_strcmp(gCurrentUserSession->getPersonaNamespace(), NAMESPACE_ORIGIN) != 0;
    else if (blaze_strcmp(targetNamespace.c_str(), gCurrentUserSession->getPersonaNamespace()) != 0)
        return USER_ERR_DISALLOWED_PLATFORM;

    // On single-platform deployments, we treat all users as if they're opted out of cross-platform
    // (and are therefore only able to look up users on their own platform), but we never return USER_ERR_CROSS_PLATFORM_OPTOUT.
    // This means that RPCs which look up users by persona name but don't support multi-platform lookups in the Origin namespace
    // (e.g. lookupUsersByPersonaNames) will always return USER_ERR_USER_NOT_FOUND when the caller requests the Origin namespace
    // on single-platform xone/ps4 deployments.
    if (gController->isSharedCluster())
    {
        UserSessionDataPtr userSessionData = nullptr;
        BlazeRpcError optErr = gCurrentUserSession->fetchSessionData(userSessionData);
        if (userSessionData == nullptr)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].setPlatformsAndNamespaceForCurrentUserSession: Error(" << ErrorHelp::getErrorName(optErr) << ") looking up cross platform opt for user(" << gCurrentUserSession->getId() << ")");
            return ERR_SYSTEM;
        }
        crossPlatformOptIn = userSessionData->getExtendedData().getCrossPlatformOptIn();
    }

    if (crossPlatformOptIn)
        platformSet = &gUserSessionManager->getUnrestrictedPlatforms(gCurrentUserSession->getClientPlatform());
    else if (requireCrossPlatformOptIn && gController->isSharedCluster())
        return USER_ERR_CROSS_PLATFORM_OPTOUT;
    else
        platformSet = &gUserSessionManager->getNonCrossplayPlatformSet(gCurrentUserSession->getClientPlatform());

    return ERR_OK;
}

const ClientPlatformSet& UserSessionManager::getUnrestrictedPlatforms(ClientPlatformType platform) const
{
    auto iter = mRestrictedPlatformSets.find(platform);
    if (iter != mRestrictedPlatformSets.end())
    {
        return (iter->second);
    }

    // didn't have an unrestricted platform, so return a single-platform set
    return getNonCrossplayPlatformSet(platform);
}

const ClientPlatformSet& UserSessionManager::getNonCrossplayPlatformSet(ClientPlatformType platform) const
{
    static ClientPlatformSet commonPlatformSet;

    static ClientPlatformSet emptyPlatformSet;

    // this handles any platforms not in the non-crossplay platform set. 
    // 'common' platform requires special handling, any non-hosted platform an empty set
    switch (platform)
    {
    case common:
        commonPlatformSet.clear();
        for (auto serverPlatformsIter : gController->getHostedPlatforms())
        {
            commonPlatformSet.insert(serverPlatformsIter);
        }
        return commonPlatformSet;
    default:
        auto iter = mNonCrossplayPlatformSets.find(platform);
        if (iter != mNonCrossplayPlatformSets.end())
        {
            return (iter->second);
        }
        // wasn't common and wasn't in the non crossplay platform sets - return the empty set
        return emptyPlatformSet;
    }
}

bool UserSessionManager::onValidateConfig(UserSessionsConfig& config, const UserSessionsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (!config.getUseDatabase())
    {
        BLAZE_FATAL_LOG(Log::USER, "UserSessionManager.onValidateConfig: The UserSessions component must be configured to use a database.  See the useDatabase setting in usersessions.cfg");
        return false;
    }

    if (gController->isSharedCluster() && !config.getAuditPersonas().empty())
    {
        BLAZE_FATAL_LOG(Log::USER, "UserSessionManager.onValidateConfig: Audit Persona list is not empty! The UserSessions component can not specify audit personas when hosting a shared cluster.");
        return false;
    }

    validatePlatformRestrictions(config, validationErrors);

    // validate qos settings
    mQosConfig.validateQosSettingsConfig(config.getQosSettings(), validationErrors);

    // validate authorization settings
    validateAuthorizationConfig(config.getAuthorization(), validationErrors);

    validatePINSettings(config.getPINSettings(), validationErrors);

    if (config.getNegativePersonaNameCacheSize() <= 0)
    {
        eastl::string msg;
        msg.sprintf("[UserSessionManager].onValidateConfig(): Negative lru cache sizes must be positive");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    return validationErrors.getErrorMessages().empty();
}

const uint16_t USERSESSION_IDTYPE_NEGATIVE_ID = 1;

bool UserSessionManager::onConfigure()
{
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].onConfigure: configuring component.");

    const UserSessionsConfig& config = getConfig();
    AccountInfoDbCalls::initAccountInfoDbCalls();

    for (DbIdByPlatformTypeMap::const_iterator itr = getDbIdByPlatformTypeMap().begin(); itr != getDbIdByPlatformTypeMap().end(); ++itr)
    {
        if (itr->first != ClientPlatformType::common)
        {
            uint32_t dbId = itr->second;
            UserInfoDbCalls::UserInfoPreparedStatements& statements = mUserInfoPreparedStatementsByDbId[dbId];
            UserInfoDbCalls::initUserInfoDbCalls(dbId, statements);
        }
    }

    // parse settings for the DebugSystem
    gController->getDebugSys()->parseDebugConfig();

    // Because both usersessionmanager and uniqueidmanager are framework components the former 
    // needs to explicitly wait for the latter, to ensure that we can call reserveIdType.
    BlazeRpcError rc = gUniqueIdManager->waitForState(ACTIVE, "UserSessionManager::onConfigure");
    if (rc != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onConfigure: Failed waiting for UniqueIdManager to become ACTIVE, err: "
            << ErrorHelp::getErrorName(rc));
        return false;
    }

    if (gUniqueIdManager->reserveIdType(UserSessionsSlave::COMPONENT_ID, USERSESSION_IDTYPE_NEGATIVE_ID) != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].onConfigure: Failed reserving component id " << UserSessionsSlave::COMPONENT_ID << " with unique id manager");
        return false;
    }

    return configureCommon(config);
}


BlazeId UserSessionManager::allocateNegativeId()
{
    uint64_t counter = 0;
    BlazeRpcError error = gUniqueIdManager->getId(RpcMakeSlaveId(UserSessionsSlave::COMPONENT_ID), USERSESSION_IDTYPE_NEGATIVE_ID, counter);
    if (error != ERR_OK)
    {
        return INVALID_BLAZE_ID;
    }

    int64_t value = (int64_t)counter;
    //make sure first 13 bits are zero, even though they are already 0 actually.
    value = value & 0x0007FFFFFFFFFFFF;
    //set slaveId value  (We don't have to include this, but we keep it around for consistency with the old version. )
    value = value | static_cast<int64_t>(gController->getInstanceId()) << 51;
    //set first bit 1
    value = value | 0x8000000000000000;

    return value;
};

bool UserSessionManager::onReconfigure()
{
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].onReconfigure: reconfiguring component.");
    // reparse the settings for the DebugSystem
    gController->getDebugSys()->parseDebugConfig();

    const UserSessionsConfig& config = getConfig();

    if (configureCommon(config, true))
    {
        BLAZE_INFO_LOG(Log::USER, "[UserSessionManager].onReconfigure: Done.");
    }
    else
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].onReconfigure: Failed. Config not changed.");
    }
    return true;
}

bool UserSessionManager::onPrepareForReconfigure(const UserSessionsConfig& config)
{
    BlazeRpcError rc = parseAuthorizationConfig(config, mTempGroupManager);
    if (rc != ERR_OK)
    {
        //bail if parsing fails
        delete mTempGroupManager;
        mTempGroupManager = nullptr;

        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].onPrepareForReconfigure: Failed to parse authorization config with error " << ErrorHelp::getErrorName(rc));
        return false;
    }

    return true;
}

void UserSessionManager::onShutdown()
{
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].onShutdown: Shutting down user session manager");

    mRiverPinUtil.stop();
    
    mUpdateBlazeObjectIdJobQueue.join();

    while (!mWaitListByUserSessionIdMap.empty())
    {
        WaitListByUserSessionIdMap::iterator it = mWaitListByUserSessionIdMap.begin();
        it->second.signal(Blaze::ERR_CANCELED);
        mWaitListByUserSessionIdMap.erase(it);
    }

    // Free any remaining UserSession instances
    mUserSessionPtrByIdMap.clear();

    mUserInfoLruCache.reset();

    if (gController->getInstanceType() == ServerConfig::SLAVE)
    {
        // only core slaves needed to listen when aux slaves had been connected to
        gController->removeRemoteInstanceListener(*this);
    }

    gController->deregisterDrainStatusCheckHandler(COMPONENT_INFO.name);
    gCensusDataManager->unregisterProvider(COMPONENT_ID);
    gUserSetManager->deregisterProvider(COMPONENT_ID);
    gIdentityManager->deregisterProvider(COMPONENT_ID);
}

bool UserSessionManager::configureCommon(const UserSessionsConfig& config, bool reconfigure)
{
    mRestrictedPlatformSets.clear();
    ClientPlatformSet unrestrictedPlatforms = gController->getHostedPlatforms();
    for (auto& platLists : config.getPlatformRestrictions())
    {
        unrestrictedPlatforms.erase(platLists.first);

        // Always support self lookup:
        mRestrictedPlatformSets[platLists.first].insert(platLists.first);
        for (auto& curPlatform : *platLists.second)
            mRestrictedPlatformSets[platLists.first].insert(curPlatform);
    }

    // Fill in any missing platforms with full support: 
    for (auto& curPlatform : unrestrictedPlatforms)
        mRestrictedPlatformSets[curPlatform].insert(unrestrictedPlatforms.begin(), unrestrictedPlatforms.end());

    mNonCrossplayPlatformSets.clear();
    for (auto& platLists : config.getNonCrossplayPlatformSets())
    {
        // we've already validated that platforms aren't in more than one set

        for (auto& curPlatform : *platLists.second)
        {    

            // skip over non-hosted platforms
            if (!gController->isPlatformHosted(curPlatform))
                continue;

            // for each non crossplay platform set, we insert the full list for each included platform
            for (auto& curMemberPlatform : *platLists.second)
            {
                // skip over non-hosted platforms
                if (!gController->isPlatformHosted(curMemberPlatform))
                    continue;

                mNonCrossplayPlatformSets[curPlatform].insert(curMemberPlatform);
            }
        }
    }

    // now ensure that all hosted platforms have a non-crossplay platform set defined that includes itself
    ClientPlatformSet hostedPlatforms = gController->getHostedPlatforms();
    for (auto& curPlatform : hostedPlatforms)
        mNonCrossplayPlatformSets[curPlatform].insert(curPlatform);

    mClientPlatformsByProjectIdMap.clear();
    const TitlePINInfoMap& titlePINInfoMap = config.getPINSettings().getTitlePINInfo();
    for (const ClientPlatformType curPlatform : gController->getHostedPlatforms())
    {
        TitlePINInfoMap::const_iterator itr = titlePINInfoMap.find(curPlatform);
        if (itr != titlePINInfoMap.end())
        {
            const TitlePINInfo* titlePINInfo = itr->second;
            if (blaze_stricmp(titlePINInfo->getTitleIdType(), "projectid") == 0)
            {
                const char8_t* projectId = titlePINInfo->getTitleId();

                ClientPlatformTypeList* projectPlatforms = nullptr;
                auto findIt = mClientPlatformsByProjectIdMap.find(projectId);
                if (findIt != mClientPlatformsByProjectIdMap.end())
                {
                    projectPlatforms = findIt->second;
                }
                else
                {
                    projectPlatforms = mClientPlatformsByProjectIdMap.allocate_element();
                    mClientPlatformsByProjectIdMap.insert(ClientPlatformsByProjectIdMap::value_type(projectId, projectPlatforms));
                }
                projectPlatforms->push_back(curPlatform);
            }
        }
    }

    const char8_t* nameHandler = config.getNameHandler();
    if (*nameHandler == '\0')
        nameHandler = NameHandler::getDefaultNameHandlerType();
    if (mNameHandler != nullptr)
    {
        delete mNameHandler;
        mNameHandler = nullptr;
    }
    mNameHandler = NameHandler::createNameHandler(nameHandler);
    if (mNameHandler == nullptr)
    {
        char8_t validTypes[256];
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].configureCommon: invalid name handler (" << nameHandler << ") given for "
            << UserSessionsConfig::NAME_HANDLER_NAME << "; valid values are:" << NameHandler::getValidTypes(validTypes, sizeof(validTypes)));
        return false;
    }

    if (gController->isInstanceTypeConfigMaster())
    {
        if (reconfigure)
        {
            gSelector->scheduleFiberCall<UserSessionManager, const UserSessionsConfig&>(
                this, &UserSessionManager::parseAuditPersonasConfig, config, "UserSessionManager::configureCommon");
        }
        else
        {
            parseAuditPersonasConfig(config);
        }
    }

    if (!reconfigure)
    {
        BlazeRpcError rc = parseAuthorizationConfig(config, mTempGroupManager);
        if (rc != ERR_OK)
        {
            //bail if parsing fails 
            delete mTempGroupManager;
            mTempGroupManager = nullptr;

            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].configureCommon: Failed to parse authorization config with error " << ErrorHelp::getErrorName(rc));
            return false;
        }
    }

    mQosConfig.configure(config.getQosSettings());

    Authorization::GroupManager* oldGroupManger = mGroupManager;
    mGroupManager = mTempGroupManager;
    mTempGroupManager = nullptr;
    delete oldGroupManger;

    // Client UED to General UED mapping: 
    mUEDClientConfigMap.clear();

    UserExtendedDataIdMap uedMap;
    for (UserSessionClientUEDConfigList::const_iterator it = getConfig().getUserSessionClientUEDConfigList().begin(), end = getConfig().getUserSessionClientUEDConfigList().end(); it != end; ++it)
    {
        const UserSessionClientUEDConfig* configTemp = *it;

        // Sanity Check the rules: (others in UserSessionsMasterImpl::onConfigure)
        if (!configTemp->getName() || configTemp->getName()[0] == '\0')
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].configureCommon: Invalid UserSessionClientUEDConfig name given. UED id(" << configTemp->getId() << ")");
            return false;  
        }

        if (configTemp->getMin() > configTemp->getMax() ||
            configTemp->getMin() > configTemp->getDefault() ||
            configTemp->getDefault() > configTemp->getMax())
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].configureCommon: Invalid UserSessionClientUEDConfig Min, Max, or Default given. UED id(" 
                            << configTemp->getId() << ") Name: " << configTemp->getName() << " Min: "<< configTemp->getMin() << " Max: "<< configTemp->getMax() << " Default: " << configTemp->getDefault());
            return false;  
        }

        if (uedMap.find(configTemp->getName()) != uedMap.end())
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].configureCommon: Multiple UserSessionClientUEDConfig entries for Name: " << configTemp->getName());
            return false;  
        }

        if (EA::StdC::Strstr(configTemp->getName(), "_"))
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].configureCommon: Invalid UserSessionClientUEDConfig name given (underscores are not allowed). Name: " << configTemp->getName());
            return false;  
        }

        uedMap.insert(UserExtendedDataIdMap::value_type(configTemp->getName(), UED_KEY_FROM_IDS(INVALID_COMPONENT_ID, configTemp->getId())));

        // Update the id -> config map:
        if (mUEDClientConfigMap.find(configTemp->getId()) != mUEDClientConfigMap.end())
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].onConfigure: Multiple UserSessionClientUEDConfig entries for Id: " << configTemp->getId());
            return false;  
        }

        mUEDClientConfigMap[configTemp->getId()] = configTemp;
    }

    // now add the reputation UED key, and parse the reputation expression
    uedMap.insert(UserExtendedDataIdMap::value_type(config.getReputation().getEvaluatedReputationUEDName(), UED_KEY_FROM_IDS(UserSessionManager::COMPONENT_ID, ReputationService::EXTENDED_DATA_REPUTATION_VALUE_KEY)));
    if (!mReputationServiceFactory.createReputationExpression())
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionManager.configureCommon: failed to parse reputation expression.");
        return false;
    }


    if (!mReputationServiceFactory.createReputationServiceUtil())
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionManager.configureCommon: failed to create the reputation service util.");
        return false;
    }

    // populate the UED names
    BlazeRpcError error = updateUserSessionExtendedIds(uedMap);
    if (error != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionManager.configureCommon: updateUserSessionExtendedIds failed with error " 
            << ErrorHelp::getErrorName(error) << ".");
    }

    mUserInfoLruCache.setMaxSize(config.getUserCacheMax());

    mRiverPinUtil.configure(config.getPINSettings());

    return true;
}

void UserSessionManager::parseAuditPersonasConfig(const UserSessionsConfig& config)
{
    const PersonaNameList& auditPersonaList = config.getAuditPersonas();
    if (!auditPersonaList.empty())
    {
        PersonaNameVector personas;
        personas.reserve(auditPersonaList.size());
        for (PersonaNameList::const_iterator i = auditPersonaList.begin(), e = auditPersonaList.end(); i != e; ++i)
        {
            const char8_t* personaName = i->c_str();
            personas.push_back(personaName);
        }

        // Resolve persona names into blaze IDs
        // Note: Audit logging via UserSessionsConfig is deprecated (replaced by the enableAuditLogging RPC) --
        // so to preserve legacy behaviour and avoid adding new options (e.g. platform) to a deprecated config field,
        // this lookup just fetches all users with the given persona name (in the primary namespace for their platform).
        UserInfoListByPersonaNameByNamespaceMap results;
        BlazeRpcError err = gUserSessionManager->lookupUserInfoByPersonaNamesMultiNamespace(personas, results, true /*primaryNamespaceOnly*/, false /*restrictCrossPlatformResults*/);
        if (err != Blaze::ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].parseAuditPersonasConfig: error("
                << Blaze::ErrorHelp::getErrorName(err) << ") looking up audit personas.");
        }
        else
        {
            UpdateAuditLoggingRequest req;
            BlazeIdSet blazeIds;
            for (const auto& namespaceItr : results)
            {
                for (const auto& personaItr : namespaceItr.second)
                {
                    for (const auto& userItr : personaItr.second)
                    {
                        if (blazeIds.insert(userItr->getId()).second)
                        {
                            AuditLogEntryPtr entry = req.getAuditLogEntries().pull_back();
                            entry->setBlazeId(userItr->getId());
                        }
                    }
                }
            }
            gController->enableAuditLogging(req);
        }
    }

    auto numAuditEntries = Logger::getNumAuditEntries();
    if (numAuditEntries > 0)
    {
        if (Logger::isAuditLoggingSuppressed())
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].parseAuditPersonasConfig: Audit logging suppressed, audit entries(" << numAuditEntries << ").");
        }
        else
        {
            BLAZE_INFO_LOG(Log::USER, "[UserSessionManager].parseAuditPersonasConfig: Audit logging allowed, audit entries(" << numAuditEntries << ").");
        }
    }
    else
    {
        if (Logger::isAuditLoggingSuppressed())
        {
            BLAZE_INFO_LOG(Log::USER, "[UserSessionManager].parseAuditPersonasConfig: Audit logging suppressed, no users currently audited.");
        }
        else
        {
            BLAZE_INFO_LOG(Log::USER, "[UserSessionManager].parseAuditPersonasConfig: Audit logging allowed, no users currently audited.");
        }
    }
}

bool UserSessionManager::shouldAuditSession(UserSessionId userSessionId)
{
    if (getState() >= CONFIGURED)
    {
        const UserSessionPtr userSession = gUserSessionManager->getSession(userSessionId);

        if (userSession != nullptr)
        {
            return Logger::shouldAudit(userSession->getBlazeId(), userSession->getUniqueDeviceId(), userSession->getExistenceData().getClientIp(), userSession->getPersonaName(), 
                userSession->getPlatformInfo().getEaIds().getNucleusAccountId(), userSession->getPlatformInfo().getClientPlatform());
        }
    }

    return false;
}


bool UserSessionManager::onResolve()
{
    mUserSessionExistenceListener.registerFieldHandler(mExistenceFieldHandler);
    mUserSessionExistenceListener.registerRecordHandler(mExistenceRecordHandler);
    gStorageManager->registerStorageListener(mUserSessionExistenceListener);

    return true;
}

void UserSessionManager::upsertUserSessionExistence(UserSessionExistenceData& existence)
{
    EA_ASSERT_MSG(!existence.isNotRefCounted(), "UserSessionExistenceData must be ref-countable!");

    UserSessionPtrByIdMap::insert_return_type insertRet = mUserSessionPtrByIdMap.insert(existence.getUserSessionId());
    UserSessionExistencePtr& userSession = insertRet.first->second;

    const bool isNew = insertRet.second;
    if (isNew)
    {
        // This is the first we've heard of this UserSession
        userSession = BLAZE_NEW UserSessionExistence();
    }
    else
    {
        // This UserSession already exists.  We can take special action based on what has changed.
        mDispatcher.dispatch<const UserSession&, const UserSessionExistenceData&>(&UserSessionSubscriber::onUserSessionMutation, *userSession, existence);
        // Remove this UserSession from all of the intrusive lists and indexes, as the indices may have changed.  We re-insert down below.
        userSession->removeFromAllLists();

        if (userSession->isAuxLocal())
            mGaugeAuxLocalUserSessionCount.decrement(1, userSession->getClientType());
    }

    // Now set the existence, from now on the old existence is gone.
    userSession->mExistenceData = &existence;

    if (userSession->isAuxLocal())
        mGaugeAuxLocalUserSessionCount.increment(1, userSession->getClientType());

    mExistenceByBlazeIdMap[userSession->getBlazeId()].push_back(*userSession);
    mExistenceByAccountIdMap[userSession->getAccountId()].push_back(*userSession);
    if (userSession->getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
        mExistenceByPsnIdMap[userSession->getPlatformInfo().getExternalIds().getPsnAccountId()].push_back(*userSession);
    if (userSession->getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
        mExistenceByXblIdMap[userSession->getPlatformInfo().getExternalIds().getXblAccountId()].push_back(*userSession);
    if (userSession->getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        mExistenceBySteamIdMap[userSession->getPlatformInfo().getExternalIds().getSteamAccountId()].push_back(*userSession);
    if (!userSession->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
        mExistenceBySwitchIdMap[userSession->getPlatformInfo().getExternalIds().getSwitchId()].push_back(*userSession);

    if (mClientTypeDescriptions[userSession->getClientType()].getPrimarySession())
        mPrimaryExistenceByBlazeIdMap[userSession->getBlazeId()].push_back(*userSession);

    if (isNew)
    {
        // Update user session metrics
        mGaugeUserSessionCount.increment(1, userSession->getProductName(), userSession->getClientType(), userSession->getClientVersion());
        mUserSessionsCreated.increment(1, userSession->getProductName(), userSession->getClientType(), userSession->getClientVersion());

        mDispatcher.dispatch<const UserSession&>(&UserSessionSubscriber::onUserSessionExistence, *userSession);
    }

    WaitListByUserSessionIdMap::iterator it = mWaitListByUserSessionIdMap.find(existence.getUserSessionId());
    if (it != mWaitListByUserSessionIdMap.end())
    {
        it->second.signal(ERR_OK);
        mWaitListByUserSessionIdMap.erase(it);
    }
}

void UserSessionManager::onUpsertUserSessionExistence(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue)
{
    UserSessionExistenceData& existence = static_cast<UserSessionExistenceData&>(fieldValue);
    upsertUserSessionExistence(existence);
}

void UserSessionManager::onEraseUserSessionExistence(StorageRecordId recordId)
{
    UserSessionId userSessionId = recordId;

    UserSessionPtrByIdMap::iterator it = mUserSessionPtrByIdMap.find(userSessionId);
    if (it == mUserSessionPtrByIdMap.end())
    {
        // The UserSession isn't here, nothing to do.  Note, this is a common occurence when a *local* UserSession has logged out.
        return;
    }

    UserSessionPtr userSession = it->second;

    mUserSessionPtrByIdMap.erase(it);

    userSession->removeFromAllLists(); // always safely remove from all intrusive lists

    ExistenceByBlazeIdMap::iterator bidIt = mExistenceByBlazeIdMap.find(userSession->getBlazeId());
    if (bidIt != mExistenceByBlazeIdMap.end() && bidIt->second.empty())
        mExistenceByBlazeIdMap.erase(bidIt);

    ExistenceByAccountIdMap::iterator aidIt = mExistenceByAccountIdMap.find(userSession->getAccountId());
    if (aidIt != mExistenceByAccountIdMap.end() && aidIt->second.empty())
        mExistenceByAccountIdMap.erase(aidIt);

    if (userSession->getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
    {
        ExistenceByPsnAccountIdMap::iterator eidIt = mExistenceByPsnIdMap.find(userSession->getPlatformInfo().getExternalIds().getPsnAccountId());
        if (eidIt != mExistenceByPsnIdMap.end() && eidIt->second.empty())
            mExistenceByPsnIdMap.erase(eidIt);
    }
    if (userSession->getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
    {
        ExistenceByXblAccountIdMap::iterator eidIt = mExistenceByXblIdMap.find(userSession->getPlatformInfo().getExternalIds().getXblAccountId());
        if (eidIt != mExistenceByXblIdMap.end() && eidIt->second.empty())
            mExistenceByXblIdMap.erase(eidIt);
    }
    if (userSession->getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
    {
        ExistenceBySteamAccountIdMap::iterator eidIt = mExistenceBySteamIdMap.find(userSession->getPlatformInfo().getExternalIds().getSteamAccountId());
        if (eidIt != mExistenceBySteamIdMap.end() && eidIt->second.empty())
            mExistenceBySteamIdMap.erase(eidIt);
    }

    if (!userSession->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
    {
        ExistenceBySwitchIdMap::iterator eidIt = mExistenceBySwitchIdMap.find(userSession->getPlatformInfo().getExternalIds().getSwitchId());
        if (eidIt != mExistenceBySwitchIdMap.end() && eidIt->second.empty())
            mExistenceBySwitchIdMap.erase(eidIt);
    }

    // Update metrics
    mGaugeUserSessionCount.decrement(1, userSession->getProductName(), userSession->getClientType(), userSession->getClientVersion());
    if (userSession->isAuxLocal())
        mGaugeAuxLocalUserSessionCount.decrement(1, userSession->getClientType());
    if (mClientTypeDescriptions[userSession->getClientType()].getPrimarySession())
    {
        PrimaryExistenceByBlazeIdMap::iterator peIt = mPrimaryExistenceByBlazeIdMap.find(userSession->getBlazeId());
        if (peIt != mPrimaryExistenceByBlazeIdMap.end() && peIt->second.empty())
            mPrimaryExistenceByBlazeIdMap.erase(peIt);
    }

    // NOTE: Enable session to be found by the sendPINEvents() which can be invoked by component code triggered from the onUserSessionExtinction() dispatcher.
    // This is a lite-touch approach that avoids possible impact on component code that may depend on existing behavior whereby getSession() returns nullptr within onUserSessionExtinction() dispatcher.
    mExtinctUserSessionPtrByIdMap[userSessionId] = userSession;

    mDispatcher.dispatch<const UserSession&>(&UserSessionSubscriber::onUserSessionExtinction, *userSession);

    mExtinctUserSessionPtrByIdMap.erase(userSessionId);
}

/*! ************************************************************************************************/
/*! \brief entry point for the census data component to poll the GameManager slave for census info.
***************************************************************************************************/
Blaze::BlazeRpcError UserSessionManager::getCensusData(CensusDataByComponent& censusData)
{
    // Query each UserSessionMaster for its census data
    Component::InstanceIdList umInstances;
    getMaster()->getComponentInstances(umInstances, true);
    typedef Component::MultiRequestResponseHelper<UserManagerCensusData, void> Response;
    Component::MultiRequestResponseHelper<UserManagerCensusData, void> responses(umInstances, false);
    BlazeRpcError err = getMaster()->sendMultiRequest(Blaze::UserSessionsMaster::CMD_INFO_GETCENSUSDATA, nullptr, responses);
    if (err != ERR_OK)
        return err;


    UserManagerCensusData *umCensusData = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "UserManagerCensusData") UserManagerCensusData(*Allocator::getAllocator(COMPONENT_MEMORY_GROUP));

    ConnectedCountsMap& connectedCountsMap = umCensusData->getConnectedPlayerCounts();
    for (int i = 0; i < CLIENT_TYPE_INVALID; ++i)
    {
        // using invalid so we get the overall counts, and not platform-specific ones.
        connectedCountsMap[static_cast<ClientType>(i)] = gUserSessionManager->getUserSessionCountByClientType(static_cast<ClientType>(i));
    }

    ConnectedCountsByPingSiteMap& connectedCountsByPingSiteMap = umCensusData->getConnectedPlayerCountByPingSite();
    for (Response::const_iterator it = responses.begin(), end = responses.end(); it != end; ++it)
    {
        const UserManagerCensusData& data = static_cast<const UserManagerCensusData&>(*(*it)->response);
        addCensusResponse(data, connectedCountsByPingSiteMap);
    }

    // Due to a bug in sendMultiRequest, we get an error when we send the request to the local instance.
    // So instead, we skip the local instance above and do a separate request here to the local instance.
    if (gUserSessionMaster != nullptr)
    {
        UserManagerCensusData response;
        gUserSessionMaster->processGetCensusData(response, nullptr);
        addCensusResponse(response, connectedCountsByPingSiteMap);
    }

    if (censusData.find(getComponentId()) == censusData.end())
    {
        censusData[getComponentId()] = censusData.allocate_element();
    }
    censusData[getComponentId()]->set(umCensusData);

    return err; 
}

void UserSessionManager::addCensusResponse(const UserManagerCensusData& data, ConnectedCountsByPingSiteMap& connectedCountsByPingSiteMap)
{
    for (ConnectedCountsByPingSiteMap::const_iterator dataIt = data.getConnectedPlayerCountByPingSite().begin()
         , dataEnd = data.getConnectedPlayerCountByPingSite().end()
         ; dataIt != dataEnd
         ; ++dataIt)
    {
        ConnectedCountsByPingSiteMap::iterator countIt = connectedCountsByPingSiteMap.insert(eastl::make_pair(dataIt->first, 0)).first;
        countIt->second += dataIt->second;
    }
}

void UserSessionManager::createSHA256Sum(char8_t* buf, size_t bufLen, UserSessionId id,
    const Blaze::Int256Buffer& random256Bits, int64_t creationTime)
{
    EA_ASSERT(bufLen >= HashUtil::SHA256_STRING_OUT);

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, &id, sizeof(id));

    // hash with random 256 bits
    int64_t random[4] = { 
        random256Bits.getPart1(), random256Bits.getPart2(),
        random256Bits.getPart3(), random256Bits.getPart4() };
    SHA256_Update(&sha256, &random, sizeof(random));

    SHA256_Update(&sha256, &creationTime, sizeof(creationTime));
    unsigned char result[SHA256_DIGEST_LENGTH];
    SHA256_Final(result, &sha256);

    // convert to readable portable digits.
    HashUtil::sha256ResultToReadableString(result, buf, bufLen);
}

bool UserSessionManager::getUserExtendedDataId(const UserExtendedDataName& name, uint16_t& componentId, uint16_t& dataId)
{
    UserExtendedDataKey key = 0;
    bool success;

    success = getUserExtendedDataKey(name, key);

    if (success)
    {
        componentId = COMPONENT_ID_FROM_UED_KEY(key);
        dataId = DATA_ID_FROM_UED_KEY(key);
    }

    return success;
}

bool UserSessionManager::getUserExtendedDataKey(const UserExtendedDataName& name, UserExtendedDataKey& key)
{

    bool foundKey = false;

    UserExtendedDataIdMap::const_iterator uedItr = mUserExtendedDataIdMap.find(name);
    if (uedItr != mUserExtendedDataIdMap.end())
    {
        foundKey = true;
        key = uedItr->second;
    }

    return foundKey;
}

const char8_t* UserSessionManager::getUserExtendedDataName(UserExtendedDataKey key)
{

    UserExtendedDataIdMap::const_iterator uedItr = mUserExtendedDataIdMap.begin();
    UserExtendedDataIdMap::const_iterator uedEnd = mUserExtendedDataIdMap.end();

    for(; uedItr != uedEnd; ++uedItr)
    {
        if (uedItr->second == key)
        {
            // found the name!
            return uedItr->first;
        }
    }

    return nullptr;
}

void UserSessionManager::addUserExtendedDataId(UserExtendedDataIdMap &userExtendedDataIdMap, const char8_t* name, uint16_t componentId, uint16_t dataId)
{
    //UserExtendedDataName dataName;
    char8_t dataName[MAX_USEREXTENDEDDATANAME_LEN];
    blaze_snzprintf(dataName, MAX_USEREXTENDEDDATANAME_LEN, "%s_%s", BlazeRpcComponentDb::getComponentNameById(componentId), name);
    userExtendedDataIdMap.insert(UserExtendedDataIdMap::value_type(dataName, UED_KEY_FROM_IDS(componentId,dataId)));
}

void UserSessionManager::addUserRegionForCensus()
{
    for (UserSessionPtrByIdMap::iterator itr = mUserSessionPtrByIdMap.begin(), end = mUserSessionPtrByIdMap.end(); itr != end; ++itr)
    {
        UserSessionPtr userSession = itr->second;
        if (userSession->isGameplayUser())
            gCensusDataManager->addUserRegion(userSession->getBlazeId(), userSession->getSessionCountry());
    }
}

void UserSessionManager::getOnlineClubsUsers(BlazeIdSet &blazeIds)
{
    for (UserSessionPtrByIdMap::iterator itr = mUserSessionPtrByIdMap.begin(); itr != mUserSessionPtrByIdMap.end(); ++itr)
    {
        UserSessionPtr userSession = itr->second;
        if (userSession->isGameplayUser() || (userSession->getClientType() == CLIENT_TYPE_HTTP_USER))
            blazeIds.insert(userSession->getBlazeId());
    }
}

void UserSessionManager::insertUniqueDeviceIdForUserSession(UserSessionId sessionId, const char8_t* uniqueDeviceid)
{
    Blaze::UserSessionsMaster* userMaster = getMaster();
    if (userMaster != nullptr)
    {
        InsertUniqueDeviceIdMasterRequest req;
        req.setUserSessionId(sessionId);
        req.setUniqueDeviceId(uniqueDeviceid);
        BlazeRpcError err = userMaster->insertUniqueDeviceIdMaster(req);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].insertUniqueDeviceIdForUserSession: insert unique device Id from sessionId(" << sessionId << ") failed with error "
                << Blaze::ErrorHelp::getErrorName(err));
        }
    }
}

void UserSessionManager::updateDsUserIndexForUserSession(UserSessionId sessionId, int32_t dirtySockUserIndex)
{
    Blaze::UserSessionsMaster* userMaster = getMaster();
    if (userMaster != nullptr)
    {
        UpdateDirtySockUserIndexMasterRequest req;
        req.setUserSessionId(sessionId);
        req.setDirtySockUserIndex(dirtySockUserIndex);
        BlazeRpcError err = userMaster->updateDirtySockUserIndexMaster(req);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].updateDSUserIndexForUserSession: update user session dirty sock user index from sessionId(" << sessionId << ") failed with error "
                << Blaze::ErrorHelp::getErrorName(err));
        }
    }
}


BlazeRpcError UserSessionManager::getIpAddressForUserSession(UserSessionId sessionId, IpAddress& ipAddress)
{
    GetUserIpAddressRequest req;
    GetUserIpAddressResponse resp;
    UserSessionsMaster* userMaster = getMaster();
    req.setUserSessionId(sessionId);
    BlazeRpcError err = ERR_OK;
    if (userMaster != nullptr)
    {
        err = userMaster->getUserIpAddress(req, resp);

        if (err == ERR_OK)
        {
            resp.getIpAddress().copyInto(ipAddress);
        }
        else
        {
            BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].getIpAddressForUserSession: get user ip address for sessionId(" << sessionId << ") failed with error "
                << Blaze::ErrorHelp::getErrorName(err));  
        }
    }
    return err;
}


/*!*********************************************************************************/
/*! \brief Request Users extended data from each listener

    \param userData The users info to pull extended data for
    \param extenededData The resultant extended data
    \param copyFromExistingSession Allow copying extended user data to use stale data from other sessions
************************************************************************************/
BlazeRpcError UserSessionManager::requestUserExtendedData(const UserInfoData &userData, UserSessionExtendedData &extendedData, bool copyFromExistingSession)
{
    if (copyFromExistingSession)
    {
        // Check to see if we can copy the extended data from another session for the same user
        // since extended user data is always fetched at login time. We prefer the primary session's
        // extended data because it is the most complete copy, but we'll settle for others if there is no choice.

        PrimaryExistenceByBlazeIdMap::iterator peit = mPrimaryExistenceByBlazeIdMap.find(userData.getId());
        if (peit != mPrimaryExistenceByBlazeIdMap.end())
        {
            for (PrimarySessionExistenceList::iterator it = peit->second.begin(), end = peit->second.end(); it != end; ++it)
            {
                UserSessionExistence& userSession = static_cast<UserSessionExistence&>(*it);
                if (getClientTypeDescription(userSession.getClientType()).getHasUserExtendedData())
                    return getUserExtendedDataInternal(userSession.getUserSessionId(), extendedData);
            }
        }
        ExistenceByBlazeIdMap::iterator eit = mExistenceByBlazeIdMap.find(userData.getId());
        if (eit != mExistenceByBlazeIdMap.end())
        {
            for (BlazeIdExistenceList::iterator it = eit->second.begin(), end = eit->second.end(); it != end; ++it)
            {
                UserSessionExistence& userSession = static_cast<UserSessionExistence&>(*it);
                if (getClientTypeDescription(userSession.getClientType()).getHasUserExtendedData())
                    return getUserExtendedDataInternal(userSession.getUserSessionId(), extendedData);
            }
        }
    }

    // let local providers do their thing first
    BlazeRpcError err = mExtendedDataDispatcher.dispatchReturnOnError<const UserInfoData &, UserSessionExtendedData &>(&ExtendedDataProvider::onLoadExtendedData, userData, extendedData);
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].requestUserExtendedData: A local provider failed to load user(" 
            << userData.getId() << ") extended data. Error: " << ErrorHelp::getErrorName(err));
        return err;
    }

    // give a shot to remote providers
    RemoteUserExtendedDataProviderSet remoteUserExtendedDataProviderSetCopy(mRemoteUEDProviders);
    bool hasInvalidRemoteProviders = false;
    for (RemoteUserExtendedDataProviderSet::const_iterator it = remoteUserExtendedDataProviderSetCopy.begin(),
         itEnd = remoteUserExtendedDataProviderSetCopy.end(); it != itEnd; ++it)
    {
        Component *component = gController->getComponent(*it, false /*reqLocal*/, true /*reqActive*/);
        if (component == nullptr)
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].requestUserExtendedData: Remote provider(" 
                << *it << ") does not exist.");
            // clean up remote user extended data provider registrations
            hasInvalidRemoteProviders = true;
            // skip over this provider - it might have gone away while we were iterating
            continue;
        }

        RpcCallOptions opts;
        opts.routeTo.setInstanceId(component->selectInstanceId());

        RemoteUserExtendedDataUpdateRequest request;
        request.setComponentId(*it);
        request.setUserInfo(const_cast<UserInfoData&>(userData)); // safe to cast because userData isn't modified from the request
        
        UserSessionExtendedData response;
        err = remoteLoadUserExtendedData(request, response, opts);

        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].requestUserExtendedData: Remote provider " << component->getName() << "(" 
                << component->getId() << ")" << " failed to load user(" << userData.getId() << ") extended data from instance(" << opts.routeTo.getAsInstanceId() 
                << "). Error: " << ErrorHelp::getErrorName(err));
            break;
        }

        const UserExtendedDataMap& remoteExtendedData = response.getDataMap();
        UserExtendedDataMap& localExtendedData = extendedData.getDataMap();
        for (UserExtendedDataMap::const_iterator i = remoteExtendedData.begin(), e = remoteExtendedData.end(); i != e; ++i)
        {
            localExtendedData[i->first] = i->second;
        }

        const ObjectIdList& remoteList = response.getBlazeObjectIdList();
        ObjectIdList& localList = extendedData.getBlazeObjectIdList();
        for (ObjectIdList::const_iterator i = remoteList.begin(), e = remoteList.end(); i != e; ++i)
        {
            if (*i != EA::TDF::OBJECT_ID_INVALID)
                localList.push_back(*i);
        }
    }

    if (hasInvalidRemoteProviders)
        refreshRemoteUserExtendedDataProviderRegistrations();

    return err;

}

/*!*********************************************************************************/
/*! \brief Request a refresh of Users extended data from each listener

\param userData The users info to pull extended data for
\param extenededData The resultant extended data
************************************************************************************/
BlazeRpcError UserSessionManager::refreshUserExtendedData(const UserInfoData& userData, UserSessionExtendedData& extendedData)
{
    // let local providers do their thing first
    BlazeRpcError err = mExtendedDataDispatcher.dispatchReturnOnError<const UserInfoData &, UserSessionExtendedData &>(&ExtendedDataProvider::onRefreshExtendedData, userData, extendedData);
    if (err != ERR_OK)
        return err;

    // give a shot to remote providers
    for (RemoteUserExtendedDataProviderSet::const_iterator it = mRemoteUEDProviders.begin(),
         itEnd = mRemoteUEDProviders.end(); it != itEnd; it++)
    {
        Component *component = gController->getComponent(*it, false /*reqLocal*/, true /*reqActive*/);
        if (component == nullptr)
            return ERR_SYSTEM;
        
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(component->selectInstanceId());

        RemoteUserExtendedDataUpdateRequest request;
        request.setComponentId(*it);
        request.setUserInfo(const_cast<UserInfoData&>(userData)); // safe to cast because userData isn't modified from the request
        extendedData.copyInto(request.getUserSessionExtendedData());
        
        UserSessionExtendedData response;
        err = remoteRefreshUserExtendedData(request, response, opts);

        if (err != ERR_OK)
            return err;

        response.copyInto(extendedData);
    }

    return ERR_OK;
}

BlazeRpcError UserSessionManager::getRemoteUserExtendedData(UserSessionId remoteSessionId, UserSessionExtendedData& extendedData)
{
    if (getMaster() != nullptr)
    {
        GetUserExtendedDataRequest req;
        req.setUserSessionId(remoteSessionId);
        GetUserExtendedDataResponse resp;
        resp.setExtendedData(extendedData);

        return getMaster()->getUserExtendedData(req, resp);
    }
    return ERR_SYSTEM;
}

BlazeRpcError UserSessionManager::getUserExtendedDataInternal(UserSessionId userSessionId, UserSessionExtendedData& extendedData)
{
    BlazeRpcError rc = getRemoteUserExtendedData(userSessionId, extendedData);
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::USER,"[UserSessionManager].getUserExtendedDataInternal: Failed to get UED for remote UserSession(" 
            << userSessionId << "), error: " << ErrorHelp::getErrorName(rc));
        return rc;
    }

    return ERR_OK;
}

// This method can be called in place of syncUserSessions(), to avoid caching user sessions on the local blaze instance.
BlazeRpcError UserSessionManager::getRemoteUserInfo(UserSessionId remoteSessionId, UserInfoData& userInfo)
{
    BlazeRpcError rc = ERR_SYSTEM;
    if (getMaster() != nullptr)
    {
        GetUserInfoDataRequest req;
        req.setUserSessionId(remoteSessionId);
        GetUserInfoDataResponse resp;
        resp.setUserInfo(userInfo);
        rc = getMaster()->getUserInfoData(req, resp);
        if (rc != ERR_OK)
        {
            BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].getRemoteUserInfo: no user info fetched for session id "
                << remoteSessionId);
        }
        else if (userInfo.getId() == INVALID_BLAZE_ID)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].getRemoteUserInfo: User info fetched for session id "
                << remoteSessionId << " has an invalid blaze id.");
            rc = ERR_SYSTEM;
        }
    }
    return rc;
}

BlazeRpcError UserSessionManager::sessionResetGeoIPById(BlazeId blazeId)
{
    // lookup user to check if it's valid blazeid
    UserInfoPtr userInfo;
    
    BlazeRpcError err = lookupUserInfoByBlazeId(blazeId, userInfo);

    if (err != Blaze::ERR_OK)
        return err;

    err = UserInfoDbCalls::resetGeoIPById(blazeId, userInfo->getPlatformInfo().getClientPlatform());

    // update the user session in memory
    if (err == Blaze::ERR_OK)
    {
        GeoLocationData resetData; 
        resetData.setBlazeId(blazeId); 

        err = fetchGeoIPDataById(resetData);
        if (err == Blaze::ERR_OK || err == USER_ERR_GEOIP_LOOKUP_FAILED)
        {
            err = updateCurrentUserSessionsGeoIP(resetData);
        }
    }

    return err;
}

BlazeRpcError UserSessionManager::updateUserSessionExtendedIds(const UserExtendedDataIdMap &userExtendedDataIdMap)
{
     BLAZE_SPAM_LOG(Log::USER,"[UserSessionManager].updateUserSessionExtendedIds: updating ids");
    // does the request have anything different than what's currently in our extended data
    UserExtendedDataIdMap::const_iterator itr = userExtendedDataIdMap.begin();
    UserExtendedDataIdMap::const_iterator end = userExtendedDataIdMap.end();
    if (itr == end)
    {
        // nothing to do
        BLAZE_WARN_LOG(Log::USER,"[UserSessionManager].updateUserSessionExtendedIds: no IDs to update");
        return ERR_OK;
    }
    size_t updatedUEDcount = 0;
    for (; itr != end; ++itr)
    {
        UserExtendedDataKey key;
        if (getUserExtendedDataKey(itr->first, key) && itr->second == key)
        {
            // no change
            BLAZE_TRACE_LOG(Log::USER,"[UserSessionManager].updateUserSessionExtendedIds: attempt non-change to existing ID ("
                << itr->first << "," << SbFormats::HexLower(itr->second) << ")");
            continue;
        }
        else if (getUserExtendedDataKey(itr->first, key) && itr->second != key)
        {
            // log this, but still do a replacement of the old key
            BLAZE_WARN_LOG(Log::USER,"[UserSessionManager].updateUserSessionExtendedIds: attempt to change key to existing ID ("
                << itr->first << "," << SbFormats::HexLower(itr->second) << ")");
        }

        mUserExtendedDataIdMap.insert(UserExtendedDataIdMap::value_type(itr->first, itr->second));
    }

    if (updatedUEDcount == 0)
    {
        BLAZE_TRACE_LOG(Log::USER,"[UserSessionManager].updateUserSessionExtendedIds: no change to existing IDs");
        
    }

    return ERR_OK;
}

BlazeRpcError UserSessionManager::waitForUserSessionExistence(UserSessionId userSessionId, const TimeValue& relTimeout)
{
    if (!UserSession::isValidUserSessionId(userSessionId))
        return ERR_SYSTEM;

    if (getSession(userSessionId) != nullptr)
        return ERR_OK;

    TimeValue absoluteTimeout;
    if (relTimeout != 0)
        absoluteTimeout = TimeValue::getTimeOfDay() + relTimeout;

    return mWaitListByUserSessionIdMap[userSessionId].wait("UserSessionManager::waitForUserSessionExistence", absoluteTimeout);
}

BlazeRpcError UserSessionManager::sessionOverrideGeoIPById(GeoLocationData& overrideData)
{
    // lookup user to check if it's valid blazeid
    UserInfoPtr userInfo;
    
    BlazeRpcError err = lookupUserInfoByBlazeId(overrideData.getBlazeId(), userInfo);

    if (err != Blaze::ERR_OK)
        return err;

    err = UserInfoDbCalls::overrideGeoIPById(overrideData, userInfo->getPlatformInfo().getClientPlatform(), false);

    if (err == Blaze::ERR_OK)
    {
        return updateCurrentUserSessionsGeoIP(overrideData);
    }

    return err;
}

BlazeRpcError UserSessionManager::updateUserCrossPlatformOptIn(const OptInRequest& optInRequest, ClientPlatformType platform)
{

    VERIFY_WORKER_FIBER();

    BlazeRpcError err = ERR_OK;

    err = UserInfoDbCalls::setCrossPlatformOptInById(optInRequest, platform);
    if (err == ERR_OK)
    {
        broadcastUserInfoUpdatedNotification(optInRequest.getBlazeId());
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByIdentification(const UserIdentification& userId, UserInfoPtr& userInfo, bool restrictCrossPlatformResults)
{
    VERIFY_WORKER_FIBER();

    BlazeRpcError err = USER_ERR_USER_NOT_FOUND;
    if (restrictCrossPlatformResults && gCurrentUserSession->getClientPlatform() == common)
    {
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_USER_LOOKUP_CROSSPLATFORM, false))
            return ERR_AUTHORIZATION_REQUIRED;
        restrictCrossPlatformResults = false;
    }

    if (userId.getBlazeId() != INVALID_BLAZE_ID)
    {
        err = lookupUserInfoByBlazeId(userId.getBlazeId(), userInfo);
    }
    else
    {
        ClientPlatformType platform = userId.getPlatformInfo().getClientPlatform();
        if (!gController->isPlatformHosted(platform))
        {
            eastl::string platformInfoStr;
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserData: Unable to look up user; provided UserIdentification does not specify a BlazeId or valid client platform (platform is '"
                << ClientPlatformTypeToString(platform) << "')");
            return USER_ERR_USER_NOT_FOUND;
        }

        if (restrictCrossPlatformResults)
        {
            if (platform == gCurrentUserSession->getClientPlatform())
            {
                restrictCrossPlatformResults = false;
            }
            else
            {
                const ClientPlatformSet& unrestrictedPlatforms = getUnrestrictedPlatforms(gCurrentUserSession->getClientPlatform());
                if (unrestrictedPlatforms.find(platform) == unrestrictedPlatforms.end())
                    return USER_ERR_DISALLOWED_PLATFORM;
            }
        }

        if (userId.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
        {
            err = lookupUserInfoByOriginPersonaId(userId.getPlatformInfo().getEaIds().getOriginPersonaId(), platform, userInfo);
        }
        else if (userId.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
        {
            err = lookupUserInfoByAccountId(userId.getPlatformInfo().getEaIds().getNucleusAccountId(), platform, userInfo);
        }
        else if (!userId.getNameAsTdfString().empty())
        {
            // Note: If restrictCrossPlatformResults is true at this point, then the caller's platform doesn't match the platform of the user being looked up.
            // Since the only platforms that share a namespace are pc and nx (which both use the Origin namespace), we don't need an extra strcmp to check
            // whether the namespace of the user being looked up matches the namespace of the caller.
            const char8_t* personaNamespace = gController->getNamespaceFromPlatform(platform);
            if (restrictCrossPlatformResults && blaze_strcmp(personaNamespace, NAMESPACE_ORIGIN) != 0)
                return USER_ERR_DISALLOWED_PLATFORM;

            err = lookupUserInfoByPersonaName(userId.getName(), platform, userInfo, personaNamespace);
        }
        else if (has1stPartyPlatformInfo(userId.getPlatformInfo()))
        {
            if (restrictCrossPlatformResults)
                return USER_ERR_DISALLOWED_PLATFORM;

            err = lookupUserInfoByPlatformInfo(userId.getPlatformInfo(), userInfo);
        }
    }

    if (restrictCrossPlatformResults && err == ERR_OK && gCurrentUserSession->getClientPlatform() != userInfo->getPlatformInfo().getClientPlatform())
    {
        const ClientPlatformSet& unrestrictedPlatforms = getUnrestrictedPlatforms(gCurrentUserSession->getClientPlatform());
        if (unrestrictedPlatforms.find(userInfo->getPlatformInfo().getClientPlatform()) == unrestrictedPlatforms.end())
            return USER_ERR_DISALLOWED_PLATFORM;

        UserSessionDataPtr userSessionData = nullptr;
        BlazeRpcError optErr = gCurrentUserSession->fetchSessionData(userSessionData);
        if (userSessionData == nullptr)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByIdentification: Error(" << ErrorHelp::getErrorName(optErr) << ") looking up cross platform opt for user(" << gCurrentUserSession->getId() << ")");
            return ERR_SYSTEM;
        }
        if (!userSessionData->getExtendedData().getCrossPlatformOptIn())
            return USER_ERR_CROSS_PLATFORM_OPTOUT;
    }

    return err;
}

/*! \brief On the appropriate slave(s), update local user session counts, then update the in memory user session's geo ip data. 
           Pre: User specified by input data's BlazeId.*/
BlazeRpcError UserSessionManager::updateCurrentUserSessionsGeoIP(const GeoLocationData& overrideData)
{
    GeoLocationData masterRequest;
    overrideData.copyInto(masterRequest);
    UserSessionIdList userSessionIds;
    getUserSessionIdsByBlazeId(overrideData.getBlazeId(), userSessionIds);
    return sendToMasters(userSessionIds.begin(), userSessionIds.end(),
        &UserSessionManager::fetchUserSessionId, &UserSessionsMaster::updateGeoDataMaster, masterRequest);
}

BlazeRpcError UserSessionManager::setUserGeoOptIn(BlazeId blazeId, ClientPlatformType platform, bool optInFlag)
{
    GeoLocationData overrideData;
    overrideData.setBlazeId(blazeId);
    overrideData.setOptIn(optInFlag);
    return UserInfoDbCalls::overrideGeoIPById(overrideData, platform, true);
}

BlazeRpcError UserSessionManager::fetchGeoIPDataById(GeoLocationData& data, bool checkOptIn)
{
    GeoLocationData fetchedGeoData;
    UserSessionId sessionId = getPrimarySessionId(data.getBlazeId());
    if (sessionId == INVALID_USER_SESSION_ID)
        sessionId = getUserSessionIdByBlazeId(data.getBlazeId());
    if (sessionId == INVALID_USER_SESSION_ID)
    {
        // The user is not logged in, but there may be
        // GeoIP data available in the database
        BlazeRpcError err = UserInfoDbCalls::lookupLatLonById(data.getBlazeId(), fetchedGeoData);
        if (err != ERR_OK)
            return err;
    }
    else
    {
        RpcCallOptions opts;
        GetUserGeoIpDataRequest request;
        request.setUserSessionId(sessionId);
        request.setBlazeId(data.getBlazeId());

        opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, sessionId);
        BlazeRpcError err = UserSessionsSlave::getUserGeoIpData(request, fetchedGeoData, opts);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].fetchGeoIPDataById: Failed to retrieve GeoIp Data for User(" << data.getBlazeId()
                << "), UserSession(" << sessionId << ").");
            return err;
        }
    }

    // caller can always lookup his own geoip data even if he is not opt-in
    if (checkOptIn && gCurrentUserSession != nullptr)
    {
        checkOptIn = !((gCurrentUserSession->getBlazeId() == data.getBlazeId()));
    }

    if (checkOptIn && !fetchedGeoData.getOptIn() && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_FETCH_GEOIP_DATA, true))
    {
        return GEOIP_ERR_USER_OPTOUT;
    }

    fetchedGeoData.setBlazeId(data.getBlazeId());
    fetchedGeoData.copyInto(data);

    return ERR_OK;
}

BlazeRpcError UserSessionManager::getGeoIpDataWithOverrides(BlazeId id, const InetAddress& connAddr, GeoLocationData& geoLocData)
{
    if (!getConfig().getUseGeoipData())
        return ERR_OK;

    // Location overrides can only be performed by supplying a valid latitude and longitude to the OverrideUserGeoIPData RPC.
    // If latitude/longitude can't be looked up, the override is not set. Thus if geoLocData.getIsOverridden() is true,
    // the location override is complete and valid, but we still fetch to only fill out the rest (i.e., ISP).

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*) gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (ident == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].getGeoIpDataWithOverrides(" << id << "," << connAddr.getIpAsString() << "): NucleusIdentity component is not available");
        return ERR_SYSTEM;
    }

    BlazeRpcError rc = ERR_OK;
    {
        OAuth::AccessTokenUtil tokenUtil;
        UserSession::SuperUserPermissionAutoPtr ptr(true);
        rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (rc != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].getGeoIpDataWithOverrides(" << id << "," << connAddr.getIpAsString() << "): failed to retrieve server access token");
            return ERR_SYSTEM;
        }

        NucleusIdentity::GetIpGeoLocationRequest req;
        NucleusIdentity::GetIpGeoLocationResponse rsp;
        NucleusIdentity::IdentityError err;
        req.setIpAddress(connAddr.getIpAsString());
        req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());

        rc = ident->getIpGeoLocation(req, rsp, err);

        if (rc != ERR_OK)
        {
            BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].getGeoIpDataWithOverrides(" << id << "," << connAddr.getIpAsString() << "): couldn't look up geolocation data: " << ErrorHelp::getErrorName(rc)
                << "\nreq:" << SbFormats::PrefixAppender("  ", (StringBuilder() << req).get())
                << "\nerr:" << SbFormats::PrefixAppender("  ", (StringBuilder() << err).get())
                << "\nrsp:" << SbFormats::PrefixAppender("  ", (StringBuilder() << rsp).get())
                << "\n");
            mMetrics.mGeoIpLookupsFailed.increment();
            return USER_ERR_GEOIP_LOOKUP_FAILED;
        }

        rc = convertGeoLocationData(id, rsp.getIpGeoLocation(), geoLocData);
    }
    return rc;
}

BlazeRpcError UserSessionManager::convertGeoLocationData(BlazeId id, const NucleusIdentity::IpGeoLocation& geoLoc, GeoLocationData& geoLocData)
{
    if (id != INVALID_BLAZE_ID)
    {
        // check for any override to use
        BlazeRpcError err = UserInfoDbCalls::lookupLatLonById(id, geoLocData);
        if ((err != ERR_OK) && (err != USER_ERR_USER_NOT_FOUND))
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager:].convertGeoLocationData: lookupLatLonById(" << id << ") failed with " << ErrorHelp::getErrorName(err));
            return USER_ERR_GEOIP_LOOKUP_FAILED;
        }
    }

    if (!geoLocData.getIsOverridden())
    {
        char8_t* strEnd = nullptr;
        geoLocData.setLatitude(EA::StdC::StrtoF32(geoLoc.getLatitude(), &strEnd));
        geoLocData.setLongitude(EA::StdC::StrtoF32(geoLoc.getLongitude(), &strEnd));

        // validate useful latitude/longitude
        if (geoLocData.getLatitude() == 0.0f && geoLocData.getLongitude() == 0.0f)
        {
            // treating (0,0) response from Nucleus as a lookup "failure", so we'll use our own default.
            geoLocData.setLatitude(GEO_IP_DEFAULT_LATITUDE);
            geoLocData.setLongitude(GEO_IP_DEFAULT_LONGITUDE);
        }
    }
    geoLocData.setCity(geoLoc.getCity());
    geoLocData.setCountry(geoLoc.getCountry());
    geoLocData.setStateRegion(geoLoc.getRegion());
    geoLocData.setISP(geoLoc.getISPName());
    geoLocData.setTimeZone(geoLoc.getTimeZone());

    return ERR_OK;
}

ConnectionGroupId UserSessionManager::makeConnectionGroupId(SliverId sliverId, const ConnectionId connId)
{
    ConnectionGroupId result = (ConnectionGroupId)(((SliverKey)connId << 14) | sliverId);
    EA_ASSERT(result >> 32 == 0);
    return result;
}

/*!*********************************************************************************/
/*! \brief Updates a user's info in the db and the replicated UserSessions map.
    
    This function will not return until the replicated data has been broadcast globally.
    
    \param data The new user data to publish.
    \param nullExternalId If true the externalid and externalstring fields are nulled out regardless of the PlatformInfo in the provided UserInfoData
    \param updateTimeFields If true the previouslogindatetime and lastlogindatetime fields are updated
    \param updateCrossPlatformOpt If true the crossplatformopt field is updated
************************************************************************************/
BlazeRpcError UserSessionManager::upsertUserInfo(const UserInfoData& userInfo, bool nullExternalId, bool updateTimeFields, bool updateCrossPlatformOpt, bool& isFirstLogin, bool& isFirstConsoleLogin, uint32_t& previousAccountCountry, bool broadcastUpdateNotification)
{    
    VERIFY_WORKER_FIBER();

    UserInfoDbCalls::UpsertUserInfoResults upsertUserInfoResults(userInfo.getAccountCountry());

    //Update the user info record.
    BlazeRpcError err = UserInfoDbCalls::upsertUserInfo(userInfo, nullExternalId, updateTimeFields, updateCrossPlatformOpt, upsertUserInfoResults);
    previousAccountCountry = upsertUserInfoResults.previousCountry;
    isFirstLogin = upsertUserInfoResults.newRowInserted;
    isFirstConsoleLogin = upsertUserInfoResults.firstSetExternalData;

    // If this user's platform info changed, we need to update the PlatformInfo
    // of all users linked to his Nucleus account
    AccountId accountIdToUpdate = upsertUserInfoResults.changedPlatformInfo ? userInfo.getPlatformInfo().getEaIds().getNucleusAccountId() : INVALID_ACCOUNT_ID;

    updateUserInfoCache(userInfo.getId(), accountIdToUpdate, broadcastUpdateNotification);
    return err;
}

/*!*********************************************************************************/
/*! \brief Updates a user's account and user info info in the db and the replicated UserSessions map.
           Should only be called when a new user session is created.

    This function will not return until the replicated data has been broadcast globally.
************************************************************************************/
BlazeRpcError UserSessionManager::upsertUserAndAccountInfo(const UserInfoData& userInfo, UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool updateCrossPlatformOpt, bool broadcastUpdateNotification)
{
    VERIFY_WORKER_FIBER();

    // Update the account info record
    BlazeRpcError err = AccountInfoDbCalls::upsertAccountInfo(userInfo);
    if (err == USER_ERR_EXISTS)
    {
        err = AccountInfoDbCalls::resolve1stPartyIdConflicts(userInfo);

        // USER_NOT_FOUND means that resolve1stPartyIdConflicts didn't resolve any conflicts
        if (err == USER_ERR_USER_NOT_FOUND)
            err = USER_ERR_EXISTS;
        else if (err == ERR_OK)
            err = AccountInfoDbCalls::upsertAccountInfo(userInfo);
    }

    if (err != ERR_OK)
    {
        if (err == USER_ERR_EXISTS)
        {
            // There must be another user in the accountinfo db with a conflicting persona name.
            // ConsoleRenameUtil::doRename will update that user with the correct persona name from Nucleus.
            // But first, temporarily set this user's persona name to nullptr,
            // to prevent any naming loops/deadlocks.
            err = updateOriginPersonaName(userInfo.getPlatformInfo().getEaIds().getNucleusAccountId(), nullptr, false /*broadcastUpdateNotification*/);
            if (err == ERR_OK)
                return USER_ERR_EXISTS;
        }
        return err;
    }

    bool broadcastNotification = broadcastUpdateNotification;

    //Update the user info record.
    err = UserInfoDbCalls::upsertUserInfo(userInfo, false /*nullExternalId*/, true /*updateTimeFields*/, updateCrossPlatformOpt, upsertUserInfoResults);

    // If we get USER_ERR_EXISTS, then let ConsoleRenameUtil::doRename broadcast the update notification
    if (err == USER_ERR_EXISTS)
        broadcastNotification = false;

    // If this user's platform info changed, we need to update the PlatformInfo
    // of all users linked to his Nucleus account
    AccountId accountIdToUpdate = upsertUserInfoResults.changedPlatformInfo ? userInfo.getPlatformInfo().getEaIds().getNucleusAccountId() : INVALID_ACCOUNT_ID;

    updateUserInfoCache(userInfo.getId(), accountIdToUpdate, broadcastUpdateNotification);
    return err;
}

/*!*********************************************************************************/
/*! \brief Updates a user's Origin persona name in the db and the replicated UserSessions map.

    This function will not return until the replicated data has been broadcast globally.
************************************************************************************/
BlazeRpcError UserSessionManager::updateOriginPersonaName(AccountId nucleusAccountId, const char8_t* originPersonaName, bool broadcastUpdateNotification)
{
    char8_t canonicalPersona[MAX_PERSONA_LENGTH];
    const char8_t* originName = nullptr;
    const char8_t* canonicalName = nullptr;
    if (originPersonaName != nullptr && originPersonaName[0] != '\0')
    {
        originName = originPersonaName;
        getNameHandler().generateCanonical(originPersonaName, canonicalPersona, sizeof(canonicalPersona));
        canonicalName = canonicalPersona;
    }

    BlazeRpcError err = AccountInfoDbCalls::updateOriginPersonaName(nucleusAccountId, originPersonaName, canonicalPersona);

    if (err == ERR_OK)
    {
        // This account may have a linked user on a platform that uses Origin persona names instead of 1st-party persona names.
        // If so, the origin persona name needs to be updated in the corresponding platform-specific userinfo db table.
        err = UserInfoDbCalls::updateOriginPersonaName(nucleusAccountId, originName, canonicalName);

        // Update the PlatformInfo of the UserInfo cache entries linked to this Nucleus account
        // Note that updateUserInfoCache ignores the BlazeId when account id is provided
        updateUserInfoCache(INVALID_BLAZE_ID, nucleusAccountId, broadcastUpdateNotification);
    }
    return err;
}

/*!*********************************************************************************/
/*! \brief Updates a user's last used locale and error in the db and the replicated UserSessions map.
    
    This function will not return until the replicated data has been broadcast globally.
************************************************************************************/
BlazeRpcError UserSessionManager::updateLastUsedLocaleAndError(const UserInfoData& userInfo)
{    
    VERIFY_WORKER_FIBER();

    //Update the user info record.
    BlazeRpcError err = UserInfoDbCalls::updateUserInfoLastUsedLocaleAndError(userInfo.getId(), userInfo.getPlatformInfo().getClientPlatform(), userInfo.getLastAuthErrorCode(), userInfo.getLastLocaleUsed(), userInfo.getLastCountryUsed());

    broadcastUserInfoUpdatedNotification(userInfo.getId());

    return err;
}

/*!*********************************************************************************/
/*! \brief Updates a user's info on all the instances that own usersessions with the same BlazeId
    
    This function will not return until the replicated data has been updated globally.
    
    \param data The new user data to publish.
************************************************************************************/
//BlazeRpcError UserSessionManager::updateUserInfoInternal(const UserInfoData &data)
//{    
//    BlazeRpcError finalErr = ERR_OK;
//
//    UserSessionIdList userSessionIds;
//    getUserSessionIdsByBlazeId(data.getId(), userSessionIds, &ClientTypeFlags::getUpdateUserInfoData);
//
//    for (UserSessionIdList::const_iterator it = userSessionIds.begin(), end = userSessionIds.end(); it != end; ++it)
//    {
//        RpcCallOptions opts;
//        opts.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, *it);
//        BlazeRpcError err = getMaster()->updateUserInfoMaster(data, opts);
//        if (err != ERR_OK)
//        {
//            // TODO: log error
//            finalErr = err;
//        }
//
//    }
//
//    return finalErr;
//}

template <typename idType, typename idMapType>
void getUserSessionIdsByTemplate(const idMapType & idMap,  idType id, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const, bool localOnly)
{
    auto eit = idMap.find(id);
    if (eit != idMap.end())
    {
        const uint32_t maxUserSessions = gUserSessionManager->getConfig().getMaxConcurrentUserSessions();

        const size_t maxToLog = 10;
        eastl::vector<const UserSessionExistence*> userSessionsForLog;
        userSessionsForLog.reserve(maxToLog);

        for (auto i = eit->second.begin(), e = eit->second.end(); i != e; ++i)
        {
            const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(*i);
            if ((clientTypeCheckFunc == nullptr || (gUserSessionManager->getClientTypeDescription(userSession.getClientType()).*clientTypeCheckFunc)()) &&
                (!localOnly || UserSession::isOwnedByThisInstance(userSession.getUserSessionId())))
            {
                if (userSessionIds.size() < maxUserSessions)
                {
                    userSessionIds.push_back(userSession.getUserSessionId());

                    // prepare log if looks like we may go over the maxUserSessions limit
                    if ((eit->second.size() > maxUserSessions) && (userSessionsForLog.size() <= maxToLog))
                    {
                        userSessionsForLog.push_back(&userSession);
                    }
                }
                else
                {
                    eastl::string sessStr;
                    for (const auto& sess : userSessionsForLog)
                    {
                        eastl::string platformInfoStr;
                        sessStr.append_sprintf("(%" PRIu64 ": blazeId=%" PRIi64 ", platformInfo=(%s), %s, %s, %s), ",
                            sess->getId(), sess->getBlazeId(), platformInfoToString(sess->getPlatformInfo(), platformInfoStr), ClientPlatformTypeToString(sess->getClientPlatform()), ClientTypeToString(sess->getClientType()), UserSessionTypeToString(sess->getUserSessionType()));
                    }
                    BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getUserSessionIdsByAccountId: Number of concurrent user sessions for user: "
                        << id << " exceeds configured limit: " << maxUserSessions << ", truncating result.  Fiber context: " << Fiber::getCurrentContext() <<
                        ".  Sessions: " << sessStr << (userSessionsForLog.size() < userSessionIds.size() ? "...(etc)," : "") << " User may have logged in too many times.");
                    break;
                }
            }
        }
    }
}


void UserSessionManager::getUserSessionIdsByAccountId(AccountId accountId, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const, bool localOnly) const
{
    getUserSessionIdsByTemplate(mExistenceByAccountIdMap, accountId, userSessionIds, clientTypeCheckFunc, localOnly);
}

void UserSessionManager::getUserSessionIdsByBlazeId(BlazeId blazeId, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const, bool localOnly) const
{
    getUserSessionIdsByTemplate(mExistenceByBlazeIdMap, blazeId, userSessionIds, clientTypeCheckFunc, localOnly);
}

void UserSessionManager::getUserSessionIdsByPlatformInfo(const PlatformInfo& platformInfo, UserSessionIdList& userSessionIds, bool (ClientTypeFlags::*clientTypeCheckFunc)() const, bool localOnly) const
{
    getUserSessionIdsByTemplate(mExistenceByPsnIdMap, platformInfo.getExternalIds().getPsnAccountId(), userSessionIds, clientTypeCheckFunc, localOnly);
    getUserSessionIdsByTemplate(mExistenceByXblIdMap, platformInfo.getExternalIds().getXblAccountId(), userSessionIds, clientTypeCheckFunc, localOnly);
    getUserSessionIdsByTemplate(mExistenceBySteamIdMap, platformInfo.getExternalIds().getSteamAccountId(), userSessionIds, clientTypeCheckFunc, localOnly);
    getUserSessionIdsByTemplate(mExistenceBySwitchIdMap, platformInfo.getExternalIds().getSwitchId(), userSessionIds, clientTypeCheckFunc, localOnly);
    getUserSessionIdsByTemplate(mExistenceByAccountIdMap, platformInfo.getEaIds().getNucleusAccountId(), userSessionIds, clientTypeCheckFunc, localOnly);
}

TimeValue UserSessionManager::getSessionCleanupTimeout(ClientType clientType) const
{
    UserSessionsConfig::TimeValueByClientTypeMap::const_iterator it = getConfig().getSessionCleanupTimeoutByClientType().find(clientType);
    if (it != getConfig().getSessionCleanupTimeoutByClientType().end())
    {
        return it->second;
    }

    return getConfig().getSessionCleanupTimeout();
}

UserSessionPtr UserSessionManager::getSession(UserSessionId userSessionId)
{
    if (UserSession::isValidUserSessionId(userSessionId))
    {
        UserSessionPtrByIdMap::iterator it = mUserSessionPtrByIdMap.find(userSessionId);
        if (it != mUserSessionPtrByIdMap.end())
            return it->second;
    }

    return nullptr;
}

UserSessionId UserSessionManager::getUserSessionIdByBlazeId(BlazeId blazeId) const
{
    ExistenceByBlazeIdMap::const_iterator eit = mExistenceByBlazeIdMap.find(blazeId);
    if (eit != mExistenceByBlazeIdMap.end() && !eit->second.empty())
    {
        const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(eit->second.front());
        return userSession.getUserSessionId();
    }
    return INVALID_USER_SESSION_ID;
}

UserSessionId UserSessionManager::getUserSessionIdByPlatformInfo(const PlatformInfo& platformInfo) const
{
    switch (platformInfo.getClientPlatform())
    {
    case ps4:
    case ps5:
        if (platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
        {
            ExistenceByPsnAccountIdMap::const_iterator eit = mExistenceByPsnIdMap.find(platformInfo.getExternalIds().getPsnAccountId());
            if (eit != mExistenceByPsnIdMap.end() && !eit->second.empty())
            {
                const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(eit->second.front());
                return userSession.getUserSessionId();
            }
        }
        return INVALID_USER_SESSION_ID;
    case xone:
    case xbsx:
        if (platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
        {
            ExistenceByXblAccountIdMap::const_iterator eit = mExistenceByXblIdMap.find(platformInfo.getExternalIds().getXblAccountId());
            if (eit != mExistenceByXblIdMap.end() && !eit->second.empty())
            {
                const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(eit->second.front());
                return userSession.getUserSessionId();
            }
        }
        return INVALID_USER_SESSION_ID;
    case nx:
        if (!platformInfo.getExternalIds().getSwitchIdAsTdfString().empty())
        {
            ExistenceBySwitchIdMap::const_iterator eit = mExistenceBySwitchIdMap.find(platformInfo.getExternalIds().getSwitchId());
            if (eit != mExistenceBySwitchIdMap.end() && !eit->second.empty())
            {
                const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(eit->second.front());
                return userSession.getUserSessionId();
            }
        }
        return INVALID_USER_SESSION_ID;
    case pc:
        if (platformInfo.getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
        {
            ExistenceByAccountIdMap::const_iterator eit = mExistenceByAccountIdMap.find(platformInfo.getEaIds().getNucleusAccountId());
            if (eit != mExistenceByAccountIdMap.end() && !eit->second.empty())
            {
                for (const auto& sessionIt : eit->second)
                {
                    const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(sessionIt);
                    if (userSession.getPlatformInfo().getClientPlatform() == pc)
                        return userSession.getUserSessionId();
                }
            }
        }
        return INVALID_USER_SESSION_ID;
    case steam:
        if (platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        {
            ExistenceBySteamAccountIdMap::const_iterator eit = mExistenceBySteamIdMap.find(platformInfo.getExternalIds().getSteamAccountId());
            if (eit != mExistenceBySteamIdMap.end() && !eit->second.empty())
            {
                const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(eit->second.front());
                return userSession.getUserSessionId();
            }
        }
        return INVALID_USER_SESSION_ID; 
    case stadia:
        if (platformInfo.getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
        {
            ExistenceByStadiaAccountIdMap::const_iterator eit = mExistenceByStadiaIdMap.find(platformInfo.getExternalIds().getStadiaAccountId());
            if (eit != mExistenceByStadiaIdMap.end() && !eit->second.empty())
            {
                const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(eit->second.front());
                return userSession.getUserSessionId();
            }
        }
        return INVALID_USER_SESSION_ID; 
    default:
        return INVALID_USER_SESSION_ID;
    }
}


UserSessionId UserSessionManager::getUserSessionIdByAccountId(AccountId accountId, ClientPlatformType platform) const
{
    ExistenceByAccountIdMap::const_iterator eit = mExistenceByAccountIdMap.find(accountId);
    if (eit != mExistenceByAccountIdMap.end() && !eit->second.empty())
    {
        for (const auto& sessionIt : eit->second)
        {
            const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(sessionIt);
            if (userSession.getPlatformInfo().getClientPlatform() == platform)
                return userSession.getUserSessionId();
        }
    }
    return INVALID_USER_SESSION_ID;
}

template<typename InputIterator, typename SessionIdFetchFunction, typename SendToMasterFunction, typename Request>
BlazeRpcError UserSessionManager::sendToMasters(InputIterator first, InputIterator last, SessionIdFetchFunction fetchFunction, SendToMasterFunction sendFunction, const Request& req) const
{
    typedef eastl::vector_set<SliverIdentity> MasterSliverIdentitySet;
    MasterSliverIdentitySet masterSliverIdentities;
    for (; first != last; ++first)
    {
        UserSessionId sessionId = fetchFunction(*first);
        if (sessionId != INVALID_USER_SESSION_ID)
        {
            RpcCallOptions opts;
            opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, sessionId);
            if (masterSliverIdentities.insert(opts.routeTo.getAsSliverIdentity()).second)
            {
                BlazeRpcError rc = (getMaster()->*sendFunction)(req, opts, nullptr);
                if (rc != Blaze::ERR_OK)
                    return rc;
            }
        }
    }
    return Blaze::ERR_OK;
}

template<typename InputIterator, typename SessionIdFetchFunction, typename SendToMasterFunction, typename Request, typename Response>
BlazeRpcError UserSessionManager::sendToMasters(InputIterator first, InputIterator last, SessionIdFetchFunction fetchFunction, SendToMasterFunction sendFunction, const Request& req, Response& resp) const
{
    typedef eastl::vector_set<SliverIdentity> MasterSliverIdentitySet;
    MasterSliverIdentitySet masterSliverIdentities;
    for (; first != last; ++first)
    {
        UserSessionId sessionId = fetchFunction(*first);
        if (sessionId != INVALID_USER_SESSION_ID)
        {
            RpcCallOptions opts;
            opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, sessionId);
            if (masterSliverIdentities.insert(opts.routeTo.getAsSliverIdentity()).second)
            {
                BlazeRpcError rc = (getMaster()->*sendFunction)(req, resp, opts, nullptr);
                if (rc != Blaze::ERR_OK)
                    return rc;
            }
        }
    }
    return Blaze::ERR_OK;
}

/*! ***********************************************************************/
/*! \brief Updates a session's extended data in the global map

    \param[in] session - The session to update the data for
    \param[in] componentId - The id of the component that owns the data
    \param[in] key - A component unique key for the data
    \param[in] value - The value of the data.
***************************************************************************/
BlazeRpcError UserSessionManager::updateExtendedData(UserSessionId userSessionId, uint16_t componentId, uint16_t key, UserExtendedDataValue value) const
{
    UpdateExtendedDataRequest request;
    request.setComponentId(componentId);
    request.setIdsAreSessions(true);

    request.getUserOrSessionToUpdateMap().reserve(1);
    UpdateExtendedDataRequest::ExtendedDataUpdate *update = request.getUserOrSessionToUpdateMap().allocate_element();
    (update->getDataMap())[key] = value;
    (request.getUserOrSessionToUpdateMap())[userSessionId] = update;

    RpcCallOptions opts;
    opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, userSessionId);
    return getMaster()->updateExtendedDataMaster(request, opts);
}

/*! ***********************************************************************/
/*! \brief Removes a session's extended data in the global map by component/key

    \param[in] session - The session to update the data for
    \param[in] componentId - The id of the component that owns the data
    \param[in] key - A component unique key for the data
***************************************************************************/
BlazeRpcError UserSessionManager::removeExtendedData(UserSessionId userSessionId, uint16_t componentId, uint16_t key) const
{
    UpdateExtendedDataRequest request;
    request.setComponentId(componentId);
    request.setIdsAreSessions(true);
    request.setRemove(true);

    request.getUserOrSessionToUpdateMap().reserve(1);
    UpdateExtendedDataRequest::ExtendedDataUpdate *update = request.getUserOrSessionToUpdateMap().allocate_element();
    (update->getDataMap())[key] = 0;
    (request.getUserOrSessionToUpdateMap())[userSessionId] = update;

    RpcCallOptions opts;
    opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, userSessionId);
    return getMaster()->updateExtendedDataMaster(request, opts);
}


/*! ***************************************************************************/
/*!    \brief Sends a single error event to the PIN service for a single user session id.
              Fire and forget- assumes caller doesn't care about the result of pin submission attempts.

    \param[in] errorId - The char8_t version of BlazeRpcError
    \param[in] errorType - The char8_t version of errorType for the BlazeRpcError
    \param[in] serviceName - (optional) Override service name for error event.
    \param[in] headerCore - (optional) Override all variables with those specified in headerCore, and don't depend on current user session for them

*******************************************************************************/
void UserSessionManager::sendPINErrorEvent(const char8_t* errorId, RiverPoster::ErrorTypes errorType, const char8_t* serviceName/* = ""*/, RiverPoster::PINEventHeaderCore* headerCore /*= nullptr*/)
{
    if ((UserSession::getCurrentUserSessionId() == INVALID_USER_SESSION_ID) && (headerCore == nullptr))
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].sendPINErrorEvent: Unable to send error event because target session id is invalid for error: " << errorId << " and error type: " << errorType);
        return;
    }

    RiverPoster::ErrorEventPtr errorEvent = RiverPoster::ErrorEventPtr(BLAZE_NEW RiverPoster::ErrorEvent());
    errorEvent->setErrorId(errorId);
    errorEvent->setErrorType(RiverPoster::ErrorTypesToString(errorType));
    errorEvent->setServerType(RiverPoster::PIN_SDK_TYPE_BLAZE);

    UserSessionId targetUserSessionId = INVALID_USER_SESSION_ID;

    if (headerCore != nullptr)
    {
        EA::TDF::MemberVisitOptions memberVisitOptions;
        memberVisitOptions.onlyIfSet = true;
        headerCore->copyInto(errorEvent->getHeaderCore(), memberVisitOptions);

        if (serviceName[0] != '\0')
        {
            errorEvent->setServerName(serviceName);
        }
    }
    else
    {
        // If a user session is not passed in, we are going to depend on the gCurrentUserSession for the PIN Event information.
        targetUserSessionId = UserSession::getCurrentUserSessionId();
        if (gCurrentUserSession != nullptr)
        {
            errorEvent->getHeaderCore().setClientIp(InetAddress(gCurrentUserSession->getExistenceData().getClientIp(), 0, InetAddress::NET).getIpAsString());
            errorEvent->setServerName(gCurrentUserSession->getServiceName());
            targetUserSessionId = gCurrentUserSession->getSessionId();
        }
        if (UserSession::isValidUserSessionId(targetUserSessionId))
        {
            eastl::string pinSessionId;
            pinSessionId.sprintf("%" PRIu64, targetUserSessionId);
            errorEvent->setSessionId(pinSessionId.c_str());
        }
    }

    errorEvent->getHeaderCore().setEventName(RiverPoster::ERROR_EVENT_NAME);

    PINSubmissionPtr errorRequest = BLAZE_NEW PINSubmission;
    RiverPoster::PINEventList *pinEventList = errorRequest->getEventsMap().allocate_element();
    EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinEvent = pinEventList->pull_back();
    pinEvent->set(*errorEvent);
    errorRequest->getEventsMap()[targetUserSessionId] = pinEventList;
    sendPINEvents(errorRequest);
}

/*! ***************************************************************************/
/*!    \brief Sends a single error event to the PIN service for a single user session id.
              Fire and forget- assumes caller doesn't care about the result of pin submission attempts.

    \param[in] errorId - The char8_t version of BlazeRpcError
    \param[in] errorType - The char8_t version of errorType for the BlazeRpcError
    \param[in] userSessionIdList - List of user session ids to send the PIN Error Event for.

*******************************************************************************/
void UserSessionManager::sendPINErrorEventToUserSessionIdList(const char8_t* errorId, RiverPoster::ErrorTypes errorType, UserSessionIdList& userSessionIdList)
{
    if (userSessionIdList.empty())
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].sendPINErrorEventToUserSessionIdList: Unable to send error event because target userSessionIdList is empty for error: " << errorId << " and error type: " << errorType);
        return;
    }

    RiverPoster::ErrorEventPtr errorEvent = RiverPoster::ErrorEventPtr(BLAZE_NEW RiverPoster::ErrorEvent());
    errorEvent->setErrorId(errorId);
    errorEvent->setErrorType(RiverPoster::ErrorTypesToString(errorType));
    errorEvent->getHeaderCore().setEventName(RiverPoster::ERROR_EVENT_NAME);
    errorEvent->setServerType(RiverPoster::PIN_SDK_TYPE_BLAZE);
    errorEvent->setServerName(gController->getDefaultServiceName());

    UserSessionIdList::const_iterator itr =  userSessionIdList.begin();
    PINSubmissionPtr errorRequest = BLAZE_NEW PINSubmission;
    for (; itr != userSessionIdList.end(); ++itr)
    {
        UserSessionId userSessionId = *itr;
        eastl::string pinSessionId;
        pinSessionId.sprintf("%" PRIu64, userSessionId);
        errorEvent->setSessionId(pinSessionId.c_str());

        RiverPoster::PINEventList *pinEventList = errorRequest->getEventsMap().allocate_element();
        EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinEvent = pinEventList->pull_back();
        pinEvent->set(*errorEvent);
        errorRequest->getEventsMap()[userSessionId] = pinEventList;
    }

    sendPINEvents(errorRequest);
}

/*! ***************************************************************************/
/*!    \brief Does a bulk PIN event report on a set of users or sessions.
              Fire and forget- assumes caller doesn't care about the result of pin submission attempts.
    \param request the events to send
*******************************************************************************/
void UserSessionManager::sendPINEvents(PINSubmissionPtr request)
{
    if (request == nullptr)
        return;

    // Convert the PINEventsMap into a list of events per platform
    eastl::map<ClientPlatformType, RiverPoster::PINEventList> events;

    PINEventsMap::iterator eventMapIter = request->getEventsMap().begin();
    PINEventsMap::iterator eventMapEnd = request->getEventsMap().end();
    for (; eventMapIter != eventMapEnd; ++eventMapIter)
    {
        UserSessionId userSessionId = eventMapIter->first;
        // need to iterate over the map to get all the events we received
        UserSessionPtr session = getSession(userSessionId); // this is a 'best effort' lookup
        if (session == nullptr)
        {
            UserSessionPtrByIdMap::const_iterator it = mExtinctUserSessionPtrByIdMap.find(userSessionId);
            if (it != mExtinctUserSessionPtrByIdMap.end())
                session = it->second;
        }

        const UserSessionExistenceData* existenceData = (session != nullptr) ? &session->getExistenceData() : nullptr;

        // we won't be setting the step numbers here, but we do everything else
        RiverPoster::PINEventList::iterator eventListIter = eventMapIter->second->begin();
        RiverPoster::PINEventList::iterator eventListEnd = eventMapIter->second->end();
        for (; eventListIter != eventListEnd; ++eventListIter)
        {
            // get the HeaderCore and set the step number
            bool validListEntry = (*eventListIter)->getTypeDescription().isTdfObject();
            if (validListEntry)
            {
                // TDF object, get the tdf, check for our magic member.
                EA::TDF::Tdf* eventTdf = (*eventListIter)->get();
                if (eventTdf == nullptr)
                {
                    BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].sendPINEvents: "
                        << "PIN event TDF object is null for UserSessionId(" << eventMapIter->first << ") with event list size(" << eventMapIter->second->size() << ")");
                    continue;
                }

                //for dedicated servers, only connection pin events are allowed to be sent
                if (existenceData != nullptr)
                {
                    // We specifically allow the connection events for the dedicated servers to the players.  
                    // Not sure what DS events are supposed to be ignored, but the Login event is still gone at least. 
                    if (eventTdf->getTypeDescription().getTdfId() != RiverPoster::ConnectionEvent::TDF_ID && 
                        eventTdf->getTypeDescription().getTdfId() != RiverPoster::ConnectionStatsEvent::TDF_ID)
                    {
                        if ((existenceData->getClientType() == CLIENT_TYPE_DEDICATED_SERVER) ||
                            existenceData->getUserSessionType() == USER_SESSION_TRUSTED)
                        {
                            continue;
                        }
                    }
                }

                EA::TDF::TdfMemberIterator headerCoreIter(*eventTdf); 
                validListEntry = eventTdf->getIteratorByName(RiverPoster::EVENT_HEADER_MEMBER_NAME, headerCoreIter);
                if (validListEntry)
                {
                    EA::TDF::Tdf &tdf = headerCoreIter.asTdf();
                    EA::TDF::TdfMemberIteratorConst itr(tdf);
                    validListEntry = (tdf.getTdfId() == RiverPoster::PINEventHeaderCore::TDF_ID);
                    if (validListEntry)
                    {
                        RiverPoster::PINEventHeaderCore &headerCore = static_cast<RiverPoster::PINEventHeaderCore&>(tdf);
                        filloutPINEventHeader(headerCore, userSessionId, existenceData);

                        ClientPlatformType eventPlatform = gController->getDefaultClientPlatform();
                        if (gController->isSharedCluster() && existenceData != nullptr)
                            eventPlatform = existenceData->getPlatformInfo().getClientPlatform();

                        events[eventPlatform].push_back(*eventListIter);
                    }
                }
            }

            if (!validListEntry)
            {
                // wrong type of object, or missing HeaderCore, log error and skip event
                BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].sendPINEvents: Event for user session '" << eventMapIter->first << "' of type '" 
                    <<  (*eventListIter)->get()->getClassName() << "' had no member RiverPoster::PINEventHeaderCore named '"  << RiverPoster::EVENT_HEADER_MEMBER_NAME << "'.");
            }
        }
    }

    for (auto& eventItr : events)
    {
        if (!eventItr.second.empty())
            doSendPINEvent(eventItr.first, eventItr.second);
    }
}

void UserSessionManager::filloutPINEventHeader(RiverPoster::PINEventHeaderCore& headerCore, UserSessionId userSessionId, const UserSessionExistenceData* existenceData) const
{
    if (!headerCore.isIsSessionSet())
        headerCore.setIsSession(UserSession::isValidUserSessionId(userSessionId));

    // user info
    bool condition = (headerCore.isPlayerIdSet() && headerCore.isPlayerIdTypeSet()) || (!headerCore.isPlayerIdSet() && !headerCore.isPlayerIdTypeSet());
    BLAZE_ASSERT_COND_LOG(Log::USER, condition, "If PlayerId is set, PlayerIdType must also be set.");

    if (headerCore.getIsSession() && !headerCore.isSessionIdSet())
    {
        eastl::string pinSessionId;
        pinSessionId.sprintf("%" PRIu64, userSessionId);
        headerCore.setSessionId(pinSessionId.c_str());
    }

    if (!headerCore.isEventTimestampSet())
    {
        char8_t timeString[64];
        TimeValue::getTimeOfDay().toAccountString(timeString, sizeof(timeString), true);
        headerCore.setEventTimestamp(timeString);
    }

    if (!headerCore.isEventTypeSet())
        headerCore.setEventType(RiverPoster::PIN_EVENT_TYPE_BLAZE_SERVER);

    if (existenceData != nullptr)
    {
        // {{ (eliminate by: GOS-30204)
        if (!headerCore.isStepNumberSet())
        {
            // This synthetic step number generation is a temporary stop-gap while the EA Data Team upgrades their code to handle data-deduplication 
            // via existing 64-bit event timestamp.
            TimeValue creationTimeOffset = (TimeValue::getTimeOfDay() - existenceData->getCreationTime());
            if (creationTimeOffset > 0)
                headerCore.setStepNumber(static_cast<uint32_t>(creationTimeOffset.getMicroSeconds() / 100)); // NOTE: This gives 2.5 days of 100 microsec resolution, according to Dave O: "should be enough for everyone"
        }
        // }}

        if (!headerCore.isPlayerIdSet())
        {
            eastl::string playerId;
            playerId.sprintf("%" PRId64, existenceData->getBlazeId());
            headerCore.setPlayerId(playerId.c_str());
            headerCore.setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);
        }

        ClientType clientType = existenceData->getClientType();

        if (!headerCore.isPlatformSet())
        {
            if (clientType == CLIENT_TYPE_HTTP_USER)
            {
                headerCore.setPlatform(PINClientPlatformToString(RiverPoster::PINClientPlatform::web));
            }
            else if (clientType == CLIENT_TYPE_DEDICATED_SERVER)
            {
                headerCore.setPlatform(blazeClientPlatformTypeToPINClientPlatformString(gController->getDefaultClientPlatform()));
            }
            else
            {
                headerCore.setPlatform(blazeClientPlatformTypeToPINClientPlatformString(existenceData->getPlatformInfo().getClientPlatform()));
            }
        }

        if (!headerCore.isGameBuildVersionSet() && existenceData->getClientVersion()[0] != '\0')
        {
            headerCore.setGameBuildVersion(existenceData->getClientVersion());
        }

        if (existenceData->getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
        {
            char8_t buf[64] = "";
            blaze_snzprintf(buf, sizeof(buf), "%" PRIi64, existenceData->getPlatformInfo().getEaIds().getNucleusAccountId());
            RiverPoster::PINEventHeaderCore::AdditionalPlayerIdMapMap& idMap = headerCore.getAdditionalPlayerIdMap();
            idMap[RiverPoster::PIN_PLAYER_ID_TYPE_ACCOUNT].set(buf);
        }

        if ((has1stPartyPlatformInfo(existenceData->getPlatformInfo())) && ((clientType == CLIENT_TYPE_GAMEPLAY_USER) || (clientType == CLIENT_TYPE_LIMITED_GAMEPLAY_USER)))
        {
            ClientPlatformType platform = existenceData->getPlatformInfo().getClientPlatform();
            RiverPoster::PINEventHeaderCore::AdditionalPlayerIdMapMap& idMap = headerCore.getAdditionalPlayerIdMap();
            EA::TDF::TdfHashValue hash = EA::StdC::kCRC32InitialValue;
            if (platform == ClientPlatformType::xone || platform == ClientPlatformType::ps4 || platform == ClientPlatformType::xbsx || platform == ClientPlatformType::ps5 
                || platform == ClientPlatformType::steam || platform == ClientPlatformType::stadia)
            {
                // This should be okay, since we only want one external id (the active one)
                ExternalId extId = getExternalIdFromPlatformInfo(existenceData->getPlatformInfo());
                hash = EA::StdC::CRC32(&extId, sizeof(ExternalId), hash, false);
            }
            else if (platform == ClientPlatformType::nx)
                hash = EA::StdC::CRC32(existenceData->getPlatformInfo().getExternalIds().getSwitchId(), existenceData->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().length(), hash, false);

            char8_t buf[33] = "";
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu32, hash);
            switch (platform)
            {
            case xbsx:
            case xone:
                idMap["xuid_hash"].set(buf);
                break;
            case ps5:
            case ps4:
                idMap["psnid_hash"].set(buf);
                break;
            case nx:
                idMap["switchid_hash"].set(buf);
                break;
            case steam:
                idMap["steamid_hash"].set(buf);
                break; 
            case stadia:
                idMap["stadiaid_hash"].set(buf);
                break; 
            default:
                break;
            }
        }

        if (!headerCore.isDeviceLocaleSet() && (existenceData->getSessionLocale() != 0))
        {
            char8_t lang[3]; // 2 for the code and 1 for the terminator = 3
            char8_t ctry[3]; // 2 for the code and 1 for the terminator = 3
            LocaleTokenCreateLanguageString(lang, existenceData->getSessionLocale());
            LocaleTokenCreateCountryString(ctry, existenceData->getSessionLocale());
            eastl::string deviceLocaleString;
            deviceLocaleString.sprintf("%s_%s", lang, ctry);
            headerCore.setDeviceLocale(deviceLocaleString.c_str());
        }

        // try to get release type from the PIN config
        const char8_t* releaseType = getProductReleaseType(existenceData->getProductName());
        if (releaseType != nullptr)
        {
            headerCore.setReleaseType(releaseType);
        }

        uint32_t clientIp = existenceData->getClientIp();
        if (clientIp != 0 && !headerCore.isClientIpSet())
        {
            char8_t addrStr[Login::MAX_IPADDRESS_LEN] = { 0 };
            blaze_snzprintf(addrStr, sizeof(addrStr), "%d.%d.%d.%d", NIPQUAD(clientIp));
            headerCore.setClientIp(addrStr);
        }

        EA::TDF::GenericTdfType *headerCoreTdf = headerCore.getCustomDataMap().allocate_element();
        headerCoreTdf->set(existenceData->getSessionFlags().getIsChildAccount());
        headerCore.getCustomDataMap()[RiverPoster::PIN_UNDERAGE_FLAG] = headerCoreTdf;
    }
    else
    {
        // set some required fields if we don't have the data
        if (!headerCore.isPlayerIdSet())
        {
            headerCore.setPlayerId(RiverPoster::PIN_REQUIRED_FIELD_VALUE_UNKNOWN);
            headerCore.setPlayerIdType(RiverPoster::PIN_REQUIRED_FIELD_VALUE_UNKNOWN);
        }

        if (!headerCore.isPlatformSet())
        {
            if (!gController->isSharedCluster() || getConfig().getPINSettings().getUseServerPlaform())
                headerCore.setPlatform(blazeClientPlatformTypeToPINClientPlatformString(gController->getDefaultClientPlatform()));
            else
                headerCore.setPlatform(RiverPoster::PINClientPlatformToString(RiverPoster::PINClientPlatform::unknown));
        }

        EA::TDF::GenericTdfType *headerCoreTdf = headerCore.getCustomDataMap().allocate_element();
        headerCoreTdf->set(false);
        headerCore.getCustomDataMap()[RiverPoster::PIN_UNDERAGE_FLAG] = headerCoreTdf;
    }

    // title info
    if (!headerCore.isReleaseTypeSet() || (headerCore.getReleaseType()[0] == '\0'))
    {
        headerCore.setReleaseType(RiverPoster::PIN_REQUIRED_FIELD_VALUE_UNKNOWN);
    }
}

const char8_t* UserSessionManager::getProductReleaseType(const char8_t* productName) const
{
    // try to get release type from the PIN config
    const auto& productNamePINInfoPair = getConfig().getPINSettings().getProductNamesPINInfo().find(productName);
    if (productNamePINInfoPair != getConfig().getPINSettings().getProductNamesPINInfo().end())
    {
        return productNamePINInfoPair->second->getReleaseType();
    }

    BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getProductReleaseType: The product (" << productName << ") is not hosted by this instance. No release type info available.");
    return nullptr;
}

void UserSessionManager::getProjectIdPlatforms(const char8_t* projectId, ClientPlatformTypeList& platforms) const
{
    const auto findIt = mClientPlatformsByProjectIdMap.find(projectId);
    if (findIt != mClientPlatformsByProjectIdMap.end())
        findIt->second->copyInto(platforms);
}

void UserSessionManager::sendPINLoginFailureEvent(RiverPoster::LoginEventPtr loginEvent, UserSessionId userSessionId, const UserSessionExistenceData& existenceData)
{
    RiverPoster::PINEventHeaderCore &headerCore = loginEvent->getHeaderCore();
    filloutPINEventHeader(headerCore, userSessionId, &existenceData);

    RiverPoster::PINEventList events;
    EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinEvent = events.pull_back();
    pinEvent->set(*loginEvent);
    ClientPlatformType platform = gController->isSharedCluster() ? existenceData.getPlatformInfo().getClientPlatform() : gController->getDefaultClientPlatform();
    doSendPINEvent(platform, events);
}

RiverPoster::PINClientPlatform UserSessionManager::blazeClientPlatformTypeToPINClientPlatform(ClientPlatformType clientPlatformType)
{
    switch(clientPlatformType)
    {
    case steam:
    case pc:
        return RiverPoster::PINClientPlatform::pc;
    case xone:
        return RiverPoster::PINClientPlatform::xbox_one;
    case xbsx:
        return RiverPoster::PINClientPlatform::xbsx;
    case ps4:
        return RiverPoster::PINClientPlatform::ps4;
    case ps5:
        return RiverPoster::PINClientPlatform::ps5;
    case android:
        return RiverPoster::PINClientPlatform::android;
    case ios:
        return RiverPoster::PINClientPlatform::iOS;
    case stadia:
        return RiverPoster::PINClientPlatform::stadia;
    case nx:
    case mobile:
    case verizon:
    case facebook:
    case facebook_eacom:
    case bebo:
    case twitter:
    case friendster:
        return RiverPoster::PINClientPlatform::other;
    case NATIVE:
    case INVALID:
    case common:
    case legacyprofileid:
    default:
        return RiverPoster::PINClientPlatform::unknown;
    }
}

const char8_t* UserSessionManager::blazeClientPlatformTypeToPINClientPlatformString(ClientPlatformType clientPlatformType)
{
    if (clientPlatformType == nx)
        return "switch";

    return PINClientPlatformToString(blazeClientPlatformTypeToPINClientPlatform(clientPlatformType));
}

void UserSessionManager::doSendPINEvent(ClientPlatformType platform, RiverPoster::PINEventList& events)
{
    RiverPoster::PostPINEventRequestBodyPtr pinRequest = BLAZE_NEW RiverPoster::PostPINEventRequestBody;
    pinRequest->setEventList(events);

    if (!pinRequest->getEventList().empty())
    {
        // http headers
        pinRequest->setTaxonomyVersion(getConfig().getPINSettings().getPINTaxonomyVersion());
        pinRequest->setSdkType(RiverPoster::PIN_SDK_TYPE_BLAZE);

        // title info
        TitlePINInfoMap::const_iterator itr = getConfig().getPINSettings().getTitlePINInfo().find(platform);
        if (itr == getConfig().getPINSettings().getTitlePINInfo().end())
        {
            pinRequest->setTitleId(gController->getDefaultServiceName());
            pinRequest->setTitleIdType(RiverPoster::PIN_TITLE_ID_TYPE_COMMON);
        }
        else
        {
            pinRequest->setTitleId(itr->second->getTitleId());
            pinRequest->setTitleIdType(itr->second->getTitleIdType());
        }

        pinRequest->setEntityType(RiverPoster::PIN_ENTITY_TYPE_PLAYER);

        char8_t timeString[64];
        TimeValue::getTimeOfDay().toAccountString(timeString, sizeof(timeString), true);
        pinRequest->setPostTimestamp(timeString);

        // Encode event payload as JSON
        EA::TDF::JsonEncoder encoder;
        encoder.setIncludeTDFMetaData(false);

        // Strip out whitespace as it just inflates the payload size
        EA::TDF::JsonEncoder::FormatOptions opts;
        opts.options[EA::Json::JsonWriter::kFormatOptionIndentSpacing] = 0;
        opts.options[EA::Json::JsonWriter::kFormatOptionLineEnd] = 0;
        encoder.setFormatOptions(opts);

        EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        visitOpt.onlyIfSet = true;
#endif

        RawBufferPtr buf = BLAZE_NEW RawBuffer(BUFFER_SIZE_16K);
        RawBufferIStream istream(*buf);
        if (encoder.encode(istream, *pinRequest, nullptr, visitOpt))
        {
            if (Logger::isLogEventEnabled(Log::PINEVENT) && getConfig().getPINSettings().getEnablePersistentStorage())
            {
                // Write the events, which are encoded as JSON in buf (via istream), to the pinevent log file.
                // But first, make sure to add the new-line character.
                istream.Write("\n", 1);
                Logger::logEvent(Log::PINEVENT, reinterpret_cast<const char8_t*>(buf->data()), buf->datasize());
            }
            else
            {
                // Send the events directly to the River service
                mRiverPinUtil.sendPinData(buf);
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].logPINEvent: Failed to encode PIN event!");
        }

        if (buf->datasize() > 0)
        {
            const uint32_t eventCount = pinRequest->getEventList().size();
            mMetrics.mPinEventsSent.increment(eventCount);
            mMetrics.mPinEventBytesSent.increment(buf->datasize());
        }
    }
}

UserSessionId getUserSessionIdFromSessionUpdateMap(const UpdateExtendedDataRequest::UserOrSessionToUpdateMap::value_type& pair)
{
    return (UserSessionId) pair.first;
}

/*! ***************************************************************************/
/*!    \brief Does a bulk update on a set of users or sessions over a set of data values.

    \param request The data to update.
    \return Any error.
*******************************************************************************/
BlazeRpcError UserSessionManager::updateExtendedData(UpdateExtendedDataRequest &request) const
{
    UpdateExtendedDataRequest::UserOrSessionToUpdateMap::iterator it = request.getUserOrSessionToUpdateMap().begin();
    UpdateExtendedDataRequest::UserOrSessionToUpdateMap::iterator end = request.getUserOrSessionToUpdateMap().end();
    while (it != end)
    {
        // Remove the map to update whose data is empty.
        if (it->second == nullptr || it->second->getDataMap().empty())
        {
            
            it = request.getUserOrSessionToUpdateMap().erase(it);
        }
        else
            ++it;
    }

    BlazeRpcError rc = Blaze::ERR_OK;
    if (!request.getUserOrSessionToUpdateMap().empty())
    {
        it = request.getUserOrSessionToUpdateMap().begin();
        end = request.getUserOrSessionToUpdateMap().end();
        if (request.getIdsAreSessions())
        {
            rc = sendToMasters(it, end, &getUserSessionIdFromSessionUpdateMap, &UserSessionsMaster::updateExtendedDataMaster, request);
        }
        else
        {
            UserSessionIdList fullUserSessionIds;
            for (; it != end; ++it)
            {
                // getUserSessionIdsByBlazeId has a 10 (default) session limit, so we can't just reuse the list.
                UserSessionIdList userSessionIds;
                getUserSessionIdsByBlazeId(it->first, userSessionIds);

                UserSessionIdList::const_iterator curId = userSessionIds.begin();
                UserSessionIdList::const_iterator endId = userSessionIds.end();
                for (; curId != endId; ++curId)
                {                
                    fullUserSessionIds.push_back(*curId);
                }
            }
            rc = sendToMasters(fullUserSessionIds.begin(), fullUserSessionIds.end(), &UserSessionManager::fetchUserSessionId, &UserSessionsMaster::updateExtendedDataMaster, request);
        }
    }

    // Return OK if successful, or NO maps to update.
    return rc;
}

BlazeRpcError UserSessionManager::updateUserSessionServerAttribute(BlazeId blazeId, UpdateUserSessionAttributeRequest &request)
{
    UpdateUserSessionAttributeMasterRequest masterReq;
    masterReq.setReq(request);
    masterReq.setBlazeId(blazeId);

    UserSessionIdList userSessionIds;
    getUserSessionIdsByBlazeId(blazeId, userSessionIds);

    return sendToMasters(userSessionIds.begin(), userSessionIds.end(), &UserSessionManager::fetchUserSessionId, &UserSessionsMaster::updateUserSessionServerAttributeMaster, masterReq);
}

void UserSessionManager::removeUserFromNegativeCache(const char8_t* personaNamespace, const char8_t* personaName)
{
    NegativePersonaNameCacheByNamespaceMap::iterator it = mNegativePersonaNameCacheByNamespaceMap.find(personaNamespace);
    if (it != mNegativePersonaNameCacheByNamespaceMap.end())
    {
        NegativePersonaNameCache& negativePersonaNameCache = it->second;
        negativePersonaNameCache.remove(personaName);
    }
}

void UserSessionManager::onUserSubscriptionsUpdated(const NotifyUpdateUserSubscriptions& data, UserSession*)
{
    if (gController->getInstanceType() != ServerConfig::SLAVE)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].onUserSubscriptionsUpdated: "
            << "Only core slaves should receive user session subscriptions notifications.");
        return;
    }

    UserSessionMasterPtr targetUserSession = gUserSessionMaster->getLocalSession(data.getTargetUserSessionId());
    if (targetUserSession == nullptr)
    {
        // Do a quick sanity check...
        InstanceId targetInstanceId = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(data.getTargetUserSessionId()));
        if (targetInstanceId != gController->getInstanceId())
        {
            BLAZE_ASSERT_LOG(Log::USER, "UserSessionManager.onUserSubscriptionsUpdated: TargetUserSessionId(" << data.getTargetUserSessionId() << ") is not owned by this instance.  This should not happen.");
        }

        // The UserSession is no longer here due to logout/disconnect/etc. This is normal. Nothing to do, just return.
        return;
    }

    switch (data.getAction())
    {
    case ACTION_ADD:
        targetUserSession->addUsersAsync(data.getBlazeIds());
        break;
    case ACTION_REMOVE:
        targetUserSession->removeUsersAsync(data.getBlazeIds());
        break;
    }
}

void UserSessionManager::onSessionSubscriptionsUpdated(const NotifyUpdateSessionSubscriptions& data, UserSession*)
{
    if (gController->getInstanceType() != ServerConfig::SLAVE)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].onSessionSubscriptionUpdated: "
            << "Only core slaves should receive user session subscriptions notifications.");
        return;
    }

    UserSessionMasterPtr targetUserSession = gUserSessionMaster->getLocalSession(data.getTargetUserSessionId());
    if (targetUserSession == nullptr)
    {
        // Do a quick sanity check...
        InstanceId targetInstanceId = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(data.getTargetUserSessionId()));
        if (targetInstanceId != gController->getInstanceId())
        {
            BLAZE_ASSERT_LOG(Log::USER, "UserSessionManager.onSessionSubscriptionsUpdated: TargetUserSessionId(" << data.getTargetUserSessionId() << ") is not owned by this instance.  This should not happen.");
        }

        // The UserSession is no longer here due to logout/disconnect/etc. This is normal. Nothing to do, just return.
        return;
    }

    switch (data.getAction())
    {
    case ACTION_ADD:
        targetUserSession->addNotifiersAsync(data.getNotifierIds());
        break;
    case ACTION_REMOVE:
        targetUserSession->removeNotifiersAsync(data.getNotifierIds());
        break;
    }
}

RegisterRemoteSlaveSessionError::Error UserSessionManager::processRegisterRemoteSlaveSession(const Message* message)
{
    // this method registers a remote server instance, by creating a SlaveSession in the ConnectionManager.
    // currently this method is intentionally left blank.
    return RegisterRemoteSlaveSessionError::ERR_OK;
}

/*! **********************************************************************************************/
/*! \brief Check a session's connectivity to the client

    This method re-directs to the internal RPC hitting the slave with the local session.

    \param[in] userSessionId - The session to check
    \param[in] timeToWait - Relative timeout to wait for connectivity check

    \return Whether the session is connected or not
**************************************************************************************************/
bool UserSessionManager::checkConnectivity(UserSessionId userSessionId, const TimeValue& timeToWait)
{
    if (!getSessionExists(userSessionId))
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].checkConnectivity: no session for id " << userSessionId);
        return false;
    }

    RpcCallOptions opts;
    CheckConnectivityResponse response;
    CheckConnectivityRequest request;
    request.setUserSessionId(userSessionId);
    request.setConnectivityTimeout(timeToWait);
    opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, userSessionId);
    BlazeRpcError rc = UserSessionsSlave::checkConnectivity(request, response, opts);
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].checkConnectivity: RPC error "
            << (ErrorHelp::getErrorName(rc)) << " for id " << userSessionId);
        return false;
    }

    rc = (BlazeRpcError) response.getConnectivityResult();
    if (rc != ERR_OK)
    {
        if (rc != ERR_TIMEOUT)
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].checkConnectivity: unexpected result "
                << (ErrorHelp::getErrorName(rc)) << " for id " << userSessionId);
        }
        return false;
    }

    return true;
}

void UserSessionManager::onRemoteInstanceChanged(const RemoteServerInstance& instance)
{    
    if (!instance.hasComponent(UserSessionManager::COMPONENT_ID))
    {
        // if there isn't a usersessions component, don't listen
        return;
    }
    
    if ((instance.getInstanceType() == ServerConfig::SLAVE) ||
        (instance.getInstanceType() == ServerConfig::AUX_SLAVE))
    {
        // update the slave mapping if this is a SLAVE or AUX_SLAVE instance
        Selector::FiberCallParams params(Fiber::STACK_SMALL);
        gSelector->scheduleFiberCall(this, &UserSessionManager::updateSlaveMapping, instance.getInstanceId(), "UserSessionManager::updateSlaveMapping", params);
    }

    // we always grab the remote extended data registrations
    if (instance.isActive())
    {
        getRemoteUserExtendedDataProviderRegistration(instance.getInstanceId());
    }
    else if (!instance.isConnected())
    {
        // remove the instance's remote registrations
        refreshRemoteUserExtendedDataProviderRegistrations();
    }
}

void UserSessionManager::updateSlaveMapping(InstanceId instanceId)
{
    const RemoteServerInstance* instance = gController->getRemoteServerInstance(instanceId);
    if (instance == nullptr)
    {
        return;
    }

    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].updateSlaveMapping: "
        << gController->getInstanceName() << ":" << gController->getInstanceId() << " " 
        << (instance->isConnected() ? "connected to" : "disconnected from") << " remote " 
        << instance->getInstanceName() << ":" << instance->getInstanceId() << ".");

    // During the blocking call to registerRemoteSlaveSession the instance may be deleted from under our feet,
    // so cache off these parameters in order to be able to log about them in case of error
    char8_t instanceName[64];
    blaze_strnzcpy(instanceName, instance->getInstanceName(), sizeof(instanceName));

    // create a SlaveSession on the remote slave (both slaves and aux slaves do this)
    RpcCallOptions opts;
    opts.routeTo.setInstanceId(instanceId);
    BlazeRpcError rc = registerRemoteSlaveSession(opts);
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].updateSlaveMapping: "
            "failed to register a remote slave session " << gController->getInstanceName() 
            << ":" << gController->getInstanceId() << " -> " << instanceName
            << ":" << instanceId << ", err: " << (ErrorHelp::getErrorName(rc)) << ".");
        return;
    }
 
    //Refetch the instance after the call returns for extra safety
    instance = gController->getRemoteServerInstance(instanceId);
    if (instance == nullptr)
    {
        return;
    }

    // NOTE: Only core slave creates core->aux mappings on the master
    if (gController->getInstanceType() == ServerConfig::SLAVE &&
        instance->getInstanceType() == ServerConfig::AUX_SLAVE)
    {
        if (gUserSessionMaster != nullptr)
        {
            UpdateSlaveMappingRequest req;
            req.setCoreInstanceId(gController->getInstanceId());
            req.setAuxInstanceId(instance->getInstanceId());
            req.setAuxTypeIndex(instance->getInstanceTypeIndex());
            req.setRequestType(instance->isConnected() ? UpdateSlaveMappingRequest::MAPPING_CREATE : UpdateSlaveMappingRequest::MAPPING_REMOVE);
            req.setImmediatePartialLocalUserReplication(instance->getImmediatePartialLocalUserReplication());
            rc = gUserSessionMaster->updateSlaveMapping(req);
            if (rc != ERR_OK)
            {
                BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].updateSlaveMapping: "
                    "failed to update " << gController->getInstanceName() << ":" << gController->getInstanceId() 
                    << " -> " << instance->getInstanceName() << ":" << instance->getInstanceId() 
                    << " mapping on master, error: " << (ErrorHelp::getErrorName(rc)));
            }
        }
        else
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].updateSlaveMapping: "
                "failed to update " << gController->getInstanceName() << ":" << gController->getInstanceId() 
                << " -> " << instance->getInstanceName() << ":" << instance->getInstanceId() 
                << " mapping on master, master proxy is nullptr.");
        }
    }
}

BlazeRpcError UserSessionManager::updateBlazeObjectIdList(UpdateBlazeObjectIdInfoPtr info, BlazeId blazeId)
{
    UserSessionId sessionId = info->getSessionId();
    if (UserSession::isValidUserSessionId(sessionId))
    {
        if (getSessionExists(sessionId))
        {
            mUpdateBlazeObjectIdJobQueue.queueFiberJob<UserSessionManager, UpdateBlazeObjectIdInfoPtr>(
                this, &UserSessionManager::doUpdateBlazeObjectIdListByUserSessionId, info, "UserSessionManager::doUpdateBlazeObjectIdListByUserSessionId");
        }
    }
    else if (blazeId != INVALID_BLAZE_ID)
    {
        mUpdateBlazeObjectIdJobQueue.queueFiberJob<UserSessionManager, UpdateBlazeObjectIdInfoPtr, BlazeId>(
            this, &UserSessionManager::doUpdateBlazeObjectIdListByBlazeId, info, blazeId, "UserSessionManager::doUpdateBlazeObjectIdListByBlazeId");
    }

    return ERR_OK;
}

void UserSessionManager::doUpdateBlazeObjectIdListByUserSessionId(UpdateBlazeObjectIdInfoPtr updateInfo)
{
    UserSessionId userSessionId = updateInfo->getSessionId();;
    if (getSessionExists(userSessionId))
    {
        // Validate the user session actually exists before putting it into
        // a batch of updates. This is a safety net to avoid spam updates
        // if a mass user disconnect generates many object id update events
        // on remote usersessions that have already been erased.
        UpdateBlazeObjectIdListRequest request;
        request.setInfo(*updateInfo);
            
        RpcCallOptions opts;
        opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, userSessionId);
        BlazeRpcError rc = getMaster()->updateBlazeObjectIdList(request, opts);
        if (rc == Blaze::ERR_OK)
            mMetrics.mBlazeObjectIdListUpdateCount.increment();
    }
}

void UserSessionManager::doUpdateBlazeObjectIdListByBlazeId(UpdateBlazeObjectIdInfoPtr updateInfo, BlazeId blazeId)
{
    // build a list of sessions that will be affected by this send, and use that list to send a multitargeted RPC to all sharded masters
    UserSessionIdList userSessionIds;
    getUserSessionIdsByBlazeId(blazeId, userSessionIds);

    UpdateBlazeObjectIdListRequest request;
    for (UserSessionIdList::const_iterator it = userSessionIds.begin(), end = userSessionIds.end(); it != end; ++it)
    {
        UserSessionId userSessionId = *it;
        if (UserSession::isValidUserSessionId(userSessionId))
        {
            // Set the session id from the iterator because it's not set when updating by BlazeId
            updateInfo->setSessionId(userSessionId);

            request.setInfo(*updateInfo);

            RpcCallOptions opts;
            opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, userSessionId);
            BlazeRpcError rc = getMaster()->updateBlazeObjectIdList(request, opts);
            if (rc == Blaze::ERR_OK)
                mMetrics.mBlazeObjectIdListUpdateCount.increment();
        }
    }
}

/*! ***********************************************************************/
/*! \brief Associates a EA::TDF::ObjectId to a UserSession.

    \param[in] session - The session to update the data for
    \param[in] EA::TDF::ObjectId - EA::TDF::ObjectId to insert
***************************************************************************/
BlazeRpcError UserSessionManager::insertBlazeObjectIdForSession(UserSessionId sessionId, const EA::TDF::ObjectId& blazeObjectId)
{
    UpdateBlazeObjectIdInfoPtr updateInfo = BLAZE_NEW UpdateBlazeObjectIdInfo;
    updateInfo->setSessionId(sessionId);
    updateInfo->setBlazeObjectId(blazeObjectId);
    updateInfo->setAdd(true);
    
    return updateBlazeObjectIdList(updateInfo, INVALID_BLAZE_ID);
}

/*! ***********************************************************************/
/*! \brief Associates a EA::TDF::ObjectId to all user sessions for a user

    \param[in] blazeId - The blaze id to update.
    \param[in] EA::TDF::ObjectId - EA::TDF::ObjectId to insert
***************************************************************************/
BlazeRpcError UserSessionManager::insertBlazeObjectIdForUser(BlazeId blazeId, const EA::TDF::ObjectId& blazeObjectId)
{
    UpdateBlazeObjectIdInfoPtr updateInfo = BLAZE_NEW UpdateBlazeObjectIdInfo;
    updateInfo->setBlazeObjectId(blazeObjectId);
    updateInfo->setAdd(true);

    return updateBlazeObjectIdList(updateInfo, blazeId);
}

/*! ***********************************************************************/
/*! \brief Removes a EA::TDF::ObjectId from a user session.

    \param[in] session - The session to update the data for
    \param[in] EA::TDF::ObjectId - EA::TDF::ObjectId to remove
***************************************************************************/
BlazeRpcError UserSessionManager::removeBlazeObjectIdForSession(UserSessionId sessionId, const EA::TDF::ObjectId& blazeObjectId)
{    
    UpdateBlazeObjectIdInfoPtr updateInfo = BLAZE_NEW UpdateBlazeObjectIdInfo;
    updateInfo->setSessionId(sessionId);
    updateInfo->setBlazeObjectId(blazeObjectId);
    updateInfo->setAdd(false);

    return updateBlazeObjectIdList(updateInfo, INVALID_BLAZE_ID);
}

/*! ***********************************************************************/
/*! \brief Removes a EA::TDF::ObjectId from all sessions for a given user

    \param[in] blazeId - The id of the user to update
    \param[in] EA::TDF::ObjectId - EA::TDF::ObjectId to remove
***************************************************************************/
BlazeRpcError UserSessionManager::removeBlazeObjectIdForUser(BlazeId blazeId, const EA::TDF::ObjectId& blazeObjectId)
{
    UpdateBlazeObjectIdInfoPtr updateInfo = BLAZE_NEW UpdateBlazeObjectIdInfo;
    updateInfo->setBlazeObjectId(blazeObjectId);
    updateInfo->setAdd(false);

    return updateBlazeObjectIdList(updateInfo, blazeId);
}

/*! ***********************************************************************/
/*! \brief Removes all BlazeObjects of a type from a user session.

    \param[in] session - The session to update the data for
    \param[in] bobjType - ObjectType to remove
***************************************************************************/
BlazeRpcError UserSessionManager::removeBlazeObjectTypeForSession(UserSessionId sessionId, const EA::TDF::ObjectType& bobjType)
{
    UpdateBlazeObjectIdInfoPtr updateInfo = BLAZE_NEW UpdateBlazeObjectIdInfo;
    updateInfo->setSessionId(sessionId);
    updateInfo->setBlazeObjectId(EA::TDF::ObjectId(bobjType, 0));
    updateInfo->setAdd(false);

    return updateBlazeObjectIdList(updateInfo, INVALID_BLAZE_ID);
}

/*! ***********************************************************************/
/*! \brief Removes all BlazeObjects of a type from all sessions for a given user

    \param[in] blazeId - The id of the user to update
    \param[in] bobjType - ObjectType to remove
***************************************************************************/
BlazeRpcError UserSessionManager::removeBlazeObjectTypeForUser(BlazeId blazeId, const EA::TDF::ObjectType& bobjType)
{
    UpdateBlazeObjectIdInfoPtr updateInfo = BLAZE_NEW UpdateBlazeObjectIdInfo;
    updateInfo->setBlazeObjectId(EA::TDF::ObjectId(bobjType, 0));
    updateInfo->setAdd(false);

    return updateBlazeObjectIdList(updateInfo, blazeId);
}







/*!************************************************************************************************************************************************************/
/*!************************************************************************************************************************************************************/
/*!************************************************************************************************************************************************************/
/*!************************************************************************************************************************************************************/
/*!************************************************************************************************************************************************************/
/*!************************************************************************************************************************************************************/
/*!************************************************************************************************************************************************************/



/*!*********************************************************************************/
/*! \brief Looks up as set of users with the given Blaze Ids 
    
    This call will look up a set of users from the DB if it they are not already in the local or
    global user info cache.

    \param ids The user ids to fetch
    \param results Map of user ids to users.
    \param checkStatus Return only active users or active & non-active users.
************************************************************************************/
BlazeRpcError UserSessionManager::lookupUserInfoByBlazeIds(const BlazeIdVector& blazeIds, BlazeIdToUserInfoMap& results, bool ignoreStatus, bool isRepLagAcceptable, ClientPlatformType clientPlatformType /*= INVALID*/)
{
    VERIFY_WORKER_FIBER();

    if (blazeIds.empty())
        return ERR_OK;
    
    if (blazeIds.size() > getConfig().getUserCacheMax())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUsersById: lookup request size of " << blazeIds.size() << " exceeds max cache size of " << getConfig().getUserCacheMax() << ".  Increase max cache size.");
        return ERR_SYSTEM;
    }

    BlazeIdVector remainingIds;

    for (BlazeIdVector::const_iterator it = blazeIds.begin(), end = blazeIds.end(); it != end; ++it)
    {
        BlazeId blazeId = *it;

        if ((blazeId == INVALID_BLAZE_ID) || results.find(blazeId) != results.end())
            continue;

        UserInfoPtr userInfo = findResidentUserInfoByBlazeId(blazeId);
        if (userInfo != nullptr)
            results[blazeId] = userInfo;
        else
            remainingIds.push_back(blazeId);
    }
    
    BlazeRpcError err = ERR_OK;
    if (!remainingIds.empty())
    {
        UserInfoPtrList userInfoList;
        CountByPlatformMap metricsCountMap;

        err = UserInfoDbCalls::lookupUsersByIds(remainingIds, userInfoList, metricsCountMap, !ignoreStatus, isRepLagAcceptable, clientPlatformType);
        if (err == ERR_OK)
        {
            for (UserInfoPtrList::iterator it = userInfoList.begin(), end = userInfoList.end(); it != end; ++it)
            {
                UserInfoPtr& userInfo = *it;
                results[userInfo->getId()] = userInfo;
            }
        }

        for (const auto& entry : metricsCountMap)
            mMetrics.mUserLookupsByBlazeIdActualDbHits.increment(entry.second, entry.first);
        mMetrics.mUserLookupsByBlazeIdDbHits.increment(remainingIds.size());
    }

    mMetrics.mUserLookupsByBlazeId.increment(blazeIds.size());

    for (BlazeIdToUserInfoMap::iterator it = results.begin(), end = results.end(); it != end; ++it)
    {
        addUserInfoToIndices(it->second.get(), true);
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByBlazeId(BlazeId blazeId, UserInfoPtr& userInfo, bool ignoreStatus, bool isRepLagAcceptable, ClientPlatformType clientPlatformType /*= INVALID*/)
{
    BlazeIdToUserInfoMap results;
    BlazeRpcError err = lookupUserInfoByBlazeIds(BlazeIdVector(&blazeId, (&blazeId)+1), results, ignoreStatus, isRepLagAcceptable, clientPlatformType);
    if (err == ERR_OK)
    {
        if (!results.empty())
            userInfo = results.front().second;
        else
            err = USER_ERR_USER_NOT_FOUND;
    }

    return err;
}


BlazeRpcError UserSessionManager::lookupUserInfoByPlatformInfoVector(const PlatformInfoVector& platformInfoList, UserInfoPtrList& results, bool ignoreStatus)
{
    VERIFY_WORKER_FIBER();

    if (platformInfoList.empty())
        return ERR_OK;

    if (platformInfoList.size() > getConfig().getUserCacheMax())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUsersById: lookup request size of " << platformInfoList.size() << " exceeds max cache size of " << getConfig().getUserCacheMax() << ".  Increase max cache size.");
        return ERR_SYSTEM;
    }

    PlatformInfoVector remainingIds;

    for (const PlatformInfo& platformInfo : platformInfoList)
    {
        if (!has1stPartyPlatformInfo(platformInfo))
            continue;

        UserInfoPtr userInfo = findResidentUserInfoByPlatformInfo(platformInfo);
        if (userInfo != nullptr)
            results.push_back(userInfo);
        else
            remainingIds.push_back(platformInfo);
    }

    BlazeRpcError err = ERR_OK;
    if (!remainingIds.empty())
    {
        UserInfoPtrList userInfoList;
        CountByPlatformMap metricsCountMap;

        err = UserInfoDbCalls::lookupUsersByPlatformInfo(remainingIds, userInfoList, metricsCountMap, !ignoreStatus);
        if (err == ERR_OK)
        {
            for (UserInfoPtrList::iterator it = userInfoList.begin(), end = userInfoList.end(); it != end; ++it)
            {
                UserInfoPtr& userInfo = *it;
                results.push_back(userInfo);
            }
        }

        for (const auto& entry : metricsCountMap)
            mMetrics.mUserLookupsByExternalStringOrIdDbHits.increment(entry.second, entry.first);
    }

    for (const auto& platformInfo : platformInfoList)
        mMetrics.mUserLookupsByExternalStringOrId.increment(1, platformInfo.getClientPlatform());

    for (auto it : results)
    {
        addUserInfoToIndices(it.get(), true);
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPlatformInfo(const PlatformInfo& accountId, UserInfoPtr& userInfo, bool ignoreStatus)
{
    UserInfoPtrList results;
    BlazeRpcError err = lookupUserInfoByPlatformInfoVector(PlatformInfoVector(&accountId, (&accountId)+1), results, ignoreStatus);
    if (err == ERR_OK)
    {
        if (!results.empty())
            userInfo = results.front();
        else
            err = USER_ERR_USER_NOT_FOUND;
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByAccountIds(const AccountIdVector& accountIds, ClientPlatformType platform, UserInfoPtrList& results, bool /*ignoreStatus*/)
{
    VERIFY_WORKER_FIBER();

    if (accountIds.empty())
        return ERR_OK;

    AccountIdVector remainingAccountIds;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    UserInfoByAccountIdByPlatformMap::iterator platIt = mUsersByAccountId.find(platform);
    if (platIt == mUsersByAccountId.end())
    {
        remainingAccountIds.assign(accountIds.begin(), accountIds.end());
    }
    else
    {
        for (const auto& accountId : accountIds)
        {
            UserInfoByAccountIdMap::iterator uIt = platIt->second.find(accountId);
            if (uIt != platIt->second.end())
            {
                results.push_back(uIt->second);
                cachedPlatformInfo[uIt->second->getPlatformInfo().getEaIds().getNucleusAccountId()] = uIt->second->getPlatformInfo();
            }
            else
                remainingAccountIds.push_back(accountId);
        }
    }

    BlazeRpcError err = ERR_OK;
    if (!remainingAccountIds.empty())
    {
        UserInfoPtrList userInfoList;
        err = UserInfoDbCalls::lookupUsersByAccountIds(remainingAccountIds, platform, results, &cachedPlatformInfo);
        if (err == ERR_OK)
        {
            for (UserInfoPtrList::iterator it = userInfoList.begin(), end = userInfoList.end(); it != end; ++it)
            {
                UserInfoPtr& userInfo = *it;
                results.push_back(userInfo);
                addUserInfoToIndices(it->get(), true);
            }
        }
        mMetrics.mUserLookupsByAccountIdDbHits.increment(remainingAccountIds.size(), platform);
    }

    mMetrics.mUserLookupsByAccountId.increment(accountIds.size(), platform);
    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByAccountIds(const AccountIdVector& accountIds, UserInfoPtrList& results, bool /*ignoreStatus*/)
{
    VERIFY_WORKER_FIBER();

    if (accountIds.empty())
        return ERR_OK;

    BlazeRpcError retErr = ERR_OK;
    for (const auto& platform : gController->getHostedPlatforms())
    {
        BlazeRpcError err = lookupUserInfoByAccountIds(accountIds, platform, results);
        if (retErr == ERR_OK)
            retErr = err;
    }

    return retErr;
}

BlazeRpcError UserSessionManager::lookupUserInfoByAccountId(AccountId accountId, UserInfoPtrList& results, bool /*ignoreStatus*/)
{
    BlazeRpcError err = lookupUserInfoByAccountIds(AccountIdVector(&accountId, (&accountId)+1), results);
    if ((err == ERR_OK) && results.empty())
        err = USER_ERR_USER_NOT_FOUND;

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByAccountId(AccountId accountId, ClientPlatformType platform, UserInfoPtr& userInfo, bool /*ignoreStatus*/)
{
    UserInfoPtrList results;
    BlazeRpcError err = lookupUserInfoByAccountIds(AccountIdVector(&accountId, (&accountId) + 1), platform, results);
    if (err == ERR_OK)
    {
        if (!results.empty())
            userInfo = results.front();
        else
            err = USER_ERR_USER_NOT_FOUND;
    }

    return err;
}


BlazeRpcError UserSessionManager::lookupUserInfoByOriginPersonaIds(const OriginPersonaIdVector& originPersonaIds, ClientPlatformType platform, UserInfoPtrList& results, bool ignoreStatus)
{
    VERIFY_WORKER_FIBER();

    if (originPersonaIds.empty())
        return ERR_OK;

    if (originPersonaIds.size() > getConfig().getUserCacheMax())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUsersByOriginPersonaIds: lookup request size of " << originPersonaIds.size() << " exceeds max cache size of " << getConfig().getUserCacheMax() << ".  Increase max cache size.");
        return ERR_SYSTEM;
    }

    OriginPersonaIdVector remainingIds;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (OriginPersonaIdVector::const_iterator it = originPersonaIds.begin(), end = originPersonaIds.end(); it != end; ++it)
    {
        OriginPersonaId originPersonaId = *it;

        if (originPersonaId == INVALID_ORIGIN_PERSONA_ID)
            continue;

        UserInfoPtr userInfo = findResidentUserInfoByOriginPersonaId(originPersonaId, platform);
        if (userInfo != nullptr)
        {
            results.push_back(userInfo);
            cachedPlatformInfo[userInfo->getPlatformInfo().getEaIds().getNucleusAccountId()] = userInfo->getPlatformInfo();
        }
        else
            remainingIds.push_back(originPersonaId);
    }

    BlazeRpcError err = ERR_OK;
    if (!remainingIds.empty())
    {
        UserInfoPtrList userInfoList;

        err = UserInfoDbCalls::lookupUsersByOriginPersonaIds(remainingIds, platform, userInfoList, !ignoreStatus, &cachedPlatformInfo);
        if (err == ERR_OK)
        {
            for (UserInfoPtrList::iterator it = userInfoList.begin(), end = userInfoList.end(); it != end; ++it)
            {
                UserInfoPtr& userInfo = *it;
                results.push_back(userInfo);
                addUserInfoToIndices(it->get(), true);
            }
        }

        mMetrics.mUserLookupsByOriginPersonaIdDbHits.increment(remainingIds.size(), platform);
    }

    mMetrics.mUserLookupsByOriginPersonaId.increment(originPersonaIds.size(), platform);
    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByOriginPersonaId(OriginPersonaId originPersonaId, ClientPlatformType platform, UserInfoPtr& userInfo, bool ignoreStatus)
{
    UserInfoPtrList results;
    BlazeRpcError err = lookupUserInfoByOriginPersonaIds(OriginPersonaIdVector(&originPersonaId, (&originPersonaId)+1), platform, results, ignoreStatus);
    if (err == ERR_OK)
    {
        if (!results.empty())
            userInfo = results.front();
        else
            err = USER_ERR_USER_NOT_FOUND;
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaNames(const PersonaNameVector& personaNames, ClientPlatformType platform, PersonaNameToUserInfoMap& results, const char8_t* personaNamespace, bool ignoreStatus)
{
    VERIFY_WORKER_FIBER();

    // TODO_MC: Many of these check (including in other lookup functions) seem to be pointless because the number of records returned can be greater than the input
    if (personaNames.size() > getConfig().getUserCacheMax())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: lookup request size of " << personaNames.size() << " exceeds max cache size of " << getConfig().getUserCacheMax() << ".  Increase max cache size.");
        return ERR_SYSTEM;
    }

    if ((personaNamespace == nullptr) || (*personaNamespace == '\0'))
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: Missing persona namespace for request.  Namespace is required.");
        return ERR_SYSTEM;
    }

    if (blaze_strcmp(personaNamespace, gController->getNamespaceFromPlatform(platform)) != 0)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: Requested platform (" << ClientPlatformTypeToString(platform) << ") does not use requested namespace (" << personaNamespace <<
            "). Use lookupUserInfoByOriginPersonaNames to look up users by Origin persona name across all platforms.");
        return USER_ERR_USER_NOT_FOUND;
    }

    NegativePersonaNameCacheByNamespaceMap::iterator negCacheIt = mNegativePersonaNameCacheByNamespaceMap.find(personaNamespace);
    if (negCacheIt == mNegativePersonaNameCacheByNamespaceMap.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: personaNamespace (" << personaNamespace << ") is not supported.");
        return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_OK;
    NegativePersonaNameCache& negativePersonaNameCache = negCacheIt->second;

    PersonaNameVector remaining;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (PersonaNameVector::const_iterator it = personaNames.begin(), end = personaNames.end(); it != end; ++it)
    {
        const char8_t* personaName = *it;

        err = negativePersonaNameCache.shouldPerformLookup(personaName);
        if (err == ERR_OK)
        {
            UserInfoPtr userInfo = findResidentUserInfoByPersonaName(personaName, platform, personaNamespace);
            if (userInfo != nullptr)
            {
                cachedPlatformInfo[userInfo->getPlatformInfo().getEaIds().getNucleusAccountId()] = userInfo->getPlatformInfo();
                results[userInfo->getPersonaName()] = userInfo;
            }
            else
                remaining.push_back(personaName);
        }
    }

    if (!remaining.empty()) 
    {
        UserInfoPtrList userInfoList;
        err = UserInfoDbCalls::lookupUsersByPersonaNames(personaNamespace, remaining, platform, userInfoList, !ignoreStatus, &cachedPlatformInfo);
        if (err == ERR_OK)
        {
            for (UserInfoPtrList::iterator userInfoIt = userInfoList.begin(), end = userInfoList.end(); userInfoIt != end; ++userInfoIt)
            {
                UserInfoPtr& userInfo = *userInfoIt;
                results[userInfo->getPersonaName()] = userInfo;
            }
        }

        mMetrics.mUserLookupsByPersonaNameDbHits.increment(remaining.size(), platform);
    }

    mMetrics.mUserLookupsByPersonaName.increment(personaNames.size(), platform);

    for (PersonaNameToUserInfoMap::iterator it = results.begin(), end = results.end(); it != end; ++it)
    {
        addUserInfoToIndices(it->second.get(), true);
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaNames(const PersonaNameVector& personaNames, PersonaNameToUserInfoMap& results, const char8_t* /*personaNamespace*/, bool ignoreStatus)
{
    if (!gController->isSharedCluster())
        return lookupUserInfoByPersonaNames(personaNames, gController->getDefaultClientPlatform(), results, gController->getNamespaceFromPlatform(gController->getDefaultClientPlatform()), ignoreStatus);
    else
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: Missing platform for request. Use the api variant capable of passing platform.");
        return ERR_SYSTEM;
    }
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaName(const char8_t* personaName, ClientPlatformType platform, UserInfoPtr& userInfo, const char8_t* personaNamespace, bool ignoreStatus)
{
    PersonaNameToUserInfoMap results;
    BlazeRpcError err = lookupUserInfoByPersonaNames(PersonaNameVector(&personaName, (&personaName)+1), platform, results, personaNamespace, ignoreStatus);
    if (err == ERR_OK)
    {
        if (!results.empty())
            userInfo = results.front().second;
        else
            err = USER_ERR_USER_NOT_FOUND;
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaName(const char8_t* personaName, UserInfoPtr& userInfo, const char8_t* /*personaNamespace*/, bool ignoreStatus)
{
    if (!gController->isSharedCluster())
        return lookupUserInfoByPersonaName(personaName, gController->getDefaultClientPlatform(), userInfo, gController->getNamespaceFromPlatform(gController->getDefaultClientPlatform()), ignoreStatus);
    else
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaName: Missing platform for request. Use the api variant capable of passing platform.");
        return ERR_SYSTEM;
    }
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaNamesMultiNamespace(const PersonaNameVector& personaNames, UserInfoListByPersonaNameByNamespaceMap& results, bool primaryNamespaceOnly, bool restrictCrossPlatformResults, bool ignoreStatus)
{
    eastl::string callerNamespace;
    bool crossPlatformOptIn;
    const ClientPlatformSet* platformSet = &gController->getHostedPlatforms();
    if (restrictCrossPlatformResults)
    {
        BlazeRpcError err = setPlatformsAndNamespaceForCurrentUserSession(platformSet, callerNamespace, crossPlatformOptIn);
        if (err != ERR_OK)
            return err;
    }

    for (const auto& platform : *platformSet)
    {
        const char8_t* platformNamespace = gController->getNamespaceFromPlatform(platform);
        if (blaze_strcmp(platformNamespace, NAMESPACE_ORIGIN) == 0)
        {
            if (!primaryNamespaceOnly)
            {
                // If we're including users whose primary namespace isn't Origin, then we need to make a separate call to
                // lookupUserInfoByOriginPersonaNames (since a regular lookupUserInfoByPersonaNames call won't find all such users).
                // In this case, calling lookupUserInfoByPersonaNames for a platform that uses the Origin namespace is redundant, so we skip it.
                continue;
            }
        }
        else if (!callerNamespace.empty() && blaze_strcmp(platformNamespace, callerNamespace.c_str()) != 0)
        {
            // Skip lookups in namespaces the caller isn't permitted to search.
            continue;
        }

        PersonaNameToUserInfoMap lookupByPersonaNameResults;
        BlazeRpcError err = lookupUserInfoByPersonaNames(personaNames, platform, lookupByPersonaNameResults, platformNamespace, ignoreStatus);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.lookupUserInfoByPersonaNamesMultiNamespace: Error (" << ErrorHelp::getErrorName(err) << ") occurred when looking up user info by persona names in namespace '" << platformNamespace << "'");
        }
        UserInfoListByPersonaNameMap& usersByNameMap = results[platformNamespace];
        for (const auto& userIt : lookupByPersonaNameResults)
        {
            usersByNameMap[userIt.first].push_back(userIt.second);
        }
    }

    if (!primaryNamespaceOnly)
    {
        UserInfoListByPersonaNameMap& usersByOriginNameMap = results[NAMESPACE_ORIGIN];
        BlazeRpcError err = lookupUserInfoByOriginPersonaNames(personaNames, *platformSet, usersByOriginNameMap);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.lookupUserInfoByPersonaNamesMultiNamespace: Error (" << ErrorHelp::getErrorName(err) << ") occurred when looking up user info by persona names in Origin namespace");
        }
    }

    mMetrics.mUserLookupsByPersonaNameMultiNamespace.increment(personaNames.size());
    return ERR_OK;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaNameMultiNamespace(const char8_t* personaName, UserInfoListByPersonaNameByNamespaceMap& results, bool primaryNamespaceOnly, bool restrictCrossPlatformResults, bool ignoreStatus)
{
    BlazeRpcError err = lookupUserInfoByPersonaNamesMultiNamespace(PersonaNameVector(&personaName, (&personaName)+1), results, primaryNamespaceOnly, restrictCrossPlatformResults, ignoreStatus);
    if ((err == ERR_OK) && results.empty())
        err = USER_ERR_USER_NOT_FOUND;

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaNamePrefix(const char8_t* personaNamePrefix, ClientPlatformType platform, const uint32_t maxResultCount, PersonaNameToUserInfoMap& results, const char8_t* personaNamespace)
{
    VERIFY_WORKER_FIBER();

    if ((personaNamespace == nullptr) || (*personaNamespace == '\0'))
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: Missing persona namespace for request.  Namespace is required.");
        return ERR_SYSTEM;
    }

    if (blaze_strcmp(personaNamespace, gController->getNamespaceFromPlatform(platform)) != 0)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].lookupUserInfoByPersonaNames: Requested platform (" << ClientPlatformTypeToString(platform) << ") does not use requested namespace (" << personaNamespace <<
            "). Use lookupUserInfoByOriginPersonaNames to look up users by Origin persona name across all platforms.");
        return USER_ERR_USER_NOT_FOUND;
    }

    UserInfoPtrList userInfoList;
    BlazeRpcError err = UserInfoDbCalls::lookupUsersByPersonaNamePrefix(personaNamespace, personaNamePrefix, platform, maxResultCount, userInfoList);
    if (err == ERR_OK)
    {
        for (UserInfoPtrList::iterator it = userInfoList.begin(), end = userInfoList.end(); it != end; ++it)
        {
            UserInfoPtr& userInfo = *it;

            results[userInfo->getPersonaName()] = userInfo;
        }
    }

    mMetrics.mUserLookupsByPersonaNameDbHits.increment(1, platform);
    mMetrics.mUserLookupsByPersonaName.increment(1, platform);
    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByPersonaNamePrefixMultiNamespace(const char8_t* personaNamePrefix, const uint32_t maxResultCountPerPlatform, UserInfoListByPersonaNameByNamespaceMap& results, bool primaryNamespaceOnly, bool restrictCrossPlatformResults)
{
    VERIFY_WORKER_FIBER();

    eastl::string callerNamespace;
    bool crossPlatformOptIn;
    const ClientPlatformSet* platformSet = &gController->getHostedPlatforms();
    if (restrictCrossPlatformResults)
    {
        BlazeRpcError err = setPlatformsAndNamespaceForCurrentUserSession(platformSet, callerNamespace, crossPlatformOptIn);
        if (err != ERR_OK)
            return err;
    }

    BlazeRpcError retError = ERR_OK;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (const auto& platform : *platformSet)
    {
        const char8_t* platformNamespace = gController->getNamespaceFromPlatform(platform);
        if (blaze_strcmp(platformNamespace, NAMESPACE_ORIGIN) == 0)
        {
            if (!primaryNamespaceOnly)
            {
                // If we're including users whose primary namespace isn't Origin, then we need to make a separate call to
                // lookupUserInfoByOriginPersonaNamePrefix (since a regular lookupUsersByPersonaNamePrefixMultiNamespace call won't find all such users).
                // In this case, calling lookupUsersByPersonaNamePrefixMultiNamespace for a platform that uses the Origin namespace is redundant, so we skip it.
                continue;
            }
        }
        else if (!callerNamespace.empty() && blaze_strcmp(platformNamespace, callerNamespace.c_str()) != 0)
        {
            // Skip lookups in namespaces the caller isn't permitted to search.
            continue;
        }

        UserInfoPtrList userInfoList;
        retError = UserInfoDbCalls::lookupUsersByPersonaNamePrefixMultiNamespace(personaNamePrefix, platform, maxResultCountPerPlatform, userInfoList, &cachedPlatformInfo);
        if (retError != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.lookupUserInfoByPersonaNamePrefixMultiNamespace: Error (" << ErrorHelp::getErrorName(retError) << ") occurred when looking up user info by persona name prefix '" << personaNamePrefix << "'");
        }
        for (auto& userIt : userInfoList)
        {
            UserInfoListByPersonaNameMap& personaNameToUserInfoMap = results[userIt->getPersonaNamespace()];
            personaNameToUserInfoMap[userIt->getPersonaName()].push_back(userIt);
        }

        mMetrics.mUserLookupsByPersonaNameDbHits.increment(1, platform);
    }

    if (!primaryNamespaceOnly)
    {
        UserInfoListByPersonaNameMap& usersByOriginNameMap = results[NAMESPACE_ORIGIN];
        BlazeRpcError err = lookupUserInfoByOriginPersonaNamePrefix(personaNamePrefix, *platformSet, maxResultCountPerPlatform, usersByOriginNameMap);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.lookupUserInfoByPersonaNamePrefixMultiNamespace: Error (" << ErrorHelp::getErrorName(err) << ") occurred when looking up user info by Origin persona name prefix '" << personaNamePrefix << "'");
            if (retError == ERR_OK)
                retError = err;
        }
    }

    for (const auto& platform : *platformSet)
        mMetrics.mUserLookupsByPersonaName.increment(1, platform);
    return retError;
}

void UserSessionManager::findUserInfoByOriginPersonaNamesHelper(const ClientPlatformSet& unrestrictedPlatforms, const PersonaNameVector& names, UserInfoListByPersonaNameMap& results, PlatformInfoVector& platformInfoList, PersonaNameVector& remainingNames)
{
    PersonaMapByNamespaceMap::const_iterator personaItr = mPersonaMapByNamespaceMap.find(NAMESPACE_ORIGIN);
    for (const auto& name : names)
    {
        bool foundUser = false;
        if (personaItr != mPersonaMapByNamespaceMap.end())
        {
            UserInfoByPersonaMap::const_iterator userItr = personaItr->second.find(name);
            if (userItr != personaItr->second.end() && !userItr->second.empty())
            {
                foundUser = true;

                // All of our cached users should have up-to-date PlatformInfo.
                // Use the PlatformInfo to find out which additional users need to be looked up.
                ClientPlatformSet platformSet;
                const PlatformInfo& platformInfo = userItr->second.begin()->second->getPlatformInfo();
                getPlatformSetFromExternalIdentifiers(platformSet, platformInfo.getExternalIds());
                for (const auto& platIt : userItr->second)
                {
                    if (unrestrictedPlatforms.find(platIt.first) != unrestrictedPlatforms.end())
                        results[name].push_back(platIt.second);
                    platformSet.erase(platIt.first);
                }

                // We know which users we need to look up, so add them to the list of users to search by PlatformInfo
                for (auto& platform : platformSet)
                {
                    if (unrestrictedPlatforms.find(platform) == unrestrictedPlatforms.end())
                        continue;

                    PlatformInfo& newPlatformInfo = platformInfoList.push_back();
                    platformInfo.copyInto(newPlatformInfo);
                    newPlatformInfo.setClientPlatform(platform);
                }
            }
        }
        if (!foundUser)
            remainingNames.push_back(name);
    }
}

BlazeRpcError UserSessionManager::lookupUserInfoByOriginPersonaNames(const PersonaNameVector& personaNames, const ClientPlatformSet& platformSet, UserInfoListByPersonaNameMap& results)
{
    VERIFY_WORKER_FIBER();

    PlatformInfoVector platformInfoList;
    PersonaNameVector remainingNames;
    findUserInfoByOriginPersonaNamesHelper(platformSet, personaNames, results, platformInfoList, remainingNames);

    // First look up the users we've already cached PlatformInfo for
    UserInfoPtrList lookupByPlatformInfoResults;
    CountByPlatformMap metricsCountMap;
    BlazeRpcError retError = UserInfoDbCalls::lookupUsersByPlatformInfo(platformInfoList, lookupByPlatformInfoResults, metricsCountMap, true /*checkStatus*/, false /*refreshPlatformInfo*/);

    for (const auto& userIt : lookupByPlatformInfoResults)
        results[userIt->getPlatformInfo().getEaIds().getOriginPersonaName()].push_back(userIt.get());

    // Now look up the users we couldn't find in any of our local caches
    if (!remainingNames.empty())
    {
        BlazeIdsByPlatformMap blazeIdMap;
        BlazeRpcError err = AccountInfoDbCalls::getBlazeIdsByOriginPersonaNames(remainingNames, blazeIdMap);
        if (err == ERR_OK)
            err = getUserInfoFromBlazeIdMap(blazeIdMap, platformSet, UINT32_MAX, results);

        if (retError == ERR_OK)
            retError = err;

        mMetrics.mUserLookupsByOriginPersonaNameAccountInfoDbHits.increment(remainingNames.size());
    }

    for (const auto& entry : metricsCountMap)
        mMetrics.mUserLookupsByOriginPersonaNameUserInfoDbHits.increment(entry.second, entry.first);
    mMetrics.mUserLookupsByOriginPersonaName.increment(personaNames.size());
    return retError;
}

BlazeRpcError UserSessionManager::getUserInfoFromBlazeIdMap(const BlazeIdsByPlatformMap& blazeIdMap, const ClientPlatformSet& includedPlatforms, const uint32_t maxResultCountPerPlatform, UserInfoListByPersonaNameMap& usersByOriginName)
{
    BlazeIdVector blazeIds;
    for (const auto& platItr : blazeIdMap)
    {
        if (includedPlatforms.find(platItr.first) == includedPlatforms.end())
            continue;

        uint32_t resultCount = 0;
        for (const auto& userItr : platItr.second)
        {
            if (++resultCount > maxResultCountPerPlatform)
                break;

            blazeIds.push_back(userItr);
        }
    }

    if (blazeIds.empty())
        return ERR_OK;

    BlazeIdToUserInfoMap resultMap;
    BlazeRpcError err = lookupUserInfoByBlazeIds(blazeIds, resultMap);
    if (err == ERR_OK)
    {
        for (const auto& userItr : resultMap)
            usersByOriginName[userItr.second->getPlatformInfo().getEaIds().getOriginPersonaName()].push_back(userItr.second);
    }

    return err;
}

BlazeRpcError UserSessionManager::lookupUserInfoByOriginPersonaNamePrefix(const char8_t* personaNamePrefix, const ClientPlatformSet& platformSet, const uint32_t maxResultCountPerPlatform, UserInfoListByPersonaNameMap& results)
{
    VERIFY_WORKER_FIBER();

    BlazeIdsByPlatformMap blazeIdMap;
    BlazeRpcError err = AccountInfoDbCalls::getBlazeIdsByOriginPersonaNamePrefix(personaNamePrefix, blazeIdMap);
    if (err == ERR_OK)
        err = getUserInfoFromBlazeIdMap(blazeIdMap, platformSet, maxResultCountPerPlatform, results);

    mMetrics.mUserLookupsByOriginPersonaNameAccountInfoDbHits.increment();
    mMetrics.mUserLookupsByOriginPersonaName.increment();

    return err;
}

/*!*********************************************************************************/
/*! \brief Updates the user info attribute for a set of users with the given Blaze Ids 
    
    This call will update the user info attribute for a set of users from the DB

    \param ids The user ids to update
    \param attribute
    \param mask
    \param platform - if not INVALID, specifies the platform of the user(s) whose attributes will be updated
************************************************************************************/
Blaze::BlazeRpcError UserSessionManager::updateUserInfoAttribute(const BlazeIdVector &ids, UserInfoAttribute attribute, UserInfoAttributeMask mask, ClientPlatformType platform)
{
    VERIFY_WORKER_FIBER();

    BlazeRpcError err = ERR_OK;

    uint32_t rowsUpdated = 0;
    if (platform != ClientPlatformType::INVALID)
        err = UserInfoDbCalls::updateUserInfoAttribute(ids, platform, attribute, mask, rowsUpdated);
    else
        err = UserInfoDbCalls::updateUserInfoAttribute(ids, attribute, mask);

    if (err == ERR_OK)
    {
        for (BlazeIdVector::const_iterator it = ids.begin(), end = ids.end(); it != end; ++it)
        {
            broadcastUserInfoUpdatedNotification(*it);
        }
    }

    return err;
}


/*!*********************************************************************************/
/*! \brief Gets the user with the given Blaze Id 
    
    This call returns immediately and only operates on the data in the local cache.
    To search all users in the system, use the "lookup" version.

    \param id The Blaze id of the user to fetch
    \return The user, or nullptr if the user is not present in the manager. 
************************************************************************************/
UserInfo* UserSessionManager::findResidentUserInfoByBlazeId(BlazeId blazeId)
{
    UserInfoByBlazeIdMap::iterator it = mUsersByBlazeId.find(blazeId);
    return (it != mUsersByBlazeId.end() ? it->second : nullptr);
}

UserInfo* UserSessionManager::findResidentUserInfoByPersonaName(const char8_t* personaName, ClientPlatformType platform, const char8_t* personaNamespace)
{
    const UserInfoByPersonaMap* map = nullptr;
    if (personaNamespace != nullptr)
    {
        PersonaMapByNamespaceMap::const_iterator it = mPersonaMapByNamespaceMap.find(personaNamespace);
        if (it != mPersonaMapByNamespaceMap.end())
            map = &(it->second);
    }

    if (map != nullptr)
    {
        UserInfoByPersonaMap::const_iterator it = map->find(personaName);
        if (it != map->end())
        {
            UserInfoByPlatform::const_iterator userIt = it->second.find(platform);
            if (userIt != it->second.end())
                return userIt->second;
        }
    }

    return nullptr;
}

UserInfo* UserSessionManager::findResidentUserInfoByPlatformInfo(const PlatformInfo& platformInfo)
{
    switch (platformInfo.getClientPlatform())
    {
    case ps4:
    case ps5:
        if (platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
        {
            UserInfoByPsnAccountIdMap::iterator it = mUsersByPsnAccountId.find(platformInfo.getExternalIds().getPsnAccountId());
            if (it != mUsersByPsnAccountId.end())
                return it->second;
        }
        return nullptr;
    case xone:
    case xbsx:
        if (platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
        {
            UserInfoByXblAccountIdMap::iterator it = mUsersByXblAccountId.find(platformInfo.getExternalIds().getXblAccountId());
            if (it != mUsersByXblAccountId.end())
                return it->second;
        }
        return nullptr;
    case nx:
        if (!platformInfo.getExternalIds().getSwitchIdAsTdfString().empty())
        {
            UserInfoBySwitchIdMap::iterator it = mUsersBySwitchId.find(platformInfo.getExternalIds().getSwitchId());
            if (it != mUsersBySwitchId.end())
                return it->second;
        }
        return nullptr;
    case pc:
        if (platformInfo.getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
        {
            return findResidentUserInfoByAccountId(platformInfo.getEaIds().getNucleusAccountId(), pc);
        }
        return nullptr;
    case steam:
        if (platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        {
            UserInfoBySteamAccountIdMap::iterator it = mUsersBySteamAccountId.find(platformInfo.getExternalIds().getSteamAccountId());
            if (it != mUsersBySteamAccountId.end())
                return it->second;
        }
        return nullptr; 
    case stadia:
        if (platformInfo.getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
        {
            UserInfoByStadiaAccountIdMap::iterator it = mUsersByStadiaAccountId.find(platformInfo.getExternalIds().getStadiaAccountId());
            if (it != mUsersByStadiaAccountId.end())
                return it->second;
        }
        return nullptr; 
    default:
        return nullptr;
    }
}

UserInfo* UserSessionManager::findResidentUserInfoByOriginPersonaId(OriginPersonaId originPersonaId, ClientPlatformType platform)
{
    UserInfoByOriginPersonaIdByPlatformMap::iterator it = mUsersByOriginPersonaId.find(platform);
    if (it != mUsersByOriginPersonaId.end())
    {
        UserInfoByOriginPersonaIdMap::iterator uit = it->second.find(originPersonaId);
        if (uit != it->second.end())
            return uit->second;
    }
    return nullptr;
}

UserInfo* UserSessionManager::findResidentUserInfoByAccountId(AccountId accountId, ClientPlatformType platform)
{
    UserInfoByAccountIdByPlatformMap::iterator it = mUsersByAccountId.find(platform);
    if (it != mUsersByAccountId.end())
    {
        UserInfoByAccountIdMap::iterator uit = it->second.find(accountId);
        if (uit != it->second.end())
            return uit->second;
    }
    return nullptr;
}

UserSessionId UserSessionManager::getPrimarySessionId(BlazeId id) 
{
    UserSessionId sessionId = INVALID_USER_SESSION_ID;
    PrimaryExistenceByBlazeIdMap::const_iterator it = mPrimaryExistenceByBlazeIdMap.find(id);
    if (it != mPrimaryExistenceByBlazeIdMap.end() && !it->second.empty())
    {
        const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(it->second.front());
        sessionId = userSession.getUserSessionId();
    }
    return sessionId;
}

/*! ***************************************************************************/
/*!    \brief Returns blaze id given a session id

    \param id The session id.
    \return  BlazeId of the user.
*******************************************************************************/
BlazeId UserSessionManager::getBlazeId(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getBlazeId();
    return INVALID_BLAZE_ID;
}

/*! ***************************************************************************/
/*!    \brief Returns external id given a session id

    \param id The session id.
    \return  BlazeId of the user.
*******************************************************************************/
const PlatformInfo& UserSessionManager::getPlatformInfo(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getPlatformInfo();

    static PlatformInfo dummyPlatformInfo;
    return dummyPlatformInfo;
}



/*! ***************************************************************************/
/*!    \brief Returns account id given a session id

    \param id The session id.
    \return  Account Id of the user.
*******************************************************************************/
AccountId UserSessionManager::getAccountId(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getAccountId();

    return INVALID_ACCOUNT_ID;
}

/*! ***************************************************************************/
/*!    \brief Returns Account country given a session id

    \param id The session id.
    \return  Account country of the user.
*******************************************************************************/
uint32_t UserSessionManager::getAccountCountry(UserSessionId id)
{
    UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getAccountCountry();

    return 0;
}

const char8_t* UserSessionManager::getPersonaName(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getPersonaName();
    return nullptr;
}

const char8_t* UserSessionManager::getPersonaNamespace(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getPersonaNamespace();
    return nullptr;
}

const char8_t* UserSessionManager::getUniqueDeviceId(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getUniqueDeviceId();
    return nullptr;
}

DeviceLocality UserSessionManager::getDeviceLocality(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getDeviceLocality();
    return DEVICELOCALITY_UNKNOWN;
}

uint32_t UserSessionManager::getClientAddress(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getClientIp();
    return 0;
}

/*! ***************************************************************************/
/*!    \brief Returns Account locale given a session id

    \param id The session id.
    \return  Locale of the user.
*******************************************************************************/
Locale UserSessionManager::getAccountLocale(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getAccountLocale();

    return 0;
}

/*! ***************************************************************************/
/*!    \brief Returns Account locale given a session id

    \param id The session id.
    \return  Locale of the user.
*******************************************************************************/
Locale UserSessionManager::getSessionLocale(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getSessionLocale();

    return 0;
}

/*! ***************************************************************************/
/*!    \brief Returns client type given a session id

    \param id The session id.
    \return  ClientType of the user session.
*******************************************************************************/
ClientType UserSessionManager::getClientType(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getClientType();

    return CLIENT_TYPE_INVALID;
}

/*! ***************************************************************************/
/*!    \brief Returns proto tunnel version given a session id

    \param id The session id.
    \return  proto tunnel version (char8_t*) given a session id
*******************************************************************************/
const char8_t* UserSessionManager::getProtoTunnelVer(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getProtoTunnelVer();
    return nullptr;
}

/*! ***************************************************************************/
/*!    \brief Returns client version given a session id

    \param id The session id.
    \return  client version (char8_t*) given a session id
*******************************************************************************/
const char8_t* UserSessionManager::getClientVersion(UserSessionId id)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        return userSession->getExistenceData().getClientVersion();
    return nullptr;
}

/*! ***************************************************************************/
/*!    \brief Returns service name of given a session id

    \param id The session id.
*******************************************************************************/
void UserSessionManager::getServiceName(UserSessionId id, ServiceName& serviceName)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        serviceName = userSession->getServiceName();
}

void UserSessionManager::getProductName(UserSessionId id, ProductName& productName)
{
    const UserSessionPtr userSession = getSession(id);
    if (userSession != nullptr)
        productName = userSession->getProductName();
}
bool UserSessionManager::isSessionAuthorized(UserSessionId id, const Authorization::Permission& permission, bool suppressWarnLog)
{
    bool authorized = false;
    if (mGroupManager != nullptr)
    {
        const UserSessionPtr userSession = getSession(id);
        if (userSession != nullptr)
        {
            const char8_t* authGroupName = userSession->getAuthGroupName();
            const Authorization::Group* aAuthGroup = mGroupManager->getGroup(authGroupName);
            if (aAuthGroup != nullptr)
            {
                authorized = aAuthGroup->isAuthorized(permission);

                // The suppressWarnLog param is used only as an indication that the caller would like this function to generate a WARN log if
                // the authorization check fails.  This is really just to avoid copy/pasting log entries, and having to gather the info this function figures out on its own.
                if (authorized == false && suppressWarnLog == false)
                {
                    BLAZE_WARN_LOG(Log::USER, "[UserSession.isAuthorized] - the access group (" << authGroupName << ") the session (" 
                        << id << ") belongs to does not have (" << (ComponentBaseInfo::getPermissionNameById(permission)) << ") permission.");
                }
            }
            else
            {
                if (authGroupName[0] == '\0')
                {
                    //empty AuthGroupName means that the user failed ip check default group
                    //log a less severe message in case of whitelist failure
                    BLAZE_TRACE_LOG(Log::USER,"UserSession(:" << id<<").isSessionAuthorized: Failed to look up authorization group("
                        <<authGroupName <<"). Access denied, failed IP whitelist check. Requested Permission("
                        << static_cast<int64_t>(permission) << ").");
                }
                else 
                {
                    //AuthGroupName stored should alway be valid. log an error. 
                    BLAZE_WARN_LOG(Log::USER,"UserSession(:" << id << ").isAuthorized: Failed to look up authorization group(" 
                        << authGroupName << "). Please check config. Access denied. Requested Permission(" 
                        << static_cast<int64_t>(permission) << "). ***** Please check config *****");
                }
            }
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::USER,"UserSession(" << id << ").isAuthorized] Can't get GroupManager. Access denied.");
    }
    return authorized;
}

/*! ***************************************************************************/
/*!    \brief Returns whether the user is online or not.

    \param id The users id.
    \return  True if the user has any active online sessions, false otherwise.
*******************************************************************************/
bool UserSessionManager::isUserOnline(BlazeId blazeId)
{
    ExistenceByBlazeIdMap::const_iterator eit = mExistenceByBlazeIdMap.find(blazeId);
    // if at least one session is found, the user is considered online
    return (eit != mExistenceByBlazeIdMap.end() && !eit->second.empty());
}

/*! ***************************************************************************/
/*!    \brief Returns whether there is at least one online user linked to the
              given Nucleus account.
*******************************************************************************/
bool UserSessionManager::isAccountOnline(AccountId accountId)
{
    ExistenceByAccountIdMap::const_iterator eit = mExistenceByAccountIdMap.find(accountId);
    return (eit != mExistenceByAccountIdMap.end() && !eit->second.empty());
}

/*! ******************************************************************************************/
/*!    \brief Populates the provided BlazeIdSet with the BlazeIds of 
              all online users linked to the specified Nucleus account.
       \return The number of BlazeIds inserted into the provided BlazeIdSet
*********************************************************************************************/
uint32_t UserSessionManager::getOnlineBlazeIdsByAccountId(AccountId accountId, BlazeIdSet& blazeIds)
{
    uint32_t origSize = blazeIds.size();
    ExistenceByAccountIdMap::const_iterator eit = mExistenceByAccountIdMap.find(accountId);
    if (eit != mExistenceByAccountIdMap.end())
    {
        for (const auto& sessionIt : eit->second)
        {
            const UserSessionExistence& userSession = static_cast<const UserSessionExistence&>(sessionIt);
            blazeIds.insert(userSession.getBlazeId());
        }
    }
    return blazeIds.size() - origSize;
}

RequestUserExtendedDataProviderRegistrationError::Error 
    UserSessionManager::processRequestUserExtendedDataProviderRegistration(
    UpdateUserExtendedDataProviderRegistrationResponse &response,
    const Blaze::Message *message)
{
    // reserve space in the response
    response.getComponentIds().reserve(mDataProviders.size());

    // we send out the set of our local data providers to the requestor
    DataProvidersByComponent::const_iterator it = mDataProviders.begin();
    DataProvidersByComponent::const_iterator end = mDataProviders.end();
    for (; it != end; ++it)
    {
        response.getComponentIds().push_back(it->first);
    }

    response.setHostInstanceId(gController->getInstanceId());

    // send out the set of local extended data ids
    mUserExtendedDataIdMap.copyInto(response.getDataIdMap());

    return RequestUserExtendedDataProviderRegistrationError::ERR_OK;
}

/*! ***********************************************************************/
/*! \brief Registers an object as a user session extended data provider

    \param[in] componentId - the component id of the object to register
    \param[in] provider - the class instance providing the user session extended data
***************************************************************************/
bool UserSessionManager::registerExtendedDataProvider(ComponentId componentId, ExtendedDataProvider& provider)
{
    if (mDataProviders.find(componentId) != mDataProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].registerExtendedDataProvider: extended data provider already registered with component " << componentId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].registerExtendedDataProvider: added extended data provider for component(" << componentId << "): " << &provider);

    mMetrics.mGaugeUserExtendedDataProviders.increment();
    mDataProviders[componentId] = &provider;
    mExtendedDataDispatcher.addDispatchee(provider);

    RemoteUserExtendedDataProviderSet::iterator it = mRemoteUEDProviders.find(componentId);
    if (it != mRemoteUEDProviders.end())
    {
        // we have this provider in remote list
        // because it was registered on some other slave before the same
        // component registered it on this slave, so yank it out of remote list
        mRemoteUEDProviders.erase(it);
    }

    return true;
}

/*! ***********************************************************************/
/*! \brief deregisters an object as a user session extended data provider

    \param[in] componentId - the component id of the object to register
    /return - true of the operation was successful, false otherwise
***************************************************************************/
bool UserSessionManager::deregisterExtendedDataProvider(ComponentId componentId)
{
    DataProvidersByComponent::iterator it = mDataProviders.find(componentId);
    if (it == mDataProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionManager].deregisterExtendedDataProvider: extended data provider not registered with for component " << componentId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].deregisterExtendedDataProvider: remove extended data provider for component(" << componentId << ")");

    mMetrics.mGaugeUserExtendedDataProviders.decrement();
    mExtendedDataDispatcher.removeDispatchee(*it->second);
    mDataProviders.erase(it);

    return true;
}

ExtendedDataProvider* UserSessionManager::getLocalUserExtendedDataProvider(ComponentId componentId) const
{
    DataProvidersByComponent::const_iterator it = mDataProviders.find(componentId);
    if (it == mDataProviders.end())
        return nullptr;

    return it->second;
}

ValidateSessionKeyError::Error UserSessionManager::processValidateSessionKey(
    const ValidateSessionKeyRequest &request, SessionInfo &response, const Message *message)
{
    // TODO_MC: How is this RPC intended to be used?  It only works if you hit the owning instance.
    if (gUserSessionMaster != nullptr)
    {
        const char8_t* sessionKey = request.getSessionKey();
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSessionBySessionKey(sessionKey);
        if (userSession != nullptr)
        {
            response.setSessionKey(sessionKey);
            response.setBlazeId(userSession->getUserInfo().getId());
            response.setAccountId(userSession->getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId());
            response.setDisplayName(userSession->getUserInfo().getPersonaName());

            return ValidateSessionKeyError::ERR_OK;
        }
    }

    return ValidateSessionKeyError::USER_ERR_SESSION_NOT_FOUND;
}  /*lint !e1961 - Don't make the function to const because its parent class is generated from template file */


/*!*********************************************************************************/
/*! \brief Returns true if the given login key is valid. 
************************************************************************************/
//const UserSessionData* UserSessionManager::getLocalSessionBySessionKey(const char8_t *sessionKey)
//{
//    //First parse the key into its parts
//    char8_t* keyHashSum = nullptr;
//    UserSessionId sessionId = INVALID_USER_SESSION_ID;
//    if (!getUserSessionIdAndHashSumFromSessionKey(sessionKey, sessionId, keyHashSum))
//        return nullptr;
//
//    UserSessionPtr session = gUserSessionMaster->getLocalSession(sessionId);
//    if ((session == nullptr) || !session->validateKeyHashSum(keyHashSum))
//        return nullptr;
//
//    return session.get();
//}

bool UserSessionManager::getUserSessionIdAndHashSumFromSessionKey(const char8_t *sessionKey, UserSessionId& sessionId, char8_t*& hashSum)
{
    if ((sessionKey == nullptr) || (*sessionKey == '\0'))
        return false;

    sessionId = EA::StdC::StrtoU64(sessionKey, &hashSum, 16);
    if ((hashSum == nullptr) || (*hashSum != '_'))
        return false;

    ++hashSum;

    return true;
}

UserSessionId UserSessionManager::getUserSessionIdFromSessionKey(const char8_t *sessionKey) const
{
    char8_t* hashSum = nullptr;
    UserSessionId sessionId;
    if (getUserSessionIdAndHashSumFromSessionKey(sessionKey, sessionId, hashSum))
        return sessionId;
    
    return INVALID_USER_SESSION_ID;
}

BlazeRpcError UserSessionManager::getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results)
{
    BlazeIdToUserInfoMap userInfoMap;
    BlazeRpcError err = lookupUserInfoByBlazeIds(keys, userInfoMap, false);
    if (err == ERR_OK)
    {
        for (BlazeIdToUserInfoMap::const_iterator iter = userInfoMap.begin(); iter != userInfoMap.end(); ++iter)
        {
            const UserInfo* info = iter->second.get();
            IdentityInfo* identity = results.allocate_element();
            results[info->getId()] = identity;
            identity->setBlazeObjectId(info->getBlazeObjectId());
            identity->setIdentityName(info->getIdentityName());
        }
    }
    return err;
}

BlazeRpcError UserSessionManager::getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& entityName, IdentityInfo& result)
{
    if (bobjType != ENTITY_TYPE_USER)
    {
        return Blaze::ERR_NOT_SUPPORTED; 
    }

    // [GOS-32202] getEntityByName doesn't work for shared cluster (persona name alone doesn't uniquely identify a user)
    if (gController->isSharedCluster())
        return ERR_SYSTEM;

    UserInfoPtr info;
    ClientPlatformType platform = gController->getDefaultClientPlatform();
    BlazeRpcError err = lookupUserInfoByPersonaName(entityName, platform, info, gController->getNamespaceFromPlatform(platform));
    if (err == Blaze::ERR_OK)
    {
        result.setBlazeObjectId(info->getBlazeObjectId());
        result.setIdentityName(info->getIdentityName());
    }
    else
    {
        err = Blaze::ERR_ENTITY_NOT_FOUND;
    } 
    return err;
}

/* \brief return server metrics 
    For a full description of the metrics for this health check, see
    https://docs.developer.ea.com/display/blaze/Blaze+Health+Check+Definitions
*/
void UserSessionManager::getStatusInfo(ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();

    int i, j;
    char8_t key[128];
    char8_t value[64];

    //Total
    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByBlazeId.getTotal());
    map["TotalUserLookupsByBlazeId"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByBlazeIdDbHits.getTotal());
    map["TotalUserLookupsByBlazeIdDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByBlazeIdActualDbHits.getTotal());
    map["TotalUserLookupsByBlazeIdActualDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByExternalStringOrId.getTotal());
    map["TotalUserLookupsByExternalStringOrId"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByExternalStringOrIdDbHits.getTotal());
    map["TotalUserLookupsByExternalStringOrIdDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByAccountId.getTotal());
    map["TotalUserLookupsByAccountId"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByOriginPersonaId.getTotal());
    map["TotalUserLookupsByOriginPersonaId"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByOriginPersonaName.getTotal());
    map["TotalUserLookupsByOriginPersonaName"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByAccountIdDbHits.getTotal());
    map["TotalUserLookupsByAccountIdDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByOriginPersonaIdDbHits.getTotal());
    map["TotalUserLookupsByOriginPersonaIdDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByOriginPersonaNameUserInfoDbHits.getTotal());
    map["TotalUserLookupsByOriginPersonaNameUserInfoDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByOriginPersonaNameAccountInfoDbHits.getTotal());
    map["TotalUserLookupsByOriginPersonaNameAccountInfoDbHits"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByPersonaName.getTotal());
    map["TotalUserLookupsByPersonaName"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByPersonaNameMultiNamespace.getTotal());
    map["TotalUserLookupsByPersonaNameMultiNamespace"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUserLookupsByPersonaNameDbHits.getTotal());
    map["TotalUserLookupsByPersonaNameDbHits"] = value;

    mMetrics.mUserLookupsByPersonaName.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByPersonaName", value.getTotal());
    });

    mMetrics.mUserLookupsByExternalStringOrId.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByExternalStringOrId", value.getTotal());
    });

    mMetrics.mUserLookupsByAccountId.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByAccountId", value.getTotal());
    });

    mMetrics.mUserLookupsByOriginPersonaId.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByOriginPersonaId", value.getTotal());
    });

    mMetrics.mUserLookupsByBlazeIdActualDbHits.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByBlazeIdActualDbHits", value.getTotal());
    });

    mMetrics.mUserLookupsByPersonaNameDbHits.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByPersonaNameDbHits", value.getTotal());
    });

    mMetrics.mUserLookupsByExternalStringOrIdDbHits.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByExternalStringOrIdDbHits", value.getTotal());
    });

    mMetrics.mUserLookupsByAccountIdDbHits.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByAccountIdDbHits", value.getTotal());
    });

    mMetrics.mUserLookupsByOriginPersonaIdDbHits.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByOriginPersonaIdDbHits", value.getTotal());
    });

    mMetrics.mUserLookupsByOriginPersonaNameUserInfoDbHits.iterate([&map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        UserSessionsManagerMetrics::addPerPlatformStatusInfo(map, tagList, "TotalUserLookupsByOriginPersonaNameUserInfoDbHits", value.getTotal());
    });

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mDisconnections.getTotal());
    map["TotalDisconnections"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mBlazeObjectIdListUpdateCount.getTotal());
    map["TotalBlazeObjectIdListUpdateCount"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mUEDMessagesCoallesced.getTotal());
    map["TotalUEDMessagesCoallesced"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mGeoIpLookupsFailed.getTotal());
    map["TotalGeoIpLookupsFailed"] = value;
    
    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mGaugeUsers.get());
    map["GaugeUsers"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mGaugeUserExtendedDataProviders.get());
    map["GaugeUserExtendedDataProviders"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mGaugeUserSessionSubscribers.get());
    map["GaugeUserSessionSubscribers"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mReputationLookups.getTotal());
    map["TotalReputationLookups"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mReputationLookupsWithValueChange.getTotal());
    map["TotalReputationLookupsWithValueChange"] = value;


    //Gauge
    if (gUserSessionMaster != nullptr)
    {
        Metrics::SumsByTagValue typeModeStatusSums;
        gUserSessionMaster->mLocalUserSessionCounts.aggregate({ Metrics::Tag::client_type, Metrics::Tag::client_status, Metrics::Tag::client_mode }, typeModeStatusSums);
        eastl::map<eastl::string, uint64_t> countByClientType;
        for(auto& entry : typeModeStatusSums)
        {
            blaze_snzprintf(value, sizeof(value), "%" PRIu64, entry.second);
            blaze_snzprintf(key, sizeof(key), "GaugeUS_%s_%s_%s_Slave", entry.first[0], entry.first[1], entry.first[2]);
            map[key] = value;

            countByClientType[entry.first[0]] += entry.second;
        }

        for(auto& entry : countByClientType)
        {
            blaze_snzprintf(value, sizeof(value), "%" PRIu64, entry.second);
            blaze_snzprintf(key, sizeof(key), "GaugeUS_%s_Slave", entry.first.c_str());
            map[key] = value;
        }
    }
    else
    {
        // NOTE: This case is only relevant for aux slaves that still need to surface 'local user session count by client type' 
        // metrics tracking user session assignment, despite not having a UserSessionsMaster component (which is what tracks these values on core slaves).
        for (i = 0; i < CLIENT_TYPE_INVALID; ++i)
        {
            const char8_t* clientTypeStr = ClientTypeToString((ClientType)i);
            uint64_t count = mGaugeAuxLocalUserSessionCount.get({ { Metrics::Tag::client_type, clientTypeStr } });
            if (count > 0)
            {
                // only output this metric when there are local user sessions assigned to this instance to avoid metric pollution on non-auxslave instances
                blaze_snzprintf(value, sizeof(value), "%" PRIu64, count);
                blaze_snzprintf(key, sizeof(key), "GaugeUS_%s_Slave", clientTypeStr);
                map[key] = value;
            }
        }
    }

    for (i = 0; i < CLIENT_TYPE_INVALID; ++i)
    {
        const char8_t* clientTypeStr = ClientTypeToString((ClientType)i);
        uint64_t count = mGaugeUserSessionCount.get({ { Metrics::Tag::client_type, clientTypeStr } });
        blaze_snzprintf(value, sizeof(value), "%" PRIu64, count);
        blaze_snzprintf(key, sizeof(key), "GaugeUS_%s_Master", clientTypeStr);
        map[key] = value;
    }

    if (gUserSessionMaster != nullptr)
    {
        // NOTE: We intentionally constrain reported metrics to CLIENT_TYPE_GAMEPLAY_USER client type below 
        // because other client types (e.g: dedicated servers) are not required to perform QoS and cannot 
        // reliably supply Blaze with valid pingsite information. This causes Blaze to use a default(first valid) 
        // pingsite for said client types. When these default values are aggregated to generate legacy metrics 
        // counts output by the getStatus() RPC it results in data pollution. (See GOS-31900)

        const ClientType gameplayClientType = CLIENT_TYPE_GAMEPLAY_USER;
        const char8_t* clientTypeStr = ClientTypeToString(gameplayClientType);

        uint64_t total = 0;
        Metrics::SumsByTagValue pingsiteSums;
        gUserSessionMaster->mLocalUserSessionCounts.aggregate({ Metrics::Tag::client_type, Metrics::Tag::pingsite }, pingsiteSums);
        for(auto& pingSite : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
        {
            const char8_t* pingSiteAlias = pingSite.first.c_str();

            uint64_t count = pingsiteSums[{ clientTypeStr, pingSiteAlias }];
            total += count;
            blaze_snzprintf(value, sizeof(value), "%" PRIu64, count);
            blaze_snzprintf(key, sizeof(key), "GaugeUS_%s_Slave", pingSiteAlias);
            map[key] = value;
        }

        blaze_snzprintf(value, sizeof(value), "%" PRIu64, total);
        map["GaugeUserSessionsSlave"] = value;
    }

    for (i = 0; i < DISCONNECTTYPE_MAX; ++i)
    {
        const char8_t* reason = UserSessionDisconnectTypeToString((UserSessionDisconnectType)i);
        uint64_t v = mMetrics.mDisconnections.getTotal({ { Metrics::Tag::disconnect_type, reason } });

        blaze_snzprintf(key, sizeof(key), "TotalDisconnects_%s", reason);
        blaze_snzprintf(value, sizeof(value), "%" PRIu64, v);
        map[key] = value;
    }

    uint64_t disconnectionsByClientState[ClientState::STATUS_MAX][ClientState::MODE_MAX];
    memset(&disconnectionsByClientState, 0, sizeof(disconnectionsByClientState));
    mMetrics.mDisconnections.iterate([&disconnectionsByClientState](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ClientState::Status status;
            ClientState::Mode mode;
            if (ClientState::ParseStatus(tagList[1].second.c_str(), status) && ClientState::ParseMode(tagList[2].second.c_str(), mode))
            {
                disconnectionsByClientState[status][mode] += value.getTotal();
            }
    });

    for (i = 0; i < ClientState::STATUS_MAX; ++i)
    {
        const char8_t* clientStatus = ClientState::StatusToString((ClientState::Status)i);

        for (j = 0; j < ClientState::MODE_MAX; ++j)
        {
            const char8_t* clientMode = ClientState::ModeToString((ClientState::Mode)j);

            blaze_snzprintf(key, sizeof(key), "TotalDisconnectsClientState_%s_%s", clientStatus, clientMode);
            blaze_snzprintf(value, sizeof(value), "%" PRIu64, disconnectionsByClientState[i][j]);
            map[key] = value;
        }
    }

    uint64_t disconnectionsByClientTypeAndReason[CLIENT_TYPE_INVALID][UserSessionDisconnectType::DISCONNECTTYPE_MAX];
    memset(&disconnectionsByClientTypeAndReason, 0, sizeof(disconnectionsByClientTypeAndReason));
    mMetrics.mDisconnections.iterate([&disconnectionsByClientTypeAndReason](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
        ClientType clientType;
        UserSessionDisconnectType disconnectType;
        if (ParseClientType(tagList[1].second.c_str(), clientType) && ParseUserSessionDisconnectType(tagList[2].second.c_str(), disconnectType))
        {
            disconnectionsByClientTypeAndReason[clientType][disconnectType] += value.getTotal();
        }
    });

    for (i = 0; i < ClientType::CLIENT_TYPE_INVALID; ++i)
    {
        const char8_t* clientType = ClientTypeToString((ClientType)i);

        for (j = 0; j < UserSessionDisconnectType::DISCONNECTTYPE_MAX; ++j)
        {
            if (disconnectionsByClientTypeAndReason[i][j] > 0)
            {
                const char8_t* disconnectType = UserSessionDisconnectTypeToString((UserSessionDisconnectType)j);

                blaze_snzprintf(key, sizeof(key), "TotalDisconnectsClientTypeAndReason_%s_%s", clientType, disconnectType);
                blaze_snzprintf(value, sizeof(value), "%" PRIu64, disconnectionsByClientTypeAndReason[i][j]);
                map[key] = value;
            }
        }
    }

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mPinEventsSent.getTotal());
    map["TotalPINEventsSent"] = value;
    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mMetrics.mPinEventBytesSent.getTotal());
    map["TotalPINEventBytesSent"] = value;

    getStatusInfoGlobals(status);
}

uint32_t UserSessionManager::getUserSessionCountByClientType(ClientType clientType, eastl::list<eastl::string>* productNames /*= nullptr*/) const
{ 
    const char8_t* clientTypeStr = ClientTypeToString(clientType);
    if ((productNames == nullptr) || productNames->empty())
    {
        return (uint32_t) mGaugeUserSessionCount.get({ {Metrics::Tag::client_type, clientTypeStr} });
    }
    else
    {
        // Just doing the Naive solution here.  If we add tag groups, the lookup is essentially free.
        uint64_t total = 0;
        for (auto curName : *productNames)
            total += mGaugeUserSessionCount.get({ {Metrics::Tag::product_name, curName.c_str() }, {Metrics::Tag::client_type, clientTypeStr} });

        return static_cast<uint32_t>(total);
    }
}

uint32_t UserSessionManager::getGameplayUserSessionCount(eastl::list<eastl::string>* productNames /*= nullptr*/) const
{
    return getUserSessionCountByClientType(CLIENT_TYPE_GAMEPLAY_USER, productNames) + getUserSessionCountByClientType(CLIENT_TYPE_LIMITED_GAMEPLAY_USER, productNames);
}


/* \brief return system-wide metrics */
void UserSessionManager::getStatusInfoGlobals(ComponentStatus& status) const
{    
    Blaze::ComponentStatus::InfoMap& userSessionsStatusMap = status.getInfo();

    char8_t buf[64];

    uint64_t totalSessionsRemovedClientLogout = mUserSessionsGlobalMetrics.mSessionsRemoved.getTotal({ { Metrics::Tag::disconnect_type, UserSessionDisconnectTypeToString(DISCONNECTTYPE_CLIENT_LOGOUT) } });
    uint64_t totalSessionsRemovedDuplicateLogin = mUserSessionsGlobalMetrics.mSessionsRemoved.getTotal({ { Metrics::Tag::disconnect_type, UserSessionDisconnectTypeToString(DISCONNECTTYPE_DUPLICATE_LOGIN) } });

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, totalSessionsRemovedClientLogout);
    userSessionsStatusMap["TotalUserSessionsRemovedClientLogout"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, totalSessionsRemovedDuplicateLogin);
    userSessionsStatusMap["TotalUserSessionsRemovedDuplicateLogin"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64,mUserSessionsGlobalMetrics.mSessionsRemoved.getTotal() - totalSessionsRemovedClientLogout - totalSessionsRemovedDuplicateLogin);
    userSessionsStatusMap["TotalUserSessionsRemovedOther"]=buf;

    // get() calls require doing lots of string comparisons. (Each tag gets a comparison)
    // Since we know that we're outputting all values (or all non-zero values at least) we can skip the get calls, and just use the values directly:

    char8_t metricname[256];

    // The tag and product names used here DO NOT MATTER for iterate().  (They only apply to get() functions)
    mUserSessionsCreated.iterate({ { Metrics::Tag::product_name, "" } },
        [&status, &buf](const Metrics::TagPairList& tagList, const Metrics::Counter& value)
    {
        const auto productName = tagList[0].second;

        // 1 map<string> lookup per product (unavoidable)
        auto infoIter = status.getInfoByProductName().find(productName.c_str());
        if (infoIter == status.getInfoByProductName().end())
            infoIter = status.getInfoByProductName().insert(eastl::make_pair(productName.c_str(), status.getInfoByProductName().allocate_element())).first;

        // 2 map<string> lookup (unavoidable)
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.getTotal());
        (*infoIter->second)["TotalUserSessionsCreated"] = buf;              // 2 map<string> lookup (unavoidable)
    }
    );

    // Since this doesn't require a string lookup, it's faster to just track it here: 
    mUserSessionsCreated.iterate({ { Metrics::Tag::product_name, "" }, { Metrics::Tag::client_type, "" } },
        [&status, &buf, &metricname](const Metrics::TagPairList& tagList, const Metrics::Counter& value)
        {
            const auto productName = tagList[0].second;
            const auto clientType = tagList[1].second;

            // 1 map<string> lookup per product/client combination (unavoidable)
            auto infoIter = status.getInfoByProductName().find(productName.c_str());
            if (infoIter == status.getInfoByProductName().end())
                infoIter = status.getInfoByProductName().insert(eastl::make_pair(productName.c_str(), status.getInfoByProductName().allocate_element())).first;

            //Total
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.getTotal());
            blaze_snzprintf(metricname, sizeof(metricname), "TotalUSCreated_%s", clientType.c_str());
            
            // 2 map<string> lookup (unavoidable)
            (*infoIter->second)[metricname] = buf;
        }
    );

    mGaugeUserSessionCount.iterate({ { Metrics::Tag::product_name, "" } },
        [&status, &buf](const Metrics::TagPairList& tagList, const Metrics::Gauge& value)
    {
        const auto productName = tagList[0].second;

        // 1 map<string> lookup per product (unavoidable)
        auto infoIter = status.getInfoByProductName().find(productName.c_str());
        if (infoIter == status.getInfoByProductName().end())
            infoIter = status.getInfoByProductName().insert(eastl::make_pair(productName.c_str(), status.getInfoByProductName().allocate_element())).first;

        // 2 map<string> lookup (unavoidable)
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.get());
        (*infoIter->second)["GaugeUserSessions"] = buf;              // 2 map<string> lookup (unavoidable)
    }
    );

    mGaugeUserSessionCount.iterate({ { Metrics::Tag::product_name, "" }, { Metrics::Tag::client_type, "" } },
        [&status, &buf, &metricname](const Metrics::TagPairList& tagList, const Metrics::Gauge& value)
    {
        const auto productName = tagList[0].second;
        const auto clientType = tagList[1].second;

        // 1 map<string> lookup per product/client combination (unavoidable)
        auto infoIter = status.getInfoByProductName().find(productName.c_str());
        if (infoIter == status.getInfoByProductName().end())
            infoIter = status.getInfoByProductName().insert(eastl::make_pair(productName.c_str(), status.getInfoByProductName().allocate_element())).first;

        //Total
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.get());
        blaze_snzprintf(metricname, sizeof(metricname), "GaugeUS_%s", clientType.c_str());

        // 2 map<string> lookup (unavoidable)
        (*infoIter->second)[metricname] = buf;
    }
    );


    mGaugeUserSessionCount.iterate({ { Metrics::Tag::blaze_sdk_version, "" } },
        [&status, &buf](const Metrics::TagPairList& tagList, const Metrics::Gauge& value)
    {
        const auto clientVersion = tagList[0].second;

        // No gauge metrics for client versions that are no longer in use:
        if (value.get() == 0)
            return;

        // 1 map<string> lookup per product (unavoidable)
        auto infoIter = status.getInfoByClientVersion().find(clientVersion.c_str());
        if (infoIter == status.getInfoByClientVersion().end())
            infoIter = status.getInfoByClientVersion().insert(eastl::make_pair(clientVersion.c_str(), status.getInfoByClientVersion().allocate_element())).first;

        // 2 map<string> lookup (unavoidable)
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.get());
        (*infoIter->second)["GaugeUserSessions"] = buf;              // 2 map<string> lookup (unavoidable)
    }
    );

    mGaugeUserSessionCount.iterate({ { Metrics::Tag::client_type, "" }, { Metrics::Tag::blaze_sdk_version, "" } },
        [&status, &buf, &metricname](const Metrics::TagPairList& tagList, const Metrics::Gauge& value)
    {
        const auto clientType = tagList[0].second;
        const auto clientVersion = tagList[1].second;

        // 1 map<string> lookup per product/client combination (unavoidable)
        auto infoIter = status.getInfoByClientVersion().find(clientVersion.c_str());
        if (infoIter == status.getInfoByClientVersion().end())
        {
            // We rely on the previous iteration to fill in the getInfoByClientVersion, so if we get here, we shouldn't have any elements to track:
            if (value.get() != 0)
            {
                BLAZE_ERR_LOG(Log::USER, "UserSessionMaster::getStatusInfoGlobals: Not outputting client version " << clientVersion.c_str() << " but client type " << clientType.c_str() << " has usage count of " << value.get() << ".");
            }
            return;
        }

        //Total
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.get());
        blaze_snzprintf(metricname, sizeof(metricname), "GaugeUS_%s", clientType.c_str());

        // 2 map<string> lookup (unavoidable)
        (*infoIter->second)[metricname] = buf;
    }
    );

    if (gUserSessionMaster != nullptr)
    {

        gUserSessionMaster->mLocalUserSessionCounts.iterate({ { Metrics::Tag::product_name, "" }, { Metrics::Tag::client_type, "" }, { Metrics::Tag::client_status, "" }, { Metrics::Tag::client_mode, "" } },
            [&status, &buf, &metricname](const Metrics::TagPairList& tagList, const Metrics::Gauge& value)
        {
            const auto productName = tagList[0].second;
            const auto clientType = tagList[1].second;
            const auto clientStatus = tagList[2].second;
            const auto clientMode = tagList[3].second;

            // 1 map<string> lookup per product/client combination (unavoidable)
            auto infoIter = status.getInfoByProductName().find(productName.c_str());
            if (infoIter == status.getInfoByProductName().end())
                infoIter = status.getInfoByProductName().insert(eastl::make_pair(productName.c_str(), status.getInfoByProductName().allocate_element())).first;

            //Total
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.get());
            blaze_snzprintf(metricname, sizeof(metricname), "GaugeUS_%s_%s_%s_Slave", clientType.c_str(), clientStatus.c_str(), clientMode.c_str());

            // 2 map<string> lookup (unavoidable)
            (*infoIter->second)[metricname] = buf;
        }
        );

        // The per-product name pingsite metrics tracked here are only for the gameplay users:
        // (Same rationale as the global versions)
        const ClientType gameplayClientType = CLIENT_TYPE_GAMEPLAY_USER;
        const char8_t* clientTypeStr = ClientTypeToString(gameplayClientType);

        Metrics::SumsByTagValue pingsiteSums;
        gUserSessionMaster->mLocalUserSessionCounts.aggregate({ Metrics::Tag::product_name, Metrics::Tag::client_type, Metrics::Tag::pingsite }, pingsiteSums);
        
        // We just added the entries above.  No need to do the same logic here:
        for (auto& infoIter : status.getInfoByProductName())
        {
            const char8_t* productName = infoIter.first.c_str();
            for (auto& pingSite : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
            {
                const char8_t* pingSiteAlias = pingSite.first.c_str();

                uint64_t count = pingsiteSums[{ productName, clientTypeStr, pingSiteAlias }];
                blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, count);
                blaze_snzprintf(metricname, sizeof(metricname), "GaugeUS_%s_Slave", pingSiteAlias);
                (*infoIter.second)[metricname] = buf;
            }
        }
    }

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mUserSessionsCreated.getTotal());
    userSessionsStatusMap["TotalUserSessionsCreated"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mMetrics.mGaugeUserSessionsOverall.get());
    userSessionsStatusMap["GaugeUserSessionsOverall"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mMetrics.mGaugeOwnedUserSessions.get());
    userSessionsStatusMap["GaugeOwnedUserSessions"]=buf;

    //Gauge
    for (int i = 0; i < CLIENT_TYPE_INVALID; ++i)
    {
        const char8_t* clientTypeStr = ClientTypeToString((ClientType)i);
        blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mGaugeUserSessionCount.get({ {Metrics::Tag::client_type, clientTypeStr} }) );
        blaze_snzprintf(metricname,sizeof(metricname),"GaugeUS_%s", clientTypeStr);
        userSessionsStatusMap[metricname] = buf;
    }
}

BlazeRpcError UserSessionManager::filloutUserData(const UserInfo& userInfo, Blaze::UserData& userData, bool includeNetworkAddress, ClientPlatformType callerPlatform)
{
    // fill out the user information
    UserInfo::filloutUserIdentification(userInfo, userData.getUserInfo());
    
    UserSessionId primarySessionId = getPrimarySessionId(userInfo.getId());
    if (UserSession::isValidUserSessionId(primarySessionId))
    {
        // NOTE: The session may be a primary remote subscribed session, 
        // so its more efficient to use getSession() than getLocalSession().
        if (UserSession::isOwnedByThisInstance(primarySessionId))
        {
            UserSessionMasterPtr primaryUserSession = gUserSessionMaster->getLocalSession(primarySessionId);
            if (primaryUserSession != nullptr)
                primaryUserSession->getExtendedData().copyInto(userData.getExtendedData());
            else
                primarySessionId = INVALID_USER_SESSION_ID;
        }
        else
        {
            // user is online, but on a remote instance, fetch UED from it and refresh it.
            BlazeRpcError err = gUserSessionManager->getRemoteUserExtendedData(primarySessionId, userData.getExtendedData());
            if (err != ERR_OK)
                return err;
        }
    }

    BlazeRpcError err;
    if (UserSession::isValidUserSessionId(primarySessionId))
    {
        //updating extended data before dispatching
        err = refreshUserExtendedData(userInfo, userData.getExtendedData());
    }
    else
    {
        // Request extended data from listeners
        err = requestUserExtendedData(userInfo, userData.getExtendedData());
    }

    if (err == ERR_OK)
    {
        // if there is a primary session then the user is online
        userData.getStatusFlags().setOnline(UserSession::isValidUserSessionId(primarySessionId));
        if (!includeNetworkAddress)
        {
            NetworkAddress address;
            address.copyInto(userData.getExtendedData().getAddress());
        }
        
    }

    obfuscate1stPartyIds(callerPlatform, userData.getUserInfo().getPlatformInfo());

    return err;
}

// static
void UserSessionManager::obfuscate1stPartyIds(ClientPlatformType callerPlatform, PlatformInfo& platformInfo)
{
    if (callerPlatform == common)
        return;

    ExternalIdentifiers extIdsCopy = platformInfo.getExternalIds();
    ExternalIdentifiers emptyExtIds;
    emptyExtIds.copyInto(platformInfo.getExternalIds());

    switch (callerPlatform)
    {
    case xone:
    case xbsx:
        platformInfo.getExternalIds().setXblAccountId(extIdsCopy.getXblAccountId());
        break;
    case ps4:
    case ps5:
        platformInfo.getExternalIds().setPsnAccountId(extIdsCopy.getPsnAccountId());
        break;
    case nx:
        platformInfo.getExternalIds().setSwitchId(extIdsCopy.getSwitchId());
        break;
    case steam:
        platformInfo.getExternalIds().setSteamAccountId(extIdsCopy.getSteamAccountId());
        break;
    case stadia:
        platformInfo.getExternalIds().setStadiaAccountId(extIdsCopy.getStadiaAccountId());
    break; 
    default:
        break;
    }
}

/*** Private Methods ******************************************************************************/

void UserSessionManager::addUserInfoToIndices(UserInfo* userInfo, bool cacheUserInfo)
{
    EA_ASSERT(userInfo != nullptr);

    if (cacheUserInfo)
        mUserInfoLruCache.add(userInfo->getId(), userInfo);

    if (userInfo->getIsIndexed())
        return;

    mUsersByBlazeId[userInfo->getId()] = userInfo;

    //skip users that don't have an external id
    if (((userInfo->getPlatformInfo().getClientPlatform() == ps4) || (userInfo->getPlatformInfo().getClientPlatform() == ps5)) && userInfo->getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
        mUsersByPsnAccountId[userInfo->getPlatformInfo().getExternalIds().getPsnAccountId()] = userInfo;
    if (((userInfo->getPlatformInfo().getClientPlatform() == xone) || (userInfo->getPlatformInfo().getClientPlatform() == xbsx)) && userInfo->getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
        mUsersByXblAccountId[userInfo->getPlatformInfo().getExternalIds().getXblAccountId()] = userInfo;
    if (userInfo->getPlatformInfo().getClientPlatform() == nx && !userInfo->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
        mUsersBySwitchId[userInfo->getPlatformInfo().getExternalIds().getSwitchId()] = userInfo;
    if ((userInfo->getPlatformInfo().getClientPlatform() == steam) && userInfo->getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        mUsersBySteamAccountId[userInfo->getPlatformInfo().getExternalIds().getSteamAccountId()] = userInfo;
    if ((userInfo->getPlatformInfo().getClientPlatform() == stadia) && userInfo->getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
        mUsersByStadiaAccountId[userInfo->getPlatformInfo().getExternalIds().getStadiaAccountId()] = userInfo;
    
    bool insertedOriginPersonaName = false;

    // Skip users logged in without persona name
    if (*userInfo->getPersonaName() != '\0')
    {
        UserInfoByPersonaMap& personNameMap = mPersonaMapByNamespaceMap[userInfo->getPersonaNamespace()];
        personNameMap[userInfo->getPersonaName()][userInfo->getPlatformInfo().getClientPlatform()] = userInfo;
        insertedOriginPersonaName = (blaze_strcmp(userInfo->getPersonaNamespace(), NAMESPACE_ORIGIN) == 0);
    }

    if (!insertedOriginPersonaName && !userInfo->getPlatformInfo().getEaIds().getOriginPersonaNameAsTdfString().empty())
    {
        UserInfoByPersonaMap& originNameMap = mPersonaMapByNamespaceMap[NAMESPACE_ORIGIN];
        originNameMap[userInfo->getPlatformInfo().getEaIds().getOriginPersonaName()][userInfo->getPlatformInfo().getClientPlatform()] = userInfo;
    }

    if (userInfo->getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
    {
        UserInfoByOriginPersonaIdMap& userInfoByOriginPersonaId = mUsersByOriginPersonaId[userInfo->getPlatformInfo().getClientPlatform()];
        userInfoByOriginPersonaId[userInfo->getPlatformInfo().getEaIds().getOriginPersonaId()] = userInfo;
    }

    if (userInfo->getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
    {
        UserInfoByAccountIdMap& userInfoByAccountId = mUsersByAccountId[userInfo->getPlatformInfo().getClientPlatform()];
        userInfoByAccountId[userInfo->getPlatformInfo().getEaIds().getNucleusAccountId()] = userInfo;
    }

    userInfo->setIsIndexed(true);
}

void UserSessionManager::removeUserInfoFromIndicesByPersona(UserInfo* userInfo, const char8_t* personaName, const char8_t* personaNamespace)
{
    PersonaMapByNamespaceMap::iterator it = mPersonaMapByNamespaceMap.find(personaNamespace);
    if (it != mPersonaMapByNamespaceMap.end())
    {
        bool platformPersonaFound = false;
        ClientPlatformType platform = userInfo->getPlatformInfo().getClientPlatform();
        UserInfoByPersonaMap& usersByPersona = it->second;
        UserInfoByPersonaMap::iterator userIt = usersByPersona.find(personaName);
        if (userIt != usersByPersona.end())
        {
            UserInfoByPlatform::iterator platIt = userIt->second.find(platform);
            if (platIt != userIt->second.end())
            {
                platformPersonaFound = true;

                // erase the map entry if it matches the removed UserInfo  
                if (platIt->second == userInfo)
                    userIt->second.erase(platIt);
            }
        }

        if (!platformPersonaFound)
        {
            BLAZE_INFO_LOG(Log::USER, "[UserSessionManager].removeUserInfoFromIndicesByPersona - unexpected lookup failure by persona(" << personaName << ") in personaNamespace (" << personaNamespace << ") for blazeId(" << userInfo->getId() << ")");
        }
    }
}

template<typename T, typename T2>
void removeUserMapEntry(T id, T invalidId, T2& map, const char* idName)
{
    if (id != invalidId)
    {
        eastl_size_t entriesRemoved = map.erase(id);
        if (entriesRemoved <= 0)
            BLAZE_INFO_LOG(Log::USER, "UserSessionMaster.removeUserInfoFromIndices: Failed to remove entry for "<< idName <<" ( " << id << ") from map.  " <<
                                     "Check if a code path could cause the user to change ids/platform.");
    }
}

void UserSessionManager::removeUserInfoFromIndices(UserInfo* userInfo)
{
    EA_ASSERT(userInfo != nullptr);
    
    if (!userInfo->getIsIndexed())
        return;

    userInfo->setIsIndexed(false);

    //Toss the object
    BlazeId blazeId = userInfo->getId();
    
    removeUserMapEntry(blazeId, INVALID_BLAZE_ID, mUsersByBlazeId, "BlazeId");

    if (((userInfo->getPlatformInfo().getClientPlatform() == ps4) || userInfo->getPlatformInfo().getClientPlatform() == ps5))
        removeUserMapEntry(userInfo->getPlatformInfo().getExternalIds().getPsnAccountId(), INVALID_PSN_ACCOUNT_ID, mUsersByPsnAccountId, "PSNId");
    if ((userInfo->getPlatformInfo().getClientPlatform() == xone) || (userInfo->getPlatformInfo().getClientPlatform() == xbsx))
        removeUserMapEntry(userInfo->getPlatformInfo().getExternalIds().getXblAccountId(), INVALID_XBL_ACCOUNT_ID, mUsersByXblAccountId, "XBLId");
    if (userInfo->getPlatformInfo().getClientPlatform() == nx)
        removeUserMapEntry(userInfo->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString(), EA::TDF::TdfString(), mUsersBySwitchId, "SwitchId");
    if (userInfo->getPlatformInfo().getClientPlatform() == steam)
        removeUserMapEntry(userInfo->getPlatformInfo().getExternalIds().getSteamAccountId(), INVALID_STEAM_ACCOUNT_ID, mUsersBySteamAccountId, "STEAMId");
    if (userInfo->getPlatformInfo().getClientPlatform() == stadia)
        removeUserMapEntry(userInfo->getPlatformInfo().getExternalIds().getStadiaAccountId(), INVALID_STADIA_ACCOUNT_ID, mUsersByStadiaAccountId, "STADIAId");

    bool removedOriginPersonaName = false;

    //skip users with no persona name
    if (*userInfo->getPersonaName() != '\0')
    {
        removeUserInfoFromIndicesByPersona(userInfo, userInfo->getPersonaName(), userInfo->getPersonaNamespace());
        removedOriginPersonaName = (blaze_strcmp(userInfo->getPersonaNamespace(), NAMESPACE_ORIGIN) == 0);
    }

    if (!removedOriginPersonaName && !userInfo->getPlatformInfo().getEaIds().getOriginPersonaNameAsTdfString().empty())
        removeUserInfoFromIndicesByPersona(userInfo, userInfo->getPlatformInfo().getEaIds().getOriginPersonaName(), NAMESPACE_ORIGIN);

    UserInfoByOriginPersonaIdByPlatformMap::iterator oItr = mUsersByOriginPersonaId.find(userInfo->getPlatformInfo().getClientPlatform());
    if (oItr != mUsersByOriginPersonaId.end())
        removeUserMapEntry(userInfo->getPlatformInfo().getEaIds().getOriginPersonaId(), INVALID_ORIGIN_PERSONA_ID, oItr->second, "OriginPersonaId");

    UserInfoByAccountIdByPlatformMap::iterator aItr = mUsersByAccountId.find(userInfo->getPlatformInfo().getClientPlatform());
    if (aItr != mUsersByAccountId.end())
        removeUserMapEntry(userInfo->getPlatformInfo().getEaIds().getNucleusAccountId(), INVALID_ACCOUNT_ID, aItr->second, "NucleusAccountId");
}

void UserSessionManager::updateUserInfoCache(BlazeId blazeId, AccountId accountId, bool broadcastUpdateNotification)
{
    if (accountId != INVALID_ACCOUNT_ID)
    {
        for (const auto& platform : gController->getHostedPlatforms())
        {
            UserInfoPtr userInfo = findResidentUserInfoByAccountId(accountId, platform);
            if (userInfo != nullptr)
            {
                if (broadcastUpdateNotification)
                    broadcastUserInfoUpdatedNotification(userInfo->getId());
                else
                    removeUserInfoFromIndices(userInfo.get());
            }
        }
    }
    else
    {
        if (broadcastUpdateNotification)
            broadcastUserInfoUpdatedNotification(blazeId);
        else
        {
            UserInfo* userInfo = findResidentUserInfoByBlazeId(blazeId);
            if (userInfo != nullptr)
                removeUserInfoFromIndices(userInfo);
        }
    }
}

void UserSessionManager::broadcastUserInfoUpdatedNotification(BlazeId blazeId, AccountId accountId /*= INVALID_ACCOUNT_ID*/)
{
    UserInfoUpdated userInfoUpdated;
    userInfoUpdated.setBlazeId(blazeId);
    userInfoUpdated.setAccountId(accountId);
    sendNotifyUserInfoUpdatedToAllSlaves(&userInfoUpdated);
}

void UserSessionManager::onNotifyUserInfoUpdated(const UserInfoUpdated& data, UserSession*)
{
    BlazeIdList blazeIds;
    if (data.getAccountId() != INVALID_ACCOUNT_ID)
    {
        for (const auto& platform : gController->getHostedPlatforms())
        {
            UserInfoPtr userInfo = findResidentUserInfoByAccountId(data.getAccountId(), platform);
            if (userInfo != nullptr)
            {
                blazeIds.push_back(userInfo->getId());
                removeUserInfoFromIndices(userInfo.get());
            }
        }
    }
    else
    {
        blazeIds.push_back(data.getBlazeId());
        UserInfoPtr userInfo = findResidentUserInfoByBlazeId(data.getBlazeId());
        if (userInfo != nullptr)
            removeUserInfoFromIndices(userInfo.get());
    }

    if (gUserSessionMaster != nullptr)
    {
        for (auto& blazeId : blazeIds)
            gSelector->scheduleFiberCall<UserSessionsMasterImpl, BlazeId>(gUserSessionMaster, &UserSessionsMasterImpl::updateUserInfoInLocalUserSessions, blazeId, "UserSessionsMasterImpl::updateUserInfoInLocalUserSessions");
    }
}

//  return a list of user sessions by Id attached to the specified Blaze user.
BlazeRpcError UserSessionManager::getSessionIds(BlazeId blazeId, UserSessionIdList& ids)
{
    return getSessionIds(EA::TDF::ObjectId(ENTITY_TYPE_USER, blazeId), ids);
}


// userset
BlazeRpcError UserSessionManager::getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids)
{
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].getSessionIds");

    if (bobjId.type == ENTITY_TYPE_USER)
    {
        getUserSessionIdsByBlazeId(bobjId.id, ids);
        return ERR_OK;
    }
    else if (bobjId.type == ENTITY_TYPE_CONN_GROUP || bobjId.type == ENTITY_TYPE_LOCAL_USER_GROUP)
    {
        if (gUserSessionMaster != nullptr)
            return gUserSessionMaster->getConnectionGroupSessionIds(bobjId, ids);
        else
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getSessionIds - Cannot be called on an instance with no UserSessionsMaster component loaded!");
        }
    }
    else
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getSessionIds - Not Implemented for type " << bobjId.type.toString().c_str() << "!");
    }

    return ERR_SYSTEM;
}

BlazeRpcError UserSessionManager::getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
{
    BlazeRpcError result = ERR_OK;
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].getUserIds");

    if (bobjId.type == ENTITY_TYPE_USER)
    {
        UserInfoPtr info;
        result = lookupUserInfoByBlazeId(bobjId.id, info);
        if (result == ERR_OK)
            ids.push_back(info->getId());

        return result;
    }
    else if (bobjId.type == ENTITY_TYPE_CONN_GROUP || bobjId.type == ENTITY_TYPE_LOCAL_USER_GROUP)
    {
        if (gUserSessionMaster != nullptr)
            return gUserSessionMaster->getConnectionGroupUserBlazeIds(bobjId, ids);
        else
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getUserBlazeIds - Cannot be called on an instance with no UserSessionsMaster component loaded!");
        }
    }
    else
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getUserBlazeIds - Not Implemented for type " << bobjId.type.toString().c_str() << "!");
    }

    return ERR_SYSTEM;
}

BlazeRpcError UserSessionManager::getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
{
    BlazeRpcError result = ERR_OK;
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionManager].getUserIds");

    if (bobjId.type == ENTITY_TYPE_USER)
    {
        UserInfoPtr info;
        result = lookupUserInfoByBlazeId(bobjId.id, info);
        if (result == ERR_OK)
            UserInfo::filloutUserIdentification(*info, *ids.pull_back());

        return result;
    }
    else if (bobjId.type == ENTITY_TYPE_CONN_GROUP || bobjId.type == ENTITY_TYPE_LOCAL_USER_GROUP)
    {
        if (gUserSessionMaster != nullptr)
            return gUserSessionMaster->getConnectionGroupUserIds(bobjId, ids);
        else
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getUserIds - Cannot be called on an instance with no UserSessionsMaster component loaded!");
        }
    }
    else
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].getUserIds - Not Implemented for type " << bobjId.type.toString().c_str() << "!");
    }

    return ERR_SYSTEM;
}

RoutingOptions UserSessionManager::getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId)
{
    RoutingOptions routeTo;
    if (bobjId.type == ENTITY_TYPE_CONN_GROUP || bobjId.type == ENTITY_TYPE_LOCAL_USER_GROUP)   // Local user group is just an alias to a conn group. 
    {
        routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, bobjId.id);
    }
    else // for users, we can just get (any) instance id, since we can just lookup the data associated with that user from what we have stored locally: 
    {
        routeTo.setInstanceId(getLocalInstanceId());
    }

    return routeTo;
}
BlazeRpcError UserSessionManager::countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    count = 0;

    if (bobjId.type != ENTITY_TYPE_USER)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager]: invalid object type [" << bobjId.type.toString().c_str() << "] passed to countSessions.");
        return USER_ERR_INVALID_PARAM;
    }

    UserSessionIdList ids;
    BlazeRpcError result = getSessionIds(bobjId, ids);
    if (result == ERR_OK)
        count = ids.size();

    return result;
}

BlazeRpcError UserSessionManager::countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    count = 0;

    if (bobjId.type != ENTITY_TYPE_USER)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager]: invalid object type [" << bobjId.type.toString().c_str() << "] passed to countUsers.");
        return USER_ERR_INVALID_PARAM;
    }

    UserInfoPtr info;
    BlazeRpcError result = lookupUserInfoByBlazeId(bobjId.id, info);
    if (result == ERR_OK)
        count = 1;

    return result;
}

void UserSessionManager::createClientTypeDescriptions()
{
    // These client type flags allow callers of getUserSessionIdsByBlazeId() to specify a clientTypeCheckFunc
    // that limits the scope of the call to a specific ClientType.
    {
        ClientTypeFlags& flags = mClientTypeDescriptions[CLIENT_TYPE_GAMEPLAY_USER];
        flags.setClientTypeGameplayUser();
        flags.setEnforceSingleLogin         (true);
        flags.setPrimarySession             (true);
        flags.setLogMetrics                 (true);
        flags.setUpdateUserInfoData         (true);
        flags.setValidateSandboxId          (true);
        flags.setHasUserExtendedData        (true);
        flags.setAllowAsGamePlayer          (true);
        flags.setAllowAsLocalConnGroupMember(true);
        flags.setTrackConnectionMetrics     (true);
        flags.setEnableConnectivityChecks   (false);
        flags.setCheckExternalId            (true);
    }
    {
        ClientTypeFlags& flags = mClientTypeDescriptions[CLIENT_TYPE_LIMITED_GAMEPLAY_USER];
        flags.setClientTypeLimitedGameplayUser();
        flags.setEnforceSingleLogin         (true);
        flags.setPrimarySession             (true);
        flags.setLogMetrics                 (true);
        flags.setUpdateUserInfoData         (true);
        flags.setValidateSandboxId          (true);
        flags.setHasUserExtendedData        (true);
        flags.setAllowAsGamePlayer          (true);
        flags.setAllowAsLocalConnGroupMember(true);
        flags.setTrackConnectionMetrics     (true);
        flags.setEnableConnectivityChecks   (false);
        flags.setCheckExternalId            (true);
    }
    {
        ClientTypeFlags& flags = mClientTypeDescriptions[CLIENT_TYPE_DEDICATED_SERVER];
        flags.setClientTypeDedicatedServer();
        flags.setEnforceSingleLogin         (false);
        flags.setPrimarySession             (false);
        flags.setLogMetrics                 (false);
        flags.setUpdateUserInfoData         (false);
        flags.setValidateSandboxId          (false);
        flags.setHasUserExtendedData        (false);
        flags.setAllowAsGamePlayer          (false);
        flags.setAllowAsLocalConnGroupMember(true);
        flags.setTrackConnectionMetrics     (true);
        flags.setEnableConnectivityChecks   (true); 
        flags.setCheckExternalId            (false);
    }
    {
        ClientTypeFlags& flags = mClientTypeDescriptions[CLIENT_TYPE_HTTP_USER];
        flags.setClientTypeHttpUser();
        flags.setEnforceSingleLogin         (false);
        flags.setPrimarySession             (false);
        flags.setLogMetrics                 (false);
        flags.setUpdateUserInfoData         (true);
        flags.setValidateSandboxId          (false);
        flags.setHasUserExtendedData        (true);
        flags.setAllowAsGamePlayer          (false);
        flags.setAllowAsLocalConnGroupMember(false);
        flags.setTrackConnectionMetrics     (false);
        flags.setEnableConnectivityChecks   (false);
        flags.setCheckExternalId            (false);
    }
    {
        ClientTypeFlags& flags = mClientTypeDescriptions[CLIENT_TYPE_TOOLS];
        flags.setClientTypeTools();
        flags.setEnforceSingleLogin         (false);
        flags.setPrimarySession             (false);
        flags.setLogMetrics                 (false);
        flags.setUpdateUserInfoData         (false);
        flags.setValidateSandboxId          (false);
        flags.setHasUserExtendedData        (false);
        flags.setAllowAsGamePlayer          (false);
        flags.setAllowAsLocalConnGroupMember(false);
        flags.setTrackConnectionMetrics     (false);
        flags.setEnableConnectivityChecks   (false);
        flags.setCheckExternalId            (false);
    }
    {
        ClientTypeFlags& flags = mClientTypeDescriptions[CLIENT_TYPE_INVALID];
        flags.setClientTypeTools();
        flags.setEnforceSingleLogin         (false);
        flags.setPrimarySession             (false);
        flags.setLogMetrics                 (false);
        flags.setUpdateUserInfoData         (false);
        flags.setValidateSandboxId          (false);
        flags.setHasUserExtendedData        (false);
        flags.setAllowAsGamePlayer          (false);
        flags.setAllowAsLocalConnGroupMember(false);
        flags.setTrackConnectionMetrics     (false);
        flags.setEnableConnectivityChecks   (false);
        flags.setCheckExternalId            (false);
    }
}

/*! \brief Subscribes a class for user info events. */
void UserSessionManager::addSubscriber(UserSessionSubscriber& subscriber) 
{ 
    mMetrics.mGaugeUserSessionSubscribers.increment();
    mDispatcher.addDispatchee(subscriber); 
}

/*! \brief Unsubscribes a class for user info events. */
void UserSessionManager::removeSubscriber(UserSessionSubscriber& subscriber) 
{ 
    mMetrics.mGaugeUserSessionSubscribers.decrement();
    mDispatcher.removeDispatchee(subscriber); 
}

BlazeRpcError UserSessionManager::onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData)
{
    // this will never fail, though the underlying call to get external reputation data can
    // the reputation service simply provides the default reputation value in the case of a lookup failure
    ReputationService::ReputationServiceUtilPtr reputationServiceUtil = getReputationServiceUtil();
    reputationServiceUtil->onLoadExtendedData(data, extendedData);

    extendedData.setUserInfoAttribute(data.getUserInfoAttribute());
    extendedData.setCrossPlatformOptIn(data.getCrossPlatformOptIn());

    // Normally we load Client Config UED when it logs in. For a user who's not logged in load defaults here.
    if (!isUserOnline(data.getId()))
    {
        UserExtendedDataMap& dataMap = extendedData.getDataMap();
        for (UEDClientConfigMap::const_iterator iter = mUEDClientConfigMap.begin();
            iter != mUEDClientConfigMap.end(); ++iter)
        {
            uint32_t clientKey = (uint32_t)iter->first;
            if (dataMap.find(clientKey) == dataMap.end())
            {
                const UserSessionClientUEDConfig* config = iter->second;
                UserExtendedDataKey key = UED_KEY_FROM_IDS(Component::INVALID_COMPONENT_ID, config->getId());
                dataMap[ key ] = config->getDefault();
            }
        }
    }

    return ERR_OK;
}

BlazeRpcError UserSessionManager::onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) 
{
    ReputationService::ReputationServiceUtilPtr reputationServiceUtil = getReputationServiceUtil();
    return reputationServiceUtil->onRefreshExtendedData(data, extendedData); 
}

bool UserSessionManager::getGeoIpData(const InetAddress& connAddr, GeoLocationData& geoLocData)
{
    return (getGeoIpDataWithOverrides(INVALID_BLAZE_ID, connAddr, geoLocData) == ERR_OK);
}

uint32_t UserSessionManager::getLocalUserSessionCountByClientType(ClientType clientType) const
{
    uint32_t countByClientType = 0;

    if (gUserSessionMaster != nullptr)
    {
        countByClientType = (uint32_t)gUserSessionMaster->mLocalUserSessionCounts.get({ { Metrics::Tag::client_type, ClientTypeToString(clientType) } });
    }
    return countByClientType;
}

uint32_t UserSessionManager::getLocalUserSessionCount() const
{
    return (uint32_t) gUserSessionMaster->mLocalUserSessionCounts.get();
}

BlazeRpcError UserSessionManager::getLocalUserSessionCounts(Blaze::GetPSUResponse &response) const
{
    if (gUserSessionMaster == nullptr)
        return ERR_OK;

    PSUMap& map = response.getPSUMap();

    enum METRIC_KIND { MK_CLIENT_TYPE, MK_MAX_COUNT };
    const Metrics::TagPairList tagInitList = {              // The order must match what is seen in the gUserSessionMaster->mLocalUserSessionCounts tag groups...
        Metrics::TagPair(Metrics::Tag::client_type, "")    // tags[ MK_CLIENT_TYPE ]
    };

    gUserSessionMaster->mLocalUserSessionCounts.iterate(tagInitList,
        [&map](const Metrics::TagPairList& tags, const Metrics::Gauge& value)
        {
            if (tags.size() < (size_t)MK_MAX_COUNT)
                return; // Shouldn't happen, but we don't want to crash...  So, sanity check here.
            char8_t key[256];
            const char8_t* clientType = tags[MK_CLIENT_TYPE].second.c_str();
            blaze_snzprintf(key, sizeof(key), "%s", clientType);
            map[key] += (uint32_t)value.get();
        });

    return ERR_OK;
}

bool UserSessionManager::isReputationDisabled()
{
    return mReputationServiceFactory.isReputationDisabled();
}

Blaze::ReputationService::ReputationServiceUtilPtr UserSessionManager::getReputationServiceUtil()
{ 
    return mReputationServiceFactory.getReputationServiceUtil();
}

eastl::string UserSessionManager::formatAdminActionContext(const Message* message)
{
    eastl::string msg("Admin action invoked by: ");
    char8_t namebuff[256] = "<unknown connection>";
    if (message != nullptr)
    {
        message->getPeerInfo().toString(namebuff, sizeof(namebuff));
        if (!message->getFrame().superUserPrivilege)
        { 
            BlazeId blazeId = gUserSessionManager->getBlazeId(message->getUserSessionId());
            if (blazeId != INVALID_BLAZE_ID)
            {
                msg.append_sprintf("<blazeId = 0x%" PRIx64 "> @ %s", blazeId, namebuff);
            }
            else
            {
                msg.append_sprintf("<unknown user> @ %s", namebuff);
            }
        }
        else
        {
            msg.append_sprintf("<superuser> @ %s", namebuff);
        }
    }
    else
    {
        msg.append_sprintf("<unknown user> @ %s", namebuff);
    }
    return msg;
}

uint16_t UserSessionManager::getLatencyIndex(uint32_t latency)
{
    // shortcut for cases of 0 latency
    if (latency == 0)
        return 0;

    uint16_t latencyIndex = (uint16_t)(ceil((double)latency / LATENCY_BUCKET_SIZE));
    if (latencyIndex >= MAX_NUM_LATENCY_BUCKETS - 1)
        latencyIndex = MAX_NUM_LATENCY_BUCKETS - 1;

    return latencyIndex * LATENCY_BUCKET_SIZE;
}

UpdateLocalUserGroupError::Error UserSessionManager::processUpdateLocalUserGroup(const UpdateLocalUserGroupRequest& request, UpdateLocalUserGroupResponse& response, const Message* message)
{
    return gUserSessionMaster->updateLocalUserGroup(request, response, message);
}

const char8_t* ClientTypeToPinString(ClientType clientType)
{
    switch (clientType)
    {
    case CLIENT_TYPE_GAMEPLAY_USER: return "user";
    case CLIENT_TYPE_HTTP_USER: return "web_user";
    case CLIENT_TYPE_DEDICATED_SERVER: return "dedicated_server";
    case CLIENT_TYPE_TOOLS: return "client_tools";
    case CLIENT_TYPE_LIMITED_GAMEPLAY_USER: return "limited_user";
    default: return "invalid";
    }
}

void UserSessionManager::getPlatformSetFromExternalIdentifiers(ClientPlatformSet& platformSet, const ExternalIdentifiers& externalIds)
{
    platformSet.clear();
    for (const auto& platform : gController->getHostedPlatforms())
    {
        switch (platform)
        {
        case pc:
            // Since all users have Origin accounts and PC users don't have a 1st party external id,
            // we always assume there is a linked PC user
            platformSet.insert(platform);
            break;
        case xone:
        case xbsx:
            if (externalIds.getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
                platformSet.insert(platform);
            break;
        case ps4:
        case ps5:
            if (externalIds.getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
                platformSet.insert(platform);
            break;
        case nx:
            if (!externalIds.getSwitchIdAsTdfString().empty())
                platformSet.insert(platform);
            break;
        case steam:
            if (externalIds.getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
                platformSet.insert(platform);
            break;
        case stadia:
            if (externalIds.getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
                platformSet.insert(platform);
            break; 
        default:
            break;
        }
    }
}

PreparedStatementId UserSessionManager::getUserInfoPreparedStatement(uint32_t dbId, UserInfoDbCalls::UserInfoCallType callType) const
{
    UserInfoDbCalls::PreparedStatementsByDbIdMap::const_iterator itr = mUserInfoPreparedStatementsByDbId.find(dbId);
    if (itr == mUserInfoPreparedStatementsByDbId.end())
        return INVALID_PREPARED_STATEMENT_ID;
    return itr->second[callType];
}

void UserSessionManager::obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const
{
    switch (tdf.getTdfId())
    {
    case UserSessionLoginInfo::TDF_ID:
        return obfuscatePlatformInfo(platform, *(static_cast<UserSessionLoginInfo*>(&tdf)));
    case NotifyUserAdded::TDF_ID:
        return obfuscatePlatformInfo(platform, static_cast<NotifyUserAdded*>(&tdf)->getUserInfo());
    case UserIdentification::TDF_ID:
        return obfuscatePlatformInfo(platform, *(static_cast<UserIdentification*>(&tdf)));
    default:
        BLAZE_ASSERT_LOG(Log::USER, "[UserSessionManager].obfuscatePlatformInfo: Platform info obfuscation not implemented for Tdf class " << tdf.getFullClassName());
        break;
    }
}
void UserSessionManager::obfuscatePlatformInfo(ClientPlatformType platform, UserSessionLoginInfo& tdf)
{
    if (platform != tdf.getClientPlatform())
        tdf.setExtId(INVALID_EXTERNAL_ID);

    obfuscate1stPartyIds(platform, tdf.getPlatformInfo());
}
void UserSessionManager::obfuscatePlatformInfo(ClientPlatformType platform, UserIdentification& tdf)
{
    if (platform != tdf.getPlatformInfo().getClientPlatform())
        tdf.setExternalId(INVALID_EXTERNAL_ID);

    obfuscate1stPartyIds(platform, tdf.getPlatformInfo());
}
void UserSessionManager::obfuscatePlatformInfo(ClientPlatformType platform, UserIdentificationList& list)
{
    for (auto& it : list)
        obfuscatePlatformInfo(platform, *it);
}

bool UserSessionManager::shouldSendNotifyEvent(const ClientPlatformSet& platformSet, const Notification& notification) const
{
    const EA::TDF::Tdf* tdf = notification.getPayload();
    if (tdf == nullptr)
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].shouldSendNotifyEvent: Event with eventId(" << notification.getNotificationId() << ") has nullptr payload, and will not be submitted to the Notify service.");
        return false;
    }
    switch (tdf->getTdfId())
    {
    case Login::TDF_ID:
        return platformSet.find(static_cast<const Login*>(tdf)->getPlatformInfo().getClientPlatform()) != platformSet.end();
    case Logout::TDF_ID:
        return platformSet.find(static_cast<const Logout*>(tdf)->getPlatformInfo().getClientPlatform()) != platformSet.end();
    default:
        BLAZE_WARN_LOG(Log::USER, "[UserSessionManager].shouldSendNotifyEvent: Unhandled event with eventId(" << notification.getNotificationId() << ") and TdfId(" << tdf->getTdfId() << "). Event will be submitted to the Notify service regardless of platform.");
        return true;
    }
}

} //namespace Blaze

