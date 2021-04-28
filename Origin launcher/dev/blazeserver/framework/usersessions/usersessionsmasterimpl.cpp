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

#include "framework/database/dbscheduler.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/tdf/userextendeddatatypes_server.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/tdf/userevents_server.h"
#include "framework/controller/controller.h"
#include "framework/replication/replicatedmap.h"
#include "framework/rpc/usersessionsslave_stub.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/userdefines.h"
#include "framework/redis/redislockmanager.h"
#include "framework/redis/redismanager.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/slivers/slivermanager.h"
#include "framework/event/eventmanager.h"
#include "framework/storage/storagemanager.h"
#include "framework/usersessions/userinfodb.h"
#include "framework/component/componentmanager.h"
#include "framework/util/uuid.h"
#include "framework/system/fibermanager.h"
#include "nucleusidentity/tdf/nucleusidentity.h" // for IpGeoLocation
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/util/random.h"

namespace Blaze
{


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

// The script makes check and delete operation atomic (we should perhaps use the redis lock like we do at the time of key write).
// The Lua (redis scripting language) arrays are '1' based. 
// The script below takes the first(and only key) and gets it's value. If the value matches the caller's argument, the key is deleted and 1 is returned.
// If the key or value does not match, 0 is returned. 
RedisScript UserSessionsMasterImpl::msDeleteSingleLoginKeyScript(1,
    "local value = redis.call('GET', KEYS[1]);"
    "if (value == ARGV[1]) then"
    "    redis.call('DEL', KEYS[1]);"
    "    return 1;"
    "else"
    "    return 0;"
    "end;");

// static
UserSessionsMaster* UserSessionsMaster::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Component") UserSessionsMasterImpl();
}

const char8_t UserSessionsMasterImpl::PUBLIC_USERSESSION_FIELD[] = "pub.userSession";
const char8_t UserSessionsMasterImpl::PRIVATE_USERSESSION_FIELD[] = "pri.userSession";
const char8_t UserSessionsMasterImpl::PRIVATE_CUSTOM_STATE_FIELD[] = "pri.customState";
const char8_t UserSessionsMasterImpl::PRIVATE_CUSTOM_STATE_FIELD_FMT[] = "pri.customState.%" PRIu16;
const char8_t* UserSessionsMasterImpl::VALID_USERSESSION_FIELD_PREFIXES[] = { PUBLIC_USERSESSION_FIELD, PRIVATE_USERSESSION_FIELD, PRIVATE_CUSTOM_STATE_FIELD, nullptr };

static const uint16_t USERSESSIONS_ID_TYPE_SESSION      = 1;
static const uint16_t USERSESSIONS_ID_TYPE_CONNGROUP    = 2;
static const uint16_t USERSESSIONS_ID_TYPE_PIN          = 3;

UserSessionsMasterImpl::UserSessionsMasterImpl() : 
    mUserSessionsByBlazeId(BlazeStlAllocator("UserSessionsMasterImpl::mUserSessionsByBlazeId", COMPONENT_MEMORY_GROUP)),
    mAuxIdByCoreIdMap(BlazeStlAllocator("UserSessionsMasterImpl::mAuxIdByCoreIdMap", COMPONENT_MEMORY_GROUP)),
    mAuxInfoMap(BlazeStlAllocator("UserSessionsMasterImpl::mAuxInfoMap", COMPONENT_MEMORY_GROUP)),
    mUEDClientConfigMap(BlazeStlAllocator("UserSessionsMasterImpl::mUEDClientConfigMap", COMPONENT_MEMORY_GROUP)),
    mConnGroupUserSetMap(BlazeStlAllocator("UserSessionsMasterImpl::mConnGroupUserSetMap", COMPONENT_MEMORY_GROUP)),
    mCleanupTimerByInstanceIdMap(BlazeStlAllocator("UserSessionsMasterImpl::mCleanupTimerByInstanceIdMap", COMPONENT_MEMORY_GROUP)),
    mUserSessionTable(UserSessionsMaster::COMPONENT_ID, UserSessionsMaster::COMPONENT_MEMORY_GROUP, UserSessionsMaster::LOGGING_CATEGORY),
    mSingleLoginIds(RedisId(COMPONENT_INFO.name, "singleLoginIds")),
    mLocalSessionByConnGroupIdMap(BlazeStlAllocator("UserSessionsMasterImpl::mLocalSessionByConnGroupIdMap", UserSessionsMasterImpl::COMPONENT_MEMORY_GROUP)),
    mLocalUserSessionCounts(getMetricsCollection(), "gaugeUserSessions", Metrics::Tag::product_name, Metrics::Tag::client_type, Metrics::Tag::platform, Metrics::Tag::client_status, Metrics::Tag::client_mode, Metrics::Tag::pingsite),
    mNextNoDbUniqueUserSessionIdBase(SLIVER_KEY_BASE_MIN),
    mUserSessionCleanupJobQueue("UserSessionsMasterImpl::mUpdateBlazeObjectIdJobQueue"),
    mQosSettingsChangedOnLastReconfigure(false),
    mCensusCacheExpiry(0)
{
    EA_ASSERT(gUserSessionMaster == nullptr);
    gUserSessionMaster = this;

    // Tag Groups Aggregate values together to make lookups less expensive (very useful here since ISP tag results in 1000s of unique entries)
    // Platform field is currently unused and duplicates product_name. (Though it is much smaller and better suited for metric names)
    mLocalUserSessionCounts.defineTagGroups({
        { Metrics::Tag::client_type },
        { Metrics::Tag::product_name },
        { Metrics::Tag::client_type, Metrics::Tag::platform },
        { Metrics::Tag::client_type, Metrics::Tag::pingsite },
        { Metrics::Tag::product_name, Metrics::Tag::client_type, Metrics::Tag::pingsite },
        { Metrics::Tag::client_type, Metrics::Tag::client_status, Metrics::Tag::client_mode },
        { Metrics::Tag::product_name, Metrics::Tag::client_type, Metrics::Tag::client_status, Metrics::Tag::client_mode },
        { Metrics::Tag::platform, Metrics::Tag::pingsite } });   

    mUserSessionCleanupJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
}

UserSessionsMasterImpl::~UserSessionsMasterImpl()
{
    gUserSessionMaster = nullptr;
}

/*** Private Methods ******************************************************************************/

bool UserSessionsMasterImpl::onValidateConfig(UserSessionsConfig& config, const UserSessionsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    return true;
}

UserSessionMasterPtr UserSessionsMasterImpl::getLocalSession(UserSessionId userSessionId)
{
    if (UserSession::isValidUserSessionId(userSessionId))
    {
        UserSessionMasterPtrByIdMap::const_iterator it = mUserSessionMasterPtrByIdMap.find(userSessionId);
        if (it != mUserSessionMasterPtrByIdMap.end())
            return it->second;
    }

    return nullptr;
}

UserSessionMasterPtr UserSessionsMasterImpl::getLocalSessionBySessionKey(const char8_t* sessionKey)
{
    //First parse the key into its parts
    char8_t* keyHashSum = nullptr;
    UserSessionId userSessionId = INVALID_USER_SESSION_ID;
    if (!UserSessionManager::getUserSessionIdAndHashSumFromSessionKey(sessionKey, userSessionId, keyHashSum))
        return nullptr;

    UserSessionMasterPtr userSession = getLocalSession(userSessionId);
    if ((userSession == nullptr) || !userSession->validateKeyHashSum(keyHashSum))
        return nullptr;

    return userSession;
}

bool UserSessionsMasterImpl::onConfigure()
{
    // array of aux slave info mappings accessed by instance type index
    mAuxIdByCoreIdMap.clear();
    mAuxIdByCoreIdMap.resize(gController->getSlaveTypeCount());

    mUEDClientConfigMap.clear();

    UserSessionClientUEDConfigList::const_iterator iter = getConfig().getUserSessionClientUEDConfigList().begin();
    for (; iter != getConfig().getUserSessionClientUEDConfigList().end(); ++iter)
    {
        // Sanity Checks are done in UserSessionManager::configureCommon: 
        const UserSessionClientUEDConfig* config = *iter;
        mUEDClientConfigMap[config->getId()] = config;
    }

    // Because both usersessionmanager and uniqueidmanager are framework components the former 
    // needs to explicitly wait for the latter, to ensure that we can call reserveIdType.
    BlazeRpcError rc = gUniqueIdManager->waitForState(ACTIVE, "UserSessionsMasterImpl::onConfigure");
    if (rc != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onConfigure: Failed waiting for UniqueIdManager to become ACTIVE, err: " 
            << ErrorHelp::getErrorName(rc));
        return false;
    }

    // Setup our unique id management
    rc = gUniqueIdManager->reserveIdType(COMPONENT_ID, USERSESSIONS_ID_TYPE_SESSION);
    if (rc != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onConfigure: Failed reserving component id " << 
            COMPONENT_ID << " with unique id manager for type(USERSESSIONS_ID_TYPE_SESSION), err: " << ErrorHelp::getErrorName(rc));
        return false;
    }

    rc = gUniqueIdManager->reserveIdType(COMPONENT_ID, USERSESSIONS_ID_TYPE_CONNGROUP);
    if (rc != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onConfigure: Failed reserving component id " << 
            COMPONENT_ID << " with unique id manager for type(USERSESSIONS_ID_TYPE_CONNGROUP), err: " << ErrorHelp::getErrorName(rc));
        return false;
    }

    rc = gUniqueIdManager->reserveIdType(COMPONENT_ID, USERSESSIONS_ID_TYPE_PIN);
    if (rc != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onConfigure: Failed reserving component id " << 
            COMPONENT_ID << " with unique id manager for type(USERSESSIONS_ID_TYPE_PIN), err: " << ErrorHelp::getErrorName(rc));
        return false;
    }

    return true;
}

bool UserSessionsMasterImpl::onPrepareForReconfigure(const UserSessionsConfig& newConfig)
{
    mQosSettingsChangedOnLastReconfigure = !getConfig().getQosSettings().equalsValue(newConfig.getQosSettings());
    return true;
}

bool UserSessionsMasterImpl::onReconfigure()
{
    if (!mQosSettingsChangedOnLastReconfigure)
        return true;

    const QosConfigInfo& qosConfig = getConfig().getQosSettings();
    for (SessionByConnGroupIdMap::iterator itr = mLocalSessionByConnGroupIdMap.begin(); itr != mLocalSessionByConnGroupIdMap.end(); ++itr)
    {
        if (!itr->second.empty())
            gUserSessionManager->sendNotifyQosSettingsUpdatedToUserSession(&(*(itr->second.begin())), &qosConfig);
    }
    return true;
}

bool UserSessionsMasterImpl::onResolve()
{
    mUserSessionTable.registerFieldPrefixes(VALID_USERSESSION_FIELD_PREFIXES);
    mUserSessionTable.registerRecordHandlers(
        UpsertStorageRecordCb(this, &UserSessionsMasterImpl::onImportStorageRecord),
        EraseStorageRecordCb(this, &UserSessionsMasterImpl::onExportStorageRecord));

    if (gStorageManager == nullptr)
    {
        BLAZE_FATAL_LOG(Log::USER, "[UserSessionsMasterImpl] Storage manager is missing.  This shouldn't happen (was it in the components.cfg?).");
        return false;
    }
    gStorageManager->registerStorageTable(mUserSessionTable);

    return true;
}

bool UserSessionsMasterImpl::onActivate()
{
    return true;
}

void UserSessionsMasterImpl::onShutdown()
{
    mUserSessionCleanupJobQueue.join();

    // detach the storage table, no slivers will be imported or exported anymore
    if (gStorageManager != nullptr)
    {
        gStorageManager->deregisterStorageTable(mUserSessionTable);
    }

    if ((!mLocalSessionByConnGroupIdMap.empty() || !mConnGroupUserSetMap.empty()) && (gSliverManager != nullptr))
    {
        if (gSliverManager->isSliverNamespaceAbandoned(UserSessionsMaster::COMPONENT_ID))
        {
            // If the sliver namespace is abandoned, then that would usually mean the entire cluster is shutting down, and this condition cannot be avoided.
            BLAZE_TRACE_LOG(Log::USER, "UserSessionsMasterImpl.onShutdown: The ConnGroup maps are not empty. "
                "mLocalSessionByConnGroupIdMap.size(" << mLocalSessionByConnGroupIdMap.size() << "), "
                "mConnGroupUserSetMap.size(" << mConnGroupUserSetMap.size() << ")");
        }
        else
        {
            // If the sliver namespace is NOT abandoned, then that should mean all UserSessions would have been migrated to a different coreSlave, and these maps should be empty by now.
            BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.onShutdown: The ConnGroup maps are not empty. This should not occur during a graceful ZDT shutdown. "
                "mLocalSessionByConnGroupIdMap.size(" << mLocalSessionByConnGroupIdMap.size() << "), "
                "mConnGroupUserSetMap.size(" << mConnGroupUserSetMap.size() << ")");
        }
    }

    mLocalSessionByConnGroupIdMap.clear();
    mConnGroupUserSetMap.clear();
}

void UserSessionsMasterImpl::removeAuxSlave(InstanceId instanceId)
{
    AuxSlaveInfoMap::iterator it = mAuxInfoMap.find(instanceId);
    if (it == mAuxInfoMap.end())
    {
        // early out
        return;
    }
    const uint8_t typeIndex = it->second.typeIndex;
    if (typeIndex >= gController->getSlaveTypeCount())
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].removeAuxSlave: "
            "auxslave(" << instanceId << ") instance index " << (int32_t)typeIndex 
            << " exceeds max slave type count " << (int32_t)gController->getSlaveTypeCount());
        return;
    }

    //removeAuxSlave() did get called prior to the auxSlave coming back up, clean up mCleanupTimerByInstanceIdMap
    mCleanupTimerByInstanceIdMap.erase(instanceId);

    BLAZE_INFO_LOG(Log::USER, "[UserSessionsMasterImpl].removeAuxSlave: auxslave(" << instanceId << ") mapping removed.");
    AuxSlaveIdByCoreSlaveIdMap& idMap = mAuxIdByCoreIdMap[typeIndex];
    // get rid of core slave id mappings that refer to the id of the removed aux slave
    for (AuxSlaveIdByCoreSlaveIdMap::reverse_iterator i = idMap.rbegin(); i != idMap.rend();)
    {
        if (i->second == instanceId)
        {
            i = idMap.erase(i);
            continue;
        }
        ++i;
    }
    int32_t remapSessionCount = it->second.sessionCount;
    // get rid of existing aux slave information
    mAuxInfoMap.erase(it);
    
    // iterate all user sessions and remap the instance 
    // id of the current type index to new instance ids
    // (based on the current load on all the aux slaves)
    for (UserSessionMasterPtrByIdMap::iterator si = mUserSessionMasterPtrByIdMap.begin(), se = mUserSessionMasterPtrByIdMap.end(); si != se && remapSessionCount > 0; ++si)
    {
        UserSessionMaster& userSession = *si->second;
        InstanceId& auxInstanceId = userSession.mExistenceData->getInstanceIds()[typeIndex];
        if (auxInstanceId == instanceId)
        {
            // IMPROVE: Here we do not perform iterative assignment of user sessions, we just assign them all immediately, this reduces the effectiveness of the throttled assignment code...
            auxInstanceId = addSessionToAuxSlave(typeIndex, gController->getInstanceId());

            upsertUserExistenceData(*userSession.mExistenceData);

            // processed one session, decrement session count
            --remapSessionCount;
        }
    }    
}

void UserSessionsMasterImpl::assignSessionsToAuxSlaves(uint8_t slaveTypeIndex, InstanceId coreInstanceId)
{
    AuxSlaveIdByCoreSlaveIdMap& idMap = mAuxIdByCoreIdMap[slaveTypeIndex];
    AuxSlaveIdByCoreSlaveIdMapItPair result = idMap.equal_range(coreInstanceId);
    if (result.first == result.second)
    {
        // this should not normally be hit, it means that the aux instance was created
        // and removed before the usersession assignment process got a chance to run.
        BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].assignSessionsToAuxSlaves: "
            "core instance(" << coreInstanceId << ") has no aux instances mapped at index(" << (int32_t)slaveTypeIndex << ").");
        return;
    }
    
    // NOTE: currently we do *not* attempt to re-balance session/auxlsave instance assignment
    // when a new auxslave is added to the system. This would be easy to do, in a manner
    // analogous to the remapping done in onErase() handler; however, reassigning
    // an existing session to a new auxslave instance while an existing auxslave owns
    // said session could prove finicky to component code that might assume that sessions
    // always remain local to a single auxslave for the entire lifetime of the session and
    // the auxslave they live on.

    uint32_t assignedSessionCount = 0;
    for (UserSessionMasterPtrByIdMap::iterator si = mUserSessionMasterPtrByIdMap.begin(), se = mUserSessionMasterPtrByIdMap.end(); si != se && assignedSessionCount < AUX_USER_ASSIGN_BATCH_MAX; ++si)
    {
        UserSessionMaster& userSession = *si->second;
        // ensure usersession belongs to the primary slave that scheduled the assignment
        if (coreInstanceId == gController->getInstanceId())
        {
            InstanceId& auxInstanceId = userSession.mExistenceData->getInstanceIds()[slaveTypeIndex];
            // ensure usersession is not mapped to a different aux slave of same type
            if (auxInstanceId == INVALID_INSTANCE_ID)
            {
                auxInstanceId = addSessionToAuxSlave(slaveTypeIndex, coreInstanceId);

                upsertUserExistenceData(*userSession.mExistenceData);

                // processed one session
                ++assignedSessionCount;
            }
        }
    }
    if (assignedSessionCount > 0)
    {
        BLAZE_INFO_LOG(Log::USER, "[UserSessionsMasterImpl].assignSessionsToAuxSlaves: "
            "assigned " << assignedSessionCount << " user sessions to aux slaves mapped by core instance(" << coreInstanceId 
            << ") at index(" << (int32_t)slaveTypeIndex << ").");
    }
    if (assignedSessionCount < AUX_USER_ASSIGN_BATCH_MAX)
    {
        // we ran out of user sessions that belong to the specified core slave
        return;
    }
    // we have more user sessions to assign, reschedule the assignment process
    gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + AUX_USER_ASSIGN_DELAY_MSEC*1000, this,
        &UserSessionsMasterImpl::assignSessionsToAuxSlaves, slaveTypeIndex, coreInstanceId,
        "UserSessionsMasterImpl::assignSessionsToAuxSlaves");
}

InstanceId UserSessionsMasterImpl::addSessionToAuxSlave(uint8_t slaveTypeIndex, InstanceId coreInstanceId)
{
    InstanceId instanceId = INVALID_INSTANCE_ID;
    AuxSlaveIdByCoreSlaveIdMap& idMap = mAuxIdByCoreIdMap[slaveTypeIndex];
    AuxSlaveIdByCoreSlaveIdMapItPair result = idMap.equal_range(coreInstanceId);
    auto reserveSize = result.second - result.first;
    if (reserveSize > 0)
    {
        eastl::vector<InstanceId> readySlaves;
        eastl::vector<InstanceId> drainingSlaves;
        readySlaves.reserve(reserveSize);
        for (auto itr = result.first; itr != result.second; ++itr)
        {
            auto auxInstanceId = itr->second;
            auto auxItr = mAuxInfoMap.find(auxInstanceId);
            if (auxItr == mAuxInfoMap.end())
                continue;
            auto* remoteInstance = gController->getRemoteServerInstance(auxInstanceId);
            if (remoteInstance == nullptr)
                continue;
            if (remoteInstance->isDraining())
            {
                if (drainingSlaves.capacity() == 0)
                    drainingSlaves.reserve(reserveSize);
                drainingSlaves.push_back(auxInstanceId);
            }
            else
            {
                readySlaves.push_back(auxInstanceId);
            }
        }

        // First attempt to load balance across ready instances, if that fails to find anything, then we need to load balance across draining instances.
        decltype(readySlaves)* auxSlaveIdsByState[] = { &readySlaves, &drainingSlaves };
        for (auto* auxSlaveIds : auxSlaveIdsByState)
        {
            auto count = auxSlaveIds->size();
            if (count > 1)
            {
                // NOTE: This algorithm is adapted from the idea summarized here: https://brooker.co.za/blog/2012/01/17/two-random.html
                // The key theorem and associated proof shows that in a system with non-deterministic delay of information propagation 
                // a randomized selection process simply weighted by the desirable outcome from those two choices shall produce the most 
                // even load balancing results.
                auto instanceIndex1 = Random::getRandomNumber(count);
                auto instanceId1 = auxSlaveIds->at(instanceIndex1);
                auto sessionCount1 = mAuxInfoMap.find(instanceId1)->second.sessionCount;
                instanceId = instanceId1;
                if (sessionCount1 > 0)
                {
                    auxSlaveIds->erase_unsorted(auxSlaveIds->begin() + instanceIndex1);
                    auto instanceIndex2 = Random::getRandomNumber(count - 1);
                    auto instanceId2 = auxSlaveIds->at(instanceIndex2);
                    auto sessionCount2 = mAuxInfoMap.find(instanceId2)->second.sessionCount;
                    if (sessionCount2 < sessionCount1)
                        instanceId = instanceId2;
                }
                break;
            }
            if (count == 1)
            {
                // When there is only one choice there is no choice
                instanceId = auxSlaveIds->front();
                break;
            }
        }

        auto auxItr = mAuxInfoMap.find(instanceId);
        if (auxItr != mAuxInfoMap.end())
            ++auxItr->second.sessionCount; // bump count on chosen aux slave
    }

    return instanceId;
}

void UserSessionsMasterImpl::removeSessionFromAuxSlave(InstanceId auxInstanceId)
{
    AuxSlaveInfoMap::iterator it = mAuxInfoMap.find(auxInstanceId);
    if (it != mAuxInfoMap.end())
    {
        if (it->second.sessionCount > 0)
            --(it->second.sessionCount);   
    }
}

void UserSessionsMasterImpl::onSlaveSessionRemoved(SlaveSession& session)
{    
    InstanceId instanceId = SlaveSession::getSessionIdInstanceId(session.getId());
    
    BLAZE_TRACE_LOG(Log::USER, "[UserSessionsMasterImpl].onSlaveSessionRemoved: sessionId=" 
        << SbFormats::HexLower(session.getId()) << ", instanceId=" << instanceId);

    // remove all core -> aux slave mappings we had for this core slave, in
    // case the core slave did not make a call to remove the mappings itself
    for (uint8_t index = 0, indexEnd = gController->getSlaveTypeCount(); index < indexEnd; ++index)
    {
        AuxSlaveIdByCoreSlaveIdMap& idMap = mAuxIdByCoreIdMap[index];
        AuxSlaveIdByCoreSlaveIdMapItPair result = idMap.equal_range(instanceId);
        // NOTE: Only the multiple mappings of core -> aux need to be removed here.
        idMap.erase(result.first, result.second);
    }
    
    AuxSlaveInfoMap::iterator it = mAuxInfoMap.find(instanceId);
    if (it != mAuxInfoMap.end())
    {
        CleanupTimerByInstanceIdMap::insert_return_type ret = mCleanupTimerByInstanceIdMap.insert(eastl::make_pair(instanceId, 0));
        if (ret.second)
        {
            TimerId timerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + getConfig().getUserRemapOnAuxSlaveRemovalTimeout(), this, &UserSessionsMasterImpl::removeAuxSlave, instanceId, "UserSessionsMasterImpl::removeAuxSlave");
            BLAZE_INFO_LOG(Log::USER, "[UserSessionsMasterImpl].onSlaveSessionRemoved: scheduled timer=" << timerId << " for removal of auxSlave(" << instanceId << ")");

            ret.first->second = timerId;
        }
    }
}

void UserSessionsMasterImpl::onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot)
{
    StorageRecordSnapshot::FieldEntryMap::const_iterator existenceField;
    StorageRecordSnapshot::FieldEntryMap::const_iterator dataField;

    existenceField = getStorageFieldByPrefix(snapshot.fieldMap, PUBLIC_USERSESSION_FIELD);
    if (existenceField == snapshot.fieldMap.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onImportStorageRecord: Field prefix=" << PUBLIC_USERSESSION_FIELD << " matches no fields in usersession(" << snapshot.recordId << ").");
        mUserSessionTable.eraseRecord(snapshot.recordId);
        return;
    }

    dataField = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_USERSESSION_FIELD);
    if (dataField == snapshot.fieldMap.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onImportStorageRecord: Field prefix=" << PRIVATE_USERSESSION_FIELD << " matches no fields in usersession(" << snapshot.recordId << ").");
        mUserSessionTable.eraseRecord(snapshot.recordId);
        return;
    }

    UserSessionExistenceDataPtr userSessionExistence = static_cast<UserSessionExistenceData*>(existenceField->second.get());
    UserSessionDataPtr userSessionData = static_cast<UserSessionData*>(dataField->second.get());
    EA_ASSERT(userSessionExistence->getUserSessionId() == snapshot.recordId && userSessionData->getUserSessionId() == snapshot.recordId);

    BlazeRpcError err = ERR_SYSTEM;
    UserInfoPtr userInfo;
    switch (userSessionExistence->getUserSessionType())
    {
    case USER_SESSION_NORMAL:
    case USER_SESSION_WAL_CLONE:
        err = gUserSessionManager->lookupUserInfoByBlazeId(userSessionExistence->getBlazeId(), userInfo);
        break;
    case USER_SESSION_GUEST:
        err = gUserSessionManager->lookupUserInfoByPlatformInfo(userSessionExistence->getPlatformInfo(), userInfo);
        break;
    case USER_SESSION_TRUSTED:
        userInfo = BLAZE_NEW UserInfo();
        userInfo->setId(userSessionExistence->getBlazeId());
        userInfo->setPersonaName(userSessionData->getTrustedLoginId());
        err = ERR_OK;
        break;
    }

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].onImportStorageRecord: Failed to lookup user info with err(" << ErrorHelp::getErrorName(err) << "). UserSessionExistenceData:\n" << *userSessionExistence);
        mUserSessionTable.eraseRecord(snapshot.recordId);
        return;
    }

    UserSessionMasterPtrByIdMap::iterator it = mUserSessionMasterPtrByIdMap.find(snapshot.recordId);
    if (it != mUserSessionMasterPtrByIdMap.end())
    {
        BLAZE_ASSERT_LOG(Log::USER, "[UserSessionsMasterImpl].onImportStorageRecord: A UserSession with id(" << snapshot.recordId << ") already exists.");
        return;
    }

    UserSessionMasterPtr userSessionMaster = BLAZE_NEW UserSessionMaster(snapshot.recordId, userInfo);
    userSessionMaster->mExistenceData = userSessionExistence;
    userSessionMaster->mUserSessionData = userSessionData;

    // Handle the case where we updated the server binary to track underage status on the existence data
    // Any time new fields are added to the existence data or usersessiondata,
    // they need to be set here (if available) from the cached userinfo
    userSessionMaster->mExistenceData->getSessionFlags().setIsChildAccount(userInfo->getIsChildAccount());
    userSessionMaster->mExistenceData->getSessionFlags().setStopProcess(userInfo->getStopProcess());

    size_t componentIdOffset = strlen(PRIVATE_CUSTOM_STATE_FIELD) + 1; // +1 for the '.'
    StorageRecordSnapshot::FieldEntryMap::const_iterator customStateFieldItr = Blaze::getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_CUSTOM_STATE_FIELD);
    for (; customStateFieldItr != snapshot.fieldMap.end(); ++customStateFieldItr)
    {
        const char8_t* componentIdStr = customStateFieldItr->first.c_str() + componentIdOffset;

        if (blaze_strncmp(customStateFieldItr->first.c_str(), PRIVATE_CUSTOM_STATE_FIELD, componentIdOffset-1) != 0)
            continue; // Not a field we're interested in.

        ComponentId componentId = Component::INVALID_COMPONENT_ID;
        if ((customStateFieldItr->first.size() < componentIdOffset) || (blaze_str2int(componentIdStr, &componentId) == componentIdStr))
        {
            // The custom state field entries should always have the '.compId' suffix.
            BLAZE_ERR_LOG(getLogCategory(), "UserSessionsMasterImpl.onImportStorageRecord: The StorageRecord with recordId(" << snapshot.recordId << ") has an incomplete custom state field entry(" << customStateFieldItr->first.c_str() << ")");
            continue;
        }

        bool isComponentKnown = false;
        BlazeRpcComponentDb::getComponentNameById(componentId, &isComponentKnown);
        if (!isComponentKnown)
        {
            BLAZE_WARN_LOG(getLogCategory(), "UserSessionsMasterImpl.onImportStorageRecord: The StorageRecord with recordId(" << snapshot.recordId << ") and custom state field entry(" << customStateFieldItr->first.c_str() << ") refers to an unknown componentId(" << componentId << ")");
        }

        EA::TDF::TdfPtr tdfPtr = customStateFieldItr->second;

        userSessionMaster->insertComponentStateTdf(componentId, tdfPtr);
    }

    addLocalUserSession(userSessionMaster, ownedSliver);

    mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionImported, *userSessionMaster);
}

void UserSessionsMasterImpl::onExportStorageRecord(StorageRecordId recordId)
{
    UserSessionMasterPtrByIdMap::iterator it = mUserSessionMasterPtrByIdMap.find(recordId);
    if (it != mUserSessionMasterPtrByIdMap.end())
    {
        UserSessionMasterPtr userSessionMaster = it->second;

        mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionExported, *userSessionMaster);
        PeerInfo* peerInfo = userSessionMaster->getPeerInfo();
        if ((peerInfo != nullptr) && peerInfo->isPersistent())
        {
            peerInfo->closePeerConnection(DISCONNECTTYPE_USERSESSION_MIGRATING, false);
        }
        removeLocalUserSession(userSessionMaster);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::USER, "[UserSessionsMasterImpl].onExportStorageRecord: A UserSession with id(" << recordId << ") does not exist.");
    }
}

void UserSessionsMasterImpl::upsertUserExistenceData(UserSessionExistenceData& userExistenceData)
{
    mUserSessionTable.upsertField(userExistenceData.getUserSessionId(), PUBLIC_USERSESSION_FIELD, userExistenceData);
}

void UserSessionsMasterImpl::upsertUserSessionData(UserSessionData& userSessionData)
{
    mUserSessionTable.upsertField(userSessionData.getUserSessionId(), PRIVATE_USERSESSION_FIELD, userSessionData);
}

uint64_t UserSessionsMasterImpl::allocateUniqueSliverKey(SliverId sliverId, uint16_t idType)
{
    uint64_t uniqueIdBase = 0;

    BlazeRpcError err = gUniqueIdManager->getId(COMPONENT_ID, idType, uniqueIdBase, SLIVER_KEY_BASE_MIN);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.allocateUniqueId: Failed to allocate a unique id of type(" << idType << ") with err(" << ErrorHelp::getErrorName(err) << ")");
        return BuildSliverKey(INVALID_SLIVER_ID, 0);
    }

    return BuildSliverKey(sliverId, uniqueIdBase);
}

UserSessionId UserSessionsMasterImpl::allocateUniqueUserSessionId(SliverId sliverId)
{
    return allocateUniqueSliverKey(sliverId, USERSESSIONS_ID_TYPE_SESSION);
}

ConnectionGroupId UserSessionsMasterImpl::allocateUniqueConnectionGroupId(SliverId sliverId)
{
    return allocateUniqueSliverKey(sliverId, USERSESSIONS_ID_TYPE_CONNGROUP);
}

void UserSessionsMasterImpl::addLocalUserSession(UserSessionMasterPtr localUserSession, OwnedSliver& ownedSliver)
{
    // For now, the desired single code-path flow should follow a pattern where this method is always called before
    // the UserSessionMaster has been attached to a connection.
    EA_ASSERT(localUserSession->getPeerInfo() == nullptr);

    localUserSession->setOwnedSliverRef(&ownedSliver);

    UserSessionMasterPtrByIdMap::const_iterator it = mUserSessionMasterPtrByIdMap.find(localUserSession->getUserSessionId());
    EA_ASSERT_MSG(it == mUserSessionMasterPtrByIdMap.end(), "Can't clobber existing user session!"); // TODO_MC: replace this with a better check, higher up perhaps?
    mUserSessionMasterPtrByIdMap[localUserSession->getUserSessionId()] = localUserSession;

    BlazeIdUserSessionId blazeIdUserSessionId = {localUserSession->getBlazeId(), localUserSession->getSessionId()};
    mUserSessionsByBlazeId[blazeIdUserSessionId] = localUserSession.get();


    if (localUserSession->getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
    {
        ConnectionGroupData& connectionGroupData = mConnGroupUserSetMap[localUserSession->getConnectionGroupId()];
        connectionGroupData.mUserSessionIdSet.insert(localUserSession->getSessionId());

        localUserSession->mNotificationJobQueue = &connectionGroupData.mNotificationJobQueue;

        if (localUserSession->getSessionFlags().getInLocalUserGroup())
        {
            SessionByConnGroupIdMap::insert_return_type insertRet = mLocalSessionByConnGroupIdMap.insert(localUserSession->getConnectionGroupId());
            ConnectionGroupList& conGroupList = insertRet.first->second;
            if (insertRet.second)
            {
                // generate uuid for this user
                UUID uuid;
                UUIDUtil::generateUUID(uuid);
                localUserSession->setUUID(uuid);
            }
            else
            {
                if (!conGroupList.empty())
                {
                    UserSessionMaster& existingUserSession = conGroupList.front();
                    localUserSession->setUUID(existingUserSession.getUUID());
                }
            }
            conGroupList.push_back(*localUserSession);
        }
    }
    else
    {
        localUserSession->mNotificationJobQueue = BLAZE_NEW NotificationJobQueue("LocalUserSession::NotificationJobQueue");
    }

    localUserSession->mNotificationJobQueue->mSliverId = localUserSession->getSliverId();

    localUserSession->setOrUpdateCleanupTimeout();
    if (gUserSessionManager->getClientTypeDescription(localUserSession->getClientType()).getEnableConnectivityChecks())
        localUserSession->enableConnectivityChecking();

    updateLocalUserSessionCountsAddOrRemoveSession(*localUserSession, true);
}

void UserSessionsMasterImpl::removeLocalUserSession(UserSessionMasterPtr localUserSession)
{
    if (localUserSession->isCoalescedUedUpdatePending())
        localUserSession->sendCoalescedExtendedDataUpdate(true);

    updateLocalUserSessionCountsAddOrRemoveSession(*localUserSession, false);

    localUserSession->disableConnectivityChecking();
    localUserSession->detachPeerInfo();
    localUserSession->cancelCleanupTimeout();

    if (localUserSession->getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
    {
        if (localUserSession->getSessionFlags().getInLocalUserGroup())
        {
            SessionByConnGroupIdMap::iterator connGroupIt = mLocalSessionByConnGroupIdMap.find(localUserSession->getConnectionGroupId());
            if (connGroupIt != mLocalSessionByConnGroupIdMap.end())
            {
                localUserSession->removeFromConnectionGroupList();
                if (connGroupIt->second.empty())
                    mLocalSessionByConnGroupIdMap.erase(connGroupIt);
            }
        }

        ConnGroupUserSetMap::iterator it = mConnGroupUserSetMap.find(localUserSession->getConnectionGroupId());
        if (it != mConnGroupUserSetMap.end())
        {
            ConnectionGroupData& connGroupData = it->second;

            UserSessionIdVectorSet& userSessionIdSet = connGroupData.mUserSessionIdSet;
            userSessionIdSet.erase(localUserSession->getSessionId());
            if (userSessionIdSet.empty())
            {
                connGroupData.mNotificationJobQueue.cancelAllWork();
                mConnGroupUserSetMap.erase(localUserSession->getConnectionGroupId());
            }
        }
    }
    else
    {
        localUserSession->mNotificationJobQueue->cancelAllWork();
        delete localUserSession->mNotificationJobQueue;
    }

    localUserSession->mNotificationJobQueue = nullptr;

    BlazeIdUserSessionId blazeIdUserSessionId = {localUserSession->getBlazeId(), localUserSession->getUserSessionId()};
    mUserSessionsByBlazeId.erase(blazeIdUserSessionId);

    mUserSessionMasterPtrByIdMap.erase(localUserSession->getUserSessionId());

    localUserSession->setOwnedSliverRef(nullptr);
}

BlazeRpcError UserSessionsMasterImpl::finalizeAndInstallNewUserSession(UserSessionMasterPtr newUserSession, OwnedSliver& ownedSliver, PeerInfo* peerInfo, 
    const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded, const NucleusConnect::GetAccessTokenResponse* accessTokenResponse, const char8_t* ipAddr)
{
    // Check to see if this is a request for a parent session
    UserSessionMasterPtr parentSession;
    if (UserSession::isValidUserSessionId(newUserSession->getParentSessionId()))
    {
        parentSession = getLocalSession(newUserSession->getParentSessionId());
        if (parentSession == nullptr)
        {
            // This is really just a sanity check.  Pretty much no way this can happen (currently), but may happen if someone
            // introduced a bug by calling this method with invalid values provided in the newUserSession.
            BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.finalizeAndInstallNewUserSession: Invalid parent session Id " << newUserSession->getParentSessionId() << ".");
            return ERR_SYSTEM;
        }

        if (!newUserSession->isGuestSession())
        {
            if (UserSession::isValidUserSessionId(parentSession->getChildSessionId()))
            {
                BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.finalizeAndInstallNewUserSession: Parent session already cloned! Parent Id " << newUserSession->getParentSessionId() << " Child Id " << parentSession->getChildSessionId() << ".");
                return ERR_SYSTEM;
            }

            parentSession->setChildSessionId(newUserSession->getUserSessionId());
        }
        else
        {
            // This is just an indication that, at the end of this method, there's no need to upsert the parentSession data in the StorageTable.
            parentSession = nullptr;
        }
    }

    GeoLocationData geoLocData;
    if (getConfig().getUseGeoipData())
    {
        // Check for overrides if we are using the database and this session is 
        // not stateless (stateless users cannot override their geolocation data)
        geoIpSucceeded = (gUserSessionManager->convertGeoLocationData(UserSessionManager::isStatelessUser(newUserSession->getBlazeId()) ? INVALID_BLAZE_ID : newUserSession->getBlazeId(), geoLoc, geoLocData) == ERR_OK);
        // GeoIp error is tracked via out parameter geoIpSucceeded, so log a warning (which is already done in convertGeoLocationData) and continue
    }

    newUserSession->setLatitude(geoLocData.getLatitude());
    newUserSession->setLongitude(geoLocData.getLongitude());
    newUserSession->getExtendedData().setCountry(geoLocData.getCountry());   // Country and time zone are only used for gathering metrics
    newUserSession->getExtendedData().setTimeZone(geoLocData.getTimeZone());
    newUserSession->getExtendedData().setISP(geoLocData.getISP());
    newUserSession->mUserInfo->getPlatformInfo().copyInto(newUserSession->mExistenceData->getPlatformInfo());
    newUserSession->mExistenceData->getPlatformInfo().setClientPlatform(newUserSession->mUserInfo->getPlatformInfo().getClientPlatform()); // this is duplication, but PIN event generation on *any* blaze instance expect to get this value without blocking!
    
    newUserSession->mExistenceData->getInstanceIds().reserve(gController->getSlaveTypeCount());
    for (uint8_t index = 0, indexEnd = gController->getSlaveTypeCount(); index < indexEnd; ++index)
    {
        // assign this session to a set of auxslave instances
        // TODO_MC: The local InstanceId is passed in here, but it is meaningless in a slivered world.  Fix!
        newUserSession->mExistenceData->getInstanceIds().push_back(addSessionToAuxSlave(index, gController->getInstanceId()));
    }

    Int256Buffer randBuf;
    gRandom->getStrongRandomNumber256(randBuf);
    randBuf.copyInto(newUserSession->getRandom());

    TimeValue creationTime = TimeValue::getTimeOfDay();
    newUserSession->setCreationTime(creationTime);
    newUserSession->mExistenceData->setCreationTime(creationTime); // (eliminate by: GOS-30204)
    newUserSession->mExistenceData->getSessionFlags().setInLocalUserGroup(newUserSession->canBeInLocalConnGroup());
    newUserSession->mExistenceData->getSessionFlags().setUseNotificationCache(peerInfo == nullptr || !peerInfo->supportsNotifications());
    newUserSession->mExistenceData->getSessionFlags().setIgnoreInactivityTimeout(peerInfo != nullptr && peerInfo->getIgnoreInactivityTimeout());

    // This includes pinning the UserSessionPtr in the mUserSessionMasterPtrByIdMap.
    addLocalUserSession(newUserSession, ownedSliver);

    gUserSessionManager->upsertUserSessionExistence(*newUserSession->mExistenceData);

    UserSession::setCurrentUserSessionId(newUserSession->getUserSessionId());
    Blaze::OAuth::OAuthSlaveImpl* oAuthSlave = static_cast<Blaze::OAuth::OAuthSlaveImpl*>(gController->getComponent(Blaze::OAuth::OAuthSlaveImpl::COMPONENT_ID, true/*reqLocal*/, true/*reqActive*/));
    if (oAuthSlave != nullptr && accessTokenResponse != nullptr)
    {
        if ((accessTokenResponse->getRefreshToken()[0] == '\0' || accessTokenResponse->getRefreshExpiresIn() == 0) && !oAuthSlave->isJwtFormat(accessTokenResponse->getAccessToken()))
        {
            // A refresh token (and its corresponding expiry time) may be omitted from an access token response if the 'offline' scope is not included.
            // Without any expiry time, the access token will be uncached almost immediately-–leading to login issues and/or account-related (or token-related) requests failing.
            BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.finalizeAndInstallNewUserSession: fetched OPAQUE access token for BlazeId ("
                << newUserSession->getBlazeId() << ") does not include refresh token (" << accessTokenResponse->getRefreshToken() << ") or refresh expiry ("
                << accessTokenResponse->getRefreshExpiresIn() << "). (missing 'offline' scope in requestor's client ID?)");
        }

        const char8_t* productName = newUserSession->getProductName();
        if (productName[0] == '\0')
            productName = gController->getServiceProductName(peerInfo->getServiceName());

        oAuthSlave->addUserSessionTokenInfoToCache(*accessTokenResponse, productName, ipAddr);
    }
    ReputationService::ReputationServiceUtilPtr reputationServiceUtil = gUserSessionManager->getReputationServiceUtil();
    reputationServiceUtil->doSessionReputationUpdate(newUserSession->getUserSessionId());

    if (peerInfo != nullptr)
        newUserSession->attachPeerInfo(*peerInfo);

    // Subscribe the new UserSession to itself
    newUserSession->addUser(newUserSession->getBlazeId(), false);
    newUserSession->addNotifier(newUserSession->getUserSessionId(), true);

    upsertUserExistenceData(*newUserSession->mExistenceData);
    upsertUserSessionData(*newUserSession->mUserSessionData);

    mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionLogin, *newUserSession);

    if (parentSession != nullptr)
    {
        upsertUserSessionData(*parentSession->mUserSessionData);
    }

    BLAZE_INFO_LOG(Log::USER, "[UserManager]: User Session Created: "
        "UserSessionId=" << newUserSession->getUserSessionId() << ", "
        "ClientType=" << ClientTypeToString(newUserSession->getClientType()) << ", "
        "ClientPlatform=" << ClientPlatformTypeToString(newUserSession->getClientPlatform()) << ", "
        "BlazeId=" << newUserSession->getBlazeId() << ", "
        "PersonaId=" << newUserSession->getUserInfo().getId() << ", "
        "Persona=" << newUserSession->getUserInfo().getPersonaName() << ", "
        "ConnGroup=" << newUserSession->getConnectionGroupId() << ", "
        "Real Addr=" << (*newUserSession->getConnectionAddr() == '\0' ? "Unknown" : newUserSession->getConnectionAddr()));

    const char8_t* projectId = nullptr;
    auto projectIdIter = mProductProjectIdMap.find(newUserSession->getProductName());
    if (projectIdIter != mProductProjectIdMap.end())
    {
        projectId = projectIdIter->second;
    }

    generateLoginEvent(
        ERR_OK,
        newUserSession->getUserSessionId(),
        newUserSession->getUserSessionType(),
        newUserSession->getPlatformInfo(),
        newUserSession->getUniqueDeviceId(),
        newUserSession->getDeviceLocality(),
        newUserSession->getBlazeId(),
        newUserSession->getUserInfo().getPersonaName(),
        newUserSession->getSessionLocale(),
        newUserSession->getSessionCountry(),
        newUserSession->getBlazeSDKVersion(),
        newUserSession->getClientType(),
        (*newUserSession->getConnectionAddr() == '\0' ? "Unknown" : newUserSession->getConnectionAddr()),
        newUserSession->getServiceName(),
        newUserSession->getProductName(),
        geoLocData,
        projectId,
        newUserSession->getAccountLocale(),
        newUserSession->getAccountCountry(),
        newUserSession->getClientVersion());

    char8_t keyBuf[MAX_SESSION_KEY_LENGTH];
    newUserSession->getKey(keyBuf);

    UserSessionLoginInfo userSessionLoginInfo;
    newUserSession->getUserInfo().getPlatformInfo().copyInto(userSessionLoginInfo.getPlatformInfo());
    userSessionLoginInfo.setSessionKey(             keyBuf);
    userSessionLoginInfo.setConnectionGroupObjectId(newUserSession->getConnectionGroupObjectId());
    userSessionLoginInfo.setUserSessionType(        newUserSession->getUserSessionType());
    userSessionLoginInfo.setBlazeId(                newUserSession->getBlazeId());
    userSessionLoginInfo.setPersonaId(              newUserSession->getBlazeId());
    userSessionLoginInfo.setAccountId(              userSessionLoginInfo.getPlatformInfo().getEaIds().getNucleusAccountId());
    userSessionLoginInfo.setAccountLocale(          newUserSession->getUserInfo().getAccountLocale());
    userSessionLoginInfo.setAccountCountry(         newUserSession->getUserInfo().getAccountCountry());
    userSessionLoginInfo.setDisplayName(            newUserSession->getUserInfo().getPersonaName());
    userSessionLoginInfo.setPersonaNamespace(       newUserSession->getUserInfo().getPersonaNamespace());
    userSessionLoginInfo.setClientPlatform(         userSessionLoginInfo.getPlatformInfo().getClientPlatform());
    userSessionLoginInfo.setLastAuthenticated(      (uint32_t)newUserSession->getUserInfo().getPreviousLoginDateTime());
    userSessionLoginInfo.setLastLoginDateTime(      newUserSession->getUserInfo().getLastLoginDateTime());
    userSessionLoginInfo.setIsFirstLogin(           newUserSession->getSessionFlags().getFirstLogin());
    userSessionLoginInfo.setIsFirstConsoleLogin(    newUserSession->getSessionFlags().getFirstConsoleLogin());
    
    gUserSessionManager->sendUserAuthenticatedToUserSession(newUserSession.get(), &userSessionLoginInfo, true);

    return ERR_OK;
}

BlazeRpcError UserSessionsMasterImpl::createNewUserSessionInternal(const UserInfoData& userData, PeerInfo& peerInfo, ClientType clientType, int32_t userIndex, 
    const char8_t* productName, const char8_t* deviceId, DeviceLocality deviceLocality, const char8_t* accessScope, const SessionFlags& sessionFlags, bool isTrustedLogin,
    const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded, const NucleusConnect::GetAccessTokenResponse* accessTokenResponse, const char8_t* ipAddr,
    UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool dbUpsertCompleted, bool updateCrossPlatformOpt)
{
    VERIFY_WORKER_FIBER();

    if (clientType == CLIENT_TYPE_INVALID)
        clientType = peerInfo.getClientType();

    //check if exceeds the max number of concurrent user sessions per user by BlazeId
    if (clientType == CLIENT_TYPE_HTTP_USER)
    {
        UserSessionIdList httpUserSessionIds;
        gUserSessionManager->getUserSessionIdsByBlazeId(userData.getId(), httpUserSessionIds, &ClientTypeFlags::getClientTypeHttpUser);
        if (httpUserSessionIds.size() >= getConfig().getMaxConcurrentHttpSessions())
        {
            BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: User(" << userData.getId() << ")'s concurrent sessions for connecting to the "
                "HTTP endpoint are more than the configured limit of " << getConfig().getMaxConcurrentHttpSessions() << ". User session Ids in use (" << httpUserSessionIds << ")");
            return ERR_SYSTEM;
        }
    }

    bool sessionDuplicateLogin = false;
    if (UserSession::getCurrentUserSessionId() != INVALID_USER_SESSION_ID)
    {
        sessionDuplicateLogin = true;
        destroyUserSession(UserSession::getCurrentUserSessionId(), DISCONNECTTYPE_DUPLICATE_LOGIN, 0, FORCED_LOGOUTTYPE_LOGGEDIN_ELSEWHERE);
    }

    OwnedSliver* ownedSliver = nullptr;
    if (peerInfo.getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
        ownedSliver = mUserSessionTable.getSliverOwner().getOwnedSliver(GetSliverIdFromSliverKey(peerInfo.getConnectionGroupId()));
    else if (gController->isInService())
        ownedSliver = mUserSessionTable.getSliverOwner().getLeastLoadedOwnedSliver();

    if (ownedSliver == nullptr)
    {
        if (!gController->isInService())
        {
            BLAZE_TRACE_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: While not in-service, an attempt to obtain a sliver for use did not succeed; "
                "User(" << userData.getPersonaName() << "), BlazeId(" << userData.getId() << ")");
        }
        else
        {
            BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: An attempt to obtain a sliver for use did not succeed; "
                "User(" << userData.getPersonaName() << "), BlazeId(" << userData.getId() << ")");
        }
        
        if (peerInfo.isPersistent())
        {
            char8_t peerInfoBuf[256];
            peerInfo.toString(peerInfoBuf, sizeof(peerInfoBuf));
            BLAZE_INFO_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Close persistent client inbound connection [" << peerInfoBuf
                << "] when no sliver is available. SessionDuplicateLogin(" << sessionDuplicateLogin << ")");
            peerInfo.closePeerConnection(DISCONNECTTYPE_CLOSE_NORMAL, true);
        }
        return Blaze::ERR_MOVED;
    }

    Sliver::AccessRef sliverAccess;
    if (!ownedSliver->getPrioritySliverAccess(sliverAccess))
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Could not get priority sliver access for SliverId(" << ownedSliver->getSliverId() << ")");
        return ERR_SYSTEM;
    }
    if (peerInfo.isPersistent() && (peerInfo.getConnectionGroupId() == INVALID_CONNECTION_GROUP_ID))
    {
        peerInfo.setConnectionGroupId(allocateUniqueConnectionGroupId(ownedSliver->getSliverId()));
    }

    StringBuilder singleLoginKey;
    makeSingleLoginKey(singleLoginKey, userData.getPlatformInfo().getEaIds().getNucleusAccountId(), userData.getPlatformInfo().getClientPlatform());

    const ClientTypeFlags& clientTypeFlags = gUserSessionManager->getClientTypeDescription(clientType);

    RedisAutoLock singleLoginLock(singleLoginKey.get());
    if (clientTypeFlags.getEnforceSingleLogin())
    {
        RedisError redisErr = singleLoginLock.lock();
        if (redisErr != REDIS_ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER,"UserSessionsMasterImpl.createUserSession: Unexpected Redis error Error " << redisErr << " occurred when locking singleLoginLock." ); 
            return ERR_SYSTEM;
        }
        SingleLoginIdentity otherIdentity;
        otherIdentity.first = INVALID_BLAZE_ID;
        otherIdentity.second = INVALID_USER_SESSION_ID;
        redisErr = mSingleLoginIds.find(singleLoginLock.getKey(), otherIdentity);
        if (redisErr != REDIS_ERR_NOT_FOUND)
        {
            if (redisErr != REDIS_ERR_OK)
            {
                BLAZE_ERR_LOG(Log::USER,"UserSessionsMasterImpl.createUserSession: Unexpected Redis error Error " << redisErr << " occurred when finding login id." ); 
                return ERR_SYSTEM;
            }

            // if the existing login is the same persona, or we're enforcing single logins per-account, we force the old usersession to log out.
            // if it's a different persona on the same platform, we kick the other persona's session offline unless the Blaze server is configured to prevent the subsequent login
            if (getConfig().getSingleLoginCheckPerAccount() || (otherIdentity.first == userData.getId()) || getConfig().getForceMultiLinkedSessionOffline())
            {
                ForceSessionLogoutRequest request;
                request.setSessionId(otherIdentity.second);
                request.setReason(DISCONNECTTYPE_DUPLICATE_LOGIN);
                request.setForcedLogoutReason(FORCED_LOGOUTTYPE_LOGGEDIN_ELSEWHERE);

                BlazeRpcError err = gUserSessionMaster->forceSessionLogoutMaster(request);
                if ((err != ERR_OK) && (err != USER_ERR_SESSION_NOT_FOUND))
                {
                    BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Error " << ErrorHelp::getErrorName(err) << " occurred when forceSessionLogoutMaster was called.");
                    return ERR_SYSTEM;
                }
            }
            else // have to block the new login if we're not able to bounce the previous session offline
            {
                BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Error " << ErrorHelp::getErrorName(ERR_DUPLICATE_LOGIN) 
                    << " occurred for user(" << userData.getPersonaName() << "), BlazeId(" << userData.getId() << "), BlazeId(" 
                    << otherIdentity.first << ") was logged in from the same account and platform.");
                return ERR_DUPLICATE_LOGIN;
            }
        }
    }
    else if (clientType == CLIENT_TYPE_HTTP_USER)
    {
        

        // Primary persona status is stored in the userinfo table (excludes Trusted Login users).
        // Retrieving userinfo is only possible for users logging in via a hosted platform (with a schema that contains the userinfo table)
        if (!isTrustedLogin && gController->isPlatformHosted(userData.getPlatformInfo().getClientPlatform()))
        {
            // The duplicate login prevention above isn't suited to session types that don't enforce a single login (e.g. CLIENT_TYPE_HTTP_USER)
            // since a user can have multiple active HTTP sessions. Block any HTTP persona that is not the primary persona.

            // We still check the single login table to ensure the user isn't concurrently logged in with a different persona on the same platform/account from a client type that enforces single login
            SingleLoginIdentity otherIdentity;
            otherIdentity.first = INVALID_BLAZE_ID;
            otherIdentity.second = INVALID_USER_SESSION_ID;
            RedisError redisErr = mSingleLoginIds.find(singleLoginLock.getKey(), otherIdentity);
            if (redisErr != REDIS_ERR_NOT_FOUND)
            {
                if (redisErr != REDIS_ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Unexpected Redis error Error " << redisErr << " occurred when finding login id.");
                    return ERR_SYSTEM;
                }

                // if the existing login is the same persona everything is fine. If the title allows concurrent logins on different platforms we'll not have reached this code, since the lock's key will be different
                if (otherIdentity.first != userData.getId())
                {
                    // another persona on the same platform (or for the same account, depending on config) is currently authenticated. Fail login- we don't let a WAL session boot a regular session, ever
                    BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Error " << ErrorHelp::getErrorName(ERR_DUPLICATE_LOGIN)
                        << " occurred when authenticating a CLIENT_TYPE_HTTP_USER session for user(" << userData.getPersonaName() << "), BlazeId(" << userData.getId() << "), account is already logged with BlazeId("
                        << otherIdentity.first << ").");
                    return ERR_DUPLICATE_LOGIN;
                }
            }


            bool isPrimaryPersona = false;
            BlazeRpcError lookupErr = UserInfoDbCalls::lookupIsPrimaryPersonaById(userData.getId(), userData.getPlatformInfo().getClientPlatform(), isPrimaryPersona);
            if (lookupErr == ERR_OK)
            {
                if (!isPrimaryPersona && !(getConfig().getForceMultiLinkedSessionOffline() && getConfig().getPrimaryPersonaUpdatableByWalAuthentication()))
                {
                    // this isn't the primary persona.
                    BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Blocking login for user(" << userData.getPersonaName()
                        << "), BlazeId(" << userData.getId() << ") that is not the primary persona for Nucleus account(" << userData.getPlatformInfo().getEaIds().getNucleusAccountId() << ").");
                    return ERR_NOT_PRIMARY_PERSONA;
                }
            }
            else if (lookupErr == USER_ERR_USER_NOT_FOUND)
            {

                // new user, make sure the nucleus account doesn't already have a primary persona for this platform
                BlazeId primaryBlazeId = INVALID_BLAZE_ID;
                // this is skipped if we won't allow the new login to kick existing sessions offline, since we handle non-primary logins later, and that does similar DB lookups, no need to repeat.
                BlazeRpcError primaryLookupErr = UserInfoDbCalls::lookupPrimaryPersonaByAccountId(userData.getPlatformInfo().getEaIds().getNucleusAccountId(), userData.getPlatformInfo().getClientPlatform(), primaryBlazeId);
                if (primaryLookupErr == USER_ERR_USER_NOT_FOUND)
                {
                    // couldn't find a primary persona, continue
                    BLAZE_TRACE_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Assuming new user(" << userData.getPersonaName()
                        << "), BlazeId(" << userData.getId() << ") has no primary persona for Nucleus account(" << userData.getPlatformInfo().getEaIds().getNucleusAccountId()
                        << ") on platform '" << ClientPlatformTypeToString(userData.getPlatformInfo().getClientPlatform()) << "'.");
                }
                else
                {
                    // primaryBlazeId should never equal the current user's Blaze id, since lookupErr would have returned ERR_OK at the start of the outer if-else block, but verify it anyhow.
                    if ((primaryLookupErr == ERR_OK) && (primaryBlazeId != userData.getId()) 
                        && !(getConfig().getForceMultiLinkedSessionOffline() && getConfig().getPrimaryPersonaUpdatableByWalAuthentication()))
                    {
                        // there is already a primary persona, fail
                        BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Blocking login for user(" << userData.getPersonaName()
                            << "), BlazeId(" << userData.getId() << ") that is not the primary persona for Nucleus account(" << userData.getPlatformInfo().getEaIds().getNucleusAccountId() << ").");
                        return ERR_NOT_PRIMARY_PERSONA;
                    }

                }
                
            }
            else
            {
                BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: UserInfoDbCalls::lookupIsPrimaryPersonaById() failed with lookupErr(" << Blaze::ErrorHelp::getErrorName(lookupErr) << ")");
                return lookupErr;
            }
        }
    }

    BlazeRpcError err = ERR_OK;
    UserInfoPtr sessionUserInfo;

    bool upsertUserInfoNeeded = true;
    if (isTrustedLogin)
        upsertUserInfoNeeded = false;
    else if (!gController->isPlatformHosted(userData.getPlatformInfo().getClientPlatform())) // Upserting userinfo is only possible for users logging in via a hosted platform (with a schema that contains the userinfo table)
        upsertUserInfoNeeded = false;
    else if (!clientTypeFlags.getUpdateUserInfoData())
    {
        // We want to keep the record in the userinfo table 'relatively' up to date, but we don't want to spam
        // the DB with updates for the same user, when the only thing that's changed is the login date/time stamp.
        // So, we do not upsert the userinfo table if the last known login was within the last 5 seconds,
        // unless the existing userinfo is marked as non-primary.
        UserInfo* existingUserInfo = gUserSessionManager->findResidentUserInfoByBlazeId(userData.getId());
        if ((existingUserInfo != nullptr) && ((TimeValue::getTimeOfDay().getSec() - existingUserInfo->getLastLoginDateTime() < 5) || !existingUserInfo->getIsPrimaryPersona()))
        {
            upsertUserInfoNeeded = false;
        }
    }

    if (upsertUserInfoNeeded)
    {
        if (!dbUpsertCompleted)
        {
            err = gUserSessionManager->upsertUserAndAccountInfo(userData, upsertUserInfoResults, updateCrossPlatformOpt, clientTypeFlags.getUpdateUserInfoData());
            if (err != ERR_OK)
            {
                BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: gUserSessionManager->upsertUserAndAccountInfo() failed with err(" << Blaze::ErrorHelp::getErrorName(err) << ")");
                return err; // if USER_ERR_EXISTS is returned here, doConsoleRename is called from LoginCommand
            }
        }

        // Use DB master to lookup to avoid replication lag if DB tech does not have synchronous write replication. 
        err = gUserSessionManager->lookupUserInfoByBlazeId(userData.getId(), sessionUserInfo, false, false, userData.getPlatformInfo().getClientPlatform());
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: gUserSessionManager->lookupUserInfoByBlazeId() failed with err(" << Blaze::ErrorHelp::getErrorName(err) << ")");
            return err;
        }

        BlazeIdList conflictingBlazeIds;
        err = UserInfoDbCalls::resolveMultiLinkedAccountConflicts(userData, conflictingBlazeIds);
        if (err == ERR_OK)
        {
            // If there are different HTTP personas on the same platform, we force the old HTTP usersession to log out.
            // (Conflicting non-HTTP single-login users were already handled above.)
            for (const BlazeId conflictingBlazeId : conflictingBlazeIds)
            {
                UserSessionIdList conflictingHttpUserSessionIds;
                gUserSessionManager->getUserSessionIdsByBlazeId(conflictingBlazeId, conflictingHttpUserSessionIds, &ClientTypeFlags::getClientTypeHttpUser);
                for (const UserSessionId conflictHttpUserSessionId : conflictingHttpUserSessionIds)
                {
                    ForceSessionLogoutRequest request;
                    request.setSessionId(conflictHttpUserSessionId);
                    request.setReason(DISCONNECTTYPE_DUPLICATE_LOGIN);
                    request.setForcedLogoutReason(FORCED_LOGOUTTYPE_LOGGEDIN_ELSEWHERE);

                    BlazeRpcError httpErr = gUserSessionMaster->forceSessionLogoutMaster(request);
                    if ((httpErr != ERR_OK) && (httpErr != USER_ERR_SESSION_NOT_FOUND))
                    {
                        BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Error " << ErrorHelp::getErrorName(httpErr) << " occurred when forceSessionLogoutMaster was called for conflicting HTTP persona.");
                        return ERR_SYSTEM;
                    }
                }
            }
        }
        // USER_NOT_FOUND means that resolveMultiLinkedAccountConflicts didn't resolve any conflicts
        else if (err != USER_ERR_USER_NOT_FOUND)
        {    
            BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: UserInfoDbCalls::resolveMultiLinkedAccountConflicts() failed with err(" << Blaze::ErrorHelp::getErrorName(err) << ")");
            return err;
        }
        
        // NOTE: User state that is not stored in our DB needs to be set explicitly here
        sessionUserInfo->setStopProcess(userData.getStopProcess());
        sessionUserInfo->setVoipDisabled(userData.getVoipDisabled());
    }
    else
    {
        sessionUserInfo = BLAZE_NEW UserInfo();
        userData.copyInto(*sessionUserInfo);
    }

    // IMPORTANT: Do not reference 'userData' anymore unless you are looking for potentially old/incorrect details about the user.
    //            We've fully initialized a new 'sessionUserInfo' instance that contains the most up-to-date
    //            data for the user logging in.  So use 'sessionUserInfo' instead of 'userData'.

    AccessGroupName tmpAccessGroupName;
    if (!resolveAccessGroup(clientType, peerInfo, accessScope, tmpAccessGroupName))
    {
        BLAZE_WARN_LOG(Log::USER,"[UserSessionsMasterImpl].createUserSession: User(" << sessionUserInfo->getId() << ") failure resolving authorization group. See previous errors for details.");
    }
    else if (peerInfo.getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
    {
        BLAZE_TRACE_LOG(Log::USER,"[UserSessionsMasterImpl].createUserSession: User(" << sessionUserInfo->getId() << ") resolved to AuthGroup(" << tmpAccessGroupName << ")");
    }
    
    UserSessionId newUserSessionId = allocateUniqueUserSessionId(ownedSliver->getSliverId());
    if (!UserSession::isValidUserSessionId(newUserSessionId))
    {
        BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl].createUserSession: User(" << sessionUserInfo->getId() << ") failed to allocateUniqueUserSessionId!");
        return ERR_SYSTEM;
    }

    UserSessionMasterPtr newUserSession = BLAZE_NEW UserSessionMaster(newUserSessionId, sessionUserInfo);

    newUserSession->setUserSessionType(isTrustedLogin ? USER_SESSION_TRUSTED : USER_SESSION_NORMAL);
    newUserSession->setAuthGroupName(tmpAccessGroupName.c_str());

    newUserSession->setServiceName(peerInfo.getServiceName());
    if (*newUserSession->getServiceName() == '\0')
        newUserSession->setServiceName(gController->getDefaultServiceName(newUserSession->getClientPlatform()));

    newUserSession->setProductName(productName);
    if (*newUserSession->getProductName() == '\0')
        newUserSession->setProductName(gController->getServiceProductName(newUserSession->getServiceName()));

    if (isTrustedLogin)
        newUserSession->mUserSessionData->setTrustedLoginId(sessionUserInfo->getPersonaName());

    newUserSession->mExistenceData->getSessionFlags().setBits(sessionFlags.getBits());
    newUserSession->mExistenceData->getSessionFlags().setFirstLogin(upsertUserInfoResults.newRowInserted);
    newUserSession->mExistenceData->getSessionFlags().setFirstConsoleLogin(upsertUserInfoResults.firstSetExternalData);
    newUserSession->mExistenceData->getSessionFlags().setIsChildAccount(sessionUserInfo->getIsChildAccount());
    newUserSession->mExistenceData->getSessionFlags().setStopProcess(sessionUserInfo->getStopProcess());
    newUserSession->mExistenceData->setClientIp(peerInfo.getRealPeerAddress().getIp());

    newUserSession->setBlazeId(sessionUserInfo->getId());
    sessionUserInfo->getPlatformInfo().copyInto(newUserSession->getPlatformInfo());
    newUserSession->setAccountId(sessionUserInfo->getPlatformInfo().getEaIds().getNucleusAccountId());
    newUserSession->setPersonaName(sessionUserInfo->getPersonaName());
    newUserSession->setPersonaNamespace(sessionUserInfo->getPersonaNamespace());
    newUserSession->setClientPlatform(newUserSession->getPlatformInfo().getClientPlatform());
    newUserSession->setUniqueDeviceId(deviceId);
    newUserSession->setDeviceLocality(deviceLocality);
    newUserSession->setAccountLocale(sessionUserInfo->getAccountLocale());
    newUserSession->setAccountCountry(sessionUserInfo->getAccountCountry());
    newUserSession->setPreviousAccountCountry(upsertUserInfoResults.previousCountry);

    char8_t realPeerAddr[32];
    newUserSession->setConnectionUserIndex(userIndex);
    newUserSession->setClientType(clientType);
    newUserSession->setSessionLocale(peerInfo.getLocale());
    newUserSession->setSessionCountry(peerInfo.getCountry());
    newUserSession->setConnectionAddr(peerInfo.getRealPeerAddress().toString(realPeerAddr, sizeof(realPeerAddr)));
    newUserSession->setConnectionGroupId(peerInfo.getConnectionGroupId());
    if (newUserSession->getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
        newUserSession->getExtendedData().getBlazeObjectIdList().push_back(EA::TDF::ObjectId(ENTITY_TYPE_CONN_GROUP, newUserSession->getConnectionGroupId()));
    if (peerInfo.getClientInfo() != nullptr)
    {
        ClientInfo& clientInfo = *peerInfo.getClientInfo();

        newUserSession->setClientVersion(clientInfo.getClientVersion());
        newUserSession->setBlazeSDKVersion(clientInfo.getBlazeSDKVersion());
        newUserSession->setProtoTunnelVer(clientInfo.getProtoTunnelVersion());
        clientInfo.copyInto(newUserSession->getClientInfo());
    }

    // Allow listeners to fill in extended data for the session.
    // Omit dedicated servers, and stateless http express login users who won't have pre-existing extended data.
    if (clientTypeFlags.getHasUserExtendedData() && !isTrustedLogin)
    {
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        err = gUserSessionManager->requestUserExtendedData(*sessionUserInfo, newUserSession->getExtendedData(), false);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].createUserSession: Fail to load user extended data!");
            return err;
        }
    }

    // Set ExtendedData to defaults if missing
    UserExtendedDataMap& dataMap = newUserSession->getExtendedData().getDataMap();
    for (UEDClientConfigMap::const_iterator it = mUEDClientConfigMap.begin(), end = mUEDClientConfigMap.end(); it != end; ++it)
    {
        uint32_t clientKey = (uint32_t)it->first;
        UserExtendedDataMap::const_iterator clientUEDIter = dataMap.find(clientKey);
        if (clientUEDIter == dataMap.end())
        {
            const UserSessionClientUEDConfig* config = it->second;
            UserExtendedDataKey key = UED_KEY_FROM_IDS(Component::INVALID_COMPONENT_ID, config->getId());
            dataMap[key] = config->getDefault();
        }
    }

    err = finalizeAndInstallNewUserSession(newUserSession, *ownedSliver, &peerInfo, geoLoc, geoIpSucceeded, accessTokenResponse, ipAddr);
    if (err == ERR_OK)
    {
        EA_ASSERT(gUserSessionManager->getSession(newUserSession->getUserSessionId()) != nullptr);

        if (singleLoginLock.hasLock())
        {
            SingleLoginIdentity newSessionIdentity;
            newSessionIdentity.first = newUserSession->getBlazeId();
            newSessionIdentity.second = newUserSession->getUserSessionId();
            RedisError redisErr = mSingleLoginIds.upsert(singleLoginLock.getKey(), newSessionIdentity);
            if (redisErr != REDIS_ERR_OK)
            {
                destroyUserSession(newUserSessionId, DISCONNECTTYPE_ERROR_NORMAL, 0, FORCED_LOGOUTTYPE_INVALID);
                err = ERR_SYSTEM;
            }
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createUserSession: Could not finalize and install a new UserSession with err(" << ErrorHelp::getErrorName(err) << ")");
    }

    return err;
}

BlazeRpcError UserSessionsMasterImpl::createNormalUserSession(const UserInfoData& userData, PeerInfo& connection, ClientType clientType, int32_t userIndex,
    const char8_t* productName, bool skipPeriodicEntitlementCheck, const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded,
    UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool dbUpsertCompleted, bool updateCrossPlatformOpt, const char8_t* deviceId/* = ""*/, DeviceLocality deviceLocality,
    const NucleusConnect::GetAccessTokenResponse* accessTokenResponse, const char8_t* ipAddr)
{
    SessionFlags sessionFlags;
    sessionFlags.setSkipPeriodicEntitlementStatusCheck(skipPeriodicEntitlementCheck);

    return createNewUserSessionInternal(userData, connection, clientType, userIndex, productName, deviceId, deviceLocality, "", sessionFlags, false, geoLoc, geoIpSucceeded, accessTokenResponse, ipAddr, upsertUserInfoResults, dbUpsertCompleted, updateCrossPlatformOpt);
}

BlazeRpcError UserSessionsMasterImpl::createTrustedUserSession(const UserInfoData& userData, PeerInfo& connection, ClientType clientType, int32_t userIndex,
    const char8_t* productName, const char8_t* accessScope, const NucleusIdentity::IpGeoLocation& geoLoc, bool& geoIpSucceeded)
{
    SessionFlags sessionFlags;
    sessionFlags.setSkipPeriodicEntitlementStatusCheck();

    UserInfoDbCalls::UpsertUserInfoResults upsertUserInfoResults(userData.getAccountCountry());
    return createNewUserSessionInternal(userData, connection, clientType, userIndex, productName, "", DEVICELOCALITY_UNKNOWN, accessScope, sessionFlags, true, geoLoc, geoIpSucceeded, nullptr, "", upsertUserInfoResults, false, false);
}

BlazeRpcError UserSessionsMasterImpl::createWalUserSession(const UserSessionMaster& parentUserSession, UserSessionMasterPtr& childWalUserSession, bool& geoIpSucceeded)
{
    BlazeRpcError err = ERR_SYSTEM;

    if (!parentUserSession.canBeParentToWalCloneSession())
    {
        return err;
    }

    // Make sure we aren't already cloned
    if (UserSession::isValidUserSessionId(parentUserSession.getChildSessionId()))
    {
        childWalUserSession = getLocalSession(parentUserSession.getChildSessionId());
        if (childWalUserSession != nullptr)
            err = ERR_OK;
    }
    else
    {
        OwnedSliver* ownedSliver = mUserSessionTable.getSliverOwner().getOwnedSliver(parentUserSession.getSliverId());
        if (ownedSliver != nullptr)
        {
            Sliver::AccessRef sliverAccess;
            if (ownedSliver->getPrioritySliverAccess(sliverAccess))
            {
                UserSessionId userSessionId = allocateUniqueUserSessionId(ownedSliver->getSliverId());
                if (!UserSession::isValidUserSessionId(userSessionId))
                {
                    BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl].createWalUserSession: User(" << parentUserSession.getId() << ") failed to allocateUniqueUserSessionId!");
                    return ERR_SYSTEM;
                }

                // Make a new session, which is initialized from all the values in the parent session.
                UserSessionMasterPtr newUserSession = BLAZE_NEW UserSessionMaster(parentUserSession);

                // Now, override values specific to this session (relative to the parent session that provided the initial state).
                newUserSession->setUserSessionId(userSessionId);
                newUserSession->setParentSessionId(parentUserSession.getUserSessionId());
                newUserSession->setClientType(CLIENT_TYPE_HTTP_USER);
                newUserSession->setConnectionGroupId(INVALID_CONNECTION_GROUP_ID);

                newUserSession->setUserSessionType(USER_SESSION_WAL_CLONE);

                NucleusIdentity::IpGeoLocation geoLoc;
                geoLoc.setLatitude(eastl::to_string(parentUserSession.getLatitude()).c_str());
                geoLoc.setLongitude(eastl::to_string(parentUserSession.getLongitude()).c_str());
                geoLoc.setCountry(parentUserSession.getExtendedData().getCountry());
                geoLoc.setTimeZone(parentUserSession.getExtendedData().getTimeZone());
                geoLoc.setISPName(parentUserSession.getExtendedData().getISP());

                err = finalizeAndInstallNewUserSession(newUserSession, *ownedSliver, nullptr, geoLoc, geoIpSucceeded, nullptr, "");
                if (err == ERR_OK)
                    childWalUserSession = newUserSession;
            }
            else
            {
                BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createWalUserSession: Could not get priority sliver access for SliverId(" << ownedSliver->getSliverId() << ")");
                err = ERR_SYSTEM;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl].createWalUserSession: unable to find a sliver for parent " << parentUserSession.getSliverId() << "." ); 
        }
    }

    return err;
}

BlazeRpcError UserSessionsMasterImpl::createGuestUserSession(const UserSessionMaster& parentUserSession, UserSessionMasterPtr& childGuestUserSession, PeerInfo& connection, uint32_t userIndex)
{
    if (!parentUserSession.canBeParentToGuestSession())
    {
        BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.createGuestUserSession: The parent UserSession(" << parentUserSession.getUserSessionId() << "), "
            "which of Type(" << UserSessionTypeToString(parentUserSession.getUserSessionType()) << ") and ClientType(" << ClientTypeToString(parentUserSession.getClientType()) << "), "
            "cannot be a parent to a guest session.");
        return USER_ERR_INVALID_SESSION_INSTANCE;
    }

    OwnedSliver* ownedSliver = mUserSessionTable.getSliverOwner().getOwnedSliver(GetSliverIdFromSliverKey(parentUserSession.getPeerInfo()->getConnectionGroupId()));
    if (ownedSliver == nullptr)
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createGuestUserSession: Could not find the owned sliver with SliverId(" << GetSliverIdFromSliverKey(parentUserSession.getPeerInfo()->getConnectionGroupId()) << ") "
            "for ConnectionGroupId(" << parentUserSession.getPeerInfo()->getConnectionGroupId() << ")");
        return ERR_SYSTEM;
    }

    Sliver::AccessRef sliverAccess;
    if (!ownedSliver->getPrioritySliverAccess(sliverAccess))
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createGuestUserSession: Could not get priority sliver access for SliverId(" << ownedSliver->getSliverId() << ")");
        return ERR_SYSTEM;
    }

    UserSessionId userSessionId = allocateUniqueUserSessionId(ownedSliver->getSliverId());
    if (!UserSession::isValidUserSessionId(userSessionId))
    {
        BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl].createGuestUserSession: User(" << parentUserSession.getId() << ") failed to allocateUniqueUserSessionId!");
        return ERR_SYSTEM;
    }

    // Make a new session, which is initialized from all the values in the parent session.
    UserSessionMasterPtr newUserSession = BLAZE_NEW UserSessionMaster(parentUserSession);

    // Now, override values specific to this session (relative to the parent session that provided the initial state).
    newUserSession->setUserSessionId(userSessionId);
    newUserSession->setConnectionUserIndex(userIndex);
    newUserSession->setParentSessionId(parentUserSession.getUserSessionId());
    newUserSession->mExistenceData->getSessionFlags().setFirstLogin(false);
    newUserSession->mExistenceData->getSessionFlags().setFirstConsoleLogin(false);

    // Guest sessions do not have a real BlazeId. They are initialized with a negative BlazeId, which doesn't map to any
    // real user.  This is to prevent collisions in maps keyed off of a UserSession's ::getBlazeId().  Note, however, the associated *real*
    // BlazeId used when creating the guest session can be found in UserSessionMaster::getUserInfo().getId().
    newUserSession->setBlazeId(gUserSessionManager->allocateNegativeId());

    newUserSession->setUserSessionType(USER_SESSION_GUEST);

    bool geoIpSucceeded = true;
    NucleusIdentity::IpGeoLocation geoLoc;
    BlazeRpcError err = finalizeAndInstallNewUserSession(newUserSession, *ownedSliver, &connection, geoLoc, geoIpSucceeded, nullptr, "");
    if (err == ERR_OK)
    {
        EA_ASSERT(gUserSessionManager->getSession(newUserSession->getUserSessionId()) != nullptr);

        childGuestUserSession = newUserSession;
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.createGuestUserSession: Could not finalize and install a new UserSession with err(" << ErrorHelp::getErrorName(err) << ")");
        err = ERR_SYSTEM;
    }

    return err;
}

BlazeRpcError UserSessionsMasterImpl::destroyUserSession(UserSessionId userSessionId, UserSessionDisconnectType disconnectType, int32_t socketError, UserSessionForcedLogoutType forcedLogoutType)
{
    // destroyUserSession() should never perform blocking operation itself.  It needs to remove the UserSession from all
    // indices, and destroyUserSession() for any child UserSessions.  Basically, everything that can be done in a non-blocking
    // manner to cleanup the UserSession.  At the bottom of this method, a job is scheduled to run on a blocking fiber that
    // will perform all necessary blocking operations.
    Fiber::BlockingSuppressionAutoPtr supressBlocking;

    LogContextOverride logContextOverride(userSessionId);

    UserSessionMasterPtr userSession = getLocalSession(userSessionId);
    if (EA_UNLIKELY(userSession == nullptr))
    {
        BLAZE_TRACE_LOG(Log::USER, "UserSessionsMasterImpl.destroyUserSession: Cannot find a local UserSession with id(" << userSessionId << "), "
            "disconnectType(" << UserSessionDisconnectTypeToString(disconnectType) << "), socketError(" << socketError << ")");
        return USER_ERR_SESSION_NOT_FOUND;
    }

    Sliver::AccessRef priorityAccess;
    SliverPtr owningSliver = gSliverManager->getSliver(userSession->getSliverIdentity());
    if ((owningSliver != nullptr) && !owningSliver->getPriorityAccess(priorityAccess))
    {
        // We get here under very rare circumstances, and the following log entry shouldn't be seen too often (relative to the PSU).
        // Just before a UserSession is removed from this instance in remove, any outstanding notifications (e.g. UED updates) are flushed.
        // The act of flushing notifications calls BlockingSocket::send(). But, if the client has *just closed the socket*, then the call
        // to ::write() will pickup the fact that the socket has been closed.  This, in turn, will wake up the Connection::readLoop, which will
        // trigger the connection tear down logic/event, which includes destroying all UserSessions attached to the connection. The timing
        // for this situation to occur should be exceedingly rare, but is entirely possible.
        eastl::string callstack;
        CallStack::getCurrentStackSymbols(callstack);
        BLAZE_INFO_LOG(Log::USER, "UserSessionsMasterImpl.destroyUserSession: Could not destroy UserSession(" << userSessionId << ") because it is being migrated; "
            "disconnectType(" << UserSessionDisconnectTypeToString(disconnectType) << "), socketError(" << socketError << "). This should be a rare event.\n"
            "Callstack:\n" << SbFormats::PrefixAppender("    ", callstack.c_str()) << "\n");
        return ERR_MOVED;
    }

    for (const auto& auxInstanceId : userSession->getInstanceIds())
    {
        if (auxInstanceId != INVALID_INSTANCE_ID)
        {
            // unlink this session from auxslave instance it's assigned to
            removeSessionFromAuxSlave(auxInstanceId);
        }
    }

    // Delete any child sessions
    if (UserSession::isValidUserSessionId(userSession->getChildSessionId()))
    {
        BLAZE_TRACE_LOG(Log::USER, "UserSessionsMasterImpl::removeSession() Destroying child session [" << userSession->getChildSessionId() << "]");

        UserSessionMasterPtr childSession = getLocalSession(userSession->getChildSessionId());
        if (childSession != nullptr)
        {
            if (childSession->getUserSessionType() != USER_SESSION_WAL_CLONE)
            {
                BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.destroyUserSession: UserSessionId(" << userSessionId << ") has a ChildSessionId(" << userSession->getChildSessionId() << ") that is not a USER_SESSION_WAL_CLONE type.  This should not happen.");
            }

            destroyUserSession(childSession->getUserSessionId(), disconnectType, socketError, forcedLogoutType);
        }
        else
        {
            BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.destroyUserSession: UserSessionId(" << userSessionId << ") has a ChildSessionId(" << userSession->getChildSessionId() << ") but that child UserSession cannot be found.  This should not happen.");
        }
    }

    if (!userSession->isGuestSession())
    {
        ConnGroupUserSetMap::iterator cgIt = mConnGroupUserSetMap.find(userSession->getConnectionGroupId());
        if (cgIt != mConnGroupUserSetMap.end())
        {
            UserSessionIdVectorSet guestSessionsToRemove;

            UserSessionIdVectorSet& userSessionIdSet = cgIt->second.mUserSessionIdSet;
            for (UserSessionIdVectorSet::iterator i = userSessionIdSet.begin(), e = userSessionIdSet.end(); i != e; ++i)
            {
                UserSessionMasterPtr connGroupMemberUserSession = getLocalSession(*i);
                if ((connGroupMemberUserSession != nullptr) && (connGroupMemberUserSession->getParentSessionId() == userSession->getUserSessionId()))
                {
                    guestSessionsToRemove.insert(*i);
                }
            }

            for (UserSessionIdVectorSet::iterator i = guestSessionsToRemove.begin(), e = guestSessionsToRemove.end(); i != e; ++i)
            {
                destroyUserSession(*i, DISCONNECTTYPE_PARENT_SESSION_LOGOUT, socketError, FORCED_LOGOUTTYPE_INVALID);
            }
        }
    }

    if (userSession->getUserSessionType() == USER_SESSION_WAL_CLONE)
    {
        // If we're destroying a cloned UserSession, then we need to update its parent's UserSession::mChildSessionId,
        // which should currently be set to this UserSessionId.
        UserSessionMasterPtr parentUserSession = getLocalSession(userSession->getParentSessionId());
        if (parentUserSession != nullptr)
        {
            if (parentUserSession->getChildSessionId() != userSession->getUserSessionId())
            {
                BLAZE_ASSERT_LOG(Log::USER, "UserSessionsMasterImpl.destroyUserSession: UserSessionId(" << userSessionId << ") has a ParentSessionId(" << userSession->getChildSessionId() << "), "
                    "but that parent UserSession has a ChildSessionId(" << parentUserSession->getChildSessionId() << "). This should not occur.");
            }

            parentUserSession->setChildSessionId(INVALID_USER_SESSION_ID);

            upsertUserSessionData(*parentUserSession->mUserSessionData);
        }
    }

    // Need to generate the event before removeLocalUserSession(), because PIN header gen code does another lookup.
    // Switch to a new fiber for this - logout events are triggered by ClientInboundRpcConnection::close(), which may be invoked from
    // the readLoop or writeLoop fiber; neither has a stack size large enough to handle a call to generateLogoutEvent at this frame. [GOS-31296]
    Fiber::CreateParams fiberParams;
    fiberParams.blocking = false;
    fiberParams.stackSize = Fiber::STACK_LARGE;
    fiberParams.namedContext = "UserSessionsMasterImpl::generateLogoutEvent";
    gFiberManager->scheduleCall<UserSessionsMasterImpl, const UserSessionMasterPtr>(this, &UserSessionsMasterImpl::generateLogoutEvent, userSession, disconnectType, socketError, fiberParams);

    UserSessionLogoutInfo userSessionLogoutInfo;
    userSessionLogoutInfo.setBlazeId(userSession->getBlazeId());
    userSessionLogoutInfo.setUserSessionType(userSession->getUserSessionType()); 
    userSessionLogoutInfo.setForcedLogoutReason(forcedLogoutType);

    gUserSessionManager->sendUserUnauthenticatedToUserSession(userSession.get(), &userSessionLogoutInfo, true);

    removeLocalUserSession(userSession);

    if (UserSession::getCurrentUserSessionId() == userSession->getUserSessionId())
    {
        UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
    }

    mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionLogout, *userSession);
    gUserSessionManager->onEraseUserSessionExistence(userSession->getUserSessionId());

    char8_t timeBuf[64];
    TimeValue interval = TimeValue::getTimeOfDay() - userSession->getCreationTime();
    BLAZE_INFO_LOG(Log::USER, "[UserManager]: User Session Ended: "
        "UserSessionId=" << userSession->getUserSessionId() << ", "
        "ClientType=" << ClientTypeToString(userSession->getClientType()) << ", "
        "BlazeId=" << userSession->getBlazeId() << ", "
        "PersonaId=" << userSession->getUserInfo().getId() << ", "
        "Persona=" << userSession->getUserInfo().getPersonaName() << ", "
        "ConnGroup=" << userSession->getConnectionGroupId() << ", "
        "Duration=" << interval.toIntervalString(timeBuf, sizeof(timeBuf)) << "(" << interval.getSec() << "s)" << ", "
        "Disconnect Reason=" << UserSessionDisconnectTypeToString(disconnectType) << ", "
        "Socket Err=" << socketError << ", "
        "Real Addr=" << (*userSession->getConnectionAddr() == '\0' ? "Unknown" : userSession->getConnectionAddr()));

    mUserSessionTable.eraseRecord(userSession->getUserSessionId());

    BLAZE_TRACE_LOG(Log::USER, "UserSessionsMasterImpl.destroyUserSession: Removed session " << userSession->getUserSessionId() << ".");

    if (userSession->getClientTypeFlags().getTrackConnectionMetrics())
    {
        gUserSessionManager->getMetricsObj().mDisconnections.increment(1, userSession->getClientType(), disconnectType, userSession->getClientState().getStatus(), userSession->getClientState().getMode());
    }

    gUserSessionManager->mUserSessionsGlobalMetrics.mSessionsRemoved.increment(1, disconnectType);

    mUserSessionCleanupJobQueue.queueFiberJob(this, &UserSessionsMasterImpl::performBlockingUserSessionCleanupOperations,
        userSession->getUserSessionId(), userSession->getBlazeId(), userSession->getAccountId(), userSession->getPlatformInfo().getClientPlatform(), userSession->getClientType(), "UserSessionsMasterImpl::performBlockingUserSessionCleanupOperations");

    return ERR_OK;
}

void UserSessionsMasterImpl::performBlockingUserSessionCleanupOperations(UserSessionId userSessionId, BlazeId blazeId, AccountId accountId, ClientPlatformType platform, ClientType clientType)
{
    bool cleanupSingleLoginKey = true;
    if (!UserSessionManager::isStatelessUser(blazeId))
    {
        // Don't update the last logout time of users on platforms that aren't hosted (e.g. users logged in to the 'common' platform)
        if (gController->isPlatformHosted(platform))
        {
            BlazeRpcError err = UserInfoDbCalls::updateLastLogoutDateTime(blazeId, platform);
            if (err != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl]: @ { " << Fiber::getCurrentContext() << " } Unable to update last logout datetime for BlazeId:" << blazeId << " with error:" << ErrorHelp::getErrorName(err));
            }
        }
    }
    else // if (UserSessionManager::isStatelessUser(blazeId))
    {
        // stateless users can only skip cleaning up the single login key if their client type does not enforce single login.
        // most stateless users won't be using a client type that enforces single login.
        const ClientTypeFlags& clientTypeFlags = gUserSessionManager->getClientTypeDescription(clientType);
        if (!clientTypeFlags.getEnforceSingleLogin())
        {
            cleanupSingleLoginKey = false;
        }
    }

    if(cleanupSingleLoginKey)
    {
        if (gUserSessionManager->getClientTypeDescription(clientType).getEnforceSingleLogin())
        {
            RedisCluster* redisCluster = gRedisManager->getRedisCluster(mSingleLoginIds.getCollectionId().getNamespace());
            if (redisCluster != nullptr)
            {
                StringBuilder singleLoginKey;
                makeSingleLoginKey(singleLoginKey, accountId, platform);

                RedisCommand redisCmd;
                redisCmd.beginKey();
                redisCmd.add(eastl::make_pair(mSingleLoginIds.getCollectionKey(INVALID_SLIVER_ID), singleLoginKey.get()));
                redisCmd.end();
                
                SingleLoginIdentity sessionIdentity;
                sessionIdentity.first = blazeId;
                sessionIdentity.second = userSessionId;
                redisCmd.add(sessionIdentity);

                RedisResponsePtr redisResp;
                RedisError redisError = redisCluster->execScript(msDeleteSingleLoginKeyScript, redisCmd, redisResp);
                if (redisError != REDIS_ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::USER, "UserSessionsMasterImpl.performBlockingUserSessionCleanupOperations: Executing the DeleteSingleLoginKeyScript redis script failed with err(" << redisError.error << ")");
                }
                else if (redisResp->isInteger() && redisResp->getInteger() != 1) //1 is the return value we use in the msDeleteSingleLoginKeyScript script
                {
                    BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.performBlockingUserSessionCleanupOperations: the DeleteSingleLoginKeyScript redis script failed to find key(" << singleLoginKey << ")" <<
                        "whose value is (" << sessionIdentity << "). Note: Redis key/value format encoding may be slightly different than value outputted here.");
                }
            }
        }
    }
}

void UserSessionsMasterImpl::updateUserInfoInLocalUserSessions(BlazeId blazeId)
{
    BlazeIdUserSessionId first = {blazeId, 0};
    BlazeIdUserSessionId last = {blazeId + 1, 0};
    UserSessionsByBlazeIdMap::iterator it = mUserSessionsByBlazeId.upper_bound(first);
    UserSessionsByBlazeIdMap::iterator end = mUserSessionsByBlazeId.upper_bound(last);

    // If there are no local UserSessions, then there's nothing to do.
    if (it == end)
        return;

    // Use DB master to lookup to avoid replication lag if DB tech does not have synchronous write replication. 
    UserInfoPtr userInfo;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo, true, false);
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::USER, "UserSessionsMasterImpl.updateUserInfoInLocalUserSessions: Call to lookupUserInfoByBlazeId(" << blazeId << ") failed with err(" << ErrorHelp::getErrorName(err) << ")");
        return;
    }

    // NOTE: We have to do another lookup to get the range, once again.  This is because the above lookupUserInfoByBlazeId()
    //       call can block - which leaves open the possibility of the iterators being invalidated.
    it = mUserSessionsByBlazeId.upper_bound(first);
    end = mUserSessionsByBlazeId.upper_bound(last);
    for (; it != end; ++it)
    {
        UserSessionMaster& userSession = *it->second;

        // Swap out the guys old UserInfo for the new one.  This will normally bring the
        // old UserInfoPtr ref-count down to zero.
        userSession.changeUserInfo(userInfo);

        // Check all values that are also stored in the UserSessionExistenceData, and only if
        // any of them have changed, should we do the upsertUserExistenceData() call at the end.
        bool userExistenceDataUpsertRequired = false;
        if (userSession.getAccountLocale() != userInfo->getAccountLocale())
        {
            userExistenceDataUpsertRequired = true;
            userSession.setAccountLocale(userInfo->getAccountLocale());
        }

        if (userSession.getAccountCountry() != userInfo->getAccountCountry())
        {
            userExistenceDataUpsertRequired = true;
            userSession.setPreviousAccountCountry(userSession.getAccountCountry());
            userSession.setAccountCountry(userInfo->getAccountCountry());
        }

        if (userSession.getAccountId() != userInfo->getPlatformInfo().getEaIds().getNucleusAccountId())
        {
            userExistenceDataUpsertRequired = true;
            userSession.setAccountId(userInfo->getPlatformInfo().getEaIds().getNucleusAccountId());
        }

        // Check if any part of the PlatformInfo has changed, I guess...
        if (!userInfo->getPlatformInfo().equalsValue(userSession.getPlatformInfo()))
        {
            userExistenceDataUpsertRequired = true;
            userInfo->getPlatformInfo().copyInto(userSession.getPlatformInfo());
        }


        if (userSession.mExistenceData->getSessionFlags().getIsChildAccount() != userInfo->getIsChildAccount())
        {
            userExistenceDataUpsertRequired = true;
            userSession.mExistenceData->getSessionFlags().setIsChildAccount(userInfo->getIsChildAccount());
        }

        if (userSession.mExistenceData->getSessionFlags().getStopProcess() != userInfo->getStopProcess())
        {
            userExistenceDataUpsertRequired = true;
            userSession.mExistenceData->getSessionFlags().setStopProcess(userInfo->getStopProcess());
        }

        if (userExistenceDataUpsertRequired)
            upsertUserExistenceData(*userSession.mExistenceData);

        bool userSessionExtendedDataChanged = false;

        if (userSession.getExtendedData().getCrossPlatformOptIn() != userSession.getUserInfo().getCrossPlatformOptIn())
        {
            userSessionExtendedDataChanged = true;
            userSession.getExtendedData().setCrossPlatformOptIn(userInfo->getCrossPlatformOptIn());
        }

        if (userSession.getExtendedData().getUserInfoAttribute() != userSession.getUserInfo().getUserInfoAttribute())
        {
            userSessionExtendedDataChanged = true;
            userSession.getExtendedData().setUserInfoAttribute(userSession.getUserInfo().getUserInfoAttribute());
        }

        if (userSessionExtendedDataChanged)
        {
            handleExtendedDataChange(userSession);
        }
    }
}

GetCensusDataError::Error UserSessionsMasterImpl::processGetCensusData(Blaze::UserManagerCensusData &response, const Message* message)
{
    TimeValue now = TimeValue::getTimeOfDay();

    if (now < mCensusCacheExpiry)
    {
        // Just return the cached response
        mCensusCache.copyInto(response);
        return GetCensusDataError::ERR_OK;
    }

    // Cache entry has expired so build a new one, reset the expiry time, and return the data
    Metrics::SumsByTagValue sums;
    mLocalUserSessionCounts.aggregate({ Metrics::Tag::pingsite }, sums);


    ConnectedCountsByPingSiteMap& map = response.getConnectedPlayerCountByPingSite();

    // Combine the values from each product name:
    for (auto& pingSite : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
    {
        const char8_t* pingSiteAlias = pingSite.first.c_str();
        uint32_t count = (uint32_t)sums[{ pingSiteAlias }];
        if (count != 0)
        {
            if (map.find(pingSiteAlias) == map.end())
                map[pingSiteAlias] = count;
            else
                map[pingSiteAlias] += count;
        }
    }

    mCensusCacheExpiry = now + (1 * 1000 * 1000);
    response.copyInto(mCensusCache);

    return GetCensusDataError::ERR_OK;
}

UpdateGeoDataMasterError::Error UserSessionsMasterImpl::processUpdateGeoDataMaster(const GeoLocationData& data, const Message* message)
{    
    BlazeIdUserSessionId first = {data.getBlazeId(), 0};
    BlazeIdUserSessionId last = {data.getBlazeId()+1, 0};
    for (UserSessionsByBlazeIdMap::iterator it = mUserSessionsByBlazeId.upper_bound(first), end = mUserSessionsByBlazeId.upper_bound(last); it != end; ++it)
    {
        bool extendedDataChanged = false;
        UserSessionMaster& session = *it->second;

        updateLocalUserSessionCountsAddOrRemoveSession(session, false);

        session.setLatitude(data.getLatitude());
        session.setLongitude(data.getLongitude());

        const char8_t* country = data.getCountry();
        if ((country != nullptr) && (country[0] != '\0'))
        {
            session.getExtendedData().setCountry(country);
            extendedDataChanged = true;
        }
        const char8_t* timeZone = data.getTimeZone();
        if ((timeZone != nullptr) && (timeZone[0] != '\0'))
        {
            session.getExtendedData().setTimeZone(timeZone);
            extendedDataChanged = true;
        }
        const char8_t* isp = data.getISP();
        if ((isp != nullptr) && (isp[0] != '\0'))
        {
            session.getExtendedData().setISP(isp);
            extendedDataChanged = true;
        }

        updateLocalUserSessionCountsAddOrRemoveSession(session, true);
        
        if (extendedDataChanged)
            handleExtendedDataChange(session);
        else
            upsertUserSessionData(*session.mUserSessionData);
     }

    return UpdateGeoDataMasterError::ERR_OK;
}

GetUserExtendedDataError::Error UserSessionsMasterImpl::processGetUserExtendedData(const Blaze::GetUserExtendedDataRequest &request, Blaze::GetUserExtendedDataResponse &response, const Message*)
{
    UserSessionMasterPtr session = getLocalSession(request.getUserSessionId());
    if (session != nullptr)
    {
        if (request.getSubscriberSessionId() != INVALID_USER_SESSION_ID)
        {
            session->getSubscribedSessions()[request.getSubscriberSessionId()]++;
            upsertUserSessionData(*session->mUserSessionData);
        }

        session->getExtendedData().copyInto(response.getExtendedData()); // NOTE: We could have used set() only if allowref were to use TdfPtr
        return GetUserExtendedDataError::ERR_OK;
    }
    return GetUserExtendedDataError::USER_ERR_SESSION_NOT_FOUND;
}

GetUserInfoDataError::Error UserSessionsMasterImpl::processGetUserInfoData(const Blaze::GetUserInfoDataRequest &request, Blaze::GetUserInfoDataResponse &response, const Message*)
{
    UserSessionMasterPtr session = getLocalSession(request.getUserSessionId());
    if (session != nullptr)
    {
        session->getUserInfo().copyInto(response.getUserInfo()); // NOTE: We could have used set() only if allowref were to use TdfPtr
        return GetUserInfoDataError::ERR_OK;
    }
    return GetUserInfoDataError::USER_ERR_SESSION_NOT_FOUND;
}

GetUserSessionDataError::Error UserSessionsMasterImpl::processGetUserSessionData(const Blaze::GetUserInfoDataRequest& request, Blaze::GetUserSessionDataResponse &response, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getUserSessionId());
    if (session != nullptr)
    {
        response.setUserSessionData(*session->getUserSessionData());
        ConnGroupUserSetMap::const_iterator it = mConnGroupUserSetMap.find(session->getConnectionGroupId());
        if (it != mConnGroupUserSetMap.end())
        {
            const ConnectionGroupData& connGroupData = it->second;
            const UserSessionIdVectorSet& userSessionIdSet = connGroupData.mUserSessionIdSet;

            for (auto userSessionId : userSessionIdSet)
                response.getUsersOnConnection().push_back(userSessionId);
        }

        return GetUserSessionDataError::ERR_OK;
    }
    return GetUserSessionDataError::USER_ERR_SESSION_NOT_FOUND;
}


UpdateExtendedDataMasterError::Error UserSessionsMasterImpl::processUpdateExtendedDataMaster(const Blaze::UpdateExtendedDataRequest& request, const Message* message)
{
    const UpdateExtendedDataRequest::UserOrSessionToUpdateMap& map = request.getUserOrSessionToUpdateMap();
    for (UpdateExtendedDataRequest::UserOrSessionToUpdateMap::const_iterator it = map.begin(), end = map.end(); it != end; ++it)
    {
        if (request.getIdsAreSessions())
        {
            //Just lookup the session and set it
            UserSessionMasterPtr session = getLocalSession(it->first);
            if (session != nullptr)
                doExtendedDataUpdate(*session, request.getComponentId(), *it->second, request.getRemove());
        }
        else
        {
            BlazeIdUserSessionId first = {it->first, 0};
            BlazeIdUserSessionId last = {it->first+1, 0};
            for (UserSessionsByBlazeIdMap::iterator uit = mUserSessionsByBlazeId.upper_bound(first), uend = mUserSessionsByBlazeId.upper_bound(last); uit != uend; ++uit)
            {
                doExtendedDataUpdate(*uit->second, request.getComponentId(), *it->second, request.getRemove());
            }
        }
    }
    return UpdateExtendedDataMasterError::ERR_OK;
}

void UserSessionsMasterImpl::doExtendedDataUpdate(UserSessionMaster& session, uint16_t componentId, const Blaze::UpdateExtendedDataRequest::ExtendedDataUpdate &update, bool removeKeys)
{    
    const UserExtendedDataMap &map = update.getDataMap();
    for (UserExtendedDataMap::const_iterator it = map.begin(), end = map.end(); it != end; ++it)
    {
        uint32_t dataKey = (componentId << 16) | it->first;
        if (!removeKeys)
            session.getExtendedData().getDataMap()[dataKey] = it->second;
        else
            session.getExtendedData().getDataMap().erase(dataKey);
    }

    handleExtendedDataChange(session);
}

UpdateBlazeObjectIdListError::Error UserSessionsMasterImpl::processUpdateBlazeObjectIdList(const Blaze::UpdateBlazeObjectIdListRequest& request, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getInfo().getSessionId());
    if (session != nullptr)
    {
        UserSession::updateBlazeObjectId(*session->mUserSessionData, request.getInfo());

        handleExtendedDataChange(*session);

        return UpdateBlazeObjectIdListError::ERR_OK;
    }
    return UpdateBlazeObjectIdListError::USER_ERR_SESSION_NOT_FOUND;
}

RefreshQosPingSiteMapMasterError::Error UserSessionsMasterImpl::processRefreshQosPingSiteMapMaster(const Blaze::RefreshQosPingSiteMapRequest& request, const Message* message)
{
    if (EA_UNLIKELY(gUserSessionManager == nullptr))
    {
        return RefreshQosPingSiteMapMasterError::ERR_SYSTEM;
    }

    if (gUserSessionManager->getQosConfig().validatePingSiteAliasMap(request.getPingSiteAliasMap(), request.getProfileVersion()))
    {
        BLAZE_TRACE_LOG(Log::USER, "[RefreshQosPingSiteMapCommand] PingSiteInfo is already up to date.");
        return RefreshQosPingSiteMapMasterError::ERR_OK;
    }

    if (gUserSessionManager->getQosConfig().attemptRefreshQosPingSiteMap() == false)
    {
        BLAZE_WARN_LOG(Log::USER, "[RefreshQosPingSiteMapCommand] Attempted to refresh QoS info on a 1.0 server. (Only supported on QoS 2.0 servers.) No op.");
        return RefreshQosPingSiteMapMasterError::ERR_SYSTEM;
    }

    return RefreshQosPingSiteMapMasterError::ERR_OK;
}

UpdateNetworkInfoMasterError::Error UserSessionsMasterImpl::processUpdateNetworkInfoMaster(const Blaze::UpdateNetworkInfoMasterRequest& request, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getSessionId());
    if (session == nullptr)
        return UpdateNetworkInfoMasterError::USER_ERR_NO_EXTENDED_DATA;

    UpdateNetworkInfoMasterError::Error err = UpdateNetworkInfoMasterError::ERR_OK;
    UserSessionExtendedData& data = session->getExtendedData();
    const NetworkInfo& info = request.getNetworkInfo();
    const Util::NetworkQosData& qosData = info.getQosData();

    PingSiteLatencyByAliasMap pingSiteLatencyMap;
    info.getPingSiteLatencyByAliasMap().copyInto(pingSiteLatencyMap);

    // This check is used to determine if the QoS ping sites have changed.  
    if (!request.getOpts().getNetworkAddressOnly() && !request.getOpts().getNatInfoOnly() && !gUserSessionManager->getQosConfig().validateAndCleanupPingSiteAliasMap(pingSiteLatencyMap, qosData.getQosProfileVersion()))
    {
        // If the PingSites are different, we still allow the client to go through, but we have the server check for updates:
        if (gUserSessionManager->getQosConfig().attemptRefreshQosPingSiteMap() == false)
        {
            // This indicates that the QoS ping site is using 1.0 settings, and doesn't need to be updated.
            gUserSessionManager->sendNotifyQosSettingsUpdatedToUserSession(session.get(), &(gUserSessionManager->getQosConfig().getQosSettings()));
            return err;
        }
    }

    if (info.getAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET)
    {
        // Copy over address
        info.getAddress().copyInto(data.getAddress());
    }
    else
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].updateNetworkInfoAndQosMetrics: "
            << "Client is sending up network info without an address set.  This may be valid in Arson but should not happen normally.  User address is unchanged.");
    }

    if (!request.getOpts().getNetworkAddressOnly() || !gUserSessionManager->getQosConfig().isQosVersion1())
    {
        if (request.getOpts().getNatInfoOnly())
        {
            data.getQosData().setNatType(info.getQosData().getNatType());
        }
        else
        {
            //Copy over NAT type and bandwidth data
            qosData.copyInto(data.getQosData());

            bool updateMetrics = request.getOpts().getUpdateMetrics() && !pingSiteLatencyMap.empty();

            QosMetricsByGeoIPData* metrics = nullptr;
            QosMetrics* metricsByCountryByISP = nullptr;
            QosMetrics* metricsByCountry = nullptr;

            if (updateMetrics)
            {
                const char8_t* country;
                const char8_t* isp;

                if (*data.getCountry() == '\0')
                    country = "NONE";
                else
                    country = data.getCountry();
                if (*data.getISP() == '\0')
                    isp = "NONE";
                else
                    isp = data.getISP();

                metrics = &(mLocalQosMetrics[session->getClientType()]);
                metricsByCountryByISP = &(metrics->mByCountryByISP[isp][country]);
                metricsByCountry = &(metrics->mByCountry[country]);
            }

            // put latency into latency map (and list, for backwards compatibility) and figure out what the best alias is
            int32_t bestLatency = INT32_MAX;

            pingSiteLatencyMap.copyInto(data.getLatencyMap());
            data.getLatencyList().reserve(pingSiteLatencyMap.size());   // DEPRECATED with QoS 2.0:


            // Count the number of failures that occurred in the attempt to connect to all ping sites
            uint16_t numFailures = 0;

            // Decrement the count for this ping site. We'll increment it
            // when we handle the UED change
            updateLocalUserSessionCountsAddOrRemoveSession(*session, false);


            // QoS 2.0 error case fix-ups: 
            if (!gUserSessionManager->getQosConfig().isQosVersion1())
            {
                // if the map was empty, add in entries for all known ping sites, with max ping:
                if (data.getLatencyMap().empty())
                {
                    for (const auto it : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
                        data.getLatencyMap()[it.first] = static_cast<int32_t>(MAX_QOS_LATENCY);
                }

                // Update the latencies used in the map:  (No negative numbers)
                int32_t numSitesMatchingBestLatency = 0;
                for (auto it : data.getLatencyMap())
                {
                    int32_t errCode = 0;
                    int32_t latency = it.second;
                    if (latency > static_cast<int32_t>(MAX_QOS_LATENCY) || latency < 0)
                    {
                        ++numFailures;
                        errCode = (latency < 0) ? latency : MAX_QOS_LATENCY_ERR_CODE;
                        it.second = static_cast<int32_t>(MAX_QOS_LATENCY);
                        latency = static_cast<int32_t>(MAX_QOS_LATENCY);
                    }

                    if (latency < bestLatency)
                    {
                        data.setBestPingSiteAlias(it.first);
                        bestLatency = latency;
                        numSitesMatchingBestLatency = 1;
                    }
                    else if (latency == bestLatency)
                    {
                        // randomly choose the best pingsite from among all sites with equal latency. 
                        ++numSitesMatchingBestLatency;
                        if (Random::getRandomNumber(numSitesMatchingBestLatency) == 0) // 1/2 chance, then 1/3, then 1/4, etc.
                            data.setBestPingSiteAlias(it.first);
                    }
                    

                    if (updateMetrics)
                    {
                        // Increment count for this ping site and this error
                        LatencyOrErrorCountByPingSiteAlias::iterator psItr = metricsByCountry->mErrorCountByPingSiteAlias.insert(eastl::make_pair(it.first.c_str(), CountByErrorType())).first;
                        ++(psItr->second.insert(eastl::make_pair(errCode, 0)).first->second);
                        psItr = metricsByCountryByISP->mErrorCountByPingSiteAlias.insert(eastl::make_pair(it.first.c_str(), CountByErrorType())).first;
                        ++(psItr->second.insert(eastl::make_pair(errCode, 0)).first->second);
                    }
                }
            }
            else
            {
                // Qos 1.0 latency list code  (DEPRECATED with QoS 2.0):
                for (PingSiteLatencyByAliasMap::const_iterator it = pingSiteLatencyMap.begin(), itend = pingSiteLatencyMap.end(); it != itend; ++it)
                {
                    int32_t errCode = 0;
                    int32_t latency = it->second;
                    if (latency > static_cast<int32_t>(MAX_QOS_LATENCY))
                    {
                        ++numFailures;
                        errCode = MAX_QOS_LATENCY_ERR_CODE;

                        //need to cap this to MAX_QOS_LATENCY
                        BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].updateNetworkInfoAndQosMetrics: "
                            << "ping value provided for pingsite '" << it->first << "' was '" << latency << "' greater than MAX_QOS_LATENCY value of '"
                            << MAX_QOS_LATENCY << "', capping value.");
                        latency = static_cast<int32_t>(MAX_QOS_LATENCY);
                    }
                    else if (latency < 0)
                    {
                        ++numFailures;
                        errCode = latency;

                        // negative values are hresult error codes, not valid ping times
                        BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].updateNetworkInfoAndQosMetrics: "
                            << "pingsite '" << it->first << "' produced the error code '" << latency << "'"
                            << " setting value to MAX_QOS_LATENCY.");
                        latency = static_cast<int32_t>(MAX_QOS_LATENCY);
                    }

                    data.getLatencyList().push_back(it->second);
                    if (latency < bestLatency)
                    {
                        data.setBestPingSiteAlias(it->first);
                        bestLatency = latency;
                    }

                    if (updateMetrics)
                    {
                        // Increment count for this ping site and this error
                        LatencyOrErrorCountByPingSiteAlias::iterator psItr = metricsByCountry->mErrorCountByPingSiteAlias.insert(eastl::make_pair(it->first.c_str(), CountByErrorType())).first;
                        ++(psItr->second.insert(eastl::make_pair(errCode, 0)).first->second);
                        psItr = metricsByCountryByISP->mErrorCountByPingSiteAlias.insert(eastl::make_pair(it->first.c_str(), CountByErrorType())).first;
                        ++(psItr->second.insert(eastl::make_pair(errCode, 0)).first->second);
                    }
                }
            }

            updateLocalUserSessionCountsAddOrRemoveSession(*session, true);

            // If we currently have an unresolved latitude/longitude, update these values using the best ping site's location
            if (session->getLatitude() == GEO_IP_DEFAULT_LATITUDE && session->getLongitude() == GEO_IP_DEFAULT_LONGITUDE)
            {
                if (bestLatency <= getConfig().getPingSiteLatencyThresholdForFallbackGeoipLookup())
                {
                    //we do this as a fallback if the geoip against the clients ip failed, we use the geoip of its best pingsite
                    PingSiteInfoByAliasMap::const_iterator iter = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().find(data.getBestPingSiteAlias());
                    if (iter != gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().end())
                    {
                        InetAddress pingSiteAddr(iter->second->getAddress(), iter->second->getPort());
                        NameResolver::LookupJobId resolveId;
                        if (gNameResolver->resolve(pingSiteAddr, resolveId) != ERR_OK)
                        {
                            BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].updateNetworkInfoAndQosMetrics: Failed to resolve IP address for "
                                << "pingsite '" << iter->first << "'.");
                        }
                        else
                        {
                            GeoLocationData geoData;
                            if (!gUserSessionManager->getGeoIpData(pingSiteAddr, geoData))
                            {
                                BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].updateNetworkInfoAndQosMetrics: Failed to lookup geo location data for "
                                    << "pingsite '" << iter->first << "'.");
                                err = UpdateNetworkInfoMasterError::USER_ERR_GEOIP_LOOKUP_FAILED;
                            }
                            else
                            {
                                session->setLatitude(geoData.getLatitude());
                                session->setLongitude(geoData.getLongitude());
                            }
                        }
                    }
                }
                else
                {
                    BLAZE_TRACE_LOG(Log::USER, "[UserSessionsMasterImpl].updateNetworkInfoAndQosMetrics: Latency for best pingsite '" << data.getBestPingSiteAlias()
                        << "' is greater than threshold for GeoIP lookup: " << getConfig().getPingSiteLatencyThresholdForFallbackGeoipLookup() << "ms.");
                }
            }

            if (updateMetrics)
            {
                // Increment NAT type and UPnP status counts
                ++metricsByCountry->mNatTypeCount[qosData.getNatType()];
                ++metricsByCountry->mUpnpStatusCount[session->getClientMetrics().getStatus()];
                ++metricsByCountryByISP->mNatTypeCount[qosData.getNatType()];
                ++metricsByCountryByISP->mUpnpStatusCount[session->getClientMetrics().getStatus()];

                // Increment count for this number of failures
                ++(metricsByCountry->mFailuresPerQosTestCount.insert(eastl::make_pair(numFailures, 0)).first->second);
                ++(metricsByCountryByISP->mFailuresPerQosTestCount.insert(eastl::make_pair(numFailures, 0)).first->second);

                // [DEPRECATED]  The following (until latency) are QoS 1.0 metrics that are kept for compatibility with older clients.
                // Increment NAT and bandwidth error counts for this best ping site
                int32_t natError = eastl::min((int32_t)qosData.getNatErrorCode(), 0); // non-negative error codes indicate success
                int32_t bandwidthError = eastl::min((int32_t)qosData.getBandwidthErrorCode(), 0);

                LatencyOrErrorCountByPingSiteAlias::iterator psItr = metricsByCountry->mNatErrorCountByBestPingSiteAlias.insert(eastl::make_pair(data.getBestPingSiteAlias(), CountByErrorType())).first;
                ++(psItr->second.insert(eastl::make_pair(natError, 0)).first->second);
                psItr = metricsByCountryByISP->mNatErrorCountByBestPingSiteAlias.insert(eastl::make_pair(data.getBestPingSiteAlias(), CountByErrorType())).first;
                ++(psItr->second.insert(eastl::make_pair(natError, 0)).first->second);

                psItr = metricsByCountry->mBandwidthErrorCountByBestPingSiteAlias.insert(eastl::make_pair(data.getBestPingSiteAlias(), CountByErrorType())).first;
                ++(psItr->second.insert(eastl::make_pair(bandwidthError, 0)).first->second);
                psItr = metricsByCountryByISP->mBandwidthErrorCountByBestPingSiteAlias.insert(eastl::make_pair(data.getBestPingSiteAlias(), CountByErrorType())).first;
                ++(psItr->second.insert(eastl::make_pair(bandwidthError, 0)).first->second);

                // Increment count for this best ping site and this best latency bucket
                int32_t latencyBucket = UserSessionManager::getLatencyIndex(bestLatency);

                psItr = metricsByCountry->mLatencyByBestPingSiteAlias.insert(eastl::make_pair(data.getBestPingSiteAlias(), CountByErrorType())).first;
                ++((psItr->second).insert(eastl::make_pair(latencyBucket, 0)).first->second);
                psItr = metricsByCountryByISP->mLatencyByBestPingSiteAlias.insert(eastl::make_pair(data.getBestPingSiteAlias(), CountByErrorType())).first;
                ++((psItr->second).insert(eastl::make_pair(latencyBucket, 0)).first->second);
            }
        }
    }

    handleExtendedDataChange(*session);

    return err;
}

UpdateUserSessionServerAttributeMasterError::Error UserSessionsMasterImpl::processUpdateUserSessionServerAttributeMaster(const Blaze::UpdateUserSessionAttributeMasterRequest &req, const Message* message)
{
    return UpdateUserSessionServerAttributeMasterError::commandErrorFromBlazeError(
        doUserSessionAttributeUpdate(req.getReq(), req.getBlazeId()));
}

BlazeRpcError UserSessionsMasterImpl::doUserSessionAttributeUpdate(const Blaze::UpdateUserSessionAttributeRequest& request, Blaze::BlazeId blazeId)
{
    BlazeIdUserSessionId first = {blazeId, 0};
    BlazeIdUserSessionId last = {blazeId + 1, 0};
    UserSessionsByBlazeIdMap::iterator it = mUserSessionsByBlazeId.upper_bound(first);
    UserSessionsByBlazeIdMap::iterator end = mUserSessionsByBlazeId.upper_bound(last);
    if (it == end)
    {
        return Blaze::USER_ERR_SESSION_NOT_FOUND;
    }

    uint32_t notFoundCount = 0;
    for (; it != end; ++it)
    {
        UserSessionMaster* session = it->second;

        if (!gUserSessionManager->getClientTypeDescription(session->getClientType()).getUpdateUserInfoData())
        {
            continue;
        }

        bool newValue = true;
        UserSessionAttributeMap &map = session->getServerAttributes();
        if (request.getRemove())
        {
            if (map.erase(request.getKey()) == 0)
            {
                ++notFoundCount;
                newValue = false;
            }
        }
        else
        {
            eastl::pair<UserSessionAttributeMap::iterator, bool> result = map.insert(eastl::make_pair(request.getKey(), request.getValue()));
            if (!result.second)
            {
                if (result.first->second == request.getValue())
                    newValue = false;
                else
                    result.first->second = request.getValue();
            }
        }

        if (newValue)
        {
            upsertUserSessionData(*session->mUserSessionData);
        }
    }

    if (notFoundCount > 0)
        return Blaze::USER_ERR_KEY_NOT_FOUND;

    return Blaze::ERR_OK;
}

UpdateUserSessionClientDataMasterError::Error UserSessionsMasterImpl::processUpdateUserSessionClientDataMaster(const Blaze::UpdateUserSessionClientDataMasterRequest& request, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getSessionId());
    if (session == nullptr)
    {
        return UpdateUserSessionClientDataMasterError::USER_ERR_SESSION_NOT_FOUND;
    }
    
    const UpdateUserSessionClientDataRequest& updateReq = request.getReq();
    const EA::TDF::Tdf* clientData = updateReq.getClientData();
    if (clientData != nullptr)
        session->getExtendedData().setClientData(*clientData->clone(getAllocator()));

    handleExtendedDataChange(*session);

    return UpdateUserSessionClientDataMasterError::ERR_OK;
}

UpdateHardwareFlagsMasterError::Error UserSessionsMasterImpl::processUpdateHardwareFlagsMaster(const Blaze::UpdateHardwareFlagsMasterRequest& request, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getSessionId());
    if (session == nullptr)
        return UpdateHardwareFlagsMasterError::USER_ERR_NO_EXTENDED_DATA;

    const UpdateHardwareFlagsRequest& updateReq = request.getReq();
    if (session->getExtendedData().getHardwareFlags().getBits() != updateReq.getHardwareFlags().getBits())
    {
        session->getExtendedData().getHardwareFlags().setBits(updateReq.getHardwareFlags().getBits());

        handleExtendedDataChange(*session);
    }

    return UpdateHardwareFlagsMasterError::ERR_OK;
}

BlazeRpcError UserSessionsMasterImpl::updateSlaveMapping(const UpdateSlaveMappingRequest& request)
{
    const uint8_t index = request.getAuxTypeIndex();
    if (index >= gController->getSlaveTypeCount())
    {
        BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].processUpdateSlaveMappingMaster: "
            "auxslave(" << request.getAuxInstanceId() << ") instance index " << index << " exceeds max slave type count " << gController->getSlaveTypeCount());
        return ERR_SYSTEM;
    }
    
    if (mAuxInfoMap.insert(eastl::make_pair(request.getAuxInstanceId(), AuxSlaveInfo(0, index, request.getImmediatePartialLocalUserReplication()))).second)
    {
        BLAZE_INFO_LOG(Log::USER, "[UserSessionsMasterImpl].addAuxSlave: auxslave(" << request.getAuxInstanceId() << ")");
    }
    else
    {
        CleanupTimerByInstanceIdMap::const_iterator it = mCleanupTimerByInstanceIdMap.find(request.getAuxInstanceId());
        if (it != mCleanupTimerByInstanceIdMap.end())
        {
            //cancel the scheduled removeAuxSlave() call since auxSlave is already back up so no need to perform user mapping rebalancing
            gSelector->cancelTimer(it->second);
            mCleanupTimerByInstanceIdMap.erase(request.getAuxInstanceId());

            return ERR_OK;
        }
    }

    AuxSlaveIdByCoreSlaveIdMap& idMap = mAuxIdByCoreIdMap[index];
    AuxSlaveIdByCoreSlaveIdMapItPair result = idMap.equal_range(request.getCoreInstanceId());
    if (request.getRequestType() == UpdateSlaveMappingRequest::MAPPING_REMOVE)
    {
        // NOTE: Only the multiple mappings of core -> aux need to be removed here.
        idMap.erase(result.first, result.second);
        return ERR_OK;
    }
    for (; result.first != result.second; ++result.first)
    {
        // if mapping already exists, ignore it
        if (result.first->second == request.getAuxInstanceId())
        {
            // this should never happen, but has no negative consequences
            BLAZE_WARN_LOG(Log::USER, "[UserSessionsMasterImpl].processUpdateSlaveMappingMaster: core(" << request.getCoreInstanceId() << ") -> auxslave(" << request.getAuxInstanceId() << ") "
                "mapping @ instance index " << index << " already exists.");
            return ERR_SYSTEM;
        }
    }
    // create core -> aux mapping
    idMap.insert(eastl::make_pair(request.getCoreInstanceId(), request.getAuxInstanceId()));
    // since we have a new mapping, kick off assignment of usersessions to auxslaves
    assignSessionsToAuxSlaves(index, request.getCoreInstanceId());
    return ERR_OK;
}

InsertUniqueDeviceIdMasterError::Error UserSessionsMasterImpl::processInsertUniqueDeviceIdMaster(const Blaze::InsertUniqueDeviceIdMasterRequest& request, const Message* message)
{
    // Add the device Id into local map
    const char8_t* id = request.getUniqueDeviceId();
    if ((id != nullptr) && (id[0] != '\0'))
    {
        UserSessionMasterPtr session = getLocalSession(request.getUserSessionId());
        if (session == nullptr)
        {
            return InsertUniqueDeviceIdMasterError::USER_ERR_SESSION_NOT_FOUND;
        }

        if (session->getUniqueDeviceId() != request.getUniqueDeviceId())
        {
            session->setUniqueDeviceId(request.getUniqueDeviceId());
            upsertUserExistenceData(*session->mExistenceData);
        }
    }

    return InsertUniqueDeviceIdMasterError::ERR_OK;
}

UpdateDirtySockUserIndexMasterError::Error UserSessionsMasterImpl::processUpdateDirtySockUserIndexMaster(const UpdateDirtySockUserIndexMasterRequest& request, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getUserSessionId());
    if (session == nullptr)
    {
        return UpdateDirtySockUserIndexMasterError::USER_ERR_SESSION_NOT_FOUND;
    }

    if (session->getDirtySockUserIndex() != request.getDirtySockUserIndex())
    {
        session->setDirtySockUserIndex(request.getDirtySockUserIndex());
        upsertUserSessionData(*session->mUserSessionData);
    }
    return UpdateDirtySockUserIndexMasterError::ERR_OK;
}

UpdateLocalUserGroupError::Error UserSessionsMasterImpl::updateLocalUserGroup(const UpdateLocalUserGroupRequest& request, UpdateLocalUserGroupResponse& response, const Message* message)
{
    UserSessionMasterPtr localUserSession = getLocalSession(UserSession::getCurrentUserSessionId());
    if (localUserSession == nullptr)
    {
        return UpdateLocalUserGroupError::ERR_SYSTEM;
    }

    UserSessionIdList sessionIdList;
    BlazeRpcError err = gUserSetManager->getSessionIds(localUserSession->getConnectionGroupObjectId(), sessionIdList);
    if (err != Blaze::ERR_OK)
    {
        BLAZE_ERR_LOG(Log::USER, "[UpdateLocalUserGroupCommand].execute: Failed to retrieve connection group (" << localUserSession->getConnectionGroupObjectId().toString().c_str() << ") "
            "user ids for calling user (blaze id: " << localUserSession->getBlazeId() << "), error " << ErrorHelp::getErrorName(err));
        return UpdateLocalUserGroupError::commandErrorFromBlazeError(err);
    }

    typedef eastl::set<uint32_t> ConnUserIndexUpdateSet;
    ConnUserIndexUpdateSet connUserIndexUpdateSet;

    ConnectionUserIndexList::const_iterator itr, end;
    for (itr = request.getConnUserIndexList().begin(), end = request.getConnUserIndexList().end(); itr != end; ++itr)
    {
        connUserIndexUpdateSet.insert(*itr);
    }

    response.getConnUserIndexList().reserve(connUserIndexUpdateSet.size());

    for (UserSessionIdList::const_iterator sessionItr = sessionIdList.begin(), sessionEnd = sessionIdList.end(); sessionItr != sessionEnd; ++sessionItr)
    {
        UserSessionMasterPtr session = getLocalSession(*sessionItr);
        if (session != nullptr)
        {
            if (connUserIndexUpdateSet.find(session->getConnectionUserIndex()) != connUserIndexUpdateSet.end())
            {
                session->mExistenceData->getSessionFlags().setInLocalUserGroup();
                uint32_t userIndex = session->getConnectionUserIndex();
                response.getConnUserIndexList().push_back(userIndex);
            }
            else
            {
                session->mExistenceData->getSessionFlags().clearInLocalUserGroup();
            }

            upsertUserSessionData(*session->mUserSessionData);
        }
    }

    if (!request.getConnUserIndexList().empty()) 
        response.setUserGroupObjectId(localUserSession->getLocalUserGroupObjectId());
    else
        response.setUserGroupObjectId(EA::TDF::OBJECT_ID_INVALID);

    return UpdateLocalUserGroupError::ERR_OK;
}

GetUserIpAddressError::Error UserSessionsMasterImpl::processGetUserIpAddress(const Blaze::GetUserIpAddressRequest& request, Blaze::GetUserIpAddressResponse& response, const Message* message)
{
    UserSessionMasterPtr session = getLocalSession(request.getUserSessionId());
    if (session != nullptr)
    {
        const PeerInfo* peerInfo = session->getPeerInfo();
        if (peerInfo != nullptr)
        {
            const InetAddress& addr = peerInfo->getRealPeerAddress();
            response.getIpAddress().setIp(addr.getIp(InetAddress::HOST));
            response.getIpAddress().setPort(addr.getPort(InetAddress::HOST));
        }
        return GetUserIpAddressError::ERR_OK;
    }
    return GetUserIpAddressError::USER_ERR_SESSION_NOT_FOUND;
}

bool UserSessionsMasterImpl::isImmediatePartialLocalUserReplication(InstanceId instanceId)
{
    AuxSlaveInfoMap::const_iterator it = mAuxInfoMap.find(instanceId);
    return (it != mAuxInfoMap.end() && it->second.immediatePartialLocalUserReplication);
}

AddUserSessionSubscriberError::Error UserSessionsMasterImpl::processAddUserSessionSubscriber(const Blaze::UserSessionSubscriberRequest& request, const Message* message)
{
    UserSessionMasterPtr userSession = getLocalSession(request.getNotifierSessionId());
    if (userSession == nullptr)
        return AddUserSessionSubscriberError::USER_ERR_SESSION_NOT_FOUND;

    int32_t& count = userSession->getSubscribedSessions()[request.getSubscriberSessionId()];
    if (count++ == 0)
    {
        //UserStatus userStatus;
        //userStatus.setBlazeId(userSession->getBlazeId());
        //userStatus.getStatusFlags().setSubscribed();
        //userStatus.getStatusFlags().setOnline();
        //gUserSessionManager->sendUserUpdatedToUserSessionById(request.getSubscriberSessionId(), &userStatus, request.getSendImmediately());

        //NetworkAddress* tempAddr = &userSession->getExtendedData().getAddress();
        //NetworkAddress emptyAddr;
        //userSession->getExtendedData().setAddress(emptyAddr);

        //UserSessionExtendedDataUpdate update;
        //update.setBlazeId(userSession->getBlazeId());
        //update.setSubscribed(true);
        //update.setExtendedData(userSession->getExtendedData());

        //gUserSessionManager->sendUserSessionExtendedDataUpdateToUserSessionById(request.getSubscriberSessionId(), &update, request.getSendImmediately());

        //userSession->getExtendedData().setAddress(*tempAddr);
    }

    upsertUserSessionData(*userSession->mUserSessionData);

    return AddUserSessionSubscriberError::ERR_OK;
}

RemoveUserSessionSubscriberError::Error UserSessionsMasterImpl::processRemoveUserSessionSubscriber(const Blaze::UserSessionSubscriberRequest &request, const Message* message)
{
    UserSessionMasterPtr userSession = getLocalSession(request.getNotifierSessionId());
    if (userSession == nullptr)
        return RemoveUserSessionSubscriberError::USER_ERR_SESSION_NOT_FOUND;

    UserSessionData::Int32ByUserSessionId::iterator it = userSession->getSubscribedSessions().find(request.getSubscriberSessionId());
    if (it != userSession->getSubscribedSessions().end())
    {
        EA_ASSERT(it->second > 0);

        if (--it->second == 0)
        {
            userSession->getSubscribedSessions().erase(it);

            //UserStatus userStatus;
            //userStatus.setBlazeId(userSession->getBlazeId());
            //userStatus.getStatusFlags().clearSubscribed();
            //userStatus.getStatusFlags().clearOnline();
            //gUserSessionManager->sendUserUpdatedToUserSessionById(request.getSubscriberSessionId(), &userStatus, request.getSendImmediately());
        }
    }

    upsertUserSessionData(*userSession->mUserSessionData);

    return RemoveUserSessionSubscriberError::ERR_OK;
}

FetchUserSessionsError::Error UserSessionsMasterImpl::processFetchUserSessions(const Blaze::FetchUserSessionsRequest& request, Blaze::FetchUserSessionsResponse& response, const Message* message)
{
    if (request.getUserSessionIds().empty())
        return FetchUserSessionsError::ERR_OK;

    // TODO_MC: Right now, this implementation can only be used to get one UserSession at a time.
    //          This is purely to satisfy the needs of UserSessionManager::fetchUserSessions() who implementation is to send one request at a time.
    //          Furthermore, we use a UserSessionDataList in order to have the decoder allocate the UserSessionData tdf on the heap, so we can take
    //          advantage of the TdfPtr, so we don't have to copy any data from the response...  Just grab a ref to the UserSessionDataPtr.
    //          Revisit this if we want to optimize.

    UserSessionMasterPtr userSession = getLocalSession(request.getUserSessionIds().front());
    if (userSession == nullptr)
        return FetchUserSessionsError::USER_ERR_SESSION_NOT_FOUND;

    response.getData().push_back(userSession->mUserSessionData);

    return FetchUserSessionsError::ERR_OK;
}

ValidateUserExistenceError::Error UserSessionsMasterImpl::processValidateUserExistence(const Blaze::ValidateUserExistenceRequest& request, const Message* message)
{
    UserSessionMasterPtr userSession = getLocalSession(request.getUserSessionId());
    return (userSession != nullptr) ? ValidateUserExistenceError::ERR_OK : ValidateUserExistenceError::USER_ERR_SESSION_NOT_FOUND;
}
 
ForceSessionLogoutMasterError::Error UserSessionsMasterImpl::processForceSessionLogoutMaster(const Blaze::ForceSessionLogoutRequest& request, const Message* message)
{
    BlazeRpcError err = destroyUserSession(request.getSessionId(), request.getReason(), 0, request.getForcedLogoutReason()); 
    return ForceSessionLogoutMasterError::commandErrorFromBlazeError(err);
}

void UserSessionsMasterImpl::handleExtendedDataChange(UserSessionMaster& session)
{
    ReputationService::ReputationServiceUtilPtr reputationServiceUtil = gUserSessionManager->getReputationServiceUtil();
    reputationServiceUtil->onRefreshExtendedData(session.getUserInfo(), session.getExtendedData());

    upsertUserSessionData(*session.mUserSessionData);

    session.sendCoalescedExtendedDataUpdate();
}

BlazeRpcError UserSessionsMasterImpl::getConnectionGroupUserIds(const ConnectionGroupObjectId& connGroupId, UserIdentificationList& results)
{
    InstanceId sliverInstanceId = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(connGroupId.id));

    if (sliverInstanceId == gController->getInstanceId())
    {
        SessionByConnGroupIdMap::const_iterator it = mLocalSessionByConnGroupIdMap.find(connGroupId.id);
        if (it != mLocalSessionByConnGroupIdMap.end())
        {
            for (ConnectionGroupList::const_iterator i = it->second.begin(), e = it->second.end(); i != e; ++i)
            {
                const UserSessionMaster& session = static_cast<const UserSessionMaster&>(*i);
                if (connGroupId.type != ENTITY_TYPE_LOCAL_USER_GROUP || session.getSessionFlags().getInLocalUserGroup())
                {
                    session.filloutUserIdentification(*results.pull_back());
                }
            }
        }
        return ERR_OK;
    }
    BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].getConnectionGroupUserIds: This method only looks up local connection group ids. ConnGroupId(" 
        << connGroupId.id << ") is owned by instanceId(" << sliverInstanceId << ")");
    return ERR_SYSTEM;
}

BlazeRpcError UserSessionsMasterImpl::getConnectionGroupUserBlazeIds(const ConnectionGroupObjectId& connGroupId, BlazeIdList& result)
{
    InstanceId sliverInstanceId = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(connGroupId.id));

    if (sliverInstanceId == gController->getInstanceId())
    {
        SessionByConnGroupIdMap::const_iterator it = mLocalSessionByConnGroupIdMap.find(connGroupId.id);
        if (it != mLocalSessionByConnGroupIdMap.end())
        {
            for (ConnectionGroupList::const_iterator i = it->second.begin(), e = it->second.end(); i != e; ++i)
            {
                const UserSessionMaster& session = static_cast<const UserSessionMaster&>(*i);
                if (connGroupId.type != ENTITY_TYPE_LOCAL_USER_GROUP || session.getSessionFlags().getInLocalUserGroup())
                {
                    result.push_back(session.getBlazeId());
                }
            }
        }
        return ERR_OK;
    }
    BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].getConnectionGroupUserIds: This method only looks up local connection group ids. ConnGroupId(" 
        << connGroupId.id << ") is owned by instanceId(" << sliverInstanceId << ")");
    return ERR_SYSTEM;
}

BlazeRpcError UserSessionsMasterImpl::getConnectionGroupSessionIds(const ConnectionGroupObjectId& connGroupId, UserSessionIdList& result)
{
    if (connGroupId.id != INVALID_CONNECTION_GROUP_ID)
    {
        InstanceId sliverInstanceId = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(connGroupId.id));

        if (sliverInstanceId == gController->getInstanceId())
        {
            SessionByConnGroupIdMap::const_iterator it = mLocalSessionByConnGroupIdMap.find(connGroupId.id);
            if (it != mLocalSessionByConnGroupIdMap.end())
            {
                for (ConnectionGroupList::const_iterator i = it->second.begin(), e = it->second.end(); i != e; ++i)
                {
                    const UserSessionMaster& session = static_cast<const UserSessionMaster&>(*i);
                    if (connGroupId.type != ENTITY_TYPE_LOCAL_USER_GROUP || session.getSessionFlags().getInLocalUserGroup())
                    {
                        result.push_back(session.getSessionId());
                    }
                }
            }
            return ERR_OK;
        }

        BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].getConnectionGroupSessionIds: This method only looks up local connection group ids. ConnGroupId(" 
            << connGroupId.id << ") is owned by instanceId(" << sliverInstanceId << ")");
    }

    return ERR_SYSTEM;
}

BlazeRpcError UserSessionsMasterImpl::updateClientState(UserSessionId userSessionId, const ClientState& request)
{
    UserSessionMasterPtr userSession = getLocalSession(userSessionId);
    if (userSession == nullptr)
    {
        return USER_ERR_SESSION_NOT_FOUND;
    }

    ClientState::Mode oldMode = userSession->getClientState().getMode();
    ClientState::Status oldStatus = userSession->getClientState().getStatus();
    ClientState::Mode newMode = request.getMode();
    ClientState::Status newStatus = request.getStatus();

    if ((oldMode != newMode) || (oldStatus != newStatus))
    {
        updateLocalUserSessionCountsAddOrRemoveSession(*userSession, false);

        userSession->getClientState().setMode(newMode);
        userSession->getClientState().setStatus(newStatus);

        updateLocalUserSessionCountsAddOrRemoveSession(*userSession, true);

        upsertUserSessionData(*userSession->mUserSessionData);
    }

    return ERR_OK;
}

bool UserSessionsMasterImpl::resolveAccessGroup( ClientType clientType, const PeerInfo& peerInfo, const char8_t* accessScope, AccessGroupName& accessGroupName)
{
    //lookup in database
    if (gUserSessionManager->getGroupManager() == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl].resolveAccessGroup: mGroupManager is nullptr");
        return false;
    }

    Authorization::GroupManager& groupManager = *gUserSessionManager->getGroupManager();
    Authorization::Group* authGroup = nullptr;
    if (*accessScope != '\0')
    {
        char8_t* accessScopeCopy = blaze_strdup(accessScope);
        char8_t* saveptr = nullptr;
        char8_t* scope = blaze_strtok(accessScopeCopy, " ", &saveptr);
        bool found = false;
        while (scope != nullptr && !found)
        {
            AuthorizationConfig::ScopeOverrideMap::const_iterator iter = getConfig().getAuthorization().getScopeOverrides().begin();
            AuthorizationConfig::ScopeOverrideMap::const_iterator end = getConfig().getAuthorization().getScopeOverrides().end();
            for (; iter != end; ++iter)
            {
                if (blaze_strcmp(scope, iter->first) == 0)
                {
                    accessGroupName = iter->second;
                    authGroup = groupManager.getGroup(accessGroupName.c_str());
                    found = true;
                    break;
                }
            }

            if (!found)
                scope = blaze_strtok(nullptr, " ", &saveptr);
        }
        BLAZE_FREE(accessScopeCopy);
    }

    if (authGroup == nullptr)
    {
        BLAZE_TRACE_LOG(Log::USER,"[UserSessionsMasterImpl].resolveAccessGroup: No access control entry found for user. Fall back to the default for the client type(" << ClientTypeToString(clientType) << ")");
    }

    const InetAddress& ipAddr = peerInfo.getPeerAddress();
    bool trustedClient = peerInfo.getTrustedClient();

    if (authGroup)
    {
        if ((authGroup->isAuthorized(ipAddr)) && (trustedClient || !authGroup->getRequireTrustedClient()))
            return true;

        BLAZE_WARN_LOG(Log::USER,"[UserSessionsMasterImpl].resolveAccessGroup: User ip(" << ipAddr << ") and trusted client status (" << trustedClient
            << ") fails authorization check for Group(" << (authGroup ? authGroup->getName() : "") << "). Fall back to default for client type(" 
            << ClientTypeToString(clientType) << ")" );

        if (trustedClient && authGroup->getRequireTrustedClient())
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSessionsMasterImpl].resolveAccessGroup: Trusted Clients (mTLS) should not require IpWhiteList checks. " <<
                "Make sure that Group(" << (authGroup ? authGroup->getName() : "") << ") is not trying to use the IpWhiteList if a Scoped AccessGroup is used.  ");
        }
    }

    //if authGroup not passing auth checks or not found, we should treat it as default group for the client type
    const AccessGroupName* pAccessGroupName = groupManager.getDefaultGroupName(clientType);
    if (pAccessGroupName)
    {
        BLAZE_TRACE_LOG(Log::USER,"[UserSessionsMasterImpl].resolveAccessGroup: Fall back to default(" << pAccessGroupName->c_str() << ") for the client type(" << clientType 
            << ":" << ClientTypeToString(clientType) << ")");

        accessGroupName.set(pAccessGroupName->c_str());
        authGroup = groupManager.getGroup(accessGroupName.c_str());
    }
    else
    {
        //if we are here, deployment was not done right.
        BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl].resolveAccessGroup: Fall back failed as none defined for clientType("
            << clientType << ":" << ClientTypeToString(clientType) << "). ***Please check your config***.");
        accessGroupName.set(nullptr);
        return false;
    }

    if (authGroup == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER,"[UserSessionsMasterImpl.resolveAccessGroup: Could not fetch default group "
            << accessGroupName.c_str() << " for client type (" << clientType << ":" << ClientTypeToString(clientType)
            << ") from group manager. ***Please check your config***.");
        accessGroupName.set(nullptr);
        return false;
    }

    // check authorization for fallback group
    if ((authGroup->isAuthorized(ipAddr)) && (trustedClient || !authGroup->getRequireTrustedClient()))
        return true;

    //someone unauthorized tried to log in
    BLAZE_WARN_LOG(Log::USER,"[UserSessionsMasterImpl].resolveAccessGroup: Fall back to group " << accessGroupName.c_str() << " failed. All permission denied.  User ip (" << ipAddr
        << ") or trusted client status (" << trustedClient << ") fails auth checks in default group for client type ("
        << clientType << ":" << ClientTypeToString(clientType) << ").");

    accessGroupName.set(nullptr);
    return false;
}

void UserSessionsMasterImpl::updateLocalUserSessionCountsAddOrRemoveSession(const UserSessionMaster& session, bool addSession)
{
    const ClientType clientType = session.getClientType();
    const ClientState& clientState = session.getClientState();
    const UserSessionExtendedData& extendedData = session.getExtendedData();

    if (addSession)
    {
        mLocalUserSessionCounts.increment(1, session.getProductName(), clientType, session.getClientPlatform(), clientState.getStatus(), clientState.getMode(), extendedData.getBestPingSiteAlias());
    }
    else
    {
        mLocalUserSessionCounts.decrement(1, session.getProductName(), clientType, session.getClientPlatform(), clientState.getStatus(), clientState.getMode(), extendedData.getBestPingSiteAlias());
    }
}

void setUserDeviceIdMap(UserSessionMasterPtr userSession, ClientPlatformType platform, RiverPoster::PINEventHeaderCore &headerCore)
{
    if (*userSession->getUniqueDeviceId() == '\0')
        return; // no unique id is available, so don't add an empty entry.

    EA::TDF::TdfString* deviceIdString = nullptr;
    switch (platform)
    {
        // Note:  The UniqueDeviceId we get back from Nucleus is basically worthless on Mobile platforms.
        //  If the login is via HTTP (Companion app) then the device id is a made up value.  (pid_type == NUCLEUS)  
        //  Otherwise, the value is whatever Nimble sent up, with no indication which device type it is.  (pid_type == AUTHENTICATOR_ANONYMOUS)

    // case ios:       deviceIdString = &headerCore.getDeviceIdMap()["idfa" / "idfv" / "machash"];     break;  // Unable to distinguish
    // case android:   deviceIdString = &headerCore.getDeviceIdMap()["androidid" / "gaid"];            break;  // Unable to distinguish
    case xone:
    case xbsx:
        deviceIdString = &headerCore.getDeviceIdMap()["xboxid"];
        break;
    case ps4:
    case ps5:
        deviceIdString = &headerCore.getDeviceIdMap()["psid"];
        break;
    // case nx:        deviceIdString = &headerCore.getDeviceIdMap()["nxid"];           break;      // nxid doesn't exist in PIN, but device id returned by Nucleus is valid.
    case pc:        deviceIdString = &headerCore.getDeviceIdMap()["eadeviceid"];     break;
    case steam:     deviceIdString = &headerCore.getDeviceIdMap()["eadeviceid"];     break; //eadeviceid is used for both PC and Steam
    //case stadia:    deviceIdString = &headerCore.getDeviceIdMap()["eadeviceid"];     break; //NA on Stadia based on conversations with C&I and PIN team. C&I does not have deviceId on Stadia and PIN team does not have any use of eadeviceid.
    default:        break;
    }

    if (deviceIdString != nullptr)
        *deviceIdString = userSession->getUniqueDeviceId();
}

bool UserSessionsMasterImpl::getCrossplayOptInForPin(const UserSessionMasterPtr userSession, ClientPlatformType clientPlatform) const
{
    // For pin reporting, we should also check whether the platform is allowed to crossplay (in addition to user's own setting). This way, we don't end up reporting user as opted into 
    // crossplay for single platform or shared cluster without crossplay (because by default, our DB/User session layer opts a user into crossplay regardless of the deployment type)  
    bool hasOptedInCrossplay = false;
    if (gUserSessionManager->getUnrestrictedPlatforms(clientPlatform).size() > 1)
    {
        if (userSession != nullptr)
        {
            hasOptedInCrossplay = userSession->isCrossplayEnabled();
        }
    }

    return hasOptedInCrossplay;
}

void UserSessionsMasterImpl::generateLoginEvent(BlazeRpcError err, UserSessionId sessionId, UserSessionType sessionType, const PlatformInfo& platformInfo, const char8_t* deviceId, DeviceLocality deviceLocality,
                                            PersonaId personaId, const char8_t* personaName, Locale connLocale, uint32_t connCountry, const char8_t* sdkVersion, ClientType clientType,
                                            const char8_t* ipaddress, const char8_t* serviceName, const char8_t* productName, const GeoLocationData& geoLocData, const char8_t* projectId, Locale accountLocale /*=LOCALE_BLANK*/, uint32_t accountCountry /*=0*/,
                                            const char8_t* clientVersion/* = ""*/, bool includePINErrorEvent/*= false*/, bool isUnderage/* = false*/)
{

    Login event;

    platformInfo.copyInto(event.getPlatformInfo());

    if (projectId != nullptr)
        event.setProjectId(projectId);

    event.setBlazeId(personaId);
    event.setServiceName(serviceName);

    event.setAccountId(event.getPlatformInfo().getEaIds().getNucleusAccountId());
    event.setExternalId(getExternalIdFromPlatformInfo(event.getPlatformInfo()));
    event.setSessionType(sessionType);
    event.setClientPlatform(event.getPlatformInfo().getClientPlatform());
    event.setUniqueDeviceId(deviceId);
    event.setDeviceLocality(deviceLocality);

    if (personaName != nullptr)
        event.setPersonaName(personaName);

    if (connLocale != LOCALE_BLANK)
    {
        char8_t buf[32];
        LocaleTokenCreateLocalityString(buf, connLocale);
        event.setConnectionLocale(buf);
    }

    if (connCountry != 0)
    {
        char8_t buf[32];
        LocaleTokenCreateCountryString(buf, connCountry); // connection/session country/region is not locale-related, but re-using locale macro for convenience
        event.setConnectionCountry(buf);
    }

    if (accountLocale != LOCALE_BLANK)
    {
        char8_t buf[32];
        LocaleTokenCreateLocalityString(buf, accountLocale);
        event.setLocale(buf);
    }

    if (accountCountry != 0)
    {
        char8_t buf[32];
        LocaleTokenCreateCountryString(buf, accountCountry); // account country is not locale-related, but re-using locale macro for convenience
        event.setCountry(buf);
    }

    event.setClientType(ClientTypeToString(clientType));
    event.setError(ErrorHelp::getErrorName(err));

    if (ipaddress != nullptr)
        event.setIpAddress(ipaddress);

    event.setGeoCity(geoLocData.getCity());
    event.setGeoCountry(geoLocData.getCountry());
    event.setGeoStateRegion(geoLocData.getStateRegion());
    event.setLatitude(geoLocData.getLatitude());
    event.setLongitude(geoLocData.getLongitude());
    event.setTimeZone(geoLocData.getTimeZone());
    event.setISP(geoLocData.getISP());

    event.setSessionId(sessionId);

    gEventManager->submitEvent(err == ERR_OK ? UserSessionsSlave::EVENT_LOGINEVENT : UserSessionsSlave::EVENT_LOGINFAILUREEVENT, event, true);

    // GOS-31099 - Ignore dedicated servers and trusted sessions until we can implement
    // proper handling of non-player sessions for PIN
    if (!((clientType == CLIENT_TYPE_DEDICATED_SERVER) || (sessionType == USER_SESSION_TRUSTED)))
    {
        RiverPoster::LoginEventPtr loginEvent = BLAZE_NEW RiverPoster::LoginEvent;
        loginEvent->setStatus((err == ERR_OK ? "success" : "error"));
        loginEvent->setStatusCode(ErrorHelp::getErrorName(err));
        loginEvent->setType(RiverPoster::PIN_SDK_TYPE_BLAZE);

        RiverPoster::PINEventHeaderCore &headerCore = loginEvent->getHeaderCore();

        if (UserSession::isValidUserSessionId(sessionId))
        {
            StringBuilder str;
            str << sessionId;
            headerCore.setSessionId(str.get());

            auto userSession = mUserSessionMasterPtrByIdMap.find(sessionId);
            if (userSession != mUserSessionMasterPtrByIdMap.end())
                setUserDeviceIdMap(userSession->second, event.getPlatformInfo().getClientPlatform(), headerCore);
        }

        headerCore.setEventName(RiverPoster::LOGIN_EVENT_NAME);
        InetAddress clientIp(ipaddress);
        headerCore.setClientIp(clientIp.getHostname());
        headerCore.setSdkVersion(sdkVersion);
        
        EA::TDF::GenericTdfType *headerCoreTdf = headerCore.getCustomDataMap().allocate_element();
        headerCoreTdf->set(!getCrossplayOptInForPin(getLocalSession(sessionId), event.getPlatformInfo().getClientPlatform()));
        headerCore.getCustomDataMap()[RiverPoster::PIN_CROSSPLAY_OPTOUT_FLAG] = headerCoreTdf;

        if (err != ERR_OK)
        {
            // Failed logins are a special case, because we don't have a UserSession, but we do have enough data to populate the PIN event
            // (If this section is changed, update UserSessionsMasterImpl::sendLoginPINError below.)
            UserSessionExistenceData dummySession;
            dummySession.setBlazeId(personaId);
            dummySession.setClientType(clientType);
            platformInfo.copyInto(dummySession.getPlatformInfo());
            dummySession.setClientVersion(clientVersion);
            dummySession.setSessionLocale(connLocale);
            dummySession.setServiceName(serviceName);
            dummySession.setProductName(productName);
            dummySession.getSessionFlags().setIsChildAccount(isUnderage);

            gUserSessionManager->sendPINLoginFailureEvent(loginEvent, sessionId, dummySession);

            if (includePINErrorEvent)
            {
                if (err == ERR_COULDNT_CONNECT)
                {
                    gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE),
                        RiverPoster::external_system_unavailable, serviceName, &headerCore);
                }
                else
                {
                    gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::authentication_failed, serviceName, &headerCore);
                }
            }
        }
        else
        {
            PINSubmissionPtr req = BLAZE_NEW PINSubmission;
            RiverPoster::PINEventList *pinEventList = req->getEventsMap().allocate_element();
            EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinEvent = pinEventList->pull_back();
            pinEvent->set(*loginEvent);
            req->getEventsMap()[sessionId] = pinEventList;
            gUserSessionManager->sendPINEvents(req);
        }
    }
}

// send a PIN error event only
void UserSessionsMasterImpl::sendLoginPINError(BlazeRpcError err, UserSessionId sessionId, const PlatformInfo& platformInfo,
    PersonaId personaId, Locale connLocale, uint32_t connCountry, const char8_t* sdkVersion, ClientType clientType,
    const char8_t* ipaddress, const char8_t* serviceName, const char8_t* productName,
    const char8_t* clientVersion, bool isUnderage/* = false*/)
{
    RiverPoster::PINEventHeaderCore headerCore;

    if (UserSession::isValidUserSessionId(sessionId))
    {
        StringBuilder str;
        str << sessionId;
        headerCore.setSessionId(str.get());

        auto userSession = mUserSessionMasterPtrByIdMap.find(sessionId);
        if (userSession != mUserSessionMasterPtrByIdMap.end())
            setUserDeviceIdMap(userSession->second, platformInfo.getClientPlatform(), headerCore);
    }

    headerCore.setEventName(RiverPoster::LOGIN_EVENT_NAME);
    InetAddress clientIp(ipaddress);
    headerCore.setClientIp(clientIp.getHostname());
    headerCore.setSdkVersion(sdkVersion);

    if (err != ERR_OK)
    {
        // Failed logins are a special case, because we don't have a UserSession, but we do have enough data to populate the PIN event
        // (If this section is changed, update UserSessionsMasterImpl::generateLoginEvent above.)
        UserSessionExistenceData dummySession;
        dummySession.setBlazeId(personaId);
        dummySession.setClientType(clientType);
        platformInfo.copyInto(dummySession.getPlatformInfo());
        dummySession.setClientVersion(clientVersion);
        dummySession.setSessionLocale(connLocale);
        dummySession.setServiceName(serviceName);
        dummySession.setProductName(productName);
        dummySession.getSessionFlags().setIsChildAccount(isUnderage);

        gUserSessionManager->filloutPINEventHeader(headerCore, sessionId, &dummySession);

        if (err == ERR_COULDNT_CONNECT)
        {
            gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE),
                RiverPoster::external_system_unavailable, serviceName, &headerCore);
        }
        else
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::authentication_failed, serviceName, &headerCore);
        }
    }
}

void UserSessionsMasterImpl::generateLogoutEvent(const UserSessionMasterPtr userSession, UserSessionDisconnectType disconnectType, int32_t socketError)
{
    Logout event;

    auto projectIdIter = mProductProjectIdMap.find(userSession->getProductName());
    if (projectIdIter != mProductProjectIdMap.end())
    {
        event.setProjectId(projectIdIter->second);
    }

    auto contentIdIter = mProductTrialContentIdMap.find(userSession->getProductName());
    if (contentIdIter != mProductTrialContentIdMap.end())
    {
        contentIdIter->second->copyInto(event.getTrialContentIds());
    }

    BlazeRpcError err = ERR_OK;
    if (disconnectType == DISCONNECTTYPE_DUPLICATE_LOGIN)
        err = ERR_DUPLICATE_LOGIN;
    else if (disconnectType != DISCONNECTTYPE_CLIENT_LOGOUT)
        err = ERR_DISCONNECTED;

    userSession->getUserInfo().getPlatformInfo().copyInto(event.getPlatformInfo());

    uint64_t sessionDuration = (uint64_t)(TimeValue::getTimeOfDay() - userSession->getCreationTime()).getSec();
    event.setBlazeId(userSession->getBlazeId());
    event.setServiceName(userSession->getServiceName());
    event.setSessionId(userSession->getUserSessionId());
    event.setTimeOnline((uint32_t)sessionDuration);
    event.setSessionType(userSession->getUserSessionType());
    event.setClientType(ClientTypeToString(userSession->getClientType()));
    event.setPersonaName(userSession->getUserInfo().getPersonaName());
    event.setAccountId(userSession->getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId());
    event.setExternalId(getExternalIdFromPlatformInfo(userSession->getUserInfo().getPlatformInfo()));
    event.setUniqueDeviceId(userSession->getUniqueDeviceId());
    event.setDeviceLocality(userSession->getDeviceLocality());
    event.setIpAddress(userSession->getConnectionAddr());
    userSession->getClientState().copyInto(event.getClientState());
    userSession->getClientMetrics().copyInto(event.getClientMetrics());
    userSession->getClientUserMetrics().copyInto(event.getClientUserMetrics());

    // qos results
    const Blaze::PingSiteLatencyByAliasMap &pingSiteLatencyMap = userSession->getExtendedData().getLatencyMap();
    if (!pingSiteLatencyMap.empty())
    {
        eastl::string networkInfoBuf;
        for (Blaze::PingSiteLatencyByAliasMap::const_iterator latencyItr = pingSiteLatencyMap.begin(); latencyItr != pingSiteLatencyMap.end(); ++latencyItr)
        {
            networkInfoBuf.append_sprintf("%s=%d", latencyItr->first.c_str(), latencyItr->second);
            networkInfoBuf.append(",");
        }
        networkInfoBuf.pop_back(); // remove the last comma
        event.setNetworkInfo(networkInfoBuf.c_str());
    }

    userSession->getExtendedData().getQosData().copyInto(event.getQosData());

    event.setCloseReason(UserSessionDisconnectTypeToString(disconnectType));
    event.setSocketError(socketError);
    event.setError(ErrorHelp::getErrorName(err));

    gEventManager->submitEvent(err == ERR_OK ? UserSessionsSlave::EVENT_LOGOUTEVENT : UserSessionsSlave::EVENT_DISCONNECTEVENT, event, true);

    // send events to PIN
    PINSubmissionPtr req = BLAZE_NEW PINSubmission;
    RiverPoster::PINEventList *pinEventList = req->getEventsMap().allocate_element();
    InetAddress clientIp(userSession->getConnectionAddr());

    // accessibility PIN event (including tts and stt)
    if ((event.getClientUserMetrics().getSttEventCount() > 0) || (event.getClientUserMetrics().getTtsEventCount() > 0))
    {
        RiverPoster::AccessibilityUsageEventPtr accessibilityEvent = BLAZE_NEW RiverPoster::AccessibilityUsageEvent;
        accessibilityEvent->getHeaderCore().setEventName(RiverPoster::ACCESSIBILITY_EVENT_NAME);
        accessibilityEvent->getHeaderCore().setClientIp(clientIp.getHostname());

        // stt event
        if (event.getClientUserMetrics().getSttEventCount() > 0)
        {
            RiverPoster::AccessibilityFeatureAttributesPtr sttEvent = accessibilityEvent->getFeatureAttr().allocate_element();
            sttEvent->setFeature("stt");
            sttEvent->setUsageCount(event.getClientUserMetrics().getSttEventCount());
            sttEvent->setEmptyResultCount(event.getClientUserMetrics().getSttEmptyResultCount());
            sttEvent->setErrorCount(event.getClientUserMetrics().getSttErrorCount());
            sttEvent->setResponseDelay(event.getClientUserMetrics().getSttDelay());
            sttEvent->getElementAttr().setSubmitType("ms");
            sttEvent->getElementAttr().setSubmitCount(event.getClientUserMetrics().getSttDurationMsSent());
            sttEvent->getElementAttr().setResultType("char");
            sttEvent->getElementAttr().setResultCount(event.getClientUserMetrics().getSttCharCountRecv());
            accessibilityEvent->getFeatureAttr().push_back(sttEvent);
        }

        // tts event
        if (event.getClientUserMetrics().getTtsEventCount() > 0)
        {
            RiverPoster::AccessibilityFeatureAttributesPtr ttsEvent = accessibilityEvent->getFeatureAttr().allocate_element();
            ttsEvent->setFeature("tts");
            ttsEvent->setUsageCount(event.getClientUserMetrics().getTtsEventCount());
            ttsEvent->setEmptyResultCount(event.getClientUserMetrics().getTtsEmptyResultCount());
            ttsEvent->setErrorCount(event.getClientUserMetrics().getTtsErrorCount());
            ttsEvent->setResponseDelay(event.getClientUserMetrics().getTtsDelay());
            ttsEvent->getElementAttr().setSubmitType("char");
            ttsEvent->getElementAttr().setSubmitCount(event.getClientUserMetrics().getTtsCharCountSent());
            ttsEvent->getElementAttr().setResultType("ms");
            ttsEvent->getElementAttr().setResultCount(event.getClientUserMetrics().getTtsDurationMsRecv());
            accessibilityEvent->getFeatureAttr().push_back(ttsEvent);
        }
        
        // add both the stt/tts event to the pin event list
        EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinAccessibilityEvent = pinEventList->pull_back();
        pinAccessibilityEvent->set(*accessibilityEvent);
    }

    //logout event
    RiverPoster::LogoutEventPtr logoutEvent = BLAZE_NEW RiverPoster::LogoutEvent;
    setUserDeviceIdMap(userSession, event.getPlatformInfo().getClientPlatform(), logoutEvent->getHeaderCore());
    logoutEvent->getHeaderCore().setEventName(RiverPoster::LOGOUT_EVENT_NAME);
    logoutEvent->getHeaderCore().setClientIp(clientIp.getHostname());
    
    EA::TDF::GenericTdfType *headerCoreTdf = logoutEvent->getHeaderCore().getCustomDataMap().allocate_element();
    headerCoreTdf->set(!getCrossplayOptInForPin(userSession, event.getPlatformInfo().getClientPlatform()));
    logoutEvent->getHeaderCore().getCustomDataMap()[RiverPoster::PIN_CROSSPLAY_OPTOUT_FLAG] = headerCoreTdf;

    logoutEvent->setEndReason((err == ERR_OK ? "normal" : "error"));
    logoutEvent->setStatusCode(ErrorHelp::getErrorName(err));
    logoutEvent->setType(RiverPoster::PIN_SDK_TYPE_BLAZE);
    logoutEvent->setSessionDuration(sessionDuration);
    EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinLogoutEvent = pinEventList->pull_back();
    pinLogoutEvent->set(*logoutEvent);

    req->getEventsMap()[userSession->getId()] = pinEventList;
    gUserSessionManager->sendPINEvents(req);
}

/*! **********************************************************************************************/
/*! \brief Check if local user session is unresponsive. Note: if user session is not local returns false.
**************************************************************************************************/
bool UserSessionsMasterImpl::isLocalUserSessionUnresponsive(UserSessionId userSessionId)
{
    UserSessionMasterPtr localUserSession = getLocalSession(userSessionId);
    return ((localUserSession != nullptr) && localUserSession->isConnectionUnresponsive());
}

} //namespace Blaze
