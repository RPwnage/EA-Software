/*! ************************************************************************************************/
/*!
    \file hostinjectionjob.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/connection/selector.h"

#include "gamemanager/hostinjectionjob.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/tdf/gamebrowser_server.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace GameManager
{

const float HostInjectionJob::EXACT_MATCH_THRESHOLD = 1.0;

HostInjectionJob::HostInjectionJob( 
    GameId gameId, const ChooseHostForInjectionRequest &hostInjectionRequest)
    : mGameId(gameId), mFiberHandle(Fiber::INVALID_FIBER_ID)
{
    hostInjectionRequest.copyInto(mChooseHostForInjectionRequest);
    
}

HostInjectionJob::~HostInjectionJob()
{
    // If this is deleted, we need to cancel the active fiber
    Fiber::cancel(mFiberHandle, ERR_CANCELED);
}

void HostInjectionJob::start()
{
    gFiberManager->scheduleCall(this, &HostInjectionJob::attemptHostInjection, Fiber::CreateParams("HostInjectionJob::attemptHostInjection", Fiber::CreateParams()), &mFiberHandle);
}

void HostInjectionJob::attemptHostInjection()
{
    if (!gUserSessionManager->getSessionExists(mChooseHostForInjectionRequest.getJoinerSessionInfo().getSessionId()))
    {
        TRACE_LOG("[HostInjectionJob].attemptHostInjection initial joining user session (" << mChooseHostForInjectionRequest.getJoinerSessionInfo().getSessionId()
            << ") was nullptr when looked up.");
        terminateHostInjectionJob(Blaze::GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
        return;
    }
    
    
    const char8_t* templateName = nullptr;
    FindDedicatedServerRulesMap findDedicatedServerRulesMap;
    GameSessionMasterPtr dedicatedServerGame = gGameManagerMaster->getWritableGameSession(mGameId);
    if (dedicatedServerGame != nullptr)
    {
        // Construct FindDedicatedServerRules based on the virtual game's dedicated server attributes.
        templateName = dedicatedServerGame->getCreateGameTemplate();

        for (const auto& attrib : dedicatedServerGame->getDedicatedServerAttribs())
        {
            const DedicatedServerAttributeRuleServerConfigMap& configMap = gGameManagerMaster->getConfig().getMatchmakerSettings().getRules().getDedicatedServerAttributeRules();
            for (const auto& configRule : configMap)
            {
                // Check if there is a rule containing this attribute name. If so, add the entry
                if (blaze_stricmp(configRule.second->getAttributeName(), attrib.first) == 0)
                {
                    const RuleName& ruleName = configRule.first;
                    FindDedicatedServerRulesMap::iterator findItr = findDedicatedServerRulesMap.find(ruleName);
                    if (findItr == findDedicatedServerRulesMap.end())
                    {
                        findDedicatedServerRulesMap[ruleName] = findDedicatedServerRulesMap.allocate_element();
                    }
                    
                    FindDedicateServerRequireData* data = findDedicatedServerRulesMap[ruleName];
                    if (data != nullptr)
                    {
                        data->setDesiredValue(attrib.second);
                        data->setMinFitThresholdValue(EXACT_MATCH_THRESHOLD);
                    }
                }
            }
        }
    }

    // Initialize the lists:
    GameIdsByFitScoreMap gamesToSend;    
    ClientPlatformTypeList tempList;
    BlazeRpcError error = findDedicatedServerHelper(nullptr, gamesToSend,
                                                    gGameManagerMaster->getAllowMismatchedPingSiteForVirtualizedGames(),
                                                    &mChooseHostForInjectionRequest.getGamePingSiteAliasList(),
                                                    mChooseHostForInjectionRequest.getGameProtocolVersionString(),
                                                    mChooseHostForInjectionRequest.getRequiredPlayerCapacity(),
                                                    mChooseHostForInjectionRequest.getNetworkTopology(),
                                                    templateName,
                                                    &findDedicatedServerRulesMap,
                                                    &mChooseHostForInjectionRequest.getJoinerSessionInfo(),
                                                    dedicatedServerGame ? dedicatedServerGame->getBasePlatformList() : tempList);
    if (error != ERR_OK)
    {
        WARN_LOG("[HostInjectionJob] No available resettable dedicated servers found.");
        terminateHostInjectionJob(error);
        return;
    }

    ChooseHostForInjectionResponse injectionResponse;
    
    if (!gGameManagerMaster->getDedicatedServerForHostInjection(mChooseHostForInjectionRequest, injectionResponse, gamesToSend))
    {
        // there were no hosts available for injection
        TRACE_LOG("[HostInjectionJob].attemptHostInjection game session (" << mGameId << ") was unable to locate a viable host.");
        terminateHostInjectionJob(Blaze::GAMEMANAGER_ERR_NO_HOSTS_AVAILABLE_FOR_INJECTION);
        return;
    }

    // we blocked to check other masters, need to ensure that the game session is still around before attempting the injection
    GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(mGameId);
    if (gameSession != nullptr)
    {
        if (!gameSession->isInactiveVirtual())
        {
            // game state is changed, something went wrong
            ERR_LOG("[HostInjectionJob].attemptHostInjection game session (" << mGameId << ") wasn't in INACTIVE_VIRTUAL state on lookup.");
            terminateHostInjectionJob(Blaze::GAMEMANAGER_ERR_NOT_VIRTUAL_GAME);
            return;
        }
        TRACE_LOG("[HostInjectionJob].attemptHostInjection Injecting host user (" << injectionResponse.getHostInfo().getUserInfo().getId() <<
            ") from remote resettable game session (" << injectionResponse.getHostPreviousGameId() << ") as host of dedicated server game (" << mGameId << ").");
        // the master owning the game has already destroyed it, just complete the injection.
        BlazeRpcError err = gameSession->injectHost(injectionResponse.getHostInfo(), injectionResponse.getHostPreviousGameId(), injectionResponse.getHostNetworkAddressList(), mChooseHostForInjectionRequest.getGameProtocolVersionString());
        if (err != ERR_OK)
        {
            terminateHostInjectionJob(err);
            return;
        }
    }
    else
    {
        TRACE_LOG("[HostInjectionJob].attemptHostInjection game session (" << mGameId << ") was destroyed before injection could be attempted.");
    }

    // if the game was nullptr, but the job is still around game destruction happened, players were already notified.

    gGameManagerMaster->removeHostInjectionJob(mGameId);
}

void HostInjectionJob::terminateHostInjectionJob(BlazeRpcError joinError)
{
    GameSessionMasterPtr dedicatedServerGame = gGameManagerMaster->getWritableGameSession(mGameId);
    if (dedicatedServerGame != nullptr)
    {
        // iterate through the whole roster to notify that the injection failed.
        // if it is nullptr, the game was already destructed, roster was already notified

        // Send off the HTTP event as well:
        dedicatedServerGame->buildAndSendTournamentGameEvent(dedicatedServerGame->getGameEndEventUri(), joinError);

        // now eject host, if there is one.                      
        if (dedicatedServerGame->getDedicatedServerHostSessionExists())
        {
            // If we get here for a non-virtualized game, ejectHost will end up destroying the game (rather than attempting more injections)
            dedicatedServerGame->ejectHost(dedicatedServerGame->getDedicatedServerHostSessionId(), dedicatedServerGame->getDedicatedServerHostBlazeId());
        }
        else
        {
            // clean up a failed ejection with no host
            dedicatedServerGame->cleanUpFailedRemoteInjection();
        }
    }

    gGameManagerMaster->removeHostInjectionJob(mGameId);
}

} // namespace GameManager
} // namespace Blaze
