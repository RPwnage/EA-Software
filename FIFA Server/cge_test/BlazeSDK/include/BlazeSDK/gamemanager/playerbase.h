/*! ************************************************************************************************/
/*!
    \file playerbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_BASE_H
#define BLAZE_GAMEMANAGER_PLAYER_BASE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanagercomponent.h"
#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usercontainer.h"
#include "BlazeSDK/gamemanager/gamemanagerhelpers.h" // for SlotType helpers


namespace Blaze
{

namespace GameManager
{
    class GameManagerAPI;

    /*! ************************************************************************************************/
    /*! \brief PlayerBase is an abstract class containing shared data and getter methods for blaze game players.

        PlayerBase is a common base class for Players and GameBrowserPlayers.
    ***************************************************************************************************/
    class BLAZESDK_API PlayerBase : private UserManager::UserContainer
    {
            NON_COPYABLE(PlayerBase);
    public:
   
        /*! **********************************************************************************************************/
        /*! \brief Returns the player's id (BlazeId).
            \return    the player's BlazeId.
        **************************************************************************************************************/
        BlazeId getId() const { return mId; }

        /*! ************************************************************************************************/
        /*! \brief Return the player's name (equivalent to getUser()->getName()).
            \return the player's name.
        ***************************************************************************************************/
        const char8_t* getName() const { return mUser->getName(); }

        /*! ************************************************************************************************/
        /*! \brief return the persona namespace of a player
        ***************************************************************************************************/
        const char8_t* getPlayerPersonaNamespace() const { return mUser->getPersonaNamespace(); }

        /*! ****************************************************************************/
        /*! \brief return the client platform (xone, ps4, etc.) of a player
        ********************************************************************************/
        ClientPlatformType getPlayerClientPlatform() const { return mUser->getClientPlatform(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the underlying User object for this player.
            \return    The player's UserManager User object.
        **************************************************************************************************************/
        const UserManager::User* getUser() const { return mUser; }

        /*! ************************************************************************************************/
        /*! \brief Returns the value of the supplied player attribute (or nullptr, if no attribute exists with that name).

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            Note: You should avoid caching off pointers to values returned as they can be invalidated
                when game attributes are updated.  Pointers will possibly be invalidated during a call to
                BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
                cause the pointer to be invalidated.

            \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
            \return the value of the supplied attribute name (or nullptr, if the attib doesn't exist).
        ***************************************************************************************************/
        const char8_t* getPlayerAttributeValue(const char8_t* attributeName) const;

        /*! ************************************************************************************************/
        /*! \brief Get the player's attribute map (read only)

            Note: You should avoid caching off pointers to values inside the map as they can be invalidated
                when player attributes are updated.  Pointers will possibly be invalidated during a call to
                BlazeHub::idle().  Holding a pointer longer than the time in between calls to idle() can
                cause the pointer to be invalidated.

            \return Returns a const reference to the player's attribute map.
        ***************************************************************************************************/
        const Blaze::Collections::AttributeMap& getPlayerAttributeMap() const { return mPlayerAttributeMap; }

        /*! ************************************************************************************************/
        /*! \brief get the player's custom data blob.
            \return the player's custom data blob
        ***************************************************************************************************/
        const EA::TDF::TdfBlob* getCustomData() const { return &mCustomData; }

        
        /*! ************************************************************************************************/
        /*! \brief get the player's team index
        \return the player's team index, UNSPECIFIED_TEAM_INDEX if the player isn't in a team.
        ***************************************************************************************************/
        TeamIndex getTeamIndex() const { return mTeamIndex; }

         /*! ************************************************************************************************/
        /*! \brief get the player's current role
        \return the player's role name
        ***************************************************************************************************/
        const RoleName& getRoleName() const { return mRoleName; }

        /*! ************************************************************************************************/
        /*! \brief Returns the players connection state with respect to the entire mesh.
        ***************************************************************************************************/
        PlayerState getPlayerState() const { return mPlayerState; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if the user is a spectator
        ***************************************************************************************************/
        bool isSpectator() const { return isSpectatorSlot(mSlotType); }

         /*! ************************************************************************************************/
        /*! \brief Returns true if the user is a participant in the game
        ***************************************************************************************************/
        bool isParticipant() const { return isParticipantSlot(mSlotType); }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this player is an active player in the game.
            \return    True if this is an active player.
        **************************************************************************************************************/
        bool isActive() const { return ( (mPlayerState == ACTIVE_CONNECTED) ||
                                         (mPlayerState == ACTIVE_CONNECTING) ||
                                         (mPlayerState == ACTIVE_MIGRATING) ||
                                         (mPlayerState == ACTIVE_KICK_PENDING) ); }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this player is queued to enter the game instead of actually in the game.
            \return    True if this is an queued player.
        **************************************************************************************************************/
        virtual bool isQueued() const { return (mPlayerState == QUEUED); }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this player is reserved to enter the game instead of actually in the game.
            \return    True if this is a reserved player.
        **************************************************************************************************************/
        bool isReserved() const { return (mPlayerState == RESERVED); }

    //! \cond INTERNAL_DOCS

        GameManagerAPI * getGameManagerAPI() { return mGameManagerApi; }
    protected:

        /*! ************************************************************************************************/
        /*! \brief Init the PlayerBase data from the supplied replicatedPlayerData obj.
        
        \param[in] gameManagerApi        the gameManagerApi that owns this player
        \param[in] replicatedPlayerData  the obj to init the player's base data from
        \param[in] memGroupId            mem group to be used by this class to allocate memory 
        ***************************************************************************************************/
        PlayerBase(GameManagerAPI *gameManagerApi, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        /*! ************************************************************************************************/
        /*! \brief Init the PlayerBase data from the supplied gameBrowserPlayerData obj.
        
            \param[in] gameManagerApi         the gameManagerApi that owns this player
            \param[in] gameBrowserPlayerData  the obj to init the player's base data from
            \param[in] memGroupId             mem group to be used by this class to allocate memory
        ***************************************************************************************************/
        PlayerBase(GameManagerAPI *gameManagerApi, const GameBrowserPlayerData *playerData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        //! \brief destructor
        virtual ~PlayerBase();

    protected:
        GameManagerAPI *mGameManagerApi;
        BlazeId mId;
        SlotType mSlotType;
        TeamIndex mTeamIndex;
        RoleName mRoleName;
        PlayerState mPlayerState;
        const UserManager::User *mUser;
        Collections::AttributeMap mPlayerAttributeMap;
        EA::TDF::TdfBlob mCustomData;

    //! \endcond
    };

} //namespace GameManager
} //namespace Blaze
#endif // BLAZE_GAMEMANAGER_PLAYER_BASE_H
