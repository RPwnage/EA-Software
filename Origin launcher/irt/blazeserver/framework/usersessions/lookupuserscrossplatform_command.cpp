/*************************************************************************************************/
/*!
    \file   lookupuserscrossplatform_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersCrossPlatformCommand

    Resolve user lookup requests from client.

    \note

    Responses include both user identification and extended user data.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/accountinfodb.h"
#include "framework/rpc/usersessionsslave/lookupuserscrossplatform_stub.h"

namespace Blaze
{

class LookupUsersCrossPlatformCommand : public LookupUsersCrossPlatformCommandStub
{
    NON_COPYABLE(LookupUsersCrossPlatformCommand);
public:
    LookupUsersCrossPlatformCommand(Message* message, Blaze::LookupUsersCrossPlatformRequest* request, UserSessionsSlave* componentImpl)
        : LookupUsersCrossPlatformCommandStub(message, request),
        mComponent(componentImpl),
        mUnrestrictedPlatformSet(nullptr)
    {
    }

    ~LookupUsersCrossPlatformCommand() override
    {
    }

private:
    typedef eastl::hash_map<eastl::string, ClientPlatformType, CaseInsensitiveStringHash> PlatformByOriginPersonaNameMap;
    typedef eastl::map<ClientPlatformType, AccountIdVector> AccountIdsByPlatformMap;
    typedef eastl::map<ClientPlatformType, OriginPersonaIdVector> OriginPersonaIdsByPlatformMap;

    BlazeRpcError lookupUsersByAccountId(UserInfoPtrList& results, PlatformInfoByAccountIdMap& cachedPlatformInfo, const AccountIdsByPlatformMap& remainingAccountIds)
    {
        BlazeRpcError retError = ERR_OK;
        for (const auto& itr : remainingAccountIds)
        {
            BlazeRpcError err = ERR_OK;
            if (mRequest.getLookupOpts().getMostRecentPlatformOnly()) // when MostRecentPlatformOnly is true, use the PlatformInfo we looked up earlier instead of the UserSessionManager cache
                err = UserInfoDbCalls::lookupUsersByAccountIds(itr.second, itr.first, results, &cachedPlatformInfo);
            else
                err = gUserSessionManager->lookupUserInfoByAccountIds(itr.second, itr.first, results);
            if (retError == ERR_OK)
                retError = err;
        }
        return retError == USER_ERR_USER_NOT_FOUND ? ERR_OK : retError;
    }

    BlazeRpcError lookupUsersByOriginId(UserInfoPtrList& results, const OriginPersonaIdsByPlatformMap& remainingOriginPersonaIds)
    {
        BlazeRpcError retError = ERR_OK;
        for (const auto& itr : remainingOriginPersonaIds)
        {
            BlazeRpcError err = gUserSessionManager->lookupUserInfoByOriginPersonaIds(itr.second, itr.first, results);
            if (retError == ERR_OK)
                retError = err;
        }
        return retError == USER_ERR_USER_NOT_FOUND ? ERR_OK : retError;
    }

    BlazeRpcError lookupUsersBy1stPartyId(UserInfoPtrList& results, const PlatformInfoVector& remainingPlatformInfo)
    {
        BlazeRpcError retError = gUserSessionManager->lookupUserInfoByPlatformInfoVector(remainingPlatformInfo, results);
        return retError == USER_ERR_USER_NOT_FOUND ? ERR_OK : retError;
    }

    BlazeRpcError lookupUsersByOriginName(UserInfoPtrList& results, const PersonaNameVector& remainingOriginNames, const PlatformByOriginPersonaNameMap& platformByOriginName)
    {
        UserInfoListByPersonaNameMap lookupByOriginNameResults;
        BlazeRpcError retError = gUserSessionManager->lookupUserInfoByOriginPersonaNames(remainingOriginNames, *mUnrestrictedPlatformSet, lookupByOriginNameResults);
        if (retError != ERR_OK)
            return retError == USER_ERR_USER_NOT_FOUND ? ERR_OK : retError;

        for (auto& personaIt : lookupByOriginNameResults)
        {
            if (platformByOriginName.empty())
            {
                // ClientPlatformOnly was false; the result set should include every user on the allowed platforms
                for (auto& userIt : personaIt.second)
                {
                    if (mUnrestrictedPlatformSet->find(userIt->getPlatformInfo().getClientPlatform()) != mUnrestrictedPlatformSet->end())
                        results.push_back(userIt);
                }
            }
            else
            {
                // ClientPlatformOnly was true; the result set should only include users on the platforms specified by the PlatformInfoList entries corresponding to their Origin name
                PlatformByOriginPersonaNameMap::const_iterator platIt = platformByOriginName.find(personaIt.first);
                if (platIt == platformByOriginName.end())
                    continue;

                for (auto& userIt : personaIt.second)
                {
                    if (userIt->getPlatformInfo().getClientPlatform() == platIt->second)
                    {
                        results.push_back(userIt);
                        break;
                    }
                }
            }
        }

        return ERR_OK;
    }

    BlazeRpcError lookupUsers(CrossPlatformLookupType lookupType, const PlatformInfoList& platformInfoList, UserInfoPtrList& results)
    {
        PlatformByOriginPersonaNameMap platformByOriginPersonaName;
        PlatformInfoByAccountIdMap cachedPlatformInfo;
        PersonaNameVector remainingOriginNames;
        AccountIdsByPlatformMap remainingAccountIds;
        OriginPersonaIdsByPlatformMap remainingOriginPersonaIds;
        PlatformInfoVector remainingPlatformInfo;

        bool clientPlatformOnly = mRequest.getLookupOpts().getClientPlatformOnly() || mRequest.getLookupType() == CrossPlatformLookupType::FIRST_PARTY_ID;
        bool requestedPlatformOnly = clientPlatformOnly || mRequest.getLookupOpts().getMostRecentPlatformOnly();
        for (const auto& platformInfo : platformInfoList)
        {
            ClientPlatformType clientPlatform = platformInfo->getClientPlatform();
            if (mRequest.getLookupOpts().getMostRecentPlatformOnly() && !clientPlatformOnly)
            {
                // Make sure the most recent platform is a platform the caller is allowed to interact with
                // (if clientPlatformOnly is true, we've already done this check so we skip it here) 
                if (mUnrestrictedPlatformSet->find(clientPlatform) == mUnrestrictedPlatformSet->end())
                    continue;
            }

            switch (lookupType)
            {
            case CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID:
            {
                if (mRequest.getLookupOpts().getMostRecentPlatformOnly())
                    cachedPlatformInfo[platformInfo->getEaIds().getNucleusAccountId()] = *platformInfo;

                if (requestedPlatformOnly)
                    remainingAccountIds[clientPlatform].push_back(platformInfo->getEaIds().getNucleusAccountId());
                else
                {
                    for (const auto& platform : *mUnrestrictedPlatformSet)
                        remainingAccountIds[platform].push_back(platformInfo->getEaIds().getNucleusAccountId());
                }
            }
            break;
            case CrossPlatformLookupType::ORIGIN_PERSONA_ID:
            {
                if (requestedPlatformOnly)
                    remainingOriginPersonaIds[clientPlatform].push_back(platformInfo->getEaIds().getOriginPersonaId());
                else
                {
                    for (const auto& platform : *mUnrestrictedPlatformSet)
                        remainingOriginPersonaIds[platform].push_back(platformInfo->getEaIds().getOriginPersonaId());
                }
            }
            break;
            case CrossPlatformLookupType::FIRST_PARTY_ID:
            {
                remainingPlatformInfo.push_back(*platformInfo);
            }
            break;
            case CrossPlatformLookupType::ORIGIN_PERSONA_NAME:
            {
                remainingOriginNames.push_back(platformInfo->getEaIds().getOriginPersonaName());
                if (mRequest.getLookupOpts().getClientPlatformOnly())
                    platformByOriginPersonaName[platformInfo->getEaIds().getOriginPersonaName()] = clientPlatform;
            }
            break;
            default:
                return ERR_SYSTEM;
            }
        }

        if (!remainingAccountIds.empty())
            return lookupUsersByAccountId(results, cachedPlatformInfo, remainingAccountIds);

        if (!remainingOriginPersonaIds.empty())
            return lookupUsersByOriginId(results, remainingOriginPersonaIds);

        if (!remainingPlatformInfo.empty())
            return lookupUsersBy1stPartyId(results, remainingPlatformInfo);

        if (!remainingOriginNames.empty())
            return lookupUsersByOriginName(results, remainingOriginNames, platformByOriginPersonaName);

        return ERR_OK;
    }

    LookupUsersCrossPlatformCommandStub::Errors execute() override
    {
        eastl::string callerNamespace;
        bool crossPlatformOptIn;
        BlazeRpcError retError = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(mUnrestrictedPlatformSet, callerNamespace, crossPlatformOptIn);
        if (retError != ERR_OK)
            return commandErrorFromBlazeError(retError);

        if (mRequest.getPlatformInfoList().empty())
        {
            BLAZE_ERR_LOG(Log::USER, "[LookupUsersCrossPlatform] PlatformInfoList is empty!");
            return LookupUsersCrossPlatformCommandStub::ERR_SYSTEM;
        }

        // A non-empty callerNamespace means that the caller isn't on the 'common' platform, and is therefore only permitted to search by 1st-party id on his own platform
        if (!callerNamespace.empty() && mRequest.getLookupType() == CrossPlatformLookupType::FIRST_PARTY_ID)
            mUnrestrictedPlatformSet = &gUserSessionManager->getNonCrossplayPlatformSet(gCurrentUserSession->getClientPlatform());

        // Make sure the caller isn't explicitly searching for users on platforms he isn't allowed to interact with
        if (mRequest.getLookupType() == CrossPlatformLookupType::FIRST_PARTY_ID || mRequest.getLookupOpts().getClientPlatformOnly())
        {
            ClientPlatformType callerPlatform = gCurrentUserSession->getClientPlatform();
            const ClientPlatformSet& unrestrictedPlatformSet = gUserSessionManager->getUnrestrictedPlatforms(callerPlatform);
            for (const auto& platformInfo : mRequest.getPlatformInfoList())
            {
                if (platformInfo->getClientPlatform() != callerPlatform)
                {
                    if (!gController->isPlatformHosted(platformInfo->getClientPlatform()))
                    {
                        eastl::string platformInfoStr;
                        BLAZE_ERR_LOG(Log::USER, "[LookupUsersCrossPlatform] Unable to look up user with platformInfo(" << platformInfoToString(*platformInfo, platformInfoStr) <<
                            "): requested platform is not hosted.");
                        return LookupUsersCrossPlatformCommandStub::USER_ERR_DISALLOWED_PLATFORM;
                    }
                    if (callerNamespace.empty())
                        continue;

                    if (mRequest.getLookupType() == CrossPlatformLookupType::FIRST_PARTY_ID ||
                        unrestrictedPlatformSet.find(platformInfo->getClientPlatform()) == unrestrictedPlatformSet.end())
                    {
                        BLAZE_ERR_LOG(Log::USER, "[LookupUsersCrossPlatform] User on platform '" << ClientPlatformTypeToString(callerPlatform) << "' is not permitted to look up users on platform '" << ClientPlatformTypeToString(platformInfo->getClientPlatform())
                            << (mRequest.getLookupType() == CrossPlatformLookupType::FIRST_PARTY_ID ? "' by 1st-party id" : "'"));
                        return LookupUsersCrossPlatformCommandStub::USER_ERR_DISALLOWED_PLATFORM;
                    }
                    if (!crossPlatformOptIn)
                        return LookupUsersCrossPlatformCommandStub::USER_ERR_CROSS_PLATFORM_OPTOUT;
                }
            }
        }

        UserInfoPtrList results;
        if (mRequest.getLookupOpts().getMostRecentPlatformOnly())
        {
            // When MostRecentPlatformOnly is set, we need to query the accountinfo table for each entry in the requested PlatformInfoList,
            // in order to determine which platform the most recent login was on.
            PlatformInfoList localPlatformInfoList;
            retError = AccountInfoDbCalls::getPlatformInfoForMostRecentLogin(mRequest.getLookupType(), mRequest.getPlatformInfoList(), localPlatformInfoList, mRequest.getLookupOpts().getClientPlatformOnly());
            if (retError == ERR_OK)
            {
                // Now that we have the PlatformInfoList of the users we actually want to look up, search for them by account id
                retError = lookupUsers(NUCLEUS_ACCOUNT_ID, localPlatformInfoList, results);
            }
        }
        else
        {
            retError = lookupUsers(mRequest.getLookupType(), mRequest.getPlatformInfoList(), results);
        }

        for (const auto& userItr : results)
        {
            if (mRequest.getLookupOpts().getOnlineOnly() && !gUserSessionManager->isUserOnline(userItr->getId()))
                continue;

            // fill out the user data
            BlazeRpcError err = gUserSessionManager->filloutUserData(*userItr, *mResponse.getUserDataList().pull_back(),
                (userItr->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true), gCurrentUserSession->getClientPlatform());
            if (err != Blaze::ERR_OK)
            {
                retError = err;
                break;
            }
        }

        if (retError == ERR_OK && mResponse.getUserDataList().empty())
            return LookupUsersCrossPlatformCommandStub::USER_ERR_USER_NOT_FOUND;

        return commandErrorFromBlazeError(retError);
    }

private:
    UserSessionsSlave* mComponent; // memory owned by creator, don't free
    const ClientPlatformSet* mUnrestrictedPlatformSet;
};

// static factory method impl
DEFINE_LOOKUPUSERSCROSSPLATFORM_CREATE_COMPNAME(UserSessionManager)

} // Blaze
