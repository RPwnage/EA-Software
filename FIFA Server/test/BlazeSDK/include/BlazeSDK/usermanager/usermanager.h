/*************************************************************************************************/
/*!
    \file usermanager/usermanager.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_USER_MANAGER_USER_MANAGER_H
#define BLAZE_USER_MANAGER_USER_MANAGER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/blazestateprovider.h"
#include "BlazeSDK/component/usersessionscomponent.h"
#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/userlistener.h"
#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/blaze_eastl/list.h"
#include "BlazeSDK/blaze_eastl/string.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/usergroup.h"

#ifdef EA_PLATFORM_PS4
    #include "BlazeSDK/ps4/ps4.h"
#endif

#ifdef BLAZE_USER_PROFILE_SUPPORT
    struct UserApiRefT;
    #include "BlazeSDK/blaze_eastl/hash_set.h"
#endif

namespace Blaze
{
    class BlazeHub;

    namespace GameManager 
    { 
        class PlayerBase;
    }

    namespace LoginManager
    {
        class LoginManagerImpl;
    }

namespace UserManager
{
    class UserContainer;
    class UserManager;

    /*! ************************************************************************************************/
    /*!
        \brief LocalUserGroup is a shared low-level representation of a subset of local users selected
            for taking actions together, such as joining a game or game group or room. There is only one
            local user group created under UserManager for each blaze hub. LocalUserGroup can be accessed
            through UserManager APIs. 
    ***************************************************************************************************/
    class BLAZESDK_API LocalUserGroup :
        public UserGroup
    {
        friend class UserManager;

    public:

        LocalUserGroup(BlazeHub *blazeHub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        virtual ~LocalUserGroup();

        static EA::TDF::ObjectType getEntityType() { return ENTITY_TYPE_LOCAL_USER_GROUP; }
        virtual EA::TDF::ObjectId getBlazeObjectId() const { return mId; }

        bool isEmpty() const { return mUserIndicesInGroup.empty(); }

        const Blaze::ConnectionUserIndexList& getConnUserIndexList() const { return mUserIndicesInGroup; }

        typedef Functor3<BlazeError, JobId, const Blaze::UpdateLocalUserGroupResponse *> UpdateLUGroupCb;

        /*! *******************************************************************************************/
        /*! \brief Update local user group. Tells server to update the group using the 
            specific user indices provided. User indices not provided will be removed from the user group.

            \param[in] userIndices              The list of user indices 
            \param[in] titleUpdateCb            The functor dispatched upon success/failure
            \return                             The JobId of the RPC call
        ***********************************************************************************************/
        JobId updateLocalUserGroup(const Blaze::ConnectionUserIndexList& userIndices, const UpdateLUGroupCb &titleUpdateCb);

    private:
        NON_COPYABLE(LocalUserGroup); 

        void removeDeAuthenticatedLocalUsers();

        /*! *******************************************************************************************/
        /*! \brief Internal callback for processing the response from updating local user group.
        
            \param[in] response                     The server's LocalUserGroup response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                      The functor dispatched upon completion
        ***********************************************************************************************/
        void internalUpdateLocalUserGroupCb(const Blaze::UpdateLocalUserGroupResponse *response, BlazeError error, JobId jobId, UpdateLUGroupCb titleCb);

        BlazeHub* mBlazeHub;
        Blaze::ConnectionUserIndexList mUserIndicesInGroup;
        EA::TDF::ObjectId mId;
    };

    /*! ***********************************************************************************************/
    /*! \class UserManager
        \ingroup _mod_initialization

        \brief Class used to manage Users.
    ***************************************************************************************************/
    class BLAZESDK_API UserManager :
        public UserManagerStateListener,
        public BlazeStateEventHandler,
        public Idler
    {
        friend class UserContainer;
        friend class LocalUser;
        friend class LoginManager::LoginManagerImpl;

    public:
        static const uint32_t MIN_CACHED_USER_REFRESH_INTERVAL_MS = 250; /* 1/4 sec */
        static const uint32_t USER_HASH_BUCKET_COUNT = 67; /* better when prime */
        static const uint32_t MAX_CACHED_USERS = 128;
        static const uint32_t MAX_CACHED_USER_PROFILES = 128;
        static const int32_t  USERPROFILE_TIMEOUT_MS = 20000;
#ifdef BLAZE_USER_PROFILE_SUPPORT
        static const uint32_t RESET_USER_PROFILE_CACHE_INTERVAL_MS = 600000;
#endif
        
        /*! **********************************************************************************************************/
        /*! \brief Returns the local user, or nullptr if not authenticated.
            \return    Returns the local user or nullptr if not authenticated.
        **************************************************************************************************************/
        LocalUser* getLocalUser(uint32_t userIndex) const { return userIndex < mLocalUserVector.size() ? mLocalUserVector[userIndex] : nullptr; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the local user with the specified BlazeId, or nullptr if not found or the user is not authenticated.
            \return    Returns the local user or nullptr if not authenticated.
        **************************************************************************************************************/
        LocalUser* getLocalUserById(BlazeId blazeId) const;

        /*! ************************************************************************************************/
        /*! \brief Returns the primary local user.
            \return The primary local user
        ***************************************************************************************************/
        LocalUser* getPrimaryLocalUser() const { return mLocalUserVector[mPrimaryLocalUserIndex]; }

        /*! ************************************************************************************************/
        /*! \brief Returns the index of the primary local user.
        
            \return the index of the primary local user.
        ***************************************************************************************************/
        uint32_t getPrimaryLocalUserIndex() const { return mPrimaryLocalUserIndex; }

        /*! ************************************************************************************************/
        /*! \brief Sets a new primary local user.
        
            \param[in] userIndex - the user index of the new primary user.
        ***************************************************************************************************/
        void setPrimaryLocalUser(uint32_t userIndex);

        /*! **********************************************************************************************************/
        /*! \brief Return the User with the specified id.  Note: only returns users from the local userMap.
            \param[in] blazeId the blazeId of the user to get
            \return the user, or nullptr if no user exists in the local map with the supplied id.
        **************************************************************************************************************/
        const User* getUserById( BlazeId blazeId ) const { return getUser(blazeId); }

        /*! **********************************************************************************************************/
        /*! \brief scan the user map and return the user from the default namespace with the supplied name (if one exists on the caller's platform).
                Note: inefficient, prefer getUserById.  names are not case sensitive.

            \param[in] name the name of the user to get.  Case insensitive.
            \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
        **************************************************************************************************************/
        const User* getUserByName( const char8_t* name ) const { return getUser(name); }

        /*! **********************************************************************************************************/
        /*! \brief scan the user map and return the user from the specified namespace with the supplied name (if one exists on the caller's platform).
                Note: inefficient, prefer getUserById.  names are not case sensitive.

            \param[in] personaNamespace    The namespace of the user to lookup, nullptr implies platform default namespace
            \param[in] name                the name of the user to get.  Case insensitive.
            \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
        **************************************************************************************************************/
        const User* getUserByName( const char8_t* personaNamespace, const char8_t* name ) const { return getUser(personaNamespace, name); }

        /*! **********************************************************************************************************/
        /*! \brief scan the user map and return the user from the specified namespace with the supplied name and platform.
                Note: inefficient, prefer getUserById.  names are not case sensitive.

            \param[in] personaNamespace    The namespace of the user to lookup, nullptr implies platform default namespace
            \param[in] name                the name of the user to get.  Case insensitive.
            \param[in] platform            The platform of the user to get. INVALID implies caller's platform.
            \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
        **************************************************************************************************************/
        const User* getUserByName(const char8_t* personaNamespace, const char8_t* name, ClientPlatformType platform) const { return getUser(personaNamespace, name, platform); }

        /*! **********************************************************************************************************/
        /*! \brief DEPRECATED - Returns the user with the given external identifier (  inefficient, prefer getUserById if possible.)
          
            Examples of external identifiers include XboxUid for the Xbox.

            \param[in] extid The external ID for the user (if applicable.)
            \return the User obj with the associated external ID, or nullptr if the user doesn't exist locally.
        **************************************************************************************************************/
        const User *getUserByExternalId( ExternalId extid ) const { return getUserByExternalIdInternal(extid); }

        /*! **********************************************************************************************************/
        /*! \brief  Returns the user with the given account identifier
        **************************************************************************************************************/
        const User *getUserByPlatformInfo(const PlatformInfo& platformInfo) const { return getUserByPlatformInfoInternal(platformInfo); }
        const User *getUserByXblAccountId(ExternalXblAccountId xblId) const { return getUserByXblAccountIdInternal(xblId); }
        const User *getUserByPsnAccountId(ExternalPsnAccountId psnId) const { return getUserByPsnAccountIdInternal(psnId); }
        const User *getUserBySteamAccountId(ExternalSteamAccountId steamId) const { return getUserBySteamAccountIdInternal(steamId); }
        const User *getUserByStadiaAccountId(ExternalStadiaAccountId stadiaId) const { return getUserByStadiaAccountIdInternal(stadiaId); }
        const User *getUserBySwitchId(const char8_t* switchId) const { return getUserBySwitchIdInternal(switchId); }
        const User *getUserByNucleusAccountId(ClientPlatformType platform, AccountId accountId) const { return getUserByNucleusAccountIdInternal(platform, accountId); }


        typedef Blaze::vector<LocalUser*> LocalUserVector;
        typedef Blaze::vector<const User*> UserVector;
        typedef Functor3<BlazeError, JobId, const User*> LookupUserCb;
        typedef Functor3<BlazeError, JobId, UserVector&> LookupUsersCb;
        typedef Blaze::vector<BlazeId> BlazeIdVector;
        typedef Blaze::vector<const char8_t*> PersonaNameVector;
        typedef Blaze::vector<ExternalId> ExternalIdVector;
        typedef Blaze::vector<AccountId> AccountIdVector;
        typedef Blaze::vector<OriginPersonaId> OriginPersonaIdVector;
        typedef LookupUsersCb LookupUsersByPrefixCb;    // deprecated ... use LookupUsersCb instead

        class UserVectorWrapper : public UserVector
        {
        public:
            static UserVectorWrapper* createUserVector()
            {
                UserVectorWrapper* userVector = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::userVector") UserVectorWrapper();
                return userVector;
            }
            void AddRef() { ++mRefCount; }
            void Release()
            {
                BlazeAssert(mRefCount != 0);
                if (--mRefCount == 0)
                    BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, this);
            }

        private:
            // UserVectorWrappers are only used to create UserVector smart pointers, and should only be created via createUserVector()
            UserVectorWrapper() : UserVector(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::userVector"), mRefCount(0) {}
            mutable uint32_t mRefCount;
        };
        typedef eastl::intrusive_ptr<UserVectorWrapper> UserVectorPtr;

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup a user given a blazeId. Makes a request against the blaze server if the local
                    user object isn't complete.
            \param[in] blazeId                        The BlazeId of the user to lookup
            \param[in] titleLookupUserCb            The functor dispatched upon success/failure
            \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserByBlazeId( BlazeId blazeId, const LookupUserCb &titleLookupUserCb);

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup a user on the caller's platform given a user name. Makes a request against the blaze server if the local
                    user object isn't complete; Not efficient, prefer lookup by BlazeId when possible.

            WARNING This method finds the user on the caller's platform only. Use lookupUsersByName for cross-platform lookups.

            \param[in] name                            The name of the user to lookup
            \param[in] titleLookupUserCb            The functor dispatched upon success/failure
            \return                                    The JobId of the RPC call
            
        ********************************************************************************/
        JobId lookupUserByName( const char8_t* name, const LookupUserCb &titleLookupUserCb);

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup a user on the caller's platform given a user name. Makes a request against the blaze server if the local
                    user object isn't complete; Not efficient, prefer lookup by BlazeId when possible.

            WARNING This method finds the user on the caller's platform only. Use lookupUsersByName for cross-platform lookups.

            \param[in] personaNamespace             The namespace of the user to lookup, nullptr implies platform default namespace
            \param[in] name                         The name of the user to lookup
            \param[in] titleLookupUserCb            The functor dispatched upon success/failure
            \return                                 The JobId of the RPC call
            
        ********************************************************************************/
        JobId lookupUserByName( const char8_t* personaNamespace, const char8_t* name, const LookupUserCb &titleLookupUserCb);

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup all users given a user name. Makes a request against the blaze server;
        Not efficient, prefer lookup by BlazeId when possible.

            \param[in] name                         The name of the user to lookup
            \param[in] titleLookupUsersCb           The functor dispatched upon success/failure
            \param[in] primaryNamespaceOnly         Whether to search for users in their primary namespace only (xbox for xone users, ps3 for ps4 users, etc.)
            \return                                 The JobId of the RPC call
            
        ********************************************************************************/
        JobId lookupUserByNameMultiNamespace( const char8_t* name, const LookupUsersCb &titleLookupUsersCb, bool primaryNamespaceOnly = true);

        /*! ****************************************************************************/
        /*! \brief (DEPRECATED) Attempt to lookup a user given a external id (XUID). Makes a request against the blaze server if the local
        user object isn't complete; Not efficient, prefer lookup by BlazeId when possible.

        \param[in] extid                        The ExternalId of the user to lookup
        \param[in] titleLookupUserCb            The functor dispatched upon success/failure
        \return                                 The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserByExternalId( ExternalId extid, const LookupUserCb &titleLookupUserCb);

        /*! ****************************************************************************/
        /*! \brief  Attempt to lookup a user given a Account Identifier. Makes a request against the blaze server if the local
        user object isn't complete; Not efficient, prefer lookup by BlazeId when possible.

        \param[in] platformInfo                The PlatformInfo of the user to lookup
        \param[in] titleLookupUserCb            The functor dispatched upon success/failure
        \return                                 The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserByPlatformInfo(const PlatformInfo& platformInfo, const LookupUserCb &titleLookupUserCb);
        JobId lookupUserByPsnId(ExternalPsnAccountId psnId, const LookupUserCb &titleLookupUserCb);
        JobId lookupUserByXblId(ExternalXblAccountId xblId, const LookupUserCb &titleLookupUserCb);
        JobId lookupUserBySteamId(ExternalSteamAccountId steamId, const LookupUserCb &titleLookupUserCb);
        JobId lookupUserByStadiaId(ExternalStadiaAccountId stadiaId, const LookupUserCb &titleLookupUserCb);
        JobId lookupPcUserByNucleusAccountId(AccountId accountId, const LookupUserCb &titleLookupUserCb);
        JobId lookupUserBySwitchId(const char8_t* switchId, const LookupUserCb &titleLookupUserCb);

        /*! ****************************************************************************/
        /*! \brief Lookup users given blaze ids. Makes a request against the Blaze server if the local
        user objects aren't complete. Order of User objects returned by LookupUsersCb may not match 
        order of values in BlazeIdVector.
        (This is caused by multiple levels of caching on the SDK and Blaze server.)
        
        \param[in] blazeIds             The BlazeIds of the users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByBlazeId(const BlazeIdVector *blazeIds, const LookupUsersCb &titleLookupUsersCb);

        /*! ****************************************************************************/
        /*! \brief Lookup users given persona names in the default namespace. Makes a request against
        the Blaze server if the local user objects aren't complete.
        Not efficient, prefer lookup by BlazeId when possible. Order of User 
        objects returned by LookupUsersCb may not match order of values in PersonaNameVector.
        (This is caused by multiple levels of caching on the SDK and Blaze server.)
        
        \param[in] names                The persona names of users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByName( const PersonaNameVector *names, const LookupUsersCb &titleLookupUsersCb);

        /*! ****************************************************************************/
        /*! \brief Lookup users given persona names in a specified namespace. Makes a request against
        the Blaze server if the local user objects aren't complete.
        Not efficient, prefer lookup by BlazeId when possible. Order of User 
        objects returned by LookupUsersCb may not match order of values in PersonaNameVector.
        (This is caused by multiple levels of caching on the SDK and Blaze server.)
        
        \param[in] personaNamespace     The namespace of the user to lookup, nullptr implies platform default namespace
        \param[in] names                The persona names of users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByName( const char8_t* personaNamespace, const PersonaNameVector *names, const LookupUsersCb &titleLookupUsersCb);

        /*! ****************************************************************************/
        /*! \brief Lookup users given persona names in all namespaces. Makes a request against
        the Blaze server if the local user objects aren't complete.
        Not efficient, prefer lookup by BlazeId when possible. Order of User 
        objects returned by LookupUsersCb may not match order of values in PersonaNameVector.
        (This is caused by multiple levels of caching on the SDK and Blaze server.)
        
        \param[in] names                The persona names of users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \param[in] primaryNamespaceOnly Whether to search for users in their primary namespace only (xbox for xone users, ps3 for ps4 users, etc.)
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByNameMultiNamespace( const PersonaNameVector *names, const LookupUsersCb &titleLookupUsersCb, bool primaryNamespaceOnly = true);

        /*! ****************************************************************************/
        /*! \brief Lookup users given a persona name prefix in the default namespace.
        Makes a request against the Blaze server.

        \param[in] prefixName                 The persona name of users to lookup by prefix
        \param[in] maxResultCountPerPlatform  The maximum number of results to return per platform
        \param[in] titleLookupUsersCb         The functor dispatched upon success/failure
        \param[in] primaryNamespaceOnly       Whether to search for users in their primary namespace only (xbox for xone users, ps3 for ps4 users, etc.)
        \return                               The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByNamePrefix( const char8_t* prefixName, const uint32_t maxResultCountPerPlatform, const LookupUsersCb &titleLookupUsersCb, bool primaryNamespaceOnly = true);

        /*! ****************************************************************************/
        /*! \brief Lookup users given a persona name prefix in a specified namespace.
        Makes a request against the Blaze server.

        \param[in] personaNamespace           The namespace of the user to lookup, nullptr implies platform default namespace
        \param[in] prefixName                 The persona name of users to lookup by prefix
        \param[in] maxResultCountPerPlatform  The maximum number of results to return per platform
        \param[in] titleLookupUsersCb         The functor dispatched upon success/failure
        \param[in] primaryNamespaceOnly       Whether to search for users in their primary namespace only (xbox for xone users, ps3 for ps4 users, etc.)
                                              Only applicable when personaNamespace is NAMESPACE_ORIGIN.
        \return                               The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByNamePrefix( const char8_t* personaNamespace, const char8_t* prefixName, const uint32_t maxResultCountPerPlatform, const LookupUsersCb &titleLookupUsersCb, bool primaryNamespaceOnly = true);

        /*! ****************************************************************************/
        /*! \brief Lookup users given a persona name prefix in all namespaces.
        Makes a request against the Blaze server.

        \param[in] prefixName            The persona name of users to lookup by prefix
        \param[in] maxResultCount        The max result count
        \param[in] titleLookupUsersCb    The functor dispatched upon success/failure
        \param[in] primaryNamespaceOnly  Whether to search for users in their primary namespace only (xbox for xone users, ps3 for ps4 users, etc.)
        \return                          The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByNamePrefixMultiNamespace( const char8_t* prefixName, const uint32_t maxResultCount, const LookupUsersCb &titleLookupUsersCb, bool primaryNamespaceOnly = true);

        /*! ****************************************************************************/
        /*! \brief (DEPRECATED) Use lookupUsersCrossPlatform
        Lookup users given external ids. Makes a request against the Blaze server if the local
        user objects aren't complete. Not efficient, prefer lookup by BlazeId when possible. Order of User
        objects returned by LookupUsersCb may not match order of values in ExternalIdVector.
        (This is caused by multiple levels of caching on the Blaze server.)

        \param[in] extids               The ExternalIdList of the users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByExternalId( const ExternalIdVector *extids, const LookupUsersCb &titleLookupUsersCb);

        /*! ****************************************************************************/
        /*! \brief Lookup users given account ids. Always makes a request against the Blaze server. 
        Not efficient, prefer lookup by BlazeId when possible. Order of User objects returned by 
        LookupUsersCb may not match order of values in AccountIdVector.
        (This is caused by multiple levels of caching on the Blaze server.)

        WARNING This method finds users on the caller's platform only. Use lookupUsersCrossPlatform
        for cross-platform lookups.

        \param[in] accids               The AccountIds of the users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByAccountId( const AccountIdVector *accids, const LookupUsersCb &titleLookupUsersCb);

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup a user given an origin persona id. Makes a request against the blaze server if the local
        user object isn't complete; Not efficient, prefer lookup by BlazeId when possible.

        WARNING This method finds users on the caller's platform only. Use lookupUsersCrossPlatform
        for cross-platform lookups.

        \param[in] originPersonaId              The BlazeId of the user to lookup
        \param[in] titleLookupUserCb            The functor dispatched upon success/failure
        \return                                 The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserByOriginPersonaId(OriginPersonaId originPersonaId, const LookupUserCb &titleLookupUserCb);

        /*! ****************************************************************************/
        /*! \brief Lookup users given origin persona ids. Makes a request against the Blaze server if the local
        user objects aren't complete. Not efficient, prefer lookup by BlazeId when possible. Order of User
        objects returned by LookupUsersCb may not match order of values in OriginPersonaIdVector.
        (This is caused by multiple levels of caching on the Blaze server.)

        WARNING This method finds users on the caller's platform only. Use lookupUsersCrossPlatform
        for cross-platform lookups.

        \param[in] originPersonaIds     The OriginPersonaIds of the users to lookup
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersByOriginPersonaId(const OriginPersonaIdVector *originPersonaIds, const LookupUsersCb &titleLookupUsersCb);

        /*! *******************************************************************************************************/
        /*! \brief Lookup users by NucleusAccountId, OriginPersonaId, Origin persona name, or 1st party identifier.
        Not efficient, prefer lookup by BlazeId when possible. Order of User objects returned by
        LookupUsersCb may not match order of values in the LookupUsersCrossPlatformRequest's PlatformInfoList.
        (This is caused by multiple levels of caching on the Blaze server.)

        If ClientPlatformOnly request option is set and the OnlineOnly and MostRecentPlatformOnly request options are
        both unset, then this method will search the BlazeSDK cache and make a request against the Blaze server only
        if the cached user objects aren't complete. Otherwise, this method will make a request against the Blaze server
        without searching the local cache.


        \param[in] request              The RPC request (see the class definition of LookupUsersCrossPlatformRequest for more details about each request field)
        \param[in] titleLookupUsersCb   The functor dispatched upon success/failure
        \return                         The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersCrossPlatform(LookupUsersCrossPlatformRequest& request, const LookupUsersCb &titleLookupUsersCb);

        typedef Functor3<BlazeError, JobId, const User*> FetchUserExtendedDataCb;
        /*! ****************************************************************************/
        /*! \brief Fetch the UserSessionExtended data for the user with the given BlazeId
        \param[in] blazeId                        The BlazeId of the target user
        \param[in] titleFetchUserExtendedDataCb    The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId fetchUserExtendedData(BlazeId blazeId, const FetchUserExtendedDataCb &titleFetchUserExtendedDataCb);
    
        typedef Functor3<BlazeError, JobId, bool> CheckUserOnlineStatusCb;
        /*! ****************************************************************************/
        /*! \brief Check the online status of a given user.
                The target user will have true passed in as the first parameter if the target user
                is logged in.
            \param[in] blazeId                        The BlazeId of the target user
            \param[in] titleCheckOnlineStatusCb     The functor dispatched upon success/failure
            \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId checkUserOnlineStatus(BlazeId blazeId, const CheckUserOnlineStatusCb &titleCheckOnlineStatusCb);

        /*! ************************************************************************************************/
        /*! \brief get qos ping site latency for the user

            \param[in] blazeId - the user id whose qos ping site latency map is going to be retrieved
            \param[out] qosPingSiteLatency - ping site latency keyed by alias
            \return true if the map is filled up or else return false
        ***************************************************************************************************/
        bool getQosPingSitesLatency(const BlazeId blazeId, PingSiteLatencyByAliasMap& qosPingSiteLatency) const;

        typedef Functor2<BlazeError, JobId> OverrideGeoCb;
        /*! ****************************************************************************/
        /*! \brief Override a users GeoIPData. Tells server to not use GeoIP database but 
            specific data provided.
        \param[in] geoData     The data used to override
        \param[in] titleOverrideGeoCb            The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId overrideUsersGeoIPData(const GeoLocationData* geoData, const OverrideGeoCb &titleOverrideGeoCb);

        typedef Functor2<BlazeError, JobId> GeoOptInCb;
        /*! ****************************************************************************/
        /*! \brief Opt-in/out the user's GeoIPData. Tells server to not expose user's GeoIP data
            to others.
        \param[in] optInFlag     The data used to override
        \param[in] titleGeoOptInCb            The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId setUserGeoOptIn(const bool optInFlag, const GeoOptInCb &titleGeoOptInCb);

        typedef Functor3<BlazeError, JobId, const GeoLocationData*> LookupGeoCb;
        /*! ****************************************************************************/
        /*! \brief Looks up a users GeoIPData. 
        \param[in] blazeId     The data used to override
        \param[in] titleLookupGeoCb            The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUsersGeoIPData(const BlazeId blazeId, const LookupGeoCb &titleLookupGeoCb);

    #ifdef BLAZE_USER_PROFILE_SUPPORT
        typedef Functor3<BlazeError, JobId, const UserProfilePtr> LookupUserProfileCb;
        typedef Blaze::vector<UserProfilePtr> UserProfileVector;
        typedef Functor3<BlazeError, JobId, const UserProfileVector&> LookupUserProfilesCb;

        /*! ****************************************************************************/
        /*! \brief Lookup a single user profile given User object.
            Makes a request against 1st party service to retrieve a user profile if it is not already cached.
            It is expected that the User object has suitable values that will be used
            in the lookup request.  On Xbox One, we use the ExternalId, on PS4 we use the PersonaName.

            \param[in] user         The User object identifying the user to a UserProfile for
            \param[in] titleCb      The functor dispatched upon success/failure
            \param[in] timeoutMs    The number of milliseconds before the request times out
            \param[in] forceUpdate  If false (default), try to find the user from our cache first;
                                    or if true, just request from 1st party (which can then update our cache)
            \return                 The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserProfileByUser(const User &user, const LookupUserProfileCb &titleCb, int32_t timeoutMs = USERPROFILE_TIMEOUT_MS, bool forceUpdate = false);

        /*! ****************************************************************************/
        /*! \brief Lookup user profiles given User objects.
            Makes a request against 1st party service to retrieve user profiles that are not already cached.
            It is expected that the User objects have suitable values that will be used
            in the lookup request.  On Xbox One, we use the ExternalId, on PS4 we use the PersonaName.

            \param[in] users        The User objects of the user profiles to lookup
            \param[in] titleCb      The functor dispatched upon success/failure
            \param[in] timeoutMs    The number of milliseconds before the request times out
            \param[in] forceUpdate  If false (default), try to find the user from our cache first;
                                    or if true, just request from 1st party (which can then update our cache)
            \return                 The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserProfilesByUser(const UserVector &users, const LookupUserProfilesCb &titleCb, int32_t timeoutMs = USERPROFILE_TIMEOUT_MS, bool forceUpdate = false);

        /*! ****************************************************************************/
        /*! \brief Lookup a single UserProfile given a suitable FirstPartyId.
            Makes a request against 1st party service to retrieve a user profile if it is not already cached.
            The 'lookupId' parameter is different depending on your platform. On Xbox One it is the user's XUID (e.g. ExternalId),
            on PS4 it is the user's AccountId.

            \param[in] lookupId     The FirstPartyLookupId of the user. On Xbox One, this is the XUID (e.g. ExternalId), on PS4 this is the AccountId.
            \param[in] titleCb      The functor dispatched upon success/failure
            \param[in] timeoutMs    The number of milliseconds before the request times out
            \param[in] forceUpdate  If false (default), try to find the user from our cache first;
                                    or if true, just request from 1st party (which can then update our cache)
            \param[in] avatarSize   The size of avatar to be retrieved. Since our user profile cache only stores the most recently fetched avatar size, 
                                    any request for a different size than previous most recently fetched one will REQUIRE a forceUpdate of true.
            \return                 The JobId of the RPC call
        ********************************************************************************/
        JobId lookupUserProfileByFirstPartyLookupId(FirstPartyLookupId lookupId, const LookupUserProfileCb &titleCb, int32_t timeoutMs = USERPROFILE_TIMEOUT_MS, bool forceUpdate = false, int32_t avatarSize = 's');

        /*! ****************************************************************************/
        /*! \brief Lookup a multiple UserProfiles given a list of FirstPartyIds.
            Makes a request against 1st party service to retrieve user profiles that are not already cached.
            The 'lookupIdList' parameter is a list of FirstPartyLookupIds, which is different depending on
            your platform. On Xbox One this is the XUID (e.g. ExternalId), on PS4 this is the AccountId.

            \param[in] lookupIdList A list of FirtyPartyIds of the users to lookup. On Xbox One, this contains ExternalIdList, on PS4 this contains PersonaNames.
            \param[in] titleCb      The functor dispatched upon success/failure
            \param[in] timeoutMs    The number of milliseconds before the request times out
            \param[in] forceUpdate  If false (default), try to find the user from our cache first;
                                    or if true, just request from 1st party (which can then update our cache)
            \param[in] avatarSize   The size of avatar to be retrieved. Since our user profile cache only stores the most recently fetched avatar size, 
                                    any request for a different size than previous most recently fetched one will REQUIRE a forceUpdate of true.
            \return                 The JobId of the RPC call
        ********************************************************************************/
        typedef Blaze::vector<FirstPartyLookupId> FirstPartyLookupIdVector;
        JobId lookupUserProfilesByFirstPartyLookupId(const FirstPartyLookupIdVector &lookupIdList, const LookupUserProfilesCb &titleCb, int32_t timeoutMs = USERPROFILE_TIMEOUT_MS, bool forceUpdate = false, int32_t avatarSize = 's');
    #endif

        //! \cond INTERNAL_DOCS
        // we listen for the loginManager's state changes, in order to init the local player
        void onLocalUserAuthenticated(uint32_t userIndex);
        void onLocalUserDeAuthenticated(uint32_t userIndex);
        void onLocalUserForcedDeAuthenticated(uint32_t userIndex, UserSessionForcedLogoutType forcedLogoutType);
        //! \endcond

        //TODO - PS3 has issue making this private and using a friend - make it public for now.
        UserManager(BlazeHub *blazeHub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        /*! **********************************************************************************************************/
        /*! \name Destructor
        **************************************************************************************************************/
        virtual ~UserManager();

        /*! ****************************************************************************/
        /*! \brief Customize the interval used for re-fetching cached User information from the server. 
            \note Cannot be set to a value less than MIN_CACHED_USER_REFRESH_INTERVAL_MS.
            \param cachedUserRefreshIntervalMs  The interval used by the client to determine whether CACHED user
            info has expired and needs to be re-fetched from the Blaze server.
        ********************************************************************************/
        void setCachedUserRefreshInterval(uint32_t cachedUserRefreshIntervalMs);

        /*! ****************************************************************************/
        /*! \brief Clears the cache of all looked-up users.
        ********************************************************************************/
        void clearLookupCache();

        /*! ****************************************************************************/
        /*! \brief Releases storage for any User objects that are not owned or cached
        ********************************************************************************/
        void trimUserPool();

        /*! ****************************************************************************/
        /*! \brief Allocates storage for User objects up to amount specified by MaxCachedUserCount
        ********************************************************************************/
        void reserveUserPool();

        /*! ****************************************************************************/
        /*! \brief Adds a listener to receive async notifications when extended
                user info data chagnes

            \param listener The listener to add.
        ********************************************************************************/
        void addListener(ExtendedUserDataInfoChangeListener *listener);

        /*! ****************************************************************************/
        /*! \brief Removes a listener to async notifications when extended
                user info data chagnes

            \param listener The listener to remove.
        ********************************************************************************/
        void removeListener(ExtendedUserDataInfoChangeListener *listener);

        /*! ************************************************************************************************/
        /*! \brief Adds a listener to receive async notifications when the primary local user has changed.
        
            \param[in] listener The listener to add.
        ***************************************************************************************************/
        void addPrimaryUserListener(PrimaryLocalUserListener *listener);

        /*! ************************************************************************************************/
        /*! \brief Removes a listener to receive async notifications when the primary local user has changed.
        
            \param[in] listener The listener to add.
        ***************************************************************************************************/
        void removePrimaryUserListener(PrimaryLocalUserListener *listener);

        /*! ************************************************************************************************/
        /*! \brief Adds a listener to receive async notifications when a user has authenticated
        
            \param[in] listener The listener to add.
        ***************************************************************************************************/
        void addUserManagerStateListener(UserManagerStateListener *listener) { mStateDispatcher.addDispatchee(listener); }

        /*! ************************************************************************************************/
        /*! \brief Removes a listener to receive async notifications when a user has deauthenticated
        
            \param[in] listener The listener to add.
        ***************************************************************************************************/
        void removeUserManagerStateListener(UserManagerStateListener *listener) { mStateDispatcher.removeDispatchee(listener); }

        /*! ****************************************************************************/
        /*! \brief Checks if we have any local users.

        \return true if we have local users. false otherwise. 
        ********************************************************************************/
        bool hasLocalUsers() const;


        /*! ************************************************************************************************/
        /*! \brief get the UserManager's current census data.  The data is only updated periodically by the CensusDataAPI;
                you should wait for a dispatch from CensusDataAPIListener::onNotifyCensusData before accessing
                the data.       
            
            You should avoid caching off pointers from inside the object, as they can potentially be invalidated
            during a call to BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
            cause the pointer to be invalidated.

            \return the current UserManagerCensusData, can be nullptr if the TDF is not found.
        ***************************************************************************************************/
        const UserManagerCensusData* getCensusData() const;

        LocalUserGroup& getLocalUserGroup() { return mLocalUserGroup; }

    protected:
        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);

    private:
        NON_COPYABLE(UserManager);
    
        /*! ************************************************************************************************/
        /*! \brief Internal callback for processing the response from a fetch of UserSessionExtendedData
                It attaches the extended data to a local user object, creating one if needed.
        
            \param[in] response                        The server's UserData response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                        The functor dispatched upon completion
            \param[in] blazeId                      The blaze id for the fetched user

        ***************************************************************************************************/
        void internalFetchExtendedDataCb(const Blaze::UserData *response, BlazeError error, JobId jobId, FetchUserExtendedDataCb titleCb);

        /*! ************************************************************************************************/
        /*! \brief Internal callback for processing the response from a fetch of OverrideUserGeoIPData
                
        
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                        The functor dispatched upon completion

        ***************************************************************************************************/
        void internalOverrideUserGeoIPDataCb(BlazeError error, JobId jobId, OverrideGeoCb titleCb);

        /*! ************************************************************************************************/
        /*! \brief Internal callback for processing the response from a fetch of LookupUserGeoIPData
                
            \param[in] response                     The server's GeoLocationData response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                      The functor dispatched upon completion

        ***************************************************************************************************/
        void internalLookupUserGeoIPDataCb(const Blaze::GeoLocationData *response, BlazeError error, JobId jobId, LookupGeoCb titleCb);

        /*! ****************************************************************************/
        /*! \brief Internal callback for processing the response from a check of online status.
        
        \param[in] response                       The server's UserData response
        \param[in] error                        The Blaze error code returned
        \param[in] jobId                        The JobId of the returning lookup
        \param[in] titleCb                        The functor dispatched upon completion

        ********************************************************************************/
        void internalCheckUserOnlineStatusCb(const Blaze::UserData *response, BlazeError error, JobId jobId, CheckUserOnlineStatusCb titleCb);

        /*! ****************************************************************************/
        /*! \brief Internal callback for processing the response from a lookup request
                    used for both lookupUserByBlazeId, lookupUserByName and lookupUserByExternalId.
                    Handles caching the user object created within UserManager

            \param[in] response                        The server's UserData response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                        The functor dispatched upon completion
            
        ********************************************************************************/
        void internalLookupUserCb(const Blaze::UserData *response, BlazeError error, JobId jobId, LookupUserCb titleCb);

        /*! ****************************************************************************/
        /*! \brief Internal callback for processing the response from a lookup users request
        used for lookupUsersByBlazeId, lookupUsersByName and lookupUsersByExternalId.
        Handles caching the user object created within UserManager

            \param[in] response                        The server's UserData response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                        The functor dispatched upon completion
            \param[in] cachedUsers                  Vector if users that didn't require a server lookup.

        ********************************************************************************/
        void internalLookupUsersCb(const Blaze::UserDataResponse *response, BlazeError error, JobId jobId, LookupUsersCb titleCb, UserVectorPtr cachedUsers);

        /*! ****************************************************************************/
        /*! \brief Internal callback for processing the response from a lookupUsersCrossPlatform request that returns a single user.
        Used for lookupUserByPlatformInfo and related methods.
        Handles caching the user object created within UserManager.

            \param[in] response                        The server's UserData response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                        The functor dispatched upon completion
            \param[in] caller                       The name of the calling method (for logging purposes)

        ********************************************************************************/
        void internalLookupUserByPlatformInfoCb(const Blaze::UserDataResponse *response, BlazeError error, JobId jobId, LookupUserCb titleCb, const char8_t* caller);

        /*! ************************************************************************************************/
        /*! \brief Create (or update) and fetch the User with the supplied blazeId.
            - Takes ownership by incrementing the reference count if newUserType == User::OWNED
            - Does not take ownership or increment the reference count if newUserType == User::CACHED
            \note
            - User objects returned by acquireUser() are only guranteed valid while the UserManager API exists; 
            therefore, it is @em wrong to dereference any cached User* pointers inside API destructors or anywhere
            the UserManager can no longer be assumed to be valid.
            \param[in] blazeId the blazeId of the user to add/update
            \param[in] platformInfo info on the platforms currently in use
            \param[in] isPrimaryPersona Indicates that the user is a primary persona, and therefore can be cached by AccountId and OriginPersonaName/Id
            \param[in] personaNamespace the namespace the persona name is in
            \param[in] userName the name of the user (persona name)
            \param[in] accountLocale The locale for the user.
            \param[in] accountCountry The country for the user.
            \param[in] statusFlags User status flag (currently only stores online status)
            \param[in] newUserType Indicates if the user is owned or cached
            \return The updated user
        ***************************************************************************************************/
        User* acquireUser(
            BlazeId blazeId,
            const PlatformInfo& platformInfo,
            bool isPrimaryPersona,
            const char8_t *personaNamespace,
            const char8_t *userName,
            Locale accountLocale,
            uint32_t accountCountry,
            const UserDataFlags *statusFlags,
            User::UserType newUserType
        );

        User *acquireUser(const Blaze::UserIdentification &data, User::UserType newUserType);
        
        User *acquireUser(const Blaze::UserData* data, User::UserType newUserType);

        /*! ************************************************************************************************/
        /*! \brief Destroy (or update the refCount of) the User with the supplied blazeId.
            \param[in] user the user to remove
        ***************************************************************************************************/
        void releaseUser(const User* user);

        /*! ************************************************************************************************/
        /*! \brief Check if the User object meets the validity criteria:
            - Is not nullptr
            - Is complete
            - Is up to date (if it is CACHED)
            \note The assumption is that if a user is OWNED it will be kept up to date via regular updates.
            Meanwhile, CACHED users may become stale because the Blaze server does not keep track of such 
            looked-up user objects. This means that we must periodically refresh the data inside our CACHED
            users when it becomes stale. The 'staleness' period is governed by a configurable UserManager
            parameter mStaleUserThreshold. The minimum value of this parameter is 1/4 sec.
            \param[in] user - the user to validate
        ***************************************************************************************************/
        bool isValidUser(const User* user) const;

        void setLocalUserCanTransitionToAuthenticated(uint32_t userIndex, bool canTransitionToAuthenticated);

        /*! ************************************************************************************************/
        /*! \brief Internal version of getUser; returns non-const user.
            \param[in] blazeId the id of the user to lookup
            \return a User pointer, or nullptr if the user wasn't found.
        ***************************************************************************************************/
        User* getUser( BlazeId blazeId ) const;

        /*! **********************************************************************************************************/
        /*! \brief Internal version of getUserByName; returns non-const user. scan the user map and return the user with the 
            supplied name (if one exists on the caller's platform).    Note: inefficient, prefer getUserById.  names are not case sensitive.
            \param[in] name the name of the user to get.  Case insensitive.
            \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
        **************************************************************************************************************/
        User* getUser( const char8_t* name ) const;

        /*! **********************************************************************************************************/
        /*! \brief Internal version of getUserByName; returns non-const user. scan the user map and return the user with the 
            supplied name (if one exists on the caller's platform).    Note: inefficient, prefer getUserById.  names are not case sensitive.
            \param[in] personaNamespace             The namespace of the user to lookup, nullptr implies platform default namespace
            \param[in] name the name of the user to get.  Case insensitive.
            \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
        **************************************************************************************************************/
        User* getUser( const char8_t* personaNamespace, const char8_t* name ) const;

        /*! **********************************************************************************************************/
        /*! \brief Internal version of getUserByName; returns non-const user. scan the user map and return the user with the
            supplied name and platform (if one exists).    Note: inefficient, prefer getUserById.  names are not case sensitive.
            \param[in] personaNamespace             The namespace of the user to lookup, nullptr implies platform default namespace
            \param[in] name the name of the user to get.  Case insensitive.
            \param[in] platform the platform of the user to get. INVALID implies caller's platform.
            \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
        **************************************************************************************************************/
        User* getUser(const char8_t* personaNamespace, const char8_t* name, ClientPlatformType platform) const;

        /*! **********************************************************************************************************/
        /*! \brief Helper method for lookupUsersByName; adds all cached users for the given persona name and namespace to userVector.
            \param[in] personaNamespace             The namespace of the users to lookup, nullptr implies platform default namespace
            \param[in] name the name of the users to get.  Case insensitive.
            \return true if at least one user was found
        **************************************************************************************************************/
        bool getUsers(const char8_t* personaNamespace, const char8_t* name, UserVectorPtr& userVector) const;

        /*! **********************************************************************************************************/
        /*! \brief Helper method. Replaces or deletes the cached BlazeId of the user with the
            supplied name and platform.
            \param[in] personaNamespace the namespace of the local cache entry.
            \param[in] name the name of the local cache entry.  Case insensitive.
            \param[in] platform the platform of the local cache entry.
            \param[in] newBlazeId the BlazeId to set in the local cache. If newBlazeId is INVALID_BLAZE_ID,
                                  then the cache entry is deleted instead of replaced.
        **************************************************************************************************************/
        void replaceOrDeleteUserId(const char8_t* personaNamespace, const char8_t* name, ClientPlatformType platform, BlazeId newBlazeId);

        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Internal version of getUserByExternalId; returns non-const user. Returns the user with the given external 
            identifier (  inefficient, prefer getUserById if possible.)

            Examples of external identifiers include XboxUid for the Xbox, or PSN 'Account Id' for the PS4.

            \param[in] extid The external ID for the user (if applicable.)
            \return the User obj with the associated external ID, or nullptr if the user doesn't exist locally.
        **************************************************************************************************************/
        User* getUserByExternalIdInternal(ExternalId extid) const;

        /*! **********************************************************************************************************/
        /*! \brief Internal version of getUserByPlatformInfo; returns non-const user. Returns the user with the given account
            identifier (  inefficient, prefer getUserById if possible.)

            Account identifiers include XboxUid for the Xbox, PSN 'Account Id' for the PS4, or NSA id with environment prefix for the Nintendo Switch.  

            \param[in] extid The external ID for the user (if applicable.)
            \return the User obj with the associated external ID, or nullptr if the user doesn't exist locally.
        **************************************************************************************************************/
        User *getUserByPlatformInfoInternal(const PlatformInfo& platformInfo) const;
        User *getUserByXblAccountIdInternal(ExternalXblAccountId xblId) const;
        User *getUserByPsnAccountIdInternal(ExternalPsnAccountId psnId) const;
        User *getUserBySteamAccountIdInternal(ExternalSteamAccountId steamId) const;
        User *getUserByStadiaAccountIdInternal(ExternalStadiaAccountId stadiaId) const;
        User *getUserBySwitchIdInternal(const char8_t* switchId) const;
        User *getUserByNucleusAccountIdInternal(ClientPlatformType platform, AccountId accountId) const;
        User *getUserByOriginPersonaIdInternal(ClientPlatformType platform, OriginPersonaId originPersonaId) const;
        
        /*! ****************************************************************************/
        /*! \brief Attempt to lookup users given identity vector. Makes a request against the blaze server if the local
            user objects aren't complete; Not efficient, prefer lookup by BlazeId when possible.

            \param[in] request                        The request containing users to lookup
            \param[in] users                        The vector containing cached users
            \param[in] titleLookupUserCb            The functor dispatched upon success/failure
            \return                                    The JobId of the RPC call
        ********************************************************************************/
        JobId internalLookupUsers(const LookupUsersRequest& request, UserVectorPtr users, const LookupUsersCb &titleLookupUsersCb);

        /*! ****************************************************************************/
        /*! \brief  Attempt to lookup a user given a Account Identifier. Makes a request against the blaze server if the local
            user object isn't complete; Not efficient, prefer lookup by BlazeId when possible.

            \param[in] platformInfo                The PlatformInfo of the user to lookup
            \param[in] titleLookupUserCb           The functor dispatched upon success/failure
            \param[in] caller                      The name of the calling method (for logging purposes)
            \return                                The JobId of the RPC call
        ********************************************************************************/
        JobId internalLookupUserByPlatformInfo(const PlatformInfo& platformInfo, const LookupUserCb &titleLookupUserCb, const char8_t* caller);

#ifdef BLAZE_USER_PROFILE_SUPPORT
        /*! ****************************************************************************/
        /*! \brief Returns the user profile with the given external identifier

            Examples of external identifiers include XboxUid for the Xbox 360.

            \param[in] extId The external ID for the user (if applicable)
            \return the UserProfile obj with the associated external ID, or nullptr if the user doesn't exist locally.
        ********************************************************************************/
        UserProfilePtr getUserProfile(FirstPartyLookupId lookupId) const;

        void resetUserProfileCache();
#endif

        /*! ***********************************************************************/
        /*! \brief callback for UserSessionExtendedData notification from server
            \param[in] update - The extended data update
        ***************************************************************************/
        void onExtendedDataUpdated(const UserSessionExtendedDataUpdate* update, uint32_t userIndex);

        /*! ***********************************************************************/
        /*! \brief Callback for UserAdded notification from server
            \param[in] userAdded - Information about the user that was added.
        ***************************************************************************/
        void onUserAdded(const NotifyUserAdded* userAdded, uint32_t userIndex);

        /*! ***********************************************************************/
        /*! \brief Callback for UserUpdated notification from server
            \param[in] userStatus - Information about the user that was updated.
        ***************************************************************************/
        void onUserUpdated(const UserStatus* userStatus, uint32_t userIndex);

        /*! ***********************************************************************/
        /*! \brief Callback for UserRemoved notification from server

            \param[in] userRemoved - Information about the user that was removed
        ***************************************************************************/
        void onUserRemoved(const NotifyUserRemoved* userRemoved, uint32_t userIndex);

        /*! ***********************************************************************/
        /*! \brief Callback for NotifyQosSettingsUpdated notification from server

            \param[in] qosConfigInfo - The new qos settings from the server
        ***************************************************************************/
        void onNotifyQosSettingsUpdated(const QosConfigInfo* qosConfigInfo, uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief Chooses a new primary local user, excluding the previous primary user.
        
            \param[in] previousPrimaryIndex - the previous primary index, which will be excluded from possible
                new primary index.
        ***************************************************************************************************/
        bool chooseNewPrimaryLocalUser(uint32_t previousPrimaryIndex);
        
        /*! ***********************************************************************/
        /*! \brief Callback for UserAuthenticated notification from server

            \param[in] userSessionLoginInfo - Information about the authenticated user
        ***************************************************************************/
        void onUserAuthenticated(const UserSessionLoginInfo* userSessionLoginInfo, uint32_t userIndex);

        /*! ***********************************************************************/
        /*! \brief Callback for UserUnauthenticated notification from server

            \param[in] userSessionLoginInfo - Information about the unauthenticated user
        ***************************************************************************/
        void onUserUnauthenticated(const UserSessionLogoutInfo* userSessionLogoutInfo, uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief The BlazeSDK has lost connection to the blaze server.  This is user independent, as
            all local users share the same connection.
            \param errorCode - the error describing why the disconnect occured.
        ***************************************************************************************************/
        virtual void onDisconnected(BlazeError errorCode);

        bool isGuestUser(BlazeId blazeId) const { return (blazeId < 0); }

    private:
        class InternalUsersLookupCbJob : public Job
        {
        public:
            InternalUsersLookupCbJob(UserManager* u, BlazeError error, JobId jobId, LookupUsersCb titleCb, UserVectorPtr cachedUsers)
                : mUserManager(u), mError(error), mJobId(jobId), mTitleCb(titleCb), mCachedUsers(cachedUsers)
            {
                setAssociatedTitleCb(mTitleCb);
            }

            void execute()
            {
                mUserManager->internalLookupUsersCb(nullptr, mError, mJobId, mTitleCb, mCachedUsers);
            }

            void cancel(BlazeError err)
            {
                mUserManager->internalLookupUsersCb(nullptr, err, mJobId, mTitleCb, mCachedUsers);
            }

        private:
            UserManager* mUserManager;
            BlazeError mError;
            JobId mJobId;
            LookupUsersCb mTitleCb;
            UserVectorPtr mCachedUsers;

            NON_COPYABLE(InternalUsersLookupCbJob);
        };

        struct PersonaHash
        {
            uint32_t operator()(const char8_t* name) const;
        };

        struct PersonaEqualTo
        {
            bool operator()(const char8_t* name1, const char8_t* name2) const;
        };

        typedef Blaze::hash_map<BlazeId, User*, eastl::hash<BlazeId>, eastl::equal_to<BlazeId> > UserMap;
        typedef eastl::intrusive_list<UserNode> UserList;
        typedef Blaze::map<ClientPlatformType, BlazeId> IdByPlatformMap;
        typedef Blaze::hash_map<const char8_t*, IdByPlatformMap*, PersonaHash, PersonaEqualTo> IdByPlatformByNameMap;

        typedef Blaze::hash_map<Blaze::string, IdByPlatformByNameMap*, CaseInsensitiveStringHash > IdByPlatformByNameByNamespaceMap;
        typedef Blaze::hash_map<ExternalPsnAccountId, BlazeId, eastl::hash<ExternalPsnAccountId>, eastl::equal_to<ExternalPsnAccountId> > IdByExtPsnIdMap;
        typedef Blaze::hash_map<ExternalXblAccountId, BlazeId, eastl::hash<ExternalXblAccountId>, eastl::equal_to<ExternalXblAccountId> > IdByExtXblIdMap;
        typedef Blaze::hash_map<ExternalSteamAccountId, BlazeId, eastl::hash<ExternalSteamAccountId>, eastl::equal_to<ExternalSteamAccountId> > IdByExtSteamIdMap;
        typedef Blaze::hash_map<ExternalStadiaAccountId, BlazeId, eastl::hash<ExternalStadiaAccountId>, eastl::equal_to<ExternalStadiaAccountId> > IdByExtStadiaIdMap;
        typedef Blaze::hash_map<ExternalSwitchId, BlazeId, EA::TDF::TdfStringHash, eastl::equal_to<ExternalSwitchId> > IdByExtSwitchIdMap;
        typedef Blaze::hash_map<AccountId, BlazeId, eastl::hash<AccountId>, eastl::equal_to<AccountId> > IdByAccountIdMap;
        typedef Blaze::map<ClientPlatformType, IdByAccountIdMap*> IdByAccountIdByPlatformMap;
        typedef Blaze::hash_map<OriginPersonaId, BlazeId, eastl::hash<OriginPersonaId>, eastl::equal_to<OriginPersonaId> > IdByOriginPersonaIdMap;
        typedef Blaze::map<ClientPlatformType, IdByOriginPersonaIdMap*> IdByOriginPersonaIdByPlatformMap;

        typedef Dispatcher<UserEventListener> UserEventDispatcher;
        UserEventDispatcher mUserEventDispatcher;
        
        typedef Dispatcher<PrimaryLocalUserListener> PrimaryLocalUserDispatcher;
        PrimaryLocalUserDispatcher mPrimaryLocalUserDispatcher;

        typedef Dispatcher<UserManagerStateListener> UserManagerStateDispatcher;
        UserManagerStateDispatcher mStateDispatcher;

        BlazeHub *mBlazeHub;
        MemList<User> mUserPool;
        LocalUserVector mLocalUserVector;
        LocalUserGroup mLocalUserGroup;
        UserMap mUserMap;
        UserList mOwnedList;
        UserList mCachedList;
        IdByPlatformByNameByNamespaceMap mCachedByNameByNamespaceMap; // cached or owned
        IdByExtPsnIdMap mCachedByExtPsnIdMap; // cached or owned
        IdByExtXblIdMap mCachedByExtXblIdMap; // cached or owned
        IdByExtSteamIdMap mCachedByExtSteamIdMap; // cached or owned
        IdByExtStadiaIdMap mCachedByExtStadiaIdMap; // cached or owned
        IdByExtSwitchIdMap mCachedByExtSwitchIdMap; // cached or owned
        IdByAccountIdByPlatformMap mCachedByAccountIdMap;
        IdByOriginPersonaIdByPlatformMap mCachedByOriginPersonaIdMap;
        uint32_t mUserCount;
        uint32_t mMaxCachedUserCount;
        uint32_t mMaxCachedUserProfileCount;
        uint32_t mCachedUserRefreshIntervalMs;
        uint32_t mPrimaryLocalUserIndex;

#ifdef BLAZE_USER_PROFILE_SUPPORT
        // Used to keep track of data relevant to an async UserProfile lookup request
        struct UserApiCallbackData
        {
            UserApiCallbackData()
                : mUserManager(nullptr), mUserIndex(0), mCachedUserProfiles(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::UserApiCallbackData") {}
            UserManager *mUserManager;
            int32_t mUserIndex;
            LookupUserProfilesCb mTitleMultiCb;
            LookupUserProfileCb mTitleSingleCb;
            JobId mJobId;
            UserProfileVector mCachedUserProfiles;
        };

        typedef Blaze::hash_set<UserApiCallbackData*> UserApiCallbackDataSet;
        UserApiCallbackDataSet mUserApiCallbackDataSet;

        // This is used primarily as a timeout job, or facilites a manual cancel() via JobScheduler
        class InternalLookupUserProfilesJob : public Job
        {
        public:
            InternalLookupUserProfilesJob(UserApiCallbackData* userApiCallbackData)
                : mUserApiCallbackData(userApiCallbackData) 
            {
                if (userApiCallbackData)
                {
                    // Only one of the callbacks is actually used:
                    if (userApiCallbackData->mTitleMultiCb.isValid())
                    {
                        setAssociatedTitleCb(userApiCallbackData->mTitleMultiCb);
                    }
                    else if (userApiCallbackData->mTitleSingleCb.isValid())
                    {
                        setAssociatedTitleCb(userApiCallbackData->mTitleSingleCb);
                    }
                }
            }
            virtual void execute();
            virtual void cancel(BlazeError err);
        private:
            UserApiCallbackData* mUserApiCallbackData;
            NON_COPYABLE(InternalLookupUserProfilesJob);
        };

        // Provided by DirtySDK
        UserApiRefT *mUserApiRef;

        typedef eastl::intrusive_list<UserProfile> UserProfileList;

        typedef Blaze::hash_map<FirstPartyLookupId, UserProfilePtr, eastl::hash<ExternalId>, eastl::equal_to<ExternalId> > UserProfileByFirstPartyLookupIdMap;

        // List used for LRU semantics
        UserProfileList mCachedUserProfileList;

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) 
        // Maps used for lookup, and to maintiain a ref-count on UserProfilePtrs stored in the LRU cache
        UserProfileByFirstPartyLookupIdMap mUserProfileByLookupIdMap;
#endif

        // Callback called by the DirtySDK UserApi
        void userApiCallback(UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, UserApiCallbackData* userApiCallbackData);
        static void _userApiCallback(UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, void *pUserData);

        // General purpose functions to kick a UserProfile lookup request using DirtySDKs UserApi
        JobId internalLookupUserProfiles(const FirstPartyLookupIdVector &idList, const LookupUserProfilesCb* titleMultiCb, const LookupUserProfileCb* titleSingleCb, int32_t timeoutMs, bool forceUpdate, int32_t avatarSize = 's');
        void internalLookupUserProfilesCb(BlazeError error, UserApiCallbackData* userApiCallbackData);
#endif
    };

} // namespace UserManager
} // namespace Blaze
#endif // BLAZE_USER_MANAGER_USER_MANAGER_H
