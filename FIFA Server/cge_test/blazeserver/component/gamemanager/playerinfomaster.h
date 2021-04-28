/*! ************************************************************************************************/
/*!
    \file playerinfomaster.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_INFO_MASTER_H_
#define BLAZE_GAMEMANAGER_PLAYER_INFO_MASTER_H_

#include "EASTL/functional.h"
#include "EASTL/vector_set.h"
#include "playerinfo.h"


namespace Blaze 
{
namespace GameManager 
{
    class GameManagerMasterImpl;
    class JoinGameRequest;
    class GameSessionMaster;

    class CensusEntry
    {
    public:
        uint32_t gameCount;
        uint32_t playerCount;
        CensusEntry() : gameCount(0), playerCount(0) {}
    };

    typedef eastl::vector_set<CensusEntry*> CensusEntrySet;

    class PlayerInfoMaster : public PlayerInfo
    {
    public:
        PlayerInfoMaster(const GameSessionMaster& gameSessionMaster, PlayerState state, SlotType slotType, TeamIndex teamIndex, const RoleName& roleName, 
            const UserGroupId& groupId, const UserSessionInfo& joinInfo, const char8_t* encryptedBlazeId);

        //create player from join request (may be reserved or non reserved)
        PlayerInfoMaster(const JoinGameMasterRequest& request, const GameSessionMaster& gameSessionMaster);

        PlayerInfoMaster(const PlayerInfoMaster& player);
        
        PlayerInfoMaster(PlayerDataMaster& playerDataMaster, ReplicatedGamePlayerServer& playerData);

        ~PlayerInfoMaster() override;

        void setNetworkAddress(const NetworkAddress& networkInfo) { networkInfo.copyInto(mPlayerData->getUserInfo().getNetworkAddress()); }
        void setPlayerAttributes(const Collections::AttributeMap &updatedPlayerAttributes, Collections::AttributeMap *changedPlayerAttributes = nullptr);
        void setCustomData(const EA::TDF::TdfBlob *customData) { customData->copyInto(mPlayerData->getCustomData()); }
        void setupJoiningPlayer(const JoinGameMasterRequest& request);
        void setSlotId(SlotId slotId) { mPlayerData->setSlotId(slotId); }
        void setSlotType(SlotType slotType) { mPlayerData->setSlotType(slotType); }
        void setTeamIndex(TeamIndex teamIndex) { mPlayerData->setTeamIndex(teamIndex); }
        void setRoleName(const char8_t* role) { mPlayerData->setRoleName(role); }
        void setTargetPlayerId(PlayerId playerId) { mPlayerData->setTargetPlayerId(playerId); }

        void setJoinedGameTimeStamp();

        void startJoinGameTimer();
        void cancelJoinGameTimer();
        void resetJoinGameTimer();

        void startReservationTimer(const TimeValue& origStartTime = 0, bool disconnect = false);
        void cancelReservationTimer();
        TimeValue getReservationTimerStart() const { return mPlayerDataMaster->getReservationTimerStart(); }

        void startPendingKickTimer(TimeValue timeoutDuration);
        void cancelPendingKickTimer();

        void startQosValidationTimer();
        void cancelQosValidationTimer();
        
        bool isKickPending() const { return (getPlayerState() == ACTIVE_KICK_PENDING); }
        bool isJoinCompletePending() const {return mPlayerDataMaster->getJoinGameTimeout() != 0; }        

        bool getNeedsEntryCriteriaCheck() const { return mPlayerDataMaster->getNeedsEntryCriteriaCheck(); }
        void setNeedsEntryCriteriaCheck() { mPlayerDataMaster->setNeedsEntryCriteriaCheck(true); }

        UserSessionId getOriginalPlayerSessionId() const { return mPlayerDataMaster->getOriginalPlayerSessionId(); }
        ConnectionGroupId getOriginalConnectionGroupId() const { return mPlayerDataMaster->getOriginalConnectionGroupId(); }
        UserGroupId getOriginalUserGroupId() const { return mPlayerDataMaster->getOriginalUserGroupId(); }

        SlotId getQueuedIndexWhenPromoted() const { return mPlayerDataMaster->getQueuedIndexWhenPromoted(); }
        void setQueuedIndexWhenPromoted(SlotId queuedIndex) { mPlayerDataMaster->setQueuedIndexWhenPromoted(queuedIndex); }
        JoinMethod getJoinMethod() const { return mPlayerDataMaster->getJoinMethod(); }
        void setJoinMethod(JoinMethod method) { mPlayerDataMaster->setJoinMethod(method); }
        void setScenarioInfo(const ScenarioInfo& scenarioInfo)
        {
            scenarioInfo.copyInto(mPlayerDataMaster->getScenarioInfo());
            mPlayerData->setScenarioName(scenarioInfo.getScenarioName());
        }

        void setAccountId(AccountId id) { mPlayerData->getUserInfo().getUserInfo().getPlatformInfo().getEaIds().setNucleusAccountId(id); }
        void setGameEntryType(GameEntryType gameEntryType) { mPlayerDataMaster->setGameEntryType(gameEntryType); }

        bool isUsingGamePresence() const { return mPlayerDataMaster->getIsUsingGamePresence(); }
        void setInQueue(bool queued) { mPlayerData->setInQueue(queued); }
        bool inQueue() const { return mPlayerData->getInQueue(); }

        Collections::AttributeMap& getPlayerAttribs() { return mPlayerData->getPlayerAttribs(); }

        const PlayerDataMaster* getPlayerDataMaster() const { return mPlayerDataMaster; }
        PlayerDataMaster* getPlayerDataMaster() { return mPlayerDataMaster; } // used only by GameManagerMasterImpl::upsertPublicPlayerData

        ConnectionJoinType getConnectionJoinType() const;

        typedef eastl::pair<GameId, PlayerId> GamePlayerIdPair;
        typedef eastl::pair<GameId, ConnectionGroupId> GameConnectionGroupIdPair;
        static void onJoinGameTimeout(GamePlayerIdPair idPair); // called by TimerSet from GameManagerMasterImpl
        static void onPendingKick(GamePlayerIdPair idPair); // called by TimerSet from GameManagerMasterImpl
        static void onReservationTimeout(GamePlayerIdPair idPair); // called by TimerSet from GameManagerMasterImpl
        static void onQosValidationTimeout(GamePlayerIdPair idPair); // called by TimerSet from GameManagerMasterImpl
        static void onUpdateMeshConnectionTimeout(GameConnectionGroupIdPair idPair); // called by TimerSet from GameManagerMasterImpl
        void setFinishedFirstMatch() { mPlayerDataMaster->setFinishedFirstMatch(true); }
        void setDurationInFirstMatchesSec(int64_t matchDuration) { mPlayerDataMaster->setDurationInFirstMatchesSec(matchDuration); }
        void addToCensusEntry(CensusEntry& entry);
        void removeFromCensusEntries();
        const char8_t* getProtoTunnelVer() const {return mPlayerDataMaster->getProtoTunnelVer();}
    private:
        PlayerDataMasterPtr mPlayerDataMaster;
        CensusEntrySet mCensusEntrySet; // Non-persistent cached state, intentionally not written to redis.
  
    }; // class PlayerInfoMaster

    void intrusive_ptr_add_ref(PlayerInfoMaster* ptr);
    void intrusive_ptr_release(PlayerInfoMaster* ptr);
    
    typedef eastl::intrusive_ptr<PlayerInfoMaster> PlayerInfoMasterPtr;

}  // namespace GameManager
}  // namespace Blaze

namespace eastl
{
    // Template specialization to enable TimerSet to hash GamePlayerIdPair objects, PlayerIds are always unique within a TimerSet, so we use that
    template <> 
    struct hash<Blaze::GameManager::PlayerInfoMaster::GamePlayerIdPair>
    {
        size_t operator()(Blaze::GameManager::PlayerInfoMaster::GamePlayerIdPair p) const { return static_cast<size_t>(p.second); }
    };

    // Template specialization to enable TimerSet to hash GameConnectionGroupIdPair objects, ConnectionGroupIds are always unique within a TimerSet, so we use that
    template <> 
    struct hash<Blaze::GameManager::PlayerInfoMaster::GameConnectionGroupIdPair>
    {
        size_t operator()(Blaze::GameManager::PlayerInfoMaster::GameConnectionGroupIdPair p) const { return static_cast<size_t>(p.second); }
    };
}

#endif /*BLAZE_GAMEMANAGER_PLAYER_INFO_MASTER_H_*/
