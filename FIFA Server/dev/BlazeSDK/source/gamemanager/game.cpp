/*! ************************************************************************************************/
/*!
    \file game.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"

#include "BlazeSDK/gamemanager/game.h"
#include "BlazeSDK/gamemanager/gamemanagerapijobs.h"
#include "BlazeSDK/gamemanager/gameendpoint.h"
#include "BlazeSDK/gamemanager/player.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/gamemanager/gamemanagerhelpers.h" // for SlotType helpers


#include "EASTL/algorithm.h"

namespace Blaze
{
namespace GameManager
{

    Game::CreateGameParametersBase::CreateGameParametersBase(MemoryGroupId memGroupId)
        : mGameName(nullptr, memGroupId),
        mGameModRegister(0),
        mPresenceMode(PRESENCE_MODE_STANDARD),
        mGameType(GAME_TYPE_GAMESESSION),
        mNetworkTopology(CLIENT_SERVER_DEDICATED),
        mVoipTopology(VOIP_DISABLED),
        mGameAttributes(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mGameAttributes")),
        mMeshAttributes(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mMeshAttributes")),
        mMaxPlayerCapacity(8),
        mMinPlayerCapacity(1),
        mTeamIds(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mTeamCapacities")),
        mTeamCapacity(0),
        mQueueCapacity(0),
        mEntryCriteriaMap(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mEntryCriteriaMap")),
        mServerNotResetable(false),
        mRoleInformation(memGroupId)
    {
        mPlayerCapacity[SLOT_PUBLIC_PARTICIPANT] = 8;
        mPlayerCapacity[SLOT_PRIVATE_PARTICIPANT] = 0;
        mPlayerCapacity[SLOT_PUBLIC_SPECTATOR] = 0;
        mPlayerCapacity[SLOT_PRIVATE_SPECTATOR] = 0;

        mGameSettings.setOpenToBrowsing();
        mGameSettings.setOpenToInvites();
        mGameSettings.setOpenToMatchmaking();
        // Note: other flags are off by default
    }
    Game::CreateGameParameters::CreateGameParameters(MemoryGroupId memGroupId)
        : CreateGameParametersBase(memGroupId),
        mJoiningSlotType(SLOT_PUBLIC_PARTICIPANT),
        mJoiningTeamIndex(UNSPECIFIED_TEAM_INDEX),
        mJoiningEntryType(GAME_ENTRY_TYPE_DIRECT),
        mPlayerRoles(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mPlayerRoles")),
        mUserIdentificationRoles(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mUserIdentificationRoles"))
    {
    }

    Game::ResetGameParametersBase::ResetGameParametersBase(MemoryGroupId memGroupId)
        : mGameName(nullptr, memGroupId), 
        mGameModRegister(0),
        mPresenceMode(PRESENCE_MODE_STANDARD),
        mNetworkTopology(CLIENT_SERVER_DEDICATED),
        mVoipTopology(VOIP_DISABLED),
        mGameAttributes(memGroupId, MEM_NAME(memGroupId, "Game::ResetGameParameters.mGameAttributes")),
        mTeamIds(memGroupId, MEM_NAME(memGroupId, "Game::CreateGameParams.mTeamCapacities")),
        mTeamCapacity(0),
        mQueueCapacity(0),
        mEntryCriteriaMap(memGroupId, MEM_NAME(memGroupId, "Game::ResetGameParameters.mEntryCriteriaMap")),
        mRoleInformation(memGroupId)
    {
        mPlayerCapacity[SLOT_PUBLIC_PARTICIPANT] = 8;
        mPlayerCapacity[SLOT_PRIVATE_PARTICIPANT] = 0;
        mPlayerCapacity[SLOT_PUBLIC_SPECTATOR] = 0;
        mPlayerCapacity[SLOT_PRIVATE_SPECTATOR] = 0;

        mGameSettings.setOpenToBrowsing();
        mGameSettings.setOpenToInvites();
        mGameSettings.setOpenToMatchmaking();
        // Note: ranked & joinByPlayer are off by default
    }
    Game::ResetGameParameters::ResetGameParameters(MemoryGroupId memGroupId)
        : ResetGameParametersBase(memGroupId), 
        mJoiningSlotType(SLOT_PUBLIC_PARTICIPANT),
        mJoiningTeamIndex(UNSPECIFIED_TEAM_INDEX),
        mJoiningEntryType(GAME_ENTRY_TYPE_DIRECT),
        mPlayerRoles(memGroupId, MEM_NAME(memGroupId, "Game::ResetGameParameters.mPlayerRoles")),
        mUserIdentificationRoles(memGroupId, MEM_NAME(memGroupId, "Game::ResetGameParameters.mUserIdentificationRoles"))
    {
    }
    /*! ************************************************************************************************/
    /*! \brief construct the local representation of a game.
    
        \param[in] gameManagerAPI
        \param[in] replicatedGameData the game's data (attributes/settings/etc)
        \param[in] gameRoster the list of players in the game (typically including yourself)
        \param[in] gameQueue the list of players in the queue (possibly including yourself)
        \param[in] gameSetupReason - defines the 'reason' why we're creating a local game obj (join/reset/matchmaking/follow PG/etc)
        \param[in] qosSettings - the parameters to use when performing a pre-connect QoS validation
        \param[in] lockableForPreferredJoins - whether the game will become locked for preferred joins, when
            players are removed from it. The lock blocks finding or joining the game via game browser and matchmaking,
            until its cleared via a server configured time out, or when all active players have called
            joinGameByUserList or preferredJoinOptOut in response to the lock, or the game becomes full.
        \param[in] gameAttributeName - the attribute name that represents the game mode
        \param[in] performQosOnActivePlayers if true, active players already in the roster will be set to have a qos test done during connection
        \param[in] memGroupId mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    Game::Game(GameManagerAPI &gameManagerAPI, const ReplicatedGameData *data, const ReplicatedGamePlayerList& gameRoster,
                const ReplicatedGamePlayerList& gameQueue, const GameSetupReason &setupReason, const TimeValue& telemetryInterval, const QosSettings& qosSettings, 
                bool performQosOnActivePlayers, bool lockableForPreferredJoins, const char8_t* gameAttributeName, MemoryGroupId memGroupId)
        : GameBase(data, memGroupId),
          mUUID(data->getUUID(), memGroupId),
          mPlayerMemoryPool(memGroupId),
          mActivePlayers(memGroupId, MEM_NAME(memGroupId, "Game::mActivePlayers")),
          mRosterPlayers(memGroupId, MEM_NAME(memGroupId, "Game::mRosterPlayers")),
          mQueuedPlayers(memGroupId, MEM_NAME(memGroupId, "Game::mQueuedPlayers")),
          mPlayerRosterMap(memGroupId, MEM_NAME(memGroupId, "Game::mPlayerRosterMap")),
          mDeferredJoiningPlayerMap(memGroupId, MEM_NAME(memGroupId, "Game::mDeferredJoiningPlayerMap")),
          mGameEndpointMemoryPool(memGroupId),
          mActiveGameEndpoints(memGroupId, MEM_NAME(memGroupId, "Game::mActiveGameEndpoints")),
          mGameEndpointMap(memGroupId, MEM_NAME(memGroupId, "Game::mGameEndpointMap")),
          mNetworkMeshHelper(memGroupId, gameManagerAPI.getNetworkAdapter()),
          mDedicatedServerHostId(data->getDedicatedServerHostInfo().getPlayerId()),
          mDedicatedHostEndpoint(nullptr),
          mTopologyHostMeshEndpoint(nullptr),
          mPlatformHostPlayer(nullptr),
          mPlatformHostId(data->getPlatformHostInfo().getPlayerId()),
          mPlatformHostSlotId(data->getPlatformHostInfo().getSlotId()),
          mPlatformHostConnectionSlotId(data->getPlatformHostInfo().getConnectionSlotId()),
          mMeshAttributes(memGroupId, MEM_NAME(memGroupId, "Game::mMeshAttributes")),
          mLocalPlayers(memGroupId, gameManagerAPI.getBlazeHub()->getNumUsers(), MEM_NAME(memGroupId, "Game::mLocalPlayers")),
          mLocalPlayerMap(memGroupId, MEM_NAME(memGroupId, "Game::mLocalPlayerMap")),
          mLocalEndpoint(nullptr),
          mGameManagerApi(gameManagerAPI),
          mNetworkTopology(data->getNetworkTopology()),
          mQosDurationMs(qosSettings.getDurationMs()),
          mQosIntervalMs(qosSettings.getIntervalMs()),
          mQosPacketSize(qosSettings.getPacketSize()),
          mLeavingPlayerConnFlags(0),
          mScid(data->getScid()),

          mIsMigrating(false),
          mIsMigratingPlatformHost(false),
          mIsMigratingTopologyHost(false),
          mIsLocalPlayerPreInitNetwork(false),
          mIsLocalPlayerInitNetwork(false),
          mIsDirectlyReseting(false),
          mGameReportingId(data->getGameReportingId()),
          mGameReportName(data->getGameReportName()),
          mHostMigrationType(TOPOLOGY_HOST_MIGRATION),  // No 'invalid' value exists, so we just set this to the 0 value.
          mMemGroup(memGroupId),
          mTitleContextData(nullptr),
          mSharedSeed(data->getSharedSeed()),
          mServerNotResetable(data->getServerNotResetable()),
          mIsNetworkCreated(false),
          mIsLockableForPreferredJoins(lockableForPreferredJoins),
          mTelemetryInterval(telemetryInterval),
          mTelemetryReportingJobId(INVALID_JOB_ID),
          mGameModeAttributeName(gameAttributeName, memGroupId),
          mCreateTime(data->getCreateTime()),
          mCCSMode(data->getCCSMode()),
          mDelayingInitGameNetworkForCCS(false),
          mDelayingHostMigrationForCCS(false),
          mIsExternalOwner(false)
    {
        data->getMeshAttribs().copyInto(mMeshAttributes);

        data->getExternalSessionIdentification().copyInto(mExternalSessionIdentification);
        oldToNewExternalSessionIdentificationParams(data->getExternalSessionTemplateName(), data->getExternalSessionName(), data->getExternalSessionCorrelationId(), data->getNpSessionId(), mExternalSessionIdentification);

        // On PS4, user is only in its primary game's NP session (see DevNet ticket 58807). To avoid misuse (and match old behavior), clear local NP session ids on non-primary games
        setNpSessionId("");


        //  reserve size of memory pools (if size given)
        uint32_t maxPlayerCapacity = 0;
        if (mGameManagerApi.getApiParams().mMaxGameManagerGames > 0)
        {
            maxPlayerCapacity = static_cast<uint32_t>(GameBase::getMaxPlayerCapacity() + mQueueCapacity);
            mRosterPlayers.reserve(GameBase::getMaxPlayerCapacity());
            mQueuedPlayers.reserve(mQueueCapacity);
        }

        mPlayerMemoryPool.reserve(maxPlayerCapacity, "Game::PlayerPool");
        // worst case is each player has their own endpoint (SLU), plus a dedicated server host.
        mGameEndpointMemoryPool.reserve((isDedicatedServerTopology() ? maxPlayerCapacity + 1 : maxPlayerCapacity), "Game::EndpointPool");

        // dedicated endpoint first
        if (isDedicatedServerTopology())
        {
            if (getGameState() != INACTIVE_VIRTUAL)
            {
                BlazeAssertMsg(!data->getDedicatedServerHostNetworkAddressList().empty(), "Non-virtual dedicated server game had empty network address list!");

                // build the endpoint, note that we assume the dedicated server is 'connected'
                BLAZE_SDK_DEBUGF("[GAME] Allocating endpoint for dedicated server host in game (%" PRIu64 "), for connection group (%" PRIu64 ").\n", mGameId, data->getTopologyHostInfo().getConnectionGroupId());
                mDedicatedHostEndpoint = new (mGameEndpointMemoryPool.alloc()) GameEndpoint(this, data->getDedicatedServerHostInfo().getConnectionGroupId(), 
                    data->getDedicatedServerHostInfo().getConnectionSlotId(), data->getDedicatedServerHostNetworkAddressList(), data->getUUID(), CONNECTED, mMemGroup);
                // put it in the endpoint map
                mGameEndpointMap[mDedicatedHostEndpoint->getConnectionGroupId()] = mDedicatedHostEndpoint;
                // put it in the active endpoints
                mActiveGameEndpoints[mDedicatedHostEndpoint->getConnectionSlotId()] = mDedicatedHostEndpoint;

                // Enable QoS for the dedicated server endpoint (if needed)
                mDedicatedHostEndpoint->setPerformQosDuringConnection(performQosOnActivePlayers);


                // are we the host? update the local endpoint, players manage this during add player
                UserManager::LocalUser* localUser = mGameManagerApi.getUserManager()->getPrimaryLocalUser();
                if (localUser != nullptr)
                {
                    if (localUser->getId() == getDedicatedServerHostId())
                    {
                        mLocalEndpoint = mDedicatedHostEndpoint;
                    }
                }
            }

            if (gameRoster.empty())
            {
                // dedicated topology, topology and dedicated endpoint are the same
                mTopologyHostMeshEndpoint = mDedicatedHostEndpoint;
            }
        }

        initRoster(gameRoster, performQosOnActivePlayers);
        initQueue(gameQueue);


        if (mTopologyHostMeshEndpoint == nullptr)
        {
            // get the topology host mesh endpoint. We will have built it when initializing the roster, or when we set up the dedicated server
            mTopologyHostMeshEndpoint = (GameEndpoint*)getMeshEndpointByConnectionGroupId(data->getTopologyHostInfo().getConnectionGroupId());
        }


        // if it's a host or it's ds resetter, set the persisted id secret
        if (isTopologyHost() || setupReason.getActiveMember() == GameSetupReason::MEMBER_RESETDEDICATEDSERVERSETUPCONTEXT)
        {
            mPersistedGameIdSecret.setData(data->getPersistedGameIdSecret().getData(), data->getPersistedGameIdSecret().getCount());
        }

        // set the default behavior upon joining a new game
        mRemoveLocalPlayerAfterResolve = mGameManagerApi.getApiParams().mPreventMultipleGameMembership;

        data->getCurrentlyAcceptedPlatformList().copyInto(mClientPlatformList);
        if (data->getExternalOwnerInfo().getPlayerId() != Blaze::INVALID_BLAZE_ID)
        {
            UserManager::LocalUser* localUser = mGameManagerApi.getUserManager()->getPrimaryLocalUser();
            mIsExternalOwner = (localUser && localUser->getId() == data->getExternalOwnerInfo().getPlayerId());
            data->getExternalOwnerInfo().copyInto(mExternalOwnerInfo);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    Game::~Game()
    {
        stopTelemetryReporting();

        // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
        // to call the callback.  Since the object is being deleted, we go with the remove.
        mGameManagerApi.getBlazeHub()->getScheduler()->removeByAssociatedObject(this);

        // delete player objects
        PlayerMap::iterator playerIter = mPlayerRosterMap.begin();
        PlayerMap::iterator playerEnd = mPlayerRosterMap.end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            Player* player = playerIter->second;
            mPlayerMemoryPool.free(player);
        }


        // delete endpoint objects (if any still exist)
        EndpointMap::iterator endpointIter = mGameEndpointMap.begin();
        EndpointMap::iterator endpointEnd = mGameEndpointMap.end();
        for (; endpointIter != endpointEnd; ++endpointIter)
        {
            // all endpoints are in this map, so no need to clean up the dedicated server / topology host / local endpoints independently
            GameEndpoint* endpoint = endpointIter->second;
            mGameEndpointMemoryPool.free(endpoint);
        }

        mDeferredJoiningPlayerMap.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief return a active or queued player by id.
    ***************************************************************************************************/
    Player *Game::getPlayerById(BlazeId playerId) const
    {
        Player *player = getRosterPlayerById(playerId);
        if ( player == nullptr )
        {
            player = getQueuedPlayerById(playerId);
        }

        return player;
    }

    /*! ************************************************************************************************/
    /*! \brief return the MeshEndpoint (active player) with a given blazeId.
    ***************************************************************************************************/
    const MeshMember* Game::getMeshMemberById(BlazeId blazeId) const
{
        return getActivePlayerById(blazeId);
    }

    /*! ************************************************************************************************/
    /*! \brief return the MeshEndpoint (active player) with a given name (case-insensitive)
    ***************************************************************************************************/
    const MeshMember* Game::getMeshMemberByName(const char8_t *name) const
{
        return getActivePlayerByName(name);
    }

    /*! ************************************************************************************************/
    /*! \brief get the MeshEndpoint by the connection group Id.
    ***************************************************************************************************/
    const MeshEndpoint* Game::getMeshEndpointByConnectionGroupId(ConnectionGroupId connectionGroupId) const
{
        EndpointMap::const_iterator endpointIter = mGameEndpointMap.find(connectionGroupId);
        if (endpointIter != mGameEndpointMap.end())
        {
            return endpointIter->second;
        }
        
        return nullptr; 
    }

    /*! ************************************************************************************************/
    /*! \brief return the MeshEndpoint (active players) at the supplied index.
    ***************************************************************************************************/
    const MeshEndpoint* Game::getMeshEndpointByIndex(uint16_t index) const
    {
        if (index < mActiveGameEndpoints.size())
        {
            return mActiveGameEndpoints.at(index).second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief return the MeshMember (active player) at the supplied index.
    ***************************************************************************************************/
    const MeshMember* Game::getMeshMemberByIndex(uint16_t index) const
    {
        return getActivePlayerByIndex(index);
    }


    /*! ************************************************************************************************/
    /*! \brief return the host mesh endpoint.
    ***************************************************************************************************/
    const MeshEndpoint * Game::getTopologyHostMeshEndpoint() const
    { 
        return mTopologyHostMeshEndpoint;
    }

    /*! ************************************************************************************************/
    /*! \brief return the MeshEndpoint of the dedicated topology host.  Will be nullptr if this is not a
        dedicated server.
        
    \return the host as a MeshEndpoint.
    ***************************************************************************************************/
    const MeshEndpoint * Game::getDedicatedServerHostMeshEndpoint() const
    { 
        return mDedicatedHostEndpoint; 
    }

    const MeshMember * Game::getPlatformHostMeshMember() const
{
        return getPlatformHostPlayer();
    }

    /*! ************************************************************************************************/
    /*! \brief return the local mesh endpoint.
    ***************************************************************************************************/
    const MeshEndpoint * Game::getLocalMeshEndpoint() const
    {
        return mLocalEndpoint; 
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns the endpoint slot id for the game's topology host player.
        \return the game's host slot id.
    **************************************************************************************************************/
    SlotId Game::getTopologyHostConnectionSlotId() const { return (mTopologyHostMeshEndpoint != nullptr) ? mTopologyHostMeshEndpoint->getConnectionSlotId() : 0; }

    /*! ************************************************************************************************/
    /*! \brief  Returns the topology hosts connection group id.
        \return The topology hosts connection group id
    ***************************************************************************************************/
    uint64_t Game::getTopologyHostConnectionGroupId() const { return (mTopologyHostMeshEndpoint != nullptr) ? mTopologyHostMeshEndpoint->getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID;  }

    Player* Game::getLocalPlayer() const
    {
        Player* player = getLocalPlayer(mGameManagerApi.getBlazeHub()->getPrimaryLocalUserIndex());
        // If primary local user is not a player yet but some other local user is a player, make him primary local user and return that
        // player.
        if (player == nullptr)
        {
            for (uint32_t i = 0; i < mGameManagerApi.getBlazeHub()->getNumUsers(); ++i)
            {
                if ((player = mLocalPlayers[i]) != nullptr)
                {
                    mGameManagerApi.getUserManager()->setPrimaryLocalUser(i);
                    break;
                }
            }
        }

        return player;
    }

    bool Game::isNotificationHandlingUserIndex(uint32_t userIndex) const
    {
        if (isDedicatedServerHost())
            return true;

        // We don't want to have multiple notifications coming through if we can avoid it, so we assign a single player to deal with that.
        // By default, that's the primary local user index, but if that user's not in-game, we just take the first user index that's in game. 
        // (Note: This does mean that in the worst case we might switch a few times as players are added/removed.  Ex. playerJoined(index 2) -> playerJoined(index 1) -> playerJoined(index 0) )
        uint32_t primaryUserIndex = mGameManagerApi.getBlazeHub()->getPrimaryLocalUserIndex();
        if (getLocalPlayer(primaryUserIndex))
        {
            return (userIndex == primaryUserIndex);
        }
        else
        {
            // Find the first userindex in-game:
            for (uint32_t i = 0; i < mGameManagerApi.getBlazeHub()->getNumUsers(); ++i)
            {
                if (mLocalPlayers[i] != nullptr)
                {
                    return (userIndex == i);
                }
            }
        }
        return false;
    }

    Player* Game::getLocalPlayer(uint32_t userIndex) const
    {
        if (userIndex < mGameManagerApi.getBlazeHub()->getNumUsers())
        {
            return mLocalPlayers[userIndex];
        }
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief return an active (non-queued) player by id
    ***************************************************************************************************/
    Player *Game::getActivePlayerById(BlazeId playerId) const
    {
        for (PlayerRosterList::const_iterator playerIter = mActivePlayers.begin(), playerEnd = mActivePlayers.end(); playerIter != playerEnd; ++playerIter)
        {
            Player *player = playerIter->second;
            if (player->getId() == playerId)
            {
                return player;
            }
        }

        return nullptr; 
    }


    /*! ************************************************************************************************/
    /*! \brief return a queued player by id
    ***************************************************************************************************/
    Player* Game::getQueuedPlayerById(BlazeId playerId) const
    {
        for (PlayerRosterList::const_iterator playerIter = mQueuedPlayers.begin(), playerEnd = mQueuedPlayers.end(); playerIter!=playerEnd; ++playerIter)
        {
            Player *player = playerIter->second;
            if (player->getId() == playerId)
            {
                return player;
            }
        }

        return nullptr; 
    }

    /*! ************************************************************************************************/
    /*! \brief return a player given a user pointer
    ***************************************************************************************************/
    Player* Game::getPlayerByUser(const UserManager::User *user) const
    {
        Player *player = getRosterPlayerByUser(user);
        if ( player == nullptr )
        {
            player = getQueuedPlayerByUser(user);
        }

        return player;
    }

    /*! ************************************************************************************************/
    /*! \brief return an active player (non-queued) given a user
    ***************************************************************************************************/
    Player* Game::getActivePlayerByUser(const UserManager::User *user) const
    {
        if ( user != nullptr )
        {
            return getActivePlayerById( user->getId() );
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief return a queued player given a user
    ***************************************************************************************************/
    Player* Game::getQueuedPlayerByUser(const UserManager::User *user) const
    {
        if ( user != nullptr )
        {
            return getQueuedPlayerById( user->getId() );
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief return a player given a persona name
    ***************************************************************************************************/
    Player* Game::getPlayerByName(const char8_t *personaName) const
    {
        Player *player = getRosterPlayerByName(personaName);
        if ( player == nullptr )
        {
            player = getQueuedPlayerByName(personaName);
        }

        return player;
    }

    /*! ************************************************************************************************/
    /*! \brief get active (non-queued) player by name
    ***************************************************************************************************/
    Player* Game::getActivePlayerByName(const char8_t *personaName) const
    {
        return getActivePlayerByUser( mGameManagerApi.getUserManager()->getUserByName( personaName ) );
    }

    /*! ************************************************************************************************/
    /*! \brief get queued player by name
    ***************************************************************************************************/
    Player* Game::getQueuedPlayerByName(const char8_t *personaName) const
    {
        return getQueuedPlayerByUser( mGameManagerApi.getUserManager()->getUserByName( personaName ) );
    }

    /*! ************************************************************************************************/
    /*! \brief get the first active player in the connection group.
    ***************************************************************************************************/
    Player* Game::getActivePlayerByConnectionGroupId(ConnectionGroupId connectionGroupId) const
    {
        // find the player in the player vector
        PlayerRosterList::const_iterator playerIter = mActivePlayers.begin();
        PlayerRosterList::const_iterator playerEnd = mActivePlayers.end();
        for ( ; playerIter!=playerEnd; ++playerIter )
        {
            Player *player = playerIter->second;
            if (player->getConnectionGroupId() == connectionGroupId)
            {
                return player;
            }
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief populate a list of users with the matching connectionGroupId.
    ***************************************************************************************************/
    void Game::getActivePlayerByConnectionGroupId(ConnectionGroupId connectionGroupId, PlayerVector& playerVector) const
    {
        // find the player in the player vector
        PlayerRosterList::const_iterator playerIter = mActivePlayers.begin();
        PlayerRosterList::const_iterator playerEnd = mActivePlayers.end();
        for ( ; playerIter!=playerEnd; ++playerIter )
        {
            Player *player = playerIter->second;
            if (player->getConnectionGroupId() == connectionGroupId)
            {
                playerVector.push_back(player);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief get player by index
    ***************************************************************************************************/
    Player* Game::getPlayerByIndex(uint16_t index) const
    {
        if ( index < mRosterPlayers.size() )
        {
            return mRosterPlayers.at(index).second;
        }

        PlayerRosterList::size_type queueIndex = index - mRosterPlayers.size();
        if ( queueIndex < mQueuedPlayers.size() )
        {
            return mQueuedPlayers.at((PlayerRosterList::size_type)queueIndex).second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief return an active (non-queued) player given an index
    ***************************************************************************************************/
    Player* Game::getActivePlayerByIndex(uint16_t index) const
    {
        if ( index < mActivePlayers.size() )
        {
            return mActivePlayers.at(index).second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief return a queued player given an index
    ***************************************************************************************************/
    Player* Game::getQueuedPlayerByIndex(uint16_t index) const
    {
        if ( index < mQueuedPlayers.size() )
        {
            return mQueuedPlayers.at(index).second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Get a player's index (position) in the player queue
    ***************************************************************************************************/
    uint16_t Game::getQueuedPlayerIndex(const Player* player) const
    {
        if ( getQueuedPlayerById(player->getId()) != nullptr )
        {
            return player->getSlotId();
        }

        return INVALID_QUEUE_INDEX; // player not found
    }

    // fill the supplied PlayerVector with the active (not reserved, not queued) players in the game 
    void Game::getActiveParticipants( PlayerVector& playerVector) const
    {
        playerVector.reserve(getPlayerCount());
        // find the player in the player vector
        PlayerRosterList::const_iterator playerIter = mActivePlayers.begin();
        PlayerRosterList::const_iterator playerEnd = mActivePlayers.end();
        for (; playerIter != playerEnd; ++playerIter )
        {
            Player *player = playerIter->second;
            if (player->isParticipant())
            {
                playerVector.push_back(player);
            }
        }
    }

    // fill the supplied PlayerVector with the active (not reserved) spectators in the game 
    void Game::getActiveSpectators( PlayerVector& spectatorVector) const
    {
        spectatorVector.reserve(getSpectatorCount());
        // find the player in the player vector
        PlayerRosterList::const_iterator playerIter = mActivePlayers.begin();
        PlayerRosterList::const_iterator playerEnd = mActivePlayers.end();
        for (; playerIter != playerEnd; ++playerIter )
        {
            Player *spectator = playerIter->second;
            if (spectator->isSpectator())
            {
                spectatorVector.push_back(spectator);
            }
        }
    }

    // fill the supplied PlayerVector with the active (not reserved, not queued) players in the game 
    void Game::getQueuedParticipants( PlayerVector& playerVector) const
    {
        playerVector.reserve(getQueueCount());
        // find the player in the player vector
        PlayerRosterList::const_iterator queueIter = mQueuedPlayers.begin();
        PlayerRosterList::const_iterator queueEnd = mQueuedPlayers.end();
        for (; queueIter != queueEnd; ++queueIter )
        {
            Player *player = queueIter->second;
            if (player->isParticipant())
            {
                playerVector.push_back(player);
            }
        }
    }

    // fill the supplied PlayerVector with the active (not reserved) spectators in the game 
    void Game::getQueuedSpectators( PlayerVector& spectatorVector) const
    {
        spectatorVector.reserve(getQueueCount());
        // find the player in the player vector
        PlayerRosterList::const_iterator queueIter = mQueuedPlayers.begin();
        PlayerRosterList::const_iterator queueEnd = mQueuedPlayers.end();
        for (; queueIter != queueEnd; ++queueIter )
        {
            Player *spectator = queueIter->second;
            if (spectator->isSpectator())
            {
                spectatorVector.push_back(spectator);
            }
        }
    }


    void Game::helperCallback(BlazeError err, Game* game, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor2<BlazeError, Game *>* titleCb = (const Functor2<BlazeError, Game *>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, BlazeId blazeId, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<BlazeError, Game *, BlazeId>* titleCb = (const Functor3<BlazeError, Game *, BlazeId>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game, blazeId);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, Player* player, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<BlazeError, Game *, Player*>* titleCb = (const Functor3<BlazeError, Game *, Player*>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game, player);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, const Player* player, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<BlazeError, Game *, const Player*>* titleCb = (const Functor3<BlazeError, Game *, const Player*>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game, player);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, PlayerIdList * playeridList, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<BlazeError, Game *, PlayerIdList *>* titleCb = (const Functor3<BlazeError, Game *, PlayerIdList *>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game, playeridList);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, const SwapPlayersErrorInfo * swapinfo, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<BlazeError, Game *, const SwapPlayersErrorInfo *>* titleCb = (const Functor3<BlazeError, Game *, const SwapPlayersErrorInfo *>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game, swapinfo);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, const ExternalSessionErrorInfo * extInfo, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<BlazeError, Game *, const ExternalSessionErrorInfo *>* titleCb = (const Functor3<BlazeError, Game *, const ExternalSessionErrorInfo *>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, game, extInfo);
        }
    }
    void Game::helperCallback(BlazeError err, Game* game, const BannedListResponse* bannedResp, JobId jobId)
    {
        Job* job = mGameManagerApi.getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor3<const BannedListResponse*, BlazeError, Game *>* titleCb = (const Functor3<const BannedListResponse*, BlazeError, Game *>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(bannedResp, err, game);
        }
    }


    template <typename T> T MakeFunctorHelper(Game* game)
    {
        return MakeFunctor(game, static_cast<void (Game::*)(BlazeError, Game*, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, BlazeId, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, BlazeId, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, BlazeId, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, Player*, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, Player*, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, Player*, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, const Player*, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, const Player*, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, const Player*, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, PlayerIdList *, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, PlayerIdList *, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, PlayerIdList *, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, const SwapPlayersErrorInfo *, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, const SwapPlayersErrorInfo *, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, const SwapPlayersErrorInfo *, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, const ExternalSessionErrorInfo *, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, const ExternalSessionErrorInfo *, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, const ExternalSessionErrorInfo *, JobId)>(&Game::helperCallback));
    }
    template <> Functor4<BlazeError, Game*, const BannedListResponse*, JobId> MakeFunctorHelper<Functor4<BlazeError, Game*, const BannedListResponse*, JobId>>(Game* game)
    {
        return MakeFunctor(game, static_cast<void(Game::*)(BlazeError, Game*, const BannedListResponse*, JobId)>(&Game::helperCallback));
    }

#define BACK_COMPAT_WRAPPER(funcName, funcParamDefs, newCbType, funcParams) \
    JobId Game::funcName funcParamDefs\
    {\
        newCbType helperCb = MakeFunctorHelper<newCbType>(this);\
        JobId jobId = funcName funcParams;\
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);\
        return jobId;\
    }

    BACK_COMPAT_WRAPPER(initGameComplete, (const ChangeGameStateCb &titleCb), ChangeGameStateJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(startGame, (const ChangeGameStateCb &titleCb), ChangeGameStateJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(endGame, (const ChangeGameStateCb &titleCb), ChangeGameStateJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(replayGame, (const ChangeGameStateCb &titleCb), ChangeGameStateJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(returnDedicatedServerToPool, (const ChangeGameStateCb &titleCb), ChangeGameStateJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(resetDedicatedServer, (const ChangeGameStateCb &titleCb), ChangeGameStateJobCb, (helperCb));

    BACK_COMPAT_WRAPPER(addQueuedPlayer, (const Player *player, const AddQueuedPlayerCb& titleCb), AddQueuedPlayerJobCb, (player, helperCb));
    BACK_COMPAT_WRAPPER(addQueuedPlayer, (const Player *player, TeamIndex joiningTeamIndex, const RoleName *joiningRoleName, const AddQueuedPlayerCb& titleCb), AddQueuedPlayerJobCb, (player, joiningTeamIndex, joiningRoleName, helperCb));
    BACK_COMPAT_WRAPPER(demotePlayerToQueue, (const Player *player, const AddQueuedPlayerCb& titleCb), AddQueuedPlayerJobCb, (player, helperCb));

    BACK_COMPAT_WRAPPER(leaveGame, (const LeaveGameCb &titleCb, const UserGroup* userGroup, bool makeReservation), LeaveGameJobCb, (helperCb, userGroup, makeReservation));
    BACK_COMPAT_WRAPPER(leaveGameLocalUser, (uint32_t userIndex, const LeaveGameCb &titleCb, bool makeReservation), LeaveGameJobCb, (userIndex, helperCb, makeReservation));
    BACK_COMPAT_WRAPPER(kickPlayer, (const Player *player, const KickPlayerCb &titleCb, bool banPlayer, PlayerRemovedTitleContext titleContext, const char8_t* titleContextStr, KickReason kickReason), KickPlayerJobCb, (player, helperCb, banPlayer, titleContext, titleContextStr, kickReason));
    //JobId Game::kickPlayer(const Player *player, const KickPlayerCb &titleCb, bool banPlayer, PlayerRemovedTitleContext titleContext, const char8_t* titleContextStr, KickReason kickReason)
    //{
    //    KickPlayerJobCb helperCb = MakeFunctorHelper<KickPlayerJobCb>(this);
    //    JobId jobId = kickPlayer(player, helperCb, banPlayer, titleContext, titleContextStr, kickReason);
    //    Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb); 
    //    return jobId; 
    //}

    BACK_COMPAT_WRAPPER(banUser, (const BlazeId blazeId, const BanPlayerCb &titleCb, PlayerRemovedTitleContext titleContext), BanPlayerJobCb, (blazeId, helperCb, titleContext));
    BACK_COMPAT_WRAPPER(banUser, (PlayerIdList *playerIds, const BanPlayerListCb &titleCb, PlayerRemovedTitleContext titleContext), BanPlayerListJobCb, (playerIds, helperCb, titleContext));
    BACK_COMPAT_WRAPPER(removePlayerFromBannedList, (const BlazeId blazeId, const UnbanPlayerCb &titleCb), UnbanPlayerJobCb, (blazeId, helperCb));

    BACK_COMPAT_WRAPPER(clearBannedList, (const ClearBannedListCb &titleCb), ClearBannedListJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(getBannedList, (const GetBannedListCb &titleCb), GetBannedListJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(updateGameName, (const GameName &newName, const UpdateGameNameCb &titleCb), UpdateGameNameJobCb, (newName, helperCb));
    BACK_COMPAT_WRAPPER(destroyGame, (GameDestructionReason gameDestructionReason, const DestroyGameCb &titleCb), DestroyGameJobCb, (gameDestructionReason, helperCb));
    BACK_COMPAT_WRAPPER(ejectHost, (const EjectHostCb &titleCb, bool replaceHost), EjectHostJobCb, (helperCb, replaceHost));
    BACK_COMPAT_WRAPPER(setPlayerCapacity, (const SlotCapacities &newSlotCapacities, const ChangePlayerCapacityCb &titleCb), ChangePlayerCapacityJobCb, (newSlotCapacities, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerCapacity, (const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const ChangePlayerCapacityCb &titleCb), ChangePlayerCapacityJobCb, (newSlotCapacities, newTeamsComposition, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerCapacity, (const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const RoleInformation &roleInformation, const ChangePlayerCapacityCb &titleCb), ChangePlayerCapacityJobCb, (newSlotCapacities, newTeamsComposition, roleInformation, helperCb));
    BACK_COMPAT_WRAPPER(migrateHostToPlayer, (const Player *player, const MigrateHostCb &titleCb), MigrateHostJobCb, (player, helperCb));
    BACK_COMPAT_WRAPPER(setGameModRegister, (GameModRegister newModRegister, const ChangeGameModRegisterCb &titleCb), ChangeGameModRegisterJobCb, (newModRegister, helperCb));
    BACK_COMPAT_WRAPPER(setGameEntryCriteria, (const EntryCriteriaMap& entryCriteriaMap, const RoleEntryCriteriaMap& roleSpecificEntryCriteria, const ChangeGameEntryCriteriaCb &titleCb), ChangeGameEntryCriteriaJobCb, (entryCriteriaMap, roleSpecificEntryCriteria, helperCb));

    BACK_COMPAT_WRAPPER(setGameSettings, (const GameSettings &newSettings, const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (newSettings, helperCb));
    BACK_COMPAT_WRAPPER(setGameSettings, (const GameSettings &newSettings, const GameSettings &settingsMask, const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (newSettings, settingsMask, helperCb));
    BACK_COMPAT_WRAPPER(closeGameToAllJoins, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(openGameToAllJoins, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(setOpenToBrowsing, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(clearOpenToBrowsing, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(setOpenToMatchmaking, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(clearOpenToMatchmaking, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(setOpenToInvites, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(clearOpenToInvites, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(setOpenToJoinByPlayer, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(clearOpenToJoinByPlayer, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(setRanked, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(clearRanked, (const ChangeGameSettingsCb &titleCb), ChangeGameSettingsJobCb, (helperCb));

    BACK_COMPAT_WRAPPER(changeTeamIdAtIndex, (const TeamIndex &teamIndex, const TeamId &newTeamId, const ChangeTeamIdCb &titleCb), ChangeTeamIdJobCb, (teamIndex, newTeamId, helperCb));
    BACK_COMPAT_WRAPPER(swapPlayerTeamsAndRoles, (const SwapPlayerDataList &swapPlayerDataList, const SwapPlayerTeamsAndRolesCb &titleCb), SwapPlayerTeamsAndRolesJobCb, (swapPlayerDataList, helperCb));
    BACK_COMPAT_WRAPPER(setGameAttributeValue, (const char8_t* attributeName, const char8_t* attributeValue, const ChangeGameAttributeCb &titleCb), ChangeGameAttributeJobCb, (attributeName, attributeValue, helperCb));
    BACK_COMPAT_WRAPPER(setGameAttributeMap, (const Collections::AttributeMap *changingAttributes, const ChangeGameAttributeCb &titleCb), ChangeGameAttributeJobCb, (changingAttributes, helperCb));

    BACK_COMPAT_WRAPPER(addPlayerToAdminList, (Player *player, const UpdateAdminListCb &titleCb), UpdateAdminListJobCb, (player, helperCb));
    BACK_COMPAT_WRAPPER(addPlayerToAdminList, (BlazeId blazeId, const UpdateAdminListBlazeIdCb &titleCb), UpdateAdminListBlazeIdJobCb, (blazeId, helperCb));
    BACK_COMPAT_WRAPPER(removePlayerFromAdminList, (Player *player, const UpdateAdminListCb &titleCb), UpdateAdminListJobCb, (player, helperCb));
    BACK_COMPAT_WRAPPER(removePlayerFromAdminList, (BlazeId blazeId, const UpdateAdminListBlazeIdCb &titleCb), UpdateAdminListBlazeIdJobCb, (blazeId, helperCb));
    BACK_COMPAT_WRAPPER(migratePlayerAdminList, (Player *player, const UpdateAdminListCb &titleCb), UpdateAdminListJobCb, (player, helperCb));
    BACK_COMPAT_WRAPPER(migratePlayerAdminList, (BlazeId blazeId, const UpdateAdminListBlazeIdCb &titleCb), UpdateAdminListBlazeIdJobCb, (blazeId, helperCb));

    BACK_COMPAT_WRAPPER(preferredJoinOptOut, (const PreferredJoinOptOutCb &titleCb), PreferredJoinOptOutJobCb, (helperCb));
    BACK_COMPAT_WRAPPER(updateExternalSessionImage, (const Ps4NpSessionImage& image, const UpdateExternalSessionImageCb &titleCb), UpdateExternalSessionImageJobCb, (image, helperCb));
    BACK_COMPAT_WRAPPER(updateExternalSessionStatus, (const ExternalSessionStatus& status, const UpdateExternalSessionStatusCb &titleCb), UpdateExternalSessionStatusJobCb, (status, helperCb));


#undef BACK_COMPAT_WRAPPER

    // Conversion Helpers: 
    template <typename callbackType>
    JobId JobCbHelper(const char8_t* jobName, const callbackType& titleCb, BlazeError err, Game* thisGame)
    {
        JobScheduler* scheduler = thisGame->getGameManagerAPI()->getBlazeHub()->getScheduler();
        JobId jobId = scheduler->reserveJobId();
        scheduler->scheduleFunctor(jobName, titleCb, err, thisGame, jobId, thisGame, 0, jobId);
        Job::addTitleCbAssociatedObject(scheduler, jobId, titleCb);
        return jobId;
    }
    template <typename callbackType, typename P3>
    JobId JobCbHelper(const char8_t* jobName, const callbackType& titleCb, BlazeError err, Game* thisGame, P3 p3)
    {
        JobScheduler* scheduler = thisGame->getGameManagerAPI()->getBlazeHub()->getScheduler();
        JobId jobId = scheduler->reserveJobId();
        scheduler->scheduleFunctor(jobName, titleCb, err, thisGame, p3, jobId, thisGame, 0, jobId);
        Job::addTitleCbAssociatedObject(scheduler, jobId, titleCb);
        return jobId;
    }


    JobId Game::addQueuedPlayer(const Player *player, const AddQueuedPlayerJobCb& titleCb)
    {
        return addQueuedPlayer(player, UNSPECIFIED_TEAM_INDEX, nullptr, titleCb);
    }

    JobId Game::addQueuedPlayer(const Player *player, TeamIndex joiningTeamIndex, const RoleName *joiningRoleName, const AddQueuedPlayerJobCb& titleCb)
    {
        if ( !containsPlayer(player) )
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Calling Game::addQueuedPlayer with player who is not part of this game(%" PRIu64 ").\n", mGameId);
            return JobCbHelper("addQueuedPlayerCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, player);
        }

        // Setup the request
        AddQueuedPlayerToGameRequest request;
        request.setGameId(mGameId);
        request.setPlayerId(player->getId());
        request.setPlayerTeamIndex(joiningTeamIndex);
        if (joiningRoleName)
        {
            request.setOverridePlayerRole(true); // defaults to false
            request.setPlayerRole(joiningRoleName->c_str());
        }

        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();
        JobId jobId = gameManagerComponent->addQueuedPlayerToGame(request, MakeFunctor(this, &Game::internalAddQueuedPlayerToGameCb), player->getId(), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    void Game::internalAddQueuedPlayerToGameCb(BlazeError error, JobId id, PlayerId playerId, AddQueuedPlayerJobCb titleCb)
    {
        const Player* player = getPlayerInternal(playerId);
        titleCb(error, this, player, id);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    JobId Game::demotePlayerToQueue(const Player *player, const AddQueuedPlayerJobCb& titleCb)
    {
        if ( !containsPlayer(player) )
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Calling Game::demotePlayerToQueue with player who is not part of this game(%" PRIu64 ").\n", mGameId);
            return JobCbHelper("demotePlayerToQueueCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, player);
        }

        // Setup the request
        DemoteReservedPlayerToQueueRequest request;
        request.setGameId(mGameId);
        request.setPlayerId(player->getId());

        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();
        JobId jobId = gameManagerComponent->demoteReservedPlayerToQueue(request, MakeFunctor(this, &Game::internalAddQueuedPlayerToGameCb), player->getId(), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief return true if a local player is the topology host.
    ***************************************************************************************************/
    bool Game::isTopologyHost() const 
    { 
        return (mTopologyHostMeshEndpoint == mLocalEndpoint);
    }


    /*! ************************************************************************************************/
    /*! \brief return true if a local player is the game's platform host. 
    ***************************************************************************************************/
    bool Game::isPlatformHost() const
    { 
        if (getPlatformHostPlayer() != nullptr)
        {
            return (getPlatformHostPlayer()->isLocalPlayer());
        }
        // Platform host will only be null if we don't know who the platform host is.
        // so just return that we are not.
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief set the game's settings.
    ***************************************************************************************************/
    JobId Game::setGameSettings(const GameSettings &newSettings, const ChangeGameSettingsJobCb &titleCb)
    {
        // Default empty value masks everything:
        GameSettings settingsMask;
        return setGameSettings(newSettings, settingsMask, titleCb);
    }

    JobId Game::setGameSettings(const GameSettings &newSettings, const GameSettings &settingsMask, const ChangeGameSettingsJobCb &titleCb)
    {
        if (newSettings.getBits() == mGameSettings.getBits())
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: calling Game::setGameSettings without changing any settings from their current values.\n");
        }

        mGameManagerApi.validateGameSettings(newSettings);

        SetGameSettingsRequest setGameSettingsRequest;
        setGameSettingsRequest.setGameId(mGameId);
        setGameSettingsRequest.getGameSettings().setBits(newSettings.getBits());
        setGameSettingsRequest.getGameSettingsMask().setBits(settingsMask.getBits());

        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();
        JobId jobId = gameManagerComponent->setGameSettings(setGameSettingsRequest, MakeFunctor(this, &Game::internalSetGameSettingsCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }


    /*! ************************************************************************************************/
    /*! \brief set the game's settings.
    ***************************************************************************************************/
    JobId Game::setGameModRegister(GameModRegister newModRegister, const ChangeGameModRegisterJobCb &titleCb)
    {
        if (newModRegister == mGameModRegister)
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: calling Game::setGameModRegister without changing any mods from their current values.\n" );
        }

        SetGameModRegisterRequest setGameModRegisterRequest;
        setGameModRegisterRequest.setGameId(mGameId);
        setGameModRegisterRequest.setGameModRegister(newModRegister);
        
        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();
        JobId jobId = gameManagerComponent->setGameModRegister(setGameModRegisterRequest, MakeFunctor(this, &Game::internalSetGameSettingsCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief set the game's entry criteria settings.
    ***************************************************************************************************/
    JobId Game::setGameEntryCriteria(const EntryCriteriaMap& entryCriteriaMap, const RoleEntryCriteriaMap& roleSpecificEntryCriteria, const ChangeGameEntryCriteriaJobCb &titleCb)
    {
        SetGameEntryCriteriaRequest setGameEntryCriteriaRequest;
        setGameEntryCriteriaRequest.setGameId(mGameId);
        entryCriteriaMap.copyInto(setGameEntryCriteriaRequest.getEntryCriteriaMap());
        roleSpecificEntryCriteria.copyInto(setGameEntryCriteriaRequest.getRoleEntryCriteriaMap());


        // TODO: Workaround for GOS-24271, need to clear out any entries in the map that have empty entry criteria to work around TDF decoder bug
        RoleEntryCriteriaMap &roleEntryCriteriaMap = setGameEntryCriteriaRequest.getRoleEntryCriteriaMap();
        RoleEntryCriteriaMap::iterator roleEntryCriteriaIter = roleEntryCriteriaMap.begin();
        RoleEntryCriteriaMap::iterator roleEntryCriteriaEnd = roleEntryCriteriaMap.end();
        while(roleEntryCriteriaIter != roleEntryCriteriaEnd)
        {

            // put the entry criteria in the tdf
            if (roleEntryCriteriaIter->second->empty())
            {
                RoleEntryCriteriaMap::iterator tempRoleEntryCriteriaIter = roleEntryCriteriaIter;
                // advance iterator before invalidating
                ++roleEntryCriteriaIter;
                roleEntryCriteriaMap.erase(tempRoleEntryCriteriaIter);
            }
            else
            {
                ++roleEntryCriteriaIter;
            }
            
        }
        
        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();
        JobId jobId = gameManagerComponent->setGameEntryCriteria(setGameEntryCriteriaRequest, MakeFunctor(this, &Game::internalSetGameSettingsCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief cb for setGameSettings
    ***************************************************************************************************/
    void Game::internalSetGameSettingsCb(BlazeError error, JobId id, ChangeGameSettingsJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter  could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief (fake) notification that the game settings have changed - called by GMApi
    ***************************************************************************************************/
    void Game::onNotifyGameSettingsChanged(const GameSettings *newGameSettings)
    {
        mGameSettings = *newGameSettings;
        mDispatcher.dispatch("onGameSettingUpdated", &GameListener::onGameSettingUpdated, this);
    }

    /*! ************************************************************************************************/
    /*! \brief (fake) notification that the game settings have changed - called by GMApi
    ***************************************************************************************************/
    void Game::onNotifyGameModRegisterChanged(GameModRegister newGameModRegister)
    {
        mGameModRegister = newGameModRegister;
        mDispatcher.dispatch("onGameModRegisterUpdated", &GameListener::onGameModRegisterUpdated, this);
    }

    /*! ************************************************************************************************/
    /*! \brief notification that the game entry criteria has changed - called by GMApi
    ***************************************************************************************************/
    void Game::onNotifyGameEntryCriteriaChanged(const EntryCriteriaMap& entryCriteriaMap, const RoleEntryCriteriaMap &roleEntryCriteria)
    {
        entryCriteriaMap.copyInto(mEntryCriteria);

        // iterate over the roles, clear GEC not sent in the update
        RoleEntryCriteriaMap::const_iterator roleEntryCriteriaMapEnd = roleEntryCriteria.end();
        RoleCriteriaMap::iterator roleCriteriaIter = mRoleInformation.getRoleCriteriaMap().begin();
        RoleCriteriaMap::iterator roleCriteriaEnd = mRoleInformation.getRoleCriteriaMap().end();
        for(; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            RoleEntryCriteriaMap::const_iterator roleEntryCriteriaIter = roleEntryCriteria.find(roleCriteriaIter->first);
            if (roleEntryCriteriaIter == roleEntryCriteriaMapEnd)
            {
                // clear entry criteria
                roleCriteriaIter->second->getRoleEntryCriteriaMap().clear();
            }
            else
            {
                // criteria was set
                roleEntryCriteriaIter->second->copyInto(roleCriteriaIter->second->getRoleEntryCriteriaMap());
            }
        }

        mDispatcher.dispatch("onGameEntryCriteriaUpdated", &GameListener::onGameEntryCriteriaUpdated, this);
    }

    /*! ************************************************************************************************/
    /*! \brief (fake) notification that the game's been reset (called by GMApi)
    ***************************************************************************************************/
    void Game::onNotifyGameReset(const ReplicatedGameData *gameData)
    {
        BlazeAssertMsg(mActivePlayers.empty(), "Error: resetting game that's not emtpy!");

        // otherwise, reset the game's data & dispatch
        mUUID = gameData->getUUID();
        if (EA_LIKELY(mDedicatedHostEndpoint != nullptr))
        {
            mDedicatedHostEndpoint->mUUID = gameData->getUUID();
        }

        initGameBaseData(gameData);

        // set the secret to the player who reset the game
        mPersistedGameIdSecret.setData(gameData->getPersistedGameIdSecret().getData(), gameData->getPersistedGameIdSecret().getCount());
        gameData->getCurrentlyAcceptedPlatformList().copyInto(mClientPlatformList);
        mIsCrossplayEnabled = gameData->getIsCrossplayEnabled();

        // set the external session identification even on the game server, for debugging purposes
        gameData->getExternalSessionIdentification().copyInto(mExternalSessionIdentification);
        oldToNewExternalSessionIdentificationParams(gameData->getExternalSessionTemplateName(), gameData->getExternalSessionName(), gameData->getExternalSessionCorrelationId(), gameData->getNpSessionId(), mExternalSessionIdentification);

        BLAZE_SDK_DEBUGF("[GAME] Resetting game(%" PRIu64 ":%s) UUID: %s\n", mGameId, mName.c_str(), mUUID.c_str());
        mGameManagerApi.getNetworkAdapter()->resetGame(this);        
        mDispatcher.dispatch("onGameReset", &GameListener::onGameReset, this);
    }

    /*! ************************************************************************************************/
    /*! \brief handle a change in the game's admin list
    ***************************************************************************************************/
    void Game::onNotifyGameAdminListChange(PlayerId playerId, UpdateAdminListOperation updateOperation, PlayerId updaterId)
    {
        const Player *player = getPlayerById(playerId);

        if (updateOperation == GM_ADMIN_ADDED)
        {
            if (player == nullptr && (mExternalOwnerInfo.getPlayerId() != Blaze::INVALID_BLAZE_ID && mExternalOwnerInfo.getPlayerId() != playerId))
            {
                BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyAdminListChange for unknown player(%" PRId64 ").\n", mGameId, playerId);
                return;
            }
            if (eastl::find(mAdminList.begin(), mAdminList.end(), playerId) != mAdminList.end())
            {
                BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") NotifyAdminListChange for player(%" PRId64 ") already an admin\n", mGameId, playerId);
            }
            else
            {
                mAdminList.push_back(playerId);
            }
        }
        else if (updateOperation == GM_ADMIN_REMOVED)
        {
            // Note: we don't require a player to remove admin rights (since the player removal and admin updates are separate notifications).
            PlayerIdList::iterator removeIterator = eastl::find(mAdminList.begin(), mAdminList.end(), playerId);
            if (removeIterator == mAdminList.end())
            {
                BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyAdminListChange for player(%" PRId64 ") (they're not an admin)\n", mGameId, playerId);
                return;
            }

            mAdminList.erase(removeIterator);
        }
        else if (updateOperation == GM_ADMIN_MIGRATED)
        {
            // Remove the migrator player from the admin list
            PlayerIdList::iterator removeIterator = eastl::find(mAdminList.begin(), mAdminList.end(), updaterId);
            if (removeIterator == mAdminList.end())
            {
                BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyAdminListChange for player(%" PRId64 ") (they're not an admin)\n", mGameId, updaterId);
                return;
            }

            mAdminList.erase(removeIterator);

            if (player == nullptr)
            {
                BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyAdminListChange for unknown player(%" PRId64 ").\n", mGameId, playerId);
                return;
            }
            if (eastl::find(mAdminList.begin(), mAdminList.end(), playerId) != mAdminList.end())
            {
                BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") NotifyAdminListChange for player(%" PRId64 ") already an admin\n", mGameId, playerId);
            }
            else
            {
                mAdminList.push_back(playerId);
            }
        }

        mDispatcher.dispatch("onAdminListChanged", &GameListener::onAdminListChanged, this, player, updateOperation, updaterId);
    }


    JobId Game::changeTeamIdAtIndex(const TeamIndex &teamIndex, const TeamId &newTeamId, const ChangeTeamIdJobCb &titleCb)
    {
        if ( (teamIndex == UNSPECIFIED_TEAM_INDEX) || (teamIndex == TARGET_USER_TEAM_INDEX) || (newTeamId == INVALID_TEAM_ID) )
        {
            return JobCbHelper("changeTeamIdAtIndexCb", titleCb, GAMEMANAGER_ERR_TEAM_NOT_ALLOWED, this);
        }

        TeamInfo* team = getTeamByIndex(teamIndex);
        if (team == nullptr)
        {
            // can't change ID for team that doesn't exist
            return JobCbHelper("changeTeamIdAtIndexCb", titleCb, GAMEMANAGER_ERR_TEAM_NOT_ALLOWED, this);
        }

        // client side validation (return err ok if new team is the same as current team)
        if ( team->mTeamId == newTeamId )
        {
            return JobCbHelper("changeTeamIdAtIndexCb", titleCb, ERR_OK, this);
        }

        // setup rpc request
        ChangeTeamIdRequest changeTeamIdRequest;
        changeTeamIdRequest.setGameId(mGameId);
        changeTeamIdRequest.setTeamIndex(teamIndex);
        changeTeamIdRequest.setNewTeamId(newTeamId);

        // send request, passing title's callback as the payload            
        JobId jobId = mGameManagerApi.getGameManagerComponent()->changeGameTeamId(changeTeamIdRequest, MakeFunctor(this, &Game::changeGameTeamIdCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief handle RPC response from changeTeamId
    ***************************************************************************************************/
    void Game::changeGameTeamIdCb(BlazeError error, JobId id, ChangeTeamIdJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

     /*! ************************************************************************************************/
    /*! \brief swap the players between teams and roles (sends RPC to server)
    ***************************************************************************************************/
    JobId Game::swapPlayerTeamsAndRoles(const SwapPlayerDataList &swapPlayerDataList, const SwapPlayerTeamsAndRolesJobCb &titleCb)
    {
        // setup rpc request
        SwapPlayersRequest swapPlayersRequest;
        swapPlayersRequest.setGameId(mGameId);
        swapPlayerDataList.copyInto(swapPlayersRequest.getSwapPlayers());        

        // send request, passing title's callback as the payload            
        JobId jobId = mGameManagerApi.getGameManagerComponent()->swapPlayers(swapPlayersRequest, MakeFunctor(this, &Game::internalSwapPlayersCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief handle RPC response from swapTeam
    ***************************************************************************************************/
    void Game::internalSwapPlayersCb(const SwapPlayersErrorInfo *swapPlayersError, BlazeError error, JobId id, SwapPlayerTeamsAndRolesJobCb titleCb)
    {
        titleCb(error, this, swapPlayersError, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! **********************************************************************************************************/
    /*! \brief  returns the game mode attribute value for this game.
        \return    Returns the game mode attribute value for this game, 
                    nullptr if the game is missing an attribute for the game mode.
    **************************************************************************************************************/
    const char8_t* Game::getGameMode() const
    {
        return getGameAttributeValue(mGameModeAttributeName);
    }

    /*! ************************************************************************************************/
    /*! \brief create/update a single attribute for this game (sends RPC to server)
    ***************************************************************************************************/
    JobId Game::setGameAttributeValue(const char8_t *attributeName, const char8_t *attributeValue, const ChangeGameAttributeJobCb &titleCb)
    {
        const char8_t *currentAttribValue = getGameAttributeValue(attributeName);

        // don't send an RPC if we're just setting an existing value to its current value
        if ( (currentAttribValue != nullptr) && !strcmp(attributeValue,currentAttribValue) )
        {
            return JobCbHelper("setGameAttributeValueCb", titleCb, ERR_OK, this);
        }

        // setup rpc request
        SetGameAttributesRequest setGameAttribRequest;
        setGameAttribRequest.setGameId(mGameId);
        setGameAttribRequest.getGameAttributes().insert(eastl::make_pair(attributeName, attributeValue));

        // send request, passing title's callback id along so we can dispatch it later
        JobId jobId = getAdminGameManagerComponent()->setGameAttributes(setGameAttribRequest, 
                MakeFunctor(this, &Game::internalSetGameAttributeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! **********************************************************************************************************/
    /*! \brief  create/update multiple attributes for this game in a single RPC.  NOTE: the server merges existing
            attributes with the supplied attributeMap (so omitted attributes retain their previous values).
    **************************************************************************************************************/
    JobId Game::setGameAttributeMap(const Collections::AttributeMap *changingAttributeMap, const ChangeGameAttributeJobCb &titleCb)
    {
        if (changingAttributeMap->empty())
        {
            return JobCbHelper("setGameAttributeMapCb", titleCb, ERR_OK, this);
        }

        // setup rpc request
        SetGameAttributesRequest setGameAttribRequest;
        setGameAttribRequest.setGameId(mGameId);
        setGameAttribRequest.getGameAttributes().insert(changingAttributeMap->begin(), changingAttributeMap->end());

        // send request
        JobId jobId = getAdminGameManagerComponent()->setGameAttributes(setGameAttribRequest, 
                MakeFunctor(this, &Game::internalSetGameAttributeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief setGameAttribute rpc callback
    ***************************************************************************************************/
    void Game::internalSetGameAttributeCb(BlazeError error, JobId id, ChangeGameAttributeJobCb titleCb)
    {
        titleCb(error, this, id);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief handle a notification that the game attribs have changed
    ***************************************************************************************************/
    void Game::onNotifyGameAttributeChanged(const Collections::AttributeMap *newAttributeMap)
    {
        BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") updating %d game attribute(s).\n", mGameId, (int32_t)newAttributeMap->size());
        Collections::AttributeMap::const_iterator newAttribMapIter = newAttributeMap->begin();
        Collections::AttributeMap::const_iterator end = newAttributeMap->end();
        for ( ; newAttribMapIter != end; ++newAttribMapIter )
        {
            // Note: initially, I was using "existingMap.insert(new.begin(), new.end());",
            //  but this wouldn't update values if a key already existed for it.
            const char8_t *attribName = newAttribMapIter->first.c_str();
            const char8_t *attribValue = newAttribMapIter->second.c_str();
            mGameAttributeMap[attribName] = attribValue;
            // GM_AUDIT: in theory, we shouldn't be altering the map before the dispatch - it can invalidate
            //  cached pointers and prevents titles from knowing what the previous attrib value(s) were.
        }

        mDispatcher.dispatch("onGameAttributeUpdated", &GameListener::onGameAttributeUpdated, this, newAttributeMap);
    }

    /*! ************************************************************************************************/
    /*! \brief create/update a single attribute for this game (sends RPC to server)
    ***************************************************************************************************/
    JobId Game::setDedicatedServerAttributeValue(const char8_t *attributeName, const char8_t *attributeValue, const ChangeGameAttributeJobCb &titleCb)
    {
        const char8_t *currentAttribValue = getDedicatedServerAttributeValue(attributeName);

        // don't send an RPC if we're just setting an existing value to its current value
        if ( (currentAttribValue != nullptr) && !strcmp(attributeValue,currentAttribValue) )
        {
            return JobCbHelper("setDedicatedServerAttributeValueCb", titleCb, ERR_OK, this);
        }

        // setup rpc request
        SetDedicatedServerAttributesRequest setDedicatedServerAttribRequest;
        setDedicatedServerAttribRequest.getGameIdList().push_back(mGameId);
        setDedicatedServerAttribRequest.getDedicatedServerAttributes().insert(eastl::make_pair(attributeName, attributeValue));

        // send request, passing title's callback id along so we can dispatch it later
        JobId jobId = getAdminGameManagerComponent()->setDedicatedServerAttributes(setDedicatedServerAttribRequest, 
                MakeFunctor(this, &Game::internalSetDedicatedServerAttributeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! **********************************************************************************************************/
    /*! \brief  create/update multiple attributes for this game in a single RPC.  NOTE: the server merges existing
            attributes with the supplied attributeMap (so omitted attributes retain their previous values).
    **************************************************************************************************************/
    JobId Game::setDedicatedServerAttributeMap(const Collections::AttributeMap *changingAttributeMap, const ChangeGameAttributeJobCb &titleCb)
    {
        if (changingAttributeMap->empty())
        {
            return JobCbHelper("setDedicatedServerAttributeMapCb", titleCb, ERR_OK, this);
        }

        // setup rpc request
        SetDedicatedServerAttributesRequest setDedicatedServerAttribRequest;
        setDedicatedServerAttribRequest.getGameIdList().push_back(mGameId);
        setDedicatedServerAttribRequest.getDedicatedServerAttributes().insert(changingAttributeMap->begin(), changingAttributeMap->end());

        // send request
        JobId jobId = getAdminGameManagerComponent()->setDedicatedServerAttributes(setDedicatedServerAttribRequest, 
                MakeFunctor(this, &Game::internalSetDedicatedServerAttributeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief setDedicatedServerAttribute rpc callback
    ***************************************************************************************************/
    void Game::internalSetDedicatedServerAttributeCb(BlazeError error, JobId id, ChangeGameAttributeJobCb titleCb)
    {
        titleCb(error, this, id);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief handle a notification that the game attribs have changed
    ***************************************************************************************************/
    void Game::onNotifyDedicatedServerAttributeChanged(const Collections::AttributeMap *newAttributeMap)
    {
        BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") updating %d dedicated server attribute(s).\n", mGameId, (int32_t)newAttributeMap->size());
        Collections::AttributeMap::const_iterator newAttribMapIter = newAttributeMap->begin();
        Collections::AttributeMap::const_iterator end = newAttributeMap->end();
        for (; newAttribMapIter != end; ++newAttribMapIter)
        {
            // Note: initially, I was using "existingMap.insert(new.begin(), new.end());",
            //  but this wouldn't update values if a key already existed for it.
            const char8_t *attribName = newAttribMapIter->first.c_str();
            const char8_t *attribValue = newAttribMapIter->second.c_str();
            mDedicatedServerAttributeMap[attribName] = attribValue;
            // GM_AUDIT: in theory, we shouldn't be altering the map before the dispatch - it can invalidate
            //  cached pointers and prevents titles from knowing what the previous attrib value(s) were.
        }

        mDispatcher.dispatch("onDedicatedServerAttributeUpdated", &GameListener::onDedicatedServerAttributeUpdated, this, newAttributeMap);
    }


    /*! **********************************************************************************************************/
    /*! \brief  add player to game admin list
    **************************************************************************************************************/
    JobId Game::addPlayerToAdminList(Player *player, const UpdateAdminListJobCb& titleCb)
    {
        if ( !containsPlayer(player) )
        {
            return JobCbHelper("addPlayerToAdminListCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, player);
        }

        // send the request to server
        UpdateAdminListRequest updateAdminListRequest;
        updateAdminListRequest.setGameId(mGameId);
        updateAdminListRequest.setAdminPlayerId(player->getId());

        JobId jobId = mGameManagerApi.getGameManagerComponent()->addAdminPlayer(updateAdminListRequest, MakeFunctor(this, &Game::internalUpdateAdminPlayerCb), 
                                             titleCb, player->getId());
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    JobId Game::addPlayerToAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdJobCb& titleCb)
    {
        if (blazeId == INVALID_BLAZE_ID)
        {
            return JobCbHelper("addPlayerToAdminListCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, blazeId);
        }

        // send the request to server
        UpdateAdminListRequest updateAdminListRequest;
        updateAdminListRequest.setGameId(mGameId);
        updateAdminListRequest.setAdminPlayerId(blazeId);

        JobId jobId = mGameManagerApi.getGameManagerComponent()->addAdminPlayer(updateAdminListRequest, MakeFunctor(this, &Game::internalUpdateAdminPlayerBlazeIdCb), 
                                             titleCb, blazeId);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! **********************************************************************************************************/
    /*! \brief remove a player from game admin list
    **************************************************************************************************************/
    JobId Game::removePlayerFromAdminList(Player *player, const UpdateAdminListJobCb& titleCb)
    {
        if ( !containsPlayer(player) )
        {
            return JobCbHelper("removePlayerFromAdminListCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, player);
        }

        // send request to server
        UpdateAdminListRequest updateAdminListRequest;
        updateAdminListRequest.setGameId(mGameId);
        updateAdminListRequest.setAdminPlayerId(player->getId());
        
        JobId jobId = mGameManagerApi.getGameManagerComponent()->removeAdminPlayer(updateAdminListRequest, MakeFunctor(this, &Game::internalUpdateAdminPlayerCb), 
                                                titleCb, player->getId());
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }
    JobId Game::removePlayerFromAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdJobCb& titleCb)
    {
        if (blazeId == INVALID_BLAZE_ID)
        {
            return JobCbHelper("removePlayerFromAdminListCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, blazeId);
        }

        // send request to server
        UpdateAdminListRequest updateAdminListRequest;
        updateAdminListRequest.setGameId(mGameId);
        updateAdminListRequest.setAdminPlayerId(blazeId);
        
        JobId jobId = mGameManagerApi.getGameManagerComponent()->removeAdminPlayer(updateAdminListRequest, MakeFunctor(this, &Game::internalUpdateAdminPlayerBlazeIdCb), 
                                                titleCb, blazeId);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! **********************************************************************************************************/
    /*! \brief migrate local players admin rights to specified player
    **************************************************************************************************************/
    JobId Game::migratePlayerAdminList(Player *player, const UpdateAdminListJobCb &titleCb)
    {
        if ( !containsPlayer(player) )
        {
            return JobCbHelper("migratePlayerAdminListCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, player);
        }

        // send request to server
        UpdateAdminListRequest updateAdminListRequest;
        updateAdminListRequest.setGameId(mGameId);
        updateAdminListRequest.setAdminPlayerId(player->getId());

        JobId jobId = mGameManagerApi.getGameManagerComponent()->migrateAdminPlayer(updateAdminListRequest, MakeFunctor(this, &Game::internalUpdateAdminPlayerCb), 
            titleCb, player->getId());
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }
    JobId Game::migratePlayerAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdJobCb& titleCb)
    {
        if (blazeId == INVALID_BLAZE_ID)
        {
            return JobCbHelper("migratePlayerAdminListCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, blazeId);
        }

        // send request to server
        UpdateAdminListRequest updateAdminListRequest;
        updateAdminListRequest.setGameId(mGameId);
        updateAdminListRequest.setAdminPlayerId(blazeId);

        JobId jobId = mGameManagerApi.getGameManagerComponent()->migrateAdminPlayer(updateAdminListRequest, MakeFunctor(this, &Game::internalUpdateAdminPlayerBlazeIdCb), 
            titleCb, blazeId);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief shared rpc handler for adding/removing admins
    ***************************************************************************************************/
    void Game::internalUpdateAdminPlayerBlazeIdCb(BlazeError err, JobId id, UpdateAdminListBlazeIdJobCb titleCb, PlayerId playerId)
    {
        titleCb(err, this, playerId, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/
    void Game::internalUpdateAdminPlayerCb(BlazeError err, JobId id, UpdateAdminListJobCb titleCb, PlayerId playerId)
    {
        Player *updatedAdminPlayer = getPlayerById( playerId );
        titleCb(err, this, updatedAdminPlayer, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    JobId Game::setPresenceMode(PresenceMode presenceMode, const SetPresenceModeJobCb& titleCb)
    {
        //Send request to server
        SetPresenceModeRequest request;
        request.setGameId(mGameId);
        request.setPresenceMode(presenceMode);

        JobId jobId = getAdminGameManagerComponent()->setPresenceMode(request, MakeFunctor(this, &Game::internalSetPresenceModeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    void Game::internalSetPresenceModeCb(BlazeError err, JobId id, SetPresenceModeJobCb titleCb)
    {
        titleCb(err, this, id);
    }

    /*! ************************************************************************************************/
    /*! \brief handle the game's capacity changing
    ***************************************************************************************************/
    void Game::onNotifyGameCapacityChanged(const SlotCapacitiesVector &newSlotCapacities, const TeamDetailsList& newTeamRosters, const RoleInformation& roleInformation)
    {
        // copy over the new role information
        roleInformation.copyInto(mRoleInformation);

        // copy the new capacities to the game object
        newSlotCapacities.copyInto(mPlayerCapacity);

        // if the rosters are empty team membership hasn't changed
        if (!newTeamRosters.empty())
        {
            // update team Ids and player team indexes
            mTeamIndexMap.clear();
            mTeamIndexMap.reserve(newTeamRosters.size());
            mTeamInfoVector.clear();
            mTeamInfoVector.reserve(newTeamRosters.size());
            for (TeamIndex i = 0; i < newTeamRosters.size(); ++i)
            {
                mTeamInfoVector.push_back();
                TeamInfo &newTeamInfo = mTeamInfoVector.back();
                newTeamInfo.mTeamId = newTeamRosters[i]->getTeamId();
                newTeamInfo.mTeamSize = (uint16_t)newTeamRosters[i]->getTeamRoster().size();

                // Setup the default count (0) for each role:
                RoleCriteriaMap::iterator curRole = mRoleInformation.getRoleCriteriaMap().begin();
                RoleCriteriaMap::iterator endRole = mRoleInformation.getRoleCriteriaMap().end();
                for (; curRole != endRole; ++curRole)
                {
                    newTeamInfo.mRoleSizeMap[curRole->first] = 0;
                }

                // Increment for each role with an active player:
                const TeamMemberInfoList &newTeamRoster = newTeamRosters[i]->getTeamRoster();
                for (uint16_t j = 0; j < newTeamRoster.size(); ++j)
                {
                    const char8_t *roleName = newTeamRoster[j]->getPlayerRole();
                    RoleMap::const_iterator roleIter = newTeamInfo.mRoleSizeMap.find(roleName);
                    if (roleIter == newTeamInfo.mRoleSizeMap.end())
                    {
                        BLAZE_SDK_DEBUGF("[Game::onNotifyGameCapacityChanged] Game(%" PRIu64 ") has player in unknown role(%s) after capacity change\n", mGameId, roleName);
                        newTeamInfo.mRoleSizeMap[roleName] = 1;
                    }
                    else
                    {
                        ++(newTeamInfo.mRoleSizeMap[roleName]);
                    }
                }

                mTeamIndexMap[newTeamInfo.mTeamId] = i;
            }
        }

        mGameManagerApi.getNetworkAdapter()->updateCapacity(this);

        BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ":%s): updating Game slot capacities (total: %d, public participant: %d private participant: %d public spectator: %d private spectator: %d)\n",
            mGameId, getName(), getPlayerCapacityTotal(), getPlayerCapacity(SLOT_PUBLIC_PARTICIPANT), getPlayerCapacity(SLOT_PRIVATE_PARTICIPANT), getPlayerCapacity(SLOT_PUBLIC_SPECTATOR), getPlayerCapacity(SLOT_PRIVATE_SPECTATOR));

        mDispatcher.dispatch("onPlayerCapacityUpdated", &GameListener::onPlayerCapacityUpdated, this);

        // if the rosters are empty team membership hasn't changed
        if (!newTeamRosters.empty())
        {
            // second loop through required because the capacity update notification needs to come first
            // notify for players who have been moved, and assign them to new teams
            for(TeamIndex i = 0; i < newTeamRosters.size(); ++i)
            {
                // assign players to this team
                TeamMemberInfoList &currentTeamRoster = newTeamRosters[i]->getTeamRoster();
                for(uint16_t j = 0; j < currentTeamRoster.size(); ++j)
                {
                    PlayerId playerId = currentTeamRoster[j]->getPlayerId();
                    Player *player = getPlayerById(playerId);
                    if (player != nullptr)
                    {
                        if (player->getTeamIndex() != i)
                        {   
                            // don't update the team size, it's been done already
                            player->onNotifyGamePlayerTeamChanged(i);

#if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
                            mGameManagerApi.updateGamePresenceForLocalUser(mGameManagerApi.getLocalUserIndex(player->getId()), *this, UPDATE_PRESENCE_REASON_TEAM_CHANGE, nullptr);
#endif
                        }
                        else
                        {
                            BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") ignoring teams update in [NotifyGameCapacityChanged] for local player(%" PRId64 ") without team change.\n", mGameId, playerId);
                        }

                        if (blaze_stricmp(player->getRoleName(), currentTeamRoster[j]->getPlayerRole()) != 0)
                        {   
                            // don't update the team size, it's been done already
                            player->onNotifyGamePlayerRoleChanged(currentTeamRoster[j]->getPlayerRole());
                        }
                        else
                        {
                            BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") ignoring role update in [NotifyGameCapacityChanged] for local player(%" PRId64 ") without role change.\n", mGameId, playerId);
                        }
                    }
                    else
                    {
                        BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") ignoring teams update in [NotifyGameCapacityChanged] for unknown local player(%" PRId64 ").\n", mGameId, playerId);
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief notification that a player's attribs have changed
    ***************************************************************************************************/
    void Game::onNotifyPlayerAttributeChanged(PlayerId playerId, const Collections::AttributeMap *newAttributeMap)
    {
        Player *player = getPlayerById(playerId);
        if (player != nullptr)
        {
            player->onNotifyPlayerAttributeChanged(newAttributeMap);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyPlayerAttributeChanged for unknown local player(%" PRId64 ").\n", mGameId, playerId);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief notification that a player's state has changed
    ***************************************************************************************************/
    void Game::onNotifyGamePlayerStateChanged(const PlayerId playerId, const PlayerState state, uint32_t userIndex)
    {
        Player *player = getPlayerById(playerId);
        if (player != nullptr)
        {
            // If we're moving to a RESERVED state with a disconnect reservation, we need to disconnect the player from the mesh.
            bool wasActiveNowReserved = (state == RESERVED && player->isActive());
            if (wasActiveNowReserved)    // Note: We don't have a player->getHasDisconnectReservation() at this point. 
            {
                // Remove reason only needs to be != GROUP_LEFT
                PlayerRemovedReason playerRemovedReason = PLAYER_LEFT_MAKE_RESERVATION;
                cleanupPlayerNetConnection(player, true, nullptr, isTopologyHost(), playerRemovedReason);
                // update the player's endpoint with a new one, since we're leaving the mesh
                GameEndpoint* originalEndpoint = player->getPlayerEndpoint();
                if (originalEndpoint != nullptr)
                {
                    bool updateConnectionSlotId = true;
                    // the current player will still be active, but we need to see if anyone else is still active in the game on this endpoint
                    if (originalEndpoint->getMemberCount() > 1)
                    {
                        PlayerVector connectedPlayerVector(MEM_GROUP_FRAMEWORK_TEMP, "onNotifyGamePlayerStateChanged.connectedPlayerVector");
                        getActivePlayerByConnectionGroupId(originalEndpoint->getConnectionGroupId(), connectedPlayerVector);
                        // now iterate over the players to see if any are active- 
                        for (const auto& connectedPlayer : connectedPlayerVector)
                        {
                            // at this stage the current player will be active, since we're pre-state update
                            // we'll skip over them.
                            if ((connectedPlayer->getId() != player->getId()) && connectedPlayer->isActive())
                            {
                                // if anyone else is still active, though, we're not going to update the connection slot id
                                updateConnectionSlotId = false;
                                break;
                            }
                        }
                    }
                    ReplicatedGamePlayer tempPlayerData;
                    // updatePlayerEndpoint only uses the connectionSlotId and connectionGroupId.
                    tempPlayerData.setConnectionSlotId(DEFAULT_JOINING_SLOT);
                    tempPlayerData.setConnectionGroupId(player->getConnectionGroupId());

                    // we've got to update the player endpoint because we're no longer part of the mesh
                    // only update the connectionSlotId if this is the only player on the endpoint
                    updatePlayerEndpoint(*player, tempPlayerData, updateConnectionSlotId);
                }
                // don't forget to remove from mActivePlayers
                PlayerRosterList::iterator activeIter = mActivePlayers.find(player->getSlotId());
                if (activeIter != mActivePlayers.end())
                {
                    mActivePlayers.erase(activeIter);
                }
                // Since the player's connection is no longer required, see if we can trigger any deferred CCS actions:
                handleDeferedCCSActions(userIndex);
            }
            player->onNotifyGamePlayerStateChanged(state);

            if (wasActiveNowReserved && player->isLocalPlayer())
            {
                // Get the local user, to find the user index: 
                UserManager::LocalUser* localUser = mGameManagerApi.getUserManager()->getLocalUserById(player->getUser()->getId());

                // Special edge case check for local players that switch back to reserved when their join-triggered host injection fails (and D/C reservations are enabled): 
                GameManagerApiJob *gameManagerApiJob = mGameManagerApi.getJob(localUser->getUserIndex(), getId());
                if (gameManagerApiJob != nullptr)
                {
                    gameManagerApiJob->dispatch(GAMEMANAGER_ERR_NO_HOSTS_AVAILABLE_FOR_INJECTION, this);
                    mGameManagerApi.getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                }
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyGamePlayerStateChanged for unknown local player(%" PRId64 ").\n", mGameId, playerId);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief notification that a player's Team/Role has changed
    ***************************************************************************************************/
    void Game::onNotifyGamePlayerTeamRoleSlotChanged( const PlayerId playerId, const TeamIndex newTeamIndex, const RoleName &newPlayerRole, SlotType newSlotType )
    {
        Player *player = getPlayerById(playerId);
        if (player != nullptr)
        {
            bool isNewParticipantSlot = isParticipantSlot(newSlotType);
            bool isPreviousParticipantSlot = isParticipantSlot(player->getSlotType());

            // keeping a count of team members
            if (isNewParticipantSlot)
            {
                if (incrementLocalTeamSize(newTeamIndex, newPlayerRole) == false)
                {
                    BLAZE_SDK_DEBUGF("[Game] Warning: Game(%" PRIu64 ") dropping onNotifyGamePlayerTeamRoleSlotChanged for unknown new team index(%u)\n", mGameId, newTeamIndex);
                    return;
                }
            }

            TeamIndex previousTeamIndex = player->getTeamIndex();
            const RoleName &previousPlayerRole = player->getRoleName();
            SlotType previousSlotType = player->getSlotType();
            if (isPreviousParticipantSlot)
            {
                if (decrementLocalTeamSize(previousTeamIndex, previousPlayerRole) == false)
                {
                    BLAZE_SDK_DEBUGF("[Game] Warning: Game(%" PRIu64 ") dropping onNotifyGamePlayerTeamRoleSlotChanged for unknown previous team index(%u)\n", mGameId, newTeamIndex);
                    return;
                }
            }

            if (previousTeamIndex != newTeamIndex)
            {
                player->onNotifyGamePlayerTeamChanged(newTeamIndex);

#if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
                // reflect team change to UX.
                mGameManagerApi.updateGamePresenceForLocalUser(mGameManagerApi.getLocalUserIndex(player->getId()), *this, UPDATE_PRESENCE_REASON_TEAM_CHANGE, nullptr);
#endif
            }
            if (blaze_stricmp(previousPlayerRole, newPlayerRole) != 0)
            {
                player->onNotifyGamePlayerRoleChanged(newPlayerRole);
            }
            if (newSlotType != previousSlotType)
            {
                // We do an additional check when slot type changes, since it may affect the capacity if LIMITED users are used. 
                uint16_t oldPublic, oldPrivate, newPublic, newPrivate;
                getFirstPartyCapacity(oldPublic, oldPrivate);

                player->onNotifyGamePlayerSlotChanged(newSlotType);

                getFirstPartyCapacity(newPublic, newPrivate);
                if (newPublic != oldPublic || newPrivate != oldPrivate)
                {
                    mGameManagerApi.getNetworkAdapter()->updateCapacity(this);
                }

                --mPlayerSlotCounts[previousSlotType];
                ++mPlayerSlotCounts[newSlotType];
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyGamePlayerTeamRoleSlotChanged for unknown local player(%" PRId64 ").\n", mGameId, playerId);
        }
    }

    /*! \brief update the local team info's team size. return false if team info not found. */
    bool Game::incrementLocalTeamSize( const TeamIndex newTeamIndex, const RoleName& newRoleName )
    {
        if (newTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            TeamInfo* newTeamInfo = getTeamByIndex(newTeamIndex);
            if (newTeamInfo == nullptr)
            {
                return false;
            }

            if (newTeamInfo->mRoleSizeMap.find(newRoleName) == newTeamInfo->mRoleSizeMap.end())
            {
                BLAZE_SDK_DEBUGF("[Game] Warning: Game(%" PRIu64 ") incrementLocalTeamSize for unknown new player role(%s)\n", mGameId, newRoleName.c_str());
                newTeamInfo->mRoleSizeMap[newRoleName] = 1;
            }
            else
            {
                ++(newTeamInfo->mRoleSizeMap[newRoleName]);
            }
            ++(newTeamInfo->mTeamSize);
        }
        return true;
    }

    bool Game::decrementLocalTeamSize( const TeamIndex previousTeamIndex, const RoleName& previousRoleName )
    {
        if (previousTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            TeamInfo* previousTeamInfo = getTeamByIndex(previousTeamIndex);
            if (previousTeamInfo == nullptr)
            {
                return false;
            }
            if (previousTeamInfo->mRoleSizeMap.find(previousRoleName) == previousTeamInfo->mRoleSizeMap.end())
            {
                BLAZE_SDK_DEBUGF("[Game] Warning: Game(%" PRIu64 ") decrementLocalTeamSize for unknown new player role(%s)\n", mGameId, previousRoleName.c_str());
                previousTeamInfo->mRoleSizeMap[previousRoleName] = 0;
            }
            else
            {
                --(previousTeamInfo->mRoleSizeMap[previousRoleName]);
            }
            --(previousTeamInfo->mTeamSize);
        }
        return true;
    }

    void Game::onNotifyGameTeamIdChanged( const TeamIndex teamIndex, const TeamId newTeamId )
    {
        TeamInfo *teamInfo = getTeamByIndex(teamIndex);
        if (teamInfo != nullptr)
        {
            mTeamIndexMap.erase(teamInfo->mTeamId);
            mTeamIndexMap[newTeamId] = teamIndex;
            teamInfo->mTeamId = newTeamId;

            // will get notifications to update players on old team
            mDispatcher.dispatch("onGameTeamIdUpdated", &GameListener::onGameTeamIdUpdated, this, teamIndex, newTeamId);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyGameTeamIdChanged for unknown local team(%d).\n", mGameId, teamIndex);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief notification that a player's custom data blob has changed
    ***************************************************************************************************/
    void Game::onNotifyPlayerCustomDataChanged(PlayerId playerId, const EA::TDF::TdfBlob *newBlob)
    {
        Player *player = getPlayerById(playerId);
        if (player != nullptr)
        {
            player->onNotifyPlayerCustomDataChanged(newBlob);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping onNotifyPlayerCustomDataChanged for unknown local player(%" PRId64 ").\n", mGameId, playerId);
        }
    }


    /*! ************************************************************************************************/
    /*! \brief set the total player capacity (participants and observers).
    ***************************************************************************************************/
    JobId Game::setPlayerCapacity(const SlotCapacities &newSlotCapacities, const ChangePlayerCapacityJobCb &titleCb)
    {
        TeamDetailsList emptyTeamsList;
        RoleInformation emptyRoleInformation;
        return setPlayerCapacityInternal(newSlotCapacities, emptyTeamsList, emptyRoleInformation, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief set the game's player capacities for each slot type (public and private).  Requires game admin rights.
    You cannot shrink a game's capacity for a slotType below the game's current playerCount for that slot type.
    You cannot enlarge a game's total capacity (sum of the capacities for each slot type) above the game's
    maxPlayerCapacity.
    You cannot change the game's total capacity to a value not evenly divisible by the number of teams in the session.
    Leaving newTeamsMap empty will leave the FactionId and count of teams unchanged.
    Players who must be on a specific team after the re-size should be put in the corresponding PlayerIdList of the supplied newTeamsMap. 
    If players are moved to another team, overall game capacity is reduced, or the number of teams in the game change, Blaze will make a best-effort to re-balance players between teams by player count.
    If a user specified in the request is no longer in the game session, the request is allowed to proceed as if the user was not mentioned.
 
 
    \param[in] newSlotCapacities the new capacity array for the game.
    \param[in] newTeamsComposition the the new faction IDs to use, and the players to assign to them. Omitted players are balanced between the teams as needed.
    \param[in] titleCb the functor dispatched indicating success/failure.
    ***************************************************************************************************/
    JobId Game::setPlayerCapacity(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const ChangePlayerCapacityJobCb &titleCb)
    {
        RoleInformation emptyRoleInformation;
        return setPlayerCapacityInternal(newSlotCapacities, newTeamsComposition, emptyRoleInformation, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief set the total player capacity with role information. 
    ***************************************************************************************************/
    JobId Game::setPlayerCapacity(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const RoleInformation& roleInformation, const ChangePlayerCapacityJobCb &titleCb)
    {
        return setPlayerCapacityInternal(newSlotCapacities, newTeamsComposition, roleInformation, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief set the total player and team capacity (participants and observers).
    ***************************************************************************************************/
    JobId Game::setPlayerCapacityInternal( const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const RoleInformation& roleInformation, const ChangePlayerCapacityJobCb &titleCb )
    {
        // validate the new player capacity setting
        SetPlayerCapacityRequest setPlayerCapacityRequest;
        uint16_t totalCapacity = 0;
        uint16_t totalParticipantCapacity = 0;
        uint16_t teamCapacity = 0;
        bool tooSmall = false;
        bool noChange = true;
        
        // tally new total capacity, set the capacity in the request, prep for some client validation.
        for (SlotCapacitiesVector::size_type type = SLOT_PUBLIC_PARTICIPANT; type != MAX_SLOT_TYPE && !tooSmall; ++type)
        {
            totalCapacity = static_cast<uint16_t>(totalCapacity + newSlotCapacities[type]);
            if (isParticipantSlot((SlotType)type))
                totalParticipantCapacity += (uint16_t)newSlotCapacities[type];

            if (noChange && newSlotCapacities[type] != mPlayerCapacity[type])
            {
                noChange = false;
            }
            tooSmall = newSlotCapacities[type] < mPlayerSlotCounts[type];
            setPlayerCapacityRequest.getSlotCapacities()[type] = newSlotCapacities[type];
        }

        // player capacity can't be 0
        if (EA_UNLIKELY(totalParticipantCapacity == 0))
        {
            return JobCbHelper("setPlayerCapacityInternalCb", titleCb, GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO, this, (const SwapPlayersErrorInfo*)nullptr);
        }

        if (!newTeamsComposition.empty())
        {
            if ((totalParticipantCapacity % newTeamsComposition.size()) != 0)
            {
                //teams must divide evenly into game capacity
                BLAZE_SDK_DEBUGF("[GAME]:setPlayerCapacityInternal() Warning: Game(%" PRIu64 ") supplied newTeamsComposition had (%" PRIu16 ") teams, but game capacity was (%" PRIu16 ").\n", 
                    mGameId, (uint16_t)newTeamsComposition.size(), totalParticipantCapacity);
                return JobCbHelper("setPlayerCapacityInternalCb", titleCb, GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS, this, (const SwapPlayersErrorInfo*)nullptr);
            }

            teamCapacity = totalParticipantCapacity / (uint16_t)newTeamsComposition.size();

            if (newTeamsComposition.size() != getTeamCount())
                noChange = false;
        }
        else
        {
            // no team changes in this request, make sure the capacity is evenly divisible with the current team count
            if ((getTeamCount() != 0) && (totalParticipantCapacity % getTeamCount()) != 0)
            {
                //teams must divide evenly into game capacity
                BLAZE_SDK_DEBUGF("[GAME]:setPlayerCapacityInternal() Warning: Game(%" PRIu64 ") supplied newTeamsComposition had (%" PRIu16 ") teams, but game capacity was (%" PRIu16 ").\n", 
                    mGameId, (uint16_t)newTeamsComposition.size(), totalParticipantCapacity);
                return JobCbHelper("setPlayerCapacityInternalCb", titleCb, GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS, this, (const SwapPlayersErrorInfo*)nullptr);
            }
        }

        
        if (teamCapacity != 0)
        {
            // need to verify teams are large enough for assigned players
            for(uint16_t i = 0; i < newTeamsComposition.size(); ++i)
            {
                if (newTeamsComposition[i]->getTeamRoster().size() > teamCapacity)
                {
                    // assigned too many players to a team
                    BLAZE_SDK_DEBUGF("[GAME]:setPlayerCapacityInternal() Warning: Game(%" PRIu64 ") supplied newTeamsComposition attempted to put (%" PRIu16 ") users on team at index (%" PRIu16 "), but team capacity was (%" PRIu16 ").\n", 
                        mGameId, (uint16_t)newTeamsComposition[i]->getTeamRoster().size(), i, teamCapacity);
                    return JobCbHelper("setPlayerCapacityInternalCb", titleCb, GAMEMANAGER_ERR_TEAM_FULL, this, (const SwapPlayersErrorInfo*)nullptr);
                }
            }
        }

        newTeamsComposition.copyInto(setPlayerCapacityRequest.getTeamDetailsList());

        if (roleInformation.getRoleCriteriaMap().empty())
        {
            // use default values
            RoleCriteria *defaultRoleCriteria = setPlayerCapacityRequest.getRoleInformation().getRoleCriteriaMap().allocate_element();
            defaultRoleCriteria->setRoleCapacity(teamCapacity ? teamCapacity : (totalParticipantCapacity/getTeamCount()));
            setPlayerCapacityRequest.getRoleInformation().getRoleCriteriaMap()[GameManager::PLAYER_ROLE_NAME_DEFAULT] = defaultRoleCriteria;
        }
        else
        {
            roleInformation.copyInto(setPlayerCapacityRequest.getRoleInformation());
        }

        // can't shrink below current players count of the game
        if (EA_UNLIKELY(tooSmall))
        {
            return JobCbHelper("setPlayerCapacityInternalCb", titleCb, GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_SMALL, this, (const SwapPlayersErrorInfo*)nullptr);
        }

        // can't grow above max player capacity of the game
        if (EA_UNLIKELY(totalCapacity > mMaxPlayerCapacity))
        {
            return JobCbHelper("setPlayerCapacityInternalCb", titleCb, GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE, this, (const SwapPlayersErrorInfo*)nullptr);
        }

        setPlayerCapacityRequest.setGameId(mGameId);

        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();
        JobId jobId = gameManagerComponent->setPlayerCapacity(setPlayerCapacityRequest, MakeFunctor(this, &Game::setPlayerCapacityCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;

    }

    /*! ************************************************************************************************/
    /*! \brief internal rpc callback fro setPlayerCapacityInternal
    ***************************************************************************************************/
    void Game::setPlayerCapacityCb(const SwapPlayersErrorInfo *swapPlayersError, BlazeError error, JobId id, ChangePlayerCapacityJobCb titleCb)
    {
        titleCb(error, this, swapPlayersError, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief kick a player out of a game (and possibly ban them)
    ***************************************************************************************************/
    JobId Game::kickPlayer(const Player *player, const KickPlayerJobCb &titleCb, bool banPlayer /*=false*/, uint16_t kickReasonCode /*=0*/, const char8_t* titleContextStr /*= nullptr*/, KickReason kickReason /*= KICK_NORMAL*/)
    {
        // Note: check for player membership first, or else we'll throw the 'wrong' error for a dedicated server host trying
        //   to kick himself (or a dedicated server host trying to kick nullptr).
        if (!containsPlayer(player))
        {
            return JobCbHelper("kickPlayerCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, player);
        }

        if (player->isLocalPlayer())
        {
            BLAZE_SDK_DEBUGF("[GAME] Error: cannot kick a local player out of a game (use leaveGame instead).\n");
            return JobCbHelper("kickPlayerCb", titleCb, GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN, this, player);
        }


        // send kick request to server
        RemovePlayerRequest removePlayerRequest;
        removePlayerRequest.setGameId(mGameId);
        removePlayerRequest.setPlayerId(player->getId());
        removePlayerRequest.setPlayerRemovedTitleContext(kickReasonCode);
        removePlayerRequest.setTitleContextString(titleContextStr);

        PlayerRemovedReason removeReason = PLAYER_KICKED; // avoids the 'potentially uninitialized variable' compiler error, but in fact, will always be set in the following code.
        if (banPlayer)
        {
            switch (kickReason)
            {
                case KICK_NORMAL: removeReason = PLAYER_KICKED_WITH_BAN; break;
                case KICK_POOR_CONNECTION: removeReason = PLAYER_KICKED_POOR_CONNECTION_WITH_BAN; break;
                case KICK_UNRESPONSIVE_PEER: removeReason = PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN; break;
            }
        }
        else
        {
            switch (kickReason)
            {
                case KICK_NORMAL: removeReason = PLAYER_KICKED; break;
                case KICK_POOR_CONNECTION: removeReason = PLAYER_KICKED_POOR_CONNECTION; break;
                case KICK_UNRESPONSIVE_PEER: removeReason = PLAYER_KICKED_CONN_UNRESPONSIVE; break;
            }
        }
        removePlayerRequest.setPlayerRemovedReason(removeReason);

        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();

        // getConnectionGroupObjectId() never returns INVALID_OBJECT_ID, so we check the group id directly here:
        if (player->getConnectionGroupId() != 0)
        {
            // ensure all local users follow
            removePlayerRequest.setGroupId(player->getConnectionGroupObjectId());
            JobId jobId = gameManagerComponent->leaveGameByGroup(removePlayerRequest, MakeFunctor(this, &Game::internalRemovePlayerCallback), 
                    titleCb, player->getId());
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }
        else
        {
            // if the player doesn't have a connection group (reserved external player), call remove player instead. 
            JobId jobId = gameManagerComponent->removePlayer(removePlayerRequest, MakeFunctor(this, &Game::internalRemovePlayerCallback), 
                    titleCb, player->getId());
            RpcJobBase::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief callback for removePlayer RPC (ie: kickPlayer)
    ***************************************************************************************************/
    void Game::internalRemovePlayerCallback(BlazeError err, JobId id, KickPlayerJobCb titleCb, PlayerId kickedPlayerId)
    {
        // NOTE: we don't actually remove the player (yet); the player will be removed when the server's async kicked msg comes back...
        const Player *removedPlayer = getPlayerById( kickedPlayerId );
        titleCb(err, this, removedPlayer, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief ban a user from a game (user doesn't have to be online)
    ***************************************************************************************************/
    JobId Game::banUser(const BlazeId blazeId, const BanPlayerJobCb &titleCb, uint16_t titleContext /*= 0*/)
    {
        if (getLocalPlayer() != nullptr && getLocalPlayer()->getId() == blazeId)
        {
            BLAZE_SDK_DEBUGF("[GAME] Error: cannot ban yourself from a game.\n");
            return JobCbHelper("banUserCb", titleCb, GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN, this, blazeId);
        }


        // send ban request to server
        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();  

        BanPlayerRequest banPlayerRequest;
        banPlayerRequest.setGameId(mGameId);
        banPlayerRequest.getPlayerIds().push_back(blazeId);
        banPlayerRequest.setPlayerRemovedTitleContext(titleContext);
        JobId jobId = gameManagerComponent->banPlayer(banPlayerRequest, MakeFunctor(this, &Game::internalBanUserCallback), 
            titleCb, blazeId);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief ban a lot of users from a game (users don't have to be online)
    ***************************************************************************************************/
    JobId Game::banUser(PlayerIdList *playerIds, const BanPlayerListJobCb &titleCb, uint16_t titleContext /*= 0*/)
    {
        PlayerIdList::const_iterator it = playerIds->begin();
        PlayerIdList::const_iterator end = playerIds->end();

        BanPlayerRequest banPlayerRequest;
        banPlayerRequest.setGameId(mGameId);
        banPlayerRequest.getPlayerIds().reserve(playerIds->size());
        banPlayerRequest.setPlayerRemovedTitleContext(titleContext);

        for (; it != end; ++it)
        {
            banPlayerRequest.getPlayerIds().push_back(*it);

            if ( (getLocalPlayer() != nullptr) && (*it == getLocalPlayer()->getId()) )
            {
                BLAZE_SDK_DEBUGF("[GAME] Error: cannot ban yourself from a game.\n");
                return JobCbHelper("banUserCb", titleCb, GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN, this, playerIds);
            }
        }

        // send ban request to server
        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();
        
        JobId jobId = gameManagerComponent->banPlayer(banPlayerRequest, MakeFunctor(this, &Game::internalBanUsersCallback), 
            titleCb, playerIds);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal banPlayer RPC callback
    ***************************************************************************************************/
    void Game::internalBanUserCallback(BlazeError err, JobId id, BanPlayerJobCb titleCb, BlazeId bannedBlazeId)
    {
        titleCb(err, this, bannedBlazeId, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief internal banPlayerList RPC callback
    ***************************************************************************************************/
    void Game::internalBanUsersCallback(BlazeError err, JobId id, BanPlayerListJobCb titleCb, BlazeIdList*  bannedBlazeIds)
    {
        titleCb(err, this, bannedBlazeIds, id);
    } /*lint !e1746 Avoid lint to check whether parameter 'id' could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief remove player from the game banned list.
    ***************************************************************************************************/
    JobId Game::removePlayerFromBannedList(const BlazeId blazeId, const UnbanPlayerJobCb &titleCb)
    {
        RemovePlayerFromBannedListRequest removerPlayerRequest;
        removerPlayerRequest.setGameId(mGameId);
        removerPlayerRequest.setBlazeId(blazeId);
        
        // send the request that removing a player from banned list to server
        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();   

        JobId jobId = gameManagerComponent->removePlayerFromBannedList(removerPlayerRequest, MakeFunctor(this, &Game::internalUnbanUserCallback), 
            titleCb, blazeId);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal unbanPlayer RPC callback
    ***************************************************************************************************/
    void Game::internalUnbanUserCallback(BlazeError err, JobId id, UnbanPlayerJobCb titleCb, BlazeId unbannedBlazeId)
    {
        titleCb(err, this, unbannedBlazeId, id);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief clear the game banned list.
    ***************************************************************************************************/
    JobId Game::clearBannedList(const ClearBannedListJobCb &titleCb)
    {
        BannedListRequest clearRequest;
        clearRequest.setGameId(mGameId);

        // send clear request to server
        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();   

        JobId jobId = gameManagerComponent->clearBannedList(clearRequest, MakeFunctor(this, &Game::clearBannedListCallback), 
            titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief clear banned list RPC callback
    ***************************************************************************************************/
    void Game::clearBannedListCallback(BlazeError err, JobId id, ClearBannedListJobCb titleCb)
    {
        titleCb(err, this, id);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief get the game banned list.
    ***************************************************************************************************/
    JobId Game::getBannedList(const GetBannedListJobCb &titleCb)
    {
        BannedListRequest getRequest;
        getRequest.setGameId(mGameId);

        // send get request to server
        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();   

        JobId jobId = gameManagerComponent->getBannedList(getRequest, MakeFunctor(this, &Game::getBannedListCallback), 
            titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief RPC callback for getBannedList
    ***************************************************************************************************/
    void Game::getBannedListCallback(const BannedListResponse *response, BlazeError error, JobId id, GetBannedListJobCb titleCb)
    {
        titleCb(error, this, response, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief rename the game name.
    ***************************************************************************************************/
    JobId Game::updateGameName(const GameName &newName, const UpdateGameNameJobCb &titleCb)
    {
        UpdateGameNameRequest renameRequest;
        renameRequest.setGameId(mGameId);
        renameRequest.setGameName(newName);

        // send rename request to server
        GameManagerComponent *gameManagerComponent = getAdminGameManagerComponent();   

        JobId jobId = gameManagerComponent->updateGameName(renameRequest, MakeFunctor(this, &Game::updateGameNameCallback), 
            titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief rename the game name RPC callback
    ***************************************************************************************************/
    void Game::updateGameNameCallback(BlazeError err, JobId id, UpdateGameNameJobCb titleCb)
    {
        titleCb(err, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief intentionally start a host migration (without leaving the game).
    ***************************************************************************************************/
    JobId Game::migrateHostToPlayer(const Player *suggestedNewHost, const MigrateHostJobCb &titleCb)
    {
        if (EA_UNLIKELY(suggestedNewHost == nullptr))
        {
            BlazeAssertMsg(false,  "suggestedNewHost cannot be nullptr");
            return JobCbHelper("migrateHostToPlayerCb", titleCb, ERR_SYSTEM, this);
        }
        if (EA_UNLIKELY(suggestedNewHost->getId() == mTopologyHostId)) 
        {
            BLAZE_SDK_DEBUGF("Game::migrateHostToPlayer(): Cannot migrate to current host.");
            return JobCbHelper("migrateHostToPlayerCb", titleCb, GAMEMANAGER_ERR_INVALID_NEWHOST, this);
        }
        if (EA_UNLIKELY(!isHostMigrationEnabled()))
        {            
            BLAZE_SDK_DEBUGF("[GAME] host migration not supported as it is turned off for this game.");
            return JobCbHelper("migrateHostToPlayerCb", titleCb, GAMEMANAGER_ERR_MIGRATION_NOT_SUPPORTED, this);
        }
        if (EA_UNLIKELY(!containsPlayer(suggestedNewHost))) 
        {
            BLAZE_SDK_DEBUGF("[GAME] Game::migrateHostToPlayer(): New host suggested is not a member of the game.");
            return JobCbHelper("migrateHostToPlayerCb", titleCb, GAMEMANAGER_ERR_INVALID_NEWHOST, this);
        }

        // Send host migration request to server with local callback
        // and cache titleCb for invocation later.
        MigrateHostRequest migrateHostRequest;
        migrateHostRequest.setGameId(mGameId);
        migrateHostRequest.setNewHostPlayer(suggestedNewHost->getId());

        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();
        JobId jobId = gameManagerComponent->migrateGame(migrateHostRequest, MakeFunctor(this, &Game::migrateHostCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal RPC callback fro migrateGame RPC (migrateHostToPlayer)
    ***************************************************************************************************/
    void Game::migrateHostCb(BlazeError error, JobId id, MigrateHostJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief init the game's platform host's slot
    ***************************************************************************************************/
    void Game::initPlatformHostSlotId(const SlotId platformHostSlotId, const PlayerId platformHostId)
    {
        BLAZE_SDK_DEBUGF("[GAME] Platform host id(%" PRId64 ") initialized with SlotId(%d). (Prior platform host id(%" PRId64 "))\n", platformHostId, platformHostSlotId, mPlatformHostId);
        mPlatformHostSlotId = platformHostSlotId;
        mPlatformHostId = platformHostId;
        if ((mPlatformHostPlayer = getPlayerById(platformHostId)) != nullptr)
        {
            mPlatformHostConnectionSlotId = mPlatformHostPlayer->getConnectionSlotId();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief handle the start of host migration (topology, platform, or both)
    ***************************************************************************************************/
    void Game::onNotifyHostMigrationStart(const PlayerId newHostPlayerId, const SlotId newHostSlotId, const SlotId newHostConnectionSlotId, HostMigrationType hostMigrationType, uint32_t userIndex)
    {
        Player *newHostPlayer = getPlayerById(newHostPlayerId);
        BLAZE_SDK_DEBUGF("[GAME] Host migration started, new %s host(%" PRId64 ":%s) game(%" PRIu64 ") migrationType(%d)\n", 
            (newHostPlayer != nullptr) ? (newHostPlayer->isLocalPlayer()?"Local":"External") : "nullptr", 
            newHostPlayerId, (newHostPlayer != nullptr) ? newHostPlayer->getName() : "nullptr", mGameId, hostMigrationType);

        mIsMigrating = true;
        mHostMigrationType = hostMigrationType;

        mIsMigratingPlatformHost = (hostMigrationType == PLATFORM_HOST_MIGRATION) || (hostMigrationType == TOPOLOGY_PLATFORM_HOST_MIGRATION);
        if (mIsMigratingPlatformHost)
        {
            mPlatformHostId = newHostPlayerId;
            mPlatformHostPlayer = newHostPlayer;
            mPlatformHostSlotId = newHostSlotId;
            mPlatformHostConnectionSlotId = newHostConnectionSlotId;
        }

        mIsMigratingTopologyHost = ( (hostMigrationType == TOPOLOGY_HOST_MIGRATION) || (hostMigrationType == TOPOLOGY_PLATFORM_HOST_MIGRATION) );
        if (mIsMigratingTopologyHost)
        {
            mTopologyHostId = newHostPlayerId;

            if (newHostPlayer != nullptr)
            {    
                mTopologyHostMeshEndpoint = newHostPlayer->getPlayerEndpoint();
            }
            else
            {
                mTopologyHostMeshEndpoint = nullptr;
            }
        }

        mDispatcher.dispatch("onHostMigrationStarted", &GameListener::onHostMigrationStarted, this);

        if ( (getLocalPlayer(userIndex) != nullptr) && !(getLocalPlayer(userIndex)->isActive()) )
        {
            // Non active players exit early, since they have no network yet.
            return;
        }

        if (mIsMigratingPlatformHost)
        {
            mGameManagerApi.getNetworkAdapter()->migratePlatformHost(this);
        }


        if (!hasFullCCSConnectivity())
        {
            mDelayingHostMigrationForCCS = true;

            BLAZE_SDK_DEBUGF("[GMGR] Game(%" PRIu64 ") delaying migrateTopologyHost call to network layer until all connectable end points have hosted connectivity in a hostedOnly or crossplatform game.\n", getId());
            return;
        }
        mDelayingHostMigrationForCCS = false;

        if (mIsMigratingTopologyHost)
        {
            mGameManagerApi.getNetworkAdapter()->migrateTopologyHost(this);
        }
    }
    
    /*! ************************************************************************************************/
    /*! \brief handle an async migration finished notification
    ***************************************************************************************************/
    void Game::onNotifyHostMigrationFinish()
    {
        mIsMigrating=false;
        BLAZE_SDK_DEBUGF("[GAME] Host migration finished for game (%" PRIu64 ")\n", getId());
        mDispatcher.dispatch("onHostMigrationEnded", &GameListener::onHostMigrationEnded, this);
        
        // Need to set this after dispatching the notification so clients
        // can distinguish between the type of host migration.
        mIsMigratingPlatformHost=false;
        mIsMigratingTopologyHost=false;
    }

    /*! ************************************************************************************************/
    /*! \brief add players to this game.  Initializes applicable members (localPlayer, hostPlayer).
        \param[in] gameRoster the list of replicatdPlayerData object
        \param[in] performQosOnActivePlayers - if true, a QoS check will be done when connecting to the active players in the game
    ***************************************************************************************************/
    void Game::initRoster(const ReplicatedGamePlayerList& gameRoster, bool performQosOnActivePlayers)
    {
        BlazeAssert(mRosterPlayers.empty());
        BLAZE_SDK_DEBUGF("[GAME] initializing roster for game(%" PRIu64 ").\n", mGameId);

        uint16_t totalCapacity = 0;

        for (SlotCapacitiesVector::size_type type = SLOT_PUBLIC_PARTICIPANT; type != MAX_SLOT_TYPE; ++type)
        {
            // total the current capacities for reserving
            totalCapacity = static_cast<uint16_t>(totalCapacity + mPlayerCapacity[type]);
            // initialize the used slot counts to zero
            mPlayerSlotCounts[type] = 0;
        }

        mRosterPlayers.reserve( totalCapacity );
       
        ReplicatedGamePlayerList::const_iterator rosterIter = gameRoster.begin();
        ReplicatedGamePlayerList::const_iterator rosterEnd = gameRoster.end();
        for ( ; rosterIter != rosterEnd; ++rosterIter )
        {
            const ReplicatedGamePlayer &replicatedPlayerData = **rosterIter;
            Player* player = addPlayer(replicatedPlayerData);
            if(player->isActive())
            {
                player->getPlayerEndpoint()->setPerformQosDuringConnection(performQosOnActivePlayers);
            }
        }

        BLAZE_SDK_DEBUGF("[GAME] Initialized roster for game(%" PRIu64 ") with total of %d players added.\n", mGameId, (int32_t)mRosterPlayers.size());
    }

    /*! ************************************************************************************************/
    /*! \brief initializes the games queue by adding the replicated players (if any) to the games queue.
    
        \param[in] gameQueue - the replicated queue
    ***************************************************************************************************/
    void Game::initQueue(const ReplicatedGamePlayerList& gameQueue)
    {
        BlazeAssert(mQueuedPlayers.empty());
        BLAZE_SDK_DEBUGF("[GAME] initializing queue for game(%" PRIu64 ").\n", mGameId);

        ReplicatedGamePlayerList::const_iterator queueIter = gameQueue.begin();
        ReplicatedGamePlayerList::const_iterator queueEnd = gameQueue.end();
        for ( ; queueIter != queueEnd; ++queueIter )
        {
            const ReplicatedGamePlayer &replicatedPlayerData = **queueIter;
            addPlayerToQueue(replicatedPlayerData);
        }

        BLAZE_SDK_DEBUGF("[GAME] Initialized queue for game(%" PRIu64 ") with total of %d players added.\n", mGameId, (int32_t)mQueuedPlayers.size());
    }


    bool Game::promotePlayerFromQueue(const ReplicatedGamePlayer &playerData)
    {
        if (!containsPlayer(playerData.getPlayerId()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Unable to promote queued Player(%" PRId64 "), not already a roster member in game(%" PRIu64 ").\n",
                playerData.getPlayerId(), mGameId);
            BlazeAssertMsg(false,  "Error: updating player is not already a game member!  Use addPlayer instead.");
            return false;
        }

        // Player is being promoted from the queue.
        Player *queuedPlayer = getPlayerInternal(playerData.getPlayerId());
        if ( (queuedPlayer != nullptr) && (playerData.getPlayerState() != QUEUED) )
        {
            // Remove the player from the queue
            mQueuedPlayers.erase(queuedPlayer->getSlotId());

            GameEndpoint *queuedEndpoint = queuedPlayer->getPlayerEndpoint();

            // Update the player's state and slot information.
            queuedPlayer->mPlayerState = playerData.getPlayerState();
            queuedPlayer->mSlotId = playerData.getSlotId();
            queuedPlayer->mSlotType = playerData.getSlotType();
            queuedPlayer->mTeamIndex = playerData.getTeamIndex();
            queuedPlayer->mJoinedGameTimestamp = playerData.getJoinedGameTimestamp();
            queuedPlayer->mRoleName = playerData.getRoleName();
            queuedPlayer->mDirtySockUserIndex = playerData.getDirtySockUserIndex();    // MLU Players may claim an external reservation and join.


            // Only connection slot ID can change as the result of a queue promotion
            // Connection group ID can change when claiming a reservation, however, not every member of the game will get queued player updates
            // This means we need to potentially update the queued player's endpoint here.
            // We only update the connection slot id if the user isn't reserved
            queuedEndpoint->mConnectionSlotId = playerData.getConnectionSlotId();
            updatePlayerEndpoint(*queuedPlayer, playerData, (queuedPlayer->getPlayerState() != RESERVED));
            


            BLAZE_SDK_DEBUGF("[GAME] Queued player(%" PRId64 ":%s) updated for game(%" PRIu64 ":%s) in slot(%u), state(%s).\n", 
                playerData.getPlayerId(), queuedPlayer->getName(), mGameId, mName.c_str(), 
                playerData.getSlotId(), PlayerStateToString(playerData.getPlayerState()));


            // we didn't count this player when he joined the queue, track him in the slot counts now, to balance the decrement when leaving.
            ++mPlayerSlotCounts[playerData.getSlotType()];
            incrementLocalTeamSize(playerData.getTeamIndex(), playerData.getRoleName());

            mRosterPlayers[playerData.getSlotId()] = queuedPlayer;
            if (queuedPlayer->getPlayerState() != RESERVED)
            {
                mActivePlayers[playerData.getSlotId()] = queuedPlayer;
                // if this user was part of a MLU group, this won't cause any issues, as the map was already pointed at this endpoint.

                activateEndpoint(queuedPlayer->getPlayerEndpoint());
            }

            mDispatcher.dispatch("onQueueChanged", &GameListener::onQueueChanged, this);
            if (!queuedPlayer->isLocalPlayer())
            {
                mDispatcher.dispatch("onPlayerJoinedFromQueue", &GameListener::onPlayerJoinedFromQueue, queuedPlayer);
            }

            

            return true;
        }

        return false;
    }

    bool Game::demotePlayerToQueue(const ReplicatedGamePlayer &playerData)
    {
        if (!containsPlayer(playerData.getPlayerId()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Unable to demote Player(%" PRId64 "), to the queue, not already a roster member in game(%" PRIu64 ").\n",
                playerData.getPlayerId(), mGameId);
            BlazeAssertMsg(false,  "Error: updating player is not already a game member!  Use addPlayer instead.");
            return false;
        }

        // Player is being demoted to queue.
        Player *reservedPlayer = getPlayerInternal(playerData.getPlayerId());
        if ( (reservedPlayer != nullptr) && (playerData.getPlayerState() == QUEUED || playerData.getPlayerState() == RESERVED) )
        {
            // Remove the player from the queue
            
            mRosterPlayers.erase(reservedPlayer->getSlotId());

            // we counted this player when he was reserved, decrement the count now:
            --mPlayerSlotCounts[reservedPlayer->getSlotType()];
            decrementLocalTeamSize(reservedPlayer->getTeamIndex(), reservedPlayer->getRoleName());


            GameEndpoint *queuedEndpoint = reservedPlayer->getPlayerEndpoint();

            // Update the player's state and slot information.
            PlayerState initialPlayerState = reservedPlayer->mPlayerState;
            reservedPlayer->mPlayerState = playerData.getPlayerState();
            reservedPlayer->mSlotId = playerData.getSlotId();
            reservedPlayer->mSlotType = playerData.getSlotType();
            reservedPlayer->mTeamIndex = playerData.getTeamIndex();
            reservedPlayer->mJoinedGameTimestamp = playerData.getJoinedGameTimestamp();
            reservedPlayer->mRoleName = playerData.getRoleName();
            reservedPlayer->mDirtySockUserIndex = playerData.getDirtySockUserIndex();    // MLU Players may claim an external reservation and join.


            // Only connection slot ID can change as the result of a queue demotion
            // Connection group ID can change when claiming a reservation, however, not every member of the game will get queued player updates
            // This means we need to potentially update the newly queued player's endpoint here.
            // We only update the connection slot id if the user isn't already reserved
            queuedEndpoint->mConnectionSlotId = playerData.getConnectionSlotId();
            updatePlayerEndpoint(*reservedPlayer, playerData, (initialPlayerState != RESERVED));
            


            BLAZE_SDK_DEBUGF("[GAME] Demoted player(%" PRId64 ":%s) updated for game(%" PRIu64 ":%s) in slot(%u), state(%s).\n", 
                playerData.getPlayerId(), reservedPlayer->getName(), mGameId, mName.c_str(), 
                playerData.getSlotId(), PlayerStateToString(playerData.getPlayerState()));


           
            mQueuedPlayers[playerData.getSlotId()] = reservedPlayer;


            mDispatcher.dispatch("onQueueChanged", &GameListener::onQueueChanged, this);
            if (!reservedPlayer->isLocalPlayer())
            {
                mDispatcher.dispatch("onPlayerDemotedToQueue", &GameListener::onPlayerDemotedToQueue, reservedPlayer);
            }

            

            return true;
        }

        return false;
    }

    bool Game::claimPlayerReservation(const ReplicatedGamePlayer &playerData)
    {
        if (!containsPlayer(playerData.getPlayerId()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Unable to claim reservation for Player(%" PRId64 "), not already a roster member in game(%" PRIu64 ").\n",
                playerData.getPlayerId(), mGameId);
            BlazeAssertMsg(false,  "Error: updating player is not already a game member!  Use addPlayer instead.");
            return false;
        }

        Player *reservedPlayer = getRosterPlayerById( playerData.getPlayerId());
        Player *queuedPlayer = getQueuedPlayerById(playerData.getPlayerId());
        if ( (reservedPlayer != nullptr) && reservedPlayer->isReserved() && (playerData.getPlayerState() != RESERVED) )
        {
            // existing player found; see if they're a reserved player claiming a reservation
            // Update the player's state and slot information.
            reservedPlayer->mPlayerState = playerData.getPlayerState();
            // Only update our roster list if the slot id has changed.
            if (reservedPlayer->mSlotId != playerData.getSlotId())
            {
                mRosterPlayers.erase(reservedPlayer->mSlotId);
                reservedPlayer->mSlotId = playerData.getSlotId();
                mRosterPlayers[reservedPlayer->mSlotId] = reservedPlayer;
            }

            // if team updated, update local team size info
            if (reservedPlayer->mTeamIndex != playerData.getTeamIndex())
            {
                incrementLocalTeamSize(playerData.getTeamIndex(), playerData.getRoleName());
                decrementLocalTeamSize(reservedPlayer->mTeamIndex, reservedPlayer->mRoleName);
            }

            updatePlayerEndpoint(*reservedPlayer, playerData, true);
            // we do this here, because the endpoint update will also happen for a still-queued player
            activateEndpoint(reservedPlayer->getPlayerEndpoint());

            reservedPlayer->mSlotType = playerData.getSlotType();
            reservedPlayer->mTeamIndex = playerData.getTeamIndex();
            reservedPlayer->mRoleName = playerData.getRoleName();
            reservedPlayer->mJoinedGameTimestamp = playerData.getJoinedGameTimestamp();
            reservedPlayer->mDirtySockUserIndex = playerData.getDirtySockUserIndex();    // MLU Players may claim an external reservation and join.

            // if here due to CG MM as co-creator, player's attribs may have been empty until now.
            // if here due to GM join-claim, player may have also updated them. Ensure up to date.
            playerData.getPlayerAttribs().copyInto(reservedPlayer->mPlayerAttributeMap);
             
            mActivePlayers[playerData.getSlotId()] = reservedPlayer;


            BLAZE_SDK_DEBUGF("[GAME] RESERVED Player(%" PRId64 ":%s) claiming reservation in game(%" PRIu64 ":%s) in slot(%u), state %s.\n", 
                playerData.getPlayerId(), reservedPlayer->getName(), mGameId, mName.c_str(), 
                playerData.getSlotId(), PlayerStateToString(playerData.getPlayerState()));

            // Special case: 
            // When a local player joins the game, we need to send the reservation claim dispatch as well. 
            // Normally, that is sent from the dispatchGameSetupCallback, but if this was a local user we don't go through that flow.
            bool foundOtherPlayerInGame = false;
            for (uint32_t i = 0; i < mGameManagerApi.getBlazeHub()->getNumUsers(); ++i)
            {
                Player* otherLocalPlayer = mLocalPlayers[i];
                if (otherLocalPlayer != reservedPlayer &&
                    otherLocalPlayer != nullptr &&
                    !otherLocalPlayer->isReserved() && !otherLocalPlayer->isQueued())
                {
                    foundOtherPlayerInGame = true;
                    break;
                }
            }


            if (!reservedPlayer->isLocalPlayer())
            {
                mDispatcher.dispatch("onPlayerJoinedFromReservation", &GameListener::onPlayerJoinedFromReservation, reservedPlayer);
            }
            else
            {
                mLocalEndpoint = reservedPlayer->getPlayerEndpoint();
                if (foundOtherPlayerInGame)
                {
                    mDispatcher.dispatch("onPlayerJoinedFromReservation", &GameListener::onPlayerJoinedFromReservation, reservedPlayer);
                }
            }

            return true;
        }
        else if (queuedPlayer != nullptr)
        {
            // if still queued, need to grab the new state
            queuedPlayer->mPlayerState = playerData.getPlayerState();
            // don't update the connection slot id for a queued player if they have an existing endpoint
            updatePlayerEndpoint(*queuedPlayer, playerData, false);
            if (queuedPlayer->isLocalPlayer())
            {
                mLocalEndpoint = queuedPlayer->getPlayerEndpoint();
            }
            BLAZE_SDK_DEBUGF("[GAME] RESERVED Player(%" PRId64 ":%s) claiming reservation in game(%" PRIu64 ":%s) in queue slot(%u), state %s.\n", 
                playerData.getPlayerId(), queuedPlayer->getName(), mGameId, mName.c_str(), 
                playerData.getSlotId(), PlayerStateToString(playerData.getPlayerState()));
            return true;
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief add a player to the game's roster
    ***************************************************************************************************/
    Player* Game::addPlayer(const ReplicatedGamePlayer &playerData)
    {
        if (containsPlayer(playerData.getPlayerId()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Player(%" PRId64 ") already a member in game(%" PRIu64 "), no-op addPlayer.",
                playerData.getPlayerId(), mGameId);
            return getPlayerById(playerData.getPlayerId());
        }

        if (mRosterPlayers.find(playerData.getSlotId()) != mRosterPlayers.end())
        {
            BLAZE_SDK_DEBUGF("[GAME] Player(%" PRId64 ") is overriding the slot (%" PRIu8 ") of an existing player(%" PRId64 ") in game(%" PRIu64 ") no-op addPlayer.  This indicates a BlazeSDK/server desync, and is unrecoverable.\n",
                playerData.getPlayerId(), playerData.getSlotId(), mRosterPlayers[playerData.getSlotId()]->getId(), mGameId);
            return mRosterPlayers[playerData.getSlotId()];
        }

        // player not found in the game - alloc a local player obj
        // alloc & add player to roster if a new player
        if (mGameManagerApi.getApiParams().mMaxGameManagerGames > 0)
        {
            BlazeAssert((mRosterPlayers.size()+mQueuedPlayers.size()) < static_cast<uint32_t>(GameBase::getMaxPlayerCapacity() + mQueueCapacity)  &&
                "Object count exceeded memory cap specified by the game's parameters");
        }

        GameEndpoint *endpoint = (GameEndpoint*)getMeshEndpointByConnectionGroupId(playerData.getConnectionGroupId());
        if (endpoint == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] Allocating endpoint for player (%" PRIu64 ") in game (%" PRIu64 "), for connection group (%" PRIu64 ").\n", playerData.getPlayerId(), 
                mGameId, playerData.getConnectionGroupId());
            endpoint =  new (mGameEndpointMemoryPool.alloc()) GameEndpoint(this, &playerData, mMemGroup);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Assigning existing endpoint for player (%" PRIu64 ") in game (%" PRIu64 "), for connection group (%" PRIu64 ").\n", playerData.getPlayerId(),
                mGameId, playerData.getConnectionGroupId());
        }

        Player *player = new (mPlayerMemoryPool.alloc()) Player(this, endpoint, &playerData, mMemGroup);

        for (uint32_t ui=0; ui < mGameManagerApi.getBlazeHub()->getNumUsers(); ++ui)
        {
            if (mGameManagerApi.getUserManager()->getLocalUser(ui) != nullptr)
            {
                if (player->getId() == mGameManagerApi.getUserManager()->getLocalUser(ui)->getId())
                {
                    mLocalPlayers[ui] = player;
                    mLocalPlayerMap[player->getId()] = player;
                    mLocalEndpoint = endpoint;
                }
            }
        }

        uint16_t oldPublic, oldPrivate, newPublic, newPrivate;
        getFirstPartyCapacity(oldPublic, oldPrivate);

        // Keep a map of everyone
        mPlayerRosterMap[player->getId()] = player;
        
        mGameEndpointMap[playerData.getConnectionGroupId()] = endpoint;

        getFirstPartyCapacity(newPublic, newPrivate);
        if (newPublic != oldPublic || newPrivate != oldPrivate)
        {
            mGameManagerApi.getNetworkAdapter()->updateCapacity(this);
        }

        // add player to roster
        mRosterPlayers[player->getSlotId()] = player;
        
        // Make sure host isn't allowed to be a non active player.
        if (player->isActive())
        {
            // Note that active players are stored by sequential index.
            mActivePlayers[player->getSlotId()] = player;

            if (player->getId() == mPlatformHostId)
            {
                mPlatformHostPlayer = player;
            }

            // active players should be added to the endpoints vector map
            // don't have to worry about a reassignment, as we share the endpoint between users with the same connection group ID.
            activateEndpoint(player->getPlayerEndpoint());
        }

        // we're keeping a count of the number slots used for resize purposes
        ++mPlayerSlotCounts[playerData.getSlotType()];

        // keeping a count of team members
        TeamIndex teamIndex = player->getTeamIndex();
        incrementLocalTeamSize(teamIndex, player->getRoleName());

        BLAZE_SDK_DEBUGF("[GAME] Adding %s %s(%" PRId64 ":%" PRIu64 ":%s) in state(%s) to game(%" PRIu64 ") roster in slot(%u:%s).\n",
            (player->isLocalPlayer()) ? "local" : "external", (getHostId() == player->getId()) ? "host" : "player",
            player->getId(), player->getUser()->getExternalId(), player->getName(), PlayerStateToString(player->getPlayerState()), mGameId,
            player->getSlotId(), SlotTypeToString(player->getSlotType()));

        // send notification, only for players not already in the roster.
        mDispatcher.dispatch("onPlayerJoining", &GameListener::onPlayerJoining, player);
        
        return player;
    }

    Player* Game::addPlayerToQueue(const ReplicatedGamePlayer &playerData)
    {
        if (containsPlayer(playerData.getPlayerId()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Player(%" PRId64 ") already a member in game(%" PRIu64 ") no-op addPlayerToQueue.\n",
                playerData.getPlayerId(), mGameId);
            return getPlayerById(playerData.getPlayerId());
        }
        
        if (mQueuedPlayers.find(playerData.getSlotId()) != mQueuedPlayers.end())
        {
            BLAZE_SDK_DEBUGF("[GAME] Player(%" PRId64 ") is overriding the slot (%" PRIu8 ") of an existing player(%" PRId64 ") in game(%" PRIu64 ") no-op addPlayerToQueue.  This indicates a BlazeSDK/server desync, and is unrecoverable.\n",
                playerData.getPlayerId(), playerData.getSlotId(), mQueuedPlayers[playerData.getSlotId()]->getId(), mGameId);
            return mQueuedPlayers[playerData.getSlotId()];
        }

        // player not found in the game - alloc a local player obj
        // alloc & add player to roster if a new player
        if (mGameManagerApi.getApiParams().mMaxGameManagerGames > 0)
        {
            BlazeAssert((mRosterPlayers.size()+mQueuedPlayers.size()) < static_cast<uint32_t>(GameBase::getMaxPlayerCapacity() + mQueueCapacity)  &&
                "Object count exceeded memory cap specified by the game's parameters");
        }

        GameEndpoint *endpoint = (GameEndpoint*)(getMeshEndpointByConnectionGroupId(playerData.getConnectionGroupId()));
        if (endpoint == nullptr)
        {
            // this isn't a pre-existing endpoint, allocate a new one
            BLAZE_SDK_DEBUGF("[GAME] Allocating endpoint for player (%" PRIu64 ") in game (%" PRIu64 "), for connection group (%" PRIu64 ").\n", playerData.getPlayerId(),  
                mGameId, playerData.getConnectionGroupId());
            endpoint =  new (mGameEndpointMemoryPool.alloc()) GameEndpoint(this, &playerData, mMemGroup);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Assigning existing endpoint for player (%" PRIu64 ") in game (%" PRIu64 "), for connection group (%" PRIu64 ").\n", playerData.getPlayerId(),
                mGameId, playerData.getConnectionGroupId());
        }

        Player *player = new (mPlayerMemoryPool.alloc()) Player( this, endpoint, &playerData, mMemGroup);

        for (uint32_t ui=0; ui < mGameManagerApi.getBlazeHub()->getNumUsers(); ++ui)
        {
            if (mGameManagerApi.getUserManager()->getLocalUser(ui) != nullptr)
            {
                if (player->getId() == mGameManagerApi.getUserManager()->getLocalUser(ui)->getId())
                {
                    mLocalPlayers[ui] = player;
                    mLocalPlayerMap[player->getId()] = player;
                    mLocalEndpoint = endpoint;
                }
            }
        }

        // Keep a map of everyone
        mPlayerRosterMap[player->getId()] = player;

        mGameEndpointMap[playerData.getConnectionGroupId()] = endpoint;

        BLAZE_SDK_DEBUGF("[GAME] Adding %s player(%" PRId64 ":%s) to game(%" PRIu64 ") queue at index(%u).\n",
            (player->isLocalPlayer()) ? "local" : "external",                 
            player->getId(), player->getName(), mGameId, player->getSlotId());

        // GM_TODO: Seems like this listener could be consolidated with onQueueChanged.
        mQueuedPlayers[player->getSlotId()] = player;
        
        mDispatcher.dispatch("onPlayerJoiningQueue", &GameListener::onPlayerJoiningQueue, player);
        return player;
    }

    /*! ************************************************************************************************/
    /*! \brief tell the blazeServer that the game has finished initializing itself. (state transition)
    ***************************************************************************************************/
    JobId Game::initGameComplete(const ChangeGameStateJobCb &titleCb)
    {
        if (mState == INITIALIZING || mState == CONNECTION_VERIFICATION)
        {
            GameState nextState = isGameTypeGroup() ? GAME_GROUP_INITIALIZED : PRE_GAME;
            return advanceGameState(nextState, titleCb);
        }
        else if (mState == GAME_GROUP_INITIALIZED)
        {
            // do nothing and return ok if the game group has already been initialized
            return JobCbHelper("initGameCompleteCb", titleCb, ERR_OK, this);
        }
        else
        {
            // can only call init complete while game is initializing
            return JobCbHelper("initGameCompleteCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief tell the blazeServer that the game has started (state transition)
    ***************************************************************************************************/
    JobId Game::startGame(const ChangeGameStateJobCb &titleCb)
    {
        if (mState == PRE_GAME)
        {
            JobId jobId = advanceGameState(IN_GAME, titleCb);

            if (!isJoinInProgressSupported())
            {
                mGameManagerApi.getNetworkAdapter()->lockSession(this, true);
            }
            return jobId;
        }
        else
        {
            // can only call start game while game is in pre game
            return JobCbHelper("startGameCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief tell the blazeServer that the game has ended (state transition)
    ***************************************************************************************************/
    JobId Game::endGame(const ChangeGameStateJobCb &titleCb)
    {
        if (mState == IN_GAME)
        {
            JobId jobId = advanceGameState(POST_GAME, titleCb);
            mGameManagerApi.getNetworkAdapter()->lockSession(this, false);
            return jobId;
        }
        else
        {
            // can only call endGame while in game
            return JobCbHelper("endGameCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief tell the blazeServer that the game is being replayed (state transition)
    ***************************************************************************************************/
    JobId Game::replayGame(const ChangeGameStateJobCb &titleCb)
    {
        // Only can replay if game is in POST_STATE 
        if (mState == POST_GAME)
        {
            ReplayGameRequest replayGameRequest;
            replayGameRequest.setGameId(mGameId);
            JobId jobId = getAdminGameManagerComponent()->replayGame(replayGameRequest, MakeFunctor(this, &Game::changeGameStateCb), titleCb);
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }
        else
        {
            // can only call replayGame while in post game
            return JobCbHelper("replayGameCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return this dedicated server game to the available server pool
    ***************************************************************************************************/
    JobId Game::returnDedicatedServerToPool(const ChangeGameStateJobCb &titleCb)
    {
        // can only call return to pool for dedicated server
        if (!isDedicatedServerTopology())
        {
            return JobCbHelper("returnDedicatedServerToPoolCb", titleCb, GAMEMANAGER_ERR_DEDICATED_SERVER_ONLY_ACTION, this);
        }

        // can only call return to pool from pre/init game or post game
        if (EA_UNLIKELY((mState != PRE_GAME) && (mState != INITIALIZING) && (mState != POST_GAME)))
        {
            return JobCbHelper("returnDedicatedServerToPoolCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }

        ReturnDedicatedServerToPoolRequest returnDedicatedServerToPoolRequest;
        returnDedicatedServerToPoolRequest.setGameId(mGameId);

        JobId jobId = mGameManagerApi.getGameManagerComponent()->returnDedicatedServerToPool(returnDedicatedServerToPoolRequest,
            MakeFunctor(this, &Game::changeGameStateCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief Attempt to transition the game's state from RESETABLE to INITIALIZING.  Only valid for dedicated
            servers.
    ***************************************************************************************************/
    JobId Game::resetDedicatedServer(const ChangeGameStateJobCb &titleCb)
    {
        if (!isDedicatedServerTopology())
        {
            // can only call for dedicated server
            return JobCbHelper("resetDedicatedServerCb", titleCb, GAMEMANAGER_ERR_DEDICATED_SERVER_ONLY_ACTION, this);
        }

        if (mState != RESETABLE)
        {
            // can only call if game is in RESETABLE 
            return JobCbHelper("resetDedicatedServerCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }

        mIsDirectlyReseting = true;
        return advanceGameState(INITIALIZING, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief helper to advance a game's gameState.
    ***************************************************************************************************/
    JobId Game::advanceGameState(GameState newGameState, const ChangeGameStateJobCb &titleCb)
    {
        if (EA_UNLIKELY(newGameState == mState))
        {
            // no-op; state is already set
            BLAZE_SDK_DEBUGF("[GAME].advanceGameState: Already in game state. return GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION\n");
            return JobCbHelper("advanceGameStateCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION, this);
        }

        AdvanceGameStateRequest advanceGameStateRequest;
        advanceGameStateRequest.setGameId(mGameId);
        advanceGameStateRequest.setNewGameState(newGameState);

        JobId jobId = getAdminGameManagerComponent()->advanceGameState( advanceGameStateRequest, 
            MakeFunctor(this, &Game::changeGameStateCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief handle RPC response from advanceGameState
    ***************************************************************************************************/
    void Game::changeGameStateCb(BlazeError error, JobId id, ChangeGameStateJobCb titleCb)
    {
        titleCb(error, this, id);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief handle async notification that the game's state has changed.
    ***************************************************************************************************/
    void Game::onNotifyGameStateChanged(GameState newGameState, uint32_t userIndex)
    {
        if (mState != newGameState)
        {
            BLAZE_SDK_DEBUGF("[GAME] Advancing game state %s => %s for game(%" PRIu64 ")\n", 
                GameStateToString(mState), GameStateToString(newGameState), mGameId );

            GameState previousState = mState;

            // update local state & dispatch to any listeners
            mState = newGameState;

            // similarly if previous state was unresponsive, we're simply restoring the game state.
            // Dispatch onBackFromUnresponsive. Nothing else to do here.
            if (previousState == UNRESPONSIVE)
            {
                mDispatcher.dispatch("onBackFromUnresponsive", &GameListener::onBackFromUnresponsive, this);
                return;
            }

            switch( mState )
            {
            case INITIALIZING:
                {
                    if (mIsDirectlyReseting)
                    {
                        // Note: when a game resets itself explicitly, we simply update the game's state
                        // and onNotifyGameReset is only called with a reset
                        // through game manager api not explicitly on the game itself.
                        mDispatcher.dispatch("onGameReset", &GameListener::onGameReset, this);
                        mIsDirectlyReseting = false;
                    }
                    break;
                }
            case GAME_GROUP_INITIALIZED:
                {
                    mDispatcher.dispatch("onGameGroupInitialized", &GameListener::onGameGroupInitialized, this);
                    break;
                }
            case PRE_GAME:
                {
                    mDispatcher.dispatch("onPreGame", &GameListener::onPreGame, this);
                    break;
                }
            case IN_GAME:
                {
                    if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
                    {
                        PlayerId localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();
                        Player *player = getPlayerById(localPlayerId);
                        if ((player != nullptr && player->isActive()) || (isDedicatedServerHost()))
                        {
                            mGameManagerApi.getNetworkAdapter()->startGame(this);
                        }
                    }
                    mDispatcher.dispatch("onGameStarted", &GameListener::onGameStarted, this);
                    break;
                }
            case POST_GAME:
                {
                    if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
                    {
                        PlayerId localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();
                        Player *player = getPlayerById(localPlayerId);
                        if ((player != nullptr && player->isActive()) || (isDedicatedServerHost()))
                        {
                            mGameManagerApi.getNetworkAdapter()->endGame(this);
                        }
                    }
                    mDispatcher.dispatch("onGameEnded", &GameListener::onGameEnded, this);
                    break;
                }
            case RESETABLE:
                {
                    mDispatcher.dispatch("onReturnDServerToPool", &GameListener::onReturnDServerToPool, this);
                    break;
                }
            case UNRESPONSIVE:
                {
                    mDispatcher.dispatch("onUnresponsive", &GameListener::onUnresponsive, this, previousState);
                    break;
                }
            case INACTIVE_VIRTUAL:
                {
                    // Cleanup the network when game state changes to INACTIVE_VIRTUAL due to host replacement
                    BLAZE_SDK_DEBUGF("[GAME] Destroying network mesh due to game migrating its dedicated server host.\n");
                    
                    cleanUpGameEndpoint(mDedicatedHostEndpoint);
                    mGameManagerApi.getNetworkAdapter()->destroyNetworkMesh(this);
                    
                    mIsNetworkCreated = false;
                    mIsLocalPlayerPreInitNetwork = false;
                    mIsLocalPlayerInitNetwork = false;
                    break;
                }

            default:
                //Do nothing
                break;
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: Ignoring GameStateChanged notification; game(%" PRIu64 ") is already in the updated state(%s)\n", mGameId, GameStateToString(newGameState));
        }
    }

    /*! ****************************************************************************/
    /*! \brief Async notification that a player has been removed from the game roster and
            is about to be deleted.
    ********************************************************************************/
    void Game::onPlayerRemoved(PlayerId leavingPlayerId, const PlayerRemovedReason playerRemovedReason, const PlayerRemovedTitleContext titleContext, uint32_t userIndex)
    {
        // find/verify the leaving player
        Player *leavingPlayer = getPlayerInternal(leavingPlayerId);
        if (leavingPlayer == nullptr)
        {
            // player must've already been removed
            BLAZE_SDK_DEBUGF("[GAME] Warning: dropping async NotifyPlayerRemoved sent to user index(%u) for unknown player(%" PRId64 ")\n", userIndex, leavingPlayerId);
            return;
        }

        BLAZE_SDK_DEBUGF("[GAME] %s Player(%" PRId64 ":%s) removed from game(%" PRIu64 ") due to %s. Notified local Player(%" PRId64 ") at index(%u).\n", leavingPlayer->isLocalPlayer()?"Local":"External", 
            leavingPlayerId, leavingPlayer->getName(), mGameId, PlayerRemovedReasonToString(playerRemovedReason), ((getLocalPlayer(userIndex) != nullptr) ? getLocalPlayer(userIndex)->getId() : INVALID_BLAZE_ID), userIndex);

        // cache off if the local player is the host (since removePlayer nukes the hostId on the game if the leaving player is host)
        const bool isLocalPlayerHost = isTopologyHost();

        // remove the player from the local game's roster, but leave the network connection and player obj intact (until after dispatch)
        bool wasQueued = removePlayerFromRoster(leavingPlayer);

        // need the player pointers.
        bool wasLocalPlayer = false;
        for (uint32_t localIndex = 0; localIndex < mGameManagerApi.getBlazeHub()->getNumUsers(); ++localIndex)
        {
            Player *curPlayer = mLocalPlayers[localIndex];
            if ((curPlayer != nullptr) && (curPlayer->getId() == leavingPlayerId))
            {
#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
                // before erasing local player, update primary session
                mGameManagerApi.updateGamePresenceForLocalUser(localIndex, *this, UPDATE_PRESENCE_REASON_GAME_LEAVE, nullptr, playerRemovedReason);
#endif
                mLocalPlayers[localIndex] = nullptr;
                mLocalPlayerMap.erase(leavingPlayerId);
                wasLocalPlayer = true;
                break;
            }
        }

        uint32_t connStatFlags = 0;

        if (getNetworkTopology() == CLIENT_SERVER_DEDICATED)
        {
            if (wasLocalPlayer && getDedicatedServerHostMeshEndpoint() != nullptr)
            {
                mGameManagerApi.getNetworkAdapter()->getConnectionStatusFlagsForMeshEndpoint(getDedicatedServerHostMeshEndpoint(), connStatFlags);
            }
            else if (!wasLocalPlayer && isDedicatedServerHost())
            {
                mGameManagerApi.getNetworkAdapter()->getConnectionStatusFlagsForMeshEndpoint(leavingPlayer->getMeshEndpoint(), connStatFlags);
            }
        }

        this->setLeavingPlayerConnFlags(connStatFlags);

        mDispatcher.dispatch("onPlayerRemoved", &GameListener::onPlayerRemoved, this, const_cast<const Player*>(leavingPlayer), playerRemovedReason, titleContext);

        if (wasQueued)
        {
            mDispatcher.dispatch("onQueueChanged", &GameListener::onQueueChanged, this);
        }

        // disconnect the leavingPlayer from the game's network mesh

        if (wasLocalPlayer)
        {
            if (mLocalPlayerMap.size() == 0)
            {
                // local player leaving, free the player
                cleanUpPlayer(leavingPlayer);
                // No more local players in the game, tear down the game.
                if (playerRemovedReason == HOST_INJECTION_FAILED)
                {
                    mGameManagerApi.destroyLocalGame(this, HOST_EJECTION, isLocalPlayerHost, !wasQueued);            
                }
                else
                {   
                    mGameManagerApi.destroyLocalGame(this, LOCAL_PLAYER_LEAVING, isLocalPlayerHost, !wasQueued);            
                }
                // this pointer(game*) is invalid after returning from destroyLocalGame.  Must return immediately.
                return;
            }
        }

        // If the game still persists (MLU local leave, or remote player leave) just disconnect from the player:
        cleanupPlayerNetConnection(leavingPlayer, false, getLocalPlayer(userIndex), isLocalPlayerHost, playerRemovedReason);

        // delete the leaving player
        cleanUpPlayer(leavingPlayer);

        // Since the player's connection is no longer required, see if we can trigger any deferred CCS actions:
        handleDeferedCCSActions(userIndex);
    }

    void Game::cleanupPlayerNetConnection(const Player* leavingPlayer, bool removeFromAllPlayers, const Player* localPlayer, bool isLocalPlayerHost, const PlayerRemovedReason playerRemovedReason)
    {
        if (leavingPlayer->isLocal())
        {
            // Still have at least 1 local player in the game - remove player from endpoint and tear down network
            // if they are the last active player in the game (the game still contains other non-active local players)
            if (mNetworkMeshHelper.removeMemberFromMeshEndpoint(leavingPlayer))
            {
                mGameManagerApi.getNetworkAdapter()->destroyNetworkMesh(this);
                if (playerRemovedReason != GROUP_LEFT)
                {
                    //Reset values.
                    for (PlayerRosterList::const_iterator it = mActivePlayers.begin(), itEnd = mActivePlayers.end(); it != itEnd; ++it)
                    {
                        if (it->second->getConnectionGroupId() != leavingPlayer->getConnectionGroupId())
                            mNetworkMeshHelper.removeMemberFromMeshEndpoint(it->second);
                    }
                    mIsNetworkCreated = false;
                    mIsLocalPlayerPreInitNetwork = false;
                    mIsLocalPlayerInitNetwork = false;
                }
            }
        }
        else
        {
            // if we don't want to call connect to endpoint without an initialized network, we shouldn't call disconnect from endpoint either
            DeferredJoiningPlayerMap::iterator deferredPlayerIter = mDeferredJoiningPlayerMap.find(leavingPlayer->getId());
            if ( deferredPlayerIter != mDeferredJoiningPlayerMap.end() )
            {
                mDeferredJoiningPlayerMap.erase(deferredPlayerIter);
            }

            // GM_AUDIT: adapter contract implies they can take multiple ticks (due to callback), but we immediately destroy the player pointer...

            // another player is leaving, disconnect from them, if they were actively in the game and we are too
            if (removeFromAllPlayers)
            {
                for (uint32_t i = 0; i < mGameManagerApi.getBlazeHub()->getNumUsers(); ++i)
                {
                    // We don't do the isActive check here, because this is for the disconnect reservation logic
                    Player* curLocalPlayer = getLocalPlayer(i);
                    if (leavingPlayer != curLocalPlayer && (isLocalPlayerHost || (curLocalPlayer != nullptr && curLocalPlayer->isActive())))
                    {
                        mNetworkMeshHelper.disconnectFromUser(curLocalPlayer, leavingPlayer);
                    }
                }
            }
            else
            {
                // another player is leaving, disconnect from them, if they were actively in the game and we are too
                if (leavingPlayer->isActive() && (isLocalPlayerHost || (localPlayer != nullptr && localPlayer->isActive())))
                {
                   mNetworkMeshHelper.disconnectFromUser(localPlayer, leavingPlayer);
               
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief remove the supplied player from this game's local roster.  Note: the player is not destroyed, nor is
            the player's peer mesh altered.  No RPC is sent.
        \param[in] player The player we're removing from the local roster
        \return true if the player was queued
    ***************************************************************************************************/
    bool Game::removePlayerFromRoster(Player *player)
    {
        BlazeAssert(player != nullptr);

        uint16_t oldPublic, oldPrivate, newPublic, newPrivate;
        getFirstPartyCapacity(oldPublic, oldPrivate);

        mPlayerRosterMap.erase(player->getId());

        getFirstPartyCapacity(newPublic, newPrivate);
        if (newPublic != oldPublic || newPrivate != oldPrivate)
        {
            mGameManagerApi.getNetworkAdapter()->updateCapacity(this);
        }

        // We have to check the queue by player id since the slot's are used to indicated
        // your index in the queue.
        PlayerRosterList::iterator queueIter = mQueuedPlayers.find(player->getSlotId());
        if (queueIter != mQueuedPlayers.end())
        {
            Player *queuedPlayer = queueIter->second;
            if (player->getId() == queuedPlayer->getId())
            {
                mQueuedPlayers.erase(player->getSlotId());
                return true;
            }  
        }

        PlayerRosterList::iterator rosterIter = mRosterPlayers.find(player->getSlotId());
        if (rosterIter == mRosterPlayers.end())
        {
            // We return true since roster players slot id's can never change.  So if we
            // got here, the player was removed from the queue, but we received
            // queue change notifications before the removal.
            return true;
        }
        if (rosterIter != mRosterPlayers.end())
        {
            Player *rosterPlayer = rosterIter->second;
            if (rosterPlayer->getId() != player->getId())
            {
                // Same as above, This is to make sure that a queued player who's index is being
                // used by someone else already doesn't effect the roster.
                return true;
            }
        }
        mRosterPlayers.erase(rosterIter);

        PlayerRosterList::iterator activeIter = mActivePlayers.find(player->getSlotId());
        if (activeIter != mActivePlayers.end())
        {
            mActivePlayers.erase(activeIter);
        }

        // reduce the number of players in the particular slot type
        --mPlayerSlotCounts[player->getSlotType()];

        // reduce the number of players in the particular team
        TeamIndex teamIndex = player->getTeamIndex();
        decrementLocalTeamSize(teamIndex, player->getRoleName());

        // player->isHost() returns true if any player on the console/network mesh endpoint returns true. 
        // If the last player on a network mesh endpoint is being removed, then it can no longer be the topology host. So we clear that member below. 
        // The check is against 1 and not 0. When the last player leaves, the player is still being removed and the member count is returned as 1 here.
        if (player->isHost() && player->getMeshEndpoint()->getMemberCount() == 1)
        {
            mTopologyHostMeshEndpoint = nullptr;
        }

        if (player == mPlatformHostPlayer)
        {
            mPlatformHostPlayer = nullptr;
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief return true if the game contains the supplied player in any form.
    ***************************************************************************************************/
    bool Game::containsPlayer( const Player *player ) const
    {
        if ( player != nullptr )
        {
            return containsPlayer(player->getId());
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief return true if the game contains the supplied player in any form.
    ***************************************************************************************************/
    bool Game::containsPlayer( const PlayerId playerId ) const
    {
        PlayerMap::const_iterator playerIter = mPlayerRosterMap.find(playerId);
        if (playerIter != mPlayerRosterMap.end())
        {
            return true;
        }

        return false;
    }


    /*! ************************************************************************************************/
    /*! \brief handle an async notification that an external player is joining the game.
    ***************************************************************************************************/
    void Game::onNotifyPlayerJoining(const ReplicatedGamePlayer *joiningPlayerData, uint32_t userIndex, bool performQosDuringConnection)
    {
        PlayerId joiningPlayerId = joiningPlayerData->getPlayerId();
        PlayerId localPlayerId = INVALID_BLAZE_ID;
        if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
            localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();

        if (EA_UNLIKELY( joiningPlayerId == localPlayerId ))
        {
            BlazeAssertMsg(joiningPlayerId != localPlayerId, "Error: onNotifyPlayerJoining sent to the joining player!");
            return;
        }

        Player *joiningPlayer = getPlayerById(joiningPlayerId);
        if (joiningPlayer != nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Dropping onNotifyPlayerJoining for player(%" PRId64 ") already in the game.\n", userIndex, joiningPlayerId);
            return;
        }

        joiningPlayer = addPlayer(*joiningPlayerData);

        joiningPlayer->getPlayerEndpoint()->setPerformQosDuringConnection(performQosDuringConnection);
        
        Player *localPlayer = getLocalPlayer(userIndex);
        if ( (localPlayer != nullptr) && (joiningPlayer->getConnectionGroupId() == localPlayer->getConnectionGroupId()) )
        {
            // exit early if the player is local to us, we'll let themselves establish their own connections,
            // since we share a single network adapter.
            return;
        }
        if (isTopologyHost() || ((localPlayer != nullptr) && localPlayer->isActive()) )
        {
            // only initiate connections if we are networked
            initiatePlayerConnections(joiningPlayer, userIndex);
        }
        
    }

    /*! ************************************************************************************************/
    /*! \brief Handles notificaiton that an external player is claiming a reservation.
    
        \param[in] joiningPlayerData
    ***************************************************************************************************/
    void Game::onNotifyPlayerClaimingReservation(const ReplicatedGamePlayer& joiningPlayerData, uint32_t userIndex, bool performQosDuringConnection)
    {
        PlayerId joiningPlayerId = joiningPlayerData.getPlayerId();
        PlayerId localPlayerId = INVALID_BLAZE_ID;
        if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
            localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();

        if (EA_UNLIKELY( joiningPlayerId == localPlayerId ))
        {
            BlazeAssertMsg(joiningPlayerId != localPlayerId, "Error: onNotifyPlayerClaimingReservation sent to the joining player!");
            return;
        }

        Player *joiningPlayer = getPlayerById(joiningPlayerId);
        if (joiningPlayer == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Dropping onNotifyPlayerClaimingReservation for unknown player(%" PRId64 ").\n", userIndex, joiningPlayerId);
            return;
        }

        // if the claiming player is still queued, do nothing.
        if (joiningPlayerData.getPlayerState() == QUEUED)
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Notification that queued(%" PRId64 ":%s) is claiming reservation in game(%" PRIu64 ":%s) in queue slot(%u).\n", 
                userIndex, joiningPlayerId, joiningPlayerData.getPlayerName(), mGameId, mName.c_str(), joiningPlayerData.getSlotId());
        }
        else
        {
            claimPlayerReservation(joiningPlayerData);

            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Notification that player(%" PRId64 ":%s) is claiming reservation in game(%" PRIu64 ":%s) in slot(%u).\n", 
                userIndex, joiningPlayerId, joiningPlayerData.getPlayerName(), mGameId, mName.c_str(), joiningPlayerData.getSlotId());
            

            Player *localPlayer = getLocalPlayer(userIndex);
        
            if ( (localPlayer != nullptr) && (joiningPlayer->getConnectionGroupId() == localPlayer->getConnectionGroupId()) )
            {    
                // exit early if the player is local to us, we'll let themselves establish their own connections,
                // since we share a single network adapter.
                return;
            }

            if (isTopologyHost() || ((localPlayer != nullptr) && localPlayer->isActive()) )
            {
                // only initiate connections if we are networked
                joiningPlayer->getPlayerEndpoint()->setPerformQosDuringConnection(performQosDuringConnection);
                initiatePlayerConnections(joiningPlayer, userIndex);
            }
            
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Handles notification that an external player is being promoted from the queue
    
        \param[in] joiningPlayerData
    ***************************************************************************************************/
    void Game::onNotifyPlayerPromotedFromQueue(const ReplicatedGamePlayer& joiningPlayerData, uint32_t userIndex)
    {
        PlayerId joiningPlayerId = joiningPlayerData.getPlayerId();
        PlayerId localPlayerId = INVALID_BLAZE_ID;
        if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
            localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();

        if (EA_UNLIKELY( joiningPlayerId == localPlayerId ))
        {
            BlazeAssertMsg(joiningPlayerId != localPlayerId, "Error: onNotifyPlayerJoining sent to the joining player!");
            return;
        }

        Player *joiningPlayer = getPlayerInternal(joiningPlayerId);
        if (joiningPlayer == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Dropping onNotifyPlayerPromotedFromQueue for unknown player(%" PRId64 ")\n", userIndex, joiningPlayerId);
            return;
        }

        promotePlayerFromQueue(joiningPlayerData);

        BLAZE_SDK_DEBUGF("[GAME] UI[%u] Notification that external player(%" PRId64 ":%s) is promoted from queue in game(%" PRIu64 ":%s) in slot(%u).\n", 
            userIndex, joiningPlayerId, joiningPlayerData.getPlayerName(), mGameId, mName.c_str(), joiningPlayerData.getSlotId());

        if (joiningPlayer->getPlayerState() != RESERVED) 
        { 
            Player *localPlayer = getLocalPlayer(userIndex);
            if (isTopologyHost() || ((localPlayer != nullptr) && localPlayer->isActive()) )
            {
                // only initiate connections if we are networked
                initiatePlayerConnections(joiningPlayer, userIndex);
            }

        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] onNotifyPlayerPromotedFromQueue not attempting to initiate connections since player(%" PRId64 ") is still reserved.\n", userIndex, joiningPlayerId);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Handles notification that an external player is being promoted from the queue
    
        \param[in] joiningPlayerData
    ***************************************************************************************************/
    void Game::onNotifyPlayerDemotedToQueue(const ReplicatedGamePlayer& joiningPlayerData, uint32_t userIndex)
    {
         PlayerId joiningPlayerId = joiningPlayerData.getPlayerId();
         
         // Remove the player from the game:

        Player *joiningPlayer = getPlayerInternal(joiningPlayerId);
        if (joiningPlayer == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Dropping onNotifyPlayerDemotedToQueue for unknown player(%" PRId64 ")\n", userIndex, joiningPlayerId);
            return;
        }

        demotePlayerToQueue(joiningPlayerData);

        BLAZE_SDK_DEBUGF("[GAME] UI[%u] Notification that external player(%" PRId64 ":%s) is demoted to queue in game(%" PRIu64 ":%s) in slot(%u).\n", 
            userIndex, joiningPlayerId, joiningPlayerData.getPlayerName(), mGameId, mName.c_str(), joiningPlayerData.getSlotId());
    }

    /*! ************************************************************************************************/
    /*! \brief handle notification that a local inactive player is joining the game (from queue or was reserved).  
        Adds them to the active roster.
    ***************************************************************************************************/
    void Game::onNotifyJoiningPlayerInitiateConnections(const ReplicatedGamePlayer &replicatedPlayer, const GameSetupReason& reason)
    {
        if (reason.getDatalessSetupContext() != nullptr && reason.getDatalessSetupContext()->getSetupContext() == INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT)
        {
            claimPlayerReservation(replicatedPlayer);
        }
        else if ((reason.getDatalessSetupContext() == nullptr) || (reason.getDatalessSetupContext()->getSetupContext() != HOST_INJECTION_SETUP_CONTEXT))
        {
            promotePlayerFromQueue(replicatedPlayer);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief handle notification that a local inactive player is joining the game (from queue or was reserved).  
        Updates the non Local player.
    ***************************************************************************************************/
    void Game::onNotifyLocalPlayerGoingActive(const ReplicatedGamePlayer &playerData, bool performQosDuringConnection)
    {
        Player* player = getPlayerById(playerData.getPlayerId());
        if ((player != nullptr) && player->isActive())
        {
            player->getPlayerEndpoint()->setPerformQosDuringConnection(performQosDuringConnection);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief handle async notification that a player is joining the game as inactive (reserved).
    ***************************************************************************************************/
     void Game::onNotifyInactivePlayerJoining(const ReplicatedGamePlayer &playerData, uint32_t userIndex)
     {
         PlayerId playerId = playerData.getPlayerId();
         PlayerId localPlayerId = INVALID_BLAZE_ID;
         if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
             localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();

         if (EA_UNLIKELY( playerId == localPlayerId ))
         {
             BlazeAssertMsg(false,  "Error: onNotifyInactivePlayerJoining sent to the joining player!");
             return;
         }
         
         Player* joinedPlayer = addPlayer(playerData);      
         if (joinedPlayer == nullptr)
         {
             BLAZE_SDK_DEBUGF("[GAME] UI[%u] Unable to add inactive player joining game(%" PRIu64 ":%s).\n", userIndex, mGameId, mName.c_str());
         }
     }

    /*! ************************************************************************************************/
    /*! \brief handle async notification that a player is joining the game in the queue
    ***************************************************************************************************/
     void Game::onNotifyInactivePlayerJoiningQueue(const ReplicatedGamePlayer &playerData, uint32_t userIndex)
     {
         PlayerId playerId = playerData.getPlayerId();
         PlayerId localPlayerId = INVALID_BLAZE_ID;
         if (mGameManagerApi.getUserManager()->getLocalUser(userIndex) != nullptr)
             localPlayerId = mGameManagerApi.getUserManager()->getLocalUser(userIndex)->getId();

         if (EA_UNLIKELY( playerId == localPlayerId ))
         {
             BlazeAssertMsg(false,  "Error: onNotifyInactivePlayerJoining sent to the joining player!");
             return;
         }
         
         Player* joinedPlayer = addPlayerToQueue(playerData);       
         if (joinedPlayer == nullptr)
         {
             BLAZE_SDK_DEBUGF("[GAME] UI[%u] Unable to add player to queue joining game(%" PRIu64 ":%s).\n", userIndex, mGameId, mName.c_str());
         }
     }

     /*! ************************************************************************************************/
     /*! \brief async notification to process the game queue
     ***************************************************************************************************/
     void Game::onNotifyProcessQueue()
     {
         mDispatcher.dispatch("onProcessGameQueue", &GameListener::onProcessGameQueue, this);
     }

     /*! ************************************************************************************************/
     /*! \brief async notification to change a players spot in the queue.
     
         \param[in] playerId - the player id whose position has changed.
         \param[in] newQueueIndex - the new queue index of the player
     ***************************************************************************************************/
     void Game::onNotifyQueueChanged(const PlayerIdList& playerIdList)
     {
         bool queueChanged = false;
         size_t size = playerIdList.vectorSize(); 
         for (size_t i = 0; i < size; ++i)
         {
             Player *player = getPlayerInternal(playerIdList[(PlayerIdList::size_type)i]);
             if (player != nullptr)
             {
                 // Check to see if notifications were out of order. Its possible another players
                 // queue update notification has put them in our old slot already.
                 PlayerRosterList::iterator queuedIter = mQueuedPlayers.find(player->getSlotId());
                 if (queuedIter != mQueuedPlayers.end())
                 {
                     Player *queuedPlayer = queuedIter->second;
                     if (queuedPlayer != nullptr && queuedPlayer->getId() == player->getId())
                     {
                         mQueuedPlayers.erase(player->getSlotId());
                     }
                 }

                 player->onNotifyQueueChanged(static_cast<SlotId>(i));
                 mQueuedPlayers[static_cast<SlotId>(i)] = player;
                 queueChanged = true;
             }
             else
             {
                 BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyQueueChanged for unknown player(%" PRId64 ").\n", mGameId, playerIdList[(PlayerIdList::size_type)i]);
             }
         }
         if (queueChanged)
            mDispatcher.dispatch("onQueueChanged", &GameListener::onQueueChanged, this);
     }

     /*! *************************************************************************************************************************/
     /*! \brief async notification to process the hosted connectivity info for the end point
     *****************************************************************************************************************************/
     void Game::onNotifyHostedConnectivityAvailable(const NotifyHostedConnectivityAvailable& notification, uint32_t userIndex)
     {
         PlayerId playerId = notification.getRemotePlayerId();
         BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") hosted connectivity available for player(%" PRId64 ")\n", getId(), playerId);

         Player *player = getPlayerById(playerId);
         if (player != nullptr)
         {
             bool failureNotification = notification.getHostedConnectivityInfo().getHostingServerNetworkAddress().getActiveMemberIndex() == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX;
             if (failureNotification)
             {
                 BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") hosted connectivity FAILED for player(%" PRId64 ").  Check Blaze Server logs for more details.\n", getId(), playerId);
             }

             // In the case of MLU, we will receive multiple notifications for the players that on the connections using CCS.
             // We don't need to handle every message, since all MLU players will be on the same endpoint. 
             if (!failureNotification && player->getPlayerEndpoint()->getHostedConnectivityInfo().equalsValue(notification.getHostedConnectivityInfo()))
                 return;

             player->getPlayerEndpoint()->updateHostedConnectivityInfo(notification.getHostedConnectivityInfo());
             
             // Update the Player Connections, now that we have connectivity:  (as long as network init has already occurred)
             if (!mDelayingInitGameNetworkForCCS)
             {
                 if (mDeferredJoiningPlayerMap.find(player->getId()) == mDeferredJoiningPlayerMap.end())
                 {
                     // only issue a disconnect if the remote player is on the same platform as the local player, we'd not have tried to connect in the first place cross-platform
                     if (player->getPlayerClientPlatform() == getLocalMeshEndpoint()->getEndpointPlatform())
                     {
                         mGameManagerApi.getNetworkAdapter()->disconnectFromEndpoint(player->getPlayerEndpoint());
                     }
                     mGameManagerApi.getNetworkAdapter()->connectToEndpoint(player->getPlayerEndpoint());
                 }
                 else
                 {
                     connectToDeferredJoiningPlayers();
                 }
             }
             else
             {
                 if (failureNotification && !hasFullCCSConnectivity() && !usesGamePlayNetworking())
                 {
                     // Normally, CCS failure just causes a disconnect, but when network topology is disabled (Groups) there's nothing to disconnect, 
                     // so we send an empty notification to the clients to indicate failure, since we will not be initializing the VOIP Network.
                     GameManagerApiJob *gameManagerApiJob = mGameManagerApi.getJob(userIndex, getId());
                     if (gameManagerApiJob != nullptr)
                     {
                         gameManagerApiJob->execute();
                         mGameManagerApi.getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                     }
                 }
             }

             // Handle deferred Game Network CCS operations:
             handleDeferedCCSActions(userIndex);
         }
         else
         {
             BLAZE_SDK_DEBUGF("[GAME] Warning: Game(%" PRIu64 ") dropping NotifyHostedConnectivityAvailable for unknown local player(%" PRId64 ").\n", mGameId, playerId);
         }
     }

     void Game::handleDeferedCCSActions(uint32_t userIndex)
     {
         if (mDelayingInitGameNetworkForCCS)
         {
             bool readyToInitGameNetwork = hasFullCCSConnectivity();
             if (readyToInitGameNetwork)
             {
                 mDelayingInitGameNetworkForCCS = false;

                 // Initialize the network as all the endpoints have hosted connectivity now.
                 mGameManagerApi.initGameNetwork(this, userIndex);
             }
             else
             {
                 BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") delaying initNetworkMesh until all end points have hosted connectivity in a hostedOnly game.\n", getId());
             }
         }
         else if (mDelayingHostMigrationForCCS)
         {
             bool readyToStartTopologyHostMigration = hasFullCCSConnectivity();
             // If we have the connectivity against desired endpoints, go ahead and perform host migration.
             if (readyToStartTopologyHostMigration)
             {
                 mDelayingHostMigrationForCCS = false;

                 mGameManagerApi.getNetworkAdapter()->migrateTopologyHost(this);
             }
             else
             {
                 BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") delaying migrateTopologyHost until all end points have hosted connectivity in a hostedOnly game.\n", getId());
             }
         }
     }

    /*! ************************************************************************************************/
    /*! \brief initiate game networking connections due to a player joining a game.  If a connection to the joining player is not yet ready to initiate, we'll add an entry to the internal deferred joining player map, 
        to retry later. Note: the joining player could be yourself!
         \return true if action only updated a user on an existing connection.
    ***************************************************************************************************/
    bool Game::initiatePlayerConnections(const Player* joiningPlayer, uint32_t userIndex)
    {
        if ( EA_UNLIKELY(joiningPlayer == nullptr) )
        {
            BlazeAssert(joiningPlayer != nullptr);
            return false;
        }

        Player* localPlayer = getLocalPlayer(userIndex);

        // Deferred players - anyone that is not ready to be connected to yet, gets cached off to be tried later.
        if (!isTopologyHost())
        {
            if (mIsLocalPlayerPreInitNetwork || (localPlayer == nullptr) || getLocalPlayer(userIndex)->isReserved())
            {
                BLAZE_SDK_DEBUGF("[GAME] UI[%u] we're not ready to connect to this player yet - (%" PRId64 ")\n", userIndex, joiningPlayer->getId());
                mDeferredJoiningPlayerMap.insert(eastl::make_pair(joiningPlayer->getId(), joiningPlayer));
                return false;
            }
        }
        else //(isTopologyHost())
        {
            if (!isNetworkCreated() && (!joiningPlayer->isLocalPlayer()))
            {
                BLAZE_SDK_DEBUGF("[GAME] UI[%u] we're not ready to connect to this player yet - (%" PRId64 "). The game hasn't yet created a network locally\n", userIndex, joiningPlayer->getId());
                mDeferredJoiningPlayerMap.insert(eastl::make_pair(joiningPlayer->getId(), joiningPlayer));
                return false;
            }
        }
        // if CCS is on, and the mode is hosted only or the endpoints are on different platforms, wait for hosted connectivity
        if (playerRequiresCCSConnectivity(joiningPlayer))
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] we're not ready to connect to this player yet - (%" PRId64 "). Hosted connectivity is not yet available\n", userIndex, joiningPlayer->getId());
            mDeferredJoiningPlayerMap.insert(eastl::make_pair(joiningPlayer->getId(), joiningPlayer));
            return false;
        }

        // The player can be processed now, remove it from the map
        mDeferredJoiningPlayerMap.erase(joiningPlayer->getId());

        bool newConnection = mNetworkMeshHelper.connectToUser(this, localPlayer, joiningPlayer);

        // If we're only adding a user on an endpoint, we need to update the mesh on the server here
        // as there is no callback from the adapter currently.
        if (!newConnection)
        {
            BLAZE_SDK_DEBUGF("[GAME] UI[%u] Network adapter already had connected to endpoint - no action taken\n", userIndex);
        }

        return !newConnection;
    }


    /*! ************************************************************************************************/
    /*! \brief a player has "fully" joined the game (joined game's network mesh).
    ***************************************************************************************************/
    void Game::onNotifyPlayerJoinComplete(const NotifyPlayerJoinCompleted *notifyPlayerJoinCompleted, uint32_t userIndex)
    {
        PlayerId playerId = notifyPlayerJoinCompleted->getPlayerId();
        Player *player = getPlayerInternal(playerId);
        if (player != nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] Player(%" PRId64 ":%s) has joined game(%" PRIu64 ")'s peer network mesh.\n", playerId, player->getName(), notifyPlayerJoinCompleted->getGameId());
            player->mJoinedGameTimestamp = notifyPlayerJoinCompleted->getJoinedGameTimestamp();

            bool awaitUpdatePrimaryGame = false;
#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
            // update local user's primary game before dispatching onPlayerJoinComplete to titles
            if ((getLocalPlayer(userIndex) != nullptr) && (getLocalPlayer(userIndex)->getId() == notifyPlayerJoinCompleted->getPlayerId()))
            {
                awaitUpdatePrimaryGame = true;
                mGameManagerApi.updateGamePresenceForLocalUser(userIndex, *this, UPDATE_PRESENCE_REASON_GAME_JOIN, nullptr);
            }
#endif
            if (!awaitUpdatePrimaryGame)
            {
                mDispatcher.dispatch("onPlayerJoinComplete", &GameListener::onPlayerJoinComplete, player);
            }

            if (EA_LIKELY(player->getMeshEndpoint() != nullptr))
            {
                // If we were doing MM qos validation, connapi now automatically gates adding the voip connection (or ref count, if shared) until we tell it qos is done.
                // Now that we know qos is done, tell connapi to ungate as needed. (Side: calling this no-ops if the voip was not actually gated, or if already added it).
                mGameManagerApi.getNetworkAdapter()->activateDelayedVoipOnEndpoint(player->getMeshEndpoint());

                // The ungate calls must be done 2-way, so must also ensure ungated my connections to players who were in the game before I joined.
                if (player->isLocal())
                {                    
                    for (uint16_t i = 0; i < getPlayerCount(); ++i)
                    {
                        Blaze::GameManager::Player *next = getPlayerByIndex(i);
                        if (EA_LIKELY(next != nullptr) && (next != player))//already called on 'player'
                            mGameManagerApi.getNetworkAdapter()->activateDelayedVoipOnEndpoint(next->getMeshEndpoint());
                    }
                }
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Dropping NotifyPlayerJoinComplete for player (%" PRId64 ") who isn't in the game.\n", notifyPlayerJoinCompleted->getPlayerId());
        }

    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief handle a notification that the game name has been changed
    ***************************************************************************************************/
    void Game::onNotifyGameNameChanged(const GameName &newName)
    {
        BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") updating game name to %s.\n", mGameId, newName.c_str());

        mName = newName;

        mDispatcher.dispatch("onGameNameChanged", &GameListener::onGameNameChanged, this, newName);
    }

    /*! ************************************************************************************************/
    /*! \brief handle a notification that the game presence mode has been
    ***************************************************************************************************/
    void Game::onNotifyPresenceModeChanged(PresenceMode newPresenceMode)
    {
        BLAZE_SDK_DEBUGF("[GAME] Game(%" PRIu64 ") updating presence mode to %s.\n", mGameId, PresenceModeToString(newPresenceMode));

        mPresenceMode = newPresenceMode;
    }

    /*! ************************************************************************************************/
    /*! \brief Send an RPC directing the local player to remove himself from the game.
        \param[in] titleCb   - The callback dispatched upon rpc completion.
        \param[in] userGroup - The user group who would like to leave the game together,
                                default is nullptr, means it's a single player leaves operation
        \param[in] makeReservation - Leave game and make disconnect reservation
    ***************************************************************************************************/
    JobId Game::leaveGame(const LeaveGameJobCb &titleCb, const UserGroup* userGroup /*=nullptr*/, bool makeReservation /*= false*/)
    {
        uint32_t foundIndex = mGameManagerApi.getBlazeHub()->getPrimaryLocalUserIndex();
        if (EA_UNLIKELY(getLocalPlayer(foundIndex) == nullptr))
        {
            // primary not member of game anymore, check to make sure someone is.
            // blaze server allows leave by group to succeed even though some users are not
            // in the game.  We mirror that here by allowing any of the local members still
            // in the game to issue the leave.
            for (uint32_t index=0; index < mGameManagerApi.getBlazeHub()->getNumUsers(); ++index)
            {
                if (getLocalPlayer(index) != nullptr)
                {
                    foundIndex = index;
                    break;
                }
            }

            // Didn't find anybody besides the primary to own this.
            if (foundIndex == mGameManagerApi.getBlazeHub()->getPrimaryLocalUserIndex())
            {
                // Error: can't leave a game you're not a member of
                BLAZE_SDK_DEBUG("[GAME] Error: attempting to leave a game that you're not a member of.\n");
                JobId jobId = mGameManagerApi.getBlazeHub()->getScheduler()->reserveJobId();
                LeaveGameJob *job = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "LeaveGameJob") LeaveGameJob(titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, jobId);
                mGameManagerApi.getBlazeHub()->getScheduler()->scheduleJob("LeaveGameJob", job, this, 0, jobId);
                Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
                return jobId;
            }
        }


        BLAZE_SDK_DEBUGF("[GAME] Local player(%" PRId64 ":%s) leaving game(%" PRIu64 ")\n", 
            getLocalPlayer(foundIndex)->getId(), getLocalPlayer(foundIndex)->getName(), mGameId);

        // Setup request
        RemovePlayerRequest removePlayerRequest;
        removePlayerRequest.setGameId(mGameId);
        removePlayerRequest.setPlayerId(getLocalPlayer(foundIndex)->getId());

        if (userGroup != nullptr)
        {
            // user group left
            removePlayerRequest.setPlayerRemovedReason((makeReservation) ? GROUP_LEFT_MAKE_RESERVATION : GROUP_LEFT);
            removePlayerRequest.setGroupId(userGroup->getBlazeObjectId());
            JobId jobId = mGameManagerApi.mGameManagerComponent->leaveGameByGroup(removePlayerRequest, 
                MakeFunctor(this, &Game::leaveGameCb), titleCb);
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }
        else
        {
            // single player leaving (default to connection group)
            uint32_t userIndex = foundIndex;
            UserManager::LocalUser* localUser = mGameManagerApi.getBlazeHub()->getUserManager()->getLocalUser(userIndex);
            if (localUser != nullptr)
            {
                removePlayerRequest.setGroupId(localUser->getConnectionGroupObjectId());
                removePlayerRequest.setPlayerRemovedReason((makeReservation) ? GROUP_LEFT_MAKE_RESERVATION : GROUP_LEFT);
                JobId jobId = mGameManagerApi.getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent()->leaveGameByGroup(removePlayerRequest, 
                    MakeFunctor(this, &Game::leaveGameCb), titleCb);
                Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
                return jobId;
            }

            // With conneciton groups, we should never technically get here
            // GM AUDIT - consider consolidating RPC with leavegamebygroup.
            removePlayerRequest.setPlayerRemovedReason((makeReservation) ? PLAYER_LEFT_MAKE_RESERVATION : PLAYER_LEFT);
            JobId jobId = mGameManagerApi.mGameManagerComponent->removePlayer(removePlayerRequest, 
                MakeFunctor(this, &Game::leaveGameCb), titleCb);
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }
    }


    JobId Game::leaveGameLocalUser(uint32_t userIndex, const LeaveGameJobCb &titleCb, bool makeReservation /*= false*/)
    {
        Player* localPlayer = getLocalPlayer(userIndex);
        if (EA_UNLIKELY(localPlayer == nullptr))
        {
            BLAZE_SDK_DEBUGF("[GAME] Error: attempting to leave a game that user index(%u) is not a member of.\n", userIndex);
            JobId jobId = mGameManagerApi.getBlazeHub()->getScheduler()->reserveJobId();
            LeaveGameJob *job = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "LeaveGameLocalUserJob") LeaveGameJob(titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, jobId);
            mGameManagerApi.getBlazeHub()->getScheduler()->scheduleJob("LeaveGameLocalUserJob", job, this, 0, jobId);
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }

        // User must be a local user, that is logged in.
        if (EA_UNLIKELY((userIndex >= mGameManagerApi.getBlazeHub()->getNumUsers()) || 
                (mGameManagerApi.getBlazeHub()->getUserManager()->getLocalUser(userIndex) == nullptr)))
        {
            BLAZE_SDK_DEBUGF("[GAME] Error: User index(%u) attempting to leave a game when they are not logged in.\n", userIndex);
            JobId jobId = mGameManagerApi.getBlazeHub()->getScheduler()->reserveJobId();
            LeaveGameJob *job = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "LeaveGameLocalUserJob") LeaveGameJob(titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this, jobId);
            mGameManagerApi.getBlazeHub()->getScheduler()->scheduleJob("LeaveGameLocalUserJob", job, this, 0, jobId);
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }

        if (localPlayer->isPrimaryLocalPlayer())
        {
            BLAZE_SDK_DEBUGF("[GAME] Error: user index(%u), the primary local user is attempting to leave a game individually. Switch the primary local user or remove all local users.\n", userIndex);
            JobId jobId = mGameManagerApi.getBlazeHub()->getScheduler()->reserveJobId();
            LeaveGameJob *job = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "LeaveGameLocalUserJob") LeaveGameJob(titleCb, GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN, this, jobId);
            mGameManagerApi.getBlazeHub()->getScheduler()->scheduleJob("LeaveGameLocalUserJob", job, this, 0, jobId);
            Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
            return jobId;
        }

        // Setup request
        RemovePlayerRequest removePlayerRequest;
        removePlayerRequest.setGameId(mGameId);
        removePlayerRequest.setPlayerId(localPlayer->getId());
        removePlayerRequest.setPlayerRemovedReason((makeReservation) ? PLAYER_LEFT_MAKE_RESERVATION : PLAYER_LEFT);

        // Send the request from the specified user index
        JobId jobId = mGameManagerApi.getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent()->removePlayer(removePlayerRequest, 
            MakeFunctor(this, &Game::leaveGameCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }


    /*! ************************************************************************************************/
    /*! \brief handle RPC response for leaving a game; expect a player removal notification on success
    ***************************************************************************************************/
    void Game::leaveGameCb(BlazeError error, JobId id, LeaveGameJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief destroy this game (sends RPC)
    ***************************************************************************************************/
    JobId Game::destroyGame(GameDestructionReason gameDestructionReason, const DestroyGameJobCb &titleCb)
    {
        BLAZE_SDK_DEBUGF("[GAME] Attempting to destroy game(%" PRIu64 ") reason: %s\n", mGameId, Blaze::GameManager::GameDestructionReasonToString(gameDestructionReason));

        DestroyGameRequest destroyGameRequest;
        destroyGameRequest.setGameId(mGameId);
        destroyGameRequest.setDestructionReason(gameDestructionReason);

        JobId jobId = getAdminGameManagerComponent()->destroyGame(destroyGameRequest, 
            MakeFunctor(this, &Game::destroyGameCb), titleCb, gameDestructionReason);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }


    /*! ************************************************************************************************/
    /*! \brief RPC callback for destroyGame
    ***************************************************************************************************/
    void Game::destroyGameCb(const DestroyGameResponse *response, BlazeError error, JobId id, DestroyGameJobCb titleCb, GameDestructionReason destructionReason)
    {
        titleCb(error, this, id);

        // destroy the game after dispatch so we don't invalidate game pointer(s)
        if (error == ERR_OK)
        {
            bool wasActive = (getLocalPlayer()!=nullptr ? getLocalPlayer()->isActive() : true);
            mGameManagerApi.destroyLocalGame(this, destructionReason, isTopologyHost(), wasActive);
        }
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    /*! ************************************************************************************************/
    /*! \brief ejects the current user as the host of a dedicated server game.
    ***************************************************************************************************/
    JobId Game::ejectHost(const EjectHostJobCb &titleCb, bool replaceHost /*= false*/)
    {
        if (EA_UNLIKELY(!isTopologyHost()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: calling ejectHost on game(%" PRIu64 ") which local player(%" PRId64 ") is not the topology host.\n",
                mGameId, (getLocalPlayer() ? getLocalPlayer()->getId() : INVALID_BLAZE_ID));
            return JobCbHelper("ejectHostCb", titleCb, GAMEMANAGER_ERR_NOT_TOPOLOGY_HOST, this);
        }

        if (EA_UNLIKELY(!isDedicatedServerTopology()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Warning: calling ejectHost on game(%" PRIu64 ") which is not a dedicated server.\n", mGameId);
            return JobCbHelper("ejectHostCb", titleCb, GAMEMANAGER_ERR_DEDICATED_SERVER_ONLY_ACTION, this);
        }

        // For non-virtual dedicated servers, just destroy the game if no replacement is desired:
        if (replaceHost == false && EA_UNLIKELY(!(getGameSettings().getVirtualized())))
        {
            return destroyGame(HOST_LEAVING, titleCb);
        }

        EjectHostRequest ejectHostRequest;
        ejectHostRequest.setGameId(mGameId);
        ejectHostRequest.setReplaceHost(replaceHost);

        JobId jobId = mGameManagerApi.getGameManagerComponent()->ejectHost(ejectHostRequest, 
            MakeFunctor(this, &Game::ejectHostCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    void Game::ejectHostCb(BlazeError error, JobId id, EjectHostJobCb titleCb)
    {
        titleCb(error, this, id);

        // Destroy the local game object
        if (error == ERR_OK)
        {
            bool wasActive = (getLocalPlayer()!=nullptr ? getLocalPlayer()->isActive() : true); 
            mGameManagerApi.destroyLocalGame(this, HOST_EJECTION, isTopologyHost(), wasActive);
        }
    }

    GameBase::TeamInfo* Game::getTeamByIndex(TeamIndex teamIndex)
    {
        if (teamIndex < mTeamInfoVector.size())
            return &(mTeamInfoVector[teamIndex]);
        else
            return nullptr;
    }

    /*! ****************************************************************************/
    /*! \brief Gets the QOS statistics for a given endpoint

        \param endpoint The target endpoint
        \param qosData a struct to contain the qos values
        \return a bool true, if the call was successful
    ********************************************************************************/
    bool Game::getQosStatisticsForEndpoint(const MeshEndpoint *endpoint, Blaze::BlazeNetworkAdapter::MeshEndpointQos &qosData, bool bInitialQosStats) const
    {

#ifdef ARSON_BLAZE
        RemoteConnGroupIdToQosStatsDebugMap::const_iterator itr = ((endpoint != nullptr) ? mQosStatisticsDebugMap.find(endpoint->getConnectionGroupId()) : mQosStatisticsDebugMap.end());
        if (itr != mQosStatisticsDebugMap.end())
        {
            qosData = itr->second;
            return true;
        }
#endif

        return mGameManagerApi.getNetworkAdapter()->getQosStatisticsForEndpoint(endpoint, qosData, bInitialQosStats);
    }

    /*! ****************************************************************************/
    /*! \brief Gets the QOS statistics for the topology host

        \param qosData a struct to contain the qos values
        \return a bool true, if the call was successful
    ********************************************************************************/
    bool Game::getQosStatisticsForTopologyHost(Blaze::BlazeNetworkAdapter::MeshEndpointQos &qosData, bool bInitialQosStats) const
    {
        return getQosStatisticsForEndpoint(getTopologyHostMeshEndpoint(), qosData, bInitialQosStats);
    }

#ifdef ARSON_BLAZE
    /*! *******************************************************************************************/
    /*! \brief Sets QoS data for a network connection, for debugging purposes and testing of GM/MM's QoS validation feature.
        Overrides QoS info BlazeSDK internally sends to Blaze Server via its mesh connection updates.
    ***********************************************************************************************/
    void Game::setQosStatisticsDebug(ConnectionGroupId remoteConnGroupId, const Blaze::BlazeNetworkAdapter::MeshEndpointQos& debugQosData)
    {
        mQosStatisticsDebugMap[remoteConnGroupId] = debugQosData;
    }

    void Game::clearQosStatisticsDebug(ConnectionGroupId remoteConnGroupId)
    {
        mQosStatisticsDebugMap.erase(remoteConnGroupId);
    }
#endif

    const NetworkAddress* Game::getNetworkAddress(const GameEndpoint* endpoint) const 
    {
        if (endpoint != nullptr) 
        {
            return GameBase::getNetworkAddress(endpoint->getNetworkAddressList());
        }

        return nullptr;
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns the dedicated server's network address list, used to initialize the game
    **************************************************************************************************************/
    NetworkAddressList* Game::getNetworkAddressList()
    {
        return (mDedicatedHostEndpoint != nullptr) ? mDedicatedHostEndpoint->getNetworkAddressList() : nullptr; 
    }

    /*! ************************************************************************************************/
    /*! \brief Initiates player connections to any players in the deferred joining player map.
        This is to handle connecting to any players that may join while we are still waiting for the
        network adapter to tell us the network has actually been created.
    ***************************************************************************************************/
    void Game::connectToDeferredJoiningPlayers()
    {
        // Make a copy first as we erase entries from original map if the player gets processed
        DeferredJoiningPlayerMap deferredPlayers = mDeferredJoiningPlayerMap;

        for (DeferredJoiningPlayerMap::const_iterator iter = deferredPlayers.begin(), end = deferredPlayers.end(); iter != end; ++iter)
        {
            // We use the primary user index here since we came off a callback from the singleton network adapter.
            uint32_t userIndex = getGameManagerAPI()->getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex();
            BLAZE_SDK_DEBUGF("[GAME] Local player(%" PRId64 ") is connecting to defered joining player(%" PRId64 ")\n",
                (getLocalPlayer(userIndex) ? getLocalPlayer(userIndex)->getId() : INVALID_BLAZE_ID), iter->first);

            initiatePlayerConnections(iter->second, userIndex);
        }
    }

    bool Game::resolveGameMembership(Game* newGame)
    {
        // we always first re-set this to the value from the API params, because the accesor for this member should only be used during a listener dispatch
        mRemoveLocalPlayerAfterResolve = mGameManagerApi.getApiParams().mPreventMultipleGameMembership;

        mDispatcher.dispatch("onResolveGameMembership", &GameListener::onResolveGameMembership, this, newGame);

        return mRemoveLocalPlayerAfterResolve;
    }

    /*! ************************************************************************************************/
    /*! \brief Get a roster player by blaze id.  Player is in the game roster, though could still be 
        inactive if reserved.
    
        \param[in] playerId
        \return the player
    ***************************************************************************************************/
    Player* Game::getRosterPlayerById(BlazeId playerId) const
    {
        // find the player in the player vector
        PlayerRosterList::const_iterator playerIter = mRosterPlayers.begin();
        PlayerRosterList::const_iterator playerEnd = mRosterPlayers.end();
        for ( ; playerIter!=playerEnd; ++playerIter )
        {
            Player *player = playerIter->second;
            if (player->getId() == playerId)
            {
                return player;
            }
        }

        return nullptr; // player not found
    }

    /*! ************************************************************************************************/
    /*! \brief Get a roster player by index.  Player is in the games roster, though could still
        be inactive if reserved.
    
        \param[in] index
        \return the player
    ***************************************************************************************************/
    Player* Game::getRosterPlayerByIndex(uint16_t index) const
    {
        if ( index < mRosterPlayers.size() )
        {
            Player *player = mRosterPlayers.at(index).second;
            return player;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Get a roster player by name.  Player is in the games roster, though could still
        be inactive if reserved.
    
        \param[in] personaName - players persona name
        \return the player
    ***************************************************************************************************/
    Player* Game::getRosterPlayerByName(const char8_t *personaName) const
    {
        return getRosterPlayerByUser( mGameManagerApi.getUserManager()->getUserByName( personaName ) );
    }

    /*! ************************************************************************************************/
    /*! \brief Get a roster player by name.  Player is in the games roster, though could still
        be inactive if reserved.
    
        \param[in] user
        \return the player
    ***************************************************************************************************/
    Player* Game::getRosterPlayerByUser(const UserManager::User *user) const
    {
        if ( user != nullptr )
        {
            return getRosterPlayerById( user->getId() );
        }

        return nullptr;
    }


    Player* Game::getPlayerInternal(const PlayerId playerId) const
    {
        PlayerMap::const_iterator playerIter = mPlayerRosterMap.find(playerId);
        if (playerIter != mPlayerRosterMap.end())
        {
            return playerIter->second;
        }

        return nullptr;
    }

    void Game::completeRemoteHostInjection(const NotifyGameSetup *notifyGameSetup)
    {
        if ((notifyGameSetup == nullptr) || (notifyGameSetup->getGameSetupReason().getDatalessSetupContext() == nullptr) || 
            (notifyGameSetup->getGameSetupReason().getDatalessSetupContext()->getSetupContext() != HOST_INJECTION_SETUP_CONTEXT))
        {
            // we shouldn't be here, early out
            return;
        }

        if (!isDedicatedServerTopology())
        {
            // this should only happen for dedicated server games (p2p games should just go through normal host migration)
            return;
        }

        // update host connection info
        storeHostConnectionInfoPostInjection(*notifyGameSetup);

        // update admin list with host
        notifyGameSetup->getGameData().getAdminPlayerList().copyInto(mAdminList);
    }

    void Game::storeHostConnectionInfoPostInjection(const NotifyGameSetup &notifyGameSetup)
    {
        if (!isDedicatedServerTopology())
        {
            BLAZE_SDK_DEBUGF("[GAME] storeHostConnectionInfoPostInjection, warning: possible internal error game was not a dedicated server game, while we are attempting to update host information for game(%" PRId64 ").\n", getId());
            return;
        }
        BLAZE_SDK_DEBUGF("[GAME] storeHostConnectionInfoPostInjection, update host information for game(%" PRIu64 "), hostId %" PRId64 " (old value %" PRId64 "), hostConnectionSlotId %u (old value %u), hostConnGroupId %" PRIu64 " (old value %" PRIu64 ").\n",
            getId(), notifyGameSetup.getGameData().getDedicatedServerHostInfo().getPlayerId(), mDedicatedServerHostId,
            notifyGameSetup.getGameData().getDedicatedServerHostInfo().getConnectionSlotId(), getTopologyHostConnectionSlotId(), notifyGameSetup.getGameData().getDedicatedServerHostInfo().getConnectionGroupId(), getTopologyHostConnectionGroupId());

        mDedicatedServerHostId = notifyGameSetup.getGameData().getDedicatedServerHostInfo().getPlayerId();
        mTopologyHostId = mDedicatedServerHostId;

        // build dedicated host endpoint
        // build the endpoint

        BlazeAssertMsg(getMeshEndpointByConnectionGroupId(notifyGameSetup.getGameData().getDedicatedServerHostInfo().getConnectionGroupId()) == nullptr, 
            "Host injection supplied a dedicated server host with a connection group that is already present in the game!");

        mDedicatedHostEndpoint = new (mGameEndpointMemoryPool.alloc()) GameEndpoint(this, notifyGameSetup.getGameData().getDedicatedServerHostInfo().getConnectionGroupId(), 
            notifyGameSetup.getGameData().getDedicatedServerHostInfo().getConnectionSlotId(), notifyGameSetup.getGameData().getDedicatedServerHostNetworkAddressList(), notifyGameSetup.getGameData().getUUID(), 
            CONNECTED, mMemGroup);
        
        mTopologyHostMeshEndpoint = mDedicatedHostEndpoint;

        // put it in the endpoint map
        mGameEndpointMap[mDedicatedHostEndpoint->getConnectionGroupId()] = mDedicatedHostEndpoint;
        // put it in the active endpoints
        activateEndpoint(mDedicatedHostEndpoint);
       
    }

    void Game::gameVoipConnected(Blaze::ConnectionGroupId targetConnGroupId)
    {
        PlayerVector connectedPlayerVector(MEM_GROUP_FRAMEWORK_TEMP, "gameVoipConnected.connectedPlayerVector");
        getActivePlayerByConnectionGroupId(targetConnGroupId, connectedPlayerVector);
        if (!connectedPlayerVector.empty())
        {
            mDispatcher.dispatch("onVoipConnected", &GameListener::onVoipConnected, this, &connectedPlayerVector);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] gameVoipConnected, no players from connection group(%" PRId64 ") was found in game(%" PRId64 ").\n", targetConnGroupId, mGameId );
        }
    }

    void Game::gameVoipLostConnection(Blaze::ConnectionGroupId targetConnGroupId)
    {
        PlayerVector disconnectedPlayerVector(MEM_GROUP_FRAMEWORK_TEMP, "gameVoipLostConnection.disconnectedPlayerVector");
        getActivePlayerByConnectionGroupId(targetConnGroupId, disconnectedPlayerVector);
        if (!disconnectedPlayerVector.empty())
        {
            mDispatcher.dispatch("onVoipConnectionLost", &GameListener::onVoipConnectionLost, this, &disconnectedPlayerVector);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] gameVoipLostConnection, no players from connection group(%" PRId64 ") was found in game(%" PRId64 ").\n", targetConnGroupId, mGameId );
        }
    }

    GameManagerComponent* Game::getAdminGameManagerComponent() const
    {
        // not using MLU
        if (mGameManagerApi.getBlazeHub()->getNumUsers() == 1)
        {
            return mGameManagerApi.getGameManagerComponent();
        }

        // Check the primary.
        UserManager::LocalUser* primary = mGameManagerApi.getBlazeHub()->getUserManager()->getPrimaryLocalUser();
        if (primary != nullptr)
        {
            if (isAdmin(primary->getId()))
            {
                return mGameManagerApi.getBlazeHub()->getComponentManager(primary->getUserIndex())->getGameManagerComponent();
            }
        }
        // Check other local users.
        for (uint32_t index=0; index < mGameManagerApi.getBlazeHub()->getNumUsers(); ++index)
        {
            UserManager::LocalUser* user = mGameManagerApi.getBlazeHub()->getUserManager()->getLocalUser(index);
            if (user != nullptr)
            {
                if (isAdmin(user->getId()))
                {
                    return mGameManagerApi.getBlazeHub()->getComponentManager(user->getUserIndex())->getGameManagerComponent();
                }
            }
        }
        // if nothing else, just use the primaries.  Component manager is always active.
        return mGameManagerApi.getGameManagerComponent();
    }


    GameManagerComponent* Game::getPlayerGameManagerComponent(PlayerId playerId) const
    {
        // Check to see if the player is local and use that, otherwise default to what admin method returns.
        for (uint32_t index=0; index < mGameManagerApi.getBlazeHub()->getNumUsers(); ++index)
        {
            UserManager::LocalUser* user = mGameManagerApi.getBlazeHub()->getUserManager()->getLocalUser(index);
            if (user != nullptr)
            {
                if (user->getId() == playerId)
                {
                    return mGameManagerApi.getBlazeHub()->getComponentManager(user->getUserIndex())->getGameManagerComponent();
                }
            }
        }
        return getAdminGameManagerComponent();
    }

    JobId Game::preferredJoinOptOut(const PreferredJoinOptOutJobCb &titleCb)
    {
        // early out if locking games for preferred joins is disabled.
        if (!mIsLockableForPreferredJoins)
        {
            BLAZE_SDK_DEBUGF("[GAME] Ignoring attempting to preferred join opt out of a game that is not lockable for perferred joins.\n");
            return JobCbHelper("preferredJoinOptOutCb", titleCb, ERR_SYSTEM, this);
        }

        const uint32_t userIndex = mGameManagerApi.getBlazeHub()->getPrimaryLocalUserIndex();
        Player* localPlayer = getLocalPlayer(userIndex);
        if (localPlayer == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] Ignoring attempting to opt out of a game that user index(%u) is not a member of.\n", userIndex);
            return JobCbHelper("preferredJoinOptOutCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, this);
        }

        // Setup request
        PreferredJoinOptOutRequest request;
        request.setGameId(getId());

        GameManagerComponent *gameManagerComponent = mGameManagerApi.getGameManagerComponent();
        JobId jobId = gameManagerComponent->preferredJoinOptOut(request, MakeFunctor(this, &Game::preferredJoinOptOutCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! \brief handle RPC response for opting out of preferred join */
    void Game::preferredJoinOptOutCb(BlazeError error, JobId id, PreferredJoinOptOutJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    uint16_t Game::getFirstPartyCapacity(uint16_t &publicCapacity, uint16_t &privateCapacity) const
    {
        publicCapacity = static_cast<uint16_t>( getPlayerCapacity(SLOT_PUBLIC_PARTICIPANT) + getPlayerCapacity(SLOT_PUBLIC_SPECTATOR) );
        privateCapacity = static_cast<uint16_t>( getPlayerCapacity(SLOT_PRIVATE_PARTICIPANT) + getPlayerCapacity(SLOT_PRIVATE_SPECTATOR) );

        // iterate over every player, check their slot type, and ClientData:
        for (PlayerMap::const_iterator playerIter = mPlayerRosterMap.begin(), playerEnd = mPlayerRosterMap.end(); playerIter != playerEnd; ++playerIter)
        {
            Player* player = playerIter->second;
            
            // Each slot filled with a LIMITED user needs to be removed from the first party capacities. 
            if (!player->getHasJoinFirstPartyGameSessionPermission())
            {
                SlotType slotType = player->getSlotType();
                if (slotType == SLOT_PUBLIC_PARTICIPANT || slotType == SLOT_PUBLIC_SPECTATOR)
                    --publicCapacity;
                if (slotType == SLOT_PRIVATE_PARTICIPANT || slotType == SLOT_PRIVATE_SPECTATOR)
                    --privateCapacity;
            }
        }

        return publicCapacity + privateCapacity;
    }

    void Game::startTelemetryReporting()
    {
        mTelemetryReportingJobId = mGameManagerApi.getBlazeHub()->getScheduler()->scheduleMethod("reportGameTelemetry",
            this, &Game::reportGameTelemetry, this, (uint32_t)mTelemetryInterval.getMillis());
    }

    void Game::stopTelemetryReporting()
    {
        if (mTelemetryReportingJobId != INVALID_JOB_ID)
        {
            mGameManagerApi.getBlazeHub()->getScheduler()->removeJob(mTelemetryReportingJobId);
            mTelemetryReportingJobId = INVALID_JOB_ID;
        }
    }

    void Game::filloutTelemetryReport(Blaze::GameManager::TelemetryReportRequest* request, const MeshEndpoint *meshEndpoint)
    {
        if (meshEndpoint == nullptr)
            return;

        BlazeNetworkAdapter::MeshEndpointQos qosData;
        if (getQosStatisticsForEndpoint(meshEndpoint, qosData, FALSE))
        {
            GameManager::TelemetryReport *report = request->getTelemetryReports().pull_back();
            report->setRemoteConnGroupId(meshEndpoint->getConnectionGroupId());

            report->setLatencyMs(qosData.LatencyMs);
            report->setPacketLossFloat(qosData.getPacketLossPercent());
            report->setPacketLossPercent((uint8_t)ceil(qosData.getPacketLossPercent()));
            report->setLocalPacketsReceived(qosData.LocalPacketsReceived);
            report->setRemotePacketsSent(qosData.RemotePacketsSent);

            Blaze::BlazeNetworkAdapter::MeshEndpointConnTimer meshConnTimer, voipConnTimer;
            if (mGameManagerApi.getNetworkAdapter()->getConnectionTimersForEndpoint(meshEndpoint, meshConnTimer, voipConnTimer))
            {
                report->setConnTimeAfterDemangle(meshConnTimer.DemangleConnectTime);
                report->setConnTimeBeforeDemangle(meshConnTimer.ConnectTime);
                report->setConnTimeDemangling(meshConnTimer.DemangleTime);
                // report->setConnTimeSecureAssociation(meshConnTimer.CreateSATime);

                // VOIP metrics are unused.
            }


            // Add in the PIN connection metrics:
            Blaze::BlazeNetworkAdapter::MeshEndpointPinStat pinStatData;
            if (mGameManagerApi.getNetworkAdapter()->getPinStatisticsForEndpoint(meshEndpoint, pinStatData, (uint32_t)mTelemetryInterval.getMillis()))
            {
                // Only valid when (pinStatData.bDistStatValid == true):
                report->getReportStats().setDistProcTime(pinStatData.DistProcTime);
                report->getReportStats().setDistWaitTime(pinStatData.DistWaitTime);
                report->getReportStats().setIdpps(pinStatData.Idpps);  // idmpps
                report->getReportStats().setOdpps(pinStatData.Odpps);

                //These values are included here because DirtySock intends to add in these metrics soon-ish. 
                //I just added them in here so it'll be obvious what should be updated when DS finishes their code.
                report->getReportStats().setIpps(pinStatData.Ipps);
                report->getReportStats().setOpps(pinStatData.Opps);
                report->getReportStats().setJitter(pinStatData.JitterMs);
                report->getReportStats().setLnakSent(pinStatData.Lnaksent);
                report->getReportStats().setRnakSent(pinStatData.Rnaksent);
                report->getReportStats().setRpackLost(pinStatData.RpackLost);
                report->getReportStats().setLpackLost(pinStatData.LpackLost);
                report->getReportStats().setLpSaved(pinStatData.Lpacksaved);
                report->getReportStats().setLatency(pinStatData.LatencyMs);
                // report->getReportStats().setRpSaved(pinStatData.RpackSaved);

            }
        }
    }

    void Game::reportGameTelemetry()
    {
        GameManager::TelemetryReportRequest request;
        request.setGameId(getId());
        BlazeAssertMsg(getLocalMeshEndpoint() != nullptr, "The local mesh end point can not be nullptr");
        request.setLocalConnGroupId(getLocalMeshEndpoint() != nullptr ? getLocalMeshEndpoint()->getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID);
        request.setNetworkTopology(getNetworkTopology());

        if (!isTopologyHost() && ((getNetworkTopology() == CLIENT_SERVER_DEDICATED) 
            || (getNetworkTopology() == CLIENT_SERVER_PEER_HOSTED)))
        {
            filloutTelemetryReport(&request, getTopologyHostMeshEndpoint());
        }
        else // either this is a mesh topology or the local user is the host of the hosted topology.
        {    // most topologies only need 2 endpoints for useful info to be collected
            uint16_t minEndpointCheckValue = 2;
            // for hosted topologies, the only information that matters is the connection to the host
            if( getMeshEndpointCount() >= minEndpointCheckValue)
            {
                for (uint16_t index = 0; index < getMeshEndpointCount(); ++index)
                {
                    const MeshEndpoint *meshEndpoint = getMeshEndpointByIndex(index);

                    BlazeAssertMsg(meshEndpoint != nullptr, "nullptr Endpoint in Game's Active Endpoint vector!");

                    if (!meshEndpoint->isLocal())
                    {
                        filloutTelemetryReport(&request, meshEndpoint);
                    }
                }
            }

        }
       
        if (request.getLocalConnGroupId() == 0)
        {
            // log an error, but still send to blaze server so we can see that we somehow got this result.
            BLAZE_SDK_DEBUGF("[GAME] tried to report telemetry for game(%" PRIu64 ")  with local connection group id of (0) in request.\n", getId());
        }

        if (!request.getTelemetryReports().empty())
        {
            // only send telemetry if we got telemetry
            mGameManagerApi.getGameManagerComponent()->reportTelemetry(request, MakeFunctor(this, &Game::reportTelemetryCb));
        }

        mTelemetryReportingJobId = mGameManagerApi.getBlazeHub()->getScheduler()->scheduleMethod("reportGameTelemetry",
            this, &Game::reportGameTelemetry, this, (uint32_t)mTelemetryInterval.getMillis());
    }

    void Game::reportTelemetryCb(BlazeError errorCode, JobId id)
    {
        // nothing to do here
    }

    void Game::cleanUpPlayer(Player *player)
    {
        if (player == nullptr)
        {
            return;
        }

        GameEndpoint* endpoint = player->getPlayerEndpoint();

        mPlayerMemoryPool.free(player);

        if ((endpoint != nullptr) && (endpoint->getMemberCount() == 0))
        {
            cleanUpGameEndpoint(endpoint);
        }
    }

    void Game::cleanUpGameEndpoint(GameEndpoint *endpoint)
    {
        if (endpoint == nullptr)
        {
            return;
        }

        if (endpoint->getMemberCount() != 0)
        {
            BlazeAssertMsg(false, "Cleaning up endpoint that is still in use!");
            return;
        }

        if (endpoint == mLocalEndpoint)
        {
            BLAZE_SDK_DEBUGF("[GAME] Cleaning up local endpoint in game (%" PRIu64 "), endpoint connection group (%" PRIu64 ").\n",
                mGameId, endpoint->getConnectionGroupId());
            mLocalEndpoint = nullptr;
        }

        if (endpoint == mTopologyHostMeshEndpoint)
        {
            BLAZE_SDK_DEBUGF("[GAME] Cleaning up topology host endpoint in game (%" PRIu64 "), endpoint connection group (%" PRIu64 ").\n",
                mGameId, endpoint->getConnectionGroupId());
            mTopologyHostMeshEndpoint = nullptr;
            mTopologyHostId = INVALID_BLAZE_ID;
        }

        if (endpoint == mDedicatedHostEndpoint)
        {
            BLAZE_SDK_DEBUGF("[GAME] Cleaning up dedicated server host endpoint in game (%" PRIu64 "), endpoint connection group (%" PRIu64 ").\n",
                mGameId, endpoint->getConnectionGroupId());
            mDedicatedHostEndpoint = nullptr;
            mDedicatedServerHostId = INVALID_BLAZE_ID;
        }

        // first need to remove the endpoint from the active endpoint list
        EndpointList::iterator activeEndpointIter = mActiveGameEndpoints.find(endpoint->getConnectionSlotId());
        if ((activeEndpointIter != mActiveGameEndpoints.end()) && (activeEndpointIter->second == endpoint))
        {
            // need to compare, because we build endpoints even for reserved/queued players,
            // and the connSlotId for them could be the same as someone in the active map
            BLAZE_SDK_DEBUGF("[GAME] Endpoint in game (%" PRIu64 ") was in mActiveGameEndpoints, endpoint connection group (%" PRIu64 "), erasing from map.\n",
                mGameId, endpoint->getConnectionGroupId());
            mActiveGameEndpoints.erase(activeEndpointIter);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Endpoint in game (%" PRIu64 ") was not in mActiveGameEndpoints, endpoint connection group (%" PRIu64 ").\n",
                mGameId, endpoint->getConnectionGroupId());
        }

        // then the endpoint map
        EndpointMap::iterator endpointIter = mGameEndpointMap.find(endpoint->getConnectionGroupId());
        if ((endpointIter != mGameEndpointMap.end()) && (endpointIter->second == endpoint))
        {
            // potentially, we've got a endpoint that hasn't been added to this map yet
            // so verify we have the right item.
            BLAZE_SDK_DEBUGF("[GAME] Endpoint in game (%" PRIu64 ") was in mGameEndpointMap, endpoint connection group (%" PRIu64 "), erasing from map.\n",
                mGameId, endpoint->getConnectionGroupId());
            mGameEndpointMap.erase(endpointIter);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GAME] Endpoint in game (%" PRIu64 ") was not in mGameEndpointMap, endpoint connection group (%" PRIu64 "), erasing from map.\n",
                mGameId, endpoint->getConnectionGroupId());
        }

        mGameEndpointMemoryPool.free(endpoint);
    }

    void Game::updatePlayerEndpoint(Player &player, const ReplicatedGamePlayer &playerData, bool updateConnectionSlotId )
    {

        if (player.getMeshEndpoint() == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GAME] updatePlayerEndpoint called for player (%" PRIu64 ") in game (%" PRIu64 ") had nullptr endpoint!\n",
                player.getId(), mGameId);
            BlazeAssert(false);
            return;
        }

        if (player.getPlayerEndpoint()->getConnectionGroupId() != playerData.getConnectionGroupId())
        {
            // if the usergroup is updated, update the local player status
            if (player.getConnectionGroupId() != playerData.getConnectionGroupId())
            {
                player.cacheLocalPlayerStatus();
            }

            GameEndpoint *oldEndpoint = player.getPlayerEndpoint();

            // have to check if the new connection group is already in the game! (external reserved players could be claiming reservations from the same console!)
            // if there isn't already an endpoint, we'll create a new one
            GameEndpoint *endpoint = (GameEndpoint*)getMeshEndpointByConnectionGroupId(playerData.getConnectionGroupId());
            if ( endpoint == nullptr)
            {
                BLAZE_SDK_DEBUGF("[GAME] Allocating endpoint for player (%" PRIu64 ") in game (%" PRIu64 "), for connection group (%" PRIu64 ").\n", playerData.getPlayerId(), 
                    mGameId, playerData.getConnectionGroupId());
                endpoint =  new (mGameEndpointMemoryPool.alloc()) GameEndpoint(this, &playerData, mMemGroup);
            }
            else if (updateConnectionSlotId && (endpoint->getConnectionSlotId() != playerData.getConnectionSlotId()))
            {
                // need to update the conn slot id for the existing endpoint
                // this may happen when claiming a reservation or promotion from the queue.
                BLAZE_SDK_DEBUGF("[GAME] Updating connection slot id for endpoint owned by player (%" PRIu64 ") in game (%" PRIu64 "), endpoint connection group (%" PRIu64 " -> %" PRIu64 "): (%u) -> (%u).\n", playerData.getPlayerId(),
                    mGameId, player.getConnectionGroupId(), playerData.getConnectionGroupId(), endpoint->getConnectionSlotId(), playerData.getConnectionSlotId());
                
                // existing endpoint with a new connSlotId may need to have a removal from the active map.
                deactivateEndpoint(endpoint);

                // update the connection slot ID
                endpoint->mConnectionSlotId = playerData.getConnectionSlotId();
            }
            else
            {
                BLAZE_SDK_DEBUGF("[GAME] Endpoint for player (%" PRIu64 ") in game (%" PRIu64 "), was unchanged endpoint connection group (%" PRIu64 ").\n", playerData.getPlayerId(), 
                    mGameId, playerData.getConnectionGroupId());
            }

            player.setPlayerEndpoint(endpoint);

            if(oldEndpoint->getMemberCount() == 0)
            {
                BlazeAssertMsg((oldEndpoint != endpoint), "Updating player endpoint, existing endpoint had member count of 0!");
                cleanUpGameEndpoint(oldEndpoint);
            }

            mGameEndpointMap[playerData.getConnectionGroupId()] = player.getPlayerEndpoint();
        }
        else if (updateConnectionSlotId && (player.getPlayerEndpoint()->getConnectionSlotId() != playerData.getConnectionSlotId()))
        {
            BLAZE_SDK_DEBUGF("[GAME] Updating connection slot id for endpoint owned by player (%" PRIu64 ") in game (%" PRIu64 "), endpoint connection group (%" PRIu64 "): (%u) -> (%u).\n", playerData.getPlayerId(),
                mGameId, playerData.getConnectionGroupId(), player.getConnectionSlotId(), playerData.getConnectionSlotId());

            deactivateEndpoint(player.getPlayerEndpoint());
            player.getPlayerEndpoint()->mConnectionSlotId = playerData.getConnectionSlotId();
            
        }
    }

    void Game::activateEndpoint(GameEndpoint *endpoint)
    {
        BlazeAssertMsg(endpoint != nullptr, "Attempting to activate nullptr endpoint in game!");

        if (endpoint != nullptr)
        {
            if (endpoint->getConnectionGroupId() == 0)
            {
                BLAZE_SDK_DEBUGF("[GAME] tried to activate endpoint for game(%" PRIu64 ")  with connection group id of (0).\n", getId());
            }
            else
            {
                if (mActiveGameEndpoints.find(endpoint->getConnectionSlotId()) != mActiveGameEndpoints.end())
                {
                    BlazeAssertMsg((endpoint == mActiveGameEndpoints.find(endpoint->getConnectionSlotId())->second), "Activating endpoint in connection slot that's already occupied!");
                    return;
                }
                mActiveGameEndpoints[endpoint->getConnectionSlotId()] = endpoint;
            }
        }
    }

    void Game::deactivateEndpoint(GameEndpoint *endpoint)
    {
        BlazeAssertMsg(endpoint != nullptr, "Attempting to activate nullptr endpoint in game!");

        if (endpoint != nullptr)
        {
            if (endpoint->getConnectionGroupId() == 0)
            {
                BLAZE_SDK_DEBUGF("[GAME] tried to deactivate endpoint for game(%" PRIu64 ")  with connection group id of (0).\n", getId()); 
                return;
            }

            EndpointList::iterator activeEndpointIter = mActiveGameEndpoints.find(endpoint->getConnectionSlotId());
            // check that we're pointing at the same endpoint, as players coming from reservation/queue may not have had a valid conn slot id
            if (activeEndpointIter != mActiveGameEndpoints.end() && (activeEndpointIter->second == endpoint))
            {
                mActiveGameEndpoints.erase(activeEndpointIter);
            }
 
        }
    }

    /*! ************************************************************************************************/
    /*! \brief transfer deprecated external session parameters to the ExternalSessionIdentification. For backward compatibility with older servers.
    ***************************************************************************************************/
    void Game::oldToNewExternalSessionIdentificationParams(const char8_t* sessionTemplate, const char8_t* externalSessionName, const char8_t* externalSessionCorrelationId,
        const char8_t* npSessionId, ExternalSessionIdentification& newParam)
    {
        if ((sessionTemplate != nullptr) && (sessionTemplate[0] != '\0') && (newParam.getXone().getTemplateName()[0] == '\0'))
            newParam.getXone().setTemplateName(sessionTemplate);
        if ((externalSessionName != nullptr) && (externalSessionName[0] != '\0') && (newParam.getXone().getSessionName()[0] == '\0'))
            newParam.getXone().setSessionName(externalSessionName);
        if ((externalSessionCorrelationId != nullptr) && (externalSessionCorrelationId[0] != '\0') && (newParam.getXone().getCorrelationId()[0] == '\0'))
            newParam.getXone().setCorrelationId(externalSessionCorrelationId);
        if ((npSessionId != nullptr) && (npSessionId[0] != '\0') && (newParam.getPs4().getNpSessionId()[0] == '\0'))
            newParam.getPs4().setNpSessionId(npSessionId);
    }

    /*! ************************************************************************************************/
    /*! \brief returns true if the all the remote endpoints that the local endpoint could connect to, across platforms already have hosted connectivity.
    ***************************************************************************************************/
    bool Game::allConnectableCrossPlatformEndpointsHaveHostedConnectivity() const
    {
        BlazeAssertMsg(doesLeverageCCS(), "This function should only be called if we leverage CCS for this game.");

        for (uint16_t count = 0; count < getMeshEndpointCount(); ++count)
        {
            if (getMeshEndpointByIndex(count) == getLocalMeshEndpoint())
                continue;

            if (!shouldEndpointsConnect(getMeshEndpointByIndex(count), getLocalMeshEndpoint()))
                continue;

            if (getLocalMeshEndpoint()->getEndpointPlatform() == getMeshEndpointByIndex(count)->getEndpointPlatform())
                continue;

            if (!getMeshEndpointByIndex(count)->isConnectivityHosted())
                return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief returns true if the all the remote endpoints that the local endpoint could connect to already have hosted connectivity.
    ***************************************************************************************************/
    bool Game::allConnectableEndpointsHaveHostedConnectivity() const
    {
        BlazeAssertMsg(doesLeverageCCS(), "This function should only be called if we leverage CCS for this game.");
        
        for (uint16_t count = 0; count < getMeshEndpointCount(); ++count)
        {
            if (getMeshEndpointByIndex(count) == getLocalMeshEndpoint())
                continue;

            if (!shouldEndpointsConnect(getMeshEndpointByIndex(count), getLocalMeshEndpoint()))
                continue;
            
            if (!getMeshEndpointByIndex(count)->isConnectivityHosted())
                return false;
        }

        return true;
    }
    
    /*! ************************************************************************************************/
    // Returns true if we are waiting for this player to establish CCS connectivity:
    /*! ************************************************************************************************/
    bool Game::playerRequiresCCSConnectivity(const Player* joiningPlayer) const
    {
        if (!doesLeverageCCS())
            return false;

        if (joiningPlayer->getMeshEndpoint() == getLocalMeshEndpoint())
            return false;

        if (!shouldEndpointsConnect(joiningPlayer->getMeshEndpoint(), getLocalMeshEndpoint()))
            return false;

        if (getCCSMode() == CCS_MODE_HOSTEDFALLBACK &&
            getLocalMeshEndpoint()->getEndpointPlatform() == joiningPlayer->getMeshEndpoint()->getEndpointPlatform())
            return false;

        return !joiningPlayer->getMeshEndpoint()->isConnectivityHosted();
    }

    bool Game::hasFullCCSConnectivity() const
    {
        if (!doesLeverageCCS())
            return true;

        if (getCCSMode() == CCS_MODE_HOSTEDONLY)
            return allConnectableEndpointsHaveHostedConnectivity();

        if (getCCSMode() == CCS_MODE_HOSTEDFALLBACK)
            return allConnectableCrossPlatformEndpointsHaveHostedConnectivity();

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief determine if the two end points should connect. Note that failure to not being able to connect such endpoints may 
        not be an error as certain connections are allowed to fail. 
    ***************************************************************************************************/
    bool Game::shouldEndpointsConnect(const MeshEndpoint* endpoint1, const MeshEndpoint* endpoint2) const
    {
        if (endpoint1 == nullptr || endpoint2 == nullptr)
            return false;

        if (isConnectionRequired(endpoint1, endpoint2) || (getVoipTopology() == VOIP_PEER_TO_PEER))
            return true;

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief determine if the connection between two endpoints is necessary. For example, in CSPH games, the connection against host is the only required connection.
        A P2P voip connection against a non-host player is optional.
    ***************************************************************************************************/
    bool Game::isConnectionRequired(const MeshEndpoint* endpoint1, const MeshEndpoint* endpoint2) const
    {
        if (endpoint1 == nullptr || endpoint2 == nullptr)
            return false;

        switch(mNetworkTopology)
        {
        case CLIENT_SERVER_PEER_HOSTED:
            return ((endpoint1 == getTopologyHostMeshEndpoint()) || (endpoint2 == getTopologyHostMeshEndpoint()));
        
        case CLIENT_SERVER_DEDICATED:
            return ((endpoint1 == getDedicatedServerHostMeshEndpoint()) || (endpoint2 == getDedicatedServerHostMeshEndpoint()));
        
        case PEER_TO_PEER_FULL_MESH:
            return true;

        case NETWORK_DISABLED:
            return false;

        default:
            return true;
        }
    }


    bool Game::shouldSendMeshConnectionUpdateForVoip(Blaze::ConnectionGroupId targetConnGroupId) const
    {
        const MeshEndpoint* targetEndpoint = getMeshEndpointByConnectionGroupId(targetConnGroupId);
        const MeshEndpoint* localEndpoint = getLocalMeshEndpoint();

        // VOIP_DEDICATED_SERVER: DirtySDK may send us an extra/ignorable notification that appears to be directly between two players, even
        // though their actual connection is via the dedicated server host. No need to send BlazeServer the mesh connection update for this case
        if (getVoipTopology() == Blaze::VOIP_DEDICATED_SERVER)
        {
            const MeshEndpoint* dedSrvEndpoint = getDedicatedServerHostMeshEndpoint();
            if ((targetEndpoint != dedSrvEndpoint) && (localEndpoint != dedSrvEndpoint))
                return false;
        }

        //Send the mesh update for voip notifications only if the game connection between these two end points is not required. If it is, we track the connectivity via that notification. 
        return !isConnectionRequired(targetEndpoint, localEndpoint);
    }

    /*! ************************************************************************************************/
    /*! \brief returns true if this game can use CCS. This utility function also helps to guard against configuration mistakes.
    ***************************************************************************************************/
    bool Game::doesLeverageCCS() const
    {
        if ((getCCSMode() > CCS_MODE_PEERONLY) && !isDedicatedServerTopology()
            && !(getNetworkTopology() == NETWORK_DISABLED && getVoipTopology() == VOIP_DISABLED))
        {
            return true;
        }

        return false;
    }


    JobId Game::updateExternalSessionImage(const Blaze::Ps4NpSessionImage& image, const UpdateExternalSessionImageJobCb &titleCb)
    {
        UpdateExternalSessionImageRequest request;
        request.setGameId(mGameId);
        request.getCustomImage().setData(image.getData(), image.getCount());

        JobId jobId = getAdminGameManagerComponent()->updateExternalSessionImage(request, MakeFunctor(this, &Game::internalUpdateSessionImageCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    void Game::internalUpdateSessionImageCb(const ExternalSessionErrorInfo *errorInfo, BlazeError error, JobId id, UpdateExternalSessionImageJobCb titleCb)
    {
        titleCb(error, this, errorInfo, id);
    }

    JobId Game::updateExternalSessionStatus(const ExternalSessionStatus& status, const UpdateExternalSessionStatusJobCb &titleCb)
    {
        UpdateExternalSessionStatusRequest request;
        request.setGameId(mGameId);
        status.copyInto(request.getExternalSessionStatus());

        JobId jobId = getAdminGameManagerComponent()->updateExternalSessionStatus(request, MakeFunctor(this, &Game::internalUpdateSessionStatusCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi.getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    void Game::internalUpdateSessionStatusCb(BlazeError error, JobId id, UpdateExternalSessionStatusJobCb titleCb)
    {
        titleCb(error, this, id);
    }


    //
    // Default GameListeners. 
    // We are now providing default game listeners for our non pure virtual listeners to prevent API strain when
    // we modify/add our listeners.
    //

    void GameListener::onProcessGameQueue(Blaze::GameManager::Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for notification to process game(%" PRIu64 ") queue.  Override this listener method to provide a specific implementation.\n", 
            game->getId());
    }  /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onGameNameChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::GameName newName)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for notification to change game(%" PRIu64 ") name.  Override this listener method to provide a specific implementation.\n",
            game->getId());
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onPlayerJoinedFromReservation(Blaze::GameManager::Player *player)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for notification for game that player(%" PRId64 ") claimed reservation.  Override this listener method to provide a specific implementation.\n",
            player->getId());
    }  /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onPlayerDemotedToQueue(Blaze::GameManager::Player *player)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for notification for game that local player(%" PRId64 ") was demoted to queue.  Override this listener method to provide a specific implementation.\n",
            player->getId());
    }  /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/



    void GameListener::onVoipConnected(Blaze::GameManager::Game *game, Blaze::GameManager::Game::PlayerVector *connectedPlayersVector)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onVoipConnected notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/


    void GameListener::onVoipConnectionLost(Blaze::GameManager::Game *game, Blaze::GameManager::Game::PlayerVector *disconnectedPlayersVector)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onVoipConnectionLost notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onGameGroupInitialized(Blaze::GameManager::Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onGameGroupInitialized notification for game group(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/
        
    void GameListener::onGameModRegisterUpdated(Blaze::GameManager::Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onGameModRegisterUpdated notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onGameEntryCriteriaUpdated(Blaze::GameManager::Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onGameEntryCriteriaUpdated notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/
        
    void GameListener::onPlayerRoleUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::RoleName previousRoleName)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onPlayerRoleUpdated notification for player(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            player->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/
        
    void GameListener::onPlayerSlotUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::SlotType previousSlotType)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onPlayerSlotUpdated notification for player(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            player->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onPlayerStateChanged(Blaze::GameManager::Player *player, Blaze::GameManager::PlayerState previousPlayerState)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onPlayerStateChanged notification for player(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            player->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onNpSessionIdAvailable(Blaze::GameManager::Game *game, const char8_t* npSessionId)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onNpSessionIdAvailable notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onActivePresenceUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::PresenceActivityStatus status, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onActivePresenceUpdated notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onResolveGameMembership(Blaze::GameManager::Game *game, Blaze::GameManager::Game *newGame)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onResolveGameMembership notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onUnresponsive notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    void GameListener::onBackFromUnresponsive(Blaze::GameManager::Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameListener] Default handler for onBackFromUnresponsive notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

} //GameManager
} //Blaze
