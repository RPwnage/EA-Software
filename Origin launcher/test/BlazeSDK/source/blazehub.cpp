/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"

#include "BlazeSDK/api.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/serviceresolver.h"

#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/bytevaultmanager/bytevaultmanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/component/authenticationcomponent.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/component/usersessionscomponent.h"
#include "BlazeSDK/component/utilcomponent.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/shared/framework/locales.h"

#include "BlazeSDK/blazehub.h"

#ifdef USE_TELEMETRY_SDK
#include "BlazeSDK/util/telemetryapi.h"
#endif

#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock.h" //NetCrit/NetTick
#include "DirtySDK/dirtyvers.h"

#include "EATDF/tdffactory.h"

namespace Blaze
{
    static char8_t sBlazeSdkVersionStringBuf[256] = {0};
    uint64_t getBlazeSdkVersion()        { return BLAZE_SDK_VERSION; }
    uint8_t getBlazeSdkVersionYear()     { return (uint8_t)BLAZE_SDK_VERSION_YEAR; }
    uint8_t getBlazeSdkVersionSeason()   { return (uint8_t)BLAZE_SDK_VERSION_SEASON; }
    uint8_t getBlazeSdkVersionMajor()    { return (uint8_t)BLAZE_SDK_VERSION_MAJOR; }
    uint8_t getBlazeSdkVersionMinor()    { return (uint8_t)BLAZE_SDK_VERSION_MINOR; }
    uint8_t getBlazeSdkVersionPatchRel() { return (uint8_t)BLAZE_SDK_VERSION_PATCH; }
    const char8_t* getBlazeSdkVersionModifiers(){ return BLAZE_SDK_VERSION_MODIFIERS; }

    uint32_t BlazeHub::msNumInitializedHubs = 0;

    const char8_t *getBlazeSdkVersionString()
    {
        if (sBlazeSdkVersionStringBuf[0] == '\0')
        {
            blaze_snzprintf(sBlazeSdkVersionStringBuf, sizeof(sBlazeSdkVersionStringBuf),
                (BLAZE_SDK_BRANCH[0] == '\0' ? "%u.%u.%u.%u.%u%s%s" : "%u.%u.%u.%u.%u%s (%s)"), // The tail %s is included in the first string to avoid any compiler warnings
                getBlazeSdkVersionYear(),
                getBlazeSdkVersionSeason(),
                getBlazeSdkVersionMajor(),
                getBlazeSdkVersionMinor(),
                getBlazeSdkVersionPatchRel(),
                getBlazeSdkVersionModifiers(),
                BLAZE_SDK_BRANCH);
        }
        return sBlazeSdkVersionStringBuf;
    }

BlazeHub::InitParameters::InitParameters()
:   UserCount(1),
    MaxCachedUserCount(0),
    MaxCachedUserLookups(100),
    MaxCachedUserProfileCount(0),
    OutgoingBufferSize(BlazeSender::OUTGOING_BUFFER_SIZE_DEFAULT), // 161Kb
    OverflowOutgoingBufferSize(BlazeSender::OVERFLOW_BUFFER_SIZE_MIN), // 512Kb
    DefaultIncomingPacketSize(BlazeSender::BUFFER_SIZE_DEFAULT), // 32Kb
    MaxIncomingPacketSize(BlazeSender::OVERFLOW_BUFFER_SIZE_MIN), // 512Kb
    Locale(LOCALE_DEFAULT),
    Country((uint32_t)COUNTRY_DEFAULT),
    Client(CLIENT_TYPE_GAMEPLAY_USER),
    IgnoreInactivityTimeout(false),
    Secure(true),
    Environment(ENVIRONMENT_PROD),
    GamePort(BlazeHub::DEFAULT_GAME_PORT),
    UsePlainTextTOS(false),
    EnableQos(true),
    BlazeConnectionTimeout(Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS),
    DefaultRequestTimeout(10000),
    ExtendedConnectionErrors(false),
    EnableNetConnIdle(true)
#ifdef BLAZE_USER_PROFILE_SUPPORT
    , IncludeProfileRawData(true)
#endif
#if !defined(BLAZESDK_TRIAL_SUPPORT)
    , isTrial(false)
#endif
{
    ClientName[0] = '\0';
    ClientVersion[0] = '\0';
    ClientSkuId[0] = '\0';
    ServiceName[0] = '\0';
    LocalInterfaceAddress[0] = '\0';
    AdditionalNetConnParams[0] = '\0';
    Override.RedirectorAddress[0] = '\0';
    Override.RedirectorSecure = true;
    Override.RedirectorPort = 0;
    Override.DisplayUri[0] = '\0';
}

void BlazeHub::InitParameters::logParameters() const
{
    BLAZE_SDK_DEBUGF("BlazeHub Parameters: \n");
    BLAZE_SDK_DEBUGF("                    ClientType: \"%s\"\n", ClientTypeToString(this->Client));
    BLAZE_SDK_DEBUGF("       IgnoreInactivityTimeout: \"%s\"\n", this->IgnoreInactivityTimeout?"true":"false");
    BLAZE_SDK_DEBUGF("                    ClientName: \"%s\"\n", this->ClientName);
    BLAZE_SDK_DEBUGF("                   ClientSkuId: \"%s\"\n", this->ClientSkuId);
    BLAZE_SDK_DEBUGF("                 ClientVersion: \"%s\"\n", this->ClientVersion);
    BLAZE_SDK_DEBUGF("                   Environment: %s\n", EnvironmentTypeToString(this->Environment));
    BLAZE_SDK_DEBUGF("                   ServiceName: \"%s\"\n", this->ServiceName);
#if !defined(BLAZESDK_TRIAL_SUPPORT)
    BLAZE_SDK_DEBUGF("                       isTrial: %s\n", this->isTrial?"true":"false");
#endif
    BLAZE_SDK_DEBUGF("                        Secure: %s\n", this->Secure?"true":"false");
    BLAZE_SDK_DEBUGF("         LocalInterfaceAddress: \"%s\"\n", this->LocalInterfaceAddress);
    BLAZE_SDK_DEBUGF("       AdditionalNetConnParams: \"%s\"\n", this->AdditionalNetConnParams);
    BLAZE_SDK_DEBUGF("                     UserCount: %" PRIu32 "\n", this->UserCount);
    BLAZE_SDK_DEBUGF("            MaxCachedUserCount: %" PRIu32 "\n", this->MaxCachedUserCount);
    BLAZE_SDK_DEBUGF("          MaxCachedUserLookups: %" PRIu32 "\n", this->MaxCachedUserLookups);
    BLAZE_SDK_DEBUGF("            OutgoingBufferSize: %" PRIu32 "\n", this->OutgoingBufferSize);
    BLAZE_SDK_DEBUGF("     DefaultIncomingPacketSize: %" PRIu32 "\n", this->DefaultIncomingPacketSize);
    BLAZE_SDK_DEBUGF("         MaxIncomingPacketSize: %" PRIu32 "\n", this->MaxIncomingPacketSize);
    BLAZE_SDK_DEBUGF("                        Locale: %" PRIx32 "(%c%c%c%c)\n", this->Locale, (this->Locale >> 24) & 0xFF, (this->Locale >> 16) & 0xFF, (this->Locale >> 8) & 0xFF, this->Locale & 0xFF);
    BLAZE_SDK_DEBUGF("                Country/Region: %" PRIx32 "(%c%c)\n", this->Country, (this->Country >> 8) & 0xFF, this->Country & 0xFF);
    BLAZE_SDK_DEBUGF("                      GamePort: %" PRIu16 "\n", this->GamePort);
    BLAZE_SDK_DEBUGF("    RedirectorAddress Override: \"%s\"\n", this->Override.RedirectorAddress);
    BLAZE_SDK_DEBUGF("     RedirectorSecure Override: \"%s\"\n", this->Override.RedirectorSecure?"true":"false");
    BLAZE_SDK_DEBUGF("       RedirectorPort Override: %" PRIu16 "\n", this->Override.RedirectorPort);
    BLAZE_SDK_DEBUGF("                     EnableQos: %s\n", this->EnableQos?"true":"false");
    BLAZE_SDK_DEBUGF("        BlazeConnectionTimeout: %" PRIu32 "\n", this->BlazeConnectionTimeout);
    BLAZE_SDK_DEBUGF("        ExtendedConnectionErrors: %s\n", this->ExtendedConnectionErrors?"true":"false");
    BLAZE_SDK_DEBUGF("        EnableNetConnIdle: %s\n", this->EnableNetConnIdle?"true":"false");
};

BlazeError BlazeHub::initialize(BlazeHub **hub, const InitParameters &params, EA::Allocator::ICoreAllocator *allocator, Blaze::Debug::LogFunction hook, void *data, const char8_t* certData, const char8_t* keyData)
{
    BlazeError result = ERR_OK;

    Blaze::Allocator::setAllocator(allocator);
    EA::TDF::TdfFactory::fixupTypes(*allocator, "TdfFactoryAllocs");

    BlazeAssert(allocator != nullptr);
    BlazeAssertMsg(params.UserCount != 0, "Must specify at least one user!");

    if (*hub != nullptr)
    {
        BLAZE_SDK_DEBUGF("[BlazeHub] Warning: Ignoring attempt to reinitialize hub");
        return SDK_ERR_BLAZE_HUB_ALREADY_INITIALIZED;
    }

    if (params.ClientName[0] == '\0')
        return SDK_ERR_NO_CLIENT_NAME_PROVIDED;
    if (params.ClientVersion[0] == '\0')
        return SDK_ERR_NO_CLIENT_VERSION_PROVIDED;
    if (params.ClientSkuId[0] == '\0')
        return SDK_ERR_NO_CLIENT_SKU_ID_PROVIDED;
    if (params.ServiceName[0] == '\0')
        return SDK_ERR_NO_SERVICE_NAME_PROVIDED;

    //Check that included dirtysock version matches compiled binary version
    int32_t binVers = NetConnStatus('vers', 0, nullptr, 0);
    if (binVers != -1 && DIRTYVERS != binVers)
    {
        BLAZE_SDK_DEBUGF("[BlazeHub::Initialize]: Included DS Version (0x%x) does not match binary DS version (0x%x). Aborting init.\n",
            DIRTYVERS, binVers);
        return SDK_ERR_DS_VERSION_MISMATCH;
    }

    if(NetConnStatus('open', 0, nullptr, 0) == 0)
    {
        return SDK_ERR_DIRTYSOCK_UNINITIALIZED;
    }

    Blaze::Debug::setLoggingHook( hook, data);

    BLAZE_SDK_DEBUGF("BlazeHub::initialize - BlazeSDK Version: \"%s\"\n", getBlazeSdkVersionString());
    //log startup params
    params.logParameters();

    // set service name used for on-demand CA certificate installs
    NetConnControl('snam', 0, 0, (void *)&params.ServiceName, nullptr);

    *hub = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "Blaze Hub") BlazeHub(params, MEM_GROUP_FRAMEWORK);

    if (*hub != nullptr)
    {
        result = (*hub)->initializeInternal(certData, keyData);
    }
    else
    {
        result = SDK_ERR_NO_MEM;
    }

    if (result == ERR_OK)
        ++msNumInitializedHubs;

    return result;
}


void BlazeHub::shutdown( BlazeHub *blazeHub )
{
    BLAZE_DELETE_PRIVATE(MEM_GROUP_FRAMEWORK,BlazeHub, blazeHub);

    // If there are other initialized BlazeHubs on this thread,
    // then the log buffer and EATDF type allocations are still needed
    if (--msNumInitializedHubs == 0)
    {
        EA::TDF::TdfFactory::cleanupTypeAllocations();
        Blaze::Debug::clearLogBuffer();
    }
}


bool BlazeHub::setServiceName( const char8_t* serviceName ) 
{
    if ( (mConnectionManager != nullptr) && (mConnectionManager->isConnected() || mConnectionManager->isReconnecting()) )
    {
        BLAZE_SDK_DEBUGF("[BlazeHub] Error: Attempt to change service name while connected.");
        return false;
    }
    if (serviceName != nullptr)
    {
        blaze_strnzcpy(mInitParams.ServiceName, serviceName, sizeof(mInitParams.ServiceName));

        // set service name used for on-demand CA certificate installs
        NetConnControl('snam', 0, 0, (void *)mInitParams.ServiceName, nullptr);

        BLAZE_SDK_DEBUGF("[BlazeHub] Initialization parameter ServiceName set to: \"%s\"\n", mInitParams.ServiceName);

        return true;
    }
    return false;
} 

bool BlazeHub::setEnvironment( EnvironmentType environment )
{
    if ( (mConnectionManager != nullptr) && (mConnectionManager->isConnected() || mConnectionManager->isReconnecting()) )
    {
        BLAZE_SDK_DEBUGF("[BlazeHub] Error: Attempt to change environment while connected.");
        return false;
    }

    mInitParams.Environment = environment;
    BLAZE_SDK_DEBUGF("[BlazeHub] Initialization parameter Environment set to: %s\n", EnvironmentTypeToString(mInitParams.Environment));

    return true;
} 

void BlazeHub::addUserStateEventHandler(BlazeStateEventHandler *handler)
{
    mStateDispatcher.addDispatchee( handler );
}


void BlazeHub::removeUserStateEventHandler(BlazeStateEventHandler *handler)
{
    mStateDispatcher.removeDispatchee( handler );
}


void BlazeHub::addUserStateAPIEventHandler(BlazeStateEventHandler *handler)
{
    mApiStateDispatcher.addDispatchee(handler);
}


void BlazeHub::removeUserStateAPIEventHandler(BlazeStateEventHandler *handler)
{
    mApiStateDispatcher.removeDispatchee( handler );
}


void BlazeHub::addUserStateFrameworkEventHandler(BlazeStateEventHandler *handler)
{
    mFrameworkStateDispatcher.addDispatchee(handler);
}


void BlazeHub::removeUserStateFrameworkEventHandler(BlazeStateEventHandler *handler)
{
    mFrameworkStateDispatcher.removeDispatchee( handler );
}


void BlazeHub::idle()
{
    BLAZE_SDK_SCOPE_TIMER("BlazeHub_idle");

    if (mIsIdling)
    {
        BlazeAssertMsg(false, "Error: attempting to call BlazeHub::idle() from within a blaze callback!" );
        return;
    }

    // Enter critical section
    lock();
    {
        BLAZE_SDK_SCOPE_TIMER("BlazeIdle_critSection");

        mIsIdling = true;

        uint32_t uCurrMillis = getCurrentTime();
        int32_t iTickDiff = NetTickDiff(uCurrMillis, mLastCurrMillis);

        //Save off Last time
        mLastCurrMillis = uCurrMillis;

        if (iTickDiff >= 0)
        {
            uint32_t uElapsedMillis = iTickDiff;

            if (uElapsedMillis > mIdleStarveMax && mLastCurrMillis > 0)
            {
                BLAZE_SDK_DEBUGF("BlazeHub::Idle >> Idle starved - %d ms.\n", uElapsedMillis);
            }

            if (mIdleLogInterval != 0 && (mLastCurrMillis - mLastIdleLogMillis > mIdleLogInterval) )
            {
                BLAZE_SDK_DEBUGF("BlazeHub::Idle called. Will check again after - %d ms. \n", mIdleLogInterval);
                mLastIdleLogMillis = mLastCurrMillis;
            }

            // Idle idlers
            mIdlerDispatcher.dispatch<const uint32_t, const uint32_t>("idleWrapper", &Idler::idleWrapper, uCurrMillis, uElapsedMillis);
        }

        uint32_t timePassedMs = NetTickDiff(getCurrentTime(), uCurrMillis);
        if (timePassedMs > mIdleProcessingMax && uCurrMillis > 0)
        {
            BLAZE_SDK_DEBUGF("BlazeHub::Idle >> Excessive idle Processing time - %d ms.\n", timePassedMs);
        }

        mIsIdling = false;
    }
    // Leave critical section
    unlock();
}

const char8_t* BlazeHub::getErrorName(BlazeError errorCode, uint32_t userIndex) const
{
    return getComponentManager(userIndex)->getErrorName(errorCode);
}

uint32_t BlazeHub::getCurrentTime() const
{
    return NetTick();
}

uint32_t BlazeHub::getServerTime() const
{
    return mConnectionManager->getServerTimeUtc();
}

void BlazeHub::lock()
{
    NetCritEnter(mCritSec);
}

bool BlazeHub::tryLock()
{
    return (NetCritTry(mCritSec) != 0);
}

void BlazeHub::unlock()
{
    NetCritLeave(mCritSec);
}

LoginManager::LoginManager *BlazeHub::getLoginManager(uint32_t userIndex) const
{
    // Note: login mgr is created by initialize (automatically 'registered')
    //  So mLoginManagers can't be nullptr.
    if( userIndex < mNumUsers )
    {
        return mLoginManagers[userIndex];
    }

    BlazeAssertMsg(false,  "userIndex out of bounds");
    return nullptr;
}


ComponentManager *BlazeHub::getComponentManager(uint32_t userIndex) const
{
    return mConnectionManager->getComponentManager(userIndex);
}

bool BlazeHub::isServerComponentAvailable(uint16_t componentId) const
{
    Util::ComponentIdList::const_iterator itr = mConnectionManager->getComponentIds().begin();
    Util::ComponentIdList::const_iterator end = mConnectionManager->getComponentIds().end();
    for (; itr != end; ++itr)
    {
        if (*itr == componentId)
            return true;
    }

    return false;
}


ComponentManager *BlazeHub::getComponentManager() const
{
    return (mUserManager == nullptr ? nullptr : mConnectionManager->getComponentManager(mUserManager->getPrimaryLocalUserIndex()));
}

bool BlazeHub::addUserGroupProvider(const EA::TDF::ObjectType& bobjType, UserGroupProvider *provider)
{
    if(provider != nullptr)
    {
        if(mUserGroupProviderByTypeMap.find(bobjType) != mUserGroupProviderByTypeMap.end())
        {
            return false;
        }
        // add provider to UserGroupProviders registered with Hub
        mUserGroupProviderByTypeMap[bobjType] = provider;
        return true;
    }
    return false;
}



bool BlazeHub::removeUserGroupProvider(const EA::TDF::ObjectType& bobjType, UserGroupProvider *provider)
{
    if(provider != nullptr)
    {
        UserGroupProviderByTypeMap::iterator it = mUserGroupProviderByTypeMap.find(bobjType);
        if(it == mUserGroupProviderByTypeMap.end())
        {
            return false;
        }
        // remove provider from UserGroupProviders registered with Hub
        mUserGroupProviderByTypeMap.erase(it);
        return true;
    }
    return false;
}


bool BlazeHub::addUserGroupProvider(const ComponentId id, UserGroupProvider *provider)
{
    if(provider != nullptr)
    {
        if(mUserGroupProviderByComponentMap.find(id) != mUserGroupProviderByComponentMap.end())
        {
            return false;
        }
        // add provider to UserGroupProviders registered with Hub
        mUserGroupProviderByComponentMap[id] = provider;
        return true;
    }
    return false;
}



bool BlazeHub::removeUserGroupProvider(const ComponentId id, UserGroupProvider *provider)
{
    if(provider != nullptr)
    {
        UserGroupProviderByComponentMap::iterator it = mUserGroupProviderByComponentMap.find(id);
        if(it == mUserGroupProviderByComponentMap.end())
        {
            return false;
        }
        // remove provider from UserGroupProviders registered with Hub
        mUserGroupProviderByComponentMap.erase(it);
        return true;
    }
    return false;
}

UserGroup* BlazeHub::getUserGroupById(const EA::TDF::ObjectId& bobjId) const
{
    //First see if there's a specific type provider
    EA::TDF::ObjectType type = bobjId.type;
    UserGroupProviderByTypeMap::const_iterator typeItr = mUserGroupProviderByTypeMap.find(type);
    if(typeItr != mUserGroupProviderByTypeMap.end())
    {
        return typeItr->second->getUserGroupById(bobjId);
    }

    //now look for one by component.
    UserGroupProviderByComponentMap::const_iterator it = mUserGroupProviderByComponentMap.find(bobjId.type.component);
    if(it != mUserGroupProviderByComponentMap.end())
    {
        return it->second->getUserGroupById(bobjId);
    }
    return nullptr;
}

ClientPlatformType BlazeHub::getClientPlatformType() const
{
    return mConnectionManager->getClientPlatformType();
}

ExternalId BlazeHub::getExternalIdFromPlatformInfo(const PlatformInfo& platformInfo) const
{
    ClientPlatformType targetClientPlatform = getClientPlatformType(); // get the external id that matches the local client's platform.
    if (targetClientPlatform == INVALID)
    {
        // no local platform set, use the current platform of the passed-in platform info
        targetClientPlatform = platformInfo.getClientPlatform();
    }

    switch (targetClientPlatform)
    {
    case xone:
    case xbsx:
        return (ExternalId)platformInfo.getExternalIds().getXblAccountId();
    case ps4:
    case ps5:
        return (ExternalId)platformInfo.getExternalIds().getPsnAccountId();
    case steam:
        return (ExternalId)platformInfo.getExternalIds().getSteamAccountId();
    case stadia:
        return (ExternalId)platformInfo.getExternalIds().getStadiaAccountId();
    case pc:
        // Nucleus returns the account id as the ExternalRefValue from login.  (This was not documented by Blaze)
        // It's possible that some code relies on this functionality, so we maintain it here until crossplay is fully implemented & this function is removed.
        return (ExternalId)platformInfo.getEaIds().getNucleusAccountId();
    default:
        break;
    }
    return INVALID_EXTERNAL_ID;
}

void BlazeHub::setExternalStringOrIdIntoPlatformInfo(PlatformInfo& platformInfo, ExternalId extId, const char8_t* extString, ClientPlatformType platform) const
{
    if (platform == INVALID)
        platform = getClientPlatformType();

    // Set the client platform in case the receiving function wants the platform to be set. 
    platformInfo.setClientPlatform(platform);
    switch (platform)
    {
    case xone:
    case xbsx:
        platformInfo.getExternalIds().setXblAccountId((ExternalXblAccountId)extId);
        return;
    case ps4:
    case ps5:
        platformInfo.getExternalIds().setPsnAccountId((ExternalPsnAccountId)extId);
        return;
    case steam:
        platformInfo.getExternalIds().setSteamAccountId((ExternalSteamAccountId)extId);
        return;
    case stadia:
        platformInfo.getExternalIds().setStadiaAccountId((ExternalStadiaAccountId)extId);
        return;
    case pc:
        // External Id on PC *was* just the nucleus account id.  
        platformInfo.getEaIds().setNucleusAccountId((AccountId)extId);
        return;
    case nx:
        platformInfo.getExternalIds().setSwitchId(extString);
        return;
    default:
        break;
    }
}

const char8_t* BlazeHub::platformInfoToString(const PlatformInfo& platformInfo, eastl::string& str)
{
    str = "OriginPersonaName='";
    str.append_sprintf("%s', OriginPersonaId=%" PRIu64 ", NucleusAccountId=%" PRId64 ", PsnAccountId=%" PRIu64 ", XblAccountId=%" PRIu64 ", SteamAccountId=%" PRIu64 ", StadiaAccountId=%" PRIu64 ",SwitchId=%s",
        platformInfo.getEaIds().getOriginPersonaName(), platformInfo.getEaIds().getOriginPersonaId(), platformInfo.getEaIds().getNucleusAccountId(),
        platformInfo.getExternalIds().getPsnAccountId(), platformInfo.getExternalIds().getXblAccountId(), platformInfo.getExternalIds().getSteamAccountId(), platformInfo.getExternalIds().getStadiaAccountId(),
        (platformInfo.getExternalIds().getSwitchIdAsTdfString().empty() ? "nullptr" : platformInfo.getExternalIds().getSwitchId()));
    return str.c_str();
}

bool BlazeHub::hasExternalIdentifier(const PlatformInfo& platformInfo)
{
    switch (platformInfo.getClientPlatform())
    {
    case xone:
    case xbsx:
        return platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID;
    case ps4:
    case ps5:
        return platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID;
    case steam:
        return platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID;
    case stadia:
        return platformInfo.getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID;
    case pc:
        return platformInfo.getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID;
    case nx:
        return !platformInfo.getExternalIds().getSwitchIdAsTdfString().empty();
    default:
        return false;
    }
}

bool BlazeHub::areUsersInActiveSession() const
{
    return mGameManagerAPI ? mGameManagerAPI->areUsersInActiveSession() : false;
}

void BlazeHub::onLocalUserAuthenticated(uint32_t userIndex)
{
    mFrameworkStateDispatcher.dispatch("onAuthenticated", &BlazeStateEventHandler::onAuthenticated, userIndex);
    mApiStateDispatcher.dispatch("onAuthenticated", &BlazeStateEventHandler::onAuthenticated, userIndex);
    mStateDispatcher.dispatch("onAuthenticated", &BlazeStateEventHandler::onAuthenticated, userIndex);
}

void BlazeHub::onLocalUserDeAuthenticated(uint32_t userIndex)
{
    mStateDispatcher.dispatch("onDeAuthenticated", &BlazeStateEventHandler::onDeAuthenticated, userIndex);
    mApiStateDispatcher.dispatch("onDeAuthenticated", &BlazeStateEventHandler::onDeAuthenticated, userIndex);
    mFrameworkStateDispatcher.dispatch("onDeAuthenticated", &BlazeStateEventHandler::onDeAuthenticated, userIndex);
}

void BlazeHub::onLocalUserForcedDeAuthenticated(uint32_t userIndex, UserSessionForcedLogoutType forcedLogoutType)
{
    mStateDispatcher.dispatch("onForcedDeAuthenticated", &BlazeStateEventHandler::onForcedDeAuthenticated, userIndex, forcedLogoutType);
    mApiStateDispatcher.dispatch("onForcedDeAuthenticated", &BlazeStateEventHandler::onForcedDeAuthenticated, userIndex, forcedLogoutType);
    mFrameworkStateDispatcher.dispatch("onForcedDeAuthenticated", &BlazeStateEventHandler::onForcedDeAuthenticated, userIndex, forcedLogoutType);
}


void BlazeHub::onConnected()
{
    mFrameworkStateDispatcher.dispatch("onConnected", &BlazeStateEventHandler::onConnected);
    mApiStateDispatcher.dispatch("onConnected", &BlazeStateEventHandler::onConnected);
    mStateDispatcher.dispatch("onConnected", &BlazeStateEventHandler::onConnected);
    getServiceResolver()->destroyRedirectorProxy();
}

void BlazeHub::onDisconnected(BlazeError errorCode)
{
    if (errorCode != ERR_OK)
    {
        BLAZE_SDK_DEBUGF("[BlazeHub] Received disconnect from ConnectionManager error(%s)\n", getErrorName(errorCode, getUserManager()->getPrimaryLocalUserIndex()));
    }
    mStateDispatcher.dispatch("onDisconnected", &BlazeStateEventHandler::onDisconnected, errorCode);
    mApiStateDispatcher.dispatch("onDisconnected", &BlazeStateEventHandler::onDisconnected, errorCode);
    mFrameworkStateDispatcher.dispatch("onDisconnected", &BlazeStateEventHandler::onDisconnected, errorCode);
}

void BlazeHub::createAPI(APIId id, API *api)
{
    if (mAPIStorage[id] != nullptr)
    {
        BlazeAssertMsg(false,  "Ignoring attempt to re-create API, possible memory leak!");
        return;
    }

    if(id > NULL_API && id < MAX_API_ID && api != nullptr)
    {
        mAPIStorage[id] = api;

        //Go ahead and print out any starup params this api might have.
        api->logStartupParameters();

        // if the API is 'well known', we store off the per-user api array (so we don't have to look 'em up anymore)
        switch( id )
        {
            case CENSUSDATA_API:
                mCensusDataAPI = (Blaze::CensusData::CensusDataAPI *)(api);
            break;
            case GAMEMANAGER_API:
                mGameManagerAPI = (Blaze::GameManager::GameManagerAPI *)(api);
            break;
            case STATS_API:
                mStatsAPI = (Blaze::Stats::StatsAPI *)(api);
            break;
            case LEADERBOARD_API:
                mLeaderboardAPI = (Blaze::Stats::LeaderboardAPI *)(api);
            break;
            case UTIL_API:
                mUtilAPI = (Blaze::Util::UtilAPI *)(api);
            break;
            case TELEMETRY_API:
            case MESSAGING_API:
            case ASSOCIATIONLIST_API:
                BlazeAssertMsg(false, "API is a multiple API and must be instantiated as such.");
                break;
        }
    }
}

void BlazeHub::createAPI(APIId id, APIPtrVector *apiPtrs)
{
    if (mAPIMultiStorage[id] != nullptr)
    {
        BlazeAssertMsg(false,  "Ignoring attempt to re-create multiAPI, possible memory leak!");
        return;
    }

    if(id < MAX_API_ID && mAPIMultiStorage[id] == nullptr && apiPtrs != nullptr && apiPtrs->size() == getNumUsers())
    {
        //Put the first api in single storage too for our default getAPI functionality.
        mAPIStorage[id] = apiPtrs->at(0);
        mAPIMultiStorage[id] = apiPtrs;

        //Go ahead and print out any startup params this api might have.
        apiPtrs->at(0)->logStartupParameters();

        // if the API is 'well known', we store off the per-user api array (so we don't have to look 'em up anymore)
        switch( id )
        {
        case MESSAGING_API:
            mMessagingAPIs = apiPtrs;
            break;
        case ASSOCIATIONLIST_API:
            mAssociationListAPIs = apiPtrs;
            break;
        case TELEMETRY_API:
            mTelemetryAPIs = apiPtrs;
            break;
        case CENSUSDATA_API:
        case GAMEMANAGER_API:
        case STATS_API:
        case LEADERBOARD_API:
        case UTIL_API:
            BlazeAssertMsg(false, "API is a singleton API and should be instantiated as such.");
            break;
        }
    }
}

API *BlazeHub::getAPI(APIId id, uint32_t userIndex) const
{
    if (userIndex == 0)
    {
        return getAPI(id);
    }
    else
    {
        API *result = nullptr;
        if (id < MAX_API_ID)
        {
            if (userIndex < getNumUsers())
            {
                APIPtrVector *vector = mAPIMultiStorage[id];
                result = vector != nullptr ? vector->at(userIndex) : getAPI(id);
            }
        }

        return result;
    }
}

Blaze::Messaging::MessagingAPI *BlazeHub::getMessagingAPI(uint32_t userIndex) const
{
    if(mMessagingAPIs != nullptr && userIndex < mNumUsers )
    {
        return reinterpret_cast<Blaze::Messaging::MessagingAPI *>(mMessagingAPIs->at(userIndex));
    }

    return nullptr;
}

Blaze::Telemetry::TelemetryAPI *BlazeHub::getTelemetryAPI(uint32_t userIndex) const
{
    if(mTelemetryAPIs != nullptr && userIndex < mNumUsers )
    {
        return static_cast<Blaze::Telemetry::TelemetryAPI *>(mTelemetryAPIs->at(userIndex));
    }

    return nullptr;
}

Blaze::Association::AssociationListAPI *BlazeHub::getAssociationListAPI(uint32_t userIndex) const
{
    if(mAssociationListAPIs != nullptr && userIndex < mNumUsers )
    {
        return reinterpret_cast<Blaze::Association::AssociationListAPI *>(mAssociationListAPIs->at(userIndex));
    }

    return nullptr;
}

uint32_t BlazeHub::getPrimaryLocalUserIndex() const
{
    return mUserManager->getPrimaryLocalUserIndex();
}


BlazeHub::BlazeHub(const InitParameters &params, MemoryGroupId memGroupId) :
    mIdleStarveMax(IDLE_STARVE_MAX),
    mIdleProcessingMax(IDLE_PROCESSING_MAX),
    mIdleLogInterval(0),
    mAPIStorage(memGroupId, MAX_API_ID, MEM_NAME(memGroupId, "BlazeHub::mAPIStorage")),
    mAPIMultiStorage(memGroupId, MAX_API_ID, MEM_NAME(memGroupId, "BlazeHub::mAPIMultiStorage")),
    mNumUsers(params.UserCount),
    mInitParams(params),
    mCertData(nullptr),
    mKeyData(nullptr),
    mConnectionManager(nullptr),
    mLoginManagers(memGroupId, params.UserCount, MEM_NAME(memGroupId, "BlazeHub::mLoginManagers")),
    mUserManager(nullptr),
    mUserGroupProviderByTypeMap(memGroupId, MEM_NAME(memGroupId, "BlazeHub::mUserGroupProviderByTypeMap")),
    mUserGroupProviderByComponentMap(memGroupId, MEM_NAME(memGroupId, "BlazeHub::mUserGroupProviderByComponentMap")),
    mLastCurrMillis(getCurrentTime()),
    mLastIdleLogMillis(0),
    mIsIdling(false),
    mScheduler(memGroupId),
    mServiceResolver(nullptr),

    //Init the api cache pointers
    mCensusDataAPI(nullptr),
    mGameManagerAPI(nullptr),
    mStatsAPI(nullptr),
    mLeaderboardAPI(nullptr),
    mMessagingAPIs(nullptr),
    mTelemetryAPIs(nullptr),
    mUtilAPI(nullptr),
    mAssociationListAPIs(nullptr),
    mByteVaultManager(nullptr),
    mS2SManager(nullptr)

#if ENABLE_BLAZE_SDK_LOGGING
    , mGlobalLoggingOptions(true, true)
    , mComponentTDFLoggingMap(memGroupId, MEM_NAME(memGroupId, "BlazeHub::mComponentTDFLoggingMap"))
#endif
#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
    , mBlazeSenderStateListener(nullptr)
#endif

{
    setMaxIdleProcessingTime(IDLE_PROCESSING_MAX);

    //Make our critical section
    mCritSec = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "NetCrit") NetCritT;
    NetCritInit(mCritSec, "BlazeSDK");
}

BlazeHub::~BlazeHub()
{
#if ENABLE_BLAZE_SDK_LOGGING
    resetTDFLogging();
#endif

    uint32_t id;
    for(id = (uint32_t)NULL_API; id != (uint32_t)MAX_API_ID; id++)
    {
        switch((APIIdDefinitions)id)
        {
            case NULL_API:
                break;
            case GAMEMANAGER_API:
                BLAZE_DELETE(MEM_GROUP_GAMEMANAGER, mAPIStorage[(APIIdDefinitions)id]);
                break;
            case STATS_API:
                BLAZE_DELETE(MEM_GROUP_STATS, mAPIStorage[(APIIdDefinitions)id]);
                break;
            case LEADERBOARD_API:
                BLAZE_DELETE(MEM_GROUP_LEADERBOARD, mAPIStorage[(APIIdDefinitions)id]);
                break;
            case MESSAGING_API:
                {
                    APIPtrVector* msgAPIs = mAPIMultiStorage[(APIIdDefinitions)id];
                    if (msgAPIs != nullptr)
                    {
                        APIPtrVector::iterator iter = msgAPIs->begin();
                        APIPtrVector::iterator end = msgAPIs->end();
                        for (; iter != end; ++iter)
                        {
                            BLAZE_DELETE(MEM_GROUP_MESSAGING, *(iter));
                        }
                        BLAZE_DELETE(MEM_GROUP_MESSAGING, msgAPIs);
                    }
                }
                break;
            case CENSUSDATA_API:
                BLAZE_DELETE(MEM_GROUP_CENSUSDATA, mAPIStorage[(APIIdDefinitions)id]);
                break;
            case UTIL_API:
                BLAZE_DELETE(MEM_GROUP_UTIL, mAPIStorage[(APIIdDefinitions)id]);
                break;
            case TELEMETRY_API:
                {
                    APIPtrVector* telemetryAPIs = mAPIMultiStorage[(APIIdDefinitions)id];
                    if (telemetryAPIs != nullptr)
                    {
                        APIPtrVector::iterator iter = telemetryAPIs->begin();
                        APIPtrVector::iterator end = telemetryAPIs->end();
                        for (; iter != end; ++iter)
                        {
                            BLAZE_DELETE(MEM_GROUP_TELEMETRY, *(iter));
                        }
                        BLAZE_DELETE(MEM_GROUP_TELEMETRY, telemetryAPIs);
                    }
                }
                break;
            case ASSOCIATIONLIST_API:
                {
                    APIPtrVector* alAPIs = mAPIMultiStorage[(APIIdDefinitions)id];
                    if (alAPIs != nullptr)
                    {
                        APIPtrVector::iterator iter = alAPIs->begin();
                        APIPtrVector::iterator end = alAPIs->end();
                        for (; iter != end; ++iter)
                        {
                            BLAZE_DELETE(MEM_GROUP_ASSOCIATIONLIST, *(iter));
                        }
                        BLAZE_DELETE(MEM_GROUP_ASSOCIATIONLIST, alAPIs);
                    }
                }
                break;
            case FIRST_CUSTOM_API:
                // TODO:  Martin Clouatre (april 2009) This is probably not correct, but don't know what to assume here.
                BLAZE_DELETE(MEM_GROUP_FRAMEWORK, mAPIStorage[(APIIdDefinitions)id]);
                break;
            default:
                break;
         }
    }

    if(!mLoginManagers.empty())
    {
        LoginManagerVector::iterator li = mLoginManagers.begin();
        LoginManagerVector::iterator le = mLoginManagers.end();
        for( ; li != le; ++li)
        {
            if (*li != nullptr)
            {
                if(mUserManager != nullptr)
                {
                    (*li)->removeAuthListener(mUserManager);
                }
                BLAZE_DELETE(MEM_GROUP_LOGINMANAGER,*li);
            }
        }
        mLoginManagers.clear();
    }

    if(mUserManager != nullptr)
    {
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK,mUserManager);
        mUserManager = nullptr;
    }

    if(mByteVaultManager != nullptr)
    {
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK, mByteVaultManager);
        mByteVaultManager = nullptr;
    }

    if( mConnectionManager != nullptr)
    {
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK,mConnectionManager);
        mConnectionManager = nullptr;
    }

    if (mServiceResolver != nullptr)
    {
       BLAZE_DELETE(MEM_GROUP_FRAMEWORK,mServiceResolver);
       mServiceResolver = nullptr;
    }

    if (mS2SManager != nullptr)
    {
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK,mS2SManager);
        mS2SManager = nullptr;
    }

    if (mCertData != nullptr)
    {
        BLAZE_FREE(MEM_GROUP_FRAMEWORK, mCertData);
        mCertData = nullptr;
    }

    if (mKeyData != nullptr)
    {
        BLAZE_FREE(MEM_GROUP_FRAMEWORK, mKeyData);
        mKeyData = nullptr;
    }

    removeIdler(&mScheduler);

    BlazeAssert(mIdlerDispatcher.getDispatcheeCount() == 0);

    NetCritKill(mCritSec);
    BLAZE_DELETE(MEM_GROUP_FRAMEWORK,mCritSec);
}

BlazeError BlazeHub::initializeInternal(const char8_t* certData, const char8_t* keyData)
{
    // Initialize internal timer
    addIdler(&mScheduler, "JobScheduler");

    if (certData != nullptr)
        mCertData = blaze_strdup(certData, MEM_GROUP_FRAMEWORK);
    if (keyData != nullptr)
        mKeyData = blaze_strdup(keyData, MEM_GROUP_FRAMEWORK);

    //Create the connection manager.
    mConnectionManager = ConnectionManager::ConnectionManager::create(*this);
    if (mConnectionManager != nullptr)
    {
        mConnectionManager->addStateListener(this);
    }
    else
    {
        return SDK_ERR_NO_MEM;
    }

    // create the service resolver (for talking to the redirector)
    mServiceResolver = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "ServiceResolver") ServiceResolver(*this, MEM_GROUP_FRAMEWORK);

    //Create components needed by our core components.
    Util::UtilComponent::createComponent(this);
    Authentication::AuthenticationComponent::createComponent(this);
    UserSessionsComponent::createComponent(this); //UserSessions used by UserManager for External Id/XUID/SceNpId lookup

    mByteVaultManager = ByteVaultManager::ByteVaultManager::create(this);

    // create the s2s manager
    mS2SManager = S2SManager::S2SManager::create(this);

    //Create user manager
    //user manager needs to be create before the login manager because the login manager needs to register
    //for extended data updates
    mUserManager = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "UserManager") UserManager::UserManager(this, MEM_GROUP_FRAMEWORK);
    mUserManager->addUserManagerStateListener(this);

    // Create login managers
    for( uint32_t i=0; i < mNumUsers; i++ )
    {
        mLoginManagers[i] = LoginManager::LoginManager::create( this, i );
        mLoginManagers[i]->addAuthListener(mUserManager);
    }

    return ERR_OK;
}

const char8_t* EnvironmentTypeToString(EnvironmentType type)
{
    switch (type)
    {
        case ENVIRONMENT_SDEV:
            return "ENVIRONMENT_SDEV";

        case ENVIRONMENT_STEST:
            return "ENVIRONMENT_STEST";

        case ENVIRONMENT_SCERT:
            return "ENVIRONMENT_SCERT";

        case ENVIRONMENT_PROD:
            return "ENVIRONMENT_PROD";

        default:
            return "ENVIRONMENT_UNKNOWN";
    }
}


#if ENABLE_BLAZE_SDK_LOGGING

// Enable/Disable TDF logging per component (Overrides global setting)
void BlazeHub::setComponentTDFLoggingOptions(ComponentId componentId, const TDFLoggingOptions& options) 
{
    ComponentTDFLoggingMap::insert_return_type iter = mComponentTDFLoggingMap.insert(eastl::make_pair(componentId, (ComponentTDFLogging*) nullptr));
    if (iter.second)
        iter.first->second = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "ComponentLogging") ComponentTDFLogging(options, MEM_GROUP_FRAMEWORK, "ComponentLogging");
    else
        iter.first->second->mComponentLoggingOptions = options;
}

const BlazeHub::TDFLoggingOptions& BlazeHub::getComponentTDFLoggingOptions(ComponentId componentId) const
{
    ComponentTDFLoggingMap::const_iterator iter = mComponentTDFLoggingMap.find(componentId);
    if (iter == mComponentTDFLoggingMap.cend())
        return mGlobalLoggingOptions;

    return iter->second->mComponentLoggingOptions;
}

void BlazeHub::setComponentTDFLogging(ComponentId componentId, bool enableComponentTDFLogging)
{
    setComponentTDFLoggingOptions(componentId, TDFLoggingOptions(enableComponentTDFLogging, enableComponentTDFLogging));
}

bool BlazeHub::getComponentTDFLogging(ComponentId componentId) const
{
    const TDFLoggingOptions& options = getComponentTDFLoggingOptions(componentId);
    return options.logInbound || options.logOutbound;
}

// Enable/Disable TDF logging per component RPC/Notification
void BlazeHub::setComponentCommandTDFLoggingOptions(ComponentId componentId, uint16_t commandId, const TDFLoggingOptions& options, bool notification)
{
    ComponentTDFLoggingMap::insert_return_type iter = mComponentTDFLoggingMap.insert(eastl::make_pair(componentId, (ComponentTDFLogging*) nullptr));
    if (iter.second)
    {
        // Use the global settings here.
        iter.first->second = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "ComponentLogging") ComponentTDFLogging(mGlobalLoggingOptions, MEM_GROUP_FRAMEWORK, "ComponentLogging");
    }

    if (notification)
        iter.first->second->mNotificationLoggingOptions[commandId] = options;
    else
        iter.first->second->mCommandLoggingOptions[commandId] = options;
}

const BlazeHub::TDFLoggingOptions& BlazeHub::getComponentCommandTDFLoggingOptions(ComponentId componentId, uint16_t commandId, bool notification) const
{
    ComponentTDFLoggingMap::const_iterator compIter = mComponentTDFLoggingMap.find(componentId);
    if (compIter == mComponentTDFLoggingMap.cend())
        return mGlobalLoggingOptions;

    if (notification)
    {
        ComponentTDFLogging::CommandLoggingMap::const_iterator notifIter = compIter->second->mNotificationLoggingOptions.find(commandId);
        if (notifIter == compIter->second->mNotificationLoggingOptions.cend())
            return compIter->second->mComponentLoggingOptions;

        return notifIter->second;
    }
    else
    {
        ComponentTDFLogging::CommandLoggingMap::const_iterator commandIter = compIter->second->mCommandLoggingOptions.find(commandId);
        if (commandIter == compIter->second->mCommandLoggingOptions.cend())
            return compIter->second->mComponentLoggingOptions;

        return commandIter->second;
    }
}

void BlazeHub::setComponentCommandTDFLogging(ComponentId componentId, uint16_t commandId, bool enableCommandTDFLogging, bool notification)
{
    setComponentCommandTDFLoggingOptions(componentId, commandId, TDFLoggingOptions(enableCommandTDFLogging, enableCommandTDFLogging), notification);
}

bool BlazeHub::getComponentCommandTDFLogging(ComponentId componentId, uint16_t commandId, bool notification) const
{
    const TDFLoggingOptions& options = getComponentCommandTDFLoggingOptions(componentId, commandId, notification);
    return options.logInbound || options.logOutbound;
}

// Clear/Reset Logging
void BlazeHub::resetTDFLogging()
{
    mGlobalLoggingOptions.logInbound = true;
    mGlobalLoggingOptions.logOutbound = true;

    ComponentTDFLoggingMap::iterator curIter = mComponentTDFLoggingMap.begin();
    ComponentTDFLoggingMap::iterator endIter = mComponentTDFLoggingMap.end();

    // Clear all allocations:
    for (; curIter != endIter; ++curIter)
    {
        BLAZE_DELETE( MEM_GROUP_FRAMEWORK, curIter->second );
    }
    mComponentTDFLoggingMap.clear();
}

#endif


}
