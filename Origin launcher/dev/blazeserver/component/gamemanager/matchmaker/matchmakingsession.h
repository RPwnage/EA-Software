/*! ************************************************************************************************/
/*!
    \file   matchmakingsession.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_SESSION
#define BLAZE_MATCHMAKING_SESSION

#include "EATDF/time.h"
#include "framework/util/entrycriteria.h"
#include "framework/userset/userset.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/attributes.h"
#include "framework/system/iterator.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/matchmaker/matchlist.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucket.h"
#include "gamemanager/searchcomponent/findgameshardingbroker.h"
#include "gamemanager/rolescommoninfo.h" // for RoleSizeMap
#include "gamemanager/roleentrycriteriaevaluator.h"
#include "gamemanager/tdf/matchmakingmetrics_server.h"
#include "gamemanager/tdf/matchmaker.h" // for DebugCreateGameResults

#include "EASTL/intrusive_list.h"
#include "EASTL/intrusive_hash_map.h"

namespace Blaze
{

    namespace Matchmaker
    {
        class MatchmakerSlaveImpl;
    }

namespace GameManager
{

namespace Matchmaker
{
    class  Matchmaker;
    class  MatchmakingConfig;
    class CreateGameFinalizationSettings;
    class CreateGameFinalizationTeamInfo;
    class CreateGameFinalizationGameTeamCompositionsInfo;

    typedef Blaze::GameManager::PlayerToGameMap DedicatedServerOverrideMap;

    struct FullUserSessionsMapNode : eastl::intrusive_hash_node {};
    typedef eastl::intrusive_hash_multimap<Blaze::UserSessionId, FullUserSessionsMapNode, 4099> FullUserSessionsMap;
    typedef eastl::pair<FullUserSessionsMap::iterator, FullUserSessionsMap::iterator> FullUserSessionMapEqualRangePair; // for use in map.equal_range

    typedef eastl::vector<MatchmakingSessionId> MatchedSessionsList;
    typedef eastl::hash_map<MatchmakingSessionId, MatchmakingSession*> SessionFinalizationMap;

    typedef GameManager::UserSessionGameSetupReasonMap UserSessionSetupReasonMap;

    typedef eastl::map<UserSessionId, NotifyServerMatchmakingReservedExternalPlayers> NotifyMatchmakingReservedExternalPlayersMap;

    struct FullSessionThan
    {
        bool operator()(const MatchmakingSession* a, const MatchmakingSession* b) const;
    };
    typedef eastl::set<MatchmakingSession*, FullSessionThan> FullMatchmakingSessionSet;

    /*! ************************************************************************************************/
    /*!
        \brief Helper class for tracking the NAT status currently being finalized against.
    *************************************************************************************************/
    struct FinalizedNatStatus
    {
    public:
        FinalizedNatStatus() : hasStrictNat(false), hasModerateNat(false) {}
        FinalizedNatStatus(const FinalizedNatStatus& rhs) : hasStrictNat(rhs.hasStrictNat), hasModerateNat(rhs.hasModerateNat) {}
        FinalizedNatStatus& operator=(const FinalizedNatStatus& rhs)
        { 
            hasStrictNat = (rhs.hasStrictNat);
            hasModerateNat = (rhs.hasModerateNat);
            return *this;
        }

        bool hasStrictNat;
        bool hasModerateNat;
    };

    /*! ************************************************************************************************/
    /*!
        \brief Matchmaking Sessions are identified by id, and contain criteria & state info.
    *************************************************************************************************/
    class MatchmakingSession : public EntryCriteriaEvaluator
    {
    NON_COPYABLE(MatchmakingSession)

    public:

        /*! ************************************************************************************************/
        /*! \brief Helper class that tracks all the users associated with a matchmaking session.
        *************************************************************************************************/
        struct MemberInfoListNode : public eastl::intrusive_list_node {};
        struct MemberInfo : public MemberInfoListNode, public FullUserSessionsMapNode
        {
            MemberInfo(MatchmakingSession &mmSession, const GameManager::UserSessionInfo& sessionInfo, bool isOptional)
                : mUserSessionId(sessionInfo.getSessionId()), mMMSession(mmSession),
                mIsOptionalPlayer(isOptional)
            {
                sessionInfo.copyInto(mUserSessionInfo);
            }

            bool isMMSessionOwner() const { return mUserSessionInfo.getSessionId() == mMMSession.getOwnerUserSessionId();}
            bool isPotentialHost() const;

            UserSessionId mUserSessionId; 
            MatchmakingSession &mMMSession;
            GameManager::UserSessionInfo mUserSessionInfo;
            bool mIsOptionalPlayer;
            
            NON_COPYABLE(MemberInfo);
        };
        typedef eastl::intrusive_list<MemberInfoListNode> MemberInfoList;

        struct SystemSnapshotAtSessionStart
        {
            uint64_t mTotalSessionsInScenario = 0;
            uint64_t mTotalSessionsInSilo = 0;
        };

    public:

        /*! ************************************************************************************************/
        /*! \brief Sessions must be initialized after creation; 0 is an invalid session id.

            \param[in]  master - The matchmaker slave for the session.
            \param[in]  matchmaker - The matchmaker that owns this session.
            \param[in]  id - The sessionId to use.  Zero is invalid.
            \param[in]  userSession  - user session which is the owner the MM session (start the MM session)
        *************************************************************************************************/
        MatchmakingSession(Blaze::Matchmaker::MatchmakerSlaveImpl &matchmakerSlave, Matchmaker &matchmaker, MatchmakingSessionId id, MatchmakingScenarioId scenarioId);


        //! destructor
        ~MatchmakingSession() override;

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the matchmaking session from a StartMatchmakingSessionRequest.  Returns true on success.

            \param[in] request - The StartMatchmakingInternalRequest (from the slave/client)
            \param[in] matchmakingSupplimentialData - needed to lookup the rule definitions and other data needed for matchmaking
            \param[out] err - We fill out the errMessage field if we encounter an error
            \return true on success; false otherwise.
        *************************************************************************************************/
        bool initialize(const StartMatchmakingInternalRequest &request, MatchmakingCriteriaError &err, const DedicatedServerOverrideMap &dedicatedServerOverrideMap);

        /*! ************************************************************************************************/
        /*!
            \brief takes required actions to prepare a session for reactivation after failing to finalize
        *************************************************************************************************/
        void activateReinsertedSession();

        void takeSystemSnaphot();

        /*! ************************************************************************************************/
        /*!
            \brief returns the sessionId for this session
            \return returns the sessionId for this session
        *************************************************************************************************/
        MatchmakingSessionId getMMSessionId() const { return mMMSessionId; }
        MatchmakingScenarioId getMMScenarioId() const { return mOwningScenarioId; }

        uint32_t getSessionAgeSeconds() const { return mSessionAgeSeconds; }
        uint32_t getSessionTimeoutSeconds() const { return mSessionTimeoutSeconds; }
        uint64_t getTotalSessionDurationSeconds() const { return mTotalSessionDuration.getSec(); }
        TimeValue getSessionExpirationTime() const { return mSessionStartTime + mTotalSessionDuration; }
        TimeValue getTotalSessionDuration() const { return mTotalSessionDuration; }

        TimeValue getEstimatedTimeToMatch() const
        {
            return mEstimatedTimeToMatch;
        }

        uint64_t getTotalGameplayUsersAtSessionStart() const
        {
            return mTotalGameplayUsersAtSessionStart;
        }

        uint64_t getTotalUsersInGameAtSessionStart() const
        {
            return mTotalUsersInGameAtSessionStart;
        }

        uint64_t getTotalUsersInMatchmakingAtSessionStart() const
        {
            return mTotalUsersInMatchmakingAtSessionStart;
        }

        uint64_t getCurrentMatchingSessionListPlayerCount() const
        {
            return mCurrentMatchmakingSessionListPlayerCount;
        }

        uint64_t getTotalMatchingSessionListPlayerCount() const
        {
            return mTotalMatchmakingSessionListPlayerCount;
        }
        //////////////////////////////////////////////////////////////////////////
        // There are three special users we track here:  (These may refer to the same user)
        // The OwnerUserSession is always the session that started matchmaking.   
        // The PrimaryUserSession is either the user that started matchmaking (same as Owner) for normal matchmaking, or it a randomly chosen user when indirect matchmaking is used.
        // The HostUserSession (getHostMemberInfo) is best member of the Group to Host the game, defaulting to the Primary user (if the primary user is as good of a host as the other group members). 
        //////////////////////////////////////////////////////////////////////////
        // Owner - Use when tracking ownership of session, send notifications to this user.
        // Primary - Use for rules or criteria that are require a single user's value (qos, geo location, ping site, etc.)
        // Host - Use for hosting related logic.  (Ip connectivity, NAT checks)
        //////////////////////////////////////////////////////////////////////////

        BlazeId getPrimaryUserBlazeId() const { return (getPrimaryUserSessionInfo() != nullptr ? getPrimaryUserSessionInfo()->getUserInfo().getId() : INVALID_BLAZE_ID); }
        BlazeId getPrimaryUserId() const { return getPrimaryUserBlazeId(); } // DEPRECATED
        const GameManager::UserSessionInfo* getPrimaryUserSessionInfo() const { return (getPrimaryMemberInfo() == nullptr ? nullptr : &getPrimaryMemberInfo()->mUserSessionInfo); }
        const GameManager::UserSessionInfoList& getMembersUserSessionInfo() const { return mMembersUserSessionInfo; }

        /*! ************************************************************************************************/
        /*! \brief returns the user session id of this MM session
            \return user session id of the MM session
        ***************************************************************************************************/
        const GameManager::UserSessionInfo& getOwnerUserSessionInfo() const { return mCachedMatchmakingRequestPtr->getOwnerUserSessionInfo(); }
        const UserSessionId getOwnerUserSessionId() const { return getOwnerUserSessionInfo().getSessionId(); }

        BlazeId getOwnerBlazeId() const { return getOwnerUserSessionInfo().getUserInfo().getId(); }
        BlazeId getOwnerUserId() const { return getOwnerBlazeId(); } // DEPRECATED
        const char8_t* getOwnerPersonaName() const { return getOwnerUserSessionInfo().getUserInfo().getPersonaName(); }
        const char8_t* getOwnerPersonaNamespace() const { return getOwnerUserSessionInfo().getUserInfo().getPersonaNamespace(); }
        ClientPlatformType getOwnerClientPlatform() const { return getOwnerUserSessionInfo().getUserInfo().getPlatformInfo().getClientPlatform(); }

        const PlayerJoinData& getPlayerJoinData() const { return mPlayerJoinData; }
        const GameCreationData& getGameCreationData() const { return mGameCreationData; }

        eastl::string getPlatformForMetrics() const;
        const UserGroupId getUserGroupId() const { return mPlayerJoinData.getGroupId(); }

        GameEntryType getGameEntryType() const { return mPlayerJoinData.getGameEntryType(); }

        const MemberInfo *getPrimaryMemberInfo() const { return mPrimaryMemberInfo; }
        const MemberInfoList &getMemberInfoList() const { return mMemberInfoList; }

        /*! ************************************************************************************************/
        /*! \brief gets best game host from "this" session and all it's group member MM sessions if it has member

            Note: if the session does not have group member, then we return "this" session

            \return best matchmaking session to be the host of the game
        ***************************************************************************************************/
        const MemberInfo& getHostMemberInfo() const { return static_cast<const MemberInfo &>(mMemberInfoList.front()); }
        const UserSessionId getHostUserSessionId() const { return getHostMemberInfo().mUserSessionId; }

        const GameProtocolVersionString& getGameProtocolVersionString() const { return mMatchmakingSupplementalData.mGameProtocolVersionString; }

        const TeamIdVector& getTeamIds() const { return (getCriteria().getTeamIds() != nullptr ? *getCriteria().getTeamIds() : getGameCreationData().getTeamIds()); }

        const RoleInformation& getRoleInformation() const { return mGameCreationData.getRoleInformation(); }

        /*! ************************************************************************************************/
        /*!
            \brief returns true if this session has expired (timed out), given the current time
            \return true if the session has expired, false otherwise.
        *************************************************************************************************/
        bool isExpired() const { return (mSessionAgeSeconds >= mSessionTimeoutSeconds); }

        //! \brief returns true if this session is a new session
        bool isNewSession() const { return ((mStatus & SESSION_NEW) != 0); }

        //! \brief returns true if this session is eligible to finalize
        bool isEligibleToFinalize() const { return ((mStatus & SESSION_ELIGIBLE_TO_FINALIZE) != 0); }

        //! \brief returns true if this session supports the FindGame mode
        bool isFindGameEnabled() const { return ((mStatus & SESSION_MODE_FIND_GAME) != 0); }

        //! \brief returns true if this session supports the CreateGame mode
        bool isCreateGameEnabled() const { return ((mStatus & SESSION_MODE_CREATE_GAME) != 0); }

        /*! ************************************************************************************************/
        /*! \brief check if the session is triggered by an individual player or more than one player (group with >1 players)

            \return true if the session is triggered by an individual player
        ***************************************************************************************************/
        bool isSinglePlayerSession() const { return (getPlayerCount() == 1);  };

        /*! ************************************************************************************************/
        /*! \brief return the total number of players associated with this matchmaking session (session owner +
                any group members).
            \return the total number of players in this matchmakingsession
        ***************************************************************************************************/
        uint16_t getPlayerCount() const { return mMemberInfoCount; }

        //! \brief returns the number of players joining into a given team (will be 0 for GB requests)
        uint16_t getTeamSize(TeamId teamId) const;

        bool isSingleGroupMatch() const { return (mCriteria.isPlayerCountRuleEnabled() ? mCriteria.getPlayerCountRule()->isSingleGroupMatch() : false); }

        bool isDebugCheckOnly() const { return mDebugCheckOnly; }

        bool isDebugCheckMatchSelf() const { return mDebugCheckMatchSelf; }
        
        void prepareForFinalization(MatchmakingSessionId finalizingSessionId);

        // Finalization helpers
        void finalizeAsTimeout();
        void finalizeAsTerminated();
        void finalizeAsCanceled();
        void finalizeAsGameSetupFailed();
        void finalizeAsDebugged(MatchmakingResult result) const;

        /*! ************************************************************************************************/
        /*!
            \brief broadcast and log a NotifyMatchmakingFinished msg indicating as SESSION_QOS_VALIDATION_FAILED.
                Sessions should be deleted after they are finalized.

        *************************************************************************************************/
        void finalizeAsFailedQosValidation();
        

        /*! ************************************************************************************************/
        /*! \brief try to finalize this session as createGame.  Returns true on success; also returns a list of
                additional sessions that were finalized (the sessions that joined this game).


            \param[out] filled out with the other matchmaking sessions that we need to finalize
                            (the sessions that joined this game).  Does not include this session.
            \param[in] finalzed teamIdVector of suggested team ids for the game as determined by the teamsizerules
            \return true if this session was able to finalize (create/reset a game)
        ***************************************************************************************************/
        bool attemptCreateGameFinalization(MatchmakingSessionList& finalizedSessionList, CreateGameFinalizationSettings& createGameFinalizationSettings);
        
        void evaluateCreateGame(FullMatchmakingSessionSet &fullSessionList, MatchedSessionsList& matchedSessions);

        /*! ************************************************************************************************/
        /*! \brief validate that the to-be-created game meets this session's requirements

            \param[in] mmCreateGameRequest the create game request that will be sent to GameManager
            \return true if this create game request is acceptable
        ***************************************************************************************************/
        bool evaluateFinalCreateGameRequest(const Blaze::GameManager::MatchmakingCreateGameRequest &mmCreateGameRequest) const;

        const Util::NetworkQosData& getNetworkQosData() const { return mMatchmakingSupplementalData.mNetworkQosData; }

        const GameNetworkTopology getNetworkTopology() const { return mGameCreationData.getNetworkTopology(); }

        const MatchmakingCriteria& getCriteria() const { return mCriteria; }

        const ExternalSessionMasterRequestData& getExternalSessionData() const { return mGameExternalSessionData; }

        /*! ************************************************************************************************/
        /*!
            \brief update the session's age given the current time.  NOTE: this will also change the session's
            dirty flag if any of the session's rule thresholds changed due to the new session age.

            \param[in] now the current time
            \return true if the session is dirty, otherwise, false
        ***************************************************************************************************/
        void updateSessionAge( const TimeValue &now);

        bool updateCreateGameSession();


        const MatchmakingSupplementalData& getSupplementalData() const { return mMatchmakingSupplementalData; }

        /*! ************************************************************************************************/
        /*! \brief send the async notification status out

            \param[in] numOfActiveGames num of active games on the server
        ***************************************************************************************************/
        void sendNotifyMatchmakingStatus();

        /*! ************************************************************************************************/
        /*! \brief update the async status for create game sessions to the session attempting finalization.

            Note: we need to do this because for create game session, session to be selected as host or
            session to be selected to join new created game should share the same information as the session
            which is attempting finalization.

            \param[in] matchmakingAsyncStatus   matchmaking status of the finalize session
        ***************************************************************************************************/
        void updateCreateGameAsyncStatus(const MatchmakingAsyncStatus& matchmakingAsyncStatus)
        {
            matchmakingAsyncStatus.getCreateGameStatus().copyInto(mMatchmakingAsyncStatus.getCreateGameStatus());
        }

        void updateFindGameAsyncStatus(uint32_t numGames)
        {
            numGames += mMatchmakingAsyncStatus.getFindGameStatus().getNumOfGames(); 
            mMatchmakingAsyncStatus.getFindGameStatus().setNumOfGames(numGames);
        }

        /*! ************************************************************************************************/
        /*! \brief check if it's time for another async status notification

            \param[in] statusNotificationPeriodMs - status notification period from config file
            \return true if time is due, otherwise false
        ***************************************************************************************************/
        bool isTimeForAsyncNotifiation(const Blaze::TimeValue &now, const uint32_t statusNotificationPeriodMs)
        {
            if (now > mLastStatusAsyncTimestamp && (now - mLastStatusAsyncTimestamp).getMillis() >= statusNotificationPeriodMs)
            {
                mLastStatusAsyncTimestamp = now;
                return true;
            }

            return false;
        }

        // The amount of time that has passed since the scenario started (or the mm session for non-scenarios).
        TimeValue getMatchmakingRuntime() const
        {
            return (TimeValue::getTimeOfDay() - mScenarioStartTime);
        }

        // The amount of time that has passed since the session started (may be negative if the session was delayed).
        TimeValue getSubsessionRuntime() const
        {
            return (TimeValue::getTimeOfDay() - mSessionStartTime);
        }

        bool getIsStartDelayOver() const
        {
            return (TimeValue::getTimeOfDay() > mSessionStartTime);
        }

        // When did/will the session start up? (May be delayed)
        TimeValue getSessionStartTime() const
        {
            return mSessionStartTime;
        }

        void sendFinalMatchmakingStatus();

        //EntryCriteriaEvaluator interface
        /*! ************************************************************************************************/
        /*!
        \brief returns false 

        \return returns false
        *************************************************************************************************/
        virtual bool getIgnoreEntryCriteriaWithInvite() const { return false; }

        /*! ************************************************************************************************/
        /*! Implementing the EntryCriteriaEvaluator interface
        ************************************************************************************************/

        /*! ************************************************************************************************/
        /*!
        \brief Accessor for the entry criteria for this entry criteria evaluator.  These are string
        representations of the entry criteria defined by the client or configs.

        \return EntryCriteriaMap* reference to the entry criteria for this entry criteria evaluator.
        *************************************************************************************************/
        const EntryCriteriaMap& getEntryCriteria() const override { return mGameCreationData.getEntryCriteriaMap(); }

        /*! ************************************************************************************************/
        /*!
        \brief Accessor for the entry criteria expressions for this entry criteria evaluator.  Expressions
        are used to evaluate the criteria against a given user session.

        \return ExpressionMap* reference to the entry criteria expressions for this entry criteria evaluator.
        *************************************************************************************************/
        ExpressionMap& getEntryCriteriaExpressions() override { return mExpressionMap; }
        const ExpressionMap& getEntryCriteriaExpressions() const override { return mExpressionMap; }

        /*! ************************************************************************************************/
        /*! \brief determine if this CreateGame matchmaking session is eligible for finalization.  If so, set
        the session's SESSION_ELIGIBLE_TO_FINALIZE status flag.
        ***************************************************************************************************/
        void updateCreateGameFinalizationEligibility(CreateGameFinalizationSettings& createGameFinalizationSettings);

        void sessionCleanup();

        void removeMember(MemberInfo &member);

        GameId getPrivilegedGameIdPreference() { return mPrivilegedGameIdPreference; }

        void addMatchingSession(MatchmakingSession *otherSession, const MatchInfo &matchInfo, DebugRuleResultMap& debugResults);

        bool updatePendingMatches();

        bool attemptGreedyCreateGameFinalization(MatchmakingSessionList &finalizedSessionList);

        void fillUserSessionSetupReason(UserSessionSetupReasonMap &map,
            MatchmakingResult result, FitScore fitScore, FitScore maxFitScore) const;

   
        // these methods allow the matchmaker to mark a find game session as in-progress for finalization as find game
        void setLockdown();
        void clearLockdown();
        bool isLockedDown(bool ignoreOtherSessionLock = false) const;   // If set to true, we only care about the lock if this session did it. 

        // set the id of the session that pulled in for finalization here
        // this is used during session cleanup to cache off the fit score for finalization
        void setFinalizingMatchmakingSessionId( MatchmakingSessionId mmSessionId ) { mFinalizingMatchmakingSession = mMMSessionId; }

        /*! ************************************************************************************************/
        /*! \brief Init the supplied createGameRequest for matchmaking createGame finalization.  The game's settings
                are drawn from the finalizing session (this) and the selected gameHost (if any).  Game attributes are
                voted upon.
        ***************************************************************************************************/
        bool initCreateGameRequest(CreateGameFinalizationSettings& finalizationSettings) const;

        /*! ************************************************************************************************/
        /*! \brief Build a temporary list of joining players that desire multiple roles, sort them based on the number of roles they desire
                 and then select a role for each
                 Roles need to be selected here for players desiring multiple roles, otherwise we can end up finalizing with a group of players
                 that are unable to all fit into the available roles.
                 For example, three players attempting to CG MM, each specifying a game that has three roles (A, B, C) each with a capacity of 1.
                 Two of the players desire two of the roles (A & B) while the third player desires role A. Clearly there is not enough
                 capacity to fit three players into two roles, but the rule evaluation is unable to determine this as it only compares
                 two MM sessions at a time (and each pair in this case can match). If we wait until after finalization to do role selection,
                 we'll end up with a game that only two of the three players can join.
        ***************************************************************************************************/
        void sortPlayersByDesiredRoleCounts(const GameManager::PerPlayerJoinDataList &perPlayerJoinData, UserRoleCounts<PerPlayerJoinDataPtr> &perPlayerJoinDataRoleCounts) const;

        FitScore getCreateGameFinalizationFitScore() const { return mCreateGameFinalizationFitScore; }

        const Matchmaker& getMatchmaker() const { return mMatchmaker; }

        void addFindGameMatchScore(FitScore fitScore, GameId gameId);
        void outputFindGameMatchScores() const;

        void addDebugTopResult(const MatchedGame& matchedGame);
        void outputFindGameDebugResults() const;
        void setDebugCreateGame(const CreateGameMasterRequest& request, FitScore createdGameFitScore, BlazeId creatorId);
        void addDebugMatchingSession(const DebugSessionResult& debugResults);
        // behaves like add debug matching session, but inserts the evaluation details of the finalizing session that pulled this one in.
        void addDebugFinalizingSession();
   
        void fillNotifyReservedExternalPlayers(GameId joinedGameId, NotifyMatchmakingReservedExternalPlayersMap& notificationMap) const;

        bool isSessionJoinableForTeam(const MatchmakingSession& session, const CreateGameFinalizationTeamInfo& teamInfo, const MatchmakingSession &autoJoinSession, TeamId joiningTeamId) const;
        
        void sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener );

        bool validateQosAndUpdateAvoidList(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response);

        uint16_t getQosTier() const { return mQosTier; }
        uint16_t getFailedConnectionAttempts() const { return mFailedConnectionAttempts; }

        void updateStartMatchmakingInternalRequestForFindGameRestart();
        const StartMatchmakingInternalRequestPtr getCachedStartMatchmakingInternalRequestPtr() const { return mCachedMatchmakingRequestPtr; }


        bool getDebugFreezeDecay() const { return mDebugFreezeDecay; }
        bool getDebugCheckOnly() const { return mDebugCheckOnly; }

        bool getDynamicReputationRequirement() const { return mGameCreationData.getGameSettings().getDynamicReputationRequirement(); }


        void updateMatchmakedGameMetrics(const MatchmakingFoundGameResponse& matchmakingFoundGameResp)
        {
            mMaxFitScore = matchmakingFoundGameResp.getMaxFitscore();
            mFitScore = matchmakingFoundGameResp.getFitScore();
            mMatchmakedGameCapacity = matchmakingFoundGameResp.getFoundGameParticipantCapacity();
            mMatchmakedGamePlayerCount = matchmakingFoundGameResp.getFoundGameParticipantCount();
        }

        void setMaxFitScoreForMatchmakedGame(FitScore maxFitScore) { mMaxFitScore = maxFitScore; }
        void setFitScoreForMatchmakedGame(FitScore fitScore) { mFitScore = fitScore; }
        void setMatchmakedGameCapacity(uint16_t matchmakedGameCapacity) { mMatchmakedGameCapacity = matchmakedGameCapacity; }
        void setMatchmakedGamePlayerCount(uint16_t matchmakedGamePlayerCount) { mMatchmakedGamePlayerCount = matchmakedGamePlayerCount; }

        FitScore getMaxFitScoreForMatchmakedGame() const { return mMaxFitScore; }
        FitScore getFitScoreForMatchmakedGame() const { return mFitScore; }
        uint16_t getMatchmakedGameCapacity() const { return mMatchmakedGameCapacity; }
        uint16_t getMatchmakedGamePlayerCount() const { return mMatchmakedGamePlayerCount; }

        const ScenarioInfo& getScenarioInfo() const { return mScenarioInfo; }
        const char8_t* getCreateGameTemplateName() const;

        //! \brief clears this session's CreateGame bit
        // if a session that has CG and FG enabled fails to reset a server via CG finalization, we may call this to keep it from re-attempting CG evaluation.
        void clearCreateGameEnabled() { mStatus &= ~SESSION_MODE_CREATE_GAME; }

        bool shouldSkipFindGame(uint64_t idleCounter) const { return (mLastReinsertionIdle >= idleCounter); }
        void setNextIdleForFindGameProcessing(uint64_t idleCounter) { mLastReinsertionIdle = idleCounter;}

        bool isMarkedForCancel() const { return mMarkedforCancel; }
        void markForCancel() { mMarkedforCancel = true; }

        /*! ************************************************************************************************/
        /*!
            \brief if teams are enabled select the team that this player should join.
        
            \param[in] finalizationSettings - The finalization settings for the create game matchmaking session
            \param[out] teamInfo - Information about the team to join.
        
            \return true if a teams is enabled and a team is selected.
        *************************************************************************************************/
        bool selectTeams(CreateGameFinalizationSettings& finalizationSettings, CreateGameFinalizationTeamInfo*& teamInfo) const;

        // track tag will be sent as part of "mp_match_join" event if get populated from reco service
        void setTrackingTag(const char8_t* tag) { mTrackingTag = tag; }
        const eastl::string& getTrackingTag() const { return mTrackingTag; }

        MatchmakingSubsessionDiagnostics& getDiagnostics() { return *mDiagnostics; }

        void scheduleAsyncStatus();
        void scheduleExpiry();
        bool getIsUsingFilters() const;

        BlazeRpcError findOpenRole(const CreateGameMasterRequest& createGameRequest, const PerPlayerJoinData& playerData,
            RoleName &foundRoleName, TeamIndexRoleSizeMap& roleCounts) const;
        
        const XblSessionTemplateName& getExternalSessionTemplateName() const { return mMatchmakingSessionTemplateName;}

     private:
        //clear intrusive lists
        void clearMatchingSessionList();
        void clearMatchedSessionList();

        void updateMatchingSessionListPlayerCounts();

        inline void evaluateSessionsForCreateGame(MatchmakingSession &otherSession, AggregateSessionMatchInfo &aggregateSessionMatchInfo);


        /*! ************************************************************************************************/
        /*!
            \brief send a matchmaking finished notification tdf with the result of this session
            \param[in] result - the matchmaking result.
            \param[in] totalFitScore - the totalFitScore for the result
            \param[in] gameId - the gameId to init the notifyFinished response to (only valid for successful create/join results)
        *************************************************************************************************/
        void sendNotifyMatchmakingFailed(MatchmakingResult result);

        void buildNotifyMatchmakingPseudoSuccess(NotifyMatchmakingPseudoSuccess& notify, MatchmakingResult result) const;
        void sendNotifyMatchmakingPseudoSuccess(MatchmakingResult result) const;

        void submitMatchmakingFailedEvent(MatchmakingResult matchmakingResult) const;

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the findGame/CreateGame session mode flags from the startMatchmakingRequest.
                Returns true on success.

            \param[in] request  The gameMode is determined by the startMatchmaking request
            \param[out] err We set the errMessage field if the mode is invalid
            \return true on success, false if the mode isn't valid.
        *************************************************************************************************/
        bool initSessionMode(const StartMatchmakingRequest &request, MatchmakingCriteriaError &err);

        //! \brief clears this session's NEW bit
        void clearNewSession() { mStatus &= ~SESSION_NEW; }

        //! \brief sets this session's eligibleToFinalize bit
        void setEligibleToFinalize() { mStatus |= SESSION_ELIGIBLE_TO_FINALIZE; }
        //! \brief clears this session's eligibleToFinalize bit
        void clearEligibleToFinalize() { mStatus &= ~SESSION_ELIGIBLE_TO_FINALIZE; }

        //! \brief sets this session's FindGame bit
        void setFindGameEnabled() { mStatus |= SESSION_MODE_FIND_GAME; }
        //! \brief clears this session's FindGame bit
        void clearFindGameEnabled() { mStatus &= ~SESSION_MODE_FIND_GAME; }

        //! \brief sets this session's CreateGame bit
        void setCreateGameEnabled() { mStatus |= SESSION_MODE_CREATE_GAME; }


        /*! ************************************************************************************************/
        /*! \brief chooses the 'best' session to host a game (the session with the least restrictive NAT type,
                that's small enough to allow this session to join the game).

                Note: at some point, we may want to revise this to take upstream bandwidth into account,
                    and clamp the game size to something the selected host can handle.

            \param[in,out] finalizationSettings requires the playerCapacity be set already, chooses & sets the session to host the game
        ***************************************************************************************************/
        const void chooseGameHost( CreateGameFinalizationSettings &createGameFinalizationSettings ) const;

        void orderPreferredPingSites(CreateGameFinalizationSettings &createGameFinalizationSettings) const;

        /*! ************************************************************************************************/
        /*! \brief given a playerCapacity & hostPlayer (if peer hosted topology), choose the other matching sessions
                that need to explicitly join the game.

            \param[in,out] finalizationSettings requires the playerCapacity & hostPlayer (if any) to be set
                already, chooses & sets the session to host the game
            \return false if unable to select enough sessions to satisfy criteria (required for group matching where we must select at least 1 other group)
        ***************************************************************************************************/
        bool buildGameSessionList(CreateGameFinalizationSettings &finalizationSettings) const;

        void dropBlockListMatches();


        UpdateThresholdResult updateSessionThresholds(bool forceUpdate = false);

        bool isMatchNow(const MatchInfo &matchInfo) const
        {
            return (matchInfo.sMatchTimeSeconds <= mSessionAgeSeconds) && isFitScoreMatch(matchInfo.sFitScore);
        }

        void removeMembersUserSessionInfoListEntry(const MatchmakingSession::MemberInfo &memberInfo);
        void removeUserSessionToMatchmakingMemberMapEntry(MatchmakingSession::MemberInfo &memberInfo);

        void setCreateGameFinalizationFitScore(FitScore fitScore)
        {
            mCreateGameFinalizationFitScore = fitScore;
        }

        // helper method to set up entry criteria during mm session init
        bool setUpMatchmakingSessionEntryCriteria( MatchmakingCriteriaError &err, const StartMatchmakingInternalRequest &request );

        void copyOptionalPlayerJoinData(const MemberInfo& memberInfo, const PlayerJoinData& origData, PlayerJoinData& playerJoinData) const;

        bool addSessionsToFinalizationByTeamCriteria(CreateGameFinalizationSettings &finalizationSettings) const;
        bool addSessionsToFinalizationByTeamCriteriaInternal(CreateGameFinalizationSettings &finalizationSettings, uint16_t maxFinalizationPickSequenceRetriesRemaining) const;

        bool selectNextGameTeamCompositionsForFinalization(const GameTeamCompositionsInfo*& gameTeamCompositionsInfo, const CreateGameFinalizationSettings& finalizationSettings) const;
        bool selectNextBucketAndTeamForFinalization(const MatchedSessionsBucket*& bucket, CreateGameFinalizationTeamInfo*& teamToFill, CreateGameFinalizationSettings& finalizationSettings) const;
        CreateGameFinalizationTeamInfo* selectNextTeamForFinalization(CreateGameFinalizationSettings& finalizationSettings) const;
        const MatchedSessionsBucket* selectNextBucketForFinalization(const CreateGameFinalizationTeamInfo& teamToFill, CreateGameFinalizationSettings& finalizationSettings) const;

        const MatchmakingSessionMatchNode* selectNextSessionForFinalization(const MatchedSessionsBucket& bucket, CreateGameFinalizationTeamInfo& teamToFill, CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession &hostOrAutoJoinSession) const;

        uint16_t calcMaxPermutablyRepickedSequenceLength(uint16_t maxFinalizationPickSequenceRetriesRemaining, uint16_t origMaxRetriesAllowed) const;
        size_t calcNumPermutationsOfLengthK(size_t numPossibleValues, size_t k) const;

        void calcFinalizationByTeamCompositionsSuccessMetrics(const CreateGameFinalizationSettings& finalizationSettings, uint16_t maxGameTeamCompositionsAttemptsRemaining) const;
        void calcFinalizationByTeamUedSuccessMetrics(const MatchmakingCriteria& criteria, const CreateGameFinalizationSettings& finalizationSettings, uint16_t maxFinalizationPickSequenceRetriesRemaining, UserExtendedDataValue teamSizeDiff, UserExtendedDataValue teamUedDiff) const;
        void calcFinalizationByTeamUedFailPickSeqMetrics(UserExtendedDataValue teamSizeDiff, UserExtendedDataValue teamUedDiff) const;

        // return true if this matchmaking session contains a player with the given player id
        bool sessionContainsPlayer(PlayerId playerId) const;

        bool hasMemberWithExternalSessionJoinPermission() const;

        void updateMMQoSKPIMetrics();

        
        bool validateOpenRole(const CreateGameMasterRequest& createGameRequest, const PerPlayerJoinData& playerData,
            const RoleCriteriaMap::const_iterator& roleCriteriaItr, RoleName &foundRoleName, TeamIndexRoleSizeMap& roleCounts) const;
        uint16_t countJoinersRequiringRole(TeamIndex teamIndex, const RoleName &roleName, const PlayerJoinData& groupJoinData) const;

        void timeoutSession();
        void relinquishSession();
        void sendAsyncStatusSession();

    private:

        Blaze::Matchmaker::MatchmakerSlaveImpl &mMatchmakerSlave;
        Matchmaker &mMatchmaker;
        MatchmakingSessionId mMMSessionId;
        Blaze::GameManager::MatchmakingScenarioId mOwningScenarioId;

        MemberInfo *mPrimaryMemberInfo; //The member info object pertaining to the owner of this session.
        MemberInfo *mIndirectOwnerMemberInfo;
        TimeValue            mEstimatedTimeToMatch;
        TimeValue            mSessionStartTime;
        TimeValue            mScenarioStartTime;
        TimeValue            mTotalSessionDuration; // for scenarios, this will track the total lifespan of the scenario
        uint32_t             mStartingDecayAge;
        uint32_t             mSessionAgeSeconds;
        uint32_t             mNextElapsedSecondsUpdate;
        uint32_t             mSessionTimeoutSeconds;
        uint16_t mFailedConnectionAttempts;
        uint16_t mQosTier;

        PlayerJoinData mPlayerJoinData;
        GameCreationData mGameCreationData;
        MatchmakingAsyncStatus mMatchmakingAsyncStatus;     // Initialize before mCriteria!
        MatchmakingCriteria mCriteria;

        bool mDebugCheckOnly;
        // if mDebugCheckMatchSelf is true, this session can match concurrent mm sessions that aren't part of the same scenario, even if owned by the same user
        bool mDebugCheckMatchSelf;
        bool mDebugFreezeDecay;

        // MM_TODO: create template to wrap intrusive list + size + helpers (add/remove/clear)
        MemberInfoList mMemberInfoList;
        uint16_t mMemberInfoCount;

        GameManager::UserSessionInfoList mMembersUserSessionInfo;
        ExternalXblAccountIdList mXblIdBlockList;
        GameIdList mQosGameIdAvoidList;
        BlazeIdList mQosPlayerIdAvoidList;

        //! Status flags used in the mStatus field
        enum SessionStatus
        {
            SESSION_ELIGIBLE_TO_FINALIZE =0x1,
            SESSION_MODE_FIND_GAME = 0x2,
            SESSION_MODE_CREATE_GAME = 0x4,
            SESSION_NEW = 0x8
        };
        uint8_t mStatus; //! holds a set of session status bits (dirty, eligibleToFinalize, and sessionMode bits)

        // the list of other sessions that we currently match (we pull them into our session).
        //  we own the nodes in this list
        MatchedToList mMatchedToList;

        // map of session age to MatchedToList - the pending sessions that will match at a given session age (in the future)
        typedef eastl::map<uint32_t, MatchedToList> PendingMatchListMap;
        PendingMatchListMap mPendingMatchedToListMap;

        // the list of other session that match us now or will match us in the future (and would try to pull us into their session).
        //  we don't own this memory.
        typedef eastl::intrusive_list<MatchedFromListNode> MatchedFromList;
        MatchedFromList mMatchedFromList;

        typedef eastl::multimap<FitScore, GameId, eastl::greater<FitScore> > FindGameMatchScoreMap;
        FindGameMatchScoreMap mFindGameMatchScoreMap;

        typedef eastl::multimap<FitScore, DebugTopResult, eastl::greater<FitScore> > DebugTopResultMap;
        DebugTopResultMap mDebugTopResultMap;

        MatchmakingSupplementalData mMatchmakingSupplementalData;
        Blaze::TimeValue mLastStatusAsyncTimestamp;
        ExpressionMap mExpressionMap;

        GameId mPrivilegedGameIdPreference;

        EntryCriteriaName mFailedGameEntryCriteriaName; // cached tdf for performance  MM_TODO: consider moving this (and the failure message) into the matchmaker itself

        
        FitScore mCreateGameFinalizationFitScore;
        MatchmakingSessionId mFinalizingMatchmakingSession;

        typedef eastl::map<RoleName, RoleEntryCriteriaEvaluator*, CaseInsensitiveStringLessThan> RoleEntryCriteriaEvaluatorMap;
        RoleEntryCriteriaEvaluatorMap mRoleEntryCriteriaEvaluators;
        MultiRoleEntryCriteriaEvaluator mMultiRoleEntryCriteriaEvaluator;

        ExternalSessionMasterRequestData mGameExternalSessionData;
        ExternalSessionCustomData mExternalSessionCustomData;
        ExternalSessionStatus mExternalSessionStatus;

        MatchedSessionsBucketMap mMatchedToBuckets;

        // it's quicker to cache off the original request than it is to rebuild it
        // we need it to restart FG with an updated avoid list (and correct session age) if
        // qos validation fails
        StartMatchmakingInternalRequestPtr mCachedMatchmakingRequestPtr;
        DebugCreateGameResults mDebugCreateGameResults;
        DebugSessionResult mFinalizingSessionDebugResult;

        // Xbox specific, for MPSD sessions representing MM sessions
        XblSessionTemplateName mMatchmakingSessionTemplateName;

        bool mIsFinalStatusAsyncDirty;
        
        //Matchmaked Game Metrics
        FitScore mMaxFitScore;
        FitScore mFitScore;
        uint16_t mMatchmakedGameCapacity;
        uint16_t mMatchmakedGamePlayerCount;

        //Scenario Information 
        ScenarioInfo mScenarioInfo;

        uint64_t mLastReinsertionIdle;

        bool mMarkedforCancel;
        eastl::string mTrackingTag;
        MatchmakingSubsessionDiagnosticsPtr mDiagnostics;

        //State variables for QoS metrics
        bool mUsedCC;
        bool mAllMembersFullyConnected;
        bool mAllMembersPassedLatencyValidation;
        bool mAllMembersPassedPacketLossValidation;

        // PIN metrics (at start of this matchmaking session)
        uint64_t mTotalGameplayUsersAtSessionStart;
        uint64_t mTotalUsersInGameAtSessionStart;
        uint64_t mTotalUsersInMatchmakingAtSessionStart;

        // PIN metrics (calculated at the end of this matchmaking session)
        uint64_t mCurrentMatchmakingSessionListPlayerCount;
        uint64_t mTotalMatchmakingSessionListPlayerCount;

        TimerId mExpiryTimerId;
        TimerId mStatusTimerId;

    };


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

//Provide a key lookup for our Member info nodes
namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::GameManager::Matchmaker::FullUserSessionsMapNode, Blaze::UserSessionId>
    {
        Blaze::UserSessionId operator()(const Blaze::GameManager::Matchmaker::FullUserSessionsMapNode& x) const
        { 
            return static_cast<const Blaze::GameManager::Matchmaker::MatchmakingSession::MemberInfo &>(x).mUserSessionId;
        }
    };
}

#endif // BLAZE_MATCHMAKING_SESSION
