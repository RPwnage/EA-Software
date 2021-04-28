/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/notificationcache/notificationcacheslaveimpl.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersessionmanager.h"

#include "framework/connection/connectionmanager.h"
#include "framework/controller/controller.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/replication/replicator.h"
#include "framework/util/hashutil.h"
#include "framework/tdf/userdefines.h"
#include "framework/slivers/slivermanager.h"

#include <EASTL/algorithm.h>

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

FiberLocalWrappedPtr<UserSessionExistencePtr, UserSessionExistence> gCurrentUserSession;

void intrusive_ptr_add_ref(UserSessionExistence* ptr)
{
    ++ptr->mRefCount;
}

void intrusive_ptr_release(UserSessionExistence* ptr)
{
    if (ptr->mRefCount > 0)
    {
        if (--ptr->mRefCount == 0)
        {
            delete ptr;
        }
    }
    else
    {
        EA_FAIL_MSG("Invalid UserSessionExistence::mRefCount");
    }
}

FiberLocalVar<UserSessionId> UserSession::msCurrentUserSessionId(INVALID_USER_SESSION_ID);
FiberLocalVar<uint32_t> UserSession::msSuperUserPrivilegeCounter(0);

BlazeRpcError convertToPlatformInfo(PlatformInfo& platformInfo, ExternalId extId, const char8_t* extString, AccountId accountId, ClientPlatformType platform)
{
    return convertToPlatformInfo(platformInfo, extId, extString, accountId, INVALID_ORIGIN_PERSONA_ID, nullptr, platform);
}
BlazeRpcError convertToPlatformInfo(CoreIdentification& coreIdent)
{
    return convertToPlatformInfo(coreIdent.getPlatformInfo(), coreIdent.getExternalId(), nullptr, INVALID_ACCOUNT_ID, INVALID);
}
BlazeRpcError convertToPlatformInfo(UserIdentification& userIdent)
{
    return convertToPlatformInfo(userIdent.getPlatformInfo(), userIdent.getExternalId(), nullptr, userIdent.getAccountId(), userIdent.getOriginPersonaId(), nullptr, INVALID);
}
BlazeRpcError convertToPlatformInfo(PlatformInfo& platformInfo, ExternalId extId, const char8_t* extString, AccountId accountId, OriginPersonaId originPersonaId, const char8_t* originPersonaName, ClientPlatformType platform)
{
    if (accountId != INVALID_ACCOUNT_ID)
    {
        if (platformInfo.getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID && platformInfo.getEaIds().getNucleusAccountId() != accountId)
        {
            BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: AccountId mismatch between platform info (" << platformInfo.getEaIds().getNucleusAccountId() << ") and provided value (" << accountId << ").");
            return ERR_SYSTEM;
        }
        platformInfo.getEaIds().setNucleusAccountId(accountId);
    }

    if (originPersonaId != INVALID_ORIGIN_PERSONA_ID)
    {
        if (platformInfo.getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID && platformInfo.getEaIds().getOriginPersonaId() != originPersonaId)
        {
            BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: OriginPersonaId mismatch between platform info (" << platformInfo.getEaIds().getOriginPersonaId() << ") and provided value (" << originPersonaId << ").");
            return ERR_SYSTEM;
        }
        platformInfo.getEaIds().setOriginPersonaId(originPersonaId);
    }

    if (originPersonaName != nullptr && originPersonaName[0] != '\0')
    {
        // Users can change their Origin persona names
        platformInfo.getEaIds().setOriginPersonaName(originPersonaName);
    }

    if (platform != INVALID && platform != NATIVE)
    {
        // Indicates that the PlatformInfo was filled in correctly.  Verify that there are no unexpected differences:
        if (platformInfo.getClientPlatform() != INVALID && platform != platformInfo.getClientPlatform())
        {
            BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Platform mismatch between platform info (" << platformInfo.getClientPlatform() << ") and provided value (" << platform << ").");
            return ERR_SYSTEM;
        }
        platformInfo.setClientPlatform(platform);
    }

    // If the request is coming in, and doesn't set the platform, assume that it uses the current user session's:
    if (platformInfo.getClientPlatform() == INVALID || platformInfo.getClientPlatform() == NATIVE)
    {
        if (gCurrentUserSession != nullptr)
            platformInfo.setClientPlatform(gCurrentUserSession->getClientPlatform());
        else if (!gController->isSharedCluster())
            platformInfo.setClientPlatform(gController->getDefaultClientPlatform());
    }


    // Once we have the platform set, we can try to fill in the correct external information:
    switch (platformInfo.getClientPlatform())
    {
    case xbsx:
    case xone:
        if (extId != INVALID_EXTERNAL_ID)
        {
            if (platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID && platformInfo.getExternalIds().getXblAccountId() != extId)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Xbl id mismatch between platform info (" << platformInfo.getExternalIds().getXblAccountId() << ") and provided value (" << extId << ").");
                return ERR_SYSTEM;
            }
            platformInfo.getExternalIds().setXblAccountId(extId);
        }
        break;
    case ps5:
    case ps4:
        if (extId != INVALID_EXTERNAL_ID)
        {
            if (platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID && platformInfo.getExternalIds().getPsnAccountId() != extId)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Psn id mismatch between platform info (" << platformInfo.getExternalIds().getPsnAccountId() << ") and provided value (" << extId << ").");
                return ERR_SYSTEM;
            }
            platformInfo.getExternalIds().setPsnAccountId(extId);
        }
        break;
    case pc:
        // PC should be handled by account id:
        // But for back compat with old functionality (where Nucleus returned the account id as the external id for PC users) we attempt to convert here:
        if (extId != INVALID_EXTERNAL_ID)
        {
            if (accountId != INVALID_ACCOUNT_ID && (AccountId)extId != accountId)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: AccountId/ExternalId mismatch between provided extId (" << extId << ") and provided accountId (" << accountId << ").");
                return ERR_SYSTEM;
            }

            if (platformInfo.getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID && platformInfo.getEaIds().getNucleusAccountId() != (AccountId)extId)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: AccountId mismatch between platform info (" << platformInfo.getEaIds().getNucleusAccountId() << ") and provided extId (" << extId << ").");
                return ERR_SYSTEM;
            }
            platformInfo.getEaIds().setNucleusAccountId((AccountId)extId);
        }

        break;
    case steam:
        if (extId != INVALID_EXTERNAL_ID)
        {
            if (platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID && platformInfo.getExternalIds().getSteamAccountId() != extId)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Steam id mismatch between platform info (" << platformInfo.getExternalIds().getSteamAccountId() << ") and provided value (" << extId << ").");
                return ERR_SYSTEM;
            }
            platformInfo.getExternalIds().setSteamAccountId(extId);
        }
        break; 
    case stadia:
        if (extId != INVALID_EXTERNAL_ID)
        {
            if (platformInfo.getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID && platformInfo.getExternalIds().getStadiaAccountId() != extId)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Stadia id mismatch between platform info (" << platformInfo.getExternalIds().getStadiaAccountId() << ") and provided value (" << extId << ").");
                return ERR_SYSTEM;
            }
            platformInfo.getExternalIds().setStadiaAccountId(extId);
        }
        break; 
    case nx:
        if (extString != nullptr && extString[0] != '\0')
        {
            if (!platformInfo.getExternalIds().getSwitchIdAsTdfString().empty() && platformInfo.getExternalIds().getSwitchIdAsTdfString() == extString)
            {
                BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Switch id mismatch between account identifiers (" << platformInfo.getExternalIds().getSwitchId() << ") and provided external string value (" << extString << ")");
                return ERR_SYSTEM;
            }
            platformInfo.getExternalIds().setSwitchId(extString);
        }
        break;
    case common:
        // Common is only valid in cases where an external service performed a trusted login.
        break;
    default:
        BLAZE_ERR_LOG(Log::USER, "convertToPlatformInfo: Unexpected platform provided - (" << ClientPlatformTypeToString(platformInfo.getClientPlatform()) << ").  Unable to convert.");
        return ERR_SYSTEM;
    }

    return ERR_OK;
}

Blaze::BlazeRpcError convertFromPlatformInfo(const PlatformInfo& platformInfo, ExternalId* extId, eastl::string* extString, AccountId* accountId)
{
    // We assume that the PlatformInfo will always be filled out, since all the code should have been updated to use the PlatformInfo internally.
    if (accountId != nullptr)
    {
        *accountId = platformInfo.getEaIds().getNucleusAccountId();
    }


    // Once we have the platform set, we can try to fill in the correct external information:
    switch (platformInfo.getClientPlatform())
    {
    case xbsx:
    case xone:
        if (extId != nullptr)
        {
            *extId = platformInfo.getExternalIds().getXblAccountId();
        }
        break;
    case ps5:
    case ps4:
        if (extId != nullptr)
        {
            *extId = platformInfo.getExternalIds().getPsnAccountId();
        }
        break;
    case pc:
        // PC should be handled by account id:
        if (accountId != nullptr)
        {
            *accountId = platformInfo.getEaIds().getNucleusAccountId();
        }
        if (extId != nullptr)
        {
            // Nucleus returns the account id as the ExternalRefValue from login.  (This was not documented by Blaze)
            // It's possible that some code relies on this functionality, so we maintain it here until crossplay is fully implemented & this function is removed.
            *extId = (ExternalId) platformInfo.getEaIds().getNucleusAccountId();
        }
        break;
    case nx:
        if (extString != nullptr)
        {
            *extString = platformInfo.getExternalIds().getSwitchId();
        }
        break;
    case steam:
        if (extId != nullptr)
        {
            *extId = platformInfo.getExternalIds().getSteamAccountId();
        }
        break; 
    case stadia:
        if (extId != nullptr)
        {
            *extId = platformInfo.getExternalIds().getStadiaAccountId();
        }
        break; 
    case common:
        // Common is only valid in cases where an external service performed a trusted login. 
        break;

    default:
        BLAZE_ERR_LOG(Log::USER, "convertFromPlatformInfo: Unexpected platform provided - (" << ClientPlatformTypeToString(platformInfo.getClientPlatform()) << ").  Unable to convert.");
        return ERR_SYSTEM;
    }

    return ERR_OK;
}

bool has1stPartyPlatformInfo(const PlatformInfo& platformInfo)
{
    return (platformInfo.getClientPlatform() == nx && !platformInfo.getExternalIds().getSwitchIdAsTdfString().empty()) ||
           (getExternalIdFromPlatformInfo(platformInfo) != INVALID_EXTERNAL_ID);
}
const char8_t* platformInfoToString(const PlatformInfo& platformInfo, eastl::string& str)
{
    str = "OriginPersonaId=";
    str.append_sprintf("%" PRIu64 ", NucleusAccountId=%" PRId64, platformInfo.getEaIds().getOriginPersonaId(), platformInfo.getEaIds().getNucleusAccountId());
    switch (platformInfo.getClientPlatform())
    {
    case ps4:
    case ps5:
        str.append_sprintf(", psnid=%" PRIu64, platformInfo.getExternalIds().getPsnAccountId());
        break;
    case xone:
    case xbsx:
        str.append_sprintf(", xblid=%" PRIu64, platformInfo.getExternalIds().getXblAccountId());
        break;
    case nx:
        str.append_sprintf(", switchid=%s", platformInfo.getExternalIds().getSwitchId());
        break;
    case steam:
        str.append_sprintf(", steamid=%" PRIu64, platformInfo.getExternalIds().getSteamAccountId());
        break; 
    case stadia:
        str.append_sprintf(", stadiaid=%" PRIu64, platformInfo.getExternalIds().getStadiaAccountId());
        break; 
    default:
        break;
    }
    return str.c_str();
}

Blaze::ExternalId getExternalIdFromPlatformInfo(const PlatformInfo& platformInfo)
{
    ExternalId extId = INVALID_EXTERNAL_ID;
    convertFromPlatformInfo(platformInfo, &extId, nullptr, nullptr);
    return extId;
}



/*** Public Methods For UserSession **************************************************************/
UserSessionExistence::UserSessionExistence() :
    UserSession(nullptr)
{
    BlazeIdListNode::mpNext = BlazeIdListNode::mpPrev = nullptr;
    AccountIdListNode::mpNext = AccountIdListNode::mpPrev = nullptr;
    ExternalPsnAccountIdListNode::mpNext = ExternalPsnAccountIdListNode::mpPrev = nullptr;
    ExternalXblAccountIdListNode::mpNext = ExternalXblAccountIdListNode::mpPrev = nullptr;
    ExternalSwitchIdListNode::mpNext = ExternalSwitchIdListNode::mpPrev = nullptr;
    ExternalSteamAccountIdListNode::mpNext = ExternalSteamAccountIdListNode::mpPrev = nullptr;
    ExternalStadiaAccountIdListNode::mpNext = ExternalStadiaAccountIdListNode::mpPrev = nullptr;
    PrimarySessionListNode::mpNext = PrimarySessionListNode::mpPrev = nullptr;
}

UserSessionExistence::~UserSessionExistence()
{
    removeFromAllLists();
}

void UserSessionExistence::removeFromAllLists()
{
    if (BlazeIdListNode::mpNext != nullptr && BlazeIdListNode::mpPrev != nullptr)
    {
        BlazeIdExistenceList::remove(*this);
        BlazeIdListNode::mpNext = BlazeIdListNode::mpPrev = nullptr;
    }
    if (AccountIdListNode::mpNext != nullptr && AccountIdListNode::mpPrev != nullptr)
    {
        AccountIdExistenceList::remove(*this);
        AccountIdListNode::mpNext = AccountIdListNode::mpPrev = nullptr;
    }
    if (ExternalPsnAccountIdListNode::mpNext != nullptr && ExternalPsnAccountIdListNode::mpPrev != nullptr)
    {
        ExternalPsnAccountIdExistenceList::remove(*this);
        ExternalPsnAccountIdListNode::mpNext = ExternalPsnAccountIdListNode::mpPrev = nullptr;
    }
    if (ExternalXblAccountIdListNode::mpNext != nullptr && ExternalXblAccountIdListNode::mpPrev != nullptr)
    {
        ExternalXblAccountIdExistenceList::remove(*this);
        ExternalXblAccountIdListNode::mpNext = ExternalXblAccountIdListNode::mpPrev = nullptr;
    }
    if (ExternalSwitchIdListNode::mpNext != nullptr && ExternalSwitchIdListNode::mpPrev != nullptr)
    {
        ExternalSwitchIdExistenceList::remove(*this);
        ExternalSwitchIdListNode::mpNext = ExternalSwitchIdListNode::mpPrev = nullptr;
    }
    if (ExternalSteamAccountIdListNode::mpNext != nullptr && ExternalSteamAccountIdListNode::mpPrev != nullptr)
    {
        ExternalSteamAccountIdExistenceList::remove(*this);
        ExternalSteamAccountIdListNode::mpNext = ExternalSteamAccountIdListNode::mpPrev = nullptr;
    }
    if (ExternalStadiaAccountIdListNode::mpNext != nullptr && ExternalStadiaAccountIdListNode::mpPrev != nullptr)
    {
        ExternalStadiaAccountIdExistenceList::remove(*this);
        ExternalStadiaAccountIdListNode::mpNext = ExternalStadiaAccountIdListNode::mpPrev = nullptr;
    }

    if (PrimarySessionListNode::mpNext != nullptr && PrimarySessionListNode::mpPrev != nullptr)
    {
        PrimarySessionExistenceList::remove(*this);
        PrimarySessionListNode::mpNext = PrimarySessionListNode::mpPrev = nullptr;
    }
}

BlazeRpcError UserSessionExistence::fetchSessionData(UserSessionDataPtr& sessionData) const
{
    FetchUserSessionsRequest request;
    FetchUserSessionsResponse response;

    request.getUserSessionIds().push_back(getUserSessionId());

    RpcCallOptions opts;
    opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, getUserSessionId());

    BlazeRpcError err = gUserSessionManager->getMaster()->fetchUserSessions(request, response, opts);
    if (err == ERR_OK)
        sessionData = response.getData().front().get();

    return err;
}

BlazeRpcError UserSessionExistence::fetchUserInfo(UserInfoPtr& userInfo) const
{
    BlazeRpcError err;
    if (!UserSessionManager::isStatelessUser(getBlazeId()))
    {
        // Normally, we're working with "statefull" UserSessions, so lookup via the real blazeId.
        err = gUserSessionManager->lookupUserInfoByBlazeId(getBlazeId(), userInfo);
    }
    else
    {
        // If the blazeId is "stateless", meaning a BlazeId with a negative value, then we need to fetch the
        // UserInfo via a different lookup method.  There are a number of lookup options.  We can use the platform info.
        err = gUserSessionManager->lookupUserInfoByPlatformInfo(getPlatformInfo(), userInfo);
    }

    return err;
}

UserSession::UserSession(UserSessionExistenceData* existenceData)
    : mRefCount(0), mExistenceData(existenceData)
{
}

UserSession::~UserSession()
{
    EA_ASSERT(mRefCount == 0);
}

bool UserSession::isOwnedByThisInstance(UserSessionId userSessionId)
{
    return ((gUserSessionMaster != nullptr) && (getOwnerInstanceId(userSessionId) == gController->getInstanceId()));
}

InstanceId UserSession::getOwnerInstanceId(UserSessionId userSessionId)
{
    InstanceId instanceId = INVALID_INSTANCE_ID;

    if (userSessionId != INVALID_SESSION_ID)
    {
        SliverId sliverId = GetSliverIdFromSliverKey(userSessionId);
        if (sliverId != INVALID_SLIVER_ID)
            instanceId = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, sliverId);
    }

    return instanceId;
}

SliverIdentity UserSession::makeSliverIdentity(UserSessionId userSessionId)
{
    return MakeSliverIdentity(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(userSessionId));
}

bool UserSession::isLocal() const
{
    if (isOwnedByThisInstance(getUserSessionId()))
        return true;
    auto& instanceIds = getInstanceIds();
    auto typeIndex = gController->getInstanceTypeIndex();
    if (typeIndex < instanceIds.size())
        return instanceIds[typeIndex] == gController->getInstanceId();
    else
        return false;
}

bool UserSession::isAuxLocal() const
{
    if (isOwnedByThisInstance(getUserSessionId()))
        return false;
    auto& instanceIds = getInstanceIds();
    auto typeIndex = gController->getInstanceTypeIndex();
    if (typeIndex < instanceIds.size())
        return instanceIds[typeIndex] == gController->getInstanceId();
    else
        return false;
}

const ClientTypeFlags& UserSession::getClientTypeFlags() const
{
    return gUserSessionManager->getClientTypeDescription(getClientType());
}

bool UserSession::isGameplayUser(UserSessionId userSessionId)
{
    UserSessionExistencePtr userSession = gUserSessionManager->getSession(userSessionId);
    return (userSession != nullptr) && userSession->isGameplayUser();
}

bool UserSession::isAuthorized(const Authorization::Permission& permission, bool suppressWarnLog) const
{       
    return gUserSessionManager->isSessionAuthorized(getUserSessionId(), permission, suppressWarnLog);
}

bool UserSession::isAuthorized(UserSessionId userSessionId, const Authorization::Permission& permission, bool suppressWarnLog)
{
    return gUserSessionManager->isSessionAuthorized(userSessionId, permission, suppressWarnLog);
}


// UED Values that are tracked by Name are managed on the UserSessionManager with UserExtendedDataName getUserExtendedDataId
bool UserSession::getDataValue(const UserExtendedDataMap& extendedDataMap, const UserExtendedDataName& name, UserExtendedDataValue &value)
{
    uint16_t componentId = 0;
    uint16_t dataId = 0;
    if (!gUserSessionManager->getUserExtendedDataId(name, componentId, dataId))
        return false;

    return getDataValue(extendedDataMap, componentId, dataId, value);
}

bool UserSession::getDataValue(const UserExtendedDataMap& extendedDataMap, UserExtendedDataKey& key, UserExtendedDataValue &value)
{
    return getDataValue(extendedDataMap, COMPONENT_ID_FROM_UED_KEY(key), DATA_ID_FROM_UED_KEY(key), value);
}

bool UserSession::getDataValue(const UserExtendedDataMap& extendedDataMap, uint16_t componentId, uint16_t key, UserExtendedDataValue &value)
{
    bool isSuccessful = false;
    UserExtendedDataKey dataKey = UED_KEY_FROM_IDS(componentId, key);
    UserExtendedDataMap::const_iterator itr = extendedDataMap.find(dataKey);
    if (itr != extendedDataMap.end())
    {
        value = itr->second;
        isSuccessful = true;
    }

    return isSuccessful;
}

void UserSession::setCurrentUserSessionId(UserSessionId userSessionId)
{
    msCurrentUserSessionId = userSessionId;
    gCurrentUserSession = gUserSessionManager->getSession(userSessionId).get();

    if (gUserSessionMaster != nullptr)
        gCurrentLocalUserSession = gUserSessionMaster->getLocalSession(userSessionId).get();
}

UserSessionId UserSession::getCurrentUserSessionId()
{
    return msCurrentUserSessionId;
}

BlazeId UserSession::getCurrentUserBlazeId()
{
    return gUserSessionManager->getBlazeId(msCurrentUserSessionId);
}

//void UserSession::setFromCallingFiber()
//{
//    gCurrentUserSession.copyFromCallingFiber();
//    msSuperUserPrivilegeCounter.copyFromCallingFiber();
//    msCurrentUserSessionId.copyFromCallingFiber();
//}

void UserSession::pushSuperUserPrivilege()
{
    msSuperUserPrivilegeCounter = msSuperUserPrivilegeCounter + 1;
}

void UserSession::popSuperUserPrivilege()
{
    if (msSuperUserPrivilegeCounter > 0)
    {
        msSuperUserPrivilegeCounter = msSuperUserPrivilegeCounter - 1;
    }
}

bool UserSession::isCurrentContextAuthorized(const Authorization::Permission& permission, bool suppressWarnLog)
{
    if (hasSuperUserPrivilege())
    {
        return true;
    }
    else
    {
        if (gUserSessionManager != nullptr)
        {
            return gUserSessionManager->isSessionAuthorized(UserSession::getCurrentUserSessionId(), permission, suppressWarnLog);
        }
        return  false;
    }
}

void UserSession::updateBlazeObjectId(UserSessionData& session, const Blaze::UpdateBlazeObjectIdInfo &info)
{
    //  Update ExtendedData for the requested user session.
    //  If add request - adds the EA::TDF::ObjectId to the session's extended data (nop if EA::TDF::ObjectId is invalid.)
    //  If remove request - removes the EA::TDF::ObjectId from the extended data (nop if EA::TDF::ObjectId not in the extended data list.)
    ObjectIdList &blazeObjectIdList = session.getExtendedData().getBlazeObjectIdList();
    const EA::TDF::ObjectId bobjId = info.getBlazeObjectId();

    if (info.getAdd())
    {
        if (bobjId != EA::TDF::OBJECT_ID_INVALID)
        {
            ObjectIdList::iterator it = eastl::find(blazeObjectIdList.begin(), blazeObjectIdList.end(), bobjId);
            if (it == blazeObjectIdList.end())
            {
                // insert only non-dupes
                blazeObjectIdList.push_back(bobjId);
            }
        }
    }
    else
    {
        ObjectIdList::reverse_iterator rit = blazeObjectIdList.rbegin();
        while (rit != blazeObjectIdList.rend())
        {
            const EA::TDF::ObjectId curBlazeObjectId = *rit;
            if (curBlazeObjectId.type == bobjId.type && (curBlazeObjectId.id == bobjId.id || bobjId.id == 0))
            {
                rit = blazeObjectIdList.erase(rit);
            }
            else
            {
                ++rit;
            }
        }
    }
}

/*** Private Methods ******************************************************************************/

void UserSession::updateUsers(UserSessionId targetId, const BlazeIdList& blazeIds, SubscriptionAction action)
{
    UserSessionIdList targetIds;
    targetIds.push_back(targetId);
    updateUsers(targetIds, blazeIds, action);
}

void UserSession::updateUsers(const UserSessionIdList& targetIds, BlazeId blazeId, SubscriptionAction action)
{
    BlazeIdList blazeIds;
    blazeIds.push_back(blazeId);
    updateUsers(targetIds, blazeIds, action);
}

void UserSession::updateUsers(const UserSessionIdList& targetIds, const BlazeIdList& blazeIds, SubscriptionAction action)
{
    NotifyUpdateUserSubscriptions update;
    update.setAction(action);

    // It's safe to const cast this, because notification never changes it.
    update.setBlazeIds(const_cast<BlazeIdList&>(blazeIds));

    for (UserSessionIdList::const_iterator it = targetIds.begin(), end = targetIds.end(); it != end; ++it)
    {
        if (*it == INVALID_SESSION_ID)
            continue;

        update.setTargetUserSessionId(*it);
        gUserSessionManager->sendUserSubscriptionsUpdatedToSliver(UserSession::makeSliverIdentity(*it), &update);
    }
}


void UserSession::updateNotifiers(UserSessionId targetId, const UserSessionIdList& notifierIds, SubscriptionAction action)
{
    UserSessionIdList targetIds;
    targetIds.push_back(targetId);
    updateNotifiers(targetIds, notifierIds, action);
}

void UserSession::updateNotifiers(const UserSessionIdList& targetIds, UserSessionId notifierId, SubscriptionAction action)
{
    UserSessionIdList notifierIds;
    notifierIds.push_back(notifierId);
    updateNotifiers(targetIds, notifierIds, action);
}

void UserSession::updateNotifiers(const UserSessionIdList& targetIds, const UserSessionIdList& notifierIds, SubscriptionAction action)
{
    NotifyUpdateSessionSubscriptions update;
    update.setAction(action);

    // It's safe to const cast this, because notification never changes it.
    update.setNotifierIds(const_cast<UserSessionIdList&>(notifierIds));

    for (UserSessionIdList::const_iterator it = targetIds.begin(), end = targetIds.end(); it != end; ++it)
    {
        if (*it == INVALID_SESSION_ID)
            continue;

        update.setTargetUserSessionId(*it);
        gUserSessionManager->sendSessionSubscriptionsUpdatedToSliver(UserSession::makeSliverIdentity(*it), &update);
    }
}


void UserSession::mutualSubscribe(UserSessionId userSessionId, const UserSessionIdList& userSessionIds, SubscriptionAction action)
{
    BlazeId blazeId = gUserSessionManager->getBlazeId(userSessionId);

    BlazeIdList blazeIds;
    blazeIds.reserve(userSessionIds.size());
    for (UserSessionIdList::const_iterator it = userSessionIds.begin(), end = userSessionIds.end(); it != end; ++it)
    {
        blazeIds.push_back(gUserSessionManager->getBlazeId(*it));
    }

    switch (action)
    {
    case ACTION_ADD:
        // First, add the users and then the sessions identified by the 'userSessionIds' arg to the session identified by the 'userSessionId' arg.
        updateUsers(userSessionId, blazeIds, action);
        updateNotifiers(userSessionId, userSessionIds, action);

        // Then, do the inverse.  Add the user and then the session identified by the 'userSessionId' arg to the sessions identified by the 'userSessionIds' arg.
        updateUsers(userSessionIds, blazeId, action);
        updateNotifiers(userSessionIds, userSessionId, action);
        break;
    case ACTION_REMOVE:
        // First, remove the sessions and the users identified by the 'userSessionIds' arg from the session identified by the 'userSessionId' arg.
        updateNotifiers(userSessionId, userSessionIds, action);
        updateUsers(userSessionId, blazeIds, action);

        // Then, do the inverse.  Remove the session and then the user identified by the 'userSessionId' arg from the sessions identified by the 'userSessionIds' arg.
        updateNotifiers(userSessionIds, userSessionId, action);
        updateUsers(userSessionIds, blazeId, action);
        break;
    }
}

} //namespace Blaze
