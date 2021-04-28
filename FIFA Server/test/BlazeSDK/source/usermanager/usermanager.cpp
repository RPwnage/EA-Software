/**************************************************************************************************/
/*! 
    \file usermanager.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/censusdata/censusdataapi.h"

#include "DirtySDK/dirtysock.h" //For NetTick()

#include "DirtySDK/dirtysock/netconn.h" // must include after "BlazeSDK/internal/internal.h"
#include "DirtySDK/dirtysock/dirtynames.h"

#ifdef BLAZE_USER_PROFILE_SUPPORT
    #include "DirtySDK/misc/userapi.h"
#endif

//lint -e1060  Lint can't seem to figure out that UserManager and User are friends.

namespace Blaze
{
namespace UserManager
{
    UserManager::UserManager(BlazeHub *blazeHub, MemoryGroupId memGroupId) :
        mBlazeHub(blazeHub),
        mUserPool(memGroupId),
        mLocalUserVector(memGroupId, blazeHub->getNumUsers(), MEM_NAME(memGroupId, "UserManager::mLocalUserVector")),
        mLocalUserGroup(blazeHub, memGroupId),
        mUserMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mUserMap")),
        mCachedByNameByNamespaceMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByNameByNamespaceMap")),
        mCachedByExtPsnIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByExtPsnIdMap")),
        mCachedByExtXblIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByExtXblIdMap")),
        mCachedByExtSteamIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByExtSteamIdMap")),
        mCachedByExtStadiaIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByExtStadiaIdMap")),
        mCachedByExtSwitchIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByExtSwitchIdMap")),
        mCachedByAccountIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByAccountIdMap")),
        mCachedByOriginPersonaIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mCachedByOriginPersonaIdMap")),
        mUserCount(0),
        mMaxCachedUserCount(mBlazeHub->getInitParams().MaxCachedUserCount),
        mMaxCachedUserProfileCount(mBlazeHub->getInitParams().MaxCachedUserProfileCount),
        mCachedUserRefreshIntervalMs(1000 /* default interval is 1 sec */),
        mPrimaryLocalUserIndex(0)
#ifdef BLAZE_USER_PROFILE_SUPPORT
        ,
        mUserApiCallbackDataSet(memGroupId, MEM_NAME(memGroupId, "UserManager::mUserApiCallbackDataSet")),
        mUserApiRef(nullptr),
        mUserProfileByLookupIdMap(memGroupId, MEM_NAME(memGroupId, "UserManager::mUserProfileByLookupIdMap"))
#endif
    {
        BlazeAssertMsg(blazeHub != nullptr, "Blaze hub must not be nullptr!");
        mBlazeHub->addUserStateFrameworkEventHandler(this);
        for (uint32_t userIndex = 0; userIndex < mBlazeHub->getNumUsers(); userIndex++)
        {         
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager(userIndex)->getUserSessionsComponent();
            userSessions->setUserSessionExtendedDataUpdateHandler(UserSessionsComponent::UserSessionExtendedDataUpdateCb(this, &UserManager::onExtendedDataUpdated));
            userSessions->setUserAddedHandler(UserSessionsComponent::UserAddedCb(this, &UserManager::onUserAdded));
            userSessions->setUserUpdatedHandler(UserSessionsComponent::UserUpdatedCb(this, &UserManager::onUserUpdated));
            userSessions->setUserRemovedHandler(UserSessionsComponent::UserRemovedCb(this, &UserManager::onUserRemoved));
            userSessions->setUserAuthenticatedHandler(UserSessionsComponent::UserAuthenticatedCb(this, &UserManager::onUserAuthenticated));
            userSessions->setUserUnauthenticatedHandler(UserSessionsComponent::UserUnauthenticatedCb(this, &UserManager::onUserUnauthenticated));
            userSessions->setNotifyQosSettingsUpdatedHandler(UserSessionsComponent::NotifyQosSettingsUpdatedCb(this, &UserManager::onNotifyQosSettingsUpdated));
        }
        if(mMaxCachedUserCount == 0)
        {
            BLAZE_SDK_DEBUGF(
                "[UserManager] MaxCachedUserCount not specified in BlazeHub::InitParameters,"
                " using default MAX_CACHED_USERS == %" PRIu32 " instead\n", MAX_CACHED_USERS
            );
            mMaxCachedUserCount = MAX_CACHED_USERS;
        }
        reserveUserPool();

        if(mMaxCachedUserProfileCount == 0)
        {
            BLAZE_SDK_DEBUGF(
                "[UserManager] MaxCachedUserProfileCount not specified in BlazeHub::InitParameters,"
                " using default MAX_CACHED_USER_PROFILES == %" PRIu32 " instead\n", MAX_CACHED_USER_PROFILES
            );
            mMaxCachedUserProfileCount = MAX_CACHED_USER_PROFILES;
        }

#ifdef BLAZE_USER_PROFILE_SUPPORT
        mUserApiRef = UserApiCreate();
        if (mUserApiRef != nullptr)
        {
            UserApiControl(mUserApiRef, 'rrsp', mBlazeHub->getInitParams().IncludeProfileRawData ? 1 : 0, 0, nullptr); // Disable/enable raw response
            // Add idler so that we can call UserApiUpdate()
            mBlazeHub->addIdler(this, "UserManager");
        }
        else
        {
            BLAZE_SDK_DEBUGF("[UserManager] Warning: The DirtySDK UserApi was not created successfully.  UserProfile requests will fail.");
        }

        resetUserProfileCache();
#endif
    }

    UserManager::~UserManager()
    {
#ifdef BLAZE_USER_PROFILE_SUPPORT
        if (mUserApiRef != nullptr)
        {
            mBlazeHub->removeIdler(this);
            UserApiDestroy(mUserApiRef);
        }

        while (!mUserApiCallbackDataSet.empty())
        {
            UserApiCallbackData* tmp = *mUserApiCallbackDataSet.begin();
            mBlazeHub->getScheduler()->removeByAssociatedObject(tmp);
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, tmp);
        }
#endif

        for (uint32_t userIndex = 0; userIndex < mBlazeHub->getNumUsers(); userIndex++)
        {
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager(userIndex)->getUserSessionsComponent();
            userSessions->clearUserSessionExtendedDataUpdateHandler();
            userSessions->clearUserAddedHandler();
            userSessions->clearUserUpdatedHandler();
            userSessions->clearUserRemovedHandler();
            userSessions->clearUserAuthenticatedHandler();
            userSessions->clearUserUnauthenticatedHandler();
            userSessions->clearNotifyQosSettingsUpdatedHandler();
        }

        mBlazeHub->removeUserStateFrameworkEventHandler(this);
        // UM_TODO: If the pool could delete ALL the allocated blocks automatically
        // then we can avoid this process, but for now its best to be safe, because
        // our overflow pool does not keep track of objects in overflow memory.
        while(!mCachedList.empty())
        {
            User* user = static_cast<User*>(&mCachedList.front());
            mCachedList.pop_front();
            mUserPool.free(user);
        }
        
        // NOTE: With the owned list its even trickier because depending on
        // the order of destruction of APIs and on whether or not we receive
        // all our logout notifications we may still have some owned user
        // objects with non-zero reference counts... The only correct way for 
        // those APIs to handle things is NEVER to dereference any User objects 
        // in their respective destructors!
        while(!mOwnedList.empty())
        {
            User* user = static_cast<User*>(&mOwnedList.front());
            BLAZE_SDK_DEBUGF(
                "[UserManager] Warning! User(%" PRId64 ", %s) reference count is: %" PRIu32 " at shutdown\n",
                user->getId(), user->getName(), user->getRefCount()
            );
            mOwnedList.pop_front();
            mUserPool.free(user);
        }
        
        // delete all LocalUser objects
        for(LocalUserVector::iterator i = mLocalUserVector.begin(), e = mLocalUserVector.end(); i != e; ++i)
        {
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK, *i);
        }

        IdByPlatformByNameByNamespaceMap::const_iterator nsi = mCachedByNameByNamespaceMap.begin();
        for ( ; nsi != mCachedByNameByNamespaceMap.end(); ++nsi)
        {
            for (IdByPlatformByNameMap::const_iterator ni = nsi->second->begin(); ni != nsi->second->end(); ++ni)
                BLAZE_DELETE(MEM_GROUP_FRAMEWORK, ni->second);
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK, nsi->second);
        }

        IdByAccountIdByPlatformMap::const_iterator pai = mCachedByAccountIdMap.begin();
        for (; pai != mCachedByAccountIdMap.end(); ++pai)
        {
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK, pai->second);
        }

        IdByOriginPersonaIdByPlatformMap::const_iterator poi = mCachedByOriginPersonaIdMap.begin();
        for (; poi != mCachedByOriginPersonaIdMap.end(); ++poi)
        {
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK, poi->second);
        }
    }

    void UserManager::idle(const uint32_t currentTime, const uint32_t elapsedTime)
    {
#ifdef BLAZE_USER_PROFILE_SUPPORT
        if (mUserApiRef != nullptr)
        {
            BLAZE_SDK_SCOPE_TIMER("UserManager_UserApiUpdate");

            // If any responses from first-party are available, this will trigger the _userApiCallback()
            UserApiUpdate(mUserApiRef);
        }
#endif

        // Check if any of the local users have signed out of first-party,
        // logging them out if they have
        int32_t onlineStatusMask = NetConnStatus('mask', 0, nullptr, 0);
        for (LocalUserVector::size_type i = 0; i < mLocalUserVector.size(); ++i)
        {
            LocalUser* localUser = getLocalUser(static_cast<uint32_t>(i));
            LoginManager::LoginManager* loginManager = mBlazeHub->getLoginManager(static_cast<uint32_t>(i));
            if (localUser != nullptr && loginManager != nullptr)
            {
                uint32_t dirtySockUserIndex = loginManager->getDirtySockUserIndex();
                if (!(onlineStatusMask & (1 << dirtySockUserIndex)))
                {
                    BLAZE_SDK_SCOPE_TIMER("UserManager_logout");

                    loginManager->logout();
                }
            }
        }
    }

    void UserManager::onLocalUserAuthenticated(uint32_t userIndex)
    {
        if (getPrimaryLocalUser() == nullptr)
        {
            setPrimaryLocalUser(userIndex);
        }

        if (userIndex == mPrimaryLocalUserIndex)
        {
            mPrimaryLocalUserDispatcher.dispatch<uint32_t>("onPrimaryLocalUserAuthenticated",
                &PrimaryLocalUserListener::onPrimaryLocalUserAuthenticated,
                mPrimaryLocalUserIndex);
        }

        // Now that a connection is known, check if the user cache should be increased
        int32_t maxUsersFromConfig = 0;
        if (mBlazeHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_USERMANAGER_CACHED_USERS, &maxUsersFromConfig))
        {
            if (maxUsersFromConfig > (int32_t)mMaxCachedUserCount)
            {
                mMaxCachedUserCount = maxUsersFromConfig;
                reserveUserPool();
            }
        }

        TimeValue cachedUserRefreshInterval;
        if (mBlazeHub->getConnectionManager()->getServerConfigTimeValue(BLAZESDK_CONFIG_KEY_CACHED_USER_REFRESH_INTERVAL, cachedUserRefreshInterval))
        {
            setCachedUserRefreshInterval((uint32_t)cachedUserRefreshInterval.getMillis());
        }

        // Once any user has logged in, we must not allow QoS to be re-initialized, as the qos api may compete with games for port access
        mBlazeHub->getConnectionManager()->disableQosReinitialization();

        // Dispatch to any listeners of ours.
        mStateDispatcher.dispatch("onLocalUserAuthenticated", &UserManagerStateListener::onLocalUserAuthenticated, userIndex);
    }

    void UserManager::onLocalUserForcedDeAuthenticated(uint32_t userIndex, UserSessionForcedLogoutType forcedLogoutType)
    {
        mStateDispatcher.dispatch("onLocalUserForcedDeAuthenticated", &UserManagerStateListener::onLocalUserForcedDeAuthenticated, userIndex, forcedLogoutType);
    }

    void UserManager::onLocalUserDeAuthenticated(uint32_t userIndex)
    {
        // we just lost authentication; remove the local player
        LocalUser *localUser = mLocalUserVector[userIndex];
        if (localUser != nullptr)
        {
            if (getPrimaryLocalUser() != nullptr)
            {
                // Primary local user has logged out.  Set a new one.
                if (localUser->getId() == getPrimaryLocalUser()->getId())
                {
                    if (!chooseNewPrimaryLocalUser(userIndex))
                    {
                        mPrimaryLocalUserDispatcher.dispatch<uint32_t>("onPrimaryLocalUserDeAuthenticated",
                            &PrimaryLocalUserListener::onPrimaryLocalUserDeAuthenticated,
                            mPrimaryLocalUserIndex);

                        setPrimaryLocalUser(0);
                    }
                }
            }

            // Release all resources for the localUser
            releaseUser(localUser->getUser());
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK, localUser);
            mLocalUserVector[userIndex] = nullptr;

            // NOTE: To save bandwidth the server will not send onUserUnsubscribed() notifications
            // to the LocalUser being logged out. This means that all the onUserAdded() notifications
            // that have not been matched by corresponding onUserUnsubscribed() notifications will continue
            // to hold references to corresponding User objects. To prevent these dangling references we
            // perform the special iteration below that will first make sure that the LocalUser is indeed
            // subscribed to the User object, and then release the reference on the User appropriately.
            // ALSO NOTE: We do not need to iterate the mCachedList because User objects that result from
            // onUserAdded()/onUserUnsubscribed() will ALWAYS exist in the mOwnedList!
            UserList::iterator it = mOwnedList.begin();
            while(it != mOwnedList.end())
            {
                User* user = static_cast<User*>(&(*it));
                ++it;
                if(user->isSubscribedByIndex(userIndex))
                {
                    user->clearSubscriberIndex(userIndex);
                    // release reference to User that were subscribed to by this LocalUser
                    releaseUser(user);
                }
            }

            mLocalUserGroup.removeDeAuthenticatedLocalUsers();
            // If the last local user has logged out, re-enable QoS reinitialization to allow QoS
            // latencies to be refreshed if necessary
            if (mLocalUserGroup.isEmpty())
                mBlazeHub->getConnectionManager()->enableQosReinitialization();
        }

        // Finally, dispatch to any listeners.
        mStateDispatcher.dispatch("onLocalUserDeAuthenticated", &UserManagerStateListener::onLocalUserDeAuthenticated, userIndex);
    }

    User *UserManager::acquireUser(const Blaze::UserIdentification &data, User::UserType newUserType)
    {
        return acquireUser(
            data.getBlazeId(), 
            data.getPlatformInfo(),
            data.getIsPrimaryPersona(),
            data.getPersonaNamespace(),
            data.getName(), 
            data.getAccountLocale(),
            data.getAccountCountry(),
            nullptr,
            newUserType
        );
    }
    
    User *UserManager::acquireUser(const Blaze::UserData *data, User::UserType newUserType)
    {
        // UM_TODO :: In the future we will add the UserExtendedData + UserStatusFlags + Timestamp parameters to
        // acquireUser() method and remove the refreshTransientData() and setTimestamp() methods on the User object.
        // Currently updating the transient data after the initial acquireUser() call is the easiest way to ensure
        // that cached data gets updated correctly after the Blaze::UserData object has been fetched from the server
        // via lookupUser/lookupUsers RPC call.
        
        // NOTE: acquireUser() always returns a valid User*
        // it will create one if it does not exist!
        User* user = acquireUser(
            data->getUserInfo().getBlazeId(), 
            data->getUserInfo().getPlatformInfo(),
            data->getUserInfo().getIsPrimaryPersona(),
            data->getUserInfo().getPersonaNamespace(),
            data->getUserInfo().getName(),
            data->getUserInfo().getAccountLocale(),
            data->getUserInfo().getAccountCountry(),
            &data->getStatusFlags(),
            newUserType
        );

        bool subscribed = user->getStatusFlags().getSubscribed();

        // update the transient data inside the cached User object
        user->refreshTransientData(*data);

        // retain the subscribed status for OWNED users -- because 'subscribed' isn't filled in for lookup responses
        if (user->getRefCount() > 0)
        {
            user->setSubscribed(subscribed);
        }

        // typically, only cached users need their timestamp updated because they are not 
        // registered for notifications and thus can get out of date in a hurry...
        user->setTimestamp(NetTick());
        return user;
    }

    /*! ************************************************************************************************/
    /*! \brief Create (or update) and fetch the User with the supplied blazeId.
        - Takes ownership by incrementing the reference count if newUserType == User::OWNED
        - Does not take ownership or increment the reference count if newUserType == User::CACHED
        \note
        - User objects returned by acquireUser() are only guranteed valid while the UserManager API exists; 
        therefore, it is @em wrong to dereference any cached User* pointers inside API destructors or anywhere
        the UserManager can no longer be assumed to be valid.
        \param[in] blazeId the blazeId of the user to add/update
        \param[in] personaNamespace the namespace the persona name is in
        \param[in] userName the name of the user (persona name)
        \param[in] extId the External ID (if any) for the user.
        \param[in] accountLocale The locale for the user.
        \param[in] accountCountry The country for the user.
        \param[in] statusFlags User status flag (currently only stores online status)
        \return The updated user
    ***************************************************************************************************/
    User* UserManager::acquireUser(
        BlazeId blazeId,
        const PlatformInfo& platformInfo,
        bool isPrimaryPersona,
        const char8_t *personaNamespace,
        const char8_t *userName,
        Locale accountLocale,
        uint32_t accountCountry,
        const UserDataFlags *statusFlags,
        User::UserType newUserType)
    {
        // Guest users share the same name and external ids as their parent user.
        // They must only be looked up via BlazeId, and they must not be added to external id/name caches.
        bool isGuestUser = UserManager::isGuestUser(blazeId);

        // try finding an existing user for this blazeId
        User *user = getUser(blazeId);

        // Non-guest user additional lookups:
        if (!isGuestUser)
        {
            //  Since user objects may not contain all information identifying a user, check to find
            //  whether users exist with the other keys
            if (user == nullptr)
            {
                user = const_cast<User*>(getUserByPlatformInfo(platformInfo));
            }
            if (user == nullptr && userName != nullptr && userName[0] != '\0' && personaNamespace != nullptr && personaNamespace[0] != '\0')
            {
                user = const_cast<User*>(getUserByName(personaNamespace, userName, platformInfo.getClientPlatform()));
            }
        }
        
        bool userReadded = false;
        
        if (user == nullptr)
        {
            if(mUserCount == mMaxCachedUserCount)
            {
                if(!mCachedList.empty())
                {
                    // no more room in the pool, but we have some cached Users, so free the last one
                    releaseUser(static_cast<User*>(&mCachedList.back()));
                }
                else
                {
                    BLAZE_SDK_DEBUGF("[UserManager] User object pool exhausted, using overflow storage\n");
                }
            }
            // alloc & add user to map
            user = new (mUserPool.alloc()) User(blazeId, platformInfo, isPrimaryPersona, personaNamespace, userName, accountLocale, accountCountry, statusFlags, *mBlazeHub);
            if (blazeId != 0)
            {
                mUserMap[blazeId] = user;
            }
            // new user was created, increment count
            ++mUserCount;
        }
        else
        {
            // Check if user's blaze id is updated. This happens if account is deleted and then
            // recreated and associated with the same external reference id.
            // Fix for GOS-7994
            if (blazeId != INVALID_BLAZE_ID 
                && user->getId() != INVALID_BLAZE_ID 
                && blazeId != user->getId())
            {
                // remove the user and invalidate blaze id 
                // user is re-added later with updated info (code below)
                mUserMap.erase(user->mId);
                user->mId = INVALID_BLAZE_ID;
            }

            //  update user information if not currently set
            if (user->getId() == INVALID_BLAZE_ID && blazeId != INVALID_BLAZE_ID)
            {
                user->mId = blazeId;
                // In Blaze 3 BlazeId == PersonaId, but we still
                // set the string version for compatibility
                user->setPersonaId(blazeId);
                mUserMap[blazeId] = user;
            }
            
            // Only update lookup caches and other information for non-Guest users:
            if (!isGuestUser)
            {
                if ((userName != nullptr) && (*userName != '\0') &&
                    (personaNamespace != nullptr) && (*personaNamespace != '\0'))
                {
                    // remove from lookup map; it will be re-added below
                    replaceOrDeleteUserId(personaNamespace, userName, user->getClientPlatform(), INVALID_BLAZE_ID);

                    user->setPersonaNamespace(personaNamespace);
                    user->setName(userName);
                }

                // Only primary persona users get the Nucleus Account info tracked.  Secondary accounts don't.
                if (user->getIsPrimaryPersona())
                {
                    if (platformInfo.getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
                    {
                        // remove from lookup map; it will be re-added below
                        IdByAccountIdByPlatformMap::iterator pi = mCachedByAccountIdMap.find(user->mPlatformInfo.getClientPlatform());
                        if (pi != mCachedByAccountIdMap.end())
                        {
                            IdByAccountIdMap::iterator ei = pi->second->find(user->mPlatformInfo.getEaIds().getNucleusAccountId());
                            if (ei != pi->second->end())
                            {
                                pi->second->erase(ei);
                            }
                        }

                        BLAZE_SDK_DEBUGF("[UserManager] For platform '%s': update accountId cache('%" PRIu64 "'->'%" PRIu64 "')\n", ClientPlatformTypeToString(user->mPlatformInfo.getClientPlatform()), user->getPlatformInfo().getEaIds().getNucleusAccountId(), platformInfo.getEaIds().getNucleusAccountId());
                    }

                    if (platformInfo.getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
                    {
                        // remove from lookup map; it will be re-added below
                        IdByOriginPersonaIdByPlatformMap::iterator pi = mCachedByOriginPersonaIdMap.find(user->mPlatformInfo.getClientPlatform());
                        if (pi != mCachedByOriginPersonaIdMap.end())
                        {
                            IdByOriginPersonaIdMap::iterator ei = pi->second->find(user->mPlatformInfo.getEaIds().getOriginPersonaId());
                            if (ei != pi->second->end())
                            {
                                pi->second->erase(ei);
                            }
                        }

                        BLAZE_SDK_DEBUGF("[UserManager] For platform '%s': update originPersonaId cache('%" PRIu64 "'->'%" PRIu64 "')\n", ClientPlatformTypeToString(user->mPlatformInfo.getClientPlatform()), user->getPlatformInfo().getEaIds().getOriginPersonaId(), platformInfo.getEaIds().getOriginPersonaId());
                    }
                }

                if (((platformInfo.getClientPlatform() == ps4) || (platformInfo.getClientPlatform() == ps5)) && platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
                {
                    // remove from lookup map; it will be re-added below
                    IdByExtXblIdMap::iterator ei = mCachedByExtPsnIdMap.find(user->mPlatformInfo.getExternalIds().getPsnAccountId());
                    if (ei != mCachedByExtPsnIdMap.end())
                    {
                        mCachedByExtPsnIdMap.erase(ei);
                    }

                    BLAZE_SDK_DEBUGF("[UserManager] Update psnId cache('%" PRIu64 "'->'%" PRIu64 "')\n", user->mPlatformInfo.getExternalIds().getPsnAccountId(), platformInfo.getExternalIds().getPsnAccountId());
                }

                if (((platformInfo.getClientPlatform() == xone) || (platformInfo.getClientPlatform() == xbsx)) && platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
                {
                    // remove from lookup map; it will be re-added below
                    IdByExtXblIdMap::iterator ei = mCachedByExtXblIdMap.find(user->mPlatformInfo.getExternalIds().getXblAccountId());
                    if (ei != mCachedByExtXblIdMap.end())
                    {
                        mCachedByExtXblIdMap.erase(ei);
                    }

                    BLAZE_SDK_DEBUGF("[UserManager] Update xblId cache('%" PRIu64 "'->'%" PRIu64 "')\n", user->mPlatformInfo.getExternalIds().getXblAccountId(), platformInfo.getExternalIds().getXblAccountId());
                }

                if ((platformInfo.getClientPlatform() == steam) && platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
                {
                    // remove from lookup map; it will be re-added below
                    IdByExtSteamIdMap::iterator ei = mCachedByExtSteamIdMap.find(user->mPlatformInfo.getExternalIds().getSteamAccountId());
                    if (ei != mCachedByExtSteamIdMap.end())
                    {
                        mCachedByExtSteamIdMap.erase(ei);
                    }

                    BLAZE_SDK_DEBUGF("[UserManager] Update steamId cache('%" PRIu64 "'->'%" PRIu64 "')\n", user->mPlatformInfo.getExternalIds().getSteamAccountId(), platformInfo.getExternalIds().getSteamAccountId());
                }

                if ((platformInfo.getClientPlatform() == stadia) && platformInfo.getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
                {
                    // remove from lookup map; it will be re-added below
                    IdByExtStadiaIdMap::iterator ei = mCachedByExtStadiaIdMap.find(user->mPlatformInfo.getExternalIds().getStadiaAccountId());
                    if (ei != mCachedByExtStadiaIdMap.end())
                    {
                        mCachedByExtStadiaIdMap.erase(ei);
                    }

                    BLAZE_SDK_DEBUGF("[UserManager] Update stadiaId cache('%" PRIu64 "'->'%" PRIu64 "')\n", user->mPlatformInfo.getExternalIds().getStadiaAccountId(), platformInfo.getExternalIds().getStadiaAccountId());
                }

                if (platformInfo.getClientPlatform() == nx && !platformInfo.getExternalIds().getSwitchIdAsTdfString().empty())
                {
                    // remove from lookup map; it will be re-added below
                    IdByExtSwitchIdMap::iterator ei = mCachedByExtSwitchIdMap.find(user->mPlatformInfo.getExternalIds().getSwitchId());
                    if (ei != mCachedByExtSwitchIdMap.end())
                    {
                        mCachedByExtSwitchIdMap.erase(ei);
                    }

                    BLAZE_SDK_DEBUGF("[UserManager] Update switchId cache('%s'->'%s')\n", user->mPlatformInfo.getExternalIds().getSwitchId(), platformInfo.getExternalIds().getSwitchId());
                }

                eastl::string oldPlatformInfoStr;
                eastl::string newPlatformInfoStr;
                BlazeHub::platformInfoToString(user->mPlatformInfo, oldPlatformInfoStr);
                BLAZE_SDK_DEBUGF("[UserManager] Update platformInfo: (%s) -> (%s)\n", oldPlatformInfoStr.c_str(), BlazeHub::platformInfoToString(platformInfo, newPlatformInfoStr));
                platformInfo.copyInto(user->mPlatformInfo);

                if (user->getAccountLocale() == 0)
                {
                    user->setAccountLocale(accountLocale);
                }

                if (user->getAccountCountry() == 0)
                {
                    user->setAccountCountry(accountCountry);
                }
            }

            // remove user from whichever list it's in because we always re-add it below
            UserList::remove(*user);
            if (user->getRefCount() > 0)
            {
                userReadded = true;
            }
        }
        
        // if refcount > 0 means that the user is is already owned
        if(newUserType == User::OWNED || user->getRefCount() > 0)
        {
            user->AddRef(); // owned users are only deleted when their refcount ticks down to 0
            mOwnedList.push_front(*user); // add it to the owned list
            if (!userReadded)
                mUserEventDispatcher.dispatch<const User&>("onUserAdded",
                    &UserEventListener::onUserAdded,
                    *user);
        }
        else
        {
            // NOTE: Cached users do not need references because they
            // have no concept of ownership, and will be purged when our pool fills
            mCachedList.push_front(*user); // add it to the cached list
        }

        // Update the lookup caches for non-Guest users: 
        if (!isGuestUser)
        {
            replaceOrDeleteUserId(user->getPersonaNamespace(), user->getName(), user->getClientPlatform(), user->getId());

            if (user->getIsPrimaryPersona())
            {
                if (user->getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
                {
                    IdByAccountIdMap* idByAccountIdMap;
                    IdByAccountIdByPlatformMap::const_iterator pi = mCachedByAccountIdMap.find(user->getPlatformInfo().getClientPlatform());
                    if (pi == mCachedByAccountIdMap.end())
                    {
                        idByAccountIdMap = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "UserManager::idByAccountIdMap") IdByAccountIdMap(MEM_GROUP_FRAMEWORK, "UserManager::idByAccountIdMap");
                        mCachedByAccountIdMap[user->getPlatformInfo().getClientPlatform()] = idByAccountIdMap;
                    }
                    else
                    {
                        idByAccountIdMap = pi->second;
                    }
                    (*idByAccountIdMap)[user->getPlatformInfo().getEaIds().getNucleusAccountId()] = user->getId();
                }
                if (user->getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
                {
                    IdByOriginPersonaIdMap* idByOriginPersonaIdMap;
                    IdByOriginPersonaIdByPlatformMap::const_iterator pi = mCachedByOriginPersonaIdMap.find(user->getPlatformInfo().getClientPlatform());
                    if (pi == mCachedByOriginPersonaIdMap.end())
                    {
                        idByOriginPersonaIdMap = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "UserManager::idByOriginPersonaIdMap") IdByOriginPersonaIdMap(MEM_GROUP_FRAMEWORK, "UserManager::idByOriginPersonaIdMap");
                        mCachedByOriginPersonaIdMap[user->getPlatformInfo().getClientPlatform()] = idByOriginPersonaIdMap;
                    }
                    else
                    {
                        idByOriginPersonaIdMap = pi->second;
                    }
                    (*idByOriginPersonaIdMap)[user->getPlatformInfo().getEaIds().getOriginPersonaId()] = user->getId();
                }
            }
            if (((user->getPlatformInfo().getClientPlatform() == ps4) || (user->getPlatformInfo().getClientPlatform() == ps5)) && user->getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
            {
                mCachedByExtPsnIdMap[user->getPlatformInfo().getExternalIds().getPsnAccountId()] = user->getId();
            }
            if (((user->getPlatformInfo().getClientPlatform() == xone) || (user->getPlatformInfo().getClientPlatform() == xbsx)) && user->getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
            {
                mCachedByExtXblIdMap[user->getPlatformInfo().getExternalIds().getXblAccountId()] = user->getId();
            }
            if ((user->getPlatformInfo().getClientPlatform() == steam)  && user->getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
            {
                mCachedByExtSteamIdMap[user->getPlatformInfo().getExternalIds().getSteamAccountId()] = user->getId();
            }
            if ((user->getPlatformInfo().getClientPlatform() == stadia) && user->getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
            {
                mCachedByExtStadiaIdMap[user->getPlatformInfo().getExternalIds().getStadiaAccountId()] = user->getId();
            }
            if (user->getPlatformInfo().getClientPlatform() == nx && !user->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
            {
                mCachedByExtSwitchIdMap[user->getPlatformInfo().getExternalIds().getSwitchId()] = user->getId();
            }
        }

        return user;
    }

    /*! ************************************************************************************************/
    /*! \brief Destroy (or update the refCount of) the User with the supplied blazeId.
        \param[in] user the user to remove
    ***************************************************************************************************/
    void UserManager::releaseUser(const User *user)
    {
        User* userToRelease = const_cast<User*>(user);
        bool owned = false;
        if (userToRelease->getRefCount() != 0)
        {
            owned = true;
            userToRelease->RemoveRef();
        }
        
        // NOTE: Cached (not-owned) users always have refcount of 0; therefore, we never
        // decrement their reference count before deleting them

        if (userToRelease->getRefCount() == 0)
        {
            UserList::remove(*userToRelease);

            // this user to release should be in the lookup maps

            if (owned && !mUserPool.isOverflowed())
            {
                // put at back of cache list
                mCachedList.push_back(*userToRelease);

                mUserEventDispatcher.dispatch<const User&>("onUserRemoved",
                    &UserEventListener::onUserRemoved,
                    *userToRelease);

                // keep this user in the lookup maps
            }
            else
            {
                if (userToRelease->getId() != 0)
                {
                    // if the id is valid, then the User is definitely in the map
                    mUserMap.erase(userToRelease->getId());
                }

                if (owned)
                {
                    // was owned
                    mUserEventDispatcher.dispatch<const User&>("onUserRemoved",
                        &UserEventListener::onUserRemoved,
                        *userToRelease);
                }

                if (!userToRelease->isGuestUser())
                {
                    // remove this user from the lookup maps
                    replaceOrDeleteUserId(userToRelease->getPersonaNamespace(), userToRelease->getName(), userToRelease->getClientPlatform(), INVALID_BLAZE_ID);

                    if (user->getIsPrimaryPersona())
                    {
                        if (userToRelease->getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
                        {
                            IdByAccountIdByPlatformMap::iterator pi = mCachedByAccountIdMap.find(userToRelease->getPlatformInfo().getClientPlatform());
                            if (pi != mCachedByAccountIdMap.end())
                            {
                                IdByAccountIdMap::iterator ei = pi->second->find(userToRelease->getPlatformInfo().getEaIds().getNucleusAccountId());
                                if (ei != pi->second->end())
                                {
                                    pi->second->erase(ei);
                                }
                            }
                        }
                        if (userToRelease->getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
                        {
                            IdByOriginPersonaIdByPlatformMap::iterator pi = mCachedByOriginPersonaIdMap.find(userToRelease->getPlatformInfo().getClientPlatform());
                            if (pi != mCachedByOriginPersonaIdMap.end())
                            {
                                IdByOriginPersonaIdMap::iterator ei = pi->second->find(userToRelease->getPlatformInfo().getEaIds().getOriginPersonaId());
                                if (ei != pi->second->end())
                                {
                                    pi->second->erase(ei);
                                }
                            }
                        }
                    }
                    if (((userToRelease->getPlatformInfo().getClientPlatform() == ps4) || (userToRelease->getPlatformInfo().getClientPlatform() == ps5)) && userToRelease->getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
                    {
                        IdByExtPsnIdMap::iterator ei = mCachedByExtPsnIdMap.find(userToRelease->getPlatformInfo().getExternalIds().getPsnAccountId());
                        if (ei != mCachedByExtPsnIdMap.end())
                        {
                            mCachedByExtPsnIdMap.erase(ei);
                        }
                    }
                    if (((userToRelease->getPlatformInfo().getClientPlatform() == xone) || (userToRelease->getPlatformInfo().getClientPlatform() == xbsx)) && userToRelease->getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
                    {
                        IdByExtXblIdMap::iterator ei = mCachedByExtXblIdMap.find(userToRelease->getPlatformInfo().getExternalIds().getXblAccountId());
                        if (ei != mCachedByExtXblIdMap.end())
                        {
                            mCachedByExtXblIdMap.erase(ei);
                        }
                    }
                    if ((userToRelease->getPlatformInfo().getClientPlatform() == steam) && userToRelease->getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
                    {
                        IdByExtSteamIdMap::iterator ei = mCachedByExtSteamIdMap.find(userToRelease->getPlatformInfo().getExternalIds().getSteamAccountId());
                        if (ei != mCachedByExtSteamIdMap.end())
                        {
                            mCachedByExtSteamIdMap.erase(ei);
                        }
                    }
                    if ((userToRelease->getPlatformInfo().getClientPlatform() == stadia) && userToRelease->getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
                    {
                        IdByExtStadiaIdMap::iterator ei = mCachedByExtStadiaIdMap.find(userToRelease->getPlatformInfo().getExternalIds().getStadiaAccountId());
                        if (ei != mCachedByExtStadiaIdMap.end())
                        {
                            mCachedByExtStadiaIdMap.erase(ei);
                        }
                    }
                    if (userToRelease->getPlatformInfo().getClientPlatform() == nx && !userToRelease->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
                    {
                        IdByExtSwitchIdMap::iterator ei = mCachedByExtSwitchIdMap.find(userToRelease->getPlatformInfo().getExternalIds().getSwitchId());
                        if (ei != mCachedByExtSwitchIdMap.end())
                        {
                            mCachedByExtSwitchIdMap.erase(ei);
                        }
                    }
                }

                mUserPool.free(userToRelease);
                // user deleted decrement count
                --mUserCount;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Check if the User object meets the validity criteria:
        - Is not nullptr
        - Is complete
        - Is up to date (if it is CACHED)
        \note The assumption is that if a user is OWNED it will be kept up to date via regular updates;
        meanwhile, CACHED users may become stale because the Blaze server does not keep track of such 
        looked-up user objects. This means that we must periodically refresh the data inside our CACHED
        users when it becomes stale. The 'staleness' period is governed by a configurable UserManager
        parameter mStaleUserThreshold. The minimum value of this parameter is 1/4 sec.
        \param[in] user - the user to validate
    ***************************************************************************************************/
    bool UserManager::isValidUser(const User *user) const
    {
        return (user != nullptr && user->isComplete()) &&
               (user->getRefCount() > 0 /*OWNED*/ || ((user->getTimestamp() > 0) && NetTickDiff(NetTick(), user->getTimestamp()) <= static_cast<int32_t>(mCachedUserRefreshIntervalMs)));
    }

    /*! ************************************************************************************************/
    /*!
        \brief Internal version of getUser; returns non-const user.
    
        \param[in] blazeId the id of the user to lookup
        \return a User pointer, or nullptr if the user wasn't found.
    ***************************************************************************************************/
    User* UserManager::getUser( BlazeId blazeId ) const
    {
        UserMap::const_iterator userMapIter = mUserMap.find(blazeId);
        if (userMapIter != mUserMap.end())
        {
            return userMapIter->second;
        }

        return nullptr;
    }

    User* UserManager::getUser( const char8_t* name ) const
    {
        return getUser(nullptr, name, INVALID);
    }

    /*! **********************************************************************************************************/
    /*! \brief Internal version of getUserByName; returns non-const user. scan the user map and return the user with 
    the supplied name (if one exists).Note: inefficient, prefer getUserById.  names are not case sensitive.

    \param[in] name the name of the user to get.  Case insensitive.
    \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
    **************************************************************************************************************/
    User* UserManager::getUser( const char8_t* personaNamespace, const char8_t* name ) const
    {
        return getUser(personaNamespace, name, INVALID);
    }

    /*! **********************************************************************************************************/
    /*! \brief Internal version of getUserByName; returns non-const user. scan the user map and return the user with the
        supplied name and platform (if one exists).    Note: inefficient, prefer getUserById.  names are not case sensitive.
        \param[in] personaNamespace             The namespace of the user to lookup, nullptr implies platform default namespace
        \param[in] name the name of the user to get.  Case insensitive.
        \param[in] platform the platform of the user to get. INVALID implies caller's platform.
        \return the User obj with the associated name, or nullptr if the user doesn't exist in the user map.
    **************************************************************************************************************/
    User* UserManager::getUser(const char8_t* personaNamespace, const char8_t* name, ClientPlatformType platform) const
    {
        if (personaNamespace == nullptr)
            personaNamespace = mBlazeHub->getConnectionManager()->getPersonaNamespace();

        if (platform == INVALID)
            platform = mBlazeHub->getClientPlatformType();

        IdByPlatformByNameByNamespaceMap::const_iterator nsi = mCachedByNameByNamespaceMap.find(personaNamespace);
        if (nsi != mCachedByNameByNamespaceMap.end())
        {
            IdByPlatformByNameMap::const_iterator ni = nsi->second->find(name);
            if (ni != nsi->second->end())
            {
                IdByPlatformMap::const_iterator pi = ni->second->find(platform);
                if (pi != ni->second->end())
                {
                    UserMap::const_iterator userMapIter = mUserMap.find(pi->second);
                    if (userMapIter != mUserMap.end())
                        return userMapIter->second;
                }
            }
        }
        return nullptr;
    }

    /*! **********************************************************************************************************/
    /*! \brief Helper method for lookupUsersByName; adds all cached users for the given persona name and namespace to userVector.
        \param[in] personaNamespace             The namespace of the users to lookup, nullptr implies platform default namespace
        \param[in] name the name of the users to get.  Case insensitive.
        \return true if at least one user was found
    **************************************************************************************************************/
    bool UserManager::getUsers(const char8_t* personaNamespace, const char8_t* name, UserVectorPtr& userVector) const
    {
        if (personaNamespace == nullptr)
            personaNamespace = mBlazeHub->getConnectionManager()->getPersonaNamespace();

        bool foundUser = false;
        IdByPlatformByNameByNamespaceMap::const_iterator nsi = mCachedByNameByNamespaceMap.find(personaNamespace);
        if (nsi != mCachedByNameByNamespaceMap.end())
        {
            IdByPlatformByNameMap::const_iterator ni = nsi->second->find(name);
            if (ni != nsi->second->end())
            {
                for (IdByPlatformMap::const_iterator pi = ni->second->begin(); pi != ni->second->end(); ++pi)
                {
                    UserMap::const_iterator userMapIter = mUserMap.find(pi->second);
                    if (userMapIter != mUserMap.end() && isValidUser(userMapIter->second))
                    {
                        userVector->push_back(userMapIter->second);
                        foundUser = true;
                    }
                }
            }
        }
        return foundUser;
    }

    /*! **********************************************************************************************************/
    /*! \brief Helper method. Replaces or deletes the cached BlazeId of the user with the
        supplied name and platform.
        \param[in] personaNamespace the namespace of the local cache entry.
        \param[in] name the name of the local cache entry.  Case insensitive.
        \param[in] platform the platform of the local cache entry.
        \param[in] newBlazeId the BlazeId to set in the local cache. If newBlazeId is INVALID_BLAZE_ID,
                                then the cache entry is deleted instead of replaced.
    **************************************************************************************************************/
    void UserManager::replaceOrDeleteUserId(const char8_t* personaNamespace, const char8_t* name, ClientPlatformType platform, BlazeId newBlazeId)
    {
        if (personaNamespace == nullptr || personaNamespace[0] == '\0' || name == nullptr || name[0] == '\0')
            return;

        IdByPlatformByNameMap* idByNameMap;
        IdByPlatformByNameByNamespaceMap::iterator nsi = mCachedByNameByNamespaceMap.find(personaNamespace);
        if (nsi == mCachedByNameByNamespaceMap.end())
        {
            if (newBlazeId == INVALID_BLAZE_ID)
                return;

            idByNameMap = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "UserManager::idByNameMap") IdByPlatformByNameMap(MEM_GROUP_FRAMEWORK, "UserManager::idByNameMap");
            mCachedByNameByNamespaceMap[personaNamespace] = idByNameMap;
        }
        else
            idByNameMap = nsi->second;

        IdByPlatformByNameMap::iterator ni = idByNameMap->find(name);
        if (ni == idByNameMap->end())
        {
            if (newBlazeId == INVALID_BLAZE_ID)
                return;

            IdByPlatformMap* idByPlatformMap = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "UserManager::idByPlatformMap") IdByPlatformMap(MEM_GROUP_FRAMEWORK, "UserManager::idByPlatformMap");
            (*idByNameMap)[name] = idByPlatformMap;
            (*idByPlatformMap)[platform] = newBlazeId;
        }
        else
        {
            IdByPlatformMap* idByPlatformMap = ni->second;
            if (newBlazeId == INVALID_BLAZE_ID)
            {
                idByPlatformMap->erase(platform);
                if (idByPlatformMap->empty())
                {
                    idByNameMap->erase(ni);
                    BLAZE_DELETE(MEM_GROUP_FRAMEWORK, idByPlatformMap);
                }
            }
            else
                (*idByPlatformMap)[platform] = newBlazeId;
        }
    }

    User *UserManager::getUserByXblAccountIdInternal(ExternalXblAccountId xblId) const
    {
        if (xblId == INVALID_XBL_ACCOUNT_ID)
            return nullptr;

        IdByExtXblIdMap::const_iterator ei = mCachedByExtXblIdMap.find(xblId);
        if (ei != mCachedByExtXblIdMap.end())
        {
            UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
            if (userMapIter != mUserMap.end())
            {
                return userMapIter->second;
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if (((user->getPlatformInfo().getClientPlatform() == xone) || (user->getPlatformInfo().getClientPlatform() == xbsx)) && user->getPlatformInfo().getExternalIds().getXblAccountId() == xblId)
            {
                return user;
            }
        }
        return nullptr;
    }
    User *UserManager::getUserByPsnAccountIdInternal(ExternalPsnAccountId psnId) const
    {
        if (psnId == INVALID_PSN_ACCOUNT_ID)
            return nullptr;

        IdByExtPsnIdMap::const_iterator ei = mCachedByExtPsnIdMap.find(psnId);
        if (ei != mCachedByExtPsnIdMap.end())
        {
            UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
            if (userMapIter != mUserMap.end())
            {
                return userMapIter->second;
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if (((user->getPlatformInfo().getClientPlatform() == ps4) || (user->getPlatformInfo().getClientPlatform() == ps5)) && user->getPlatformInfo().getExternalIds().getPsnAccountId() == psnId)
            {
                return user;
            }
        }
        return nullptr;
    }
    User *UserManager::getUserBySteamAccountIdInternal(ExternalSteamAccountId steamId) const
    {
        if (steamId == INVALID_STEAM_ACCOUNT_ID)
            return nullptr;

        IdByExtSteamIdMap::const_iterator ei = mCachedByExtSteamIdMap.find(steamId);
        if (ei != mCachedByExtSteamIdMap.end())
        {
            UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
            if (userMapIter != mUserMap.end())
            {
                return userMapIter->second;
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if ((user->getPlatformInfo().getClientPlatform() == steam) && user->getPlatformInfo().getExternalIds().getSteamAccountId() == steamId)
            {
                return user;
            }
        }
        return nullptr;
    }
    User *UserManager::getUserByStadiaAccountIdInternal(ExternalStadiaAccountId stadiaId) const
    {
        if (stadiaId == INVALID_STADIA_ACCOUNT_ID)
            return nullptr;

        IdByExtStadiaIdMap::const_iterator ei = mCachedByExtStadiaIdMap.find(stadiaId);
        if (ei != mCachedByExtStadiaIdMap.end())
        {
            UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
            if (userMapIter != mUserMap.end())
            {
                return userMapIter->second;
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if ((user->getPlatformInfo().getClientPlatform() == stadia) && user->getPlatformInfo().getExternalIds().getStadiaAccountId() == stadiaId)
            {
                return user;
            }
        }
        return nullptr;
    }
    User *UserManager::getUserBySwitchIdInternal(const char8_t* switchId) const
    {
        if (switchId == nullptr || switchId[0] == '\0')
            return nullptr;

        IdByExtSwitchIdMap::const_iterator ei = mCachedByExtSwitchIdMap.find(switchId);
        if (ei != mCachedByExtSwitchIdMap.end())
        {
            UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
            if (userMapIter != mUserMap.end())
            {
                return userMapIter->second;
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if (user->getPlatformInfo().getClientPlatform() == nx && user->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() == switchId)
            {
                return user;
            }
        }
        return nullptr;
    }
    User *UserManager::getUserByNucleusAccountIdInternal(ClientPlatformType platform, AccountId accountId) const
    {
        if (accountId == INVALID_ACCOUNT_ID)
            return nullptr;

        if (platform == INVALID)
            platform = mBlazeHub->getClientPlatformType();

        IdByAccountIdByPlatformMap::const_iterator pi = mCachedByAccountIdMap.find(platform);
        if (pi != mCachedByAccountIdMap.end())
        {
            IdByAccountIdMap::const_iterator ei = pi->second->find(accountId);
            if (ei != pi->second->end())
            {
                UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
                if (userMapIter != mUserMap.end())
                {
                    return userMapIter->second;
                }
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if (user->getPlatformInfo().getClientPlatform() == platform && user->getPlatformInfo().getEaIds().getNucleusAccountId() == accountId)
            {
                return user;
            }
        }
        return nullptr;
    }
    User *UserManager::getUserByOriginPersonaIdInternal(ClientPlatformType platform, OriginPersonaId originPersonaId) const
    {
        if (originPersonaId == INVALID_ORIGIN_PERSONA_ID)
            return nullptr;

        if (platform == INVALID)
            platform = mBlazeHub->getClientPlatformType();

        IdByOriginPersonaIdByPlatformMap::const_iterator pi = mCachedByOriginPersonaIdMap.find(platform);
        if (pi != mCachedByOriginPersonaIdMap.end())
        {
            IdByOriginPersonaIdMap::const_iterator ei = pi->second->find(originPersonaId);
            if (ei != pi->second->end())
            {
                UserMap::const_iterator userMapIter = mUserMap.find(ei->second);
                if (userMapIter != mUserMap.end())
                {
                    return userMapIter->second;
                }
            }
        }

        for (UserList::const_iterator i = mOwnedList.begin(), e = mOwnedList.end(); i != e; ++i)
        {
            User* user = (User*) &(*i);
            if (user->isGuestUser())  // Guests share ids with their parent and only support BlazeId lookup.
                continue;

            if (user->getPlatformInfo().getClientPlatform() == platform && user->getPlatformInfo().getEaIds().getOriginPersonaId() == originPersonaId)
            {
                return user;
            }
        }
        return nullptr;
    }

    User *UserManager::getUserByExternalIdInternal( ExternalId extid ) const
    {
        switch (mBlazeHub->getClientPlatformType())
        {
        case xone:
        case xbsx:
            return getUserByXblAccountIdInternal((ExternalXblAccountId)extid);
        case ps4:
        case ps5:
            return getUserByPsnAccountIdInternal((ExternalPsnAccountId)extid);
        case steam:
            return getUserBySteamAccountIdInternal((ExternalSteamAccountId)extid);
        case stadia:
            return getUserByStadiaAccountIdInternal((ExternalStadiaAccountId)extid);
        case pc:
            return getUserByNucleusAccountIdInternal(pc, (AccountId)extid);
        default:
            break;
        }
        return nullptr;
    }
    
    User *UserManager::getUserByPlatformInfoInternal(const PlatformInfo& platformInfo) const
    {
        switch (platformInfo.getClientPlatform())
        {
        case xone:
        case xbsx:
            return getUserByXblAccountIdInternal(platformInfo.getExternalIds().getXblAccountId());
        case ps4:
        case ps5:
            return getUserByPsnAccountIdInternal(platformInfo.getExternalIds().getPsnAccountId());
        case steam:
            return getUserBySteamAccountIdInternal(platformInfo.getExternalIds().getSteamAccountId());
        case stadia:
            return getUserByStadiaAccountIdInternal(platformInfo.getExternalIds().getStadiaAccountId());
        case nx:
             return getUserBySwitchIdInternal(platformInfo.getExternalIds().getSwitchId());
        case pc:
            return getUserByNucleusAccountIdInternal(pc, platformInfo.getEaIds().getNucleusAccountId());
        default:
            return nullptr;
        }
    }

    void UserManager::internalLookupUserCb(const Blaze::UserData *response, BlazeError error, JobId jobId, LookupUserCb titleCb)
    {    
        if( error == ERR_OK )
        {
            User* user = acquireUser(response, User::CACHED);
            titleCb(error, jobId, const_cast<const User *>(user) );
        }
        else
        {
            titleCb(error, jobId, (const User*)nullptr);
        }
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    JobId UserManager::lookupUserByBlazeId( BlazeId blazeId, const LookupUserCb &titleLookupUserCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        //Check local cache before triggering RPC
        const User *user = getUser(blazeId);
        if(isValidUser(user))
        {
            //user in local cache and has external id set
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("lookupUserByBlazeIdCb", titleLookupUserCb, ERR_OK, jobId, user,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), jobId, titleLookupUserCb);
            return jobId;
        }
        else //need to hit server for user
        {
            UserIdentification lookupUserRequest(MEM_GROUP_FRAMEWORK_TEMP);
            lookupUserRequest.setBlazeId(blazeId);
            //UM_TODO :: change to primary component manager instead of user index 0 after fix for issue 865
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
            JobId rpcJobId = userSessions->lookupUser(lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalLookupUserCb), titleLookupUserCb, jobId);
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , rpcJobId, titleLookupUserCb);
            return rpcJobId;
        }
    }

    JobId UserManager::lookupUserByName( const char8_t* name, const LookupUserCb &titleLookupUserCb)
    {
        return lookupUserByName(nullptr, name, titleLookupUserCb);
    }

    JobId UserManager::lookupUserByName( const char8_t* personaNamespace, const char8_t* name, const LookupUserCb &titleLookupUserCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        //Check local cache before triggering RPC
        const User *user = getUser(personaNamespace, name);
        if(isValidUser(user))
        {
            //user in local cache and has external id set
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("lookupUserByNameCb", titleLookupUserCb, ERR_OK, jobId, user,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , jobId, titleLookupUserCb);
            return jobId;
        }
        else //need to hit server for user
        {
            UserIdentification lookupUserRequest(MEM_GROUP_FRAMEWORK_TEMP);

            if (personaNamespace == nullptr)
                personaNamespace = mBlazeHub->getConnectionManager()->getPersonaNamespace();

            lookupUserRequest.setPersonaNamespace(personaNamespace);
            lookupUserRequest.setName(name);
            //UM_TODO :: change to primary component manager instead of user index 0 after fix for issue 865
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
            JobId rpcJobId = userSessions->lookupUser(lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalLookupUserCb), titleLookupUserCb, jobId);
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUserCb);
            return rpcJobId;
        }
    }

    JobId UserManager::lookupUserByNameMultiNamespace(const char8_t* name, const LookupUsersCb &titleLookupUsersCb, bool primaryNamespaceOnly)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        LookupUsersByPersonaNameMultiNamespaceRequest lookupUsersRequest;
        lookupUsersRequest.setPersonaName(name);
        lookupUsersRequest.setPrimaryNamespaceOnly(primaryNamespaceOnly);

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUsersByPersonaNameMultiNamespace(
                lookupUsersRequest, 
                MakeFunctor(this, &UserManager::internalLookupUsersCb),
                titleLookupUsersCb,
                userVector,
                mBlazeHub->getScheduler()->reserveJobId()
                );

         Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);

         return rpcJobId;
    }

    JobId UserManager::lookupUserByExternalId( ExternalId extid, const LookupUserCb &titleLookupUserCb)
    {
        switch (mBlazeHub->getClientPlatformType())
        {
        case ps4:
        case ps5:
            return lookupUserByPsnId((ExternalPsnAccountId)extid, titleLookupUserCb);
        case xone:
        case xbsx:
            return lookupUserByXblId((ExternalXblAccountId)extid, titleLookupUserCb);
        case steam:
            return lookupUserBySteamId((ExternalSteamAccountId)extid, titleLookupUserCb);
        case stadia:
            return lookupUserByStadiaId((ExternalStadiaAccountId)extid, titleLookupUserCb);
        case pc:
            return lookupPcUserByNucleusAccountId((AccountId)extid, titleLookupUserCb);
        default:
            break;
        }
        return INVALID_JOB_ID;
    }
    JobId UserManager::lookupUserByPlatformInfo(const PlatformInfo& platformInfo, const LookupUserCb &titleLookupUserCb)
    {
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupUserByPlatformInfo");
    }
    JobId UserManager::lookupUserByPsnId(ExternalPsnAccountId psnId, const LookupUserCb &titleLookupUserCb)
    {
        PlatformInfo platformInfo;
#if defined(EA_PLATFORM_PS5)
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)psnId, nullptr, ps5);
#else
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)psnId, nullptr, ps4);
#endif
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupUserByPsnId");
    }
    JobId UserManager::lookupUserByXblId(ExternalXblAccountId xblId, const LookupUserCb &titleLookupUserCb)
    {
        PlatformInfo platformInfo;
#if defined(EA_PLATFORM_XBSX)
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)xblId, nullptr, xbsx);
#else
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)xblId, nullptr, xone);
#endif
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupUserByXblId");
    }
    JobId UserManager::lookupUserBySteamId(ExternalSteamAccountId steamId, const LookupUserCb &titleLookupUserCb)
    {
        PlatformInfo platformInfo;
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)steamId, nullptr, steam);
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupUserBySteamId");
    }
    JobId UserManager::lookupUserByStadiaId(ExternalStadiaAccountId stadiaId, const LookupUserCb &titleLookupUserCb)
    {
        PlatformInfo platformInfo;
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)stadiaId, nullptr, stadia);
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupUserByStadiaId");
    }
    JobId UserManager::lookupUserBySwitchId(const char8_t* switchId, const LookupUserCb &titleLookupUserCb)
    {
        PlatformInfo platformInfo;
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, INVALID_EXTERNAL_ID, switchId, nx);
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupUserBySwitchId");
    }
    JobId UserManager::lookupPcUserByNucleusAccountId(AccountId accountId, const LookupUserCb &titleLookupUserCb)
    {
        // This function only looks up pc users (since PC users previously duplicated their nucleus id in the external id field)
        PlatformInfo platformInfo;
        mBlazeHub->setExternalStringOrIdIntoPlatformInfo(platformInfo, (ExternalId)accountId, nullptr, pc);
        return internalLookupUserByPlatformInfo(platformInfo, titleLookupUserCb, "lookupPcUserByNucleusAccountId");
    }

    JobId UserManager::internalLookupUserByPlatformInfo(const PlatformInfo& platformInfo, const LookupUserCb &titleLookupUserCb, const char8_t* caller)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        //Check local cache before triggering RPC
        const User *user = getUserByPlatformInfo(platformInfo);
        if (isValidUser(user))
        {
            //user in local cache and has external id set
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("lookupUserByPlatformInfoCb", titleLookupUserCb, ERR_OK, jobId, user,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), jobId, titleLookupUserCb);
            return jobId;
        }
        else //need to hit server for user
        {
            LookupUsersCrossPlatformRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
            lookupUsersRequest.setLookupType(FIRST_PARTY_ID);
            lookupUsersRequest.getLookupOpts().setBits(0);
            lookupUsersRequest.getLookupOpts().setClientPlatformOnly();
            PlatformInfoPtr newPlatformInfo = lookupUsersRequest.getPlatformInfoList().pull_back();
            platformInfo.copyInto(*newPlatformInfo);

            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
            JobId rpcJobId = userSessions->lookupUsersCrossPlatform(
                lookupUsersRequest,
                MakeFunctor(this, &UserManager::internalLookupUserByPlatformInfoCb),
                titleLookupUserCb,
                caller,
                jobId
            );

            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUserCb);
            return rpcJobId;
        }
    }

    /*! ****************************************************************************/
    /*! \brief Customize the interval used for re-fetching cached User information from the server. 
        \note Cannot be set to a value less than MIN_CACHED_USER_REFRESH_INTERVAL_MS.
        \param cachedUserRefreshIntervalMs  The interval used by the client to determine whether CACHED user
        info has expired and needs to be re-fetched from the Blaze server.
    ********************************************************************************/
    void UserManager::setCachedUserRefreshInterval(uint32_t cachedUserRefreshIntervalMs)
    {
        if(cachedUserRefreshIntervalMs >= MIN_CACHED_USER_REFRESH_INTERVAL_MS)
        {
            mCachedUserRefreshIntervalMs = cachedUserRefreshIntervalMs;
        }
        else
        {
            BLAZE_SDK_DEBUGF(
                "[UserManager:%p] "
                "Specified cachedUserRefreshIntervalMs(%" PRIu32 ") < MIN_CACHED_USER_REFRESH_INTERVAL_MS(%" PRIu32 "), "
                "setting mCachedUserRefreshIntervalMs to MIN_CACHED_USER_REFRESH_INTERVAL_MS instead.\n",
                this, cachedUserRefreshIntervalMs, MIN_CACHED_USER_REFRESH_INTERVAL_MS
            );
            mCachedUserRefreshIntervalMs = MIN_CACHED_USER_REFRESH_INTERVAL_MS;
        }
    }

    void UserManager::clearLookupCache()
    {
        // NOTE: Since cached User objects have RefCount == 0, they are always
        // deleted at once; therefore, it is safe to loop while deleting them.
        while(!mCachedList.empty())
        {
            releaseUser(static_cast<User*>(&mCachedList.front()));
        }
    }

    void UserManager::trimUserPool()
    {
        mUserPool.trim();
    }

    void UserManager::reserveUserPool()
    {
        mUserPool.reserve(mMaxCachedUserCount, MEM_NAME(MEM_GROUP_FRAMEWORK, "UserPool"));
    }

    void UserManager::setLocalUserCanTransitionToAuthenticated(uint32_t userIndex, bool canTransitionToAuthenticated)
    {
        if (mLocalUserVector.size() > userIndex)
        {
            LocalUser *localUser = mLocalUserVector[userIndex];
            if (localUser != nullptr)
                localUser->setCanTransitionToAuthenticated(canTransitionToAuthenticated);
        }
    }

    void UserManager::onUserAuthenticated(const UserSessionLoginInfo* userSessionLoginInfo, uint32_t userIndex)
    {
        LocalUser *localUser = nullptr;

        // we just authenticated; add the local player
        User* user = acquireUser(
            userSessionLoginInfo->getBlazeId(),
            userSessionLoginInfo->getPlatformInfo(),
            true,  // Logged in users are always primary personas
            userSessionLoginInfo->getPersonaNamespace(),
            userSessionLoginInfo->getDisplayName(),
            userSessionLoginInfo->getAccountLocale(),
            userSessionLoginInfo->getAccountCountry(),
            nullptr,
            User::OWNED
        );
        localUser = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "LocalUser") LocalUser(*user, userIndex, userSessionLoginInfo->getUserSessionType(), *mBlazeHub, MEM_GROUP_FRAMEWORK);

        if (localUser != nullptr)
        {
            mLocalUserVector[userIndex] = localUser;
            localUser->finishAuthentication(*userSessionLoginInfo);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    void UserManager::onUserUnauthenticated(const UserSessionLogoutInfo* userSessionLogoutInfo, uint32_t userIndex)
    {
        if (userIndex >= mLocalUserVector.size())
        {
            BLAZE_SDK_DEBUGF("Warning! UserIndex(%u) is out of bounds.\n", userIndex);
            return;
        }

        LocalUser *localUser = mLocalUserVector[userIndex];
        if (localUser != nullptr)
        {
            if (localUser->getId() == userSessionLogoutInfo->getBlazeId())
            {
                if (userSessionLogoutInfo->getForcedLogoutReason() != Blaze::FORCED_LOGOUTTYPE_INVALID)
                    onLocalUserForcedDeAuthenticated(userIndex, userSessionLogoutInfo->getForcedLogoutReason());
                
                onLocalUserDeAuthenticated(userIndex);
            }
        }
    }

    void UserManager::onDisconnected(BlazeError errorCode)
    {
        // on disconnect, clean up all users.
        for (uint32_t userIndex = 0; userIndex < mBlazeHub->getNumUsers(); ++userIndex)
        {
            LocalUser *localUser = mLocalUserVector[userIndex];
            if (localUser != nullptr)
            {
                onLocalUserDeAuthenticated(userIndex);
            }
        }
    }

    void UserManager::onExtendedDataUpdated(const UserSessionExtendedDataUpdate* update, uint32_t userIndex)
    {
        User* user = getUser(update->getBlazeId());
        if (user != nullptr)
        {
            user->setSubscribed(update->getSubscribed());

            // When MLU is used, we may get 2+ updates when a local user UED updates.
            // One will have full info (for the user index that updated), and the others will be missing the mAddress field. 
            // Here we check, and only do the full UED update for a local user when the they're the one getting the update (not their MLU friends).  
            LocalUser* localUser = getLocalUserById(update->getBlazeId());
            if (localUser == nullptr || localUser == getLocalUser(userIndex))
            {
                user->setExtendedData(&update->getExtendedData());
            }
            
            mUserEventDispatcher.dispatch<Blaze::BlazeId>("onExtendedUserDataInfoChanged",
                &UserEventListener::onExtendedUserDataInfoChanged,
                update->getBlazeId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    void UserManager::onUserAdded(const NotifyUserAdded* userAdded, uint32_t userIndex)
    {
        User* user = acquireUser(userAdded->getUserInfo(), User::OWNED);
        
        if(!user->isSubscribedByIndex(userIndex))
        {
            user->setSubscriberIndex(userIndex);

            user->setExtendedData(&userAdded->getExtendedData());
            mUserEventDispatcher.dispatch<Blaze::BlazeId>("onExtendedUserDataInfoChanged",
                &UserEventListener::onExtendedUserDataInfoChanged,
                user->getId());
        }
        else
        {
            BLAZE_SDK_DEBUGF(
                "Warning! UserIndex(%u) is already subscribed to User(%" PRId64 ", %s)\n", 
                userIndex, user->getId(), user->getName()
            );
            // something went horribly wrong, release the reference we took in acquireUser()
            releaseUser(user);
        }
    }

    void UserManager::onUserUpdated(const UserStatus* userStatus, uint32_t userIndex)
    {
        // user index does not matter on update, we just need to update the shared information.
        User* user = getUser(userStatus->getBlazeId());
        if (user != nullptr)
        {
            user->setStatusFlags(userStatus->getStatusFlags());

            mUserEventDispatcher.dispatch<const User&>("onUserUpdated",
                &UserEventListener::onUserUpdated,
                *user);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    void UserManager::onUserRemoved(const NotifyUserRemoved* userRemoved, uint32_t userIndex)
    {
        User* user = getUser(userRemoved->getBlazeId());
        if (user != nullptr)
        {
            // Make sure we don't remove a fellow local user, rely on logout to clean them up.
            // MLU - terribly inefficient, though small vector size...
            for (uint32_t index = 0; index < mLocalUserVector.size(); ++index)
            {
                if ((mLocalUserVector[index] != nullptr))
                {
                    if (mLocalUserVector[index]->getId() == user->getId())
                    {
                        return;
                    }
                }
            }
            if(user->isSubscribedByIndex(userIndex))
            {
                user->clearSubscriberIndex(userIndex);
                // check if we are unsubscribing because user went offline
                releaseUser(user);
            }
            else
            {
                BLAZE_SDK_DEBUGF(
                    "Warning! UserIndex(%u) is NOT subscribed to User(%" PRId64 ", %s)\n", 
                    userIndex, user->getId(), user->getName()
                );
            }
        }
    }

    void UserManager::onNotifyQosSettingsUpdated(const QosConfigInfo* qosConfigInfo, uint32_t userIndex)
    {
        // Note: userIndex is irrelevant here; the blazeserver will send only one NotifyQosSettingsUpdated notification per Blaze connection
        mBlazeHub->getConnectionManager()->resetQosManager(*qosConfigInfo);
    }

    /*! ****************************************************************************/
    /*! \brief Internal callback for processing the response from a lookup users request
    used for lookupUsersByBlazeId, lookupUsersByName and lookupUsersByExternalId.
    Handles caching the user object created within UserManager

    \param[in] response                        The server's UserIdentification response
    \param[in] error                        The Blaze error code returned
    \param{in] jobId                        The JobId of the returning lookup
    \param[in] titleCb                        The functor dispatched upon completion
    \param[in] cachedUsers                  Vector if users that didn't require a server lookup.

    ********************************************************************************/
    void UserManager::internalLookupUsersCb(const Blaze::UserDataResponse *response, BlazeError error, JobId jobId, LookupUsersCb titleCb, UserVectorPtr cachedUsers)
    {
        if( error == ERR_OK )
        {
            if (response != nullptr)
            {
                const UserDataList& userList = response->getUserDataList();
                cachedUsers->reserve(cachedUsers->size() + userList.size());

                for(UserDataList::const_iterator i = userList.begin(), e = userList.end(); i != e; ++i)
                {
                    cachedUsers->push_back(acquireUser(*i, User::CACHED));
                }
            }

            titleCb(error, jobId, *cachedUsers );
        }
        else
        {
            // Return ERR_OK if there is at least one user in the cache
            if (error == USER_ERR_USER_NOT_FOUND && cachedUsers->size() > 0)
            {
                error = ERR_OK;
            }
            titleCb(error, jobId, *cachedUsers);
        }
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ****************************************************************************/
    /*! \brief Internal callback for processing the response from a lookupUsersCrossPlatform request that returns a single user.
    Used for lookupUserByPlatformInfo.
    Handles caching the user object created within UserManager.

        \param[in] response                        The server's UserData response
        \param[in] error                        The Blaze error code returned
        \param[in] jobId                        The JobId of the returning lookup
        \param[in] titleCb                        The functor dispatched upon completion

    ********************************************************************************/
    void UserManager::internalLookupUserByPlatformInfoCb(const Blaze::UserDataResponse *response, BlazeError error, JobId jobId, LookupUserCb titleCb, const char8_t* caller)
    {
        if (error == ERR_OK && response->getUserDataList().size() != 1)
        {
            if (response->getUserDataList().empty())
            {
                error = USER_ERR_USER_NOT_FOUND;
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UserManager] Warning: %s returned %u results (expected single result). Only the first result will be used.\n", caller, response->getUserDataList().size());
            }
        }
        if (error == ERR_OK)
        {
            User* user = acquireUser(*response->getUserDataList().begin(), User::CACHED);
            titleCb(error, jobId, const_cast<const User *>(user));
        }
        else
        {
            titleCb(error, jobId, (const User*)nullptr);
        }
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ****************************************************************************/
    /*! \brief Attempt to lookup user given blazeIds. Makes a request against the blaze server if the local
    user objects aren't complete.
    \param[in] blazeIds                        The BlazeId of the user to lookup
    \param[in] titleLookupUsersCb            The functor dispatched upon success/failure
    \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::lookupUsersByBlazeId(const BlazeIdVector *blazeIds, const LookupUsersCb &titleLookupUsersCb)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        userVector->reserve(blazeIds->size());
        LookupUsersRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUsersRequest.setLookupType(LookupUsersRequest::BLAZE_ID);
        UserIdentificationList &userIdentificationList = lookupUsersRequest.getUserIdentificationList();
        userIdentificationList.reserve(blazeIds->size());
        //Check local cache before triggering RPC
        BlazeIdVector::const_iterator blazeIdsIter = blazeIds->begin();
        BlazeIdVector::const_iterator blazeIdsEndIter = blazeIds->end();
        for(; blazeIdsIter != blazeIdsEndIter; blazeIdsIter++)
        {
            User *user = getUser((*blazeIdsIter));
            if(isValidUser(user))
            {
                //user in local cache and has external id set
                userVector->push_back(user);
            }
            else //need to hit server for this user
            {
                UserIdentification *userIdentification = userIdentificationList.pull_back();
                userIdentification->setBlazeId((*blazeIdsIter));
            }
        }
        return internalLookupUsers(lookupUsersRequest, userVector, titleLookupUsersCb);
    }


    JobId UserManager::lookupUsersByName(const PersonaNameVector *names, const LookupUsersCb& titleLookupUsersCb)
    {
        return lookupUsersByName(nullptr, names, titleLookupUsersCb);
    }

    JobId UserManager::lookupUsersByName(const char8_t* personaNamespace, const PersonaNameVector *names, const LookupUsersCb &titleLookupUsersCb)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        userVector->reserve(names->size());
        LookupUsersByPersonaNamesRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
        
        if (personaNamespace == nullptr)
            personaNamespace = mBlazeHub->getConnectionManager()->getPersonaNamespace();

        lookupUsersRequest.setPersonaNamespace(personaNamespace);
        PersonaNameList& personaNameList = lookupUsersRequest.getPersonaNameList();
        personaNameList.reserve(names->size());

        PersonaNameVector::const_iterator namesIter = names->begin();
        PersonaNameVector::const_iterator namesEndIter = names->end();
        for(; namesIter != namesEndIter; namesIter++)
        {
            if (!getUsers(personaNamespace, *namesIter, userVector))
            {
                //need to hit server for this persona name
                personaNameList.push_back(*namesIter);
            }
        }

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUsersByPersonaNames(
                lookupUsersRequest, 
                MakeFunctor(this, &UserManager::internalLookupUsersCb),
                titleLookupUsersCb,
                userVector,
                mBlazeHub->getScheduler()->reserveJobId()
                );
        
         Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);

         return rpcJobId;
    }

    JobId UserManager::lookupUsersByNameMultiNamespace(const PersonaNameVector *names, const LookupUsersCb& titleLookupUsersCb, bool primaryNamespaceOnly)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        userVector->reserve(names->size());
        LookupUsersByPersonaNamesMultiNamespaceRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUsersRequest.setPrimaryNamespaceOnly(primaryNamespaceOnly);
        PersonaNameList& personaNameList = lookupUsersRequest.getPersonaNameList();
        personaNameList.reserve(names->size());
        PersonaNameVector::const_iterator namesIter = names->begin();
        PersonaNameVector::const_iterator namesEndIter = names->end();
        for(; namesIter != namesEndIter; namesIter++)
        {
            personaNameList.push_back(*namesIter);
        }

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUsersByPersonaNamesMultiNamespace(
                lookupUsersRequest, 
                MakeFunctor(this, &UserManager::internalLookupUsersCb),
                titleLookupUsersCb,
                userVector,
                mBlazeHub->getScheduler()->reserveJobId()
                );

         Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);

         return rpcJobId;
    }

    JobId UserManager::lookupUsersByNamePrefix( const char8_t* prefixName, const uint32_t maxResultCountPerPlatform, const LookupUsersByPrefixCb &titleLookupUsersCb, bool primaryNamespaceOnly)
    {
        return lookupUsersByNamePrefix(nullptr, prefixName, maxResultCountPerPlatform, titleLookupUsersCb, primaryNamespaceOnly);
    }

    JobId UserManager::lookupUsersByNamePrefix( const char8_t* personaNamespace, const char8_t* prefixName, const uint32_t maxResultCountPerPlatform, const LookupUsersByPrefixCb &titleLookupUsersCb, bool primaryNamespaceOnly)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        LookupUsersByPrefixRequest lookupUserRequest;

        if (personaNamespace == nullptr)
            personaNamespace = mBlazeHub->getConnectionManager()->getPersonaNamespace();

        lookupUserRequest.setPersonaNamespace(personaNamespace);
        lookupUserRequest.setPrefixName(prefixName); 
        lookupUserRequest.setMaxResultCount(maxResultCountPerPlatform);
        lookupUserRequest.setPrimaryNamespaceOnly(primaryNamespaceOnly);

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUsersByPrefix(
                lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalLookupUsersCb),
                titleLookupUsersCb,
                userVector,
                mBlazeHub->getScheduler()->reserveJobId()
                );

         Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);

         return rpcJobId;
    }

    JobId UserManager::lookupUsersByNamePrefixMultiNamespace( const char8_t* prefixName, const uint32_t maxResultCountPerPlatform, const LookupUsersByPrefixCb &titleLookupUsersCb, bool primaryNamespaceOnly)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        LookupUsersByPrefixMultiNamespaceRequest lookupUserRequest;
        lookupUserRequest.setPrefixName(prefixName); 
        lookupUserRequest.setMaxResultCount(maxResultCountPerPlatform);
        lookupUserRequest.setPrimaryNamespaceOnly(primaryNamespaceOnly);

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUsersByPrefixMultiNamespace(
                lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalLookupUsersCb),
                titleLookupUsersCb,
                userVector,
                mBlazeHub->getScheduler()->reserveJobId()
                );

         Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);

         return rpcJobId;
    }

    /*! ****************************************************************************/
    /*! \brief (DEPRECATED) Attempt to lookup users given external ids. Makes a request against the blaze server if the local
    user objects aren't complete; Not efficient, prefer lookup by BlazeId when possible.

    \param[in] extids                        The ExternalId of the user to lookup
    \param[in] titleLookupUserCb            The functor dispatched upon success/failure
    \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::lookupUsersByExternalId(const ExternalIdVector *extids, const LookupUsersCb &titleLookupUsersCb)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        userVector->reserve(extids->size());
        LookupUsersRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUsersRequest.setLookupType(LookupUsersRequest::EXTERNAL_ID);

        UserIdentificationList &userIdentificationList = lookupUsersRequest.getUserIdentificationList();
        userIdentificationList.reserve(extids->size());
        //Check local cache before triggering RPC
        for(ExternalId extId : *extids)
        {
            User *user = getUserByExternalIdInternal(extId);
            if(isValidUser(user))
            {
                //user in local cache and has external id set
                userVector->push_back(user);
            }
            else //need to hit server for this user
            {
                UserIdentification *userIdentification = userIdentificationList.pull_back();
                mBlazeHub->setExternalStringOrIdIntoPlatformInfo(userIdentification->getPlatformInfo(), extId, nullptr, mBlazeHub->getClientPlatformType());
            }
        }
        return internalLookupUsers(lookupUsersRequest, userVector, titleLookupUsersCb);
    }

    /*! ****************************************************************************/
    /*! \brief Attempt to lookup users given account ids. Always makes a request against the blaze server; 
    Not efficient, prefer lookup by BlazeId when possible.

    WARNING This method finds users on the caller's platform only. Use lookupUsersCrossPlatform
    for cross-platform lookups.

    \param[in] accids                        The NucleusId of the user to lookup
    \param[in] titleLookupUserCb            The functor dispatched upon success/failure
    \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::lookupUsersByAccountId(const AccountIdVector *accids, const LookupUsersCb &titleLookupUsersCb)
    {
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        userVector->reserve(accids->size());
        LookupUsersRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUsersRequest.setLookupType(LookupUsersRequest::ACCOUNT_ID);
        UserIdentificationList &userIdentificationList = lookupUsersRequest.getUserIdentificationList();
        userIdentificationList.reserve(accids->size());
        // NOTE: We have to bypass the cache because since each account ID may have multiple Users
        // associated with it, a server fetch is always required to determine whether we are missing any
        AccountIdVector::const_iterator accidsIter = accids->begin();
        AccountIdVector::const_iterator accidsEndIter = accids->end();
        for(; accidsIter != accidsEndIter; accidsIter++)
        {
            UserIdentification *userIdentification = userIdentificationList.pull_back();
            userIdentification->getPlatformInfo().getEaIds().setNucleusAccountId((*accidsIter));
        }
        return internalLookupUsers(lookupUsersRequest, userVector, titleLookupUsersCb);
    }

    JobId UserManager::lookupUserByOriginPersonaId(OriginPersonaId originPersonaId, const LookupUserCb &titleLookupUserCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        ClientPlatformType platform = mBlazeHub->getClientPlatformType();
        //Check local cache before triggering RPC
        const User *user = getUserByOriginPersonaIdInternal(platform, originPersonaId);
        if(isValidUser(user))
        {
            //user in local cache and has external id set
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("lookupUserByOriginPersonaIdCb", titleLookupUserCb, ERR_OK, jobId, user,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , jobId, titleLookupUserCb);
            return jobId;
        }
        else //need to hit server for user
        {
            UserIdentification lookupUserRequest(MEM_GROUP_FRAMEWORK_TEMP);
            lookupUserRequest.getPlatformInfo().setClientPlatform(platform);
            lookupUserRequest.getPlatformInfo().getEaIds().setOriginPersonaId(originPersonaId);
            //UM_TODO :: change to primary component manager instead of user index 0 after fix for issue 865
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
            JobId rpcJobId = userSessions->lookupUser( lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalLookupUserCb), titleLookupUserCb, jobId);
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUserCb);
            return rpcJobId;
        }
    }

    /*! ****************************************************************************/
    /*! \brief Attempt to lookup users given origin persona ids. Makes a request against the blaze server if the local
    user objects aren't complete; Not efficient, prefer lookup by BlazeId when possible.

    WARNING This method finds users on the caller's platform only. Use lookupUsersCrossPlatform
    for cross-platform lookups.

    \param[in] originPersonaIds              The OriginPersonaId of the user to lookup
    \param[in] titleLookupUsersCb            The functor dispatched upon success/failure
    \return                                  The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::lookupUsersByOriginPersonaId(const OriginPersonaIdVector *originPersonaIds, const LookupUsersCb &titleLookupUsersCb)
    {
        ClientPlatformType platform = mBlazeHub->getClientPlatformType();
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        userVector->reserve(originPersonaIds->size());
        LookupUsersRequest lookupUsersRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUsersRequest.setLookupType(LookupUsersRequest::ORIGIN_PERSONA_ID);
        UserIdentificationList &userIdentificationList = lookupUsersRequest.getUserIdentificationList();
        userIdentificationList.reserve(originPersonaIds->size());
        //Check local cache before triggering RPC
        OriginPersonaIdVector::const_iterator originPersonaIdsIter = originPersonaIds->begin();
        OriginPersonaIdVector::const_iterator originPersonaIdsEndIter = originPersonaIds->end();
        for(; originPersonaIdsIter != originPersonaIdsEndIter; originPersonaIdsIter++)
        {
            User *user = getUserByOriginPersonaIdInternal(platform, (*originPersonaIdsIter));
            if(isValidUser(user))
            {
                //user in local cache and has external id set
                userVector->push_back(user);
            }
            else //need to hit server for this user
            {
                UserIdentification *userIdentification = userIdentificationList.pull_back();
                userIdentification->getPlatformInfo().setClientPlatform(platform);
                userIdentification->getPlatformInfo().getEaIds().setOriginPersonaId((*originPersonaIdsIter));
            }
        }
        return internalLookupUsers(lookupUsersRequest, userVector, titleLookupUsersCb);
    }

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
    JobId UserManager::lookupUsersCrossPlatform(LookupUsersCrossPlatformRequest& request, const LookupUsersCb &titleLookupUsersCb)
    {
        LookupUsersCrossPlatformRequest localRequest(MEM_GROUP_FRAMEWORK_TEMP);
        UserVectorPtr userVector = UserVectorWrapper::createUserVector();
        bool checkCache = !request.getLookupOpts().getMostRecentPlatformOnly() && !request.getLookupOpts().getOnlineOnly() && (request.getLookupOpts().getClientPlatformOnly() || request.getLookupType() == FIRST_PARTY_ID);

        if (checkCache)
        {
            for (PlatformInfoList::iterator it = request.getPlatformInfoList().begin(); it != request.getPlatformInfoList().end(); ++it)
            {
                User *user = nullptr;
                switch (request.getLookupType())
                {
                case NUCLEUS_ACCOUNT_ID:
                    user = getUserByNucleusAccountIdInternal((*it)->getClientPlatform(), (*it)->getEaIds().getNucleusAccountId());
                    break;
                case ORIGIN_PERSONA_ID:
                    user = getUserByOriginPersonaIdInternal((*it)->getClientPlatform(), (*it)->getEaIds().getOriginPersonaId());
                    break;
                case ORIGIN_PERSONA_NAME:
                    user = getUser(NAMESPACE_ORIGIN, (*it)->getEaIds().getOriginPersonaName(), (*it)->getClientPlatform());
                    break;
                case FIRST_PARTY_ID:
                    user = getUserByPlatformInfoInternal(*(it->get()));
                    break;
                default:
                    break;
                }
                if (isValidUser(user))
                {
                    userVector->push_back(user);
                }
                else
                {
                    localRequest.getPlatformInfoList().push_back(*it);
                }
            }
            if (localRequest.getPlatformInfoList().empty())
            {
                // NOTE: Since JobScheduler::scheduleMethod does not support more than 3 args, use a custom job object.
                JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
                Job* job =
                    BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "lookupUsersCrossPlatformJob")
                    UserManager::InternalUsersLookupCbJob(this, ERR_OK, jobId, titleLookupUsersCb, userVector);
                return mBlazeHub->getScheduler()->scheduleJob("lookupUsersCrossPlatformJob", job, this, 0, jobId); // assocObj, delayMs, reservedJobId
            }
            localRequest.setLookupType(request.getLookupType());
            localRequest.getLookupOpts().setBits(request.getLookupOpts().getBits());
        }

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUsersCrossPlatform(
            checkCache ? localRequest : request,
            MakeFunctor(this, &UserManager::internalLookupUsersCb),
            titleLookupUsersCb,
            userVector,
            mBlazeHub->getScheduler()->reserveJobId()
        );

        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);
        return rpcJobId;
    }

    /*! ****************************************************************************/
    /*! \brief Attempt to lookup users given identity vector. Makes a request against the blaze server if the local
    user objects aren't complete; Not efficient, prefer lookup by BlazeId when possible.

    \param[in] request                        The request containing users to lookup
    \param[in] users                        The vector containing cached users
    \param[in] titleLookupUserCb            The functor dispatched upon success/failure
    \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::internalLookupUsers(const LookupUsersRequest& request, UserVectorPtr users, const LookupUsersCb &titleLookupUsersCb)
    {
        BlazeError rc;
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        const UserIdentificationList &userIdentificationList = request.getUserIdentificationList();
        if(userIdentificationList.empty())
        {
            rc = ERR_OK; // All users are already in the cache
        }
        else if(userIdentificationList.size() + mUserCount - mCachedList.size() > mMaxCachedUserCount)
        {
            rc = ERR_SYSTEM; // TODO: change to ERR_USER_CACHE_EXCEEDED
        }
        else
        {
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
            JobId rpcJobId = userSessions->lookupUsers(
                request, 
                MakeFunctor(this, &UserManager::internalLookupUsersCb),
                titleLookupUsersCb,
                users,
                jobId
            );
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupUsersCb);
            return rpcJobId;
        }
        // NOTE: Since JobScheduler::scheduleMethod does not support more than 3 args, use a custom job object.
        Job* job = 
            BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "UserManagerJob") 
                UserManager::InternalUsersLookupCbJob(this, rc, jobId, titleLookupUsersCb, users);
        return mBlazeHub->getScheduler()->scheduleJob("UserManagerJob", job, this, 0, jobId); // assocObj, delayMs, reservedJobId
    }

    /*! ****************************************************************************/
    /*! \brief Internal callback for processing the response from a fetch of UserData
    It attaches the extended data to a local user object, creating one if needed.

    \param[in] response                        The server's UserData response
    \param[in] error                        The Blaze error code returned
    \param{in] jobId                        The JobId of the returning lookup
    \param[in] titleCb                        The functor dispatched upon completion
    \param[in] blazeId                      The blaze id for the fetched user

    ********************************************************************************/
    void UserManager::internalFetchExtendedDataCb(const UserData *response, BlazeError error, JobId jobId, FetchUserExtendedDataCb titleCb)
    {
        User *user;
        if(error == ERR_OK) //user is logged in
        {
            //can use add user as it will just return pointer to existing user if player already exists in cache
            user = acquireUser(response, User::CACHED);
        }
        else
        {
            user = nullptr;
        }
        titleCb(error, jobId, (const User *)user);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ****************************************************************************/
    /*! \brief Fetch the UserSessionExtended data for the user with the given BlazeId
    \param[in] blazeId                        The BlazeId of the target user
    \param[in] titleFetchUserExtendedDataCb    The functor dispatched upon success/failure
    \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::fetchUserExtendedData(BlazeId blazeId, const FetchUserExtendedDataCb &titleFetchUserExtendedDataCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        //Check local cache before triggering RPC
        const User *user = getUser(blazeId);
        if(isValidUser(user))
        {
            //user in local cache
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("fetchUserExtendedDataCb", titleFetchUserExtendedDataCb, ERR_OK, jobId, user,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , jobId, titleFetchUserExtendedDataCb);
            return jobId;
        }
        else //need to hit server for user
        {
            UserIdentification lookupUserRequest(MEM_GROUP_FRAMEWORK_TEMP);
            lookupUserRequest.setBlazeId(blazeId);
            //UM_TODO :: change to primary component manager instead of user index 0 after fix for issue 865
            Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
            JobId rpcJobId = userSessions->lookupUser(lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalFetchExtendedDataCb), titleFetchUserExtendedDataCb, jobId);
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleFetchUserExtendedDataCb);
            return rpcJobId;
        }
    }


    /*! ****************************************************************************/
    /*! \brief Internal callback for processing the overrideGeoIpdata


    \param[in] error                        The Blaze error code returned
    \param{in] jobId                        The JobId of the returning lookup
    \param[in] titleCb                      The functor dispatched upon completion

    ********************************************************************************/
    void UserManager::internalOverrideUserGeoIPDataCb(BlazeError error, JobId jobId, OverrideGeoCb titleCb)
    {
        titleCb(error, jobId);
    } /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    /*! ****************************************************************************/
    /*! \brief Override a users GeoIPData. Tells server to not use GeoIP database but 
            specific data provided.
        \param[in] geoData     The data used to override
        \param[in] titleOverrideGeoCb            The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::overrideUsersGeoIPData(const GeoLocationData* geoData, const OverrideGeoCb &titleOverrideGeoCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();

        GeoLocationData overrideGeoLocationRequest(MEM_GROUP_FRAMEWORK_TEMP);
        geoData->copyInto(overrideGeoLocationRequest);
            
        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();

        JobId rpcJobId = userSessions->overrideUserGeoIPData(overrideGeoLocationRequest, 
                MakeFunctor(this, &UserManager::internalOverrideUserGeoIPDataCb), titleOverrideGeoCb, jobId);
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleOverrideGeoCb);
        return rpcJobId;
    }

    /*! ****************************************************************************/
    /*! \brief Opt-in/out the user's GeoIPData. Tells server to not expose user's GeoIP data
        to others.
        \param[in] optInFlag     The data used to override
        \param[in] titleOverrideGeoCb            The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::setUserGeoOptIn(const bool optInFlag, const GeoOptInCb &titleGeoOptInCbCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();

        OptInRequest request(MEM_GROUP_FRAMEWORK_TEMP);
        request.setOptIn(optInFlag);
            
        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
         
        JobId rpcJobId = userSessions->setUserGeoOptIn(request, 
                MakeFunctor(this, &UserManager::internalOverrideUserGeoIPDataCb), titleGeoOptInCbCb, jobId);
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleGeoOptInCbCb);
        return rpcJobId;
    }

    /*! ************************************************************************************************/
    /*! \brief Internal callback for processing the response from a fetch of LookupUserGeoIPData
                
            \param[in] response                     The server's GeoLocationData response
            \param[in] error                        The Blaze error code returned
            \param[in] jobId                        The JobId of the returning lookup
            \param[in] titleCb                      The functor dispatched upon completion

   ***************************************************************************************************/
    void UserManager::internalLookupUserGeoIPDataCb(const Blaze::GeoLocationData *response, BlazeError error, JobId jobId, LookupGeoCb titleCb)
    {
        titleCb(error, jobId, response);
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    /*! ****************************************************************************/
    /*! \brief Lookup a users GeoIPData.
        \param[in] blazeId     BlazeId to fetch data
        \param[in] titleLookupGeoCb            The functor dispatched upon success/failure
        \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::lookupUsersGeoIPData(const BlazeId blazeId, const LookupGeoCb &titleLookupGeoCb)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();

        UserIdentification lookupUserRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUserRequest.setBlazeId(blazeId);
            
        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
         

        JobId rpcJobId = userSessions->lookupUserGeoIPData(lookupUserRequest, 
                MakeFunctor(this, &UserManager::internalLookupUserGeoIPDataCb), titleLookupGeoCb, jobId);
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleLookupGeoCb);
        return rpcJobId;
    }

    LocalUserGroup::LocalUserGroup(BlazeHub *blazeHub, MemoryGroupId memGroupId) :
        mBlazeHub(blazeHub),
        mUserIndicesInGroup(memGroupId, MEM_NAME(memGroupId, "LocalUserGroup::mUserIndicesInGroup")),
        mId(EA::TDF::ObjectId(getEntityType(), static_cast<EntityId>(0)))
    {
        BlazeAssertMsg(blazeHub != nullptr, "Blaze hub must be not nullptr!");
    }

    LocalUserGroup::~LocalUserGroup()
    {
        mUserIndicesInGroup.clear();
        mId = EA::TDF::ObjectId(getEntityType(), static_cast<EntityId>(0));
    }

    JobId LocalUserGroup::updateLocalUserGroup(const Blaze::ConnectionUserIndexList& userIndices, const UpdateLUGroupCb& titleUpdateCb)
    {
        UpdateLocalUserGroupRequest request(MEM_GROUP_FRAMEWORK_TEMP);

        for (Blaze::ConnectionUserIndexList::const_iterator i = userIndices.begin(), e = userIndices.end(); i != e; ++i)
        {
            uint32_t userIndex = *i;
            LocalUser* localUser = mBlazeHub->getUserManager()->getLocalUser(userIndex);
            if (localUser != nullptr)
            {
                request.getConnUserIndexList().push_back(userIndex);
            }
        }

        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
         
        JobId rpcJobId = userSessions->updateLocalUserGroup(request, 
                MakeFunctor(this, &LocalUserGroup::internalUpdateLocalUserGroupCb), titleUpdateCb, jobId);
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleUpdateCb);
        return rpcJobId;
    }

    /*! ****************************************************************************/
    /*! \brief Remove logged-out or disconnected local users from local user group.
    ********************************************************************************/
    void LocalUserGroup::removeDeAuthenticatedLocalUsers()
    {
        Blaze::ConnectionUserIndexList::iterator iter = mUserIndicesInGroup.begin();
        while (iter != mUserIndicesInGroup.end())
        {
            uint32_t userIndex = *iter;
            LocalUser* localUser = mBlazeHub->getUserManager()->getLocalUser(userIndex);
            if (localUser == nullptr)
            {
                iter = mUserIndicesInGroup.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    /*! ****************************************************************************/
    /*! \brief Internal callback for processing the response from updating local user group.

        \param[in] response                     The server's LocalUserGroup response
        \param[in] error                        The Blaze error code returned
        \param[in] jobId                        The JobId of the returning lookup
        \param[in] titleCb                      The functor dispatched upon completion
    ********************************************************************************/
    void LocalUserGroup::internalUpdateLocalUserGroupCb(const Blaze::UpdateLocalUserGroupResponse *response, BlazeError error, JobId jobId, UpdateLUGroupCb titleCb)
    {
        if (response != nullptr)
        {
            mId = response->getUserGroupObjectId();
            mUserIndicesInGroup.clear();
            response->getConnUserIndexList().copyInto(mUserIndicesInGroup);
        }
        titleCb(error, jobId, response);
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

#ifdef BLAZE_USER_PROFILE_SUPPORT
    /*! ****************************************************************************/
    /*! \brief Returns the user profile with the given external identifier

        Examples of external identifiers include XboxUid for the Xbox 360.

        \param[in] lookupId The FirstPartyLookupId for the user (if applicable)
        \return    The UserProfilePtr (shared_ptr) obj with the associated FirstPartyLookupId, or nullptr if the user doesn't exist locally.
    ********************************************************************************/
    UserProfilePtr UserManager::getUserProfile(FirstPartyLookupId lookupId) const
    {
        UserProfileByFirstPartyLookupIdMap::const_iterator it = mUserProfileByLookupIdMap.find(lookupId);
        if (it != mUserProfileByLookupIdMap.end())
        {
            return it->second;
        }
        return UserProfilePtr();
    }

    void UserManager::resetUserProfileCache()
    {
        // IMPORTANT: The ordering here is key. We MUST remove the UserProfile from the mCachedUserProfileList
        //            intrusive_list first. Removing it from the mUserProfileByLookupIdMap hash_map first might
        //            cause the UserProfilePtr shared_ptr RefCount to ==0, and therefore delete the object.
        //            Thus making it impossible to remove from the mCachedUserProfileList intrusive_list.
        mCachedUserProfileList.clear();
        mUserProfileByLookupIdMap.clear();
        mBlazeHub->getScheduler()->scheduleMethod("resetUserProfileCache", this, &UserManager::resetUserProfileCache, this, RESET_USER_PROFILE_CACHE_INTERVAL_MS);
    }

    JobId UserManager::lookupUserProfileByUser(const User &user, const LookupUserProfileCb &titleCb, int32_t timeoutMs, bool forceUpdate)
    {
        FirstPartyLookupIdVector lookupIdList(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::lookupIdList");

        lookupIdList.push_back(user.getExternalId());

        return internalLookupUserProfiles(lookupIdList, nullptr, &titleCb, timeoutMs, forceUpdate);
    }

    JobId UserManager::lookupUserProfilesByUser(const UserVector &users, const LookupUserProfilesCb &titleCb, int32_t timeoutMs, bool forceUpdate)
    {
        FirstPartyLookupIdVector lookupIdList(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::lookupIdList");
        lookupIdList.reserve(users.size());

        UserVector::const_iterator usersItr = users.begin();
        UserVector::const_iterator usersEnd = users.end();
        for ( ; usersItr != usersEnd; ++usersItr)
        {
            lookupIdList.push_back((*usersItr)->getExternalId());
        }
        return internalLookupUserProfiles(lookupIdList, &titleCb, nullptr, timeoutMs, forceUpdate);
    }

    JobId UserManager::lookupUserProfileByFirstPartyLookupId(FirstPartyLookupId lookupId, const LookupUserProfileCb &titleCb, int32_t timeoutMs, bool forceUpdate, int32_t avatarSize)
    {
        FirstPartyLookupIdVector lookupIdList(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::lookupUserProfileByFirstPartyLookupId::lookupIdList");
        lookupIdList.push_back(lookupId);

        return internalLookupUserProfiles(lookupIdList, nullptr, &titleCb, timeoutMs, forceUpdate, avatarSize);
    }

    JobId UserManager::lookupUserProfilesByFirstPartyLookupId(const FirstPartyLookupIdVector &lookupIdList, const LookupUserProfilesCb &titleCb, int32_t timeoutMs, bool forceUpdate, int32_t avatarSize)
    {
        return internalLookupUserProfiles(lookupIdList, &titleCb, nullptr, timeoutMs, forceUpdate, avatarSize);
    }

    JobId UserManager::internalLookupUserProfiles(const FirstPartyLookupIdVector &lookupIdList, const LookupUserProfilesCb* titleMultiCb, const LookupUserProfileCb* titleSingleCb, int32_t timeoutMs, bool forceUpdate, int32_t avatarSize)
    {
        if (mUserApiRef == nullptr)
            return (uint32_t)SDK_ERR_INVALID_STATE;

        UserApiCallbackData* userApiCallbackData = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "UserManager::internalLookupUserProfiles::userApiCallbackData") UserApiCallbackData;
        mUserApiCallbackDataSet.insert(userApiCallbackData);
        userApiCallbackData->mUserManager = this;
        userApiCallbackData->mUserIndex = mBlazeHub->getLoginManager(getPrimaryLocalUserIndex())->getDirtySockUserIndex();
        userApiCallbackData->mJobId = mBlazeHub->getScheduler()->reserveJobId();
        userApiCallbackData->mCachedUserProfiles.reserve(lookupIdList.size());

        if (titleMultiCb != nullptr)
            userApiCallbackData->mTitleMultiCb = *titleMultiCb;
        else if (titleSingleCb != nullptr)
            userApiCallbackData->mTitleSingleCb = *titleSingleCb;

        DirtyUserT *dirtyUsers = (DirtyUserT*)BLAZE_ALLOC(lookupIdList.size() * sizeof(DirtyUserT), MEM_GROUP_FRAMEWORK_TEMP, "UserManager::lookupUserProfilesById::dirtyUsers");

        // check local cache before triggering call
        int32_t count = 0;
        for (int32_t i = 0; (uint32_t)i < lookupIdList.size(); ++i)
        {
            UserProfilePtr userProfile;
            if (!forceUpdate && (userProfile = getUserProfile(lookupIdList[i])) != nullptr)
            {
                // user profile in local cache
                userApiCallbackData->mCachedUserProfiles.push_back(userProfile);
            }
            else
            {
                // need to hit service for this user profile
                uint64_t nativeId = lookupIdList[i];
                DirtyUserFromNativeUser(&dirtyUsers[count++], &nativeId);
            }
        }

        JobId jobId;
        if (count == 0)
        {
            // If everything was retrieved from the cache, and no first party requests need
            // to be made, then just schedule the title callback.
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("internalLookupUserProfilesCb", 
                MakeFunctor(this, &UserManager::internalLookupUserProfilesCb), ERR_OK, userApiCallbackData,
                    userApiCallbackData, 0, userApiCallbackData->mJobId);
        }
        else
        {
            // kick off the request for the user profiles
            UserApiControl(mUserApiRef, 'avsz', avatarSize, 0, nullptr);
            int32_t ret = UserApiRequestProfilesAsync(mUserApiRef, userApiCallbackData->mUserIndex, dirtyUsers, count, UserManager::_userApiCallback, userApiCallbackData);
            if (ret >= 0)
            {
                InternalLookupUserProfilesJob *job = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "UserManager::lookupUserProfilesById::InternalLookupUserProfilesJob")
                    InternalLookupUserProfilesJob(userApiCallbackData);

                // This is essentially a timeout job.  But also, the user can use the jobId to cancel
                // the async first party request.
                jobId = mBlazeHub->getScheduler()->scheduleJob("internalLookupUserProfilesJob", job, userApiCallbackData, timeoutMs, userApiCallbackData->mJobId);
            }
            else
            {
                BlazeError callbackError;
                switch (ret)
                {
                    case (-2): callbackError = SDK_ERR_FIRST_PARTY_REQUEST_INPROGRESS; break;
                    default:   callbackError = SDK_ERR_FIRST_PARTY_REQUEST_FAILED; break;
                }
                // An error occured, call the title callback.
                jobId = mBlazeHub->getScheduler()->scheduleFunctor("internalLookupUserProfilesCb",
                    MakeFunctor(this, &UserManager::internalLookupUserProfilesCb),
                        callbackError, userApiCallbackData, userApiCallbackData, 0, userApiCallbackData->mJobId);
            }
        }

        BLAZE_FREE(MEM_GROUP_FRAMEWORK_TEMP, dirtyUsers);
        return jobId;
    }

    void UserManager::_userApiCallback(UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, void *pUserData)
    {
        UserApiCallbackData* userApiCallbackData = (UserApiCallbackData*)pUserData;
        userApiCallbackData->mUserManager->userApiCallback(pRef, pUserApiEventData, userApiCallbackData);
    }

    void UserManager::userApiCallback(UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, UserApiCallbackData* userApiCallbackData)
    {
        switch (pUserApiEventData->eEventType)
        {
            case USERAPI_EVENT_DATA:
            {
                // An error can occur after sending a request for a user profile to first party.
                // UserApi tells us a profile request has completed, but it might not have been
                // successfull (e.g. user not found). We only care about the successfull occurrences.
                if (pUserApiEventData->eError == USERAPI_ERROR_OK)
                {
                    UserApiUserDataT *userApiProfile = &pUserApiEventData->EventDetails.UserData;

                    // Because the request/response was asyncronous, there's no telling if another call to
                    // lookupUserProfile has already returned a user we were looking up as well.  So, even
                    // though we checked the cache before sending the request, check it again 
                    FirstPartyLookupId lookupId;
                    DirtyUserToNativeUser(&lookupId, sizeof(lookupId), &userApiProfile->DirtyUser);

                    UserProfilePtr profile = getUserProfile(lookupId);
                    if (profile == nullptr)
                    {
                        // If we don't know about this UserProfile yet, then create it and cache it.
                        profile = UserProfilePtr(UserProfile::Create(*userApiProfile, "UserManager::userApiCallback::profile"));

                        mUserProfileByLookupIdMap[profile->getFirstPartyLookupId()] = profile;
                    }
                    else
                    {
                        // Update the existing profile.
                        profile->update(*userApiProfile);

                        // If we do already know about this UserProfile, then remove it from the list,
                        // It will be bumped to the front below.
                        UserProfileList::remove(*profile);
                    }

                    // Insert at the front of the list (LRU semantics)
                    mCachedUserProfileList.push_front(*profile);

                    // Add this UserProfile onto the list that's being built for the eventual
                    // call to the titles callback functor.  Note that this list contains UserProfilePtr
                    // shared_ptrs.  Therefore, it will remain in memory, even if this instance gets pushed
                    // off the end of the LRU cache as a result of one or more large async profile lookups.
                    userApiCallbackData->mCachedUserProfiles.push_back(profile);

                    // If we're now larger than we want to be, drop the last item.
                    // Note that, in doing so, the UserProfilePtr shared_ptrs ref count will be decremented.
                    // If the object is not also in another list pending a title callback,
                    // it will be deleted automatically due to RefCount==0.
                    if (mCachedUserProfileList.size() > mMaxCachedUserProfileCount)
                    {
                        // IMPORTANT: The ordering here is key. We MUST remove the UserProfile from the mCachedUserProfileList
                        //            intrusive_list first. Removing it from the mUserProfileByLookupIdMap hash_map first might
                        //            cause the UserProfilePtr shared_ptr RefCount to ==0, and therefore delete the object.
                        //            Thus making it impossible to remove from the mCachedUserProfileList intrusive_list.
                        UserProfile &lruProfile = mCachedUserProfileList.back();
                        mCachedUserProfileList.pop_back();
                        mUserProfileByLookupIdMap.erase(lruProfile.getFirstPartyLookupId());
                    }
                }
                break;
            }
            case USERAPI_EVENT_END_OF_LIST:
            {
                // UserApi has received all response from first party.  Call the titles functor.
                BlazeError error;
                switch (pUserApiEventData->eError)
                {
                    case USERAPI_ERROR_OK: error = ERR_OK; break;
                    default:               error = SDK_ERR_FIRST_PARTY_REQUEST_FAILED; break;
                }

                // Find the outstanding job associated with this lookup request, and remove it.
                // Then, reschedule a job using the same JobId, so that things remain consistent
                // from the title codes pov.
                Job *job = mBlazeHub->getScheduler()->getJob(userApiCallbackData->mJobId);
                if (job != nullptr)
                {
                    mBlazeHub->getScheduler()->removeJob(job, true);
                    mBlazeHub->getScheduler()->scheduleFunctor("userApiCallbackCb", 
                        MakeFunctor(this, &UserManager::internalLookupUserProfilesCb), error, userApiCallbackData,
                            userApiCallbackData, 0, userApiCallbackData->mJobId);
                }
                else
                {
                    // If the job doesn't exist, then it has timed out or been cancelled/removed, so don't schedule the titleCb to fire
                    // but check to see if the userApiCallbackData needs to be deleted (in the event that the job was removed)
                    if (userApiCallbackData != nullptr)
                    {
                        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, userApiCallbackData);
                        userApiCallbackData = nullptr;
                    }
                }

                BLAZE_SDK_DEBUGF(
                    "UserManager: UserProfile lookup has finished. Error(%" PRIi32 "), TotalRequested(%" PRIi32 "), TotalReceived(%" PRIi32 "), TotalErrors(%" PRIi32 ")\n",
                    pUserApiEventData->eError,
                    pUserApiEventData->EventDetails.EndOfList.iTotalRequested,
                    pUserApiEventData->EventDetails.EndOfList.iTotalReceived,
                    pUserApiEventData->EventDetails.EndOfList.iTotalErrors);

                break;
            }
        }
    }

    void UserManager::internalLookupUserProfilesCb(BlazeError error, UserApiCallbackData* userApiCallbackData)
    {
        if ((error != SDK_ERR_FIRST_PARTY_REQUEST_CANCELED) && (userApiCallbackData->mCachedUserProfiles.size() > 0))
        {
            error = ERR_OK;
        }

        // Select the appropriate functor to call back into title code.
        if (userApiCallbackData->mTitleMultiCb.isValid())
        {
            userApiCallbackData->mTitleMultiCb(error, userApiCallbackData->mJobId,
                userApiCallbackData->mCachedUserProfiles);
        }
        else if (userApiCallbackData->mTitleSingleCb.isValid())
        {
            userApiCallbackData->mTitleSingleCb(error, userApiCallbackData->mJobId,
                userApiCallbackData->mCachedUserProfiles.size() > 0 ? userApiCallbackData->mCachedUserProfiles.front() : UserProfilePtr());
        }

        mUserApiCallbackDataSet.erase(userApiCallbackData);

        // We're finally done with the callback data.
        if (userApiCallbackData != nullptr)
        {
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, userApiCallbackData);
            userApiCallbackData = nullptr;
        }
    }

    void UserManager::InternalLookupUserProfilesJob::execute()
    {
        // The job has timed out. Abort all pending async requests associated with this user index.
        // Note a single user index is prevented from issuing multiple async requests simultaneously.
        UserApiControl(mUserApiCallbackData->mUserManager->mUserApiRef, 'abrt', mUserApiCallbackData->mUserIndex, 0, nullptr);
        mUserApiCallbackData->mUserManager->internalLookupUserProfilesCb(SDK_ERR_FIRST_PARTY_REQUEST_TIMEOUT, mUserApiCallbackData);
    }

    void UserManager::InternalLookupUserProfilesJob::cancel(BlazeError error)
    {
        // The user has manually cancelled the job. Abort all pending async requests associated with this user index.
        // Note a single user index is prevented from issuing multiple async requests simultaneously.
        UserApiControl(mUserApiCallbackData->mUserManager->mUserApiRef, 'abrt', mUserApiCallbackData->mUserIndex, 0, nullptr);
        mUserApiCallbackData->mUserManager->internalLookupUserProfilesCb(SDK_ERR_FIRST_PARTY_REQUEST_CANCELED, mUserApiCallbackData);
    }
#endif

    /*! ****************************************************************************/
    /*! \brief Internal callback for processing the response from a fetch of UserSessionExtendedData
           as information regarding the user's logged in state.
           It interprets any non ERR_OK BlazeError as a logged out user

    \param[in] response                        The server's UserData response;
    \param[in] error                        The Blaze error code returned
    \param{in] jobId                        The JobId of the returning lookup
    \param[in] titleCb                        The functor dispatched upon completion

    ********************************************************************************/
    void UserManager::internalCheckUserOnlineStatusCb(const UserData *response, BlazeError error, JobId jobId, CheckUserOnlineStatusCb titleCb)
    {
        if(error == ERR_OK) //user is logged in
        {
            //can use add user as it will just return pointer to existing user if player already exists in cache
            //UM_TODO: checkUserOnlineStatus should return a User object 
            /*User *user = */
                acquireUser(response, User::CACHED);
            titleCb(error, jobId, response->getStatusFlags().getOnline());
        }
        else
        {
            titleCb(error, jobId, false);
        }
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ****************************************************************************/
    /*! \brief Check the online status of a given user.
    The target user will have true passed in as the first parameter if the target user
    is logged in.
    \param[in] blazeId                        The BlazeId of the target user
    \param[in] titleCheckOnlineStatusCb     The functor dispatched upon success/failure
    \return                                    The JobId of the RPC call
    ********************************************************************************/
    JobId UserManager::checkUserOnlineStatus(BlazeId blazeId, const CheckUserOnlineStatusCb &titleCheckOnlineStatusCB)
    {
        JobId jobId = mBlazeHub->getScheduler()->reserveJobId();
        //Check local cache before triggering RPC
        LocalUser* localUser = getLocalUserById(blazeId);
        if (localUser != nullptr)
        {
            // NOTE: If a LocalUser exists for the BlazeId, then they are considered to be Online
            //       Just schedule the titles callback and return.
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("checkUserOnlineStatusCb", 
                titleCheckOnlineStatusCB, ERR_OK, jobId, true, this, 0, jobId); /*lint !e613 (Have checked the user whether valid) */ // assocObj, delayMs, reservedJobId 
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), jobId, titleCheckOnlineStatusCB);
            return jobId;
        }

        const User *user = getUser(blazeId);
        if (isValidUser(user))
        {
            //user in local cache
            jobId = mBlazeHub->getScheduler()->scheduleFunctor("checkUserOnlineStatusCb", titleCheckOnlineStatusCB,
                ERR_OK, jobId, static_cast<bool>(user->getStatusFlags().getOnline()), this, 0, jobId); /*lint !e613 (Have checked the user whether valid) */ // assocObj, delayMs, reservedJobId 
            Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , jobId, titleCheckOnlineStatusCB);
            return jobId;
        }

        //need to hit server for user
        UserIdentification lookupUserRequest(MEM_GROUP_FRAMEWORK_TEMP);
        lookupUserRequest.setBlazeId(blazeId);

        Blaze::UserSessionsComponent* userSessions = mBlazeHub->getComponentManager()->getUserSessionsComponent();
        JobId rpcJobId = userSessions->lookupUser(lookupUserRequest,
            MakeFunctor(this, &UserManager::internalCheckUserOnlineStatusCb), titleCheckOnlineStatusCB, jobId);
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler(), rpcJobId, titleCheckOnlineStatusCB);
        return rpcJobId;
    }

    void UserManager::addListener( ExtendedUserDataInfoChangeListener *listener )
    {
        mUserEventDispatcher.addDispatchee( listener );
    }

    void UserManager::removeListener( ExtendedUserDataInfoChangeListener *listener )
    {
        mUserEventDispatcher.removeDispatchee( listener);
    }

    void UserManager::addPrimaryUserListener(PrimaryLocalUserListener *listener)
    {
        mPrimaryLocalUserDispatcher.addDispatchee(listener);
    }

    void UserManager::removePrimaryUserListener(PrimaryLocalUserListener *listener)
    {
        mPrimaryLocalUserDispatcher.removeDispatchee(listener);
    }

    const UserManagerCensusData* UserManager::getCensusData() const 
    {
        CensusData::NotifyServerCensusDataItem* notifyServerCensusDataItem = mBlazeHub->getCensusDataAPI()->getCensusData(UserManagerCensusData::TDF_ID);
        if (notifyServerCensusDataItem)
        {
            return static_cast<const UserManagerCensusData*>(notifyServerCensusDataItem->getTdf());
        }
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief get qos ping site latency for the user

        \param[in] blazeId - the user id whose qos ping site latency map is going to be retrieved
        \param[out] qosPingSiteLatency - ping site latency keyed by alias
        \return true if the map is filled up or else return false
    ***************************************************************************************************/
    bool UserManager::getQosPingSitesLatency(const BlazeId blazeId, PingSiteLatencyByAliasMap& qosPingSiteLatency) const
    {
        // make sure the map is empty
        qosPingSiteLatency.clear();
        const User* user = getUserById(blazeId);
        if (user != nullptr)
        {
            const PingSiteLatencyByAliasMap* latencyMap = mBlazeHub->getConnectionManager()->getQosPingSitesLatency();
            if(latencyMap != nullptr)
            {
                const size_t latencyMapSize = latencyMap->size();
                const UserSessionExtendedData* extendData = user->getExtendedData();
                // make sure the extended data is ready or exists
                if (extendData == nullptr)
                {
                    BLAZE_SDK_DEBUGF("[UserManager] User(%" PRId64 ")'s extended data is not ready or has no extended data\n", blazeId);
                    return false;
                }

                const size_t userLatencyMapSize = extendData->getLatencyMap().size();
                if (latencyMapSize == userLatencyMapSize)
                {
                    extendData->getLatencyMap().copyInto(qosPingSiteLatency);
                }
                else
                {
                    BLAZE_SDK_DEBUGF(
                        "[UserManager] Connection manager latency map size(%" PRIsize ") does not"
                        " match user UserExtendedData latency map size(%" PRIsize ")\n",
                        latencyMapSize, userLatencyMapSize
                    );
                }
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UserManager] Connection manager latency map is nullptr\n");
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[UserManager] User(%" PRId64 ") not found\n", blazeId);
        }
        return !qosPingSiteLatency.empty();
    }

    LocalUser *UserManager::getLocalUserById(BlazeId blazeId) const
    { 
       for (uint32_t index = 0; index < mLocalUserVector.size(); ++index)
       {
           if ((mLocalUserVector[index] != nullptr) && (mLocalUserVector[index]->getId() == blazeId))
           {
               return mLocalUserVector[index];
           }
       }

       return nullptr;
    }

    void UserManager::setPrimaryLocalUser(uint32_t userIndex)
    { 
        if ((userIndex < mLocalUserVector.size()) && (mLocalUserVector[userIndex] != nullptr) 
            && (mLocalUserVector[userIndex]->getUser() != nullptr))
        {
            if (!mLocalUserVector[userIndex]->getUser()->isGuestUser())
            {
                mPrimaryLocalUserIndex = userIndex;

                BLAZE_SDK_DEBUGF("[UserManager] Primary local user is now at index %" PRIu32 ", blaze id(%" PRIu64 ").\n",
                    userIndex, mLocalUserVector[userIndex]->getId());

                mPrimaryLocalUserDispatcher.dispatch<uint32_t>("onPrimaryLocalUserChanged",
                    &PrimaryLocalUserListener::onPrimaryLocalUserChanged,
                    mPrimaryLocalUserIndex);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UserManager] Failed to set primary local user to index (%" PRIu32 ") , blaze id(%" PRIu64 "), because user is a guest user.\n",
                    userIndex, mLocalUserVector[userIndex]->getId());
            }
        }
        else if (userIndex >= mLocalUserVector.size())
        {
            BLAZE_SDK_DEBUGF("[UserManager] Failed to set primary local user because specified index (%" PRIu32 ") is invalid. mLocalUserVector.size() %" PRIsize ".\n",
                userIndex, mLocalUserVector.size());
        }
        else if (mLocalUserVector[userIndex] == nullptr)
        {
            BLAZE_SDK_DEBUGF("[UserManager] Failed to set primary local user at specified index (%" PRIu32 ") because local user object is null. \n",
                userIndex);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[UserManager] Failed to set primary local user at specified index (%" PRIu32 ") because user object part of local user object is null. \n",
                userIndex);
        }
    }

    bool UserManager::chooseNewPrimaryLocalUser(uint32_t previousPrimaryIndex)
    {
       for (uint32_t index = 0; index < mLocalUserVector.size(); ++index)
       {
           if ((index == previousPrimaryIndex) || (mLocalUserVector[index] == nullptr))
           {
               continue;
           }

           setPrimaryLocalUser(index);

           return true;
       }

       return false;
    }

    bool UserManager::hasLocalUsers() const
    {
        bool hasLocalUsers = false;
        
        for (uint32_t index = 0; index < mLocalUserVector.size(); ++index)
        {
            if ((mLocalUserVector[index] != nullptr))
            {
                hasLocalUsers = true;
                break;
            }
        }

        return hasLocalUsers;
    }
    /*! ************************************************************************************************/
    /*! \brief persona hash operator for IdByNameMap

        \param[in] name - the input name to generate a hash code
        \return uint32_t - the generated hash code
    ***************************************************************************************************/
    uint32_t UserManager::PersonaHash::operator()(const char8_t* name) const
    {
        return DirtyUsernameHash(name);
    }

    /*! ************************************************************************************************/
    /*! \brief persona equalTo operator for IdByNameMap

        \param[in] name - input account name 1
        \param[in] name - input account name 2
        \return bool - true if the two names are equal, false otherwise
    ***************************************************************************************************/
    bool UserManager::PersonaEqualTo::operator()(const char8_t* name1, const char8_t* name2) const
    {
        return DirtyUsernameCompare(name1, name2) == 0;
    }

} // namespace UserManager
} // namespace Blaze


//lint +e1060  Lint can't seem to figure out that UserManager and User are friends.

