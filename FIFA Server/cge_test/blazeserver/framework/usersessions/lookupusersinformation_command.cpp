/*************************************************************************************************/
/*!
    \file   lookupusersinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersCommand

    Resolve user lookup requests from client.

    \note

    Responses include both user identification and extended user data.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusers_stub.h"
#include "framework/rpc/usersessionsslave/lookupusersidentification_stub.h"

namespace Blaze
{

class LookupUsersBase
{
    NON_COPYABLE(LookupUsersBase);
public:
    LookupUsersBase(LookupUsersRequest& request, UserSessionsSlave* componentImpl)
        : mRequestBase(request), mComponent(componentImpl)
    {
    }

    virtual ~LookupUsersBase()
    {
    }

protected:
    virtual void reserveResponseListSize(size_t count) = 0;
    virtual BlazeRpcError filloutResponseUser(const UserInfo& userInfo, bool includeNetworkAddress) = 0;

    bool setPlatformInfoForLookup(UserIdentification& userIdentification)
    {
        ClientPlatformType callerPlatform = gCurrentUserSession->getClientPlatform();
        if (userIdentification.getPlatformInfo().getClientPlatform() != INVALID && userIdentification.getPlatformInfo().getClientPlatform() != NATIVE && userIdentification.getPlatformInfo().getClientPlatform() != callerPlatform)
        {
            eastl::string platformInfoStr;
            BLAZE_WARN_LOG(Log::USER, "[LookupUsersBase]: Skipping lookup by " << LookupUsersRequest::LookupTypeToString(mRequestBase.getLookupType()) << " for user with platformInfo(" << platformInfoToString(userIdentification.getPlatformInfo(), platformInfoStr) << "): requested platform (" << 
                ClientPlatformTypeToString(userIdentification.getPlatformInfo().getClientPlatform()) << ") does not match caller's platform (" << callerPlatform <<
                "). To look up users by PlatformInfo, NucleusAccountId or OriginPersonaId on a multi-platform server, use the lookupUsersCrossPlatform RPC instead.");
            return false;
        }
        else
        {
            // Backwards compatibility: set PlatformInfo from deprecated UserIdentification fields
            // (note that this will not overwrite the PlatformInfo with unset/invalid entries)
            convertToPlatformInfo(userIdentification.getPlatformInfo(), userIdentification.getExternalId(), nullptr, userIdentification.getAccountId(), userIdentification.getOriginPersonaId(), nullptr, callerPlatform);
            return true;
        }
    }

    BlazeRpcError executeBase()
    {
        BlazeRpcError err = ERR_OK;
        ClientPlatformType callerPlatform = gCurrentUserSession->getClientPlatform();
        if (callerPlatform == common && mRequestBase.getLookupType() != LookupUsersRequest::BLAZE_ID && mRequestBase.getLookupType() != LookupUsersRequest::PERSONA_NAME)
        {
            BLAZE_ERR_LOG(Log::USER, "[LookupUsersBase] For callers on platform 'common', lookupUsers and lookupUsersIdentification are only supported for lookups by BlazeId or persona name and primary namespace. To look up users by NucleusAccountId, "
                "OriginPersonaId, Origin persona name, or 1st party id, use lookupUsersCrossPlatform.");
            return ERR_SYSTEM;
        }

        switch (mRequestBase.getLookupType())
        {
        case LookupUsersRequest::BLAZE_ID:
        {
            eastl::string callerNamespace;
            bool crossPlatformOptIn;
            const ClientPlatformSet* platformSet = nullptr;
            err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, callerNamespace, crossPlatformOptIn);
            if (err != ERR_OK)
                break;

            BlazeIdVector blazeIdLookups(BlazeStlAllocator("LookupUsersIds", UserSessionsSlave::COMPONENT_MEMORY_GROUP));
            BlazeIdToUserInfoMap resultMap;

            const UserIdentificationList& userInfoList = mRequestBase.getUserIdentificationList();
            blazeIdLookups.reserve(userInfoList.size());
            for (UserIdentificationList::const_iterator i = userInfoList.begin(), e = userInfoList.end(); i != e; ++i)
            {
                blazeIdLookups.push_back((*i)->getBlazeId());
            }

            err = gUserSessionManager->lookupUserInfoByBlazeIds(blazeIdLookups, resultMap);
            if (err == Blaze::ERR_OK)
            {
                reserveResponseListSize(resultMap.size());
                for (BlazeIdToUserInfoMap::const_iterator i = resultMap.begin(), e = resultMap.end(); i != e; ++i)
                {
                    if (callerPlatform != common)
                    {
                        if (platformSet->find(i->second->getPlatformInfo().getClientPlatform()) == platformSet->end())
                        {
                            BLAZE_ERR_LOG(Log::USER, "[LookupUsersBase] Cannot return result for user with BlazeId(" << i->second->getId() << "): caller on platform '" << ClientPlatformTypeToString(callerPlatform) << "' with cross-platform opt-in '" << (crossPlatformOptIn ? "true" : "false")
                                << "' is not permitted to look up users on platform '" << ClientPlatformTypeToString(i->second->getPlatformInfo().getClientPlatform()) << "'");
                            const ClientPlatformSet& unrestrictedPlatformSet = gUserSessionManager->getUnrestrictedPlatforms(callerPlatform);
                            if (unrestrictedPlatformSet.find(i->second->getPlatformInfo().getClientPlatform()) == unrestrictedPlatformSet.end())
                                return USER_ERR_DISALLOWED_PLATFORM;
                            return USER_ERR_CROSS_PLATFORM_OPTOUT;
                        }
                    }
                    err = filloutResponseUser(*i->second.get(),
                        (i->second.get()->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true));
                    if (err != Blaze::ERR_OK)
                        break;
                }
            }
            break;
        }
        case LookupUsersRequest::PERSONA_NAME:
        {
            eastl::string callerNamespace;
            bool crossPlatformOptIn;
            const ClientPlatformSet* platformSet = nullptr;
            err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, callerNamespace, crossPlatformOptIn);
            if (err != ERR_OK)
                break;

            if (callerPlatform != common && callerNamespace.empty())
            {
                BLAZE_ERR_LOG(Log::USER, "[LookupUsersBase] Unable to look up users by persona name - could not determine caller's namespace.");
                err = Blaze::ERR_SYSTEM;
                break;
            }

            typedef eastl::hash_map<eastl::string, PersonaNameVector> PersonaNamesByNamespaceMap;
            PersonaNamesByNamespaceMap personaNameLookups;

            const UserIdentificationList &userInfoList = mRequestBase.getUserIdentificationList();
            for (UserIdentificationList::const_iterator i = userInfoList.begin(), e = userInfoList.end(); i != e; ++i)
            {
                const char8_t* personaNamespace = (*i)->getPersonaNamespace();
                if (personaNamespace[0] == '\0')
                {
                    if (callerNamespace.empty())
                    {
                        BLAZE_WARN_LOG(Log::USER, "[LookupUsersBase] Skipping lookup for persona name '" << (*i)->getName() << "': caller is on platform 'common' and did not specify a namespace to search.");
                        continue;
                    }
                    personaNamespace = callerNamespace.c_str();
                }
                else if (!callerNamespace.empty() && blaze_strcmp((*i)->getPersonaNamespace(), callerNamespace.c_str()) != 0)
                {
                    // Note: on single-platform deployments, all users have the same persona namespace -- so we still enforce this check to avoid a lookup that will just return USER_NOT_FOUND
                    BLAZE_WARN_LOG(Log::USER, "[LookupUsersBase] Skipping lookup for persona name '" << (*i)->getName() << "': user's namespace (" << (*i)->getPersonaNamespace() <<
                        ") does not match caller's namespace (" << callerNamespace.c_str() << ")");
                    continue;
                }
                personaNameLookups[personaNamespace].push_back((*i)->getName());
            }

            UserInfoPtrList resultList;
            resultList.reserve(userInfoList.size());
            for (auto& it : personaNameLookups)
            {
                ClientPlatformSet platforms;
                if (callerPlatform == common)
                {
                    for (const auto& platform : gController->getHostedPlatforms())
                    {
                        if (blaze_strcmp(it.first.c_str(), gController->getNamespaceFromPlatform(platform)) == 0)
                            platforms.insert(platform);
                    }
                }
                else
                {
                    platforms.insert(callerPlatform);
                }
                for (const auto& platform : platforms)
                {
                    PersonaNameToUserInfoMap resultMap;
                    BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByPersonaNames(it.second, platform, resultMap, it.first.c_str());
                    if (lookupErr == ERR_OK)
                    {
                        for (auto& result : resultMap)
                            resultList.push_back(result.second);
                    }
                    else if (lookupErr != USER_ERR_USER_NOT_FOUND)
                    {
                        err = lookupErr;
                        break;
                    }
                }
                if (err != ERR_OK)
                    break;
            }

            if (err == Blaze::ERR_OK)
            {
                reserveResponseListSize(resultList.size());
                for (const auto& it : resultList)
                {
                    err = filloutResponseUser(*it,
                        (it->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true));
                    if (err != Blaze::ERR_OK)
                        break;
                }
            }
            break;
        }
        case LookupUsersRequest::EXTERNAL_ID:
        {
            PlatformInfoVector platformInfoLookups;
            UserInfoPtrList resultList;

            UserIdentificationList &userInfoList = mRequestBase.getUserIdentificationList();
            platformInfoLookups.reserve(userInfoList.size());
            for (auto userInfoPtr : userInfoList)
            {
                if (setPlatformInfoForLookup(*userInfoPtr))
                    platformInfoLookups.push_back(userInfoPtr->getPlatformInfo());
            }

            err = gUserSessionManager->lookupUserInfoByPlatformInfoVector(platformInfoLookups, resultList);
            if (err == Blaze::ERR_OK)
            {
                reserveResponseListSize(resultList.size());
                for (UserInfoPtr i : resultList)
                {
                    err = filloutResponseUser(*i.get(),
                        (i.get()->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true));
                    if (err != Blaze::ERR_OK)
                        break;
                }
            }
            break;
        }
        case LookupUsersRequest::ACCOUNT_ID:
        {
            UserIdentificationList& userInfoList = mRequestBase.getUserIdentificationList();

            AccountIdVector accountIdLookups;
            accountIdLookups.reserve(userInfoList.size());

            UserInfoPtrList results;
            for (UserIdentificationList::iterator i = userInfoList.begin(); i != userInfoList.end(); ++i)
            {
                if (setPlatformInfoForLookup(*(*i)))
                    accountIdLookups.push_back((*i)->getPlatformInfo().getEaIds().getNucleusAccountId());
            }

            err = gUserSessionManager->lookupUserInfoByAccountIds(accountIdLookups, callerPlatform, results);

            reserveResponseListSize(results.size());
            for (UserInfoPtrList::iterator i = results.begin(), e = results.end(); i != e; ++i)
            {
                UserInfoPtr& userInfo = *i;

                err = filloutResponseUser(*userInfo,
                    (userInfo->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true));

                if (err != Blaze::ERR_OK)
                    break;
            }
            break;
        }
        case LookupUsersRequest::ORIGIN_PERSONA_ID:
        {
            UserIdentificationList& userInfoList = mRequestBase.getUserIdentificationList();
            OriginPersonaIdVector originPersonaIdLookups;
            originPersonaIdLookups.reserve(userInfoList.size());

            UserInfoPtrList results;
            for (UserIdentificationList::iterator i = userInfoList.begin(); i != userInfoList.end(); ++i)
            {
                if (setPlatformInfoForLookup(*(*i)))
                    originPersonaIdLookups.push_back((*i)->getPlatformInfo().getEaIds().getOriginPersonaId());
            }

            err = gUserSessionManager->lookupUserInfoByOriginPersonaIds(originPersonaIdLookups, callerPlatform, results);

            reserveResponseListSize(results.size());
            for (UserInfoPtrList::iterator i = results.begin(), e = results.end(); i != e; ++i)
            {
                UserInfoPtr& userInfo = *i;

                err = filloutResponseUser(*userInfo,
                    (userInfo->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true));

                if (err != Blaze::ERR_OK)
                    break;
            }
            break;
        }
        default:
            BLAZE_ERR_LOG(Log::USER,  "[LookupUsersBase] Unsupported lookup type, BlazeId|PersonaName|ExternalId|AccountId|OriginPersonaId required.");
            err = Blaze::ERR_SYSTEM;
        }
        return err;
    }

private:
    LookupUsersRequest& mRequestBase;
    UserSessionsSlave* mComponent; // memory owned by creator, don't free
};

class LookupUsersCommand : public LookupUsersCommandStub, LookupUsersBase
{
    NON_COPYABLE(LookupUsersCommand);
public:
    LookupUsersCommand(Message* message, Blaze::LookupUsersRequest* request, UserSessionsSlave* componentImpl)
        : LookupUsersCommandStub(message, request),
        LookupUsersBase(*request, componentImpl),
        mComponent(componentImpl)
    {
    }

    ~LookupUsersCommand() override
    {
    }

protected:
    void reserveResponseListSize(size_t count) override
    {
        mResponse.getUserDataList().reserve(count);
    }

    BlazeRpcError filloutResponseUser(const UserInfo& userInfo, bool includeNetworkAddress) override
    {
        return gUserSessionManager->filloutUserData(userInfo, *mResponse.getUserDataList().pull_back(), includeNetworkAddress, gCurrentUserSession->getClientPlatform());
    }

    /* Private methods *******************************************************************************/
private:
    LookupUsersCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }

private:
    UserSessionsSlave* mComponent;  // memory owned by creator, don't free
};

// static factory method impl
DEFINE_LOOKUPUSERS_CREATE_COMPNAME(UserSessionManager)

/*************************************************************************************************/
/*!
    \class LookupUsersIdentificationCommand

    Resolve user lookup requests from client.

    \note

    Responses only include user identification, but no extended user data.
*/
/*************************************************************************************************/

class LookupUsersIdentificationCommand : public LookupUsersIdentificationCommandStub, LookupUsersBase
{
public:
    LookupUsersIdentificationCommand(Message* message, Blaze::LookupUsersRequest* request, UserSessionsSlave* componentImpl)
        : LookupUsersIdentificationCommandStub(message, request),
        LookupUsersBase(*request, componentImpl),
        mComponent(componentImpl)
    {
    }

    ~LookupUsersIdentificationCommand() override
    {
    }

protected:
    void reserveResponseListSize(size_t count) override
    {
        mResponse.getUserIdentificationList().reserve(count);
    }

    BlazeRpcError filloutResponseUser(const UserInfo& userInfo, bool includeNetworkAddress) override
    {
        UserIdentification& userId = *mResponse.getUserIdentificationList().pull_back();
        UserInfo::filloutUserIdentification(userInfo, userId);
        UserSessionManager::obfuscate1stPartyIds(gCurrentUserSession->getClientPlatform(), userId.getPlatformInfo());
        return Blaze::ERR_OK;
    }

    /* Private methods *******************************************************************************/
private:
    LookupUsersIdentificationCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }

private:
    UserSessionsSlave* mComponent;  // memory owned by creator, don't free
};

// static factory method impl
DEFINE_LOOKUPUSERSIDENTIFICATION_CREATE_COMPNAME(UserSessionManager)

} // Blaze
