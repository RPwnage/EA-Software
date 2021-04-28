/*! ************************************************************************************************/
/*!
    \file gamesessionmetrics.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_SESSION_METRICS_H
#define BLAZE_GAMEMANAGER_GAME_SESSION_METRICS_H


namespace Blaze
{

class GameSessionMaster;
namespace GameManager
{
class GameSessionMetricsHandler
{
    NON_COPYABLE(GameSessionMetricsHandler);
public:
    
    GameSessionMetricsHandler();
    ~GameSessionMetricsHandler();

    void setGameSessionMaster(GameSessionMaster& session);

    void onGameStarted();
    void onGameCreated();
    void onDedicatedGameHostInitialized(const GameSettings& gameSettings);
    void onDedicatedGameHostReturnedToPool();
    void onDedicatedGameHostReset();
    void onGameModeOrPlatformChanged(const char8_t* oldGameMode, const char8_t* newGameMode, const char8_t* oldPlatform, const char8_t* newPlatform);
    void onPingSiteChanged(const char8_t* oldPingSite, const char8_t* newPingSite);
    void onPlayerCapacityUpdated(uint16_t oldTotalParticipantCapacity, uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity);
    void onGameDestroyed(bool updateTotals, GameDestructionReason destructionReason);
    void onGameFinished();
    void onHostEjected();
    void onPlayerAdded(const PlayerInfoMaster& newPlayer, bool claimedReservation, bool updateTotals = true);
    void onPlayerRemoved(const PlayerInfoMaster& leavingPlayer, const PlayerRemovedReason* playerRemovedReason = nullptr, bool updateTotals = true);
    void onPlayerReservationAdded(const PlayerInfoMaster& newPlayer, bool wasActive, bool updateTotals = true);
    void onPlayerReservationClaimFailed(const PlayerInfoMaster* player, bool failedGameEntryCriteria);
    void onGameSettingsChanged(const GameSettings& oldSettings, const GameSettings& newSettings);
    void onGameResponsivenessChanged(bool responsive);
  
    void rebuildGameMetrics();
    void teardownGameMetrics();
    void teardownPlayerMetrics(const PlayerInfoMaster& leavingPlayer);

    void calcPlayerSlotUpdatedMetrics(uint16_t oldTotalParticipantCapacity, uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity);

    static const char8_t* getGameModeAsMetric(const char8_t* gameMode);

private:
    void incrementActiveDedicatedServerMetrics();
    void decrementActiveDedicatedServerMetrics();

    void incrementActiveVirtualGames();
    void decrementActiveVirtualGames();

    void incrementVirtualGames();
    void decrementVirtualGames();

    void calcGameCreatedMetrics(bool updateTotals = true);
    void calcGameDestroyedMetrics(bool updateTotals, GameDestructionReason destructionReason);
    void calcSlotUpdatedMetrics(uint16_t oldTotalPlayerCapacity, uint16_t newTotalPlayerCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity, bool activeSlots);
    void calcActiveSlotUpdatedMetrics(uint16_t oldTotalParticipantCapacity, uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity);
    void calcPlayerRemovedReasonMetrics(PlayerRemovedReason playerRemovedReason, int64_t playerSessionDurationSecs, const PlayerInfoMaster& leavingPlayer, bool playerInQoSMMDataMap);
    void calcPlayerRemoveScenarioMetrics(PlayerRemovedReason playerRemovedReason, const PlayerInfoMaster& leavingPlayer, int64_t playerSessionDurationSecs, TimeValue removedTimestamp);
    void calcPlayerRemoveGameMetrics(PlayerRemovedReason playerRemovedReason, const PlayerInfoMaster& leavingPlayer, TimeValue leavingTimestamp);
    void calcClaimedReservationMetrics(const PlayerInfoMaster& newPlayer);
    void calcPlayerMetrics(const char8_t* oldGameMode, const char8_t* newGameMode, const char8_t* oldPingSite, const char8_t* newPingSite);
    void decrementGamesByReputation(bool allowAnyReputation);
    void incrementGamesByReputation(bool allowAnyReputation);

private:
    const ReplicatedGameData& getGameData() const; 
    const char8_t* getGameMode() const;
    const eastl::string getPlatformForMetrics() const;
    bool isActiveGame() const;
    // we track some events in time "buckets" defined here
    // Ex: num players who left in first min of play vs players who left after 5 min of play.
    static const int MAX_THRESHOLD = 4;
    static const int TIME_THRESHOLD[MAX_THRESHOLD];

    const GameSessionMaster* mGameSessionMaster;
    bool mGameCreateMetricsSet; // Non-persistent state. 
    bool mActiveSlotMetricsSet; // Non-persistent state. 
    bool mAllSlotMetricsSet; // Non-persistent state. 
};

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_GAME_SESSION_METRICS_H

