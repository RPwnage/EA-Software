/*! ************************************************************************************************/
/*!
    \file playerbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/gamemanager/playerbase.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/usermanager/usermanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Init the PlayerBase data from the supplied replicatedPlayerData obj.
    
        \param[in] gameManagerApi        the gameManagerApi that owns this player
        \param[in] replicatedPlayerData  the obj to init the player's base data from
        \param[in] memGroupId            mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    PlayerBase::PlayerBase(GameManagerAPI *gameManagerApi, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId)
        :    mGameManagerApi(gameManagerApi),
            mId(replicatedPlayerData->getPlayerId()),
            mSlotType(replicatedPlayerData->getSlotType()),
            mTeamIndex(replicatedPlayerData->getTeamIndex()),
            mRoleName(replicatedPlayerData->getRoleName()),
            mPlayerState(replicatedPlayerData->getPlayerState()),
            mPlayerAttributeMap(memGroupId, MEM_NAME(memGroupId, "PlayerBase::mPlayerAttributeMap")),
            mCustomData(memGroupId)
    {
        // create or find a user obj for this player
        Blaze::UserIdentification data;
        data.setAccountLocale(replicatedPlayerData->getAccountLocale());
        data.setAccountCountry(replicatedPlayerData->getAccountCountry());
        data.setName(replicatedPlayerData->getPlayerName());
        data.setPersonaNamespace(replicatedPlayerData->getPersonaNamespace());
        replicatedPlayerData->getPlatformInfo().copyInto(data.getPlatformInfo());
        data.setBlazeId(replicatedPlayerData->getPlayerId());
        mUser = UserContainer::addUser(mGameManagerApi->getUserManager(), data);

        // init player attribs & custom data blob
        replicatedPlayerData->getPlayerAttribs().copyInto(mPlayerAttributeMap);
        replicatedPlayerData->getCustomData().copyInto(mCustomData);
       
    }

    /*! ************************************************************************************************/
    /*! \brief Init the PlayerBase data from the supplied gameBrowserPlayerData obj.
    
        \param[in] gameManagerApi         the gameManagerApi that owns this player
        \param[in] gameBrowserPlayerData  the obj to init the player's base data from
        \param[in] memGroupId             mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    PlayerBase::PlayerBase(GameManagerAPI *gameManagerApi, const GameBrowserPlayerData *gameBrowserPlayerData, MemoryGroupId memGroupId)
        :    mGameManagerApi(gameManagerApi),
            mId( gameBrowserPlayerData->getPlayerId() ),
            mSlotType(gameBrowserPlayerData->getSlotType()),
            mTeamIndex(gameBrowserPlayerData->getTeamIndex()),
            mRoleName(gameBrowserPlayerData->getRoleName()),
            mPlayerState(gameBrowserPlayerData->getPlayerState()),
            mPlayerAttributeMap(memGroupId, MEM_NAME(memGroupId, "PlayerBase::mPlayerAttributeMap")),
            mCustomData(memGroupId)
    {
        // create or find a user obj for this player
        mUser = UserContainer::addUser(mGameManagerApi->getUserManager(), gameBrowserPlayerData->getPlayerId(), 
                                       gameBrowserPlayerData->getPlayerName(), gameBrowserPlayerData->getPlatformInfo(),
                                       gameBrowserPlayerData->getAccountLocale(), gameBrowserPlayerData->getAccountCountry(), 
                                       nullptr, gameBrowserPlayerData->getPlayerNamespace());

        // init player attribs
        gameBrowserPlayerData->getPlayerAttribs().copyInto(mPlayerAttributeMap);
        // Note: we never send the custom player data blob down while browsing
    }

    //! \brief destructor
    PlayerBase::~PlayerBase()
    {
        UserContainer::removeUser(mGameManagerApi->getUserManager(), mUser);
    }

    /*! ************************************************************************************************/
    /*! \brief Returns the value of the supplied player attribute (or nullptr, if no attribute exists with that name).

        Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
        Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

        \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
        \return the value of the supplied attribute name (or nullptr, if the attib doesn't exist).
    ***************************************************************************************************/
    const char8_t* PlayerBase::getPlayerAttributeValue(const char8_t* attributeName) const
    {
        Collections::AttributeMap::const_iterator attribMapIter = mPlayerAttributeMap.find(attributeName);
        if ( attribMapIter != mPlayerAttributeMap.end() )
        {
            return attribMapIter->second.c_str();
        }
        return nullptr;
    }

} // namespace GameManager
} // namespace Blaze

