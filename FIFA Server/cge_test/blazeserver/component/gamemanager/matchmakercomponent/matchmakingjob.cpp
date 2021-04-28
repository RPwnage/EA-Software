/*! ************************************************************************************************/
/*!
    \file matchmakingjob.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/connection/selector.h"

#include "gamemanager/matchmakercomponent/matchmakingjob.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/tdf/gamemanager.h" // for player net connection status
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h"
#include "component/gamemanager/playerinfo.h" // for getSessionExists in createOrResetRemoteGameSession()
#include "framework/util/networktopologycommoninfo.h" // for isDedicatedHostedTopology
#include "framework/util/random.h"
// event management
#include "gamemanager/tdf/gamemanagerevents_server.h"
#include "framework/event/eventmanager.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"
#include "gamemanager/tdf/gamebrowser_server.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h" // for lookupExternalUserByUserIdentification() in cleanupPlayersFromGame()
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/pinutil/pinutil.h"
namespace Blaze
{
namespace Matchmaker
{

    MatchmakingJob::MatchmakingJob(MatchmakerSlaveImpl &matchmakerSlaveComponent, const UserSessionId &owningUserSessionId, const Blaze::GameManager::MatchmakingSessionId &owningMatchmakingSessionId)
        : mComponent(matchmakerSlaveComponent),
        mOwningUserSessionId(owningUserSessionId),
        mOwningMatchmakingSessionId(owningMatchmakingSessionId),
        mState(MMJOBSTATE_PRE_START),
        mPinSubmissionPtr(nullptr)
    {
    }

    MatchmakingJob::~MatchmakingJob()
    {
    }

    bool MatchmakingJob::isOwningSessionStillValid() const
    {
        if(gUserSessionManager->getSessionExists(mOwningUserSessionId))
        {
            // a logout is the only thing that might invalidate the mm session
            return true;
        }

        return false;
    }

    BlazeRpcError MatchmakingJob::onMatchQosDataAvailable(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response)
    {
        bool areAllSessionsConnected;
        bool allSessionsPassedQosValidation;
        bool dispatchClientListener;

        areAllSessionsConnected = areAllUsersConnected(request);

        allSessionsPassedQosValidation = validateQos(request, response);
        
        dispatchClientListener = areAllSessionsConnected && allSessionsPassedQosValidation;

        // we're destroying the game if not everybody connected
        // GM won't destroy based on this response from a FG session
        response.setDestroyCreatedGame(!dispatchClientListener);

        if (mState < MMJOBSTATE_AWAITING_QOS)
        {
            // create game not ready to finish the qos (still in started state). Queue the results for until we're ready.
            mDelayedConnectionValidatedResult.mGameId = request.getGameId();
            mDelayedConnectionValidatedResult.mDispatchClientListener = dispatchClientListener;
            return ERR_OK;
        }
        sendNotifyMatchmakingSessionConnectionValidated(request.getGameId(), dispatchClientListener);

        mComponent.removeMatchmakingJob(mOwningMatchmakingSessionId);

        return ERR_OK;
    }

    void MatchmakingJob::sendDelayedNotifyMatchmakingSessionConnectionValidatedIfAvailable()
    {
        if (mDelayedConnectionValidatedResult.mGameId != Blaze::GameManager::INVALID_GAME_ID)
        {
            sendNotifyMatchmakingSessionConnectionValidated(mDelayedConnectionValidatedResult.mGameId, mDelayedConnectionValidatedResult.mDispatchClientListener);
            mComponent.removeMatchmakingJob(mOwningMatchmakingSessionId);
        }
    }

    void MatchmakingJob::setPinSubmission(const PINSubmission &pinSubmission, const GameManager::Matchmaker::MatchmakingSession* session) 
    { 
        if (mPinSubmissionPtr == nullptr)
        {
            mPinSubmissionPtr = BLAZE_NEW PINSubmission;
        }

        // expressly setting the visit options is to workaround an issue in the heat2encoder/decoder around change tracking.
        EA::TDF::MemberVisitOptions visitOptions;
        visitOptions.onlyIfNotDefault = true;
        pinSubmission.copyInto(*mPinSubmissionPtr, visitOptions);

        // Insert the scenario attributes into the PIN submission, specifically into the mp_match_join event belonging to the owning usersession
        PINEventsMap::iterator it = mPinSubmissionPtr->getEventsMap().find(session->getOwnerUserSessionId());
        if (it != mPinSubmissionPtr->getEventsMap().end())
        {
            GameManager::PINEventHelper::addScenarioAttributesToMatchJoinEvent(*(it->second), session->getScenarioInfo().getScenarioAttributes());
        }
    }

    bool MatchmakingJob::areAllUsersConnected(const Blaze::GameManager::GameSessionConnectionComplete &request) const
    {
        GameManager::GameQosDataMap::const_iterator qosIter = request.getGameQosDataMap().begin();
        GameManager::GameQosDataMap::const_iterator qosEnd = request.getGameQosDataMap().end();
        for (; qosIter != qosEnd; ++qosIter)
        {
            if (qosIter->second->getGameConnectionStatus() != GameManager::CONNECTED)
            {
                return false;
            }
        }
        return true;
    }

    bool MatchmakingJob::validateQosForMatchmakingSessions(const Blaze::Search::MatchmakingSessionIdList &matchmakingSessionIds, const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const
    {
        bool sessionPassedValidation = mComponent.getMatchmaker()->validateQosForMatchmakingSessions(matchmakingSessionIds, request, response);
        if (!sessionPassedValidation)
        {
            TRACE_LOG("[MatchmakingJob].validateQosForMatchmakingSessions - game (" << request.getGameId()
                << ") from job(" << mOwningMatchmakingSessionId << ") failed QoS validation.");
        }

        return sessionPassedValidation;
    }


    ////////////////////////////
    //// FinalizeFindGameMatchmakingJob
    ////////////////////////////

    void FinalizeFindGameMatchmakingJob::start() 
    {

        if(mState < MMJOBSTATE_STARTED)
        {
            mState = MMJOBSTATE_STARTED;
        }
        else
        {
            ERR_LOG("[FinalizeFindGameMatchmakingJob].start - Job '" << mOwningMatchmakingSessionId 
                << "' was already started, this shouldn't be possible.");
        }

        mComponent.getMatchmaker()->stopOutstandingFindGameSessions(mOwningMatchmakingSessionId, GameManager::SESSION_CANCELED);
    }

    void FinalizeFindGameMatchmakingJob::sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener )
    {

        mComponent.getMatchmaker()->dispatchMatchmakingConnectionVerifiedForSession(mOwningMatchmakingSessionId, gameId, dispatchClientListener);
        if (!dispatchClientListener)
        {
            if (mComponent.getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getContinueMatchingAfterValidationFailure())
            {
                Blaze::Search::MatchmakingSessionIdList sessionIdList;
                sessionIdList.push_back(mOwningMatchmakingSessionId);
                returnSessionsToMatchmaker(sessionIdList, true);
            }
            else
            {
                 mComponent.getMatchmaker()->destroySession(mOwningMatchmakingSessionId, GameManager::SESSION_QOS_VALIDATION_FAILED);
            }
           
        }
        else
        {
            if (mPinSubmissionPtr != nullptr)
            {
                // Update any completion events to note the real time it took to complete, including QoS:
                auto* pinEventList = mPinSubmissionPtr->getEventsMap().find(mOwningMatchmakingSessionId);
                if (pinEventList != mPinSubmissionPtr->getEventsMap().end())
                {
                    for (auto& pinEvent : *pinEventList->second)
                    {
                        // Update the match joined events for this game (not sure why this is part of a list, though)
                        if (pinEvent->get()->getTdfId() == RiverPoster::MPMatchJoinEvent::TDF_ID)
                        {
                            RiverPoster::MPMatchJoinEvent *matchJoinEvent = static_cast<RiverPoster::MPMatchJoinEvent*>(pinEvent->get());
                            auto session = mComponent.getMatchmaker()->findSession(mOwningMatchmakingSessionId);
                            if (session != nullptr)
                            {
                                matchJoinEvent->setMatchmakingDurationSeconds(session->getMatchmakingRuntime().getSec());
                                matchJoinEvent->setEstimatedTimeToMatchSeconds(session->getEstimatedTimeToMatch().getSec());
                                matchJoinEvent->setTotalUsersOnline(session->getTotalGameplayUsersAtSessionStart());
                                matchJoinEvent->setTotalUsersInGame(session->getTotalUsersInGameAtSessionStart());
                                matchJoinEvent->setTotalUsersInMatchmaking(session->getTotalUsersInMatchmakingAtSessionStart());
                                matchJoinEvent->setTotalUsersMatched(session->getCurrentMatchingSessionListPlayerCount());
                                matchJoinEvent->setTotalUsersPotentiallyMatched(session->getTotalMatchingSessionListPlayerCount());
                            }
                        }
                    }
                }

                gUserSessionManager->sendPINEvents(mPinSubmissionPtr);
            }
            mComponent.getMatchmaker()->destroySession(mOwningMatchmakingSessionId, GameManager::SUCCESS_JOINED_EXISTING_GAME, gameId);
        }
    }

    bool FinalizeFindGameMatchmakingJob::validateQos(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const
    {
        // fg only has the owning session
        Blaze::Search::MatchmakingSessionIdList matchmakingSessionIds;
        matchmakingSessionIds.push_back(mOwningMatchmakingSessionId);
        return validateQosForMatchmakingSessions(matchmakingSessionIds, request, response);
    }

    ////////////////////////////
    //// FinalizeCreateGameMatchmakingJob
    ////////////////////////////

    FinalizeCreateGameMatchmakingJob::FinalizeCreateGameMatchmakingJob(Blaze::GameManager::Matchmaker::Matchmaker &matchmaker, 
        const UserSessionId &owningUserSessionId, const Blaze::GameManager::MatchmakingSessionId &owningMatchmakingSessionId, 
        Blaze::GameManager::Matchmaker::CreateGameFinalizationSettings *createGameFinalizationSettings)
        : MatchmakingJob(matchmaker.getMatchmakerComponent(), owningUserSessionId, owningMatchmakingSessionId),
        mAutoJoinUserSessionId(createGameFinalizationSettings->mAutoJoinMemberInfo->mUserSessionInfo.getSessionId()),
        mAutoJoinUserBlazeId(createGameFinalizationSettings->mAutoJoinMemberInfo->mUserSessionInfo.getUserInfo().getId()),
        mAutoJoinMatchmakingSessionId(createGameFinalizationSettings->mAutoJoinMemberInfo->mMMSession.getMMSessionId()),
        mCreateGameFinalizationSettings(createGameFinalizationSettings),
        mMatchmaker(matchmaker)
    {
        // need to create a list of included MM session IDs
        Blaze::GameManager::Matchmaker::MatchmakingSessionList::const_iterator finalizedSessionIter = mCreateGameFinalizationSettings->getMatchingSessionList().begin();
        Blaze::GameManager::Matchmaker::MatchmakingSessionList::const_iterator finalizedSessionEnd = mCreateGameFinalizationSettings->getMatchingSessionList().end();
        for(; finalizedSessionIter != finalizedSessionEnd; ++finalizedSessionIter)
        {
            // we need the pointer to the MM session that finalized this map lets us check to see if it still exists before trying to access it
            if ((mMatchmakingSessionIdToMatchmakingSessionMap.insert(eastl::make_pair((*finalizedSessionIter)->getMMSessionId(), *finalizedSessionIter)).second))
            {
                // unique entry put it in the session ID list
                mMatchmakingSessionIdList.push_back((*finalizedSessionIter)->getMMSessionId());
            }
            else
            {
                ERR_LOG("[FinalizeCreateGameMatchmakingJob].start - Job for matchmaking session '" << mOwningMatchmakingSessionId 
                    << "' had a duplicate entry for matchmaking session '" << (*finalizedSessionIter)->getMMSessionId() << "' in the finalization setting's owning session list.");
            }
        }
    }

    FinalizeCreateGameMatchmakingJob::~FinalizeCreateGameMatchmakingJob()
    {
        delete mCreateGameFinalizationSettings;
    }

     /*! ************************************************************************************************/
        /*!
        \brief schedules a fiber to attempt create game.
    *************************************************************************************************/
    void FinalizeCreateGameMatchmakingJob::start()
    {
        if (mState < MMJOBSTATE_STARTED)
        {
            mState = MMJOBSTATE_STARTED;

            Fiber::CreateParams params;
            params.groupId = mMatchmaker.getFiberGroupId();
            gSelector->scheduleFiberCall(this, &FinalizeCreateGameMatchmakingJob::attemptCreateGame, "FinalizeCreateGameMatchmakingJob::attemptCreateGame", params);
        }
        else
        {
            ERR_LOG("[FinalizeCreateGameMatchmakingJob].start - Job '" << mOwningMatchmakingSessionId 
                << "' was already started, this shouldn't be possible.");
        }
    }

    void FinalizeCreateGameMatchmakingJob::attemptCreateGame()
    {
        if(!areMatchmakingSessionsStillValid())
        {   
            // if anybody logs out prior to creating the game, we send everyone back to try finalizing again
            returnSessionsToMatchmaker(mMatchmakingSessionIdList, false);
            mComponent.removeMatchmakingJob(mOwningMatchmakingSessionId);
            return;
        }
        // If the chosen auto join member logged out, send everyone back to try finalizing again. Note this may not be caught by the above matchmaking session
        // existence checks as the log out of a member that's not its matchmaking session's owner won't trigger removing its matchmaking session.
        if (!isAutoJoinMemberStillValid())
        {
            returnSessionsToMatchmaker(mMatchmakingSessionIdList, false);
            mComponent.removeMatchmakingJob(mOwningMatchmakingSessionId);
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].attemptCreateGame Auto join member " << mAutoJoinUserSessionId << " no longer exists.");
            return;
        }

        TRACE_LOG("[FinalizeCreateGameMatchmakingJob].attemptCreateGame Attempting to create game for job owning session " << mOwningMatchmakingSessionId << ".");

        bool willValidateQos = false;
        bool createdGame = false;
        BlazeRpcError createErr = ERR_SYSTEM;
      
        MatchmakingSessionIdToMatchmakingSessionMap::iterator sessionIter = mMatchmakingSessionIdToMatchmakingSessionMap.find(mOwningMatchmakingSessionId);
        if(sessionIter != mMatchmakingSessionIdToMatchmakingSessionMap.end())
        {

            Blaze::GameManager::CreateGameMasterResponse createResponse;

            // we try not to rely on MM pointers in the job, but we need to grab the owning session that was finalized, which is given to us as const
            // in the cg finalization settings

            Blaze::GameManager::Matchmaker::MatchmakingSession *owningMMSession = const_cast<Blaze::GameManager::Matchmaker::MatchmakingSession*>(sessionIter->second);

            LogContextOverride logAudit(owningMMSession->getOwnerUserSessionId());

            mCreateGameFinalizationSettings->getCreateGameRequest().getCreateReq().setCreateGameTemplateName(owningMMSession->getCachedStartMatchmakingInternalRequestPtr()->getCreateGameTemplateName());
            
            willValidateQos = mComponent.shouldPerformQosCheck(owningMMSession->getNetworkTopology());
            createErr = createOrResetRemoteGameSession(*mCreateGameFinalizationSettings, createResponse, owningMMSession->getNetworkTopology(), owningMMSession->getPrivilegedGameIdPreference());
            if(createErr == ERR_OK && mMatchmakingSessionIdToMatchmakingSessionMap.find(mOwningMatchmakingSessionId) != mMatchmakingSessionIdToMatchmakingSessionMap.end())
            {
                createdGame = true;
                TRACE_LOG("[FinalizeCreateGameMatchmakingJob].attemptCreateGame - created game '" 
                    << createResponse.getCreateResponse().getGameId() << "'from job'" << mOwningMatchmakingSessionId << "'.");

                // now that all the reserved players joined the external session above, fill the auto join session's onReservedExternalPlayers
                // notification, we'll send this (and all other mmsession owner's notifications) after all other players in the matchup joined.
                Blaze::GameManager::Matchmaker::NotifyMatchmakingReservedExternalPlayersMap pendingNotifications;
                mComponent.getMatchmaker()->fillNotifyReservedExternalPlayers(mOwningMatchmakingSessionId, createResponse.getCreateResponse().getGameId(), pendingNotifications);

                setPinSubmission(createResponse.getPinSubmission(), owningMMSession);
               
                joinRemainingSessions(createResponse.getCreateResponse().getGameId(), pendingNotifications);
                // add the auto-join session (wouldn't be part of the join response)
                mJoinedMatchmakingSessionIdList.push_back(mAutoJoinMatchmakingSessionId);

                if (!willValidateQos)
                {
                    // skip Qos validation delay on client
                    sendNotifyMatchmakingSessionConnectionValidated(createResponse.getCreateResponse().getGameId(), true);
                }

                // dispatch onReservedExternalPlayers to appropriate mm session owners. Note: for matchmaking,
                // we do this after all players in the matchup have joined the external session, for MPS use cases.
                for (Blaze::GameManager::Matchmaker::NotifyMatchmakingReservedExternalPlayersMap::const_iterator itr =
                    pendingNotifications.begin(); itr != pendingNotifications.end(); ++itr)
                {
                    mComponent.sendNotifyServerMatchmakingReservedExternalPlayersToSliver(UserSession::makeSliverIdentity((*itr).first), &(*itr).second);
                }
            }

        }
        
        // return remaining sessions to the matchmaker
        if (createdGame)
        {
            returnSessionsToMatchmaker(mFailedGameJoinMatchmakingSessionIdList, false);

            // clean up the FG sessions owned by joiners so we're ready in case the QoS fails
            Blaze::GameManager::MatchmakingJoinGameResponse::MatchmakingSessionIdList::const_iterator joinedListIter = mJoinedMatchmakingSessionIdList.begin();
            Blaze::GameManager::MatchmakingJoinGameResponse::MatchmakingSessionIdList::const_iterator joinedListEnd = mJoinedMatchmakingSessionIdList.end();
            for(; joinedListIter != joinedListEnd; ++joinedListIter)
            {
                mComponent.getMatchmaker()->stopOutstandingFindGameSessions(*joinedListIter, GameManager::SUCCESS_CREATED_GAME);
            }
        }
        else
        {
            // If we failed to reset a dedicated server. Disqualify the finalizing session and all pulled in sessions that support FG mode from further CG evaluation.
            bool failedToResetDedicatedServer = (createErr ==  Blaze::GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND || createErr == Blaze::GAMEBROWSER_ERR_INVALID_CRITERIA);
            if (failedToResetDedicatedServer)
            {
                mComponent.getMatchmaker()->getMatchmakerMetrics().mTotalMatchmakingCreateGameFinalizationsFailedToResetDedicatedServer.increment();
            }

            //NOTE:: we dont call returnSessionsToMatchmaker here cause we want to be able to auto put the session back into create game,
            // which requires it not to be in the job map.
            // we just pass back ids. MM can handle getting session ids for discontinued sessions.
            // and we've already destroyed the sessions that did finalize
            GameManager::Matchmaker::Matchmaker::ReinsertionActionType reinsertionAction = 
                failedToResetDedicatedServer ? GameManager::Matchmaker::Matchmaker::STOP_EVALUATING_CREATE_GAME : GameManager::Matchmaker::Matchmaker::NO_ACTION;
            returnSessionsToMatchmaker(mMatchmakingSessionIdList, false, reinsertionAction);

            mComponent.removeMatchmakingJob(mOwningMatchmakingSessionId);

            return;
        }

        if(!willValidateQos)
        {
            mComponent.removeMatchmakingJob(mOwningMatchmakingSessionId);
        }
        else 
        {
            setAwaitingQos();
            // if we crossed the wire and already have qos ready just finish job off now.
            sendDelayedNotifyMatchmakingSessionConnectionValidatedIfAvailable();
        }

        return;
    }
    
    void FinalizeCreateGameMatchmakingJob::joinRemainingSessions( Blaze::GameManager::GameId gameId, 
        Blaze::GameManager::Matchmaker::NotifyMatchmakingReservedExternalPlayersMap& pendingNotifications)
    {
        if(!isOwningSessionStillValid())
        {   // if the owning (host) session is invalid, early out, we're returning everyone to the pool
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].joinRemainingSessions - joining game '" 
                << gameId << "'from job'" << mOwningMatchmakingSessionId << "' failed due to owning session becoming invalid.");
            return;
        }

        if(!isAutoJoinMemberStillValid())
        {   // if the owning (host) session is invalid, early out, we're returning everyone to the pool
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].joinRemainingSessions - joining game '" 
                << gameId << "'from job'" << mOwningMatchmakingSessionId << "' failed due to auto join session becoming invalid.");
            return;
        }

        // don't join users that have logged out
        if(removeInvalidSessions())
        {
            // removed some sessions, we need to rebuild the matched session list
            rebuildMatchedSessions();
        }

        // we keep a list of the joined sessions so we can return any that fail to join  to the MM pool.
        Blaze::GameManager::MatchmakingJoinGameResponse response;
        BlazeRpcError err = ERR_SYSTEM;
        err = joinSessionsToRemoteGameSession(gameId, *mCreateGameFinalizationSettings, response);

       
        if(err == ERR_OK)
        {
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].joinRemainingSessions - joining game '" 
                << gameId << "'from job'" << mOwningMatchmakingSessionId << "' succeeded.");

            Blaze::GameManager::MatchmakingJoinGameResponse::MatchmakingSessionIdList::const_iterator joinedListIter = response.getJoinedMatchmakingSessions().begin();
            Blaze::GameManager::MatchmakingJoinGameResponse::MatchmakingSessionIdList::const_iterator joinedListEnd = response.getJoinedMatchmakingSessions().end();
            for(; joinedListIter != joinedListEnd; ++joinedListIter)
            {
                // successfully joined external sessions. we'll send the reservedExternalPlayers notifications after all players in the match-up joined.
                mComponent.getMatchmaker()->fillNotifyReservedExternalPlayers(*joinedListIter, gameId, pendingNotifications);
                mJoinedMatchmakingSessionIdList.push_back(*joinedListIter);
            }

            mFailedGameJoinMatchmakingSessionIdList.assign(response.getFailedToJoinMatchmakingSessions().begin(), response.getFailedToJoinMatchmakingSessions().end());

        }
        else
        {
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].joinRemainingSessions - joining game '" 
                << gameId << "'from job'" << mOwningMatchmakingSessionId << "' failed with error '"
                << ErrorHelp::getErrorName(err) << "'.");
            Blaze::Search::MatchmakingSessionIdList::const_iterator mmIdListIter = mMatchmakingSessionIdList.begin();
            Blaze::Search::MatchmakingSessionIdList::const_iterator mmIdListEnd = mMatchmakingSessionIdList.end();
            for(; mmIdListIter != mmIdListEnd; ++mmIdListIter)
            {
                // build our list of sessions to kill
                if (*mmIdListIter != mAutoJoinMatchmakingSessionId)
                    mFailedGameJoinMatchmakingSessionIdList.push_back(*mmIdListIter);
            }
        }

    }

    bool FinalizeCreateGameMatchmakingJob::removeInvalidSessions()
    {
        bool removedSessions = false;
        MatchmakingSessionIdToMatchmakingSessionMap::iterator sessionIter = mMatchmakingSessionIdToMatchmakingSessionMap.begin();
        MatchmakingSessionIdToMatchmakingSessionMap::iterator sessionEnd = mMatchmakingSessionIdToMatchmakingSessionMap.end();
        while(sessionIter != sessionEnd)
        {
            // we may erase here, so cache and advance the iterator first
            MatchmakingSessionIdToMatchmakingSessionMap::iterator currentIter = sessionIter;
            ++sessionIter;

            if(!mComponent.getMatchmaker()->isMatchmakingSessionInMap(currentIter->first))
            {
                TRACE_LOG("[FinalizeCreateGameMatchmakingJob].removeInvalidSessions - removing matchmaking session '" 
                    << currentIter->first << "'from job'" << mOwningMatchmakingSessionId << "'.");
                // the only thing that might invalidate the session is a user logout, so check that each user is still logged in
                mMatchmakingSessionIdToMatchmakingSessionMap.erase(currentIter);
                removedSessions = true;
            }

        }

        return removedSessions;
    }

    void MatchmakingJob::returnSessionsToMatchmaker(Blaze::Search::MatchmakingSessionIdList sessionIdList, bool updatedQosCriteria, GameManager::Matchmaker::Matchmaker::ReinsertionActionType reinsertionAction, GameManager::Rete::ProductionNodeId productionNodeId)
    {
        // we just pass back ids. MM can handle getting session ids for discontinued sessions.
        // and we've already destroyed the sessions that did finalize
        mComponent.getMatchmaker()->reinsertFailedToFinalizeSessions(sessionIdList, updatedQosCriteria, reinsertionAction, productionNodeId);
        sessionIdList.clear();
    }


    void FinalizeCreateGameMatchmakingJob::rebuildMatchedSessions()
    {
        mCreateGameFinalizationSettings->getMatchingSessionList().clear();
        MatchmakingSessionIdToMatchmakingSessionMap::iterator sessionIter = mMatchmakingSessionIdToMatchmakingSessionMap.begin();
        MatchmakingSessionIdToMatchmakingSessionMap::iterator sessionEnd = mMatchmakingSessionIdToMatchmakingSessionMap.end();
        for(; sessionIter != sessionEnd; ++sessionIter)
        {
            mCreateGameFinalizationSettings->getMatchingSessionList().push_back(sessionIter->second);
        }

    }

    void FinalizeCreateGameMatchmakingJob::cleanupSuccessfulSessions(GameManager::GameId gameId)
{
        // we need to take the usersessions that joined the new game, delete their associated mm sessions, and clean them from the mm map.
        Blaze::Search::MatchmakingSessionIdList::const_iterator joinedListIter = mJoinedMatchmakingSessionIdList.begin();
        Blaze::Search::MatchmakingSessionIdList::const_iterator joinedListEnd = mJoinedMatchmakingSessionIdList.end();
        for(; joinedListIter != joinedListEnd; ++joinedListIter)
        {
            GameManager::MatchmakingResult result = (mAutoJoinMatchmakingSessionId == *joinedListIter) ? GameManager::SUCCESS_CREATED_GAME : GameManager::SUCCESS_JOINED_NEW_GAME;
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].cleanupSuccessfulSessions - destroying matchmaking session '" 
                << *joinedListIter << "' with result: " << GameManager::MatchmakingResultToString(result) << ".");

            mComponent.getMatchmaker()->destroySession(*joinedListIter, result, gameId);
        }
    }

    void FinalizeCreateGameMatchmakingJob::terminateSessionsAsFailedQosValidation(Blaze::Search::MatchmakingSessionIdList failedQosList)
    {
        // we need to take the usersessions that joined the new game, delete their associated mm sessions, and clean them from the mm map.
        Blaze::Search::MatchmakingSessionIdList::const_iterator iter = failedQosList.begin();
        Blaze::Search::MatchmakingSessionIdList::const_iterator end = failedQosList.end();
        for(; iter != end; ++iter)
        {
            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].terminateSessionsAsFailedQosValidation - destroying matchmaking session '" 
                << *iter << "' after failing qos validation.");
            mComponent.getMatchmaker()->destroySession(*iter, GameManager::SESSION_QOS_VALIDATION_FAILED);
        }
    }

    bool FinalizeCreateGameMatchmakingJob::isAutoJoinMemberStillValid() const
    {
        return mComponent.getMatchmaker()->isUserInCreateGameMatchmakingSession(mAutoJoinUserSessionId, mAutoJoinMatchmakingSessionId);
    }

    bool FinalizeCreateGameMatchmakingJob::areMatchmakingSessionsStillValid() const
    {
        MatchmakingSessionIdToMatchmakingSessionMap::const_iterator sessionIter = mMatchmakingSessionIdToMatchmakingSessionMap.begin();
        MatchmakingSessionIdToMatchmakingSessionMap::const_iterator sessionEnd = mMatchmakingSessionIdToMatchmakingSessionMap.end();
        for(; sessionIter != sessionEnd; ++sessionIter)
        {
            if(!mComponent.getMatchmaker()->isMatchmakingSessionInMap(sessionIter->first))
            {
                // the only thing that might invalidate the session is a user logout, so check that each user is still logged in
                TRACE_LOG("[FinalizeCreateGameMatchmakingJob] Session " << sessionIter->first << " no longer exists.")
                return false;
            }
        }

        return true; 
    }

    /*! ************************************************************************************************/
    /*! \brief Create or Reset a game for this matchmaking session.
            The supplied host/members vote on the appropriate game values, and reservations are made for all joining players.

        WARNING: THE CREATING OR RESETTING PLAYER AND ASSOCIATED GROUP MEMBERS NOW JOIN THE GAME ALL AT ONCE.

        \param[in] finalizationSettings struct containing the host, and matchmaking sessions that should join the game explicitly
        \param[in] response TDF object containing the create game response
    ***************************************************************************************************/
    BlazeRpcError FinalizeCreateGameMatchmakingJob::createOrResetRemoteGameSession( GameManager::Matchmaker::CreateGameFinalizationSettings &finalizationSettings, GameManager::CreateGameMasterResponse &response, GameNetworkTopology networkTopology, GameManager::GameId privilegedGameIdPreference )
    {
        if (EA_UNLIKELY(!gUserSessionManager->getSessionExists(mAutoJoinUserSessionId)))
        {
            ERR_LOG("[FinalizeCreateGameMatchmakingJob(" << mOwningMatchmakingSessionId << "/" << mOwningUserSessionId << ")] internal error: player without a user session selected as auto join player.");
            return ERR_SYSTEM;
        }

        auto ajSession = mMatchmaker.findCreateGameSession(mAutoJoinMatchmakingSessionId);
        if (ajSession == nullptr)
        {
            ERR_LOG("[FinalizeCreateGameMatchmakingJob] Auto join MM session " << mAutoJoinMatchmakingSessionId << " not found, finalizaiton fail.");
            return ERR_SYSTEM;
        }

        Blaze::GameManager::Matchmaker::MatchmakingSession &autoJoinMatchmakingSession = *ajSession;

        // Get the already initialized request, still needs setup reasons
        GameManager::MatchmakingCreateGameRequest &matchmakingCreateGameRequest = finalizationSettings.getCreateGameRequest();
        matchmakingCreateGameRequest.getCreateReq().setFinalizingMatchmakingSessionId(mOwningMatchmakingSessionId);

        GameManager::FitScore maxPossibleFitScore = autoJoinMatchmakingSession.getCriteria().calcMaxPossibleFitScore();
        matchmakingCreateGameRequest.setTrackingTag(autoJoinMatchmakingSession.getTrackingTag().c_str());

        Blaze::GameManager::FindDedicatedServerRulesMap findDedicatedServerRulesMap;
        if (isDedicatedHostedTopology(networkTopology))
        {
            autoJoinMatchmakingSession.getCachedStartMatchmakingInternalRequestPtr()->getFindDedicatedServerRulesMap().copyInto(findDedicatedServerRulesMap);
        }

        // no fit score for create game.
        // refactor this to use the TDF map directly instead of the typedef
        GameManager::Matchmaker::UserSessionSetupReasonMap setupReasons;
        autoJoinMatchmakingSession.fillUserSessionSetupReason(setupReasons, GameManager::SUCCESS_CREATED_GAME, autoJoinMatchmakingSession.getCreateGameFinalizationFitScore(), maxPossibleFitScore);

        // Now fill in setup reasons for every session we matched against and their sub members.
        GameManager::Matchmaker::MatchmakingSessionList::const_iterator sessionIt = finalizationSettings.getMatchingSessionList().begin();
        GameManager::Matchmaker::MatchmakingSessionList::const_iterator sessionItEnd = finalizationSettings.getMatchingSessionList().end();
        for (; sessionIt != sessionItEnd; ++sessionIt)
        {
            const Blaze::GameManager::Matchmaker::MatchmakingSession *mmSession = *sessionIt;
            if (mmSession->getMMSessionId() != autoJoinMatchmakingSession.getMMSessionId())
            {
                mmSession->fillUserSessionSetupReason(setupReasons, GameManager::SUCCESS_JOINED_NEW_GAME, mmSession->getCreateGameFinalizationFitScore(), maxPossibleFitScore);
            }
        }

        matchmakingCreateGameRequest.getUserSessionGameSetupReasonMap().reserve(setupReasons.size());
        GameManager::Matchmaker::UserSessionSetupReasonMap::iterator setupIter = setupReasons.begin();
        GameManager::Matchmaker::UserSessionSetupReasonMap::iterator setupEnd = setupReasons.end();
        for(; setupIter != setupEnd; ++setupIter)
        {
            UserSessionId sessionId = setupIter->first;
            GameManager::GameSetupReason *reason = setupIter->second;
            matchmakingCreateGameRequest.getUserSessionGameSetupReasonMap().insert(eastl::make_pair(sessionId, reason));
        }

        // prevent debug only sessions from actually creating the game.
        auto fSession = mMatchmaker.findCreateGameSession(mCreateGameFinalizationSettings->getFinalizingMatchmakingSessionId());

        if (fSession == nullptr)
        {
            ERR_LOG("[FinalizeCreateGameMatchmakingJob] Finalizing MM session " << mAutoJoinMatchmakingSessionId << " not found, finalizaiton failed.");
            return ERR_SYSTEM;
        }

        GameManager::Matchmaker::MatchmakingSession &finalizingSession = *fSession;
        if (finalizingSession.getDebugCheckOnly())
        {
            GameManager::Matchmaker::MatchmakingSessionList debugSessionList;
            debugSessionList.reserve(finalizationSettings.getMatchingSessionList().size());

            // first, get a list of any debug sessions that were pulled in.
            GameManager::Matchmaker::MatchmakingSessionList::const_iterator sessionIter = finalizationSettings.getMatchingSessionList().begin();
            GameManager::Matchmaker::MatchmakingSessionList::const_iterator sessionEnd = finalizationSettings.getMatchingSessionList().end();
            for (; sessionIter != sessionEnd; ++sessionIter)
            {
                const GameManager::Matchmaker::MatchmakingSession *mmSession = *sessionIter;

                if (mmSession->getDebugCheckOnly())
                {
                    debugSessionList.push_back(mmSession);
                }
            }

            // now collect results 
            sessionIter = finalizationSettings.getMatchingSessionList().begin();
            for (; sessionIter != sessionEnd; ++sessionIter)
            {
                const GameManager::Matchmaker::MatchmakingSession *mmSession = *sessionIter;

                if (mmSession->getMMSessionId() == finalizingSession.getMMSessionId())
                {
                    continue;
                }
                
                const GameManager::DebugSessionResult* debugResults = finalizationSettings.getDebugResults(mmSession->getMMSessionId());
                if (debugResults != nullptr)
                {
                    finalizingSession.addDebugMatchingSession(*debugResults);
                }
            }


            // add the debug info for the finalizing session for all the pulled-in pseudo sessions
            GameManager::Matchmaker::MatchmakingSessionList::iterator debugSessionIter = debugSessionList.begin();
            GameManager::Matchmaker::MatchmakingSessionList::iterator debugSessionEnd = debugSessionList.end();
            for (; debugSessionIter != debugSessionEnd; ++debugSessionIter)
            {
                // these are stored as const in the MM finalization settings, but we treat debug sessions differently here
                GameManager::Matchmaker::MatchmakingSession *debugMMSession = const_cast<GameManager::Matchmaker::MatchmakingSession*>(*debugSessionIter);
                if (debugMMSession->getMMSessionId() != finalizingSession.getMMSessionId())
                {
                    debugMMSession->addDebugFinalizingSession();
                }
            }


            // wrap up all the debug sessions
            while (!debugSessionList.empty())
            {
                // finalize as pseudo success, and destroy the sessions
                // these are stored as const in the MM finalization settings, but we treat debug sessions differently here
                GameManager::Matchmaker::MatchmakingSessionList::iterator debugSessionIt = debugSessionList.begin();
                GameManager::Matchmaker::MatchmakingSession *debugMMSession = const_cast<GameManager::Matchmaker::MatchmakingSession*>(*debugSessionIt);

                debugMMSession->setDebugCreateGame(matchmakingCreateGameRequest.getCreateReq(), debugMMSession->getCreateGameFinalizationFitScore(), mAutoJoinUserBlazeId);
                debugMMSession->finalizeAsDebugged(GameManager::SUCCESS_PSEUDO_CREATE_GAME);
                
                debugSessionList.erase(debugSessionIt);
                sessionIter = finalizationSettings.getMatchingSessionList().begin();
                sessionEnd = finalizationSettings.getMatchingSessionList().end();
                for (; sessionIter != sessionEnd; ++sessionIter)
                {
                    const GameManager::Matchmaker::MatchmakingSession *mmSession = *sessionIter;
                    if (mmSession->getMMSessionId() == debugMMSession->getMMSessionId())
                    {
                        finalizationSettings.getMatchingSessionList().erase(sessionIter);
                        break;
                    }

                }

                // now that the session pointers are cleaned up, destroy it
                mComponent.getMatchmaker()->destroySession(debugMMSession->getMMSessionId(), GameManager::SUCCESS_PSEUDO_CREATE_GAME);
            }

            matchmakingCreateGameRequest.getUserSessionGameSetupReasonMap().clear();

            // Not an actual error.
            // Return error here so we don't further finalize the session assuming we've created a game.
            return ERR_SYSTEM;
        }

        // IMPORTANT: Blocking calls below, must verify that MM sessions still exist before referencing them from here on out!
        BlazeRpcError createErrCode = ERR_SYSTEM;

        // Reacquire external session data prior to creating the Game:
        // No clue if the non-xone sessions will have this information set correctly, but this is the same as the rest.
        BlazeRpcError extSessErr = initRequestExternalSessionData(matchmakingCreateGameRequest.getCreateReq().getExternalSessionData(),
            matchmakingCreateGameRequest.getCreateReq().getCreateRequest().getGameCreationData().getExternalSessionIdentSetup(),
            matchmakingCreateGameRequest.getCreateReq().getCreateRequest().getCommonGameData().getGameType(),
            matchmakingCreateGameRequest.getCreateReq().getUsersInfo(),
            mComponent.getConfig().getGameSession(), mComponent.getGameSessionExternalSessionUtilMgr());
        if (extSessErr != ERR_OK)
        {
            return extSessErr;
        }


        // only do game lookups if we don't have a privileged game id
        if (isDedicatedHostedTopology(networkTopology))
        {
            if (privilegedGameIdPreference != GameManager::INVALID_GAME_ID)
            {
                // If we have a game that we have been told to matchmake into for debugging purposes, find it and tell
                // master to reset that one (if it can)
                INFO_LOG("[FinalizeCreateGameMatchmakingJob(" << mOwningMatchmakingSessionId << "/" << mOwningUserSessionId  
                    << ")] .createOrResetRemoteGameSession: session is being sent to preferred game id (" 
                    << privilegedGameIdPreference << ").");
                matchmakingCreateGameRequest.setPrivilegedGameIdPreference(privilegedGameIdPreference);
                // tell gamemanager to not reset anything if the target game is unavailable
                matchmakingCreateGameRequest.setRequirePrivilegedGameId(true);
                // let's try to create the game on the server that owns our target game, this is sharded by privilegedGameIdPreference
                createErrCode = mComponent.getGameManagerMaster()->matchmakingCreateGameWithPrivilegedId(matchmakingCreateGameRequest, response);
            }
            else
            {
                GameManager::GameIdsByFitScoreMap gamesToSend;

                // Sanity check for the player capacity:
                GameManager::CreateGameRequest& createRequest = matchmakingCreateGameRequest.getCreateReq().getCreateRequest();
                uint16_t maxPlayerCapacity = 0;
                for (uint16_t curSlot = 0; curSlot < (uint16_t)createRequest.getSlotCapacities().size(); ++curSlot)
                {
                    maxPlayerCapacity += createRequest.getSlotCapacities()[curSlot];
                }
                if (maxPlayerCapacity > createRequest.getGameCreationData().getMaxPlayerCapacity())
                    createRequest.getGameCreationData().setMaxPlayerCapacity(maxPlayerCapacity);


                const char8_t* createGameTemplateName = mCreateGameFinalizationSettings->getCreateGameRequest().getCreateReq().getCreateGameTemplateName();
                if (*createGameTemplateName != '\0')
                {
                    matchmakingCreateGameRequest.getCreateReq().setCreateGameTemplateName(createGameTemplateName);
                }

                // Lookup the dedicated servers to use:
                BlazeRpcError error = findDedicatedServerHelper(nullptr, gamesToSend, 
                    true,
                    &mCreateGameFinalizationSettings->mOrderedPreferredPingSites,
                    createRequest.getCommonGameData().getGameProtocolVersionString(),
                    createRequest.getGameCreationData().getMaxPlayerCapacity(),
                    networkTopology,
                    createGameTemplateName,
                    &findDedicatedServerRulesMap,
                    Blaze::GameManager::getHostSessionInfo(matchmakingCreateGameRequest.getCreateReq().getUsersInfo()),
                    createRequest.getClientPlatformListOverride());

                if (error != ERR_OK)
                {
                    matchmakingCreateGameRequest.getUserSessionGameSetupReasonMap().clear();
                    WARN_LOG("[FinalizeCreateGameMatchmakingJob] Session " << mOwningMatchmakingSessionId << " No available resettable dedicated servers found, err = " << ErrorHelp::getErrorName(error) << ".");
                    return error;
                }

                if (!areMatchmakingSessionsStillValid() || !isAutoJoinMemberStillValid())
                {
                    matchmakingCreateGameRequest.getUserSessionGameSetupReasonMap().clear();
                    WARN_LOG("[FinalizeCreateGameMatchmakingJob(" << mOwningMatchmakingSessionId << "/" << mOwningUserSessionId << 
                        ")] at least one player logged out while trying to matchmake into dedicated server (after getting servers), when trying to finalize. Finalization failed.");
                    return ERR_SYSTEM;
                } 

                if (!gamesToSend.empty())
                {
                    // walk games to send and attempt to create/reset each one

                    createErrCode = GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;

                    bool done = false;
                    // NOTE: fit score map is ordered by lowest score; therefore, iterate in reverse
                    for (GameManager::GameIdsByFitScoreMap::reverse_iterator gameIdsMapItr = gamesToSend.rbegin(), gameIdsMapEnd = gamesToSend.rend(); gameIdsMapItr != gameIdsMapEnd; ++gameIdsMapItr)
                    {
                        GameManager::GameIdList* gameIds = gameIdsMapItr->second;
                        // random walk gameIds
                        while (!gameIds->empty())
                        {
                            if (!areMatchmakingSessionsStillValid() || !isAutoJoinMemberStillValid())
                            {                   
                                WARN_LOG("[FinalizeCreateGameMatchmakingJob(" << mOwningMatchmakingSessionId << "/" << mOwningUserSessionId << 
                                    ")] at least one player logged out while trying to matchmake into dedicated server when trying to finalize. Finalization failed.");
                                cleanupPlayersFromGame(response.getCreateResponse().getGameId(), response.getExternalSessionParams().getExternalUserJoinInfos(), true);
                                createErrCode = ERR_SYSTEM;
                                done = true;
                                break;
                            } 

                            GameManager::GameIdList::iterator i = gameIds->begin() + Random::getRandomNumber((int32_t)gameIds->size());
                            GameManager::GameId gameId = *i;
                            gameIds->erase_unsorted(i);

                            matchmakingCreateGameRequest.getCreateReq().setGameIdToReset(gameId);
                            RpcCallOptions opts;
                            opts.routeTo.setSliverIdentityFromKey(GameManager::GameManagerMaster::COMPONENT_ID, gameId);
                            BlazeRpcError err = mComponent.getGameManagerMaster()->matchmakingCreateGame(matchmakingCreateGameRequest, response, opts);
                            if (err == Blaze::ERR_OK)
                            {
                                createErrCode = Blaze::ERR_OK;
                                done = true;
                                break;
                            }
                            TRACE_LOG("[FinalizeCreateGameMatchmakingJob].createOrResetRemoteGameSession: matchmaking session " << mOwningMatchmakingSessionId << 
                                " unable to reset dedicated server during finalization, reason: " << ErrorHelp::getErrorName(err) << ".");
                        }

                        if (done)
                            break;
                    }
                }
                else
                {
                    // we need to do a game reset no suitable resettable games, bail
                    createErrCode = Blaze::GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;
                }
            }
        }
        else
        {
            // create game on any master
            createErrCode = mComponent.getGameManagerMaster()->matchmakingCreateGame(matchmakingCreateGameRequest, response);
        }

        if ( createErrCode == ERR_OK )
        {
            // if anybody logs out prior to creating the game, we need to bail out and cleanup the game since the players have already joined
            if(!areMatchmakingSessionsStillValid() || !isAutoJoinMemberStillValid())
            {                   
                cleanupPlayersFromGame(response.getCreateResponse().getGameId(), response.getExternalSessionParams().getExternalUserJoinInfos(), true);
                createErrCode = ERR_SYSTEM;
                WARN_LOG("[FinalizeCreateGameMatchmakingJob(" << mOwningMatchmakingSessionId << "/" << mOwningUserSessionId << 
                    ")] At least one player logged out after creating the game when trying to finalize. Finalization failed.");
            } 
            else
            {            
                // update external session
                ExternalSessionApiError apiErr;
                BlazeRpcError sessErr = mComponent.joinGameExternalSession(response.getExternalSessionParams(), true, autoJoinMatchmakingSession.getCachedStartMatchmakingInternalRequestPtr()->getGameExternalSessionData().getJoinInitErr(), apiErr);
                if (sessErr != ERR_OK)
                {
                    ERR_LOG("[FinalizeCreateGameMatchmakingJob].createOrResetRemoteGameSession: MMSession " << mOwningMatchmakingSessionId << " Failed to join external session after a match into game(" << response.getCreateResponse().getGameId() << "). Error: " << ErrorHelp::getErrorName(sessErr));
                    createErrCode = sessErr;//count as error
                }
                else if (!areMatchmakingSessionsStillValid() || !isAutoJoinMemberStillValid())
                {
                    cleanupPlayersFromGame(response.getCreateResponse().getGameId(), response.getExternalSessionParams().getExternalUserJoinInfos(), true);
                    createErrCode = ERR_SYSTEM;
                    WARN_LOG("[FinalizeCreateGameMatchmakingJob(" << mOwningMatchmakingSessionId << "/" << mOwningUserSessionId <<
                        ")] At least one player logged out after creating the game when trying to finalize during joinExternalSession phase. Finalization failed.");
                }
            }
        }

        //clean up allocated setup reasons
        matchmakingCreateGameRequest.getUserSessionGameSetupReasonMap().clear();
        
        return(createErrCode);
    }

    //cleanup method in case if createOrResetRemoteGameSession fails
    void FinalizeCreateGameMatchmakingJob::cleanupPlayersFromGame(Blaze::GameManager::GameId gameId, const ExternalUserJoinInfoList& externalUserInfos, bool requiresSuperUser) const
    {
        TRACE_LOG("[FinalizeCreateGameMatchmakingJob].cleanupPlayersFromGame: cleanup after failing to finalize Blaze game:" << gameId);
        UserSessionIdList sessionIds;        
        
        for (ExternalUserJoinInfoList::const_iterator itr = externalUserInfos.begin(); itr != externalUserInfos.end(); ++itr)
        {
            UserInfoPtr userInfo;
            if (ERR_OK == Blaze::GameManager::lookupExternalUserByUserIdentification((*itr)->getUserIdentification(), userInfo))
            {
                UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(userInfo->getId());
                if (primarySessionId != INVALID_USER_SESSION_ID)
                    sessionIds.push_back(primarySessionId);
            }
        }
        
        Blaze::GameManager::LeaveGameByGroupMasterRequest leaveRequest;
        Blaze::GameManager::LeaveGameByGroupMasterResponse leaveResponse;
        leaveRequest.setGameId(gameId);
        leaveRequest.setPlayerRemovedReason(Blaze::GameManager::PLAYER_JOIN_EXTERNAL_SESSION_FAILED);
        sessionIds.copyInto(leaveRequest.getSessionIdList());

        UserSession::SuperUserPermissionAutoPtr permissionPtr(requiresSuperUser);
        BlazeRpcError leaveErr = mComponent.getGameManagerMaster()->leaveGameByGroupMaster(leaveRequest, leaveResponse);
        if (ERR_OK != leaveErr)
        {
            ERR_LOG("[FinalizeCreateGameMatchmakingJob].cleanupPlayersFromGame: Cleanup after failing to finalize Blaze game, failed to leave Blaze game with error " << ErrorHelp::getErrorName(leaveErr));
        }
    }

     /*! ************************************************************************************************/
    /*! \brief joins the MatchmakingSessions listed in createGameFinalizationSettings.mOwningSessionList
        into the supplied game.  Group members are joined as well.  NOTE: we also join group members
        of createGameFinalizationSettings.mAutomaticallyJoinedMMSession.

        \param[in] gameId gameId we're joining.
        \param[in] createGameFinalizationSettings contains a listing of the matchmaking sessions to be joined
                        (as well as the session automatically joined during game creation/reset)
        \param[in] matchmaker needed so we can finalize the matchmaking session
        \param[in,out] joinGameResponse from the join request.
        \return error code resulting from join request.
    ***************************************************************************************************/
    Blaze::BlazeRpcError FinalizeCreateGameMatchmakingJob::joinSessionsToRemoteGameSession( Blaze::GameManager::GameId& gameId, const Blaze::GameManager::Matchmaker::CreateGameFinalizationSettings &createGameFinalizationSettings, Blaze::GameManager::MatchmakingJoinGameResponse &joinGameResponse ) const
    {
        // join the explicitly-joining matchmaking sessions (and group members) into the game
        const Blaze::GameManager::Matchmaker::MatchmakingSessionList &joiningSessionList = createGameFinalizationSettings.getMatchingSessionList();
        Blaze::GameManager::Matchmaker::MatchmakingSessionList::const_iterator joiningSessionIter = joiningSessionList.begin();
        Blaze::GameManager::Matchmaker::MatchmakingSessionList::const_iterator joiningSessionEnd = joiningSessionList.end();
        uint16_t totalPlayerCount = 0;
        for ( ; joiningSessionIter != joiningSessionEnd; ++joiningSessionIter )
        {
            const Blaze::GameManager::Matchmaker::MatchmakingSession *joiningSession = *joiningSessionIter;

            // Validate we aren't already switched context to the owning session.
            LogContextOverride logAudit(joiningSession->getOwnerUserSessionId());

            // Have to do the ugly cast so we can set the rule mode in the rule
            Blaze::GameManager::Matchmaker::MatchmakingSession* session = const_cast<Blaze::GameManager::Matchmaker::MatchmakingSession*>(joiningSession);

            // pre-calculated this at finalization
            Blaze::GameManager::FitScore totalFitScore = session->getCreateGameFinalizationFitScore();

            //If the current session owner actually created the game, they are a create game
            //finalization
            Blaze::GameManager::MatchmakingResult result;
            if (joiningSession->getMMSessionId() == mAutoJoinMatchmakingSessionId)
            {
                result = Blaze::GameManager::SUCCESS_CREATED_GAME;
            }
            else
            {
                result = Blaze::GameManager::SUCCESS_JOINED_NEW_GAME;
            }

            // submit matchmaking succeeded event, this will catch the game creator and all joining sessions.
            Blaze::GameManager::SuccesfulMatchmakingSession succesfulMatchmakingSession;
            succesfulMatchmakingSession.setMatchmakingSessionId(joiningSession->getMMSessionId());
            succesfulMatchmakingSession.setMatchmakingScenarioId(joiningSession->getMMScenarioId());
            succesfulMatchmakingSession.setUserSessionId(joiningSession->getOwnerUserSessionId());
            succesfulMatchmakingSession.setPersonaName(joiningSession->getOwnerPersonaName());
            succesfulMatchmakingSession.setPersonaNamespace(joiningSession->getOwnerPersonaNamespace());
            succesfulMatchmakingSession.setClientPlatform(joiningSession->getOwnerClientPlatform());
            succesfulMatchmakingSession.setMatchmakingResult(MatchmakingResultToString(result));
            succesfulMatchmakingSession.setFitScore(totalFitScore);
            succesfulMatchmakingSession.setMaxPossibleFitScore(joiningSession->getCriteria().calcMaxPossibleFitScore());
            succesfulMatchmakingSession.setGameId(gameId);

            gEventManager->submitEvent(static_cast<uint32_t>(Blaze::GameManager::GameManagerMaster::EVENT_SUCCESFULMATCHMAKINGSESSIONEVENT), succesfulMatchmakingSession);

            totalPlayerCount += joiningSession->getPlayerCount();

            // We need to create a list of users that will be used to indicate who joined successfully, and who failed. 
            if (joiningSession->getMMSessionId() != mAutoJoinMatchmakingSessionId)
            {
                // So, all the sessions that joined are good. And all the sessions that didn't, failed. 
                joinGameResponse.getJoinedMatchmakingSessions().push_back(joiningSession->getMMSessionId());

                // Technically, we should be able to determine if the session joins were successful or not, but it's not clear where that information lives. 
                // joinGameResponse.getFailedToJoinMatchmakingSessions().push_back(joiningSession->getMMSessionId());
            }
        }

        for (joiningSessionIter = joiningSessionList.begin(); joiningSessionIter != joiningSessionEnd; ++joiningSessionIter )
        {
            const Blaze::GameManager::Matchmaker::MatchmakingSession *joiningSession = *joiningSessionIter;
            Blaze::GameManager::Matchmaker::MatchmakingSession* session = const_cast<Blaze::GameManager::Matchmaker::MatchmakingSession*>(joiningSession);
            session->setMatchmakedGamePlayerCount(totalPlayerCount);
            session->setMatchmakedGameCapacity(createGameFinalizationSettings.getPlayerCapacity());
        }


        // nobody else to join to game, don't send to GM master
        TRACE_LOG( "[FinalizeCreateGameMatchmakingJob].joinSessionsToRemoteGameSession" << " Matchmaking session (" << mOwningMatchmakingSessionId << ") had no additional users to join to game (" << gameId << ").");
        return ERR_OK;
    }

    void FinalizeCreateGameMatchmakingJob::sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener )
    {

        Blaze::Search::MatchmakingSessionIdList::const_iterator iter = mJoinedMatchmakingSessionIdList.begin();
        Blaze::Search::MatchmakingSessionIdList::const_iterator end = mJoinedMatchmakingSessionIdList.end();
        for (; iter != end; ++iter)
        {
            mComponent.getMatchmaker()->dispatchMatchmakingConnectionVerifiedForSession(*iter, gameId, dispatchClientListener);
        }

        if (!dispatchClientListener)
        {
            if (mComponent.getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getContinueMatchingAfterValidationFailure())
            {
                returnSessionsToMatchmaker(mJoinedMatchmakingSessionIdList, true, GameManager::Matchmaker::Matchmaker::ReinsertionActionType::EVALUATE_SESSION);
            }
            else
            {
                terminateSessionsAsFailedQosValidation(mJoinedMatchmakingSessionIdList);
            }
        }
        else
        {
            if (mPinSubmissionPtr != nullptr)
            {
                gUserSessionManager->sendPINEvents(mPinSubmissionPtr);
            }
            cleanupSuccessfulSessions(gameId);
        }
    }

    bool FinalizeCreateGameMatchmakingJob::validateQos(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const
    {
        // in CG, either everybody passes, or everyone is booted to try again.
        // need to catch the failure case to put every user in the failed list
        if (validateQosForMatchmakingSessions(mJoinedMatchmakingSessionIdList, request, response))
        {
            return true;
        }
        else 
        {
            response.getFailedValidationList().insert(response.getFailedValidationList().end(), response.getPassedValidationList().begin(), response.getPassedValidationList().end());
            response.getPassedValidationList().clear();
            return false;
        }

    }

} // namespace Matchmaker
} // namespace Blaze
