/*! ************************************************************************************************/
/*!
\file matchmakingslave.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_MATCHMAKINGSESSIONSLAVE_H
#define BLAZE_MATCHMAKING_MATCHMAKINGSESSIONSLAVE_H

#include "EASTL/vector_map.h"

#include "framework/blazeallocator.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakingslave.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/rete/legacyproductionlistener.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    class MatchmakingSessionSlave : public Rete::LegacyProductionListener
    {
        NON_COPYABLE(MatchmakingSessionSlave);

    public:

        MatchmakingSessionSlave(MatchmakingSlave& matchmakingSlave, Rete::ProductionId sessionSlaveId, InstanceId originatingInstanceId);
        ~MatchmakingSessionSlave() override;

        BlazeRpcError initialize(const StartMatchmakingInternalRequest& request, bool evaluateGameProtocolVersionString, MatchmakingCriteriaError& criteriaError, const DedicatedServerOverrideMap &dedicatedServerOverrideMap);
        bool hasMatchedGames() const { return (mTopMatchHeap.begin() != mTopMatchHeap.end()); }

        const MatchmakingCriteria& getMatchmakingCriteria() const { return mCriteria; }

        // from ProductionListener
        Rete::ProductionId getProductionId() const override { return mReteProductionId; }
        const Rete::ReteRuleContainer& getReteRules() const override { return mCriteria.getReteRules(); }
        bool onTokenAdded(const Rete::ProductionToken& token, Rete::ProductionInfo& info) override;
        void onTokenRemoved(const Rete::ProductionToken& token, Rete::ProductionInfo& info) override;
        void onTokenUpdated(const Rete::ProductionToken& token, Rete::ProductionInfo& info) override;

        uint32_t getSessionAgeSeconds() const { return mSessionAgeSeconds; }
        bool isExpired() { return (mSessionAgeSeconds >= mSessionTimeoutSeconds); }
        bool isAboutToExpire() const { return (mNextElapsedSecondsUpdate == mSessionTimeoutSeconds); }

        void setNewSessionMatchAttempted() { mNewSessionFirstMatchAttempted = true; }
        bool getNewSessionMatchAttempted() const { return mNewSessionFirstMatchAttempted; }

        void initJoinGameRequest(JoinGameRequest &joinRequest) const;
        void initJoinGameMasterRequest(JoinGameMasterRequest &joinRequest) const;
        void initJoinGameByGroupRequest(JoinGameByGroupMasterRequest &joinRequest) const;
        bool isSinglePlayerSession() const { return (mStartMatchmakingInternalRequest.getUsersInfo().size() == 1); }
        UserSessionId getOwnerUserSessionId() const { return getOwnerSessionInfo().getSessionId(); }
        
        // Find Game sessions don't distinguish between the primary user and host session (because the host will not change).
        const UserSessionInfo& getPrimaryUserSessionInfo() const { return *Blaze::GameManager::getHostSessionInfo(mStartMatchmakingInternalRequest.getUsersInfo()); }
        UserSessionId getPrimaryUserSessionId() const { return getPrimaryUserSessionInfo().getSessionId(); }

        bool isIndirectMatchmakingRequest() const { return (getPrimaryUserSessionId() != getOwnerUserSessionId()); }

        MatchmakingSessionMode getSessionMode() const { return getStartMatchmakingRequest().getSessionData().getSessionMode(); }
        bool getDebugCheckOnly() const { return getStartMatchmakingRequest().getSessionData().getPseudoRequest(); }

        UpdateThresholdStatus updateFindGameSession();
        void updateSessionAge(const TimeValue &now);
        void updateNonReteRules(const Search::GameSessionSearchSlave& game);

        void submitMatchmakingFailedEvent(MatchmakingResult matchmakingResult) const;        
        bool isTimeForAsyncNotifiation(const Blaze::TimeValue &now, const uint32_t statusNotificationPeriodMs);

        // MM_TODO: This should actually check the session's active rules to determine the status. Can probably be determined once 
        inline bool hasNonReteRules() const { return true; }
        void reevaluateNonReteRules();

        // The amount of time that has passed since the session started (may be negative if the session was delayed).
        TimeValue getDuration() const { return TimeValue::getTimeOfDay() - mSessionStartTime; };
        TimeValue getStartTime() const { return mSessionStartTime; }

        void peekBestGames(MatchmakingFoundGames& foundGames);
        void popBestGames();

        size_t getTopMatchHeapSize() const { return mTopMatchHeapSize; }
        uint32_t getNumFailedFoundGameRequests() const { return mNumFailedFoundGameRequests; }

        void destroyIntrusiveNode(GameManager::MmSessionGameIntrusiveNode& node);

        void flagReadyForFinalization()
        {         
            if (!mTopMatchHeap.empty())
            {
                mMatchmakingSlave.addToMatchingSessions(*this);
            }
        }

        const StartMatchmakingRequest& getStartMatchmakingRequest() const { return mStartMatchmakingInternalRequest.getRequest(); }
        const GameManager::UserSessionInfo& getOwnerSessionInfo() const { return mStartMatchmakingInternalRequest.getOwnerUserSessionInfo(); }
        BlazeId getOwnerBlazeId() const { return mOwnerBlazeId; }
        BlazeId getOwnerUserId() const { return getOwnerBlazeId(); } // DEPRECATED
        UserSessionId getOwnerSessionId() const { return mOwnerSessionId; }
        const char8_t* getOwnerPersonaName() const { return getOwnerSessionInfo().getUserInfo().getPersonaName(); }

        bool checkForGameOverride();

        void matchRequiredDebugGames();

        SlaveSessionId getOriginatingSessionId() const { return mOriginatingSessionId; }

        bool hasMatchedDebugGames() const { return mHasMatchedDebugGames; }
        bool wasStartedDelayed() const { return mStartedDelayed; }

        void tallySessionDiagnosticsFG(MatchmakingSubsessionDiagnostics& sessionDiagnostics, const DiagnosticsSearchSlaveHelper& helper);
        InstanceId getOriginatingInstanceId() const { return mOriginatingInstanceId; }
    private:
    
        bool attemptFillGameOverride();
        bool attemptPrivilegedGameMatch();

        void addGameToMatchHeap(const Search::GameSessionSearchSlave& game, FitScore totalFitScore);
        void addGameToMatchHeap(GameManager::MmSessionGameIntrusiveNode& curNode, FitScore totalFitScore);
        void removeGameFromMatchHeap(GameId id);
        void removeGameFromMatchHeap(GameManager::MmSessionGameIntrusiveNode& curNode);

        bool addMatchedGameToReferenceMap(GameManager::MmSessionGameIntrusiveNode& node);

        bool isGamePotentiallyJoinable(const Search::GameSessionSearchSlave &game) const;
        bool isGamePotentiallyJoinableForUserSessions(const Search::GameSessionSearchSlave &game, const UserJoinInfoList& sessionInfos) const;

        void insertTopMatchHeap(const ProductionTopMatchHeap::iterator& loc, MmSessionGameIntrusiveNode &node);
        void pushBackTopMatchHeap(MmSessionGameIntrusiveNode &node);
        MmSessionGameIntrusiveNode& popBackTopMatchHeap();
        void removeTopMatchHeap(MmSessionGameIntrusiveNode &node);

        void pushBackProductionMatchHeap(MmSessionGameIntrusiveNode &node);
        MmSessionGameIntrusiveNode& popFrontProductionMatchHeap();
        void removeProductionMatchHeap(MmSessionGameIntrusiveNode &node) const;

        void insertGameIntrusiveMap(GameManager::MmSessionGameIntrusiveNode& node);
        void eraseGameIntrusiveMap(GameManager::MmSessionGameIntrusiveNode& node);

        bool hasPrivilegedGameIdPreference() const { return (mPrivilegedGameIdPreference != GameManager::INVALID_GAME_ID); }

        void updateNatSupplementalData(const UserSessionInfo& userSessionInfo, MatchmakingSupplementalData& data, const UserSessionsToExternalIpMap &userSessionsToExternalIpMap);

        bool getOrCreateGameIntrusiveNode(GameManager::MmSessionGameIntrusiveNode** node, Search::GameSessionSearchSlave &gameSession);

        Blaze::GameManager::MatchmakingScenarioId getScenarioId() const { return mOriginatingScenarioId; }
    private:
        
        MatchmakingSlave &mMatchmakingSlave;

        Rete::ProductionId mReteProductionId;
        InstanceId mOriginatingInstanceId;

        Blaze::GameManager::MMGameIntrusiveMap mGameIntrusiveMap;

        // General bucket of all matches (unsorted)
        Blaze::GameManager::ProductionMatchHeap mProductionMatchHeap;
        // Initially sorted TopN list of matches
        Blaze::GameManager::ProductionTopMatchHeap mTopMatchHeap;
        size_t mTopMatchHeapSize;

        bool mHasExpired;
        bool mNewSessionFirstMatchAttempted;
        bool mHasMatchedDebugGames;
        bool mStartedDelayed;

        MatchmakingAsyncStatus mMatchmakingAsyncStatus;            // Must be initialized prior to the criteria since criteria destructor requires async status reference!  GB_TODO: refactor to make this dependency more obvious
        MatchmakingCriteria mCriteria;

        // Stores off a copy of our initial matchmaking request
        GameManager::StartMatchmakingInternalRequest mStartMatchmakingInternalRequest;
        BlazeId mOwnerBlazeId;
        UserSessionId mOwnerSessionId;

        TimeValue mSessionStartTime;
        TimeValue mLastStatusAsyncTimestamp;
        uint32_t mStartingDecayAge;
        uint32_t mSessionAgeSeconds;
        uint32_t mSessionTimeoutSeconds;
        uint32_t mNextElapsedSecondsUpdate;
        uint32_t mNumFailedFoundGameRequests;

        uint32_t mNumFindGameMatches;
        uint64_t mTotalFitScore;

        Blaze::GameManager::GameId mPrivilegedGameIdPreference;
        
        SlaveSessionId mOriginatingSessionId;
        Blaze::GameManager::MatchmakingScenarioId mOriginatingScenarioId;
        GameManager::GameIdList mRemovedGameList;

    public:
        TimeValue mOtaQuickOut;
        TimeValue mOtaFull;
        uint32_t mOtaQuickOutCount;
        uint32_t mOtaFullCount;
        uint32_t mOtrCount;

        GameManager::UserSessionInfoList mMembersUserSessionInfo;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif




