/*! ************************************************************************************************/
/*!
    \file playerinfo.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/commoninfo.h"
#include "framework/usersessions/usersessionmanager.h" // gUserSessionManager
#include "gamemanager/externalsessions/externaluserscommoninfo.h" // for hasFirstPartyId

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief return the value of a specific player attribute.  returns nullptr if no attrib is defined with
                the supplied key.
    
        \param[in] key - attribute name to lookup
        \return the value of the attribute, or nullptr
    ***************************************************************************************************/
    const char8_t* PlayerInfo::getPlayerAttrib(const char8_t* key) const
    {
        Collections::AttributeMap::const_iterator attribIter = mPlayerData->getPlayerAttribs().find(key);
        if (attribIter != mPlayerData->getPlayerAttribs().end())
        {
            return attribIter->second;
        }

        return nullptr;
    }

    bool PlayerInfo::hasFirstPartyId() const 
    { 
        return Blaze::GameManager::hasFirstPartyId(getPlatformInfo()); 
    }

    bool PlayerInfo::hasExternalPlayerId() const
    {
        return (getPlatformInfo().getClientPlatform() == pc && getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID) ||
               (Blaze::GameManager::hasFirstPartyId(getPlatformInfo()));
    }

    void PlayerInfo::setPlayerState(PlayerState state)
    {
        mPlayerData->setPlayerState(state);
    }

    bool PlayerInfo::isParticipant() const 
    { 
        return isParticipantSlot(mPlayerData->getSlotType()); 
    }

    bool PlayerInfo::isSpectator() const 
    { 
        return isSpectatorSlot(mPlayerData->getSlotType()); 
    }

    void PlayerInfo::fillOutReplicatedGamePlayer(ReplicatedGamePlayer& player, bool hideNetworkAddress/*= false*/) const
    {
        player.setGameId(getGameId());
        player.setPlayerId(getPlayerId());
        getPlatformInfo().copyInto(player.getPlatformInfo());
        player.setExternalId(getExternalIdFromPlatformInfo(player.getPlatformInfo()));
        player.setAccountLocale(getAccountLocale());
        player.setAccountCountry(getAccountCountry());
        player.setSlotId(getSlotId());
        player.setPlayerSessionId(getPlayerSessionId());
        player.setPlayerName(getPlayerName());
        player.setPersonaNamespace(getPlayerNameSpace());
        player.setPlayerState(getPlayerState());
        player.setCustomData(const_cast<EA::TDF::TdfBlob&>(*getCustomData()));
        player.setPlayerAttribs(const_cast<Blaze::Collections::AttributeMap&>(getPlayerAttribs()));
        if (!hideNetworkAddress)
        {
            player.setNetworkAddress(const_cast<NetworkAddress&>(*getNetworkAddress()));
        }
        player.setSlotType(getSlotType());
        player.setTeamIndex(getTeamIndex());
        player.setRoleName(getRoleName());
        player.setUserGroupId(getUserGroupId());
        player.setJoinedViaMatchmaking(getJoinedViaMatchmaking());
        player.setJoinedGameTimestamp(getJoinedGameTimestamp());
        player.setReservationCreationTimestamp(getReservationCreationTimestamp());
        player.setConnectionGroupId(getConnectionGroupId());
        player.setConnectionSlotId(getConnectionSlotId());
        player.setHasJoinFirstPartyGameSessionPermission(mPlayerData->getHasJoinFirstPartyGameSessionPermission());
        player.getPlayerSettings().setBits(mPlayerData->getPlayerSettings().getBits());
        player.setUUID(mPlayerData->getUserInfo().getUUID());
        player.setDirtySockUserIndex(getDirtySockUserIndex());
        player.setEncryptedBlazeId(mPlayerData->getEncryptedBlazeId());
        player.setScenarioName(mPlayerData->getScenarioName());
    }

    void PlayerInfo::updateReplicatedPlayerData(GameManager::ReplicatedGamePlayerServer& playerData)
    {
        mPrevSnapshot = mPlayerData;
        mPlayerData = &playerData;
    }

    void intrusive_ptr_add_ref(PlayerInfo* ptr)
    {
        ++ptr->mRefCount;
    }

    void intrusive_ptr_release(PlayerInfo* ptr)
    {
        if (ptr->mRefCount > 0)
        {
            --ptr->mRefCount;
            if (ptr->mRefCount == 0)
                delete ptr;
        }
    }



    bool PlayerInfo::getSessionExists() const
    {
        return gUserSessionManager->getSessionExists(mPlayerData->getUserInfo().getSessionId());
    }

    bool PlayerInfo::getSessionExists(const UserSessionInfo &userSessionInfo) 
    { 
        return gUserSessionManager->getSessionExists(userSessionInfo.getSessionId()); 
    }

}  // namespace GameManager
}  // namespace Blaze
