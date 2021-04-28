/*! ************************************************************************************************/
/*!
    \file gamemanagerhelperutils.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/userset/userset.h"

namespace Blaze
{
namespace GameManager
{
BlazeRpcError findDedicatedServerHelper(GameManagerSlave* component, GameIdsByFitScoreMap& outGamesToReset, 
                                        bool allowMismatchedPingSites,
                                        const PingSiteAliasList* preferredPingSites,
                                        GameProtocolVersionString gameProtocolVersionString,
                                        uint16_t maxPlayerCapacity,
                                        GameNetworkTopology netTopology,
                                        const char8_t* templateName,
                                        const FindDedicatedServerRulesMap* findDedicatedServerRulesMap,
                                        const UserSessionInfo* hostUserSessionInfo,
                                        const ClientPlatformTypeList& overrideList)
{
    BlazeRpcError error;
    if (component == nullptr)
    {
        component = static_cast<Blaze::GameManager::GameManagerSlave*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, false, true, &error));
        if (component == nullptr || error != ERR_OK)
        {
            WARN_LOG("[findDedicatedServerHelper] An error occurred while getting the gameManagerSlave. Error: " << ErrorHelp::getErrorName(error));
            return error;
        }
    }

    FindDedicatedServersRequest request;
    FindDedicatedServersResponse response;
    MatchmakingCriteriaError criteriaError;

    request.setAllowMismatchedPingSites(allowMismatchedPingSites);
    if (preferredPingSites != nullptr)
    {
        preferredPingSites->copyInto(request.getPreferredPingSites());
    }
    request.setGameProtocolVersionString(gameProtocolVersionString);
    request.setMaxPlayerCapacity(maxPlayerCapacity);
    request.setNetTopology(netTopology);
    overrideList.copyInto(request.getClientPlatformOverrideList());

    if (templateName != nullptr && *templateName != '\0')
    {
        request.setCreateGameTemplateName(templateName);
        if (findDedicatedServerRulesMap != nullptr)
            findDedicatedServerRulesMap->copyInto(request.getFindDedicatedServerRulesMap());
    }
    
    if (hostUserSessionInfo)
    {
        hostUserSessionInfo->copyInto(request.getUserSessionInfo());
    }

    error = component->findDedicatedServers(request, response, criteriaError);
    if (error != ERR_OK)
    {
        WARN_LOG("[findDedicatedServerHelper] No available resettable dedicated servers found. Error: " << ErrorHelp::getErrorName(error));
        return error;
    }

    response.getGamesByFitScoreMap().swap(outGamesToReset);
    return ERR_OK;
}

// AUDIT: this function returns a UserSessionInfo that isn't used, only the UserJoinInfoPtr gets used.
UserSessionInfo* addHostSessionInfo(UserJoinInfoList& usersInfo, UserJoinInfoPtr* hostJoinInfoPtr)
{
    UserSessionInfo* userSessionInfo = getHostSessionInfo(usersInfo, hostJoinInfoPtr);
    if (userSessionInfo)
        return userSessionInfo;

    // AUDIT: This should never happen hopefully as the host UserInfo should have been set
    // when MM started in userIdListToJoiningInfoList.  Any CreateGameRequest should already know the host.

    // If the host user session info doesn't exist, then we add one here:
    UserJoinInfoPtr newHostInfo = usersInfo.pull_back();
    newHostInfo->setIsHost(true);
    if (hostJoinInfoPtr != nullptr)
    {
        *hostJoinInfoPtr = newHostInfo;
    }
    return &newHostInfo->getUser();
}

UserSessionInfo* getHostSessionInfo(UserJoinInfoList& usersInfo, UserJoinInfoPtr* hostJoinInfoPtr /*= nullptr*/)
{
    for(UserJoinInfoList::iterator iter = usersInfo.begin(); iter != usersInfo.end(); ++iter)
    {
        if ((*iter)->getIsHost())
        {
            if (hostJoinInfoPtr != nullptr)
            {
                *hostJoinInfoPtr = *iter;
            }
            return &(*iter)->getUser();
        }
    }

    return nullptr;
}

const UserSessionInfo* getHostSessionInfo(const UserJoinInfoList& usersInfo, UserJoinInfoPtr* hostJoinInfoPtr /*= nullptr*/)
{
    for(UserJoinInfoList::const_iterator iter = usersInfo.begin(); iter != usersInfo.end(); ++iter)
    {
        if ((*iter)->getIsHost())
        {
            if (hostJoinInfoPtr != nullptr)
            {
                *hostJoinInfoPtr = *iter;
            }
            return &(*iter)->getUser();
        }
    }

    ERR_LOG("[getHostSessionInfo] No host exists in the given UserJoinInfoList, returning nullptr. This should never happen.");
    return nullptr;
}

UserSessionId getHostUserSessionId(const UserJoinInfoList& usersInfo)
{
    const UserSessionInfo* hostUser = getHostSessionInfo(usersInfo);
    return hostUser ? hostUser->getSessionId() : INVALID_USER_SESSION_ID;
}

} // namespace GameManager
} // namespace Blaze
