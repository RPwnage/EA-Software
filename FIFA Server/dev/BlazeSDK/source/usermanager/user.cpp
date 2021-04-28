/*************************************************************************************************/
/*!
    \file user.cpp


    \attention
    (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/blazehub.h"
#include "DirtySDK/dirtysock/netconn.h" //For NetConnStatus()
#include "EAStdC/EAString.h"

namespace Blaze
{
namespace UserManager
{
    User::User(
        BlazeId blazeId,
        const PlatformInfo& platformInfo,
        bool isPrimaryPersona,
        const char8_t *personaNamespace,
        const char8_t *userName,
        Locale accountLocale,
        uint32_t accountCountry,
        const UserDataFlags *statusFlags,
        BlazeHub& blazeHub
    ) : mRefCount(0),
        mSubscribedBy(0),
        mId(blazeId),
        mIsPrimaryPersona(isPrimaryPersona),
        mAccountLocale(accountLocale),
        mAccountCountry(accountCountry),
        mTimestampMs(0),
        mExtendedData(nullptr),
        mBlazeHub(blazeHub)
    {
        platformInfo.copyInto(mPlatformInfo);
        setPersonaNamespace(personaNamespace);
        setName(userName);
        setPersonaId(blazeId);
        mStatusFlags.setBits(statusFlags ? statusFlags->getBits() : 0);
    }

    User::~User()
    {
        clearExtendedData();
    }

    void User::copyIntoUserIdentification(UserIdentification& outUserIdent) const
    {
        outUserIdent.setBlazeId(mId);
        outUserIdent.setPersonaNamespace(mPersonaNamespace);
        outUserIdent.setName(mName);
        outUserIdent.setAccountLocale(mAccountLocale);
        outUserIdent.setAccountCountry(mAccountCountry);
        mPlatformInfo.copyInto(outUserIdent.getPlatformInfo());
    }

    ExternalId User::getExternalId() const 
    {
        return mBlazeHub.getExternalIdFromPlatformInfo(mPlatformInfo);
    }

    void User::clearExtendedData()
    {
        mExtendedData = nullptr;
    }

    /*! **********************************************************************************************************/
    /*! \brief Sets the users extended session data

    \param[in] data - a pointer to the data
    **************************************************************************************************************/
    void User::setExtendedData(const UserSessionExtendedData* data)
    {
        if(mExtendedData == nullptr)
        {
            mExtendedData = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "ExtendedData") UserSessionExtendedData(MEM_GROUP_FRAMEWORK);
        }
        // NOTE: There is no need to first call clearExtendedData() because copyInto() will 
        // clear out any data already contained in vectors properly, before copying new data in.
        data->copyInto(*mExtendedData);
    }

    /*! ************************************************************************************************/
    /*!
        \brief Refresh the user data using information obtained from the Blaze server{}.
        Also set the last time this user object was updated. 
        The timestamp is 0 when the User is created.
        \note This method will only be called for CACHED User objects, because OWNED user objects
        should continuously receive notifications regarding their state and status.
        \param[in] userData - User data consisting of UserIdentification/UserExtendedData/UserDataFlags(online status)
    ***************************************************************************************************/
    void User::refreshTransientData(const UserData& userData)
    {
        blaze_strnzcpy(mName, userData.getUserInfo().getName(), sizeof(mName));
        setExtendedData(&userData.getExtendedData());
        setStatusFlags(userData.getStatusFlags());
    }

    /*! **********************************************************************************************************/
    /*! \brief Sets the status flags for the user
        \param[in] userDataFlags - set the status flags provided in a UserUpdated message
    **************************************************************************************************************/
    void User::setStatusFlags(const UserDataFlags &statusFlags)
    {
        mStatusFlags.setBits(statusFlags.getBits());
    }

#if !defined(EA_PLATFORM_PS4)
    /*! **********************************************************************************************************/
    /*! \brief Reports if the user has complete data. Does not return false for PC when externalId == 0.
    \return Returns true if the user object is complete
    **************************************************************************************************************/
    bool User::isComplete() const
    {
        /* assume PC for now (no need to check mExternalId) */
        return mName[0] != '\0' && mAccountLocale != 0 && mAccountCountry != 0;
    }
#endif
#if defined(EA_PLATFORM_PS4)
    // Audit: only use case I can think of for this is Association Lists that have users who have never logged in to title.
    // Question would be why is a similar external Id check not required on X1?
    bool User::isComplete() const
    {
        return mName[0] != '\0' && mAccountLocale != 0 && (mPlatformInfo.getClientPlatform() != ps4 || mPlatformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID);
    }
#endif

    void User::setPersonaNamespace(const char *personaNamespace)
    {
        if (personaNamespace != nullptr)
        {
            blaze_strnzcpy( mPersonaNamespace, personaNamespace, sizeof( mPersonaNamespace ) );
        }
        else
        {
            mPersonaNamespace[0] = '\0';
        }
    }

    void User::setName(const char *name)
    {
        if (name != nullptr)
        {
            blaze_strnzcpy( mName, name, sizeof( mName ) );
        }
        else
        {
            mName[0] = '\0';
        }
    }

    void User::setPersonaId(BlazeId personaId)
    {
        if(personaId != INVALID_BLAZE_ID)
            blaze_snzprintf(mPersonaId, sizeof(mPersonaId), "%" PRId64, personaId);
        else
            mPersonaId[0] = '\0';
    }

    bool User::getDataValue(uint16_t componentId, uint16_t key, UserExtendedDataValue &value) const
    {
        bool isSuccessful = false;
        if (mExtendedData != nullptr)
        {
            uint32_t dataKey = (static_cast<uint32_t>(componentId) << 16) | key;
            UserExtendedDataMap& map = mExtendedData->getDataMap();
            UserExtendedDataMap::iterator itr = map.find(dataKey);
            if (itr != map.end())
            {
                value = itr->second;
                isSuccessful = true;
            }
        }
        return isSuccessful;
    }
    
    void User::setSubscriberIndex( uint32_t userIndex )
    {
        if(userIndex < sizeof(mSubscribedBy)*8)
            mSubscribedBy |= (1 << userIndex);
#if ENABLE_BLAZE_SDK_LOGGING
        else
            BLAZE_SDK_DEBUGF("Invalid userIndex %u\n", userIndex);
#endif
    }

    void User::clearSubscriberIndex( uint32_t userIndex )
    {
        if(userIndex < sizeof(mSubscribedBy)*8)
            mSubscribedBy &= ~(1 << userIndex);
#if ENABLE_BLAZE_SDK_LOGGING
        else
            BLAZE_SDK_DEBUGF("Invalid userIndex %u\n", userIndex);
#endif
    }

    bool User::isSubscribedByIndex( uint32_t userIndex ) const
    {
        if(userIndex < sizeof(mSubscribedBy)*8)
            return (mSubscribedBy & (1 << userIndex)) != 0;
        BLAZE_SDK_DEBUGF("Invalid userIndex %u\n", userIndex);
        return false;
    }

    void User::setSubscribed(bool subscribed)
    {
        if (subscribed)
            mStatusFlags.setSubscribed();
        else
            mStatusFlags.clearSubscribed();
    }

    // return true if the supplied blazObjectId is in the user's extendedUserData's blazeObjectId list
    bool User::hasExtendedDataBlazeObjectId(const EA::TDF::ObjectId& blazeObjectId) const
    {
        if (mExtendedData != nullptr)
        {
            const ObjectIdList &blazeObjIdList = mExtendedData->getBlazeObjectIdList();
            ObjectIdList::const_iterator iter = blazeObjIdList.begin();
            ObjectIdList::const_iterator end = blazeObjIdList.end();
            for ( ; iter != end; ++iter )
            {
                if (*iter == blazeObjectId)
                {
                    return true;
                }
            }
        }

        return false;
    }

    User::OnlineStatus User::getOnlineStatus() const
    {
        if (mStatusFlags.getSubscribed())
        {
            return (mStatusFlags.getOnline() ? ONLINE : OFFLINE);
        }

        return UNKNOWN;
    }

    User::OnlineStatus User::getCachedOnlineStatus() const
    {
        return (mStatusFlags.getOnline() ? ONLINE : OFFLINE);
    }    

    // class LocalUser implementation
    LocalUser::LocalUser(User& user, uint32_t userIndex, UserSessionType sessionType, BlazeHub& blazeHub, MemoryGroupId memGroupId)
        : mUser(user),
          mUserIndex(userIndex),
          mSessionType(sessionType),
          mBlazeHub(blazeHub),
          mPostAuthDataReceived(false),
          mCanTransitionToAuthenticated(true),
          mDeauthenticating(false),
          mHardwareFlags(memGroupId),
          mUserSessionLoginInfo(memGroupId),
          mUserSessionId(INVALID_USER_SESSION_ID), // UserSession::INVALID_SESSION_ID
          mTelemetryServer(memGroupId),
          mTickerServer(memGroupId),
          mUserOptions(memGroupId)
    {
    }

    LocalUser::~LocalUser()
    {
        mBlazeHub.getScheduler()->removeByAssociatedObject(this);
        mBlazeHub.getUserManager()->removeListener(this);
    }

    void LocalUser::updateHardwareFlags() const
    {
        Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager(mUserIndex)->getUserSessionsComponent();
        if (userSessions)
        {
            userSessions->updateHardwareFlags(mHardwareFlags);
        }
    }

    /*! ****************************************************************************/
    /*! \brief Sets an attribute in the user session extended data for the given user
        \param[in] pBlazeHub                The BlazeHub
        \param[in] key                        The key for the attribute
        \param[in] value                    The attribute's value
        \param[in] cb                        The functor dispatched upon success/failure
        \return                                The JobId of the RPC call
    ********************************************************************************/
    JobId LocalUser::setUserSessionAttribute(uint16_t key, int64_t value, const UserSessionsComponent::UpdateExtendedDataAttributeCb &cb) const
    {
        JobId jobId = INVALID_JOB_ID;
        Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager(mUserIndex)->getUserSessionsComponent();
        if (userSessions != nullptr)
        {
            UpdateExtendedDataAttributeRequest request;
            request.setRemove(false);
            request.setKey(key);
            request.setComponent(0);    // INVALID_COMPONENT_ID
            request.setValue(value);
            jobId = userSessions->updateExtendedDataAttribute(request, cb);
        }
        return jobId;
    }

    /*! ****************************************************************************/
    /*! \brief removes an attribute from the user session extended data for the given user
        \param[in] pBlazeHub                The BlazeHub
        \param[in] key                        The key for the attribute
        \param[in] cb                        The functor dispatched upon success/failure
        \return                                The JobId of the RPC call
    ********************************************************************************/
    JobId LocalUser::removeUserSessionAttribute(uint16_t key, const UserSessionsComponent::UpdateExtendedDataAttributeCb &cb) const
    {
        JobId jobId = INVALID_JOB_ID;
        Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager(mUserIndex)->getUserSessionsComponent();
        if (userSessions != nullptr)
        {
            UpdateExtendedDataAttributeRequest request;
            request.setRemove(true);
            request.setKey(key);
            request.setComponent(0);    // INVALID_COMPONENT_ID
            jobId = userSessions->updateExtendedDataAttribute(request, cb);
        }
        return jobId;
    }

    JobId LocalUser::setCrossPlatformOptIn(bool optIn, const UserSessionsComponent::SetUserCrossPlatformOptInCb &cb) const
    {
        JobId jobId = INVALID_JOB_ID;
        Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager(mUserIndex)->getUserSessionsComponent();
        if (userSessions != nullptr)
        {
            OptInRequest request;
            request.setBlazeId(mUser.getId());
            request.setOptIn(optIn);
            jobId = userSessions->setUserCrossPlatformOptIn(request, cb);
        }
        return jobId;
    }

    bool User::getCrossPlatformOptIn() const 
    {
        return mExtendedData->getCrossPlatformOptIn();
    }

    void LocalUser::finishAuthentication(const UserSessionLoginInfo &info)
    {
        info.copyInto(mUserSessionLoginInfo);
        if (mUserSessionLoginInfo.getSessionKeyAsTdfString().length() > 16)
        {
            mUserSessionId = EA::StdC::StrtoI64(mUserSessionLoginInfo.getSessionKey(), nullptr, 16);
        }
        mBlazeHub.getUserManager()->addListener(this);

        Blaze::Util::PostAuthRequest req;
        req.setUniqueDeviceId(mBlazeHub.getConnectionManager()->getUniqueDeviceId());
        req.setDirtySockUserIndex(mBlazeHub.getLoginManager(mUserIndex)->getDirtySockUserIndex());
        mBlazeHub.getComponentManager(mUserIndex)->getUtilComponent()->postAuth(req,
            Util::UtilComponent::PostAuthCb(this, &LocalUser::onPostAuth));
    }

    void LocalUser::onPostAuth(const Util::PostAuthResponse* response, BlazeError error, JobId id)
    {
        if (error != Blaze::ERR_OK)
        {
            BLAZE_SDK_DEBUGF("LocalUser: Error getting ticker and telemetry data from server: %s (%d);\n", mBlazeHub.getErrorName(error), error);
            //getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, error);
            return;
        }

        setTelemetryServerResponse(&response->getTelemetryServer());
        setTickerServerResponse(&response->getTickerServer());
        setUserOptions(&response->getUserOptions());

        mPostAuthDataReceived = true;
        if (mUser.hasExtendedData() && mCanTransitionToAuthenticated)
        {
            mBlazeHub.getUserManager()->removeListener(this);
            mBlazeHub.getUserManager()->onLocalUserAuthenticated(mUserIndex);

            // ok, now let's refresh user data cached on server
            mBlazeHub.getConnectionManager()->triggerLatencyServerUpdate(mUserIndex);
        }
    }

    void LocalUser::onExtendedUserDataInfoChanged(Blaze::BlazeId blazeId)
    {
        mBlazeHub.getUserManager()->removeListener(this);

        if (mPostAuthDataReceived && mCanTransitionToAuthenticated)
        {
             mBlazeHub.getUserManager()->onLocalUserAuthenticated(mUserIndex);

            // ok, now let's refresh user data cached on server
            mBlazeHub.getConnectionManager()->triggerLatencyServerUpdate(mUserIndex);
        }
    }

    void LocalUser::setCanTransitionToAuthenticated(bool canTransitionToAuthenticated)
    {
        mCanTransitionToAuthenticated = canTransitionToAuthenticated;

        if (mUser.hasExtendedData() && mPostAuthDataReceived && mCanTransitionToAuthenticated)
        {
             mBlazeHub.getUserManager()->onLocalUserAuthenticated(mUserIndex);

            // ok, now let's refresh user data cached on server
            mBlazeHub.getConnectionManager()->triggerLatencyServerUpdate(mUserIndex);
        }
    }

    JobId LocalUser::updateUserOptions(const Blaze::Util::UserOptions &request,  Functor1<Blaze::BlazeError> cb)
    {
        JobId jobId = INVALID_JOB_ID;

        // Call the setUserOptions RPC only if the options changed
        if (mUserOptions.getTelemetryOpt() != request.getTelemetryOpt())
        {
            Util::UtilComponent *component = mBlazeHub.getComponentManager(mUserIndex)->getUtilComponent();
            jobId = component->setUserOptions(request, MakeFunctor(this, &LocalUser::onUpdateUserOptionsCb), request.getTelemetryOpt(), cb);
        }
        else
        {
            JobId reservedJobId = mBlazeHub.getScheduler()->reserveJobId();
            jobId = mBlazeHub.getScheduler()->scheduleFunctor("updateUserOptionsCb", cb, ERR_OK, this, 0, reservedJobId);
        }

        if (cb.isValid())
            Job::addTitleCbAssociatedObject(mBlazeHub.getScheduler(), jobId, cb);

        return jobId;
    }

    void LocalUser::onUpdateUserOptionsCb(BlazeError error, JobId id, Util::TelemetryOpt telOpt, Functor1<Blaze::BlazeError> cb)
    {
        if (error == ERR_OK)
        {
            // update cached option data in login manager
            Util::UserOptions rqs;
            rqs.setTelemetryOpt(telOpt);
            setUserOptions(&rqs);
        }

        if (cb.isValid())
            cb(error);
    }

#ifdef BLAZE_USER_PROFILE_SUPPORT
    UserProfile::UserProfile(const UserApiUserDataT& profile, MemoryGroupId memGroupId)
        : mMemGroup(memGroupId)
    {
        memset(&mProfile, 0, sizeof(mProfile));

        update(profile);

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX)
        DirtyUserToNativeUser(&mFirstPartyLookupId, sizeof(mFirstPartyLookupId), &mProfile.DirtyUser);
#endif
    }

    UserProfile::~UserProfile()
    {
        BLAZE_FREE(mMemGroup, mProfile.Profile.pRawData);
    }

    void UserProfile::update(const UserApiUserDataT& profile)
    {
        if (mProfile.Profile.pRawData)
        {
            BLAZE_FREE(mMemGroup, mProfile.Profile.pRawData);
        }

        memcpy(&mProfile, &profile, sizeof(mProfile));

        // TODO: Evaluate the raw data usage
        // amakoukji: The code assume the raw data is a string, which is not always the case.
        // Also the 3 substructure of UserApiUserDataT have their own raw data pointer that 
        // may be pointer to a different place in the same memory or an entirely diffrent object.
        if (profile.Profile.pRawData != nullptr)
        {
            mProfile.Profile.pRawData = blaze_strdup((char8_t*)profile.Profile.pRawData, mMemGroup);
        }
    }

    UserProfile* UserProfile::Create(const UserApiUserDataT& profile, const char8_t* allocName)
    {
        return BLAZE_NEW(MEM_GROUP_FRAMEWORK_DEFAULT, allocName) UserProfile(profile, MEM_GROUP_FRAMEWORK_DEFAULT);
    }

    void UserProfile::Destroy(UserProfile* userProfile)
    {
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_DEFAULT, userProfile);
    }

#endif

} // namespace UserManager
} // namespace Blaze
