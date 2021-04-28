/*! ************************************************************************************************/
/*!
    \file player.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_H
#define BLAZE_GAMEMANAGER_PLAYER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/jobid.h"
#include "BlazeSDK/mesh.h"
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#include "BlazeSDK/gamemanager/playerbase.h"
#include "BlazeSDK/gamemanager/game.h"


namespace Blaze
{

namespace GameManager
{
    class GameManagerAPI;
    class GameEndpoint;

    //! \brief sentinel value for an invalid index in a game's player queue.  Returned by some queue methods on the Player or Game.
    const uint16_t INVALID_QUEUE_INDEX = UINT16_MAX;

    /*! **********************************************************************************************************/
    /*! \class Player
    **************************************************************************************************************/
    class BLAZESDK_API Player : public MeshMember, public PlayerBase 
    {
    public:
        typedef Functor3<BlazeError, Player *, JobId> ChangePlayerAttributeJobCb;
        typedef Functor3<BlazeError, Player *, JobId> ChangePlayerCustomDataJobCb;
        typedef Functor3<BlazeError, Player *, JobId> ChangePlayerTeamAndRoleJobCb;
        typedef Functor2<BlazeError, Player *> ChangePlayerAttributeCb; // DEPRECATED
        typedef Functor2<BlazeError, Player *> ChangePlayerCustomDataCb; // DEPRECATED
        typedef Functor2<BlazeError, Player *> ChangePlayerTeamAndRoleCb; // DEPRECATED

    /*! **********************************************************************************************************/
    /*! \name General accessors
    **************************************************************************************************************/

        /*! **********************************************************************************************************/
        /*! \brief Returns the game this player is attached to. 
        **************************************************************************************************************/
        Game* getGame() const { return mGame; }

       

        /*! **********************************************************************************************************/
        /*! \brief Returns the type of slot this player occupies
            \return    The player's slot type.
        **************************************************************************************************************/
        SlotType getSlotType() const { return mSlotType; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if this is the primary local player for the gamemanagerApi instance.
            \return True if this is the primary local player
        ***************************************************************************************************/
        bool isPrimaryLocalPlayer() const;

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this is the network host for this mesh instance.
            \return    True if this is the network host.
        **************************************************************************************************************/
        bool isHost() const;

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this is the admin player for this gameManagerApi instance.
            \return    True if this is an admin player.
        **************************************************************************************************************/
        bool isAdmin() const { return mGame->isAdmin(mId);}

        /*! **********************************************************************************************************/
        /*! \brief Returns the index of the player within the queue, while waiting to join game.
            \return    The index of the player in the queue.  Returns INVALID_QUEUE_INDEX if the player is not in the queue.
        **************************************************************************************************************/
        uint16_t getQueueIndex() const;

      
        /*! ************************************************************************************************/
        /*! \brief get the player's team index
        \return the player's team id, INVALID_TEAM_INDEX if the player isn't in a team. (Spectators)
        ***************************************************************************************************/
        TeamId getTeamId() const { return mGame->getTeamIdByIndex(mTeamIndex); }

    /*! **********************************************************************************************************/
    /*! \name MeshMember Interface Implementation

    Players implement the MeshMember interface so they can interact with a NetworkMeshAdapter instance.
    The Mesh interface corresponds to the game, a MeshEndpoint to a console, while the MeshMember interface corresponds to a Player.
        
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Returns the game this player is attached to (as a Mesh interface pointer).  
        ***************************************************************************************************/
        virtual const Mesh* getMesh() const { return mGame; }

        /*! ************************************************************************************************/
        /*! \brief Returns the mesh endpoint this player is attached to. Users on the same machine will share a MeshEndpoint object.
        ***************************************************************************************************/
        virtual const MeshEndpoint* getMeshEndpoint() const;

        /*! ************************************************************************************************/
        /*! \brief return the Id of this player.  Note: the PlayerId is the same type as a 
                BlazeId and uniquely identifies a player in a specific game.
        ***************************************************************************************************/
        virtual BlazeId getId() const { return mId; }

         /*! ************************************************************************************************/
        /*! \brief return the name of a player
        ***************************************************************************************************/
        virtual const char8_t* getName() const { return mUser->getName(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this is the local player for this gameManagerApi instance.
            \return    True if this is the local player.
        **************************************************************************************************************/
        bool isLocalPlayer() const;
        virtual bool isLocal() const {return isLocalPlayer();}

        /*! **********************************************************************************************************/
        /*! \brief Lookup from the Usermanager if the user is found (ie. the user is local), and save to the cache.
        **************************************************************************************************************/
        void cacheLocalPlayerStatus();

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this is the platform host player of the game
            \return    True if this is the platform host of the game.
        **************************************************************************************************************/
        virtual bool isPlatformHost() const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the local user index of the player (for their console).
            \return    The the local user index of the player (for their console).
        **************************************************************************************************************/
        virtual uint32_t getLocalUserIndex() const { return mDirtySockUserIndex; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the player slot assignment for this player.
            \return    The player's slot id.
        **************************************************************************************************************/
        virtual SlotId getSlotId() const { return mSlotId; }

        /*! ************************************************************************************************/
        /*! \brief returns the endpoint slot type (SLOT_PUBLIC_PARTICIPANT, SLOT_PRIVATE_PARTICIPANT, ...)
        ***************************************************************************************************/
        virtual SlotType getMeshMemberSlotType() const { return getSlotType(); }

   /*! **********************************************************************************************************/
   /*! End MeshMember Interface Implementation
   **************************************************************************************************************/

         /*! **********************************************************************************************************/
        /*! \brief Returns the player's platform dependent network address.
        **************************************************************************************************************/
        const NetworkAddress* getNetworkAddress() const;

           /*! ************************************************************************************************/
        /*! \brief returns the connection group id.  A unique id per connection to the blaze server
            which will be equal for multiple players/members connected to Blaze on the same connection.
        ***************************************************************************************************/
        uint64_t getConnectionGroupId() const;

        /*! ************************************************************************************************/
        /*! \brief returns the slot id of the connection group
        ***************************************************************************************************/
        uint8_t getConnectionSlotId() const;

        /*! ************************************************************************************************/
        /*! \brief returns the connection group blaze object id.  A unique id per connection to the blaze server
            which will be equal for multiple players/members connected to Blaze on the same connection.
        ***************************************************************************************************/
        const ConnectionGroupObjectId getConnectionGroupObjectId() const { return EA::TDF::ObjectId(ENTITY_TYPE_CONN_GROUP, (Blaze::EntityId)getConnectionGroupId()); }

    /*! **********************************************************************************************************/
    /*! \name Player Attributes
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief create/update a single attribute for this player (sends RPC to server)

            If no attribute exists with the supplied name, one is created; otherwise the existing value
                is updated.

            Note: use setPlayerAttributeMap to update multiple attributes in a single RPC.

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            \param[in] attributeName the attribute name to set; case insensitive, must be < 32 characters
            \param[in] attributeValue the value to set the attribute to; must be < 256 characters
            \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
            \return JobId for pending action.
        ***************************************************************************************************/
        JobId setPlayerAttributeValue(const char8_t* attributeName, const char8_t* attributeValue, const ChangePlayerAttributeJobCb &callbackFunctor);
        JobId setPlayerAttributeValue(const char8_t* attributeName, const char8_t* attributeValue, const ChangePlayerAttributeCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  create/update multiple attributes for this player in a single RPC.  NOTE: the server merges existing
                attributes with the supplied attributeMap (so omitted attributes retain their previous values).
            
            If no attribute exists with the supplied name, one is created; otherwise the existing value
            is updated.

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            \param[in] changingAttributes an attributeMap containing attributes to create/update on the server.
            \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
            \return JobId for pending action.
        **************************************************************************************************************/
        JobId setPlayerAttributeMap(const Collections::AttributeMap* changingAttributes, const ChangePlayerAttributeJobCb &callbackFunctor);
        JobId setPlayerAttributeMap(const Collections::AttributeMap* changingAttributes, const ChangePlayerAttributeCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief init/update custom data blob for this player (sends RPC to server)
            
            \param[in] cBlob a blob containing player custom data to init/update on the server
            \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setCustomData(const EA::TDF::TdfBlob* cBlob, const ChangePlayerCustomDataJobCb &callbackFunctor);
        JobId setCustomData(const EA::TDF::TdfBlob* cBlob, const ChangePlayerCustomDataCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief init/update custom data blob for this player (sends RPC to server)

            \param[in] buffer a buffer containing raw player custom data to init/update on the server
            \param[in] bufferSize size of the raw player custom data to init/update on the server
            \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setCustomData(const uint8_t* buffer, uint32_t bufferSize, const ChangePlayerCustomDataJobCb &callbackFunctor);
        JobId setCustomData(const uint8_t* buffer, uint32_t bufferSize, const ChangePlayerCustomDataCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief set a player's team index(sends RPC to server)

        \param[in] teamIndex the teamIndex to set for the player
        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setPlayerTeamIndex(TeamIndex teamIndex, const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId setPlayerTeamIndex(TeamIndex teamIndex, const ChangePlayerTeamAndRoleCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief set a player's role(sends RPC to server)

        \param[in] roleName the RoleName to set for the player
        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setPlayerRole(const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId setPlayerRole(const RoleName& roleName, const ChangePlayerTeamAndRoleCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief set a player's team index and role(sends RPC to server)

        \param[in] teamIndex the teamIndex to set for the player
        \param[in] roleName the RoleName to set for the player
        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setPlayerTeamIndexAndRole(TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId setPlayerTeamIndexAndRole(TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief set a player's slot type, team index and role(sends RPC to server)
           
           NOTE: If this function is used to change from a spectator to a participant, team index 0, and the default role name will be used,
                 Call setPlayerSlotTypeTeamAndRole or promoteSpectator if you want to 

        \param[in] slotType the slotType to set for the player
        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setPlayerSlotType(SlotType slotType, const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId setPlayerSlotType(SlotType slotType, const ChangePlayerTeamAndRoleCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief set a player's slot type, team index and role(sends RPC to server)

        \param[in] slotType the slotType to set for the player
        \param[in] teamIndex the teamIndex to set for the player - Only needed when promoting a spectator
        \param[in] roleName the RoleName to set for the player   - Only needed when promoting a spectator
        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId setPlayerSlotTypeTeamAndRole(SlotType slotType, TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId setPlayerSlotTypeTeamAndRole(SlotType slotType, TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief Helper function that converts a spectator into a participant with the same public/private setting. 
                   Identical to setPlayerTeamIndexAndRole if the player is already a participant. 
                   If teams and roles are not being used, pass team index 0 and PLAYER_ROLE_NAME_DEFAULT in as params. 

        \param[in] teamIndex the teamIndex to set for the player
        \param[in] roleName the RoleName to set for the player
        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId promoteSpectator(TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId promoteSpectator(TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief Helper function that transition from participant to spectator with the same public/private setting.
                   Identical to setPlayerSlotType if the player is already a spectator. 

        \param[in] callbackFunctor this callback is dispatched when we get the RPC result from the server.
        ***************************************************************************************************/
        JobId becomeSpectator(const ChangePlayerTeamAndRoleJobCb &callbackFunctor);
        JobId becomeSpectator(const ChangePlayerTeamAndRoleCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief Returns the pointer previously set by setTitleContextData.
        ***************************************************************************************************/
        void *getTitleContextData() const { return mTitleContextData; }

        /*! ************************************************************************************************/
        /*! \brief Set the private data pointer the Title would like to associat with this object instance.

            \param[in] titleContextData a pointer to any title specific data
        ***************************************************************************************************/
        void setTitleContextData(void *titleContextData) { mTitleContextData = titleContextData; }

        /*! ****************************************************************************/
        /*! \brief Gets the QOS statistics for a player

            \param qosData a struct to contain the qos values
            \return a bool true, if the call was successful
        ********************************************************************************/
        bool getQosStatisticsForPlayer(Blaze::BlazeNetworkAdapter::MeshEndpointQos &qosData, bool bInitialQosStats = false) const;

        /*! ************************************************************************************************/
        /*! \name JoinedGameTimestamp
        \brief join timestamp, TimeValue::getTimeOfDay::getMicroseconds when joined in the master
        ***************************************************************************************************/
        inline int64_t getJoinedGameTimestamp() const { return mJoinedGameTimestamp.getMicroSeconds(); }

        /*! ************************************************************************************************/
        /*! \brief Returns true if this player is queued to enter the game instead of actually in the game.
            \return    True if this is an queued player.
        ***************************************************************************************************/
        virtual bool isQueued() const;

        /*! ************************************************************************************************/
        /*! \brief Returns the name of the Scenario used by this player if it joined via one ("" otherwise)
        ***************************************************************************************************/
        const ScenarioName& getScenarioName() const { return mScenarioName; }

    protected:

        /*! ************************************************************************************************/
        /*! \brief Returns the mesh endpoint this player is attached to. Users on the same machine will share a MeshEndpoint object.
        ***************************************************************************************************/
        GameEndpoint* getPlayerEndpoint() { return mPlayerEndpoint; }

        /*! ************************************************************************************************/
        /*! \brief Updates the mesh endpoint this player is attached to. Typically used when claiming a reservation.
                Caller is responsible for cleaning up old endpoint, if appropriate.
        ***************************************************************************************************/
        void setPlayerEndpoint(GameEndpoint *newEndpoint);

        void internalSetPlayerAttributeCb(BlazeError error, JobId id, ChangePlayerAttributeJobCb titleCb);
        void onNotifyPlayerAttributeChanged(const Collections::AttributeMap *newAttributeMap);

        void onNotifyGamePlayerStateChanged(const PlayerState state);

        void internalSetPlayerTeamCb(const SwapPlayersErrorInfo *swapPlayersError, BlazeError error, JobId id, ChangePlayerTeamAndRoleJobCb titleCb);
        void onNotifyGamePlayerTeamChanged(const TeamIndex newTeamIndex);
        void onNotifyGamePlayerRoleChanged(const RoleName& newPlayerRole);
        void onNotifyGamePlayerSlotChanged(const SlotType& newSlotType);

        void internalSetPlayerCustomDataCb(BlazeError error, JobId id, ChangePlayerCustomDataJobCb titleCb);
        void onNotifyPlayerCustomDataChanged(const EA::TDF::TdfBlob *customData);

        void onNotifyQueueChanged(const SlotId queueIndex);

        bool getHasJoinFirstPartyGameSessionPermission() const { return mPlayerSettings.getHasJoinFirstPartyGameSessionPermission(); }

        bool getHasDisconnectReservation() const { return mPlayerSettings.getHasDisconnectReservation(); }

    public:
        void helperCallback(BlazeError err, Player* game, JobId jobId);

    private:
        friend class Game;
        friend class MemPool<Player>;
        
        NON_COPYABLE(Player); // disable copy ctor & assignment operator.

        Player(Game* game, GameEndpoint *endpoint, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        ~Player();

    private:
        Game *mGame;
        GameEndpoint *mPlayerEndpoint;
        SlotId mSlotId; // Player endpoint slotId
        int32_t mDirtySockUserIndex;

        void *mTitleContextData;
        TimeValue mJoinedGameTimestamp;

        PlayerSettings mPlayerSettings;

        ScenarioName mScenarioName;

        bool mIsLocal;
   };

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PLAYER_H
