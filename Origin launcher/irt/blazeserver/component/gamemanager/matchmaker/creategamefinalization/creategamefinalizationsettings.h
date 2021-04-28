/*! ************************************************************************************************/
/*!
    \file   creategamefinalizationsettings.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_CREATE_GAME_FINALIZATION_SETTINGS
#define BLAZE_CREATE_GAME_FINALIZATION_SETTINGS

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationteaminfo.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationteamcompositionsinfo.h"
#include "gamemanager/matchmaker/rules/teaminfohelper.h"
#include "gamemanager/gamesession.h"

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
    class ExpandedPingSiteRule;
    class MatchmakingCriteria;

    typedef eastl::hash_map<MatchmakingSessionId, TeamIdToTeamIndexMap> TeamMatchmakingSessions;

    /*! ************************************************************************************************/
    /*! \brief used by createGame matchmaking session finalization.  Contains a list of matchmaking sessions
        that must join the game explicitly, as well as the game host (if any), and sessions that join
        the game automatically (as a side effect of game creation/reset).
    ***************************************************************************************************/
    class CreateGameFinalizationSettings
    {
        NON_COPYABLE(CreateGameFinalizationSettings)

    public:

        CreateGameFinalizationSettings(const MatchmakingSession& mmSession);
        
        bool canFinalizeWithSession(const MatchmakingSession *finalizingSession, const MatchmakingSession *owningSession, CreateGameFinalizationTeamInfo*& teamInfo);

        void addDebugInfoForSessionInGameSessionList(const MatchmakingSessionMatchNode& node);
        void addSessionToGameSessionList(const MatchmakingSessionMatchNode& node, CreateGameFinalizationTeamInfo* teamInfo);
        void addSessionToGameSessionList(const MatchmakingSession &matchingSession, CreateGameFinalizationTeamInfo* teamInfo);

        // return the total number of players in the sessions (automatically & explicitly joining, including group members)
        uint16_t calcFinalizingPlayerCount() const;


        bool isTeamsEnabled() const { return !mCreateGameFinalizationTeamInfoVector.empty(); }
        CreateGameFinalizationTeamInfo* getTeamInfoByTeamIndex(TeamIndex teamIndex);
        CreateGameFinalizationTeamInfo* getSmallestAvailableTeam(TeamId teamId, eastl::set<CreateGameFinalizationTeamInfo*>& teamsToSkip);

        TeamIndex getTeamIndexForMMSession(const MatchmakingSession& session, TeamId teamId) const;
        void claimTeamIndexForMMSession(const MatchmakingSession& session, TeamId teamId, TeamIndex index);

        bool areTeamSizesSufficientToCreateGame(const MatchmakingCriteria& criteria, uint16_t& teamSizeDifference, TeamIndex& largestTeam) const;
        bool areTeamsAcceptableToCreateGame(const MatchmakingCriteria& criteria, uint16_t& teamSizeDiff, UserExtendedDataValue& teamUedDiff) const;

        const char8_t* pickSequenceToLogStr(eastl::string& buf) const;
        void clearAllSessionsTriedAfterIthPick(uint16_t ithPickNumber);
        bool hasUntriedTeamsForCurrentPick(uint16_t pickNumber) const;
        bool wasTeamTriedForCurrentPick(TeamIndex teamIndex, uint16_t pickNumber) const;
        void markTeamTriedForCurrentPick(TeamIndex teamIndex, uint16_t pickNumber);

        void removeSessionsFromTeamsFromIthPickOn(uint16_t ithPickNumber, const MatchmakingSession& pullingSession, const MatchmakingSession& hostSession);

        // Remove the session from all teams that it was part of:
        bool removeSessionFromTeams(const MatchmakingSession* session);

        bool areAllFinalizingTeamSizesEqual() const;

        bool wasGameTeamCompositionsIdTried(GameTeamCompositionsId gameTeamCompositionsId) const;
        void markGameTeamCompositionsIdTried(GameTeamCompositionsId gameTeamCompositionsId);
        bool initTeamCompositionsFinalizationInfo(const GameTeamCompositionsInfo& gameTeamCompositionsInfo, const MatchmakingSession& pullingSession);

        // return the first repickable session number.
        uint16_t getOverallFirstRepickablePickNumber(const MatchmakingSession& pullingSession) const;

        const DebugSessionResult* getDebugResults(MatchmakingSessionId sessionId) const 
        { 
            DebugSessionMap::const_iterator iter = mDebugSessionMap.find(sessionId); 
            if (iter != mDebugSessionMap.end())
                return &(iter->second);

            return nullptr;
        }

        void getMembersUedMaxDeltas(const MatchmakingCriteria& criteria, UserExtendedDataValue& gameMembersDelta, UserExtendedDataValue& teamMembersDelta) const;
        bool areAllFinalizingTeamCompositionsEqual() const;
        float getFinalizingTeamCompositionsFitPercent() const;

        bool containsUserGroup(const UserGroupId &userGroupId) const { return (userGroupId != EA::TDF::OBJECT_ID_INVALID) && (mUserGroupIdSet.find(userGroupId) != mUserGroupIdSet.end()); }
        bool containsScenario(MatchmakingScenarioId scenarioId) const { return (mScenarioIdSet.find(scenarioId) != mScenarioIdSet.end()); }

        bool areAllSessionsMakingReservations() const;

        void trackTeamsWithMatchedSession(const MatchmakingSession& matchedSession);
        bool areTeamSizesSufficientToAttemptFinalize(const MatchmakingSession& finalizingSession);

        void updateFinalizedNatStatus( const MatchmakingSession& matchedSession );

        /*! ************************************************************************************************/
        /*! \brief given a matchmaking session, returns true if a network connection will be possible

            \param[in] matchedSession session to evaluate
            \param[in] finalizingSession the session attempting to finalize
            \return true if the session should be able to connect.
        ***************************************************************************************************/
        bool isSessionConnectable( const MatchmakingSession* matchedSession, const MatchmakingSession* finalizingSession ) const;

        void updatePingSitePlayerCount(const MatchmakingSession& finalizingSession, const MatchmakingSession& matchedSession);
        void buildSortedPingSitePlayerCountList(const Blaze::Matchmaker::MatchmakerSlaveImpl& matchmakerSlave, const MatchmakingCriteria& criteria, bool& pingSitePlayerCountSufficient);
        void buildOrderedAcceptedPingSiteList(const ExpandedPingSiteRule& rule, const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection,
            OrderedPreferredPingSiteList& orderedPreferredPingSites);

    public:
        // Getters
        const MultiRoleEntryCriteriaEvaluator& getMultiRoleEntryCriteraEvaluator() const { return mMultiRoleEntryCriteriaEvaluator; }
        const Blaze::GameManager::MatchmakingSessionId getFinalizingMatchmakingSessionId() const { return mFinalizingMatchmakingSessionId; }
        const CreateGameFinalizationGameTeamCompositionsInfo& getGameTeamCompositionsInfo() const { return mTeamCompositionsFinalizationInfo; }
        // return the total number of sessions in the finalization list currently. This is also the current pick number.
        uint16_t getTotalSessionsCount() const { return mMatchingSessionList.size(); }
        const MatchmakingSessionList& getMatchingSessionList() const { return mMatchingSessionList; }
        MatchmakingSessionList& getMatchingSessionList() { return mMatchingSessionList; }
        const RoleInformation& getRoleInformation() const { return mRoleInformation; }
        uint16_t getPlayerCapacity() const { return mPlayerCapacity; }
        uint16_t getPlayerSlotsRemaining() const { return mPlayerSlotsRemaining; }
        const FinalizedNatStatus getFinalizedNatStatus() const { return mNatStatus; }
        GameManager::MatchmakingCreateGameRequest& getCreateGameRequest() { return mCreateGameRequest; }

    public:
        // Setters
        void setPlayerSlotsRemaining(uint16_t slots) { mPlayerSlotsRemaining = slots; }

    public:

        CreateGameFinalizationTeamInfoVector mCreateGameFinalizationTeamInfoVector;

        /*! ************************************************************************************************/
        /*!\brief user which will be the host of a non-dedicated server game to be created, 
                 or a user which will join automatically when a dedicated server game is reset
                DO NOT USE from the matchmaking job (other than ctor).  Dependent on session existence.
                which can change once job gets executed because the job can block calling XBL services.
        *************************************************************************************************/
        const MatchmakingSession::MemberInfo *mAutoJoinMemberInfo;

        PingSiteAliasList mOrderedPreferredPingSites;

    private:
        bool hasBestPingSite(PingSiteAlias& pingSite);

    private:
        // map of matchmaking session ids to TeamIndexes
        TeamMatchmakingSessions mTeamIndexByMatchmakingSession;

        CreateGameFinalizationGameTeamCompositionsInfo mTeamCompositionsFinalizationInfo;
        FinalizedNatStatus mNatStatus;

        // List of matching MatchmakingSessions (could be subsession)
        MatchmakingSessionList mMatchingSessionList;

        GameManager::MatchmakingCreateGameRequest mCreateGameRequest;

        const Blaze::GameManager::MatchmakingSessionId mFinalizingMatchmakingSessionId;

        // Helper to handle team validation
        MatchingTeamInfoValidator mTeamValidator;

        RoleInformation mRoleInformation;
        MultiRoleEntryCriteriaEvaluator mMultiRoleEntryCriteriaEvaluator;

        // set of all user groups pulled in by matchmaking
        typedef eastl::hash_set<UserGroupId> UserGroupIdSet;
        UserGroupIdSet mUserGroupIdSet; 

        // set of all scenarios pulled in by matchmaking (not including he INVALID ones)
        typedef eastl::hash_set<MatchmakingScenarioId> MatchmakingScenarioIdSet;
        MatchmakingScenarioIdSet mScenarioIdSet; 

        // the player capacity the game should be created with
        uint16_t mPlayerCapacity; 
        // the number of slots that havent been filled. (capacity - players added)
        uint16_t mPlayerSlotsRemaining; 

        // index of the vector maps to TeamIndex
        typedef eastl::vector<bool> TeamsTriedVector;
        typedef eastl::hash_map<uint16_t, TeamsTriedVector> PickNumberToTeamsTriedVectorMap;
        PickNumberToTeamsTriedVectorMap mTeamsTriedVectorMap;

        typedef eastl::hash_set<GameTeamCompositionsId> GameTeamCompositionsIdSet;
        GameTeamCompositionsIdSet mGameTeamCompositionsIdTriedSet;

        typedef eastl::hash_map<MatchmakingSessionId, DebugSessionResult> DebugSessionMap;
        DebugSessionMap mDebugSessionMap;

        typedef eastl::vector_map<PingSiteAlias, uint32_t> PlayerCountsByAcceptedPingSiteMap;
        PlayerCountsByAcceptedPingSiteMap mPlayerCountsByAcceptedPingSiteMap;

        typedef eastl::pair<uint32_t, PingSiteAlias> PingSitePlayerCountPair;
        typedef eastl::vector<PingSitePlayerCountPair> PlayerCountsByAcceptedPingSiteList;
        PlayerCountsByAcceptedPingSiteList mSortedAcceptedPingSitePlayerCounts;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_CREATE_GAME_FINALIZATION_SETTINGS
