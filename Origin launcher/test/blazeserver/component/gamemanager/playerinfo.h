/*! ************************************************************************************************/
/*!
    \file playerinfo.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_INFO_H_
#define BLAZE_GAMEMANAGER_PLAYER_INFO_H_

#include "gamemanager/tdf/gamemanager.h"
#include "util/tdf/utiltypes.h"

#include "framework/replication/replicatedmap.h"
#include "gamemanager/rpc/gamemanagerslave_stub.h" // needed for replication friendship below
#include "gamemanager/tdf/gamemanager_server.h"  // for UserSessionInfo

namespace Blaze 
{
namespace GameManager 
{
    /*! ************************************************************************************************/
    /*! \brief return true if the given SlotType is a spectator slot
    ***************************************************************************************************/
    inline bool isSpectatorSlot(SlotType slotType)
    {
        return (slotType >= SLOT_PUBLIC_SPECTATOR) && (slotType < MAX_SLOT_TYPE);
    }

    /*! ************************************************************************************************/
    /*! \brief return true if the given SlotType is a participant slot
    ***************************************************************************************************/
    inline bool isParticipantSlot(SlotType slotType)
    {
        return (slotType < MAX_PARTICIPANT_SLOT_TYPE) && (slotType >= SLOT_PUBLIC_PARTICIPANT);
    }


    /*! ************************************************************************************************/
    /*! \brief return the slot full error code depending on the slot type(participant vs spectator)
    ***************************************************************************************************/
    inline BlazeRpcError getSlotFullErrorCode(SlotType slotType)
    {
        return isParticipantSlot(slotType) ? GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL : GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL;
    }

    // struct to hold context about the reason a player is removed.
    //   data passed down to the player's replicated map removal reason tdf
    struct PlayerRemovalContext
    {
        PlayerRemovalContext(PlayerRemovedReason reason, PlayerRemovedTitleContext titleContext, const char8_t* titleContextString) :
            mPlayerRemovedReason(reason), mPlayerRemovedTitleContext(titleContext), mTitleContextString(titleContextString)
        {
        }

        PlayerRemovedReason mPlayerRemovedReason;
        PlayerRemovedTitleContext mPlayerRemovedTitleContext;
        const char8_t* mTitleContextString;
    };


    class PlayerInfo
    {
        NON_COPYABLE(PlayerInfo);

    public:

        GameId getGameId() const { return mPlayerData->getGameId(); }
        PlayerId getPlayerId() const { return mPlayerData->getUserInfo().getUserInfo().getId(); }
        ExternalId getExternalId() const { return getExternalIdFromPlatformInfo(mPlayerData->getUserInfo().getUserInfo().getPlatformInfo()); } // DEPRECATED
        AccountId getAccountId() const { return mPlayerData->getUserInfo().getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId(); };
        AccountId getUserId() const { return getAccountId(); }  // DEPRECATED
        Locale getAccountLocale() const { return mPlayerData->getUserInfo().getUserInfo().getAccountLocale(); }
        uint32_t getAccountCountry() const { return mPlayerData->getUserInfo().getUserInfo().getAccountCountry(); }
        const char8_t* getPlayerName() const { return mPlayerData->getUserInfo().getUserInfo().getPersonaName(); }
        const char8_t* getPlayerNameSpace() const { return mPlayerData->getUserInfo().getUserInfo().getPersonaNamespace(); }
        const PlatformInfo& getPlatformInfo() const { return mPlayerData->getUserInfo().getUserInfo().getPlatformInfo(); }
        ClientPlatformType getClientPlatform() const { return mPlayerData->getUserInfo().getUserInfo().getPlatformInfo().getClientPlatform(); }
        const char8_t* getUUID() const { return mPlayerData->getUserInfo().getUUID(); }
        SlotId getSlotId() const { return mPlayerData->getSlotId(); }
        SlotType getSlotType() const { return mPlayerData->getSlotType(); }
        TeamIndex getTeamIndex() const { return mPlayerData->getTeamIndex(); }
        const char8_t* getRoleName() const { return mPlayerData->getRoleName(); }
        UserGroupId getUserGroupId() const { return mPlayerData->getUserGroupId();}
        PlayerState getPlayerState() const { return mPlayerData->getPlayerState(); }
        const EA::TDF::TdfBlob* getCustomData() const { return &mPlayerData->getCustomData(); }
        const NetworkAddress* getNetworkAddress() const { return &mPlayerData->getUserInfo().getNetworkAddress(); }
        ClientType getClientType() const { return mPlayerData->getUserInfo().getClientType(); }
        const ClientMetrics* getClientMetrics() const { return &mPlayerData->getUserInfo().getClientMetrics(); }
        const ClientUserMetrics* getClientUserMetrics() const { return &mPlayerData->getUserInfo().getClientUserMetrics(); }
        UserSessionId getPlayerSessionId() const { return mPlayerData->getUserInfo().getSessionId(); }
        const Collections::AttributeMap& getPlayerAttribs() const { return mPlayerData->getPlayerAttribs(); }
        const PlayerSettings& getPlayerSettings() const { return mPlayerData->getPlayerSettings(); }
        const char8_t* getPlayerAttrib(const char8_t* key) const;
        const TimeValue getJoinedGameTimestamp() const { return mPlayerData->getJoinedGameTimestamp(); }
        const TimeValue getReservationCreationTimestamp() const { return mPlayerData->getReservationCreationTimestamp(); }
        bool getJoinedViaMatchmaking() const { return mPlayerData->getJoinedViaMatchmaking(); }
        const char8_t* getConnectionAddr() const { return mPlayerData->getUserInfo().getConnectionAddr(); }
        ConnectionGroupId getConnectionGroupId() const { return mPlayerData->getUserInfo().getConnectionGroupId(); }
        SlotId getConnectionSlotId() const { return mPlayerData->getConnectionSlotId(); }
        PlayerId getTargetPlayerId() const { return mPlayerData->getTargetPlayerId(); }
        void setConnectionGroupId(ConnectionGroupId connGroupId) { mPlayerData->getUserInfo().setConnectionGroupId(connGroupId); }
        void setPlayerState(PlayerState state);
        bool hasHostPermission() const { return mPlayerData->getUserInfo().getHasHostPermission(); }
        const char8_t* getServiceName() const { return mPlayerData->getUserInfo().getServiceName(); }
        const char8_t* getProductName() const { return mPlayerData->getUserInfo().getProductName(); }
        const PingSiteLatencyByAliasMap& getLatencyMap() const { return mPlayerData->getUserInfo().getLatencyMap(); }
        const UserExtendedDataMap& getExtendedDataMap() const { return mPlayerData->getUserInfo().getDataMap(); }
        const Util::NetworkQosData&  getQosData() const { return mPlayerData->getUserInfo().getQosData(); }
        int32_t getDirtySockUserIndex() const { return mPlayerData->getUserInfo().getDirtySockUserIndex(); }
        const UserSessionInfo& getUserInfo() const { return mPlayerData->getUserInfo(); }
        UserSessionInfo& getUserInfo() { return mPlayerData->getUserInfo(); }

        bool isParticipant() const;
        bool isSpectator() const;

        bool getSessionExists() const;
        static bool getSessionExists(const UserSessionInfo &userSessionInfo);
             
        struct SessionIdGetFunc
        {
            UserSessionId operator()(const PlayerInfo *u) { return u->getPlayerSessionId(); }
        };

        // game state helpers
        bool isActive() const { return (mPlayerData->getPlayerState() == ACTIVE_CONNECTING) || (mPlayerData->getPlayerState() == ACTIVE_CONNECTED) || (mPlayerData->getPlayerState() == ACTIVE_MIGRATING) || (mPlayerData->getPlayerState() == ACTIVE_KICK_PENDING); }
        bool isConnected() const { return (mPlayerData->getPlayerState() == ACTIVE_CONNECTED); }
        bool isQueuedNotReserved() const { return (mPlayerData->getPlayerState() == QUEUED); }   // QUEUED Players are not reserved  (check isInQueue to find reseved queued players)
        bool isReserved() const { return (mPlayerData->getPlayerState() == RESERVED); }   // RESERVED Players may be in Queue (check isInQueue to find out)
        bool isInRoster() const { return !mPlayerData->getInQueue(); } // NOTE: PlayerState is not sufficent to determine player roster membership
        bool isInQueue() const { return mPlayerData->getInQueue(); }

        void fillOutReplicatedGamePlayer(ReplicatedGamePlayer& player, bool hideNetworkAddress = false) const;

        bool hasFirstPartyId() const;
        bool hasExternalPlayerId() const;

        // NOTE: This class should only be instantiated on the slave (and only by the replicator).  The master uses a derived class, PlayerInfoMaster
        PlayerInfo(ReplicatedGamePlayerServer* playerData = nullptr) : mRefCount(0), mPlayerData(playerData) {}
        virtual ~PlayerInfo() {}

        ReplicatedGamePlayerServer* getPlayerData() { return mPlayerData.get(); }
        const ReplicatedGamePlayerServer* getPlayerData() const { return mPlayerData.get(); }
        const ReplicatedGamePlayerServer* getPrevSnapshot() const { return mPrevSnapshot.get(); }

        void updateReplicatedPlayerData(GameManager::ReplicatedGamePlayerServer& playerData);
        void discardPrevSnapshot() { mPrevSnapshot.reset(); }

    protected:
        friend void intrusive_ptr_add_ref(PlayerInfo* ptr);
        friend void intrusive_ptr_release(PlayerInfo* ptr);

        uint32_t mRefCount;
        ReplicatedGamePlayerServerPtr mPlayerData;
        ReplicatedGamePlayerServerPtr mPrevSnapshot;
    }; // class PlayerInfo

    typedef eastl::intrusive_ptr<PlayerInfo> PlayerInfoPtr;

    void intrusive_ptr_add_ref(PlayerInfo* ptr);
    void intrusive_ptr_release(PlayerInfo* ptr);

    typedef eastl::hash_map<PlayerId, PlayerInfoPtr> PlayerInfoMap;

} // namespace GameManager
} // namespace Blaze
#endif /*BLAZE_GAMEMANAGER_PLAYER_INFO_H_*/
