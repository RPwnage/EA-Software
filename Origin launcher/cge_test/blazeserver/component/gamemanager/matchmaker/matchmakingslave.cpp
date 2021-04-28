/*! ************************************************************************************************/
/*!
\file matchmakingslave.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/connectionmanager.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakingslave.h"
#include "gamemanager/matchmaker/matchmakingsessionslave.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/rete/productionmanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief Constructor for the MatchmakingSlave
    ***************************************************************************************************/
    MatchmakingSlave::MatchmakingSlave(Search::SearchSlaveImpl &searchSlaveImpl)
        : IdlerUtil("[MatchmakingSlave]"),
        mUserSessionMMSessionSlaveMap(BlazeStlAllocator("mUserSessionMMSessionSlaveMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mSearchSlave(searchSlaveImpl),
        mRuleDefinitionCollection(nullptr),
        mTotalIdles(mSearchSlave.getMetricsCollection(), "fgIdles"),
        mLastIdleTimeMs(mSearchSlave.getMetricsCollection(), "lastFGIdleLengthMs", [this]() { return mLastIdleLength.getMillis(); }),
        mLastIdleTimeToIdlePeriodRatioPercent(mSearchSlave.getMetricsCollection(), "lastFGIdleLengthToIdlePeriodRatioPercent", [this]() { return mMatchmakingConfig.getServerConfig().getSlaveIdlePeriod().getMicroSeconds() == 0 ? 0 : (uint64_t)((100 * mLastIdleLength.getMicroSeconds()) / mMatchmakingConfig.getServerConfig().getSlaveIdlePeriod().getMicroSeconds()); }),
        mSessionSlaveMap(BlazeStlAllocator("SessionSlaveMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mMatchedSessionIdSet(BlazeStlAllocator("MatchedSessionIdSet", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mDirtyGameSet(BlazeStlAllocator("DirtyGameSet", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mGameIntrusiveNodeAllocator("MMAllocatorGameIntrusiveNode", sizeof(GameManager::MmSessionGameIntrusiveNode), GameManagerSlave::COMPONENT_MEMORY_GROUP),
        mMetrics(mSearchSlave.getMetricsCollection()),
        mActiveFGSessions(mSearchSlave.getMetricsCollection(), "activeFGSessions", [this]() { return mSessionSlaveMap.size(); }),
        mFGSessionMatches(mSearchSlave.getMetricsCollection(), "fgSessionMatches", [this]() { return mMatchedSessionIdSet.size(); }),
        mGarbageCollectableNodes(mSearchSlave.getMetricsCollection(), "garbageCollectableNodes", [this]() { return mSearchSlave.getReteNetwork().getNumGarbageCollectableNodes(); }),
        mPlayerGamesMapping(searchSlaveImpl),
        mXblAccountIdGamesMapping(searchSlaveImpl),
        mSessionJobQueue("Matchmaker::sessionJobQueue")
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Destructor for the MatchmakingSlave
    ***************************************************************************************************/
    MatchmakingSlave::~MatchmakingSlave()
    {
        delete mRuleDefinitionCollection;
        mUserSessionMMSessionSlaveMap.clear();
    }

    void MatchmakingSlave::onShutdown()
    {
        shutdownIdler();
        
        // Destroy all MM sessions on shutdown before games get destroyed. 
        SessionSlaveMap::iterator iter = mSessionSlaveMap.begin();
        SessionSlaveMap::iterator end = mSessionSlaveMap.end();
        for (; iter != end; ++iter)
        {
            MatchmakingSessionSlave *session = iter->second;
            // We need to remove as production to ensure empty lists.
            mSearchSlave.getReteNetwork().getProductionManager().removeProduction(*session);
            delete session;
        }

        mSessionJobQueue.join();
    }

    /*! ************************************************************************************************/
    /*! \brief validates Matchmaking's portion of the GameManager configuration.

        \return true if valid
    ***************************************************************************************************/
    bool MatchmakingSlave::validateConfig(const MatchmakingServerConfig& config, const MatchmakingServerConfig *referenceConfig, ConfigureValidationErrors& validationErrors) const
    {
        // Validate timers
        if (config.getStatusNotificationPeriod() < config.getIdlePeriod())
        {
            eastl::string msg;
            msg.sprintf("[MatchmakingSlave].validateConfig - Status notification period %" PRId64 " must be less than idle period %" PRId64 ".",
                config.getStatusNotificationPeriod().getMillis(), config.getIdlePeriod().getMillis());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        GameManager::Rete::StringTable stringTable;
        RuleDefinitionCollection collection(mSearchSlave.getMetricsCollection(), stringTable, mSearchSlave.getLogCategory(), false);
        collection.validateConfig(config, referenceConfig, validationErrors);

        return validationErrors.getErrorMessages().empty();
    }
    
    
    /*! ************************************************************************************************/
    /*! \brief Configure the matchmaking slave

        \param[in] configMap - Configuration map to pull our configuration from.
        \return true if configuration succeeded
    ***************************************************************************************************/
    bool MatchmakingSlave::configure(const MatchmakingServerConfig& config)
    {
        EA_ASSERT(mRuleDefinitionCollection == nullptr);
        mMatchmakingConfig.setConfig(config);
        
        mRuleDefinitionCollection = BLAZE_NEW RuleDefinitionCollection(mSearchSlave.getMetricsCollection(), mSearchSlave.getReteNetwork().getWMEManager().getStringTable(), mSearchSlave.getLogCategory(), false);
        if (!(mRuleDefinitionCollection->initRuleDefinitions(mMatchmakingConfig, config)))
        {
            return false; // stop blaze server
        }
        mRuleDefinitionCollection->initSubstringTrieRuleStringTables(mSearchSlave.getReteNetwork());

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Reconfigure the matchmaking slave

        \param[in] configMap - Configuration map to pull our configuration from.
        \return always returns true.  If reconfigure fails, we roll back to last configuration.
    ***************************************************************************************************/
    bool MatchmakingSlave::reconfigure(const MatchmakingServerConfig& config)
    {
        // Cancel anything holding a pointer to the mRuleDefinitionCollection (The current sessions)
        while (!mSessionSlaveMap.empty())
        {
            // iter will be erased from mSessionSlaveMap by cleanupSession below
            SessionSlaveMap::iterator iter = mSessionSlaveMap.begin();
            cleanupSession(iter->first, SESSION_TERMINATED);
        }

        delete mRuleDefinitionCollection;
        mRuleDefinitionCollection = nullptr;

        return configure(config);
    }

    /*! ************************************************************************************************/
    /*! \brief Notification that a user has logged out.
    
        \param[in] userSession - the user that is logging out.
    ***************************************************************************************************/
    void MatchmakingSlave::onUserSessionExtinction(UserSessionId userSessionId)
    {
        UserSessionMMSessionSlaveMap::iterator iter = mUserSessionMMSessionSlaveMap.find(userSessionId);
        if (iter != mUserSessionMMSessionSlaveMap.end())
        {
            MatchmakingSessionSlaveIdSet& sessionSet = iter->second;
            MatchmakingSessionSlaveIdSet::const_iterator sessIter = sessionSet.begin();
            MatchmakingSessionSlaveIdSet::const_iterator sessEnd = sessionSet.end();
            for (; sessIter != sessEnd; ++sessIter)
            {
                Rete::ProductionId sessionId = *sessIter;

                SessionSlaveMap::iterator mapIter = mSessionSlaveMap.find(sessionId);
                if (mapIter != mSessionSlaveMap.end())
                {
                     TRACE_LOG("[MatchmakingSlave] Removing dying user session(" << userSessionId << ") matchmaking session(" << sessionId << ").");
                    // don't need to call removeSessionForUser() because we're erasing the entry in the mUserSessionMMSessionSlaveMap
                    destroySession(mapIter);
                    mIdleMetrics.mLoggedOutSessionsAtIdleStart++;
                }
                else
                {
                    WARN_LOG("[MatchmakingSlave] Unable to find matchmaking session(" << sessionId << ") which was being tracked by user session(" << userSessionId << ")!");
                }
            }
            mUserSessionMMSessionSlaveMap.erase(userSessionId);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Adds an entry for quick lookup, associating the mm session to the user
    ***************************************************************************************************/
    void MatchmakingSlave::addSessionForUser(UserSessionId userSessionId, MatchmakingSessionSlave *session)
    {
        UserSessionMMSessionSlaveMap::iterator iter = mUserSessionMMSessionSlaveMap.find(userSessionId);
        if (iter != mUserSessionMMSessionSlaveMap.end())
        {
            MatchmakingSessionSlaveIdSet& sessionSet = iter->second;
            sessionSet.insert(session->getProductionId());
        }
        else 
        {
            // User is new to the map.
            mUserSessionMMSessionSlaveMap[userSessionId].insert(session->getProductionId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief removes an entry for quick lookup, associating the game to the user
    ***************************************************************************************************/
    void MatchmakingSlave::removeSessionForUser(UserSessionId userSessionId, MatchmakingSessionSlave *session)
    {
        UserSessionMMSessionSlaveMap::iterator iter = mUserSessionMMSessionSlaveMap.find(userSessionId);
        if (iter != mUserSessionMMSessionSlaveMap.end())
        {
            // Remove existing game from the GameSet
            MatchmakingSessionSlaveIdSet& sessionSet = iter->second;
            sessionSet.erase(session->getProductionId());

            if (iter->second.empty())
            {
                // GameSet is empty, remove the usersession entry from the map
                mUserSessionMMSessionSlaveMap.erase(userSessionId);
                TRACE_LOG("[MatchmakingSlave] Removed user(" << userSessionId << ") from matchmaking session map, has no more sessions.");
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Creates a new matchmaking slave session from the given start matchmaking request

        \param[in] req - the start matchmaking request sent from the client.
        \return a reference to the new matchmakingSessionSlave.
    ***************************************************************************************************/
    MatchmakingSessionSlave* MatchmakingSlave::createMatchmakingSession(
        MatchmakingSessionId sessionId,
        const StartMatchmakingInternalRequest& req, 
        BlazeRpcError& err, 
        MatchmakingCriteriaError& criteriaError,
        GameManager::MatchmakingSubsessionDiagnostics& diagnostics,
        InstanceId originatingInstanceId)
    {
        MatchmakingSessionSlave *sessionSlave = BLAZE_NEW MatchmakingSessionSlave(static_cast <MatchmakingSlave &>(*this), sessionId, originatingInstanceId);


        err = sessionSlave->initialize(req, mSearchSlave.getDoGameProtocolVersionStringCheck(),
            criteriaError, mDedicatedServerOverrideMap);
        if (err != ERR_OK)
        {
            delete sessionSlave;
            TRACE_LOG("[MatchmakingSlave] Failed to initialize matchmaking slave session, err(" << (ErrorHelp::getErrorName(err)) << ").");
            return nullptr;
        }

        // prep the player lists before we init RETE productions
        getXblAccountIdGamesMapping().processWatcherInitialMatches();
        getPlayerGamesMapping().processWatcherInitialMatches();

        TRACE_LOG("[MatchmakingSlave] Created new matchmaking session(" << sessionSlave->getProductionId() << ").");
        addSessionForUser(sessionSlave->getOwnerUserSessionId(), sessionSlave);

        // mSessionSlaveMap now holds pointer to created memory.
        mSessionSlaveMap.insert(eastl::make_pair(sessionSlave->getProductionId(), sessionSlave));
        
        // Even though the session may be delayed, we schedule the idle immediately, since other (later) sessions may want to start first. 
        scheduleNextIdle(TimeValue::getTimeOfDay(), mMatchmakingConfig.getServerConfig().getSlaveIdlePeriod());

        // if we have privileged game, attempt to join directly, bypassing MM/RETE all together
        if (!sessionSlave->checkForGameOverride() && !sessionSlave->wasStartedDelayed())
        {
            sessionSlave->initializeReteProductions(mSearchSlave.getReteNetwork());

            SPAM_LOG("[MatchmakingSlave] OnTokenAdded Session(" << sessionSlave->getProductionId() << ") metric quick("\
                     << sessionSlave->mOtaQuickOutCount << ") "
                     << sessionSlave->mOtaQuickOut.getMicroSeconds() <<" us, full("
                     << sessionSlave->mOtaFullCount << ") "
                     << sessionSlave->mOtaFull.getMicroSeconds() << " us, onTokenRemoved("
                     << sessionSlave->mOtrCount << ")");
        }
        
        if (mMatchmakingConfig.getServerConfig().getTrackDiagnostics() && !sessionSlave->checkForGameOverride())
        {
            // after setting session's conditions, tally match diagnostics to return to MM slave.
            // For start delayed sessions which haven't set these up yet, for simplicity, just base on a current snapshot
            // (MM will checks session actually started, before adding to system wise totals, later)
            sessionSlave->tallySessionDiagnosticsFG(diagnostics, mSearchSlave.getDiagnosticsHelper());
        }

        mMetrics.mTotalSessionsStarted.increment();
        ++mIdleMetrics.mNewSessionsAtIdleStart;
        
        return sessionSlave;
    }

    /*! ************************************************************************************************/
    /*! \brief Destroys the provided session.
    
        \param[in] iter - an iterator into the map of all slave sessions to be deleted
    ***************************************************************************************************/
    MatchmakingSlave::SessionSlaveMap::iterator& MatchmakingSlave::destroySession(SessionSlaveMap::iterator& iter)
    {
        MatchmakingSessionSlave *session = iter->second;
        TRACE_LOG("[MatchmakingSlave] Destroying session(" << session->getProductionId() << ")");

        // Remove the RETE production from the RETE network.
        mSearchSlave.getReteNetwork().getProductionManager().removeProduction(*session);

        // Remove any matching items
        removeFromMatchingSessions(*session);

        iter = mSessionSlaveMap.erase(iter);
        delete session;

        ++mIdleMetrics.mFinalizedSessionsAtIdleStart;
        return iter;
    }


    void MatchmakingSlave::onUpdate(const Search::GameSessionSearchSlave& game)
    {
        //Mark the game as dirty
        mDirtyGameSet.insert(game.getGameId());

        scheduleNextIdle(TimeValue::getTimeOfDay(), mMatchmakingConfig.getServerConfig().getSlaveIdlePeriod());
    }


    /*! ************************************************************************************************/
    /*! \brief Loops through all currently known matchmaking slave sessions, and updates their session ages
        triggering decay of our rule criteria.

        \param[in] now - the time of the start of the ide, that is triggering the update.
    ***************************************************************************************************/
    void MatchmakingSlave::updateSessionAges(const TimeValue& now, const TimeValue& prevIdle)
    {
        SessionSlaveMap::const_iterator iter = mSessionSlaveMap.begin();
        SessionSlaveMap::const_iterator end = mSessionSlaveMap.end();

        for (; iter != end; ++iter)
        {
            MatchmakingSessionSlave *sessionSlave = iter->second;
            if (EA_UNLIKELY(sessionSlave == nullptr))
            {
                TRACE_LOG("[MatchmakingSlave] null session(" << iter->first << ") found while updating session ages.");
                continue;
            }

            LogContextOverride logAudit(sessionSlave->getOwnerSessionId());

            // If we delayed the start of the session, start it up here:
            if (sessionSlave->wasStartedDelayed() && 
                prevIdle < sessionSlave->getStartTime() && sessionSlave->getStartTime() < now &&
                !sessionSlave->checkForGameOverride())
            {
                // (Always check for checkForGameOverride before setting the ReteProductions)
                sessionSlave->initializeReteProductions(mSearchSlave.getReteNetwork());
            }

            sessionSlave->updateSessionAge(now);
            UpdateThresholdStatus updateStatus = sessionSlave->updateFindGameSession();
            if (!sessionSlave->checkForGameOverride() && (updateStatus.nonReteRulesResult > NO_RULE_CHANGE || updateStatus.reteRulesResult > NO_RULE_CHANGE))
            {
                // The sessions rules have changed, update or RETE tree accordingly.
                // NOTE: We are simply removing the session as a production listener
                // to any production nodes, and then re-adding it, with the new rules
                // This can be more expensive if we have any rules that have production
                // nodes that early out, instead of being endpoints all the way down the tree.
                
                if (updateStatus.reteRulesResult > NO_RULE_CHANGE)
                {
                    sessionSlave->initializeReteProductions(mSearchSlave.getReteNetwork());
                }

                SPAM_LOG("[MatchmakingSlave] Decaying session(" << sessionSlave->getProductionId() << ") failedFGRequests:"
                    << sessionSlave->getNumFailedFoundGameRequests() << ", OnTokenAdded dropped matches:" << sessionSlave->mOtaQuickOutCount 
                    << " (" << sessionSlave->mOtaQuickOut.getMicroSeconds() << " us), OnTokenAdded tracked matches:" << sessionSlave->mOtaFullCount 
                    <<" (" << sessionSlave->mOtaFull.getMicroSeconds() << " us), OnTokenRemoved matches:" << sessionSlave->mOtrCount << ".");

                // MM_TODO: MM_PERFORMANCE: NOTE:
                // we should only re-eval for rules that updated this cycle
                if(sessionSlave->hasNonReteRules() && updateStatus.nonReteRulesResult > NO_RULE_CHANGE)
                {
                    sessionSlave->reevaluateNonReteRules();
                }

                // NOTE: that we have not removed any of our sessions matches.  This is because
                // we currently only expand the RETE tree during decay.  If we ever intend to
                // contract the RETE tree during decay, we would need to clean up any matches.
            }


            if (sessionSlave->isAboutToExpire() ||
                (sessionSlave->getStartMatchmakingRequest().getSessionData().getDebugFreezeDecay() && (TimeValue::getTimeOfDay() > sessionSlave->getStartTime())))
            {
                // If the session is about to time out, we ensure that any debug games are matched prior to timeout.
                // If the session is decay-frozen and started, also just match now. (Do it before timeout to ensure results can be sent before session destroyed).
                sessionSlave->matchRequiredDebugGames();
            }

            // Session time out
            // We will rely on the master to time use out if we have a corresponding master session.
            if (sessionSlave->isExpired())
            {
                queueSessionForTimeout(*sessionSlave);
            }
        }
    }
    

    /*! ************************************************************************************************/
    /*! \brief Update the best fitScore found by the given matchmaking session slave

        \param[in] fitScore - the fitScore for the best match.
        \param[in] session - the matchmakingSessionSlave updating its best fitScore
    ***************************************************************************************************/
    void MatchmakingSlave::addToMatchingSessions(MatchmakingSessionSlave& session)
    {
        if (mMatchedSessionIdSet.insert(session.getProductionId()).second == true)
        {
            // add to the unique-item queue
            addToMatchingSessionsQueue(session.getProductionId());
        }
    }

    void MatchmakingSlave::addToMatchingSessionsQueue(Rete::ProductionId id)
    {
        mSessionJobQueue.queueFiberJob<MatchmakingSlave, MatchmakingSessionId>(this, &MatchmakingSlave::processMatchmakingSession, id, "Matchmaker::processMatchmakingSession");
    }

    /*! ************************************************************************************************/
    /*! \brief Removes a fit score match from our list of matches.  Used when a session slave has no
        more matching games (after having matched previously).

        \param[in] session - the matchmakingSessionSlave removing its match.
    ***************************************************************************************************/
    void MatchmakingSlave::removeFromMatchingSessions(MatchmakingSessionSlave& session)
    {
        mMatchedSessionIdSet.erase(session.getProductionId());

        // side: note we let the process matchmaking sessions scan remove from mMatchedSessionIdQueue, for efficiency.
    }



    /*! ************************************************************************************************/
    /*! \brief Processes known matchmakingSessionSlaves attempting to join them to their found games.
            This is intended to be called from an idle, so we can process a configurable number of 
            games per idle.

        \returns true if there are still more matchmakingSessionSlaves to be processed.
    ***************************************************************************************************/
    void MatchmakingSlave::processMatchmakingSession(MatchmakingSessionId sessionId)
    {

        // Find the session.
        // If session was removed from being a match, skip it.
        if (mMatchedSessionIdSet.find(sessionId) == mMatchedSessionIdSet.end())
        {
            TRACE_LOG("[MatchmakingSlave] Session(" << sessionId << ") was already removed from matches, skipping session.");
            return;
        }

        // TODO: this mSessionSlaveMap is redundant remove this for efficiency.
        SessionSlaveMap::iterator sessIter = mSessionSlaveMap.find(sessionId);

        // Can't find the session.
        if (sessIter == mSessionSlaveMap.end())
        {
            WARN_LOG("[MatchmakingSlave] Unable to find session(" << sessionId << ") that told the slave it had a match, skipping session.");
            return;
        }


        MatchmakingSessionSlave *session = sessIter->second;
        if (session == nullptr)
        {
            // This should never ever happen.
            ERR_LOG("[MatchmakingSlave] Null session(" << sessionId << ") was pulled off the slave map, skipping session.");
            return;
        }

        LogContextOverride logAudit(session->getOwnerSessionId());


        // force match any required games, note, this only happens if the session had other actual matches
        session->matchRequiredDebugGames();
        removeFromMatchingSessions(*session);
        // NOTE: Can't use GetInstanceIdFromInstanceKey64(sessionId) because packer based sessions are
        // sharded by sliver and owned by packer master meanwhile notifications need to route back to packer slave.
        auto sessionInstanceId = session->getOriginatingInstanceId();
        Search::NotifyFindGameFinalizationUpdate update;
        update.setUpdateSourceInstanceId(gController->getInstanceId());
        session->peekBestGames(update.getFoundGames());
        session->popBestGames();
        SlaveSessionPtr slaveSession = gController->getConnectionManager().getSlaveSessionByInstanceId(sessionInstanceId);
        mSearchSlave.sendFindGameFinalizationUpdateNotification(slaveSession.get(), update);
    }

    uint64_t MatchmakingSlave::processDirtyGames()
    {
        //Update any sessions with dirty games
        uint64_t totalEvals = 0;
        for (GameIdSet::iterator itr = mDirtyGameSet.begin(), end = mDirtyGameSet.end(); itr != end; ++itr)
        {
            Search::GameSessionSearchSlave* game = mSearchSlave.getGameSessionSlave(*itr);
            if (game != nullptr && game->getMmSessionIntrusiveListSize() != 0)
            {
                //Now update each list that holds a ref to the game
                for (MmSessionIntrusiveList::iterator setItr = game->getMmSessionIntrusiveList().begin(),
                    setEnd = game->getMmSessionIntrusiveList().end(); setItr != setEnd; ++setItr)
                {
                    MmSessionGameIntrusiveNode &n = static_cast<MmSessionGameIntrusiveNode &>(*setItr);
                    LogContextOverride logAudit(n.mMatchmakingSessionSlave.getOwnerSessionId());
                    n.mMatchmakingSessionSlave.updateNonReteRules(*game);
                }

                totalEvals += game->getMmSessionIntrusiveListSize();
            }
        }
        mDirtyGameSet.clear();
        return totalEvals;
    }

    /*! ************************************************************************************************/
    /*! \brief Idle.  Processes matchmakingSessionSlave's that have matched games.
    ***************************************************************************************************/
    IdlerUtil::IdleResult MatchmakingSlave::idle()
    {
        mTotalIdles.increment();
        TimeValue idleStartTime = TimeValue::getTimeOfDay();

        TRACE_LOG("[MatchmakingSlave] Entering idle " << mTotalIdles.getTotal() << " at time " << idleStartTime.getMillis());

        // Decay
        updateSessionAges(idleStartTime, mLastIdleStartTime);

        TimeValue idleTime1 = TimeValue::getTimeOfDay();

        //process dirty games for non-rete matches
        uint64_t dirtyEvals = processDirtyGames();

        // Log idle
        mLastIdleStartTime = idleStartTime;
        TimeValue endTime = TimeValue::getTimeOfDay();
        mLastIdleLength = endTime - idleStartTime;
        TimeValue updateSessionAgesDuration = idleTime1 - idleStartTime;
        TimeValue processDirtyGamesDuration = endTime - idleTime1;

        TRACE_LOG("[MatchmakingSlave] Exiting idle " << mTotalIdles.getTotal() << "; idle duration " << mLastIdleLength.getMillis() << " ms (update " 
                  << updateSessionAgesDuration.getMillis() << " ms, processDG " << processDirtyGamesDuration.getMillis() << " ms), '" 
                  << mSessionSlaveMap.size() << "' active sess at idle end, '" << mIdleMetrics.mNewSessionsAtIdleStart << "' new sess('" 
                  << mIdleMetrics.mImmediateFinalizeAttemptsAtIdleStart << "' immediately matched), '" << mMatchedSessionIdSet.size() << "' sess matches, '"
                  << mIdleMetrics.mFinalizedSessionsAtIdleStart << "' sess finalized('" << mIdleMetrics.mJoinedSessionsAtIdleStart << "' joined game '" << mIdleMetrics.mLoggedOutSessionsAtIdleStart << "' logged out, '" << mIdleMetrics.mTimedOutSessionsAtIdleStart << "' timed out. ), '" 
                  << mIdleMetrics.mWorkingMemoryUpdatesAtIdle << "' wmeUpdates '" << dirtyEvals << "' dirty evals, '" 
                  << mSearchSlave.getReplicatedGamesCount() << "' total games");

        // Clear idle metrics
        mIdleMetrics.reset();

        if(!mSessionSlaveMap.empty() || !mDirtyGameSet.empty())
        {
            return SCHEDULE_IDLE_NORMALLY;
        }
        else
        {
            return NO_NEW_IDLE;
        }
    }

    void MatchmakingSlave::queueSessionForTimeout(MatchmakingSessionSlave& session)
    { 
        TRACE_LOG("[MatchmakingSlave] Session(" << session.getProductionId() << ") has expired its timeout at age " << session.getSessionAgeSeconds() << ".");
        mSessionJobQueue.queueFiberJob<MatchmakingSlave, MatchmakingSessionId, MatchmakingResult>(this, &MatchmakingSlave::cleanupSession, session.getProductionId(), SESSION_TIMED_OUT, "Matchmaker::cleanupSession");
        mMetrics.mTotalSessionsTimedOut.increment();
    }


    void MatchmakingSlave::cleanupSession(MatchmakingSessionId sessionId, MatchmakingResult result)
    {
        SessionSlaveMap::iterator mapIter = mSessionSlaveMap.find(sessionId);
        if (mapIter != mSessionSlaveMap.end())
        {
            MatchmakingSessionSlave *session = mapIter->second;

            session->submitMatchmakingFailedEvent(result);
            removeSessionForUser(session->getOwnerUserSessionId(), session);
            destroySession(mapIter);
        }
    }


    MatchmakingSessionSlave* MatchmakingSlave::findMatchmakingSession(Rete::ProductionId id)
    {
        SessionSlaveMap::iterator itr = mSessionSlaveMap.find(id);

        if (itr != mSessionSlaveMap.end())
        {
            return itr->second;
        }

        return nullptr;
    }

    void MatchmakingSlave::cancelMatchmakingSession(Rete::ProductionId id, GameManager::MatchmakingResult result)
    {
        SessionSlaveMap::iterator iter = mSessionSlaveMap.find(id);
        if (iter != mSessionSlaveMap.end())
        {
            MatchmakingSessionSlave *session = iter->second;
            LogContextOverride logAudit(session->getOwnerSessionId());

            bool success =  (result == GameManager::SUCCESS_CREATED_GAME) || 
                            (result == GameManager::SUCCESS_JOINED_NEW_GAME) ||
                            (result == GameManager::SUCCESS_JOINED_EXISTING_GAME) ||
                            (result == GameManager::SUCCESS_PSEUDO_CREATE_GAME) ||
                            (result == GameManager::SUCCESS_PSEUDO_FIND_GAME);

            if (success)
            {
                mMetrics.mTotalSessionSuccessDurationMs.increment((uint64_t)session->getDuration().getMillis());
            }
            else
            {
                mMetrics.mTotalSessionsCanceled.increment();
                mMetrics.mTotalSessionCanceledDurationMs.increment((uint64_t)session->getDuration().getMillis());
                session->submitMatchmakingFailedEvent(SESSION_CANCELED);
            }

            removeSessionForUser(session->getOwnerUserSessionId(), session);
            destroySession(iter);
        }
    }

    void MatchmakingSlave::deleteGameIntrusiveNode(Blaze::GameManager::MmSessionGameIntrusiveNode *node)
    {
        node->~MmSessionGameIntrusiveNode();
        GameManager::MmSessionGameIntrusiveNode::operator delete (node, mGameIntrusiveNodeAllocator);
    }

    Blaze::GameManager::MmSessionGameIntrusiveNode* MatchmakingSlave::createGameInstrusiveNode(Search::GameSessionSearchSlave& game, MatchmakingSessionSlave& sessionSlave)
    {
        return new (mGameIntrusiveNodeAllocator) Blaze::GameManager::MmSessionGameIntrusiveNode(game, sessionSlave);
    }

    /*! ************************************************************************************************/
    /*! \brief fill out the matchmaking's component status tdf.
    ***************************************************************************************************/
    void MatchmakingSlave::getStatusInfo(ComponentStatus& status) const
    {
        Blaze::ComponentStatus::InfoMap& parentStatusMap = status.getInfo();
        char8_t buf[64];

        // Counters
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalSessionsStarted.getTotal());
        parentStatusMap["SSTotalFGSessionsStarted"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalSessionsCanceled.getTotal());
        parentStatusMap["SSTotalFGSessionsCanceled"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalSessionsTimedOut.getTotal());
        parentStatusMap["SSTotalFGSessionsTimeout"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mTotalIdles.getTotal());
        parentStatusMap["SSTotalFGIdles"] = buf;

        // Gauges
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mLastIdleTimeMs.get());
        parentStatusMap["SSGaugeLastFGIdleLengthMs"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mLastIdleTimeToIdlePeriodRatioPercent.get());
        parentStatusMap["SSGaugeLastFGIdleLengthToIdlePeriodRatioPercent"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mActiveFGSessions.get());
        parentStatusMap["SSGaugeActiveFGSessions"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mFGSessionMatches.get());
        parentStatusMap["SSGaugeFGSessionMatches"] = buf;

        // Timings
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalSessionSuccessDurationMs.getTotal());
        parentStatusMap["SSTotalFGSuccessDurationMs"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalSessionCanceledDurationMs.getTotal());
        parentStatusMap["SSTotalFGCanceledDurationMs"] = buf;

        mGameIntrusiveNodeAllocator.logHealthChecks(parentStatusMap);

        // Rete Allocators
        const Rete::ReteNetwork& reteNetwork = mSearchSlave.getReteNetwork();
        reteNetwork.getWMEManager().getWMEFactory().getWMEAllocator().logHealthChecks(parentStatusMap);
        reteNetwork.getJoinNodeFactory().getJoinNodeAllocator().logHealthChecks(parentStatusMap);
        reteNetwork.getProductionNodeFactory().getProductionNodeAllocator().logHealthChecks(parentStatusMap);
        reteNetwork.getProductionTokenFactory().getProductionTokenAllocator().logHealthChecks(parentStatusMap);
        reteNetwork.getProductionBuildInfoFactory().getProductionBuildInfoAllocator().logHealthChecks(parentStatusMap);

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mGarbageCollectableNodes.get());
        parentStatusMap["SSGarbageCollectableNodes"] = buf;

        if (!mRuleDefinitionCollection)
            return;

        eastl::string ruleNameStr;

        RuleDefinitionList::const_iterator ruleIter = mRuleDefinitionCollection->getRuleDefinitionList().begin();
        RuleDefinitionList::const_iterator ruleEnd = mRuleDefinitionCollection->getRuleDefinitionList().end();
        for (; ruleIter != ruleEnd; ++ruleIter)
        {
            const RuleDefinition* rule = *ruleIter;
            if (rule != nullptr)
            {
                const char8_t* ruleName = rule->getName();
                if (ruleName != nullptr)
                {
                    ruleNameStr = "SSRuleSalience_";
                    ruleNameStr += ruleName;
                    blaze_snzprintf(buf,sizeof(buf),"%" PRIu32, rule->getSalience());
                    parentStatusMap[ruleNameStr.c_str()] = buf;
                }

            }
        }

        const char8_t* metricName = "SSGaugeRuleJoinNodes";
        reteNetwork.getRuleJoinCount().iterate([&parentStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, metricName, tagList, value.get()); });

        metricName = "SSTotalRuleJoinNodes";
        reteNetwork.getRuleTotalJoinCount().iterate([&parentStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, metricName, tagList, value.getTotal()); });

        metricName = "SSTotalRuleUpserts";
        reteNetwork.getRuleTotalUpserts().iterate([&parentStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, metricName, tagList, value.getTotal()); });

        metricName = "SSMaxRuleJoinNodeChildren";
        reteNetwork.getRuleMaxChildJoinCount().iterate([&parentStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, metricName, tagList, value.get()); });

        metricName = "MMTotalRuleUses";
        mRuleDefinitionCollection->getRuleDefinitionTotalUseCount().iterate([&parentStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, metricName, tagList, value.get()); });

        // Since there's only 1 metrics table, we can't get the values divided up per-rule:
        const Rete::StringTable& table = mRuleDefinitionCollection->getStringTable();
        {
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, table.getStringHashMapSize());
            parentStatusMap["SSStringTableSizeGauge"] = buf;
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, table.getZombieCountGauge());
            parentStatusMap["SSStringTableZombieCountGauge"] = buf;
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, table.getCollisionCountTotal());
            parentStatusMap["SSStringTableCollisionCountTotal"] = buf;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief dumps debug info to a file
    ***************************************************************************************************/
    void MatchmakingSlave::dumpDebugInfo()
    {
        mSearchSlave.getReteNetwork().dumpBetaNetwork();
    }

    /*! ************************************************************************************************/
    /*! \brief adds entry to DedicatedServerOverrideMap
    ***************************************************************************************************/
    void MatchmakingSlave::dedicatedServerOverride(BlazeId blazeId, GameId gameId)
    {
        DedicatedServerOverrideMap::iterator overrideIter = mDedicatedServerOverrideMap.find(blazeId);

        // if the game id is INVALID_GAME_ID and they are already in the map, clear the value.
        if (gameId == GameManager::INVALID_GAME_ID)
        {
            if (overrideIter != mDedicatedServerOverrideMap.end())
            {
                mDedicatedServerOverrideMap.erase(overrideIter);
                INFO_LOG("MatchmakingSlave.dedicatedServerOverride() Player(" << blazeId << ") cleared from dedicated server override matchmaking.");
            }
            else
            {
                INFO_LOG("MatchmakingSlave.dedicatedServerOverride() Player(" << blazeId << ") was not in the list, but was clearing the game id, so we ignored it. (dedicated server override matchmaking)");
            }
            return;
        }
        
        mDedicatedServerOverrideMap[blazeId] = gameId;
        INFO_LOG("MatchmakingSlave.dedicatedServerOverride() Player(" << blazeId << ") will now matchmake into game(" << gameId << ").");
    }

    void MatchmakingSlave::fillServersOverride(const GameManager::GameIdList& fillServersOverrideList)
    {
        fillServersOverrideList.copyInto(mFillServersOverrideList);
        INFO_LOG("MatchmakingSlave.fillServersOverride() List of (" << mFillServersOverrideList.size() << ") game ids will now be priority filled by matchmaking.");
    }

    /*! ************************************************************************************************/
    /*! \brief notify all the player's watchers about the action
        \param[in] blazeId the user joining the game
    ***************************************************************************************************/
    void MatchmakingSlave::onUserJoinGame(const BlazeId playerId, const PlatformInfo& platformInfo, const Search::GameSessionSearchSlave& gameSession)
    {
        getPlayerGamesMapping().updateWatchersOnUserAction(playerId, platformInfo.getEaIds().getNucleusAccountId(), gameSession, true);
        if ((platformInfo.getClientPlatform() == xone) || (platformInfo.getClientPlatform() == xbsx))
            getXblAccountIdGamesMapping().updateWatchersOnUserAction(platformInfo.getExternalIds().getXblAccountId(), gameSession, true);
    }
    /*! ************************************************************************************************/
    /*! \brief notify all the player's watchers about the action
        \param[in] blazeId the user leaving the game
    ***************************************************************************************************/
    void MatchmakingSlave::onUserLeaveGame(const BlazeId playerId, const PlatformInfo& platformInfo, const Search::GameSessionSearchSlave& gameSession)
    {
        getPlayerGamesMapping().updateWatchersOnUserAction(playerId, platformInfo.getEaIds().getNucleusAccountId(),gameSession,false);
        if ((platformInfo.getClientPlatform() == xone) || (platformInfo.getClientPlatform() == xbsx))
            getXblAccountIdGamesMapping().updateWatchersOnUserAction(platformInfo.getExternalIds().getXblAccountId(), gameSession, false);
    }
}
}
}

