/*! ************************************************************************************************/
/*!
    \file gamebase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/gamemanager/gamebase.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Init the GameBase data from the supplied replicatedGameData obj.
    
        \param[in] replicatedGameData the obj to init the game's base data from
        \param[in] memGroupId                 mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    GameBase::GameBase(const ReplicatedGameData *replicatedGameData, MemoryGroupId memGroupId)
        :   mName(nullptr, memGroupId),
            mPresenceMode(PRESENCE_MODE_STANDARD),
            mGameModRegister(0),
            mPlayerCapacity(memGroupId, MEM_NAME(memGroupId, "GameBase::mPlayerCapacity")),
            mPlayerSlotCounts(memGroupId, MEM_NAME(memGroupId, "GameBase::mPlayerSlotCounts")),
            mTeamIndexMap(memGroupId, MEM_NAME(memGroupId, "GameBase::mTeamIndexMap")),
            mTeamInfoVector(memGroupId, MEM_NAME(memGroupId, "GameBase::mTeamInfoVector")),
            mRoleInformation(memGroupId),
            mMaxPlayerCapacity(0),
            mMinPlayerCapacity(0),
            mQueueCapacity(0),
            mGameProtocolVersionString(nullptr, memGroupId),
            mGameAttributeMap(memGroupId, MEM_NAME(memGroupId, "GameBase::mGameAttributeMap")),
            mDedicatedServerAttributeMap(memGroupId, MEM_NAME(memGroupId, "GameBase::mDedicatedServerAttributeMap")),
            mClientPlatformList(memGroupId, MEM_NAME(memGroupId, "GameBase::mClientPlatformList")),
            mIsCrossplayEnabled(replicatedGameData->getIsCrossplayEnabled()),
            mAdminList(memGroupId, MEM_NAME(memGroupId, "GameBase::mAdminList")),
            mEntryCriteria(memGroupId, MEM_NAME(memGroupId, "GameBase::mEntryCriteria")),
            mPingSite(nullptr, memGroupId),
            mNetworkTopology(CLIENT_SERVER_PEER_HOSTED),
            mGameType(GAME_TYPE_GAMESESSION)
    {
        // Only TDF will default-resize these vectors to MAX_SLOT_TYPE; therefore, do it here.
        mPlayerCapacity.assign(Blaze::GameManager::MAX_SLOT_TYPE, 0);
        mPlayerSlotCounts.assign(Blaze::GameManager::MAX_SLOT_TYPE, 0);
        initGameBaseData(replicatedGameData);
    }

    /*! ************************************************************************************************/
    /*! \brief Init the GameBase data from the supplied gameBrowserGameData obj.
    
        \param[in] memGroupId mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    GameBase::GameBase(const GameBrowserGameData &gameBrowserGameData, MemoryGroupId memGroupId)
        :   mName(nullptr, memGroupId),
            mPresenceMode(PRESENCE_MODE_STANDARD),
            mGameModRegister(0),
            mPlayerCapacity(memGroupId, MEM_NAME(memGroupId, "GameBase::mPlayerCapacity")),
            mPlayerSlotCounts(memGroupId, MEM_NAME(memGroupId, "GameBase::mPlayerSlotCounts")),
            mTeamIndexMap(memGroupId, MEM_NAME(memGroupId, "GameBase::mTeamIndexMap")),
            mTeamInfoVector(memGroupId, MEM_NAME(memGroupId, "GameBase::mTeamInfoVector")),
            mRoleInformation(memGroupId),
            mMaxPlayerCapacity(0),
            mMinPlayerCapacity(0),
            mQueueCapacity(0),
            mGameProtocolVersionString(nullptr, memGroupId),
            mGameAttributeMap(memGroupId, MEM_NAME(memGroupId, "GameBase::mGameAttributeMap")),
            mDedicatedServerAttributeMap(memGroupId, MEM_NAME(memGroupId, "GameBase::mDedicatedServerAttributeMap")),
            mClientPlatformList(memGroupId, MEM_NAME(memGroupId, "GameBase::mClientPlatformList")),
            mIsCrossplayEnabled(gameBrowserGameData.getIsCrossplayEnabled()),
            mAdminList(memGroupId, MEM_NAME(memGroupId, "GameBase::mAdminList")),
            mEntryCriteria(memGroupId, MEM_NAME(memGroupId, "GameBase::mEntryCriteria")),
            mPingSite(nullptr, memGroupId),
            mNetworkTopology(CLIENT_SERVER_PEER_HOSTED),
            mGameType(GAME_TYPE_GAMESESSION)
    {
        // Only TDF will default-resize these vectors to MAX_SLOT_TYPE; therefore, do it here.
        mPlayerCapacity.assign(Blaze::GameManager::MAX_SLOT_TYPE, 0);
        mPlayerSlotCounts.assign(Blaze::GameManager::MAX_SLOT_TYPE, 0);
        initGameBaseData(gameBrowserGameData);
    }


    /*! ************************************************************************************************/
    /*! \brief initialize the base game's data from the supplied replicatedGameData object.  All old values are wiped.
        \param[in] replicatedGameData the new data for the game
    ***************************************************************************************************/
    void GameBase::initGameBaseData(const ReplicatedGameData *replicatedGameData)
    {
        mGameId = replicatedGameData->getGameId();
        mState = replicatedGameData->getGameState();

        mTopologyHostId = replicatedGameData->getTopologyHostInfo().getPlayerId();


        mGameProtocolVersionString = replicatedGameData->getGameProtocolVersionString();
        mName = replicatedGameData->getGameName();
        mGameSettings = replicatedGameData->getGameSettings();
        mPresenceMode = replicatedGameData->getPresenceMode();

        mGameModRegister = replicatedGameData->getGameModRegister();
        
        mVoipTopology = replicatedGameData->getVoipNetwork();

        replicatedGameData->getSlotCapacities().copyInto(mPlayerCapacity);
        // Note: no need to init the mPlayerSlotCounts - it's reserved & set to zero automatically.
        //      and will be init'd once the Game inits its roster
        mMaxPlayerCapacity = replicatedGameData->getMaxPlayerCapacity();
        mMinPlayerCapacity = replicatedGameData->getMinPlayerCapacity();

        mQueueCapacity = replicatedGameData->getQueueCapacity();

        replicatedGameData->getRoleInformation().copyInto(mRoleInformation);
        
        const TeamIdVector &teamIds = replicatedGameData->getTeamIds();
        mTeamInfoVector.clear(); // need to make sure old vector contents are wiped if updating ReplicatedGameData
        mTeamInfoVector.reserve(teamIds.size());
        for(TeamIndex i = 0; i < teamIds.size(); ++i)
        {
            mTeamInfoVector.push_back();
            TeamInfo &newTeamInfo = mTeamInfoVector.back();
            newTeamInfo.mTeamId = teamIds[i];
            newTeamInfo.mTeamSize = 0;
            // each TeamInfo struct needs an accounting of the size of each role.
            newTeamInfo.mRoleSizeMap.reserve(mRoleInformation.getRoleCriteriaMap().size());
            RoleCriteriaMap::const_iterator roleIter = mRoleInformation.getRoleCriteriaMap().begin();
            RoleCriteriaMap::const_iterator roleEnd = mRoleInformation.getRoleCriteriaMap().end();
            for(; roleIter != roleEnd; ++roleIter)
            {
                newTeamInfo.mRoleSizeMap[roleIter->first] = 0;

            }
            mTeamIndexMap[newTeamInfo.mTeamId] = i;
        }


        replicatedGameData->getAdminPlayerList().copyInto(mAdminList);
        replicatedGameData->getGameAttribs().copyInto(mGameAttributeMap);
        replicatedGameData->getDedicatedServerAttribs().copyInto(mDedicatedServerAttributeMap);
        replicatedGameData->getEntryCriteriaMap().copyInto(mEntryCriteria);

        mPersistedGameId.set(replicatedGameData->getPersistedGameId());
        blaze_strnzcpy(mCCSPool, replicatedGameData->getCCSPool(), sizeof(mCCSPool));

        mPingSite = replicatedGameData->getPingSiteAlias();
        mNetworkTopology = replicatedGameData->getNetworkTopology();
        mGameType = replicatedGameData->getGameType();
    }

    /*! ************************************************************************************************/
    /*! \brief initialize the base game data from the supplied gameBrowserGameData object. Note: we only update fields that gameBrowserGameData knows about.
        \param[in] gameBrowserGameData the new data for the game
    ***************************************************************************************************/
    void GameBase::initGameBaseData(const GameBrowserGameData &gameBrowserGameData)
    {
        // NOTE: we init a GameBrowserGame's base data when a list updates, so this needs to be reiterant

        mGameId = gameBrowserGameData.getGameId();
        mState = gameBrowserGameData.getGameState();
        mTopologyHostId = gameBrowserGameData.getHostId();

        mGameProtocolVersionString = gameBrowserGameData.getGameProtocolVersionString();
        mName = gameBrowserGameData.getGameName();
        mGameSettings = gameBrowserGameData.getGameSettings();
        mPresenceMode = gameBrowserGameData.getPresenceMode();
        
        mGameModRegister = gameBrowserGameData.getGameModRegister();

        mVoipTopology = gameBrowserGameData.getVoipTopology();

        gameBrowserGameData.getSlotCapacities().copyInto(mPlayerCapacity);
        gameBrowserGameData.getPlayerCounts().copyInto(mPlayerSlotCounts);

        mQueueCapacity = gameBrowserGameData.getQueueCapacity();

        gameBrowserGameData.getRoleInformation().copyInto(mRoleInformation);

        const GameBrowserTeamInfoVector &teamInfo = gameBrowserGameData.getGameBrowserTeamInfoVector();
        mTeamInfoVector.clear(); // need to make sure old vector contents are wiped if updating GameBrowserGame
        mTeamInfoVector.reserve(teamInfo.size());
        for(TeamIndex i = 0; i < teamInfo.size(); ++i)
        {
            const GameBrowserTeamInfo *gameBrowserTeamInfo = teamInfo[i];
            mTeamInfoVector.push_back();
            TeamInfo &newTeamInfo = mTeamInfoVector.back();
            newTeamInfo.mTeamId = gameBrowserTeamInfo->getTeamId();
            newTeamInfo.mTeamSize = gameBrowserTeamInfo->getTeamSize();
            // each TeamInfo struct needs an accounting of the size of each role.
            newTeamInfo.mRoleSizeMap.reserve(gameBrowserTeamInfo->getRoleSizeMap().size());
            RoleMap::const_iterator roleIter = gameBrowserTeamInfo->getRoleSizeMap().begin();
            RoleMap::const_iterator roleEnd = gameBrowserTeamInfo->getRoleSizeMap().end();
            for(; roleIter != roleEnd; ++roleIter)
            {
                newTeamInfo.mRoleSizeMap[roleIter->first] = roleIter->second;

            }
            mTeamIndexMap[newTeamInfo.mTeamId] = i;
        }

        // calc the mMaxPlayerCapacity client-side
        const SlotCapacitiesVector &gameBrowserCapacity = gameBrowserGameData.getSlotCapacities();
        mMaxPlayerCapacity = 0;
        for (SlotCapacitiesVector::size_type slotType = SLOT_PUBLIC_PARTICIPANT; slotType < MAX_SLOT_TYPE; ++slotType)
        {
            // static cast needed due to warnings as errors
            mMaxPlayerCapacity = static_cast<uint16_t>(mMaxPlayerCapacity + gameBrowserCapacity[slotType]);
        }

        gameBrowserGameData.getAdminPlayerList().copyInto(mAdminList);
        gameBrowserGameData.getGameAttribs().copyInto(mGameAttributeMap);
        gameBrowserGameData.getDedicatedServerAttribs().copyInto(mDedicatedServerAttributeMap);
        gameBrowserGameData.getClientPlatformList().copyInto(mClientPlatformList);
        gameBrowserGameData.getEntryCriteriaMap().copyInto(mEntryCriteria);

        mPersistedGameId.set(gameBrowserGameData.getPersistedGameId());

        mPingSite = gameBrowserGameData.getPingSiteAlias();
        mNetworkTopology = gameBrowserGameData.getNetworkTopology();
        mGameType = gameBrowserGameData.getGameType();
    }


    /*! ************************************************************************************************/
    /*! \brief Returns the value of the supplied game attribute (or nullptr, if no attribute exists with that name).

        Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
        Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

        \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
        \return the value of the supplied attribute name (or nullptr, if the attib doesn't exist).
    ***************************************************************************************************/
    const char8_t* GameBase::getGameAttributeValue(const char8_t* attributeName) const
    {
        Collections::AttributeMap::const_iterator attribMapIter = mGameAttributeMap.find(attributeName);
        if ( attribMapIter != mGameAttributeMap.end() )
        {
            return attribMapIter->second.c_str();
        }

        return nullptr;
    }

    const char8_t* GameBase::getDedicatedServerAttributeValue(const char8_t* attributeName) const
    {
        Collections::AttributeMap::const_iterator attribMapIter = mDedicatedServerAttributeMap.find(attributeName);
        if (attribMapIter != mDedicatedServerAttributeMap.end())
        {
            return attribMapIter->second.c_str();
        }

        return nullptr;
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns true if the supplied playerId is an admin for this game.
        \param    playerId the BlazeId (playerId) to check for admin rights
        \return    True if this is an admin player.
    **************************************************************************************************************/
    bool GameBase::isAdmin(BlazeId playerId) const
    {
        PlayerIdList::const_iterator adminIdIter = mAdminList.begin();
        PlayerIdList::const_iterator adminIdEnd = mAdminList.end();
        for ( ; adminIdIter != adminIdEnd; ++adminIdIter )
        {
            if(*adminIdIter == playerId)
            {
                return true;
            }
        }

        return false;
    }

    /*! **********************************************************************************************************/
    /*! \brief (DEPRECATED)  Return the player capacity of a specific team; returns 0 if teamId not present in the game.

    The playerCapacity dictates the max number of players (of a specific team) who can be in the game.
    \deprecated use getTeamCapacity instead.

    \param[in] teamIndex the id of the team capacity to lookup
    \return the capacity of the given team; 0 if team is not present in game.
    **************************************************************************************************************/
    uint16_t GameBase::getTeamCapacityByIndex(TeamIndex teamIndex) const
    {
        if(teamIndex < mTeamInfoVector.size())
        {
            return getTeamCapacity();
        }

        return 0;
    }

    /*! **********************************************************************************************************/
    /*! \brief Return the player capacity for teams in this game
        Integrators note: this method should be used in place of the currently deprecated getTeamCapacityByIndex() method.
        \return teams' capacity
    **************************************************************************************************************/
    uint16_t GameBase::getTeamCapacity() const
    {
        return (getTeamCount() != 0) ? getParticipantCapacityTotal() / getTeamCount() : 0;
    }

    /*! **********************************************************************************************************/
    /*! \brief returns the number of players in the game for a specific teamId.
    \param[in] teamId the id of the team requested.
    \return    the number of players in the team, 0 if team doesn't exist.
    **************************************************************************************************************/
    uint16_t GameBase::getTeamSizeByIndex(TeamIndex teamIndex) const
    {
        if(teamIndex < mTeamInfoVector.size())
        {
            return mTeamInfoVector[teamIndex].mTeamSize;
        }

        return 0;
    }

    // return the teamId at index or INVALID_TEAM_ID if index is bad.
    TeamId GameBase::getTeamIdByIndex(TeamIndex index) const
    {
        if (index < mTeamInfoVector.size())
        {
            return mTeamInfoVector[index].mTeamId;
        }

        return INVALID_TEAM_ID;
    }

    /*! **********************************************************************************************************/
    /*! \brief Return the player capacity for a role in the game
        \return the capacity of the given role, 0 for roles that aren't allowed in the game
    **************************************************************************************************************/
    uint16_t GameBase::getRoleCapacity(const RoleName& roleName) const
    {
        RoleCriteriaMap::const_iterator roleCriteriaIter = mRoleInformation.getRoleCriteriaMap().find(roleName);
        if(roleCriteriaIter != mRoleInformation.getRoleCriteriaMap().end())
        {
            return roleCriteriaIter->second->getRoleCapacity();
        }

        return 0;
    }

    /*! **********************************************************************************************************/
    /*! \brief returns the number of players in the game, with a specific role, for a specific teamId.
    \param[in] teamIndex the id of the team requested.
    \param[in] roleName the role to get the current size of
    \return    the number of players in the team, with this role, 0 if team doesn't exist.
    **************************************************************************************************************/
    uint16_t GameBase::getRoleSizeForTeamAtIndex(TeamIndex teamIndex, const RoleName& roleName) const
    {
        if(teamIndex < mTeamInfoVector.size())
        {
            const TeamInfo& teamInfo = mTeamInfoVector[teamIndex];
            RoleMap::const_iterator roleIter = teamInfo.mRoleSizeMap.find(roleName);
            if(roleIter != teamInfo.mRoleSizeMap.end())
            {
                return roleIter->second;
            }
        }
        return 0;
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns the game's platform dependent end point address.
    **************************************************************************************************************/
    const NetworkAddress* GameBase::getNetworkAddress() const 
    { 
        return getNetworkAddress(getNetworkAddressList()); 
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns the first end point address for a given network address list

    Note: NetworkAddressList will be deprecated in the next opportunity.  See comments in GameEndpoint::mNetworkAddressList
    **************************************************************************************************************/
    const NetworkAddress* GameBase::getNetworkAddress(const NetworkAddressList* networkAddressList) const 
    { 
        if (networkAddressList != nullptr)
        {
            NetworkAddressList::const_iterator iter = networkAddressList->begin();
            NetworkAddressList::const_iterator end = networkAddressList->end();
            if (iter != end)
            {
                const NetworkAddress* address = *iter;
                return address;
            }
        }

        return nullptr; 
    }

} // namespace GameManager
} // namespace Blaze
