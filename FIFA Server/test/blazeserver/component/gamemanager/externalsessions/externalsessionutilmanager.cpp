/*! ************************************************************************************************/
/*!
\file externalsessionumanager.cpp


\attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#include "component/gamemanager/externalsessions/externalsessionutilmanager.h"
#include "component/gamemanager/externalsessions/externalsessionutilxboxone.h"
#include "component/gamemanager/externalsessions/externalsessionutilps4.h"
#include "framework/controller/controller.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

ExternalSessionUtilManager::ExternalSessionUtilManager() 
{

}

ExternalSessionUtilManager::~ExternalSessionUtilManager()
{
    for (auto util : mUtilMap)
        delete util.second;

    mUtilMap.clear();
}

const ExternalSessionUtilMap& ExternalSessionUtilManager::getExternalSessionUtilMap() const 
{ 
    return mUtilMap; 
}

ExternalSessionUtil* ExternalSessionUtilManager::getUtil(ClientPlatformType platform) const
{
    ExternalSessionUtilMap::const_iterator iter = mUtilMap.find(platform);
    if (iter != mUtilMap.end())
        return iter->second;

    return nullptr;
}

    
bool ExternalSessionUtilManager::isMockPlatformEnabled(ClientPlatformType platform) const
{
    const ExternalSessionUtil* util = getUtil(platform);
    return util ? util->isMockPlatformEnabled() : false;
}

bool ExternalSessionUtilManager::onPrepareForReconfigure(const ExternalSessionServerConfigMap& config) const
{
    for (auto util : mUtilMap)
    {
        auto configIter = config.find(util.first);      // If an existing platform is missing a config, skip it:
        if (configIter == config.end())
        {
            TRACE_LOG("[ExternalSessionUtilManager].onPrepareForReconfigure: Missing config for existing platform " << ClientPlatformTypeToString(util.first) << ".  Using default config." );
            continue;
        }

        if (!util.second->onPrepareForReconfigure(*configIter->second))
        {
            ERR_LOG("[ExternalSessionUtilManager].onPrepareForReconfigure: Failed to reconfigure existing platform " << ClientPlatformTypeToString(util.first));
            return false;
        }
    }

    // Sanity check for new platforms:
    for (ClientPlatformType curPlatform : gController->getHostedPlatforms())
    {
        if (config.find(curPlatform) != config.end() && mUtilMap.find(curPlatform) == mUtilMap.end())
        {
            ERR_LOG("[ExternalSessionUtilManager].onPrepareForReconfigure: Somehow missing session util for platform " << ClientPlatformTypeToString(curPlatform) << " after reconfig.");
            return false;
        }
    }

    return true;
}

void ExternalSessionUtilManager::onReconfigure(const ExternalSessionServerConfigMap& config, const TimeValue reservationTimeout)
{
    for (auto util : mUtilMap)
    {
        auto configIter = config.find(util.first);      // If an existing platform is missing a config, skip the platform:
        if (configIter == config.end())
        {
            TRACE_LOG("[ExternalSessionUtilManager].onReconfigure: Missing config for existing platform " << ClientPlatformTypeToString(util.first) << ".  No changes will be made.");
            continue;
        }
        util.second->onReconfigure(*configIter->second, reservationTimeout);
    }
}

Blaze::BlazeRpcError ExternalSessionUtilManager::getAuthToken(const UserIdentification& ident, const char8_t* serviceName, eastl::string& buf) const
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if (!gController->isPlatformHosted(platform))
    {
        ERR_LOG("[ExternalSessionUtil].getAuthToken:  User (" << ident.getBlazeId() << ") attempting to getAuthToken with invalid ClientPlatform " << ClientPlatformTypeToString(platform) << ".");
        return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_OK;
    for (auto util : mUtilMap)
    {
        if (platform == util.first)
        {
            err = util.second->getAuthToken(ident, serviceName, buf);
            if (err != ERR_OK)
                return err; 
        }
    }

    return err;
}

Blaze::BlazeRpcError ExternalSessionUtilManager::join(const JoinExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionApiError& apiErr) const
{
    for (auto userInfo : params.getExternalUserJoinInfos())
    {
        ClientPlatformType userPlatform = userInfo->getUserIdentification().getPlatformInfo().getClientPlatform();
        if (!gController->isPlatformHosted(userPlatform))
        {
            ERR_LOG("[ExternalSessionUtil].join:  User (" << userInfo->getUserIdentification().getBlazeId() << ") attempting to join with invalid ClientPlatform " << ClientPlatformTypeToString(userPlatform) << ".");
            return ERR_SYSTEM;
        }
    }

    BlazeRpcError apiRpcErr = ERR_OK; // track overall rpc result across platforms
    int platformCount = 0; // track the number of platforms this call was attempted on. This may be different than the number of hosted platforms. 
   
    for (auto util : mUtilMap)
    {
        ClientPlatformType curPlatform = util.first;
        
        JoinExternalSessionParameters tempParams;
        params.copyInto(tempParams);

        // Iterate over all platforms, and join any players using that util- users are only ever added to their current platform's external session.
        for (auto userInfo = tempParams.getExternalUserJoinInfos().begin();  userInfo != tempParams.getExternalUserJoinInfos().end(); )
        {
            if (curPlatform != userInfo->get()->getUserIdentification().getPlatformInfo().getClientPlatform())
                userInfo = tempParams.getExternalUserJoinInfos().erase(userInfo);
            else
                ++userInfo;
        }

        if (!tempParams.getExternalUserJoinInfos().empty())
        {
            ++platformCount;

            BlazeRpcError err = ERR_OK;
            // If we have not already failed to init the join, try to join.
            if (joinInitErr.getPlatformRpcErrorMap().find(curPlatform) == joinInitErr.getPlatformRpcErrorMap().end())
            {
                err = util.second->join(tempParams, result, commit);
            }
            else // Our join init had failed, take our earlier error code and mark these players failed.
            {
                err = joinInitErr.getPlatformRpcErrorMap().find(curPlatform)->second;
            }
            
            if (err != ERR_OK)
            {
                apiRpcErr = ERR_SYSTEM; // We fail the call overall to allow the easy call site code which just needs to look into the return error before diving deeper in the error path.

                ExternalSessionPlatformError* platErr = apiErr.getPlatformErrorMap().allocate_element();
                platErr->setBlazeRpcErr(err);

                for (const auto& userInfo : tempParams.getExternalUserJoinInfos())
                    userInfo->getUserIdentification().copyInto(*(platErr->getUsers().pull_back()));
                
                apiErr.getPlatformErrorMap()[curPlatform] = platErr;
            }
        }
    }

    if (apiRpcErr != ERR_OK)
    {
        apiErr.setFailedOnAllPlatforms(platformCount == apiErr.getPlatformErrorMap().size()); // This check works fine on both single platform and shared cluster.
        if (apiErr.getPlatformErrorMap().size() == 1)
            apiErr.setSinglePlatformBlazeRpcErr(apiErr.getPlatformErrorMap().begin()->second->getBlazeRpcErr());
    }

    return apiRpcErr;
}

Blaze::BlazeRpcError ExternalSessionUtilManager::leave(const LeaveGroupExternalSessionParameters& params, ExternalSessionApiError& apiErr) const
{
    for (auto userInfo : params.getExternalUserLeaveInfos())
    {
        ClientPlatformType userPlatform = userInfo->getUserIdentification().getPlatformInfo().getClientPlatform();
        if (!gController->isPlatformHosted(userPlatform))
        {
            ERR_LOG("[ExternalSessionUtil].leave:  User (" << userInfo->getUserIdentification().getBlazeId() << ") attempting to leave with invalid ClientPlatform " << ClientPlatformTypeToString(userPlatform) << ".");
            return ERR_SYSTEM;
        }
    }

    // Attempt leaves on all supported platforms:
    BlazeRpcError apiRpcErr = ERR_OK;
    int platformCount = 0;
    for (auto util : mUtilMap)
    {
        ClientPlatformType curPlatform = util.first;
        LeaveGroupExternalSessionParameters tempParams;
        params.copyInto(tempParams);

        // Iterate over all platforms, and join any players using that util.
        for (auto userInfo = tempParams.getExternalUserLeaveInfos().begin(); userInfo != tempParams.getExternalUserLeaveInfos().end(); )
        {
            if (curPlatform != userInfo->get()->getUserIdentification().getPlatformInfo().getClientPlatform())
                userInfo = tempParams.getExternalUserLeaveInfos().erase(userInfo);
            else
                ++userInfo;
        }

        if (!tempParams.getExternalUserLeaveInfos().empty())
        {
            ++platformCount;
            
            BlazeRpcError err = util.second->leave(tempParams);
            if (err != ERR_OK)
            {
                apiRpcErr = ERR_SYSTEM; 

                ExternalSessionPlatformError* platErr = apiErr.getPlatformErrorMap().allocate_element();
                platErr->setBlazeRpcErr(err);

                for (const auto& userInfo : tempParams.getExternalUserLeaveInfos())
                    userInfo->getUserIdentification().copyInto(*(platErr->getUsers().pull_back()));

                apiErr.getPlatformErrorMap()[curPlatform] = platErr;
            }
        }
    }

    if (apiRpcErr != ERR_OK)
    {
        apiErr.setFailedOnAllPlatforms(platformCount == apiErr.getPlatformErrorMap().size());
        if (apiErr.getPlatformErrorMap().size() == 1)
            apiErr.setSinglePlatformBlazeRpcErr(apiErr.getPlatformErrorMap().begin()->second->getBlazeRpcErr());
    }

    return apiRpcErr;
}

Blaze::BlazeRpcError ExternalSessionUtilManager::setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult) const
{
    ClientPlatformType platform = params.getUserIdentification().getPlatformInfo().getClientPlatform();
    ExternalSessionUtil* util = getUtil(platform);
    if (util == nullptr)
    {
        ERR_LOG("[ExternalSessionUtil].setPrimary:  User (" << params.getUserIdentification().getBlazeId() << ") attempting to setPrimary with invalid ClientPlatform (" << ClientPlatformTypeToString(platform) << ").");
        return ERR_SYSTEM;
    }

    return util->setPrimary(params, result, errorResult);
}

Blaze::BlazeRpcError ExternalSessionUtilManager::clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult) const
{
    ClientPlatformType platform = params.getUserIdentification().getPlatformInfo().getClientPlatform();
    ExternalSessionUtil* util = getUtil(platform);
    if (util == nullptr)
    {
        ERR_LOG("[ExternalSessionUtil].clearPrimary:  User (" << params.getUserIdentification().getBlazeId() << ") attempting to clearPrimary with invalid ClientPlatform (" << ClientPlatformTypeToString(platform) << ").");
        return ERR_SYSTEM;
    }

    return util->clearPrimary(params, errorResult);
}

Blaze::BlazeRpcError ExternalSessionUtilManager::updatePresenceForUser(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResult& result, UpdateExternalSessionPresenceForUserErrorResult& errorResult) const
{
    ClientPlatformType platform = params.getUserIdentification().getPlatformInfo().getClientPlatform();
    ExternalSessionUtil* util = getUtil(platform);
    if (util == nullptr)
    {
        ERR_LOG("[ExternalSessionUtil].updatePresenceForUser:  User (" << params.getUserIdentification().getBlazeId() << ") attempting to updatePrimary with invalid ClientPlatform (" << ClientPlatformTypeToString(platform) << ").");
        return ERR_SYSTEM;
    }

    return util->updatePresenceForUser(params, result, errorResult);
}

bool ExternalSessionUtilManager::hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) const
{
    ClientPlatformType platform = userSession.getPlatformInfo().getClientPlatform();
    ExternalSessionUtil* util = getUtil(platform);
    if (util == nullptr)
    {
        TRACE_LOG("[ExternalSessionUtilManager].hasUpdatePrimaryExternalSessionPermission:  User (" << userSession.getBlazeId() << ") attempting to check permission with unsupported ClientPlatform (" << ClientPlatformTypeToString(userSession.getPlatformInfo().getClientPlatform()) << "). Returning false.");
        return false;
    }

    return util->hasUpdatePrimaryExternalSessionPermission(userSession);
}

bool ExternalSessionUtilManager::doesCustomDataExceedMaxSize(const ClientPlatformTypeList& gamePlatforms, uint32_t count) const
{
    for (auto platform : gamePlatforms)
    {
        ExternalSessionUtilMap::const_iterator iter = mUtilMap.find(platform);
        if (iter != mUtilMap.end())
        {
            if (count > iter->second->getConfig().getExternalSessionCustomDataMaxSize())
            {
                // game client must ensure that the custom data set in the game creation request is within the limits of configured value for each platform (lowest common denominator) 
                ERR_LOG("[ExternalSessionUtil].doesCustomDataExceedMaxSize:  exceeding configured custom data size (" << iter->second->getConfig().getExternalSessionCustomDataMaxSize() <<
                    ") on platform(" << ClientPlatformTypeToString(platform) << ") which is potentially a platform for this game. Requested (" << count << "). Game creation will fail.");
                return true;
            }
        }
    }

    return false;
}

bool ExternalSessionUtilManager::isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context) const
{
    // Just check each plat and return true if anyone needs an update.
    for (auto util : mUtilMap)
    {
        if (util.second->isUpdateRequired(origValues, newValues, context))
        {
            TRACE_LOG("[ExternalSessionUtil].isUpdateRequired:  externalsession for game (" << origValues.getGameName() << ") needs an update on platform ("
                << ClientPlatformTypeToString(util.first) << ").");
            return true;
        }
    }

    return false;
}

Blaze::BlazeRpcError ExternalSessionUtilManager::getMembers(const ExternalSessionIdentification& sessionIdentification, ExternalIdsByPlatform& memberExternalIds, ExternalSessionApiError& apiErr) const
{
    BlazeRpcError apiRpcErr = ERR_OK;

    // For this api call, we don't actually know what platforms are involved (as there is no incoming user list). So we only stuff in the information about what happened.
    // (and that is all is needed for the caller).
    
    for (auto util : mUtilMap) 
    {
        ExternalIdList externalMembers;
        
        BlazeRpcError err = util.second->getMembers(sessionIdentification, externalMembers);
        if (err != ERR_OK)
        {
            apiRpcErr = ERR_SYSTEM;
            
            ExternalSessionPlatformError* platErr = apiErr.getPlatformErrorMap().allocate_element();
            platErr->setBlazeRpcErr(err);

            apiErr.getPlatformErrorMap()[util.first] = platErr;
        }
        else
        {
            if (!externalMembers.empty())
                externalMembers.copyInto(memberExternalIds[util.first]);
        }
    }
    
    return apiRpcErr;
}

void ExternalSessionUtilManager::isFullForExternalSessions(const GameSessionMaster& gameSession, uint16_t& maxUsers, PlatformBoolMap& outFullMap) const
{
    for (auto util : mUtilMap)
    {
        // Update the fullness for all platforms:
        ClientPlatformType curPlatform = util.first;
        outFullMap[curPlatform] = util.second->isFullForExternalSession(gameSession, maxUsers);
    }
}

Blaze::BlazeRpcError ExternalSessionUtilManager::updateUserProfileNames(UserJoinInfoList& usersInfo, const ExternalIdList& externalIdsToUpdateFor, const char8_t* externalProfileToken) const
{
    // Iterate over every platform and update the user names: 
    for (auto util : mUtilMap)
    {
        // This code is only used on Xbl, since the persona lookup code only exists in Xbl.
        if ((util.first != xone) && (util.first != xbsx))
            continue;

        // Iterate over the user info, and gather the external users that match the current platform:
        ExternalIdList tempList;
        for (auto curUserInfo : usersInfo)
        {
            for (auto curExtId : externalIdsToUpdateFor)
            {
                // Technically, we could just use this function to fill in the names of everyone on the list.  It'd be simpler. 
                if (curUserInfo->getUser().getUserInfo().getPlatformInfo().getClientPlatform() == util.first && 
                    curUserInfo->getUser().getUserInfo().getPlatformInfo().getExternalIds().getXblAccountId() == curExtId)
                    tempList.push_back(curExtId);
            }
        }

        ExternalSessionUtil::ExternalUserProfiles foundProfiles;
        BlazeRpcError err = util.second->getProfiles(tempList, foundProfiles, externalProfileToken);
        if (ERR_OK != err)
            return err;//logged

        for (auto curProfile : foundProfiles)
        {
            for (auto curUserInfo : usersInfo)
            {
                // This code is only used on Xbl, since the persona lookup code only exists in Xbl.
                if (curUserInfo->getUser().getUserInfo().getPlatformInfo().getExternalIds().getXblAccountId() == curProfile.mExternalId)
                    curUserInfo->getUser().getUserInfo().setPersonaName(curProfile.mUserName.c_str());
            }
        }
    }

    return ERR_OK;
}


Blaze::BlazeRpcError ExternalSessionUtilManager::checkRestrictions(const CheckExternalSessionRestrictionsParameters& params, ExternalSessionApiError& apiErr) const
{
    for (auto userInfo : params.getExternalUserJoinInfos())
    {
        ClientPlatformType userPlatform = userInfo->getUserIdentification().getPlatformInfo().getClientPlatform();
        if (!gController->isPlatformHosted(userPlatform))
        {
            ERR_LOG("[ExternalSessionUtil].checkRestrictions:  User (" << userInfo->getUserIdentification().getBlazeId() << ") attempting to join with invalid ClientPlatform " << ClientPlatformTypeToString(userPlatform) << ".");
            return ERR_SYSTEM;
        }
    }

    BlazeRpcError apiRpcErr = ERR_OK;
    int platformCount = 0;
    for (auto util : mUtilMap)
    {
        ClientPlatformType curPlatform = util.first;
        JoinExternalSessionParameters tempParams;
        params.copyInto(tempParams);

        for (auto userInfo = tempParams.getExternalUserJoinInfos().begin(); userInfo != tempParams.getExternalUserJoinInfos().end(); )
        {
            if (curPlatform != userInfo->get()->getUserIdentification().getPlatformInfo().getClientPlatform())
                userInfo = tempParams.getExternalUserJoinInfos().erase(userInfo);
            else
                ++userInfo;
        }

        if (!tempParams.getExternalUserJoinInfos().empty())
        {
            ++platformCount;

            BlazeRpcError err = util.second->checkRestrictions(tempParams);
            if (err != ERR_OK)
            {
                apiRpcErr = ERR_SYSTEM;

                ExternalSessionPlatformError* platErr = apiErr.getPlatformErrorMap().allocate_element();
                platErr->setBlazeRpcErr(err);

                for (const auto& userInfo : tempParams.getExternalUserJoinInfos())
                    userInfo->getUserIdentification().copyInto(*(platErr->getUsers().pull_back()));

                apiErr.getPlatformErrorMap()[curPlatform] = platErr;
            }
        }
    }

    if (apiRpcErr != ERR_OK)
    {
        apiErr.setFailedOnAllPlatforms(platformCount == apiErr.getPlatformErrorMap().size());
        if (apiErr.getPlatformErrorMap().size() == 1)
            apiErr.setSinglePlatformBlazeRpcErr(apiErr.getPlatformErrorMap().begin()->second->getBlazeRpcErr());
    }

    return apiRpcErr;
}

Blaze::BlazeRpcError ExternalSessionUtilManager::handleUpdateExternalSessionImage(const UpdateExternalSessionImageRequest& request, const GetExternalSessionInfoMasterResponse& getRsp, 
                                                                            GameManagerSlaveImpl& component, UpdateExternalSessionImageErrorResult& errorResult) const
{
    UpdateExternalSessionImageParameters params;
    params.getImage().assignData(request.getCustomImage().getData(), request.getCustomImage().getCount());
    getRsp.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().copyInto(params.getSessionIdentification());

    // Attempt update on all supported platforms:
    BlazeRpcError err = ERR_OK;
    for (auto utilIter : mUtilMap)
    {
        // This code is only used on ps4
        if (utilIter.first != ps4)
            continue;

        ExternalSessionUtil* curUtil = utilIter.second;
            
        ExternalSessionUtil::BlazeIdSet tried;
        size_t attemptNum = 0;
        do
        {
            // main caller might not actually be in the external session (see DevNet ticket 58807). Ensure use someone who is
            ExternalMemberInfo& updater = params.getCallerInfo();
            ExternalMemberInfoListByActivityTypeInfo possibleUpdatersCopy;
            getRsp.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers().copyInto(possibleUpdatersCopy);
            err = curUtil->getUpdater(possibleUpdatersCopy, updater, tried);
            if (err != ERR_OK)
            {
                break;//logged
            }

            err = curUtil->updateImage(params, errorResult);

            if (err != ERR_OK)
                component.incExternalSessionFailureNonImpacting(curUtil->getPlatform());

            // retry on another possible updater if the last one was rate limited. (side: ok for PS4's no-immediate-retries TRC R4123, as its different users)
        } while (shouldRetryAfterExternalSessionUpdateError(err) &&
            (++attemptNum <= curUtil->getConfig().getExternalSessionUpdateRetryLimit()));
    }

    return err;
}

void ExternalSessionUtilManager::handleUpdateExternalSessionProperties(GameManagerMasterImpl* gmMaster, GameId gameId, ExternalSessionUpdateInfoPtr origValues, ExternalSessionUpdateEventContextPtr context) const
{
    UpdateExternalSessionPropertiesParameters params;
    if (EA_LIKELY(origValues != nullptr))
        origValues->copyInto(params.getExternalSessionOrigInfo());
    if (EA_LIKELY(context != nullptr))
        context->copyInto(params.getContext());

    // Attempt update on all supported platforms:
    for (auto utilIter : mUtilMap)
    {
        ExternalSessionUtil* curUtil = utilIter.second;

        ExternalSessionUtil::BlazeIdSet tried;
        BlazeRpcError sessErr = ERR_OK;
        size_t attemptNum = 0;
        do
        {
            if (isFinalExtSessUpdate(params.getContext()))
            {
                // we're handling game's final snapshot before cleared
                params.getContext().getFinalUpdateContext().copyInto(params.getExternalSessionUpdateInfo());
                params.getExternalSessionOrigInfo().getSessionIdentification().copyInto(params.getSessionIdentification());
            }
            else
            {
                // redis: game can disappear between async calls below. Re-fetch
                const GameSessionMaster* gameSession = gmMaster->getReadOnlyGameSession(gameId);// non-blocking
                if (gameSession == nullptr)
                {
                    TRACE_LOG("[GameManagerMasterImpl].handleUpdateExternalSessionProperties: Not updating the external session for game(" << gameId << "), as the game no longer exists.");
                    return;
                }
                // get latest params for the update
                setExternalSessionUpdateInfoFromGame(params.getExternalSessionUpdateInfo(), *gameSession);
                gameSession->getExternalSessionIdentification().copyInto(params.getSessionIdentification());
                // if a specific user must make the update call, set callerInfo to one.
                ExternalMemberInfoListByActivityTypeInfo possibleUpdatersCopy;
                gameSession->getGameDataMaster()->getTrackedExternalSessionMembers().copyInto(possibleUpdatersCopy);
                if (curUtil->getUpdater(possibleUpdatersCopy, params.getCallerInfo(), tried, &params) != ERR_OK) //may block
                {
                    break;//logged
                }
            }
            // final check, after setting params, if update still required. (Some platforms for efficiency might have getUpdater() break already, but not all (Xbox)).
            if (!curUtil->isUpdateRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo(), params.getContext()))
            {
                break;
            }

            sessErr = curUtil->update(params);

            if (sessErr != ERR_OK)
                gmMaster->incrementExternalSessionFailuresNonImpacting(utilIter.first);

            // retry on another possible updater if the last one was rate limited. (ok for PS4's no-immediate-retries TRC R4123, as its different users)
        } while (shouldRetryAfterExternalSessionUpdateError(sessErr) &&
            (++attemptNum <= curUtil->getConfig().getExternalSessionUpdateRetryLimit()));

        if ((sessErr != ERR_OK) && (isFinalExtSessUpdate(params.getContext()) || gmMaster->getReadOnlyGameSession(gameId) != nullptr))
        {
            // if here, currently not much we can do. Just wait for the next game change to trigger the next try
            WARN_LOG("[GameManagerMasterImpl].handleUpdateExternalSessionProperties: Failed to update the external session(" << toLogStr(params.getSessionIdentification()) << ") for game(" << gameId 
                << "), error(" << ErrorHelp::getErrorName(sessErr) << "), platform("<< ClientPlatformTypeToString(utilIter.first) <<"). Updates may appear at a later time.");
        }
    }
}

void ExternalSessionUtilManager::prepForReplay(GameSessionMaster& gameSession) const
{
    for (auto util : mUtilMap)
    {
        util.second->prepForReplay(gameSession);
    }
}

Blaze::BlazeRpcError ExternalSessionUtilManager::prepForCreateOrJoin(CreateGameRequest& request) const
{
    for (auto util : mUtilMap)
    {
        auto err = util.second->prepForCreateOrJoin(request);
        if (err != ERR_OK)
            return err;
    }
    return ERR_OK;
}

}

}

