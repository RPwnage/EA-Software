/*! ************************************************************************************************/
/*!
    \file player.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/gamemanager/player.h"
#include "BlazeSDK/gamemanager/gameendpoint.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Construct a new player attached to the supplied game, with the supplied data.
    
        \param[in] game                  the game the player is associated with
        \param[in] endpoint              the mesh endpoint for this player. Will be shared with other players in the same connection group.
        \param[in] replicatedPlayerData  the player's initial settings (attributes, custom data blob, etc)
        \param[in] memGroupId            mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    Player::Player(Game *game, GameEndpoint *endpoint, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId)
        :    PlayerBase(game->getGameManagerAPI(), replicatedPlayerData, memGroupId),
            mGame(game),
            mPlayerEndpoint(endpoint),
            mSlotId(replicatedPlayerData->getSlotId()),
            mDirtySockUserIndex(replicatedPlayerData->getDirtySockUserIndex()),
            mTitleContextData(nullptr),
            mJoinedGameTimestamp(replicatedPlayerData->getJoinedGameTimestamp()),
            mPlayerSettings(replicatedPlayerData->getPlayerSettings()),
            mScenarioName(replicatedPlayerData->getScenarioName())
    {
        cacheLocalPlayerStatus();

        if (mPlayerEndpoint != nullptr)
        {
            mPlayerEndpoint->incMemberCount();

            if (mPlayerSettings.getHasVoipDisabled())
            {
                mPlayerEndpoint->setVoipDisabled(true);
            }
        }
        else
        {
            BlazeAssertMsg(false, "Player object created with nullptr GameEndpoint!");
        }
    }

    Player::~Player()
    {
        // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
        // to call the callback.  Since the object is being deleted, we go with the remove.
        mGameManagerApi->getBlazeHub()->getScheduler()->removeByAssociatedObject(this);

        // the game object is responsible for cleaning up the endpoint after deleting the player
        if (mPlayerEndpoint != nullptr)
        {
            mPlayerEndpoint->decMemberCount();
            mPlayerEndpoint = nullptr;
        }
        else
        {
            BlazeAssertMsg(false, "Player object had nullptr GameEndpoint during destruction!");
        }
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this is the local player for this gameManagerApi instance.
        \return    True if this is the local player.
    **************************************************************************************************************/
    bool Player::isLocalPlayer() const
    {
        return mIsLocal;
    }

    /*! **********************************************************************************************************/
    /*! \brief Lookup from the Usermanager if the user is found (ie. the user is local), and save to the cache.
    **************************************************************************************************************/
    void Player::cacheLocalPlayerStatus()
    {
        bool isLocal = false;

        UserManager::UserManager *userManager = mGameManagerApi->getUserManager();
        for (uint32_t ui=0; ui < mGameManagerApi->getBlazeHub()->getNumUsers(); ++ui)
        {
            if (userManager->getLocalUser(ui) != nullptr)
            {
                if (mUser->getId() == userManager->getLocalUser(ui)->getId())
                {
                    isLocal = true;
                    break;
                }
            }
        }

        mIsLocal = isLocal;
    }

    bool Player::isPrimaryLocalPlayer() const
    {
        UserManager::UserManager *userManager = mGameManagerApi->getUserManager();
        UserManager::LocalUser* user = userManager->getPrimaryLocalUser();
        if (user != nullptr && user->getId() == mUser->getId())
        {
            return true;
        }

        return false;
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this is the network host for this ReplicatedGameData instance.
        \return    True if this is the network host.
    **************************************************************************************************************/
    bool Player::isHost() const
    {
        if ( mGame == nullptr )
        {
            return false;
        }

        if (mGame->getTopologyHostMeshEndpoint() == nullptr)
        {
            return false;
        }

        return (mGame->getTopologyHostMeshEndpoint() == mPlayerEndpoint);
    }

    bool Player::isPlatformHost() const
    {
        if ( mGame == nullptr )
            return false;

        return ( mGame->getPlatformHostPlayer() != nullptr) ? (mGame->getPlatformHostPlayer()->getId() == mId) : false;
    }

    void Player::setPlayerEndpoint(GameEndpoint *newEndpoint)
    {
        if (newEndpoint == mPlayerEndpoint)
        {
            // no-op
            return;
        }
        mPlayerEndpoint->decMemberCount();
        mPlayerEndpoint = newEndpoint;
        mPlayerEndpoint->incMemberCount();

        if (mPlayerSettings.getHasVoipDisabled())
        {
            mPlayerEndpoint->setVoipDisabled(true);
        }
    }
    
    /*! **********************************************************************************************************/
    /*! \brief Returns the player's platform dependent network address.
    **************************************************************************************************************/
    const NetworkAddress* Player::getNetworkAddress() const 
    { 
        return mPlayerEndpoint->getNetworkAddress(); 
    }

        /*! ************************************************************************************************/
    /*! \brief returns the connection group id.  A unique id per connection to the blaze server
        which will be equal for multiple players/members connected to Blaze on the same connection.
    ***************************************************************************************************/
    uint64_t Player::getConnectionGroupId() const 
    { 
        return mPlayerEndpoint->getConnectionGroupId(); 
    }

    /*! ************************************************************************************************/
    /*! \brief returns the slot id of the connection group
    ***************************************************************************************************/
    uint8_t Player::getConnectionSlotId() const 
    { 
        return mPlayerEndpoint->getConnectionSlotId(); 
    }

    /*! ************************************************************************************************/
    /*! \brief Returns the mesh endpoint this player is attached to. Users on the same machine will share a MeshEndpoint object.
    ***************************************************************************************************/
    const MeshEndpoint* Player::getMeshEndpoint() const 
    { 
        return mPlayerEndpoint; 
    }


    template <typename callbackType>
    JobId JobCbHelper(const char8_t* jobName, const callbackType& titleCb, BlazeError err, Player* thisPlayer)
    {
        JobScheduler* scheduler = thisPlayer->getGameManagerAPI()->getBlazeHub()->getScheduler();
        JobId jobId = scheduler->reserveJobId();
        scheduler->scheduleFunctor(jobName, titleCb, err, thisPlayer, jobId, thisPlayer, 0, jobId);
        Job::addTitleCbAssociatedObject(scheduler, jobId, titleCb);
        return jobId;
    }

    void Player::helperCallback(BlazeError err, Player* player, JobId jobId)
    {
        Job* job = mGameManagerApi->getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const Functor2<BlazeError, Player *>* titleCb = (const Functor2<BlazeError, Player *>*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err, player);
        }
    }

#define BACK_COMPAT_WRAPPER(funcName, funcParamDefs, newCbType, funcParams) \
    JobId Player::funcName funcParamDefs\
    {\
        newCbType helperCb = MakeFunctor(this, &Player::helperCallback);\
        JobId jobId = funcName funcParams;\
        Job::addTitleCbAssociatedObject(mGameManagerApi->getBlazeHub()->getScheduler(), jobId, titleCb);\
        return jobId;\
    }

    BACK_COMPAT_WRAPPER(setPlayerAttributeValue, (const char8_t* attributeName, const char8_t* attributeValue, const ChangePlayerAttributeCb &titleCb), ChangePlayerAttributeJobCb, (attributeName, attributeValue, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerAttributeMap, (const Collections::AttributeMap* changingAttributes, const ChangePlayerAttributeCb &titleCb), ChangePlayerAttributeJobCb, (changingAttributes, helperCb));
    BACK_COMPAT_WRAPPER(setCustomData, (const EA::TDF::TdfBlob* cBlob, const ChangePlayerCustomDataCb &titleCb), ChangePlayerCustomDataJobCb, (cBlob, helperCb));
    BACK_COMPAT_WRAPPER(setCustomData, (const uint8_t* buffer, uint32_t bufferSize, const ChangePlayerCustomDataCb &titleCb), ChangePlayerCustomDataJobCb, (buffer, bufferSize, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerTeamIndex, (TeamIndex teamIndex, const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (teamIndex, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerRole, (const RoleName& roleName, const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (roleName, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerTeamIndexAndRole, (TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (teamIndex, roleName, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerSlotType, (SlotType slotType, const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (slotType, helperCb));
    BACK_COMPAT_WRAPPER(setPlayerSlotTypeTeamAndRole, (SlotType slotType, TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (slotType, teamIndex, roleName, helperCb));
    BACK_COMPAT_WRAPPER(promoteSpectator, (TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (teamIndex, roleName, helperCb));
    BACK_COMPAT_WRAPPER(becomeSpectator, (const ChangePlayerTeamAndRoleCb &titleCb), ChangePlayerTeamAndRoleJobCb, (helperCb));
#undef BACK_COMPAT_WRAPPER

    /*! ************************************************************************************************/
    /*! \brief creating or updating a single player attribute (sends RPC to server)

        If no attribute exists with the supplied name, one is created; otherwise the existing value
            is updated.

        Note: use setPlayerAttributeMap to update multiple attributes in a single RPC.

        Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
        Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

        \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
        \param[in] attributeValue the value to set the attribute to; must be < 256 characters
        \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
        \return JobId for pending action.
    ***************************************************************************************************/
    JobId Player::setPlayerAttributeValue(const char8_t* attributeName, const char8_t* attributeValue, const ChangePlayerAttributeJobCb &titleCb)
    {
        // client side validation (return err ok if value(s) are already set)
        Collections::AttributeMap::const_iterator attribMapIter = mPlayerAttributeMap.find(attributeName);
        if ( attribMapIter != mPlayerAttributeMap.end() )
        {
            if (blaze_strcmp(attribMapIter->second.c_str(), attributeValue) == 0)
            {
                return JobCbHelper("setPlayerAttributeValueCb", titleCb, ERR_OK, this);
            }
        }

        // setup rpc request
        SetPlayerAttributesRequest setPlayerAttribRequest;
        setPlayerAttribRequest.setGameId(mGame->getId());
        setPlayerAttribRequest.setPlayerId(mId);
        setPlayerAttribRequest.getPlayerAttributes().insert(eastl::make_pair(attributeName, attributeValue));

        // send request, passing title's callback as the payload            
        JobId jobId = mGame->getPlayerGameManagerComponent(mId)->setPlayerAttributes(setPlayerAttribRequest, 
            MakeFunctor(this, &Player::internalSetPlayerAttributeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi->getBlazeHub()->getScheduler(), jobId, titleCb); 
        return jobId;
    }

    /*! **********************************************************************************************************/
    /*! \brief  create/update multiple attributes for this player in a single RPC.  NOTE: the server merges existing
            attributes with the supplied attributeMap (so omitted attributes retain their previous values).
        
        If no attribute exists with the supplied name, one is created; otherwise the existing value
        is updated.

        Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
        Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

        \param[in] changingAttributes an attributeMap containing attributes to create/update on the server.
        \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
        \return JobId for pending action.        
    **************************************************************************************************************/
    JobId Player::setPlayerAttributeMap(const Collections::AttributeMap *changingAttributes, const ChangePlayerAttributeJobCb &titleCb)
    {
        // Well, it's kind of too expensive for us to go through the existing attributes
        // map and new attributes map to check if they are same, so we decided to just do
        // simple check here, as long as it's a valid non empty entry we will pass on to the server
        if (!changingAttributes || changingAttributes->empty())
        {
            return JobCbHelper("setPlayerAttributeMapCb", titleCb, ERR_OK, this);
        }

        // setup rpc request
        SetPlayerAttributesRequest setPlayerAttribRequest;
        setPlayerAttribRequest.setGameId(mGame->getId());
        setPlayerAttribRequest.setPlayerId(mId);
        setPlayerAttribRequest.getPlayerAttributes().insert(changingAttributes->begin(), changingAttributes->end());

        // send request, passing title's callback as the payload            
        JobId jobId = mGame->getPlayerGameManagerComponent(mId)->setPlayerAttributes(setPlayerAttribRequest, 
            MakeFunctor(this, &Player::internalSetPlayerAttributeCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi->getBlazeHub()->getScheduler(), jobId, titleCb);    
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal handler for the setPlayerAttribute RPC callback.
    ***************************************************************************************************/
    void Player::internalSetPlayerAttributeCb(BlazeError error, JobId id, ChangePlayerAttributeJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief dispatched on async attrib update
    ***************************************************************************************************/
    void Player::onNotifyPlayerAttributeChanged(const Collections::AttributeMap *newAttributeMap)
    {
        BLAZE_SDK_DEBUGF("Player \"%s\" (id: %" PRId64 "): updating %d Player attribute(s).\n", getName(), mId, (int32_t)newAttributeMap->size());
        Collections::AttributeMap::const_iterator newAttribMapIter = newAttributeMap->begin();
        Collections::AttributeMap::const_iterator end = newAttributeMap->end();
        for ( ; newAttribMapIter != end; ++newAttribMapIter )
        {
            // Note: initially, I was using "existingMap.insert(new.begin(), new.end());",
            //  but this wouldn't update values if a key already existed for it.
            const char8_t *attribName = newAttribMapIter->first.c_str();
            const char8_t *attribValue = newAttribMapIter->second.c_str();
            mPlayerAttributeMap[attribName] = attribValue;
        }

        mGame->mDispatcher.dispatch("onPlayerAttributeUpdated", &GameListener::onPlayerAttributeUpdated, this, newAttributeMap);
    }

    /*! ************************************************************************************************/
    /*! \brief dispatched on async player state changed
    ***************************************************************************************************/
    void Player::onNotifyGamePlayerStateChanged(const PlayerState state)
    {
        PlayerState previousState = mPlayerState;
        mPlayerState = state;
        mGame->mDispatcher.dispatch("onPlayerStateChanged", &GameListener::onPlayerStateChanged, this, previousState);
    }

    /*! ************************************************************************************************/
    /*! \brief set a player's team index(sends RPC to server)

    \param[in] teamIndex the teamIndex to set for the player
    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setPlayerTeamIndex(TeamIndex teamIndex, const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        return setPlayerTeamIndexAndRole(teamIndex, mRoleName, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief set a player's team index and role(sends RPC to server)

    \param[in] roleName the RoleName to set for the player
    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setPlayerRole(const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        return setPlayerTeamIndexAndRole(mTeamIndex, roleName, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief set a player's team index and role(sends RPC to server)

    \param[in] teamIndex the teamIndex to set for the player
    \param[in] roleName the RoleName to set for the player
    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setPlayerTeamIndexAndRole(TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        return setPlayerSlotTypeTeamAndRole(mSlotType, teamIndex, roleName, titleCb);
    }


    /*! ************************************************************************************************/
    /*! \brief set a player's slot type, team index and role(sends RPC to server)
           
        NOTE: If this function is used to change from a spectator to a participant, team index 0, and the default role name will be used,
                Call setPlayerSlotTypeTeamAndRole or promoteSpectator if you want to 

    \param[in] slotType the slotType to set for the player
    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setPlayerSlotType(SlotType slotType, const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        // Call setPlayerSlotTypeTeamAndRole or promoteSpectator instead, if converting to a participant:
        if (isParticipantSlot(slotType) && isSpectator())
        {
            BLAZE_SDK_DEBUGF("[GAME] setPlayerSlotType cannot be used when moving from a participant slot to a spectator slot. Call setPlayerSlotTypeTeamAndRole or promoteSpectator instead. Player(%" PRId64 ":%s)\n", getId(), getName());
            return JobCbHelper("setPlayerSlotTypeCb", titleCb, ERR_SYSTEM, this);
        }

        return setPlayerSlotTypeTeamAndRole(slotType, mTeamIndex, mRoleName, titleCb);
    }


    /*! ************************************************************************************************/
    /*! \brief set a player's slot type, team index and role(sends RPC to server)

    \param[in] slotType the slotType to set for the player
    \param[in] teamIndex the teamIndex to set for the player - Only needed when promoting a spectator
    \param[in] roleName the RoleName to set for the player   - Only needed when promoting a spectator
    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setPlayerSlotTypeTeamAndRole(SlotType slotType, TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        if (isParticipantSlot(slotType))
        {
            if ((teamIndex >= mGame->getTeamCount()) && !((teamIndex == UNSPECIFIED_TEAM_INDEX) && isQueued()))
            {
                return JobCbHelper("setPlayerSlotTypeTeamAndRoleCb", titleCb, GAMEMANAGER_ERR_TEAM_NOT_ALLOWED, this);
            }

            // Indicates that the role name is invalid in this game
            if (mGame->getRoleCapacity(roleName) == 0)
            {
                return JobCbHelper("setPlayerSlotTypeTeamAndRoleCb", titleCb, GAMEMANAGER_ERR_ROLE_NOT_ALLOWED, this);
            }
        }

        // client side validation (return err ok if new team is the same as current team, and the role is unchanged)
        if ( mTeamIndex == teamIndex && (blaze_stricmp(roleName, mRoleName) == 0) && slotType == mSlotType )
        {
            return JobCbHelper("setPlayerSlotTypeTeamAndRoleCb", titleCb, ERR_OK, this);
        }

        

        // setup rpc request
        SwapPlayersRequest swapPlayersRequest;
        swapPlayersRequest.setGameId(mGame->getId());
        SwapPlayerData* playerData = swapPlayersRequest.getSwapPlayers().allocate_element();
        playerData->setPlayerId(mId);
        
        // Spectators don't need team/role info:
        if (isSpectatorSlot(slotType))
        {
            playerData->setTeamIndex(UNSPECIFIED_TEAM_INDEX);
            playerData->setRoleName(PLAYER_ROLE_NAME_DEFAULT);
        }
        else
        {
            playerData->setTeamIndex(teamIndex);
            playerData->setRoleName(roleName);
        }
        playerData->setSlotType(slotType);
        swapPlayersRequest.getSwapPlayers().push_back(playerData);

        // send request, passing title's callback as the payload            
        JobId jobId = mGame->getPlayerGameManagerComponent(mId)->swapPlayers(swapPlayersRequest, 
            MakeFunctor(this, &Player::internalSetPlayerTeamCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi->getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }


    /*! ************************************************************************************************/
    /*! \brief Helper function that converts a spectator into a participant with the same public/private setting. 
                Identical to setPlayerTeamIndexAndRole if the player is already a participant. 

    \param[in] teamIndex the teamIndex to set for the player
    \param[in] roleName the RoleName to set for the player
    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::promoteSpectator(TeamIndex teamIndex, const RoleName& roleName, const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        SlotType slotType = mSlotType;
        if (isParticipantSlot(slotType))
        {
            BLAZE_SDK_DEBUGF("Player \"%s\" (id: %" PRId64 "): attempting to promoterSpectator when not a spectator.\n", getName(), mId);
            return JobCbHelper("promoteSpectatorCb", titleCb, ERR_OK, this);
        }

        if (slotType == SLOT_PUBLIC_SPECTATOR)
            slotType = SLOT_PUBLIC_PARTICIPANT;
        if (slotType == SLOT_PRIVATE_SPECTATOR)
            slotType = SLOT_PRIVATE_PARTICIPANT;

        return setPlayerSlotTypeTeamAndRole(slotType, teamIndex, roleName, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief Helper function that transition from participant to spectator with the same public/private setting.
                Identical to setPlayerSlotType if the player is already a spectator. 

    \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::becomeSpectator(const ChangePlayerTeamAndRoleJobCb &titleCb)
    {
        SlotType slotType = mSlotType;
        if (isSpectatorSlot(slotType)) 
        {
            BLAZE_SDK_DEBUGF("Player \"%s\" (id: %" PRId64 "): attempting to becomeSpectator when not a participant.\n", getName(), mId);
            return JobCbHelper("becomeSpectatorCb", titleCb, ERR_OK, this);
        }

        if (slotType == SLOT_PUBLIC_PARTICIPANT)
            slotType = SLOT_PUBLIC_SPECTATOR;
        if (slotType == SLOT_PRIVATE_PARTICIPANT)
            slotType = SLOT_PRIVATE_SPECTATOR;

        return setPlayerSlotType(slotType, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief internal handler for the setPlayerAttribute RPC callback.
    ***************************************************************************************************/
    void Player::internalSetPlayerTeamCb(const SwapPlayersErrorInfo *swapPlayersError, BlazeError error, JobId id, ChangePlayerTeamAndRoleJobCb titleCb)
    {
        // don't bother with the SwapPlayersErrorInfo since this was a single-user request, the cause of the failure is explicit from the error code
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief dispatched on async player team changed
    ***************************************************************************************************/
    void Player::onNotifyGamePlayerTeamChanged( const TeamIndex newTeamIndex )
    {
        TeamIndex previousTeamIndex = mTeamIndex;
        mTeamIndex = newTeamIndex;
        mGame->mDispatcher.dispatch("onPlayerTeamUpdated", &GameListener::onPlayerTeamUpdated, this, previousTeamIndex);
    }

    /*! ************************************************************************************************/
    /*! \brief dispatched on async player role changed
    ***************************************************************************************************/
    void Player::onNotifyGamePlayerRoleChanged(const RoleName& newPlayerRole)
    {
        RoleName previousRoleName = mRoleName;
        mRoleName.set(newPlayerRole);
        mGame->mDispatcher.dispatch("onPlayerRoleUpdated", &GameListener::onPlayerRoleUpdated, this, previousRoleName);
    }

    /*! ************************************************************************************************/
    /*! \brief dispatched on async player role changed
    ***************************************************************************************************/
    void Player::onNotifyGamePlayerSlotChanged(const SlotType& newSlotType)
    {
        SlotType previousSlotType = mSlotType;
        mSlotType = newSlotType;
        mGame->mDispatcher.dispatch("onPlayerSlotUpdated", &GameListener::onPlayerSlotUpdated, this, previousSlotType);
    }
    

    /*! ************************************************************************************************/
    /*! \brief init/update custom data blob for this player (sends RPC to server)

        \param[in] cBlob a blob containing player custom data blob to update on the server
        \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setCustomData(const EA::TDF::TdfBlob* cBlob, const ChangePlayerCustomDataJobCb &titleCb)
    {
        return setCustomData(cBlob->getData(), cBlob->getSize(), titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief init/update custom data blob for this player (sends RPC to server)

        \param[in] buffer a buffer containing raw player custom data to init/update on the server
        \param[in] bufferSize size of the raw player custom data to init/update on the server
        \param[in] titleCb this callback is dispatched when we get the RPC result from the server.
    ***************************************************************************************************/
    JobId Player::setCustomData(const uint8_t* buffer, uint32_t bufferSize, const ChangePlayerCustomDataJobCb &titleCb)
    {
        SetPlayerCustomDataRequest setPlayerCustomDataRequest;
        setPlayerCustomDataRequest.setGameId(mGame->getId());
        setPlayerCustomDataRequest.setPlayerId(mId);
        setPlayerCustomDataRequest.getCustomData().setData(buffer, bufferSize);

        // send request, passing title's callback as the payload
        JobId jobId = mGame->getPlayerGameManagerComponent(mId)->setPlayerCustomData(setPlayerCustomDataRequest, 
                MakeFunctor(this, &Player::internalSetPlayerCustomDataCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameManagerApi->getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal handler for the SetPlayerCustomData RPC callback.
    ***************************************************************************************************/
    void Player::internalSetPlayerCustomDataCb(BlazeError error, JobId id, ChangePlayerCustomDataJobCb titleCb)
    {
        titleCb(error, this, id);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief called upon asycn customData changes for this player
    ***************************************************************************************************/
    void Player::onNotifyPlayerCustomDataChanged(const EA::TDF::TdfBlob *customData)
    {
        BLAZE_SDK_DEBUGF("Player \"%s\" (id: %" PRId64 "): updating Player custom data.\n", getName(), mId);
        customData->copyInto(mCustomData);
        mGame->mDispatcher.dispatch("onPlayerCustomDataUpdated", &GameListener::onPlayerCustomDataUpdated, this);
    }

    /*! ************************************************************************************************/
    /*! \brief async notification that the players queue index has changed.
    
        \param[in] queueIndex - the new queue index
    ***************************************************************************************************/
    void Player::onNotifyQueueChanged(const SlotId queueIndex)
    {
        mSlotId = queueIndex;
    }

    /*! ************************************************************************************************/
    /*! \brief return the player's index in the queue
    ***************************************************************************************************/
    uint16_t Player::getQueueIndex() const
    {
        return mGame->getQueuedPlayerIndex(this);
    }

    /*! ****************************************************************************/
    /*! \brief Gets the QOS statistics for a player

        \param qosData a struct to contain the qos values
        \return a bool true, if the call was successful
    ********************************************************************************/
    bool Player::getQosStatisticsForPlayer(Blaze::BlazeNetworkAdapter::MeshEndpointQos &qosData, bool bInitialQosStats) const
    {
        return mGameManagerApi->getNetworkAdapter()->getQosStatisticsForEndpoint(this->getMeshEndpoint(), qosData, bInitialQosStats);
    }


    bool Player::isQueued() const
    {
        return ((mPlayerState == QUEUED) || (mGame->getQueuedPlayerById(mId) != nullptr));
    }
} // GameManager
} // Blaze
