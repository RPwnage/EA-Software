/*! ************************************************************************************************/
/*!
    \file usermanager/user.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_USER_MANAGER_USER_H
#define BLAZE_USER_MANAGER_USER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazeobject.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/component/framework/tdf/userextendeddatatypes.h"
#include "BlazeSDK/component/usersessionscomponent.h"
#include "BlazeSDK/component/utilcomponent.h"
#include "BlazeSDK/usermanager/userlistener.h"
#include "BlazeSDK/memorypool.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/intrusive_hash_map.h"
#include "EASTL/shared_ptr.h"

#ifdef BLAZE_USER_PROFILE_SUPPORT
    #include "DirtySDK/misc/userapi.h"
#endif

#ifdef EA_PLATFORM_PS4
    #include "BlazeSDK/ps4/ps4.h"
#endif

namespace Blaze
{
namespace UserManager
{
    struct BLAZESDK_API UserNode : public eastl::intrusive_list_node
    {};

    /*! ************************************************************************************************/
    /*!
        \brief User is a shared low-level representation of a Blaze persona containing the User's name (persona name),
            Blaze Id, and applicable 3rd party ids.

        Users are managed by the UserManagerApi, and are typically accessed through other blaze api's.  For example,
        each game player references a user; Association lists reference users, etc.
    ***************************************************************************************************/
    class BLAZESDK_API User :
        protected UserNode,
        public BlazeObject
    {
        friend class UserManager;
        friend class LocalUser;
    public:

        enum UserType
        {
            OWNED   = 0x0,
            CACHED  = 0x1
        };

        enum OnlineStatus
        {
            UNKNOWN,
            OFFLINE,
            ONLINE
        };
        
        /*! **********************************************************************************************************/
        /*! \brief Returns the user's Blaze Id. 
            \return    Returns the user id. 
        **************************************************************************************************************/
        BlazeId getId() const { return mId; }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if the user is a guest.  
                   Guest users share the same Display Name and External Id as their parent user, but have a negative Blaze Id.
                   NOTE:  This will also return true for trusted logins made by a dedicated server. (Which lack a parent.)
            \return    Returns the user's guest status based on blaze id. 
        **************************************************************************************************************/
        bool isGuestUser() const { return (getId() < 0); }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if the User is the PrimaryPersona for this account on this platform
            \return    Returns true if the User is the PrimaryPersona for this account on this platform
        **************************************************************************************************************/
        BlazeId getIsPrimaryPersona() const { return mIsPrimaryPersona; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the namespace of the persona. 
            \return    Returns the persona namespace. 
        **************************************************************************************************************/
        const char8_t* getPersonaNamespace() const { return mPersonaNamespace; }

        /*! ****************************************************************************/
        /*! \brief return the client platform (xone, ps4, etc.) of a player
        ********************************************************************************/
        ClientPlatformType getClientPlatform() const { return mPlatformInfo.getClientPlatform(); }

        /*! ****************************************************************************/
        /*! \brief return true if the user has enabled Crossplay
        ********************************************************************************/
        bool getCrossPlatformOptIn() const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the user name (the persona name). 
            \return    Returns the user name. 
        **************************************************************************************************************/
        const char8_t* getName() const { return mName; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the persona id
            \return    Returns the persona id. 
            // Deprecated! BlazeId is equivalent to PersonaId in Blaze 3
        **************************************************************************************************************/
        const char8_t* getPersonaId() const { return mPersonaId; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the account id
        \return    Returns the account id.
        **************************************************************************************************************/
        AccountId getNucleusAccountId() const { return mPlatformInfo.getEaIds().getNucleusAccountId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the account id - DEPRECATED, use getNucleusAccountId instead()
        \return    Returns the account id. 
        **************************************************************************************************************/
        AccountId getAccountId() const { return getNucleusAccountId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the account locale settings

            The Account Locale is the user's preferred locale setting for their Nucleus account. The locale is made up of a
            base language code and a language-variant code--where the language-variant can resemble a country or region code.
            Although 32 bit integer value, the location keeping locale value actually contains string of 4 characters,
            like "enUS".

            \return    Returns account locale
        **************************************************************************************************************/
        Locale getAccountLocale() const {
            return mAccountLocale;
        }

        /*! **********************************************************************************************************/
        /*! \brief Returns the account country setting

            The Account Country is the user's country setting (as a 2-character country or region code) for their Nucleus account.
            Although 32 bit integer value, the location keeping country value actually contains string of 2 characters,
            like "US".

            \return    Returns account country
        **************************************************************************************************************/
        uint32_t getAccountCountry() const {
            return mAccountCountry;
        }

        /*! **********************************************************************************************************/
        /*! \brief Retrieves a pointer to the users extended session data.
        
            \return Returns a pointer to the users extended session data object, or null if the
                    user does not have extended session data
        **************************************************************************************************************/
        UserSessionExtendedData* getExtendedData() { return mExtendedData; }


        /*! **********************************************************************************************************/
        /*! \brief Retrieves a pointer to the users extended session data.

            \return Returns a pointer to the users extended session data object, or null if the
            user does not have extended session data
        **************************************************************************************************************/
        const UserSessionExtendedData* getExtendedData() const { return mExtendedData; }

        /*! **********************************************************************************************************/
        /*! \brief Retrieves a value indicating if the user has extended session data
        
            \return Returns true if the user has extended session data, false otherwise
        **************************************************************************************************************/
        bool hasExtendedData() const { return mExtendedData != nullptr; }

        /*! **********************************************************************************************************/
        /*! \brief Retrieves a value from the user session extended data map
        
            \param[in] componentId - The component id of the component that owns the data to be retrieved
            \param[in] key - The component specific key for the data to be retrieved
            \param[out] value - contains the value associated with the key
            \return Returns true if the operation was successful, false otherwise
        **************************************************************************************************************/
        bool getDataValue(uint16_t componentId, uint16_t key, UserExtendedDataValue &value) const;

        /*! ************************************************************************************************/
        /*! \brief Returns true if the user has userExtendedData AND the extended data's blazeObjectIdList contains
                the supplied blazeObjectId.  Often used to determine game or game group membership.
        
            \param[in] blazeObjectId the blaze object id to query for in the user's extended data blazeObjectId list.
            \return true if the supplied id was in the extended data's blaze object id list.
        ***************************************************************************************************/
        bool hasExtendedDataBlazeObjectId(const EA::TDF::ObjectId& blazeObjectId) const;

        /*! ************************************************************************************************/
        /*! \brief Returns online status of the user, if local client is subscribed to the user.

            \return The online status if subscribed to the user, UNKNOWN if not subscribed to the user.
        ***************************************************************************************************/
        OnlineStatus getOnlineStatus() const;

        /*! ************************************************************************************************/
        /*! \brief Returns last known online status of the user.

            \note This may only be refreshed if the local client has subscribed to the user, or, after a lookup user call.
        ***************************************************************************************************/
        OnlineStatus getCachedOnlineStatus() const;

        /*! **********************************************************************************************************/
        /*! \brief Reports if the user has complete data. Does not return false for PC when externalId == 0.
            \return Returns true if the user object is complete
        **************************************************************************************************************/
        bool isComplete() const;        

        /*! **********************************************************************************************************/
#if defined(EA_PLATFORM_PS4)
        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Use getExternalId() instead.
                   Retrieves the Sony ID of the user, if available.
                   Returns True if this user has a valid Sony ID. 
                   Valid only on Sony and for the local user (until a later release.)

            \param id The Sony ID of the user is copied here.
            \return   Returns True if this user has a valid Sony ID, else returns False.
        **************************************************************************************************************/
        bool getPrimarySonyId(PrimarySonyId &id) const { return false; }
#endif
        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Use getPlatformInfo().getExternalIds().getPsnAccountId(), .getXblAccountId(), etc.
                   Returns the External ID value for this user.  Note that the value of the externalID depends on the client platform.

        \return    Returns the externalID value.
        **************************************************************************************************************/
        ExternalId getExternalId() const; 

        /*! ****************************************************************************/
        /*! \brief return the platform info for the user.  (ex. XblId, PsnId, OriginId, etc.)
        ********************************************************************************/
        const PlatformInfo& getPlatformInfo() const { return mPlatformInfo; }

        /*! ****************************************************************************/
        /*! \brief return the Xbl id for the user.  (INVALID_XBL_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalXblAccountId getXblAccountId() const { return mPlatformInfo.getExternalIds().getXblAccountId(); }

        /*! ****************************************************************************/
        /*! \brief return the Psn id for the user.  (INVALID_PSN_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalPsnAccountId getPsnAccountId() const { return mPlatformInfo.getExternalIds().getPsnAccountId(); }

        /*! *********************************************************************************************/
        /*! \brief return the Switch id (NSA id with environment prefix) for the user.  (empty string otherwise)
        *************************************************************************************************/
        const char8_t* getSwitchId() const { return mPlatformInfo.getExternalIds().getSwitchId(); }

        /*! ****************************************************************************/
        /*! \brief return the Steam id for the user.  (INVALID_STEAM_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalSteamAccountId getSteamAccountId() const { return mPlatformInfo.getExternalIds().getSteamAccountId(); }

        /*! ****************************************************************************/
        /*! \brief return the Stadia id for the user.  (INVALID_STADIA_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalStadiaAccountId getStadiaAccountId() const { return mPlatformInfo.getExternalIds().getStadiaAccountId(); }
        
        /*! **********************************************************************************************************/
        /*! \brief Returns the Nucleus Account Id, which is the primary id used for Origin operations (not the Origin Persona Id).
        \return    Returns the accountId value.
        **************************************************************************************************************/
        OriginPersonaId getOriginPersonaId() const { return mPlatformInfo.getEaIds().getOriginPersonaId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the user name (the persona name).
        \return    Returns the user name.
        **************************************************************************************************************/
        const char8_t* getOriginPersonaName() const { return mPlatformInfo.getEaIds().getOriginPersonaName(); }

    
        virtual ~User();

        static bool isValidMessageEntityType(const EA::TDF::ObjectType& entityType) { return (entityType == ENTITY_TYPE_USER); }
        static EA::TDF::ObjectType getEntityType() { return ENTITY_TYPE_USER; }
        virtual EA::TDF::ObjectId getBlazeObjectId() const { return EA::TDF::ObjectId(getEntityType(), getId()); }

        void copyIntoUserIdentification(UserIdentification& outUserId) const;

    protected:
        // TODO: create a blaze tdf (account?) containing the blaze user info (name, blazeId, externalId)
        //  modify ctor to init from this.
    
        /*! ************************************************************************************************/
        /*!
            \brief initialize a user with the supplied id & name and external ID (as defined by the external 
            live service identifying the user.)  
            
            Note: the refCount is initialized to 0.
        
            \param[in] blazeId the blazeID of the user
            \param[in] platformInfo the platformInfo of the user
            \param[in] isPrimaryPersona is this the primary persona for the user
            \param[in] personaNamespace The namespace of the user
            \param[in] userName The persona name of the user
            \param[in] accountLocale The account locale of the user
            \param[in] accountCountry The account country of the user
            \param[in] statusFlags user status flag
            \param[in] blazeHub the blazeHub
        ***************************************************************************************************/
        User(
            BlazeId blazeId,
            const PlatformInfo& platformInfo,
            bool isPrimaryPersona,
            const char8_t *personaNamespace,
            const char8_t *userName,
            Locale accountLocale,
            uint32_t accountCountry,
            const UserDataFlags *statusFlags,
            BlazeHub& blazeHub
        );

        // TODO: consider removing the reference counting, and using shared/weak pointers to handle it.
        // Note: for sanity's sake, though, we'd probably need to create blaze_ versions of the templates
        //   to handle specifying our allocations.

        /*! ************************************************************************************************/
        /*!
            \brief Increment this user's refCount.
        ***************************************************************************************************/
        void AddRef() { ++mRefCount; }

        /*! ************************************************************************************************/
        /*!
            \brief Decrement this user's refCount & return the number of references remaining.
                Note: Asserts if the refCount goes negative, but doesn't clamp the value.
            \return returns the number of remaining references
        ***************************************************************************************************/
        uint32_t RemoveRef()
        {
            BlazeAssert(mRefCount > 0);
            --mRefCount;
            return mRefCount;
        }

        /*! ************************************************************************************************/
        /*!
            \brief Get the current number of references to this user.
            \return the current number of references to this user.
        ***************************************************************************************************/
        uint32_t getRefCount() const { return mRefCount; }
        
        /*! **********************************************************************************************************/
        /*! \brief Retrieves a reference to the user's status flags structure.

        \return Returns a pointer to the status flags structure
        **************************************************************************************************************/
        const UserDataFlags& getStatusFlags() const { return mStatusFlags; }
        
        /*! ************************************************************************************************/
        /*!
            \brief Get last time this user object was updated. The timestamp is 0 when the User is created.
            \return timestamp computed by calling dirtysock NetTick().
        ***************************************************************************************************/
        uint32_t getTimestamp() const { return mTimestampMs; }

        /*! ************************************************************************************************/
        /*!    \brief Sets the namespace for the user.
            \param[in] personaNamespace Null terminated string.
        ***************************************************************************************************/
        void setPersonaNamespace(const char *personaNamespace);

        /*! ************************************************************************************************/
        /*!    \brief Sets the name for the user.
            \param[in] name Null terminated string.
        ***************************************************************************************************/
        void setName(const char *name);

        /*! ************************************************************************************************/
        /*!    \brief Sets the persona id for the user.
        \param[in] personaId BlazeId is equivalent to PersonaId in Blaze 3.
        ***************************************************************************************************/
        void setPersonaId(BlazeId personaId);

        /*! **********************************************************************************************************/
        /*! \brief Sets the account locale.

            Please refer to Blaze::UserManager::User:getAccountLocale() for detail explanation of account locale.

            \param[in] accountLocale - Account locale value to set.
        **************************************************************************************************************/
        void setAccountLocale(Locale accountLocale) {
            mAccountLocale = accountLocale;
        }

        /*! **********************************************************************************************************/
        /*! \brief Sets the account country.

            Please refer to Blaze::UserManager::User:getAccountCountry() for detail explanation of account country.

            \param[in] accountCountry - Account country value to set.
        **************************************************************************************************************/
        void setAccountCountry(uint32_t accountCountry) {
            mAccountCountry = accountCountry;
        }

    private:
        NON_COPYABLE(User);

        /*! **********************************************************************************************************/
        /*! \brief Clears the users extended session data
        **************************************************************************************************************/
        void clearExtendedData();

        /*! **********************************************************************************************************/
        /*! \brief Sets the users extended session data
            \param[in] data - a pointer to the data
        **************************************************************************************************************/
        void setExtendedData(const UserSessionExtendedData* data);

        /*! ************************************************************************************************/
        /*!
            \brief Set last time this user object was updated. The timestamp is 0 when the User is created.
            \param[in] timestamp - computed by calling dirtysock NetTick().
        ***************************************************************************************************/
        void setTimestamp(uint32_t timestamp) { mTimestampMs = timestamp; }
        
        /*! ************************************************************************************************/
        /*!
            \brief Refresh the occasionally changing user data using information obtained from the Blaze server.
            Also set the last time this user object was updated. 
            The timestamp is 0 when the User is created.
            \note This method will only be called for CACHED User objects, because OWNED user objects
            should continuosly receive notifications regarding their state and status.
            \param[in] userData - User data consisting of UserIdentification/UserExtendedData/UserDataFlags(online status)
        ***************************************************************************************************/
        void refreshTransientData(const UserData& userData);

        /*! **********************************************************************************************************/
        /*! \brief Sets the status flags for the user
            \param[in] userDataFlags - set the status flags provided in a UserUpdated message
        **************************************************************************************************************/
        void setStatusFlags(const UserDataFlags &statusFlags);

        /*! **********************************************************************************************************/
        /*! \brief Sets the User Index of a LocalUser object that subscribes to this User.
            \param[in] userIndex - index of LocalUser object
        **************************************************************************************************************/
        void setSubscriberIndex(uint32_t userIndex);
        
        /*! **********************************************************************************************************/
        /*! \brief Clears the User Index of a LocalUser object that subscribed to this User.
            \param[in] userIndex - index of LocalUser object
        **************************************************************************************************************/
        void clearSubscriberIndex(uint32_t userIndex);
        
        /*! **********************************************************************************************************/
        /*! \brief Checks if User Index of a LocalUser object is subscribed to this User.
            \param[in] userIndex - index of LocalUser object
            \return bool true if userIndex is subscribed to this User
        **************************************************************************************************************/
        bool isSubscribedByIndex(uint32_t userIndex) const;
        
        /*! **********************************************************************************************************/
        /*! \brief Sets if this User is subscribed to or not
            \param[in] subscribed - Is this User subscribed to or not
        **************************************************************************************************************/
        void setSubscribed(bool subscribed);

        uint16_t mRefCount;
        uint16_t mSubscribedBy; // each set bit corresponds to User Index of LocalUser that subscribes to this User object
        BlazeId mId;
        PlatformInfo mPlatformInfo;
        bool mIsPrimaryPersona;
        Locale mAccountLocale;
        uint32_t mAccountCountry;
        uint32_t mTimestampMs; // last time(NetTick()) when User information was updated from the Blaze server
        UserDataFlags mStatusFlags; // user status flags (stores online status and subscription status)
        char8_t mPersonaNamespace[MAX_NAMESPACE_LENGTH];
        char8_t mName[MAX_PERSONA_LENGTH];
        char8_t mPersonaId[MAX_PERSONA_ID_LENGTH]; // Deprecated! BlazeId is equivalent to PersonaId in Blaze 3
        UserSessionExtendedDataPtr mExtendedData;
        BlazeHub& mBlazeHub;
    };

    /*! ************************************************************************************************/
    /*!
        \brief LocalUser is a wrapper around User which contains additional information only available/needed 
            on this hardware.
    ***************************************************************************************************/
    class BLAZESDK_API LocalUser
        : protected UserEventListener
    {
        friend class UserManager;
    public:
        virtual ~LocalUser();

        /*! **********************************************************************************************************/
        /*! \brief Returns a pointer to the underlying User entity linked to LocalUser instance
        \return    User object pointer
        **************************************************************************************************************/
        User* getUser() const { return &mUser; }
        
        /*! **********************************************************************************************************/
        /*! \brief Returns a pointer to the underlying User entity linked to LocalUser instance
        \return    User object pointer
        **************************************************************************************************************/
        const UserSessionLoginInfo* getSessionInfo() const { return &mUserSessionLoginInfo; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the UserSessionKey linked to LocalUser instance.  Generally only used in the connection code.
        \return    User session key pointer
        **************************************************************************************************************/
        const char8_t* getUserSessionKey() const { return mUserSessionLoginInfo.getSessionKey(); }

        /*! **********************************************************************************************************/
        /*! \brief Check if the UserSessionKey has been set yet.
        \return    Bool 
        **************************************************************************************************************/
        bool hasUserSessionKey() const { return mUserSessionLoginInfo.getSessionKey()[0] != '\0'; }

        /*! **********************************************************************************************************/
        /*! \brief Returns this local users index in the BlazeHub
        \return    The user index
        **************************************************************************************************************/
        uint32_t getUserIndex() const { return mUserIndex; }
        
        /*! **********************************************************************************************************/
        /*! \brief Returns a pointer to the hardware flags bit field class, from here you can get/set/clear 
            individual bits of the flags. 
            \return    Hardware flags pointer
        **************************************************************************************************************/
        HardwareFlags* getHardwareFlags() { return &mHardwareFlags.getHardwareFlags(); }

        /*! **********************************************************************************************************/
        /*! \brief Sends the current hardware flags information to the server.
            \return    void
        **************************************************************************************************************/
        void updateHardwareFlags(BlazeHub* pBlazeHub) const { return updateHardwareFlags(); }
        void updateHardwareFlags() const;

        /*! ****************************************************************************/
        /*! \brief Sets an attribute in the user session extended data for the given user
            \param[in] pBlazeHub                DEPRECATED - The BlazeHub 
            \param[in] key                      The key for the attribute
            \param[in] value                    The attribute's value
            \param[in] cb                       The functor dispatched upon success/failure
            \return                             The JobId of the RPC call
        ********************************************************************************/
        JobId setUserSessionAttribute(BlazeHub* pBlazeHub, uint16_t key, int64_t value, const UserSessionsComponent::UpdateExtendedDataAttributeCb &cb) const { return setUserSessionAttribute(key, value, cb); }
        JobId setUserSessionAttribute(uint16_t key, int64_t value, const UserSessionsComponent::UpdateExtendedDataAttributeCb &cb) const;

        /*! ****************************************************************************/
        /*! \brief removes an attribute from the user session extended data for the given user
            \param[in] pBlazeHub                DEPRECATED - The BlazeHub
            \param[in] key                      The key for the attribute
            \param[in] cb                       The functor dispatched upon success/failure
            \return                             The JobId of the RPC call
        ********************************************************************************/
        JobId removeUserSessionAttribute(BlazeHub* pBlazeHub, uint16_t key, const UserSessionsComponent::UpdateExtendedDataAttributeCb &cb) const { return removeUserSessionAttribute(key, cb);  }
        JobId removeUserSessionAttribute(uint16_t key, const UserSessionsComponent::UpdateExtendedDataAttributeCb &cb) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns a bool indicating if the User has opted-in to cross platform experiences/interactions
        \return    Returns the current status of the CrossPlatform opt-in flag for the user.
        **************************************************************************************************************/
        bool getCrossPlatformOptIn() const { return mUser.getCrossPlatformOptIn(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns a bool indicating if the User wishes to opt into cross platform experiences/interactions.
                   Title code is responsible for validating the caller is eligible, per 1st party requirements, to opt into
                   cross platform play, and that game UI and behavior meet platform holder requirements.
        \param[in] optIn                    True if the user wishes to opt into cross-play.
        \param[in] cb                       The functor dispatched upon success/failure
        \return                             The JobId of the RPC call
        **************************************************************************************************************/
        JobId setCrossPlatformOptIn(bool optIn, const UserSessionsComponent::SetUserCrossPlatformOptInCb &cb) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the user's Blaze Id. 
        \return    Returns the user id. 
        **************************************************************************************************************/
        BlazeId getId() const { return mUser.getId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the persona namespace. 
        \return    Returns the persona namespace.
        **************************************************************************************************************/
        const char8_t* getPersonaNamespace() const { return mUser.getPersonaNamespace(); }

        /*! ****************************************************************************/
        /*! \brief return the client platform (xone, ps4, etc.) of a player
        ********************************************************************************/
        ClientPlatformType getClientPlatform() const { return mUser.getClientPlatform(); }


        /*! **********************************************************************************************************/
        /*! \brief Returns the user name (the persona name). 
        \return    Returns the user name. 
        **************************************************************************************************************/
        const char8_t* getName() const { return mUser.getName(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the persona id
            \return    Returns the persona id.
        **************************************************************************************************************/
        const char8_t* getPersonaId() const { return mUser.getPersonaId(); }


        /*! **********************************************************************************************************/
        /*! \brief Returns the account locale settings

            The Account Locale is the user's preferred locale setting for their Nucleus account. The locale is made up of a
            base language code and a language-variant code--where the language-variant can resemble a country or region code.
            Although 32 bit integer value, the location keeping locale value actually contains string of 4 characters,
            like "enUS".

            \return    Returns account locale
        **************************************************************************************************************/
        Locale getAccountLocale() const { return mUser.getAccountLocale(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the account country setting

            The Account Country is the user's country setting (as a 2-character country or region code) for their Nucleus account.
            Although 32 bit integer value, the location keeping country value actually contains string of 2 characters,
            like "US".

            \return    Returns account country
        **************************************************************************************************************/
        uint32_t getAccountCountry() const { return mUser.getAccountCountry(); }

        /*! **********************************************************************************************************/
        /*! \brief Retrieves a pointer to the users extended session data.

            \return Returns a pointer to the users extended session data object, or null if the
            user does not have extended session data
        **************************************************************************************************************/
        UserSessionExtendedData* getExtendedData() { return mUser.getExtendedData(); }


        /*! **********************************************************************************************************/
        /*! \brief Retrieves a pointer to the users extended session data.

            \return Returns a pointer to the users extended session data object, or null if the
            user does not have extended session data
        **************************************************************************************************************/
        const UserSessionExtendedData* getExtendedData() const { return mUser.getExtendedData(); }

        /*! **********************************************************************************************************/
        /*! \brief Retrieves a value indicating if the user has extended session data

            \return Returns true if the user has extended session data, false otherwise
        **************************************************************************************************************/
        bool hasExtendedData() const { return mUser.hasExtendedData(); }

        /*! **********************************************************************************************************/
        /*! \brief Retrieves a value from the user session extended data map

            \param[in] componentId - The component id of the component that owns the data to be retrieved
            \param[in] key - The component specific key for the data to be retrieved
            \param[out] value - contains the value associated with the key
            \return Returns true if the operation was successful, false otherwise
        **************************************************************************************************************/
        bool getDataValue(uint16_t componentId, uint16_t key, UserExtendedDataValue &value) const 
        { return mUser.getDataValue(componentId, key, value); }

        /*! ************************************************************************************************/
        /*! \brief Returns true if the user has userExtendedData AND the extended data's blazeObjectIdList contains
                the supplied blazeObjectId.  Often used to determine game or game group membership.
        
            \param[in] blazeObjectId the blaze object id to query for in the user's extended data blazeObjectId list.
            \return true if the supplied id was in the extended data's blaze object id list.
        ***************************************************************************************************/
        bool hasExtendedDataBlazeObjectId(EA::TDF::ObjectId blazeObjectId) const { return mUser.hasExtendedDataBlazeObjectId(blazeObjectId); }

        /*! **********************************************************************************************************/
        /*! \brief Reports if the user has complete data. Does not return false for Sony user missing Sony's primary ID
        \return Returns true if the user object is complete
        **************************************************************************************************************/
        bool isComplete() const { return mUser.isComplete(); }

        /*! ************************************************************************************************/
        /*! \brief Returns the connection group blaze object
            \return the connection group blaze object
        ***************************************************************************************************/
        ConnectionGroupObjectId getConnectionGroupObjectId() const { return mUserSessionLoginInfo.getConnectionGroupObjectId(); }

        /*! ************************************************************************************************/
        /*! \brief Returns the user's session type
            \return the user's session type
        ***************************************************************************************************/
        UserSessionType getUserSessionType() const { return mSessionType; }

#if defined(EA_PLATFORM_PS4)
        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Retrieves the Sony ID of the user, if available.
        Returns True if this user has a valid Sony ID. 
        Valid only on Sony and for the local user (until a later release.)

        \param id The Sony ID of the user is copied here.
        \return    Returns True if this user has a valid Sony ID, else returns False.
        **************************************************************************************************************/
        bool getPrimarySonyId(PrimarySonyId &id) const { return mUser.getPrimarySonyId(id); }
#endif

        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Returns the External ID value for this user.

        The value of the externalID depends on the client platform.   For example the Xbox360 ExternalId value is 
        actually the user XUID.

        \return    Returns the externalID value.
        **************************************************************************************************************/
        ExternalId getExternalId() const { return mUser.getExternalId(); }

        /*! ****************************************************************************/
        /*! \brief return the platform info for the user.  (ex. XblId, PsnId, OriginId, etc.)
        ********************************************************************************/
        const PlatformInfo& getPlatformInfo() const { return mUser.getPlatformInfo(); }

        /*! ****************************************************************************/
        /*! \brief return the Xbl id for the user.  (INVALID_XBL_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalXblAccountId getXblAccountId() const { return mUser.getXblAccountId(); }

        /*! ****************************************************************************/
        /*! \brief return the Psn id for the user.  (INVALID_PSN_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalPsnAccountId getPsnAccountId() const { return mUser.getPsnAccountId(); }

        /*! *********************************************************************************************/
        /*! \brief return the Switch id (NSA id with environment prefix) for the user.  (empty string otherwise)
        *************************************************************************************************/
        const char8_t* getSwitchId() const { return mUser.getSwitchId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Nucleus Account Id, which is the primary id used for Origin operations (not the Origin Persona Id).
        \return    Returns the accountId value.
        **************************************************************************************************************/
        AccountId getNucleusAccountId() const { return mUser.getNucleusAccountId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Origin Persona Id.
        \return    Returns the personaid for this user's origin account.
        **************************************************************************************************************/
        OriginPersonaId getOriginPersonaId() const { return mUser.getOriginPersonaId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Origin Persona Name.
        \return    Returns the PersonaName for this user's origin account.
        **************************************************************************************************************/
        const char8_t* getOriginPersonaName() const { return mUser.getOriginPersonaName(); }

        

        const Util::GetTelemetryServerResponse *getTelemetryServer() const { return &mTelemetryServer; }
        const Util::GetTickerServerResponse *getTickerServer() const { return &mTickerServer; }
        const Util::UserOptions *getUserOptions() { return &mUserOptions; }

        JobId updateUserOptions(const Blaze::Util::UserOptions &request,  Functor1<Blaze::BlazeError> cb);
        bool isUserDeauthenticating() const { return mDeauthenticating; }
        void setUserDeauthenticating() { mDeauthenticating = true; }

        /*! **********************************************************************************************************/
        /*! \brief Extracts the UserSessionId from the SessionKey (which has the session id as the first 16 characters, in hex)
                   UserSessionId is not used for anything on the client, but may be useful for logging. 
        \return    User session id
        **************************************************************************************************************/
        UserSessionId getUserSessionId() const { return mUserSessionId; }

    protected:
        /*! ************************************************************************************************/
        /*!
            \brief initialize a LocalUser with the supplied User object
        
            \param[in] user        the User object instance
            \param[in] userIndex   the users index in the list of users
            \param[in] sessionType The users session type
            \param[in] blazeHub    A reference to the hub
            \param[in] memGroupId  mem group to be used by this class to allocate memory
        ***************************************************************************************************/
        LocalUser(User& user, uint32_t userIndex, UserSessionType sessionType, BlazeHub& blazeHub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        void finishAuthentication(const UserSessionLoginInfo &info);
        void setCanTransitionToAuthenticated(bool canTransitionToAuthenticated);

    private:
        void onPostAuth(const Util::PostAuthResponse* response, BlazeError error, JobId id);

        virtual void onExtendedUserDataInfoChanged(Blaze::BlazeId blazeId);
            
        void setTelemetryServerResponse(const Util::GetTelemetryServerResponse *response) { response->copyInto(mTelemetryServer); }
        void setTickerServerResponse(const Util::GetTickerServerResponse *response) { response->copyInto(mTickerServer); }
        void setUserOptions(const Util::UserOptions *response) { response->copyInto(mUserOptions); }

        void onUpdateUserOptionsCb(BlazeError error, JobId id, Util::TelemetryOpt telOpt, Functor1<Blaze::BlazeError> cb);

    private:
        NON_COPYABLE(LocalUser);
        User& mUser;
        uint32_t mUserIndex;
        UserSessionType mSessionType;
        BlazeHub& mBlazeHub;
        bool mPostAuthDataReceived;
        bool mCanTransitionToAuthenticated;
        bool mDeauthenticating;
        UpdateHardwareFlagsRequest mHardwareFlags;
        UserSessionLoginInfo mUserSessionLoginInfo;
        UserSessionId mUserSessionId;

        Util::GetTelemetryServerResponse mTelemetryServer;
        Util::GetTickerServerResponse    mTickerServer;
        Util::UserOptions mUserOptions;
    };

#ifdef BLAZE_USER_PROFILE_SUPPORT
    #if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) 
    typedef ExternalId FirstPartyLookupId;
    #endif

    struct UserProfileNode : public eastl::intrusive_list_node
    {
    };

    class UserProfile : protected UserProfileNode
    {
        friend class UserManager;
        friend class eastl::intrusive_list<UserProfile>;
        friend struct eastl::default_delete<UserProfile>;

    public:
        virtual ~UserProfile();

        // Returns the id used to lookup this UserProfile from first party.
        // On Xbox, this is the user's Xuid.
        // On PS4, this is the user's AccountId.
        FirstPartyLookupId getFirstPartyLookupId() const { return mFirstPartyLookupId; }

        // Returns the display name (if unavailable, this will be the same as GamerTag)
        const char8_t* getDisplayName() const { return mProfile.Profile.strDisplayName; }

        // Returns the gamertag (or onlineId)
        const char8_t* getGamerTag() const { return mProfile.Profile.strGamertag; }

        // Returns the avatar image URI
        const char8_t* getAvatarUrl() const { return mProfile.Profile.strAvatarUrl; }

        // Returns JSON-encoded 1st party profile info
        const char8_t* getRawData() const { return (char8_t*)mProfile.Profile.pRawData; }

        // Additional helper functions to access the DirtySDK data structures:  (Note: pRawData may not have been copied)
        const UserApiProfileT* getProfile() const { return &mProfile.Profile; }
        const UserApiPresenceT* getPresence() const { return &mProfile.Presence; }
        const UserApiRichPresenceT* getRichPresence() const { return &mProfile.RichPresence; }
#if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
        const UserApiAccessibilityT* getAccessibility() const { return &mProfile.Profile.Accessibility; }
#endif
    private:
        // Create and Destroy functions that use the internal BLAZE_NEW and BLAZE_DELETE to allocate the object.
        static UserProfile* Create(const UserApiUserDataT& profile, const char8_t* allocName);
        static void Destroy(UserProfile* userProfile);

    private:
        /*! ************************************************************************************************/
        /*!
            \brief Initialize a user profile with the supplied external id & 1st party profile data.

            \param[in] profile      The 1st party profile data of the user
            \param[in] memGroupId  mem group to be used by this class to allocate memory
        ***************************************************************************************************/
        UserProfile(const UserApiUserDataT& profile, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        void update(const UserApiUserDataT& profile);

    private:
        UserApiUserDataT     mProfile;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) 
        ExternalId mFirstPartyLookupId;
#endif
    };

    typedef eastl::shared_ptr<UserProfile> UserProfilePtr;

#endif

} // namespace UserManager
} // namespace Blaze

#ifdef BLAZE_USER_PROFILE_SUPPORT
namespace eastl
{
    // Specialized template that gets called when the last reference to
    // a UserProfilePtr is dropped, and the object needs to be deleted.
    // Note, we can't just use the delete() operator, because of special heap
    // considerations from custom allocators.
    template <>
    struct default_delete<Blaze::UserManager::UserProfile>
    {
        typedef Blaze::UserManager::UserProfile value_type;
        void operator()(const Blaze::UserManager::UserProfile* p) const
        {
            Blaze::UserManager::UserProfile::Destroy(const_cast<Blaze::UserManager::UserProfile*>(p));
        }
    };
}
#endif

EASTL_DECLARE_TRIVIAL_RELOCATE(Blaze::UserManager::User); // stl container moves can be optimized using memcpy

#endif // BLAZE_USER_MANAGER_USER_H
