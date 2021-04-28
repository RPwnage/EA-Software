/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEMANAGER_UTIL
#define BLAZE_STRESS_GAMEMANAGER_UTIL

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "framework/tdf/attributes.h"

#ifdef TARGET_censusdata
#include "censusdata/rpc/censusdataslave.h"
#endif
#include "framework/rpc/usersessionsslave.h"

#ifdef TARGET_arson
#include "arson/tdf/arson.h"
#include "arson/rpc/arsonslave.h"
#endif

#include "stressmodule.h"
#include "stressinstance.h"

#include "stats/statscommontypes.h"

#ifdef TARGET_gamereporting
#include "gamereportingutil.h"
#endif

#include "attrvalueutil.h"

using namespace Blaze::GameManager;

namespace Blaze
{

class ConfigMap;
class ConfigSequence;

namespace Stress
{

class StressInstance;
class StressModule;

class GameManagerUtilInstance;

class GameData;

///////////////////////////////////////////////////////////////////////////////
//  class GameManagerUtil
//  
//  Used by all GameManagerUtilInstance objects to update game instance.
//  and all other data that 
class GameManagerUtil
{
public:
    //  each role has different logic in the execute implementation.
    enum Role
    {
        ROLE_NONE,
        ROLE_HOST,
        ROLE_JOINER,
        ROLE_VIRTUAL_GAME_CREATOR,
        NUM_ROLES
    };

    static const char8_t* ROLE_STRINGS[NUM_ROLES];

    enum QueryJoinFailure
    {
        QUERY_JOIN_NOT_READY = -1,
        QUERY_JOIN_NO_ROOM = 0,
        QUERY_JOIN_SOME_ROOM,
        QUERY_JOIN_ROOM
    };

    GameManagerUtil();
    virtual ~GameManagerUtil();

    //  parse game manager utility specific configuration for setup
    virtual bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil);

    virtual void dumpStats(StressModule *module);

    //  call to reserve a game slot in the utility - when finished
    void startRegisterGame();
    GameData* registerGame(ReplicatedGameData &replGameData, ReplicatedGamePlayerList &replPlayerData);
    void finishRegisterGame();
    //  unregisters the game from the util - if game manager destroys the game by some other means than an explicit RPC call.
    bool unregisterGame(GameId gameId);

    /*
        Utility wrappers for RPC calls
    */  
    // Instance calls to advance game state
    void advanceGameState(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, GameState newState);
    // Instance calls to set new game settings
    void setGameSettings(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, GameSettings *newSettings);
#ifdef TARGET_arson
    void setGameSettingsArson(BlazeRpcError *err, Arson::ArsonSlave *proxy, GameId gameId, GameSettings *newSettings);
#endif
    // Instance calls to update game name
    void udpateGameName(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, const char8_t *newName);
    // Instance calls to remove a player from the game
    void removePlayer(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, PlayerId playerId, PlayerRemovedReason reason);
    // Instance calls to join a player to the game
    GameId joinGame(BlazeRpcError *err, GameManagerSlave *proxy, JoinGameResponse *resp, StressInstance *instance, bool joinByUser=false, const JoinGameRequest *reqOverride=nullptr);
    // Instance calls to join a player to the game.
    GameId joinGame(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, JoinGameResponse *resp, bool joinByUser=false, const JoinGameRequest *reqOverride=nullptr);
    // Instance calls to start a matchmake
    void startMatchmakingScenario(BlazeRpcError *err, GameManagerSlave *proxy, StartMatchmakingScenarioRequest *request, StartMatchmakingScenarioResponse *response, MatchmakingCriteriaError *error);
    // Instannce calls to cancel a matchmake
    void cancelMatchmakingScenario(BlazeRpcError *err, GameManagerSlave *proxy, MatchmakingScenarioId sessionId);
 
    // Allows overriding the create game request options.
    void setCreateGameRequestOverride(CreateGameRequest *request) { mCreateRequest = request; }
    const CreateGameRequest* getCreateGameRequestOverride() const {
        return mCreateRequest;
    }
    CreateGameRequest* getCreateGameRequestOverride()  {
        return mCreateRequest;
    }
    void setJoinGameRequestOverride(JoinGameRequest *request) { mJoinRequest = request; }
    const JoinGameRequest * getJoinGameRequestOverride() const {
        return mJoinRequest;
    }
    JoinGameRequest* getJoinGameRequestOverride() {
        return mJoinRequest;
    }

    //  returns modifyable game data
    GameData* getGameData(GameId gid);
    //  returns a gameId to join - the game should have room for the instance to join (< capacity).
    GameId pickGame();
    //  checks whether an instance can create a new game based on current utility state (timing/#games.)
    bool queryCreateNewGame() const;

    //  returns the number of games currently active.
    size_t getGameCount() const {
        return mGames.size();
    }
    uint32_t getMaxGames() const {
        return mMaxGames;
    }
    uint32_t getMaxVirtualCreators() const
    {
        return mMaxVirtualCreators;
    }
    uint32_t getGameLifespanCycles() const {
        return mGameLifespan;
    }
    uint32_t getGameLifespanDeviation() const {
        return mGameLifespanDeviation;
    }
    uint32_t getGameStartTimer() const {
        return mGameStartTimer;
    }
    uint32_t getGamePlayerLifespan() const {
        return mGamePlayerLifespan;
    }
    uint32_t getGamePlayerLifespanDeviation() const {
        return mGamePlayerLifespanDeviation;
    }
    float getGameSettingsRandomUpdates() const {
        return mGameSettingsRandomUpdates;
    }
    float getGameNameRandomUpdates() const {
        return mGameNameRandomUpdates;
    }
    float getGameExternalStatusRandomUpdates() const {
        return mGameExternalStatusRandomUpdates;
    }
    uint16_t getGamePlayerSeed() const {
        return mGamePlayerSeed;
    }
    uint16_t getGamePlayerLowerLimit() const {
        return mGamePlayerLowerLimit;
    }
    uint16_t getMaxPlayerCapacity() const {
        return mGamePlayerSeed > getGamePlayerLowerLimit() ? mGamePlayerSeed : getGamePlayerLowerLimit();
    }
    uint16_t getGamePlayerMinRequired() const {
        return mGamePlayerMinRequired;
    }
    GameNetworkTopology getGameTopology() const {
        return mGameTopology;
    }
    bool getJoinInProgress() const {
        return mJoinInProgress;
    }
    bool getShooterIntelligentDedicatedServer()
    {
        return mShooterIntelligentDedicatedServer;
    }
    bool getCensusSubscribe() const {
        return mCensusSubscribe;
    }
    const char8_t *getGameProtocolVersionString() const {
        return mGameProtocolVersionString;
    }
    bool getUseArson() const 
    {
        return mUseArson;   
    }
    const Collections::AttributeName &getGameModeAttributeName() const {
        return mGameModeAttributeName;
    }
    const ConfigSequence* getConfiguredPingSites() const { return mConfiguredPingSites; }


    void setInstanceCreateGame(StressInstance *inst, CreateGameRequest& req);
    void defaultCreateGameRequest(StressInstance *inst, CreateGameRequest& req, uint32_t gameSize = 0);
    void defaultResetGameRequest(StressInstance *inst, CreateGameRequest &req, uint32_t gameSize = 0);
    void defaultJoinGameRequest(JoinGameRequest& req);

    size_t decrementResetableServers() {
        if (mResetableServers > 0) {
            mResetableServers--;
        }
        return mResetableServers;
    }
    size_t incrementResetableServers() {
        return ++mResetableServers;
    }

    void incrementRole(Role role)
    {
        ++mRoleCount[role];
    }

    void decrementRole(Role role)
    {
        --mRoleCount[role];
    }

    uint32_t getRoleCount(Role role)
    {
        return mRoleCount[role];
    }

    //  metrics are shown when calling dumpStats
    //  diagnostics for utility not meant to measure load.
    enum Metric
    {       
        METRIC_GAMES_CREATED,
        METRIC_GAMES_DESTROYED,
        METRIC_PLAYER_LOCAL_JOIN,
        METRIC_PLAYER_JOINING,              // TODO remove
        METRIC_PLAYER_JOINCOMPLETED,
        METRIC_PLAYER_JOIN_TIMEOUTS,
        METRIC_PLAYER_LEFT,                 
        METRIC_JOIN_GAME_FULL,
        METRIC_GAMES_CONN_LOST,             // TODO remove
        METRIC_GAMES_CYCLED,
        METRIC_GAMES_UNREGISTERED,
        METRIC_GAMES_RESET,
        METRIC_GAMES_RETURNED,
        METRIC_PLAYER_REMOVED,
        METRIC_GAMES_ABORTED,
        METRIC_GAMES_EARLY_END,
        METRIC_CENSUS_UPDATES,
        NUM_METRICS
    };

    void addMetric(Metric metric);

    //  construct a game report from the stored game data.
    typedef eastl::vector<EntityId> IdVector;
    typedef eastl::map<const char *,IdVector, CaseInsensitiveStringLessThan> IdVectorMap;
    
    const AttrValues& getGameAttrValues() const {
        return mGameAttrValues;
    }
    const AttrValues& getPlayerAttrValues() const {
        return mPlayerAttrValues;
    }
    const AttrValueUpdateParams& getGameAttrUpdateParams() const {
        return mGameAttrUpdateParams;
    }
    const AttrValueUpdateParams& getPlayerAttrUpdateParams() const {
        return mPlayerAttrUpdateParams;
    }

#ifdef TARGET_gamereporting
    GameReportingUtil& getGameReportingUtil() {
        return mGameReportingUtil;
    }
#endif

    const char8_t* getMockExternalSessions() const { return mMockExternalSessions.c_str(); }
protected:
    void setInstanceJoinGame(GameId gameId, JoinGameRequest& req);
    
private:
    //  configuration values.
    uint32_t mMaxGames;
    uint32_t mMaxVirtualCreators;
    uint32_t mGameLifespan;
    uint32_t mGameLifespanDeviation;
    uint32_t mGameStartTimer;
    uint32_t mGamePlayerLifespan;
    uint32_t mGamePlayerLifespanDeviation;
    uint16_t mGamePlayerSeed;
    uint16_t mGamePlayerLowerLimit;
    uint16_t mGamePlayerMinRequired;
    float mGameSettingsRandomUpdates;
    float mGameNameRandomUpdates;
    float mGameExternalStatusRandomUpdates;
    bool mJoinInProgress;
    bool mShooterIntelligentDedicatedServer;
    bool mCensusSubscribe;
    bool mUseArson;
    Collections::AttributeName mGameModeAttributeName;

    AttrValueUpdateParams mGameAttrUpdateParams;
    AttrValueUpdateParams mPlayerAttrUpdateParams;
    AttrValues mGameAttrValues;
    AttrValues mPlayerAttrValues;
    const ConfigSequence* mConfiguredPingSites;

    GameNetworkTopology mGameTopology;

    typedef eastl::map<GameId, GameData*> GameIdToDataMap;      // map can have many entries and is updated frequently
    GameIdToDataMap mGames;
    GameIdToDataMap::iterator mNextJoinGameIt;

    CreateGameRequestPtr mCreateRequest;
    JoinGameRequestPtr mJoinRequest;

    // An attempt to be fiber-safe with multiple fibers contending to create/join games.  
    // When creating a new game, the counter is incremented before the RPC.  After the RPC the counter is decremented
    // This value is added to the mGames.size() value to get the "estimated" game count, used in the queryCreateNewGame call.
    size_t mPendingGameCount;
    size_t mResetableServers;

    char8_t mGameProtocolVersionString[64];

    //  used to display metrics during stat dump
    uint64_t mMetricCount[NUM_METRICS];
    char8_t mLogBuffer[512];

    uint32_t mRoleCount[NUM_ROLES];

    GameData* pickGameCommon();

#ifdef TARGET_gamereporting
    GameReportingUtil mGameReportingUtil;
#endif

    eastl::string mMockExternalSessions;
};

struct PlayerData;

///////////////////////////////////////////////////////////////////////////////
//  class GameManagerUtilInstance
//  
//  usage: Declare one per StressInstance.  
//  Initialize using the setStressInstance method.
//  Within the StressInstance's execute override, invoke this class's execute 
//  method to update the instance.
//  
class GameManagerUtilInstance: public AsyncHandler
{
public:

    enum RoleState
    {
        ROLESTATE_DEFAULT,
        ROLESTATE_HOST_PRE_START,
        ROLESTATE_HOST_IN_GAME,
        ROLESTATE_HOST_END_GAME,
        ROLESTATE_PLAYER_IN_GAME,
        ROLESTATE_PLAYER_END_GAME
    };

public:
    GameManagerUtilInstance(GameManagerUtil *util);
    ~GameManagerUtilInstance() override;

    //  updates the game instance belonging to its parent stress instance.
    virtual BlazeRpcError execute(); 

    //  if instance logs out, invoke this method to reset state
    void onLogout();

    void setStressInstance(StressInstance* inst);
    void clearStressInstance() { setStressInstance(nullptr); }

    StressInstance* getStressInstance() {
        return mStressInst;
    }

    GameManagerSlave* getSlave() {
        return mGameManagerProxy;
    }

#ifdef TARGET_arson
    Arson::ArsonSlave* getArsonSlave()
    {
        return mArsonSlave;
    }
#endif

    int32_t getIdent() const {
        return (mStressInst != nullptr) ? mStressInst->getIdent() : -1;
    }

    const GameData* getGameData() const {
        return (mGameId > 0) ? mUtil->getGameData(mGameId) : nullptr;
    }

    GameManagerUtil::Role getRole() const {
        return mRole;
    }
    void setRole(GameManagerUtil::Role role);
    void endRole();                 // forces role to prematurely end (cycle counter set to zero.)

    void pauseRole() {              // suspends action on current role - use resume to continue action
        mPaused = true;
    }
    void resumeRole() {
        mPaused = false;
    }

    void setGameId(GameId gameId);  // specifies the game id associated with this util (use only if bypassing the role system.)
    GameId getGameId() const { 
        return mGameId; 
    }
    BlazeId getBlazeId() const;
    BlazeId getBlazeUserId() const { return getBlazeId();  } // DEPRECATED

    void setJoinByUser(bool joinByUser) {
        mJoinByUser = joinByUser;
    }
    bool getJoinByUser() const {
        return mJoinByUser;
    }
    uint32_t getGameTimer() const {
        return mGameTimer;
    }
    uint32_t getPostGameCountdown() const {
        return mPostGameCountdown;
    }
  
    /*
        Game Modification functions
        These will send rpc's up on the next execute.
    */

    void setGameSettings(GameSettings* newSettings) { mSetGameSettingsRequest = newSettings; }
    void setUpdateGameNameRequest(const char8_t *gameName) { blaze_strnzcpy(mUpdateGameNameRequest, gameName, strlen(gameName) + 1); }
    void advanceGameState(GameState state);
    void advanceGameState(GameId gameId, GameState state);
    void updateGameSession(GameId gameId, bool migration = false, HostMigrationType migrationType = PLATFORM_HOST_MIGRATION);

    enum PostGameMode
    {
        POST_GAME_MODE_AUTO,            // after post game state change, finishes game.
        POST_GAME_MODE_CUSTOM           // pauses once game reaches post-game.  allows replay and game reporting. call finishPostGame() to end game
    };

    void setPostGameMode(PostGameMode mode) {
        mPostGameMode = mode;
    }
    void finishPostGame();
    uint32_t getCyclesLeft() { return mCyclesLeft; }
    uint32_t getPostGameCountdown() { return mPostGameCountdown; }

public:
    //  The callback should know what notification TDF to use based on the notification type.  Type-cast accordingly     
    typedef Functor1<EA::TDF::Tdf*> AsyncNotifyCb;

    void setNotificationCb(GameManagerSlave::NotificationType type, const AsyncNotifyCb& cb);
    void clearNotificationCb(GameManagerSlave::NotificationType type);

    typedef Functor1<CreateGameRequest*> CreateGameRequestCb;
    void setCreateGameRequestCallback(const CreateGameRequestCb& cb) {
        mCreateGameRequestCb = cb;
    }
    void clearIsWaitingForGameStateNotification() { mIsWaitingForGameStateNotification = false; }
    bool isWaitingForGameStateNotification() const { return mIsWaitingForGameStateNotification; }
    GameManagerUtil* getUtil() const { return mUtil; }
    
    Blaze::BlazeRpcError updateGameName();
    Blaze::BlazeRpcError updateGameExternalStatus();
protected:
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

private:
    void updateAllPlayerConnsToHost(GameId gameId, ConnectionGroupId hostConnId, BlazeId hostId);
    void updatePlayerConns(GameId gameId, ConnectionGroupId sourceConnId, ConnectionGroupId targetConnId);
    void updatePlayerConns(GameId gameId, UpdateMeshConnectionRequest request);
    void updateHostConnection(GameId gameId, ConnectionGroupId sourceGroupId, ConnectionGroupId topologyGroupId, PlayerNetConnectionStatus connStatus);

    void finalizeGameCreation(GameId gameId, bool platformHost, bool topologyHost);

    // Caller must allocate TDF.
    void scheduleCustomNotification(GameManagerSlave::NotificationType type, EA::TDF::Tdf *notification);
    void runNotificationCb(GameManagerSlave::NotificationType type, EA::TDF::TdfPtr tdf);

    // Census Data callbacks
    void censusDataSubscribe();
    void censusDataUnsubscribe();

    //  state change callback
    void newGameState();

    void setIsWaitingForGameInfo() { mIsWaitingForGameInfo = true; }
    void clearIsWaitingForGameInfo() { mIsWaitingForGameInfo = false; }
    bool isWaitingForGameInfo() const { return mIsWaitingForGameInfo; }

    void setIsWaitingForGameStateNotification() { mIsWaitingForGameStateNotification = true; }

    void clearUpdateGameNameRequest() { mUpdateGameNameRequest[0] = '\0'; }
    bool isUpdateGameNameRequestCleared() const { return (mUpdateGameNameRequest[0] == '\0'); }

    void scheduleUpdateGamePresenceForLocalUser(const GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason = SYS_PLAYER_REMOVE_REASON_INVALID);
    void updateGamePresenceForLocalUser(const GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason = SYS_PLAYER_REMOVE_REASON_INVALID);
    void populateGameActivity(GameActivity& activity, const GameData& game, const PlayerData* player);

    Blaze::ClientPlatformType getPlatform() const { return ((mStressInst != nullptr) && (mStressInst->getOwner() != nullptr) ? mStressInst->getOwner()->getPlatform() : Blaze::INVALID); }

    void setNextQosTelemetryTime(GameData& gameData);
    BlazeRpcError checkAndSendTelemetryReport();

    void randomizeLatencyData(TelemetryReport* report, GameData& gameData);

private:
    GameManagerUtil *mUtil;
#ifdef TARGET_gamereporting
    GameReportingUtilInstance *mGameReportingUtilInstance;
#endif
    StressInstance *mStressInst;
    GameManagerSlave *mGameManagerProxy;

#ifdef TARGET_arson
    Arson::ArsonSlave *mArsonSlave;
#endif

#ifdef TARGET_censusdata
    CensusData::CensusDataSlave *mCensusDataProxy;
#endif

    GameId mGameId;
    GameManagerUtil::Role mRole;
    uint32_t mCyclesLeft;
    bool mPaused;
    bool mJoinByUser;
    PostGameMode mPostGameMode;

    uint32_t mGameTimer;
    uint32_t mPostGameCountdown;
    
    // Note only handling single request per cycle.
    GameSettings *mSetGameSettingsRequest;
    char8_t mUpdateGameNameRequest[MAX_GAMENAME_LEN];

    typedef eastl::vector_map<GameManagerSlave::NotificationType, AsyncNotifyCb> NotificationCbMap;
    NotificationCbMap mNotificationCbMap;

    CreateGameRequestCb mCreateGameRequestCb;
    CreateGameRequestCb mResetGameRequestCb;

    // flag to indicate waiting for the async game info.
    bool mIsWaitingForGameInfo;
    bool mIsWaitingForGameStateNotification;
    
    // flag to indicate censusdata subscription
    bool mIsSubscribedCensusData;

    // track current primary presence game, for simulating BlazeSDK behavior. note this could be a game group
    GameId mPrimaryPresenceGameId;
    eastl::string mUpdateGameExternalStatusRequest;
    uint32_t mTotalPacketsSent; // Simulated
    uint32_t mTotalPacketsRecv; // Simulated
};

///////////////////////////////////////////////////////////////////////////////
//  Types defining games using a subset of the replicated GameManager game data.
//
class GameData;

struct PlayerData
{
    PlayerId mPlayerId;
    bool mJoinCompleted;
    ConnectionGroupId mConnGroupId;
    TeamIndex mTeamIndex;
    SlotType mSlotType;
    TimeValue mTimeAdded;

    PlayerData() : mPlayerId(0), mJoinCompleted(false), mConnGroupId(0), mTeamIndex(0), mSlotType(INVALID_SLOT_TYPE) {}
    PlayerData(const PlayerData& rhs)
    {
        *this = rhs;
    }
    PlayerData& operator=(const PlayerData& rhs)
    {
        if(&rhs != this)
        {
            mPlayerId = rhs.mPlayerId;
            mJoinCompleted = rhs.mJoinCompleted;
            mConnGroupId = rhs.mConnGroupId;
            mTeamIndex = rhs.mTeamIndex;
            mSlotType = rhs.mSlotType;
            rhs.mAttrs.copyInto(mAttrs);
            mTimeAdded = rhs.mTimeAdded;
        }
        return *this;
    }

    const Collections::AttributeMap* getAttributeMap() const {
        return &mAttrs;
    }

private:
    friend class GameData;
    friend class GameManagerUtilInstance;
    Collections::AttributeMap* getAttributeMap() {
        return &mAttrs;
    }

private:
    Collections::AttributeMap mAttrs;
};


class GameData
{
    NON_COPYABLE(GameData);
    friend class GameManagerUtilInstance;
    friend class GameManagerUtil;

public:
    GameData(GameId gameId);
    ~GameData();

    void init(ReplicatedGameData &gameData);
    void setQosTelemetryInterval(const TimeValue& qosTelemetryInterval);
    void setNewHost(PlayerId hostId, SlotId slotId, NetworkAddress *networkAddress);

    uint32_t getLocalRefCount() const {
        return mRefCount;
    }

    bool isHost(PlayerId id) const {
        return mHostId == id;
    }

    bool isPlatformHost(PlayerId id) const
    {
        return mPlatformHostId == id;
    }

    GameId getGameId() const {
        return mGameId;
    }
    GameReportingId getGameReportingId() const {
        return mGameReportingId;
    }
    void setGameReportingId(GameReportingId id)
    {
        mGameReportingId = id;
    }
    PlayerId getHostId() const {
        return mHostId;
    }

    SlotId getHostSlotId() const {
        return mHostSlotId;
    }

    ConnectionGroupId getHostConnectionGroupId() const {
        return mHostConnGroupId;
    }

    const char* getGameName() const {
        return mName;
    }
    size_t getPlayerCapacity() const {
        return mPlayerCapacity;
    }
    GameState getGameState() const {
        return mGameState;
    }
    GameState getLastGameState() const {
        return mLastGameState;
    }
    GameNetworkTopology getTopology() const {
        return mTopology;
    }
    const GameSettings* getGameSettings() const {
        return &mGameSettings;
    }
    GameManagerUtil::QueryJoinFailure queryJoinGame(bool joinInProgress) const;

    bool addPlayer(const ReplicatedGamePlayer &player);
    bool removePlayer(PlayerId playerId);

    size_t getNumPlayers() const {
        return mPlayerMap.size();
    }
    PlayerData* getPlayerData(PlayerId id) const;
    PlayerData* getPlayerDataByIndex(size_t index) const ;
    size_t getNumJoinCompleted() const {
        return mNumPlayersJoinCompleted;
    }

    typedef eastl::vector_map<PlayerId, PlayerData> PlayerMap;

    const PlayerMap &getPlayerMap() const {
        return mPlayerMap;
    }

    const Collections::AttributeMap* getAttributeMap() const {
        return &mGameAttrs;
    }

    const ExternalSessionIdentification& getExternalSessionIdentification() const { return mExternalSessionIdentification; }

    void createResetGameRequest(CreateGameRequest &req);

    uint32_t getNextPacketsSent();
    uint32_t getNextPacketsRecv();

private:
    void setGameState(GameState newState) { 
        mLastGameState = mGameState;
        mGameState = newState; 
    }
    GameSettings* getGameSettings() { 
        return &mGameSettings; 
    }
    void setGameName(const char8_t *newName) {
        blaze_strnzcpy(mName, newName, strlen(newName) + 1);
    }

    Collections::AttributeMap* getAttributeMap() {
        return &mGameAttrs;
    }

private:
    char mName[MAX_GAMENAME_LEN];
    GameId mGameId;
    GameReportingId mGameReportingId;
    PlayerId mHostId;
    SlotId mHostSlotId;
    ConnectionGroupId mHostConnGroupId;
    PlayerId mPlatformHostId;
    size_t mPlayerCapacity;
    GameState mGameState;
    GameState mLastGameState;
    GameSettings mGameSettings;
    GameNetworkTopology mTopology;

    PlayerMap mPlayerMap;
    size_t mPendingEntryCount;
    size_t mNumPlayersJoinCompleted;
    uint32_t mRefCount;

    Collections::AttributeMap mGameAttrs;
    NetworkAddressList mHostAddressList;

    ExternalSessionIdentification mExternalSessionIdentification;

    TimeValue mQosTelemetryInterval;
    TimeValue mNextTelemetryTime;
    uint32_t mPacketsSent;
    uint32_t mPacketsRecv;
};


}   // Stress
}   // Blaze


#endif
