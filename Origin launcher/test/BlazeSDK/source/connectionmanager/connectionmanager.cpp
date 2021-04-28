/**************************************************************************************************/
/*!
    \file connectionmanager.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/fire2connection.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/component/utilcomponent.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/serviceresolver.h"
#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/component/framework/tdf/networkaddress.h"
#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"
#include "BlazeSDK/component/redirectorcomponent.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/util/utilapi.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/misc/qosclient.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoupnp.h"
#include "DirtySDK/proto/protossl.h"

#include "EAStdC/EAString.h"

#if defined(EA_PLATFORM_PS5)
#include <np/np_common.h> //for SCE_NP_TITLE_ID_LEN in sendPreAuth()
#endif

#define DEFAULT_PING_PERIOD_IN_MS (15000)
#define MIN_PING_PERIOD_IN_MS (1000)
#define DEFAULT_CONN_IDLE_IN_MS (40000)

namespace Blaze
{
namespace ConnectionManager
{

ConnectionStatus::ConnectionStatus()
{
    mError = ERR_OK;
    mNetConnStatus = 0;
    mProtoSslError = PROTOSSL_ERROR_NONE;
    mSocketError = SOCKERR_NONE;
    mNumReconnections = 0;
}


ConnectionManager* ConnectionManager::create(BlazeHub &hub)
{
    return BLAZE_NEW(MEM_GROUP_FRAMEWORK, "ConnectionManager") ConnectionManager(hub, MEM_GROUP_FRAMEWORK);
}

;
ConnectionManager::ConnectionManager(BlazeHub &hub, MemoryGroupId memGroupId)
:   mBlazeHub(hub),
    mBlazeConnection(hub),
    mProtoUpnp(nullptr),
    mTimeSpentWaitingForConnection(0),
    mSilentConnect(false),
    mConnectionCbJobId(INVALID_JOB_ID),
    mUpnpJobId(INVALID_JOB_ID),
    mLastPingServerTimeS(0),
    mLastPingClientTimeMS(0),
    mUserManager(nullptr),
    mUpnpInfoDiscovered(false),
    mConnectionAttempted(false),
    mIsWaitingForNetConn(false),
    mConnecting(false),
    mConnected(false),
    mIsReconnectingInProgress(false),
    mAutoReconnectEnabled(true),
    mClientConfig(memGroupId),
    mClientPlatformTypeOverride(INVALID),
    mQosManager(hub, memGroupId)
{
    mBlazeHub.addIdler(this, "ConnectionManager");

    mBlazeConnection.setAutoConnect(true);
    mBlazeConnection.setAutoReconnect(mAutoReconnectEnabled);
    mBlazeConnection.setDefaultRequestTimeout(mBlazeHub.getInitParams().DefaultRequestTimeout);
    mBlazeConnection.setServerConnInfo(mBlazeHub.getInitParams().ServiceName);
    mBlazeConnection.setConnectionCallbacks(
        Fire2Connection::ConnectionCallback(this, &ConnectionManager::onBlazeConnect),
        Fire2Connection::ConnectionCallback(this, &ConnectionManager::onBlazeDisconnect),
        Fire2Connection::ReconnectCallback(this, &ConnectionManager::onBlazeReconnectBegin),
        Fire2Connection::ReconnectCallback(this, &ConnectionManager::onBlazeReconnectEnd));

    blaze_strnzcpy(mServerVersion, "", sizeof(mServerVersion));
    blaze_strnzcpy(mPlatform, "", sizeof(mPlatform));
    blaze_strnzcpy(mServiceName, "", sizeof(mServiceName));
    blaze_strnzcpy(mClientId, "", sizeof(mClientId));
    blaze_strnzcpy(mReleaseType, "", sizeof(mReleaseType));
}

ConnectionManager::~ConnectionManager()
{
    //Cancel our ping scheduled function
    mBlazeHub.getScheduler()->removeByAssociatedObject(this);
    mBlazeHub.removeIdler(this);

    if (mProtoUpnp != nullptr)
    {
        ProtoUpnpDestroy(mProtoUpnp);
    }

    if (mBlazeConnection.getConnectionState() != Fire2Connection::OFFLINE)
        doDisconnect(ERR_OK, ERR_OK);

    if (mConnectionAttempted)
    {
        NetConnDisconnect();
    }
}

/*! ***********************************************************************************************/
/*! \name Connection methods
***************************************************************************************************/

void ConnectionManager::generateConnectionParams(char8_t* buff, uint32_t size, uint8_t isSilentConnect/* = true*/) const
{
    memset(buff, 0, size);
    const char8_t* strEnv = nullptr;
    const char8_t* strSilent = "silent=true";
    char8_t  strPort[32];

    //TODO DirtySDK V7, should have service=0x?? so parameter order would not matter
    //as it is now service ID must be first.
    //TODO setup redirector host for PC based off environment
    switch (mBlazeHub.getInitParams().Environment)
    {
    case ENVIRONMENT_SDEV:
        strEnv = "0x45410000";  //"dev"
        break;
    case ENVIRONMENT_STEST:
        strEnv = "0x45410004";  //"prod"
        break;
    case ENVIRONMENT_SCERT:
        strEnv = "0x45410004";
        break;
    case ENVIRONMENT_PROD:
        strEnv = "0x45410004";
        break;
    default:
        //unknown environment type
        assert(strEnv != nullptr);
        break;
    }

    if(!isSilentConnect)
    {
        strSilent = "silent=false";
    }

    // format peer port specification
    blaze_snzprintf(strPort, sizeof(strPort), "peerport=%d", mBlazeHub.getInitParams().GamePort);

    //put the environment, silent flag, and any special user params together in a string
    blaze_snzprintf(buff, size, "%s %s %s %s", strEnv, strSilent, strPort, mBlazeHub.getInitParams().AdditionalNetConnParams);
}

bool ConnectionManager::connect(const ConnectionMessagesCb& connectCb, uint8_t isSilent)
{
    //abort if state is already connected
    if (mConnected)
    {
        BLAZE_SDK_DEBUGF("ConnectionManager::Connect: Attempting to connect while already connected. Action is a no-op.");

        static Blaze::Redirector::DisplayMessageList dummyMessageList;
        connectCb(ERR_OK, &dummyMessageList);
        return false;
    }

    // Reset this in case it has been changed
    if (!mBlazeConnection.getServerConnInfo().isResolved())
    {
        mBlazeConnection.setServerConnInfo(mBlazeHub.getInitParams().ServiceName);
    }

    // If auto connect was disabled due to the user signing out of 1st party, re-enable it when they call connect
    mBlazeConnection.setAutoConnect(true);

    mSilentConnect = isSilent;

    // We create a job here to manage the callback, and allow it to be removed
    ConnectionCbJob *connectionCbJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "ConnectCbJob") ConnectionCbJob(connectCb);
    mConnectionCbJobId = mBlazeHub.getScheduler()->scheduleJobNoTimeout("ConnectCbJob", connectionCbJob, this);

    mTimeSpentWaitingForConnection = 0;

    mConnecting = true;
    mIsWaitingForNetConn = true;

    int32_t status = NetConnStatus('conn', 0, nullptr, 0);
    //only call NetConnConnect when it is not already connected
    if (status != '+onl')
    {
        if (mConnectionAttempted)
        {
            NetConnDisconnect();
        }

        mConnectionAttempted = true;
        char8_t params[256];

        generateConnectionParams(params, sizeof(params), mSilentConnect);
        NetConnConnect(nullptr, params, 0xF);
    }

    return true;
}

void ConnectionManager::dispatchDisconnect(BlazeError error)
{
    // If we're already disconnected, there's no need to dispatch another callback.
    if (!mConnecting && !mConnected)
    {
        BLAZE_SDK_DEBUGF("ConnectionManager::dispatchDisconnect: Skipping dispatch since the we are not connected or connecting.");
        return;
    }

    mConnecting = false;
    mConnected = false;
    mStateDispatcher.dispatch("onDisconnected", &ConnectionManagerStateListener::onDisconnected, error);
}

const char8_t* ConnectionManager::getServerAddress() const
{
    return mBlazeConnection.getServerConnInfo().getAddress();
}

ClientPlatformType ConnectionManager::getClientPlatformType() const
{
    if (mClientPlatformTypeOverride != INVALID)
    {
        return mClientPlatformTypeOverride;
    }
    else
    {
        #if defined(EA_PLATFORM_WINDOWS)
            return pc;
        #elif defined(EA_PLATFORM_ANDROID)
            return android;
        #elif defined(EA_PLATFORM_IPHONE)
            return ios;
        #elif defined(EA_PLATFORM_OSX)
            // treat osx as pc. blaze doesn't currently support osx as a real platform
            return pc;
        #elif defined(EA_PLATFORM_XBSX)
            return xbsx;
        #elif defined(EA_PLATFORM_XBOXONE)
            return xone;
        #elif defined(EA_PLATFORM_PS4)
            return ps4;
        #elif defined(EA_PLATFORM_NX)
            return nx;
        #elif defined(EA_PLATFORM_PS5)
            return ps5;
        #elif defined(EA_PLATFORM_STADIA)
             return stadia;
        #else 
            return INVALID;
        #endif
    }
}

void ConnectionManager::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    if (mBlazeHub.getInitParams().EnableNetConnIdle)
    {
        BLAZE_SDK_SCOPE_TIMER("ConnectionManager_NetConnIdle");
        NetConnIdle();
    }

    int32_t status = NetConnStatus('conn', 0, nullptr, 0);
    
    BlazeError error = ERR_OK;
    BlazeError extendedError = ERR_OK;
    netConnectionStatusToBlazeError(status, mConnected, error, extendedError);

    if (mIsWaitingForNetConn)
    {
        if (status == '+onl')
        {
            BLAZE_SDK_SCOPE_TIMER("ConnectionManager_connect");

            mTimeSpentWaitingForConnection = 0;
            mIsWaitingForNetConn = false;
            mBlazeConnection.setAutoReconnect(false);

            mBlazeConnection.connect();
        }
        else if(mSilentConnect)
        {
            // time waiting for connection, on ps3 when connecting silently if the user isn't signed in
            // it may never be +onl, though nothing wrong has happened according to DS so we wont hit an error state
            mTimeSpentWaitingForConnection += elapsedTime;
            if (mTimeSpentWaitingForConnection >= MAX_TIME_TO_WAIT_FOR_CONNECTION)
            {
                error = SDK_ERR_CONN_FAILED;
                extendedError = SDK_ERR_NETWORK_CONN_TIMEOUT;
                mTimeSpentWaitingForConnection = 0;
            }
            else
                return;
        }
    }

    //any error condition bail
    if ((status >> 24) == '-')
    {
        if (status == '-act')
        {
            // The user has signed out of first party, so ensure that any RPCs that are called after disconnecting
            // do not automatically connect; force the client to explicitly connect again
            mBlazeConnection.setAutoConnect(false);
        }
        if (canDisconnect())
        {
            doDisconnect(error, extendedError);
        }
    }

    // refresh the QoS Manager on regular timer, or after a failure:
    if (!mBlazeHub.areUsersInActiveSession() &&
        ((mTimeOfLastQosRefresh != 0 && mQosManager.mQosConfigInfo.getClientQosRefreshRate() != 0 && 
          EA::TDF::TimeValue::getTimeOfDay() - mTimeOfLastQosRefresh > mQosManager.mQosConfigInfo.getClientQosRefreshRate()) || 
         (mTimeOfLastQosRefreshFailure != 0 && mQosManager.mQosConfigInfo.getClientQosFailureRefreshRate() != 0 &&
          EA::TDF::TimeValue::getTimeOfDay() - mTimeOfLastQosRefreshFailure > mQosManager.mQosConfigInfo.getClientQosFailureRefreshRate())) )
    {
        BLAZE_SDK_SCOPE_TIMER("ConnectionManager_refreshQos");

        mTimeOfLastQosRefreshFailure = 0;
        mTimeOfLastQosRefresh = EA::TDF::TimeValue::getTimeOfDay();
        refreshQosManager();
    }

    //service qos
    mQosManager.idle();
}

/*! ************************************************************************************************/
/*! \brief translate DirtySDK NetConnStatus('conn') status to appropriate Blaze error.
***************************************************************************************************/
void ConnectionManager::netConnectionStatusToBlazeError(int32_t status, bool connected, BlazeError& error, BlazeError& extendedError) const
{
    if (status == '-act')
    {
        error = SDK_ERR_NO_FIRST_PARTY_ACCOUNT;
        extendedError = error;
    }
    else if (status == '-dsc' || status == '-err' || status == '-xbl' || status == '-svc')
    {
        error = SDK_ERR_CONN_FAILED;
        extendedError = SDK_ERR_NETWORK_CONN_FAILED;
    }
    else if (status == '-dup')
    {
        error = ERR_DUPLICATE_LOGIN;
        extendedError = error;
    }
    else if ((status >> 24) == '-')
    {
        // Generic code
        error = (connected ? ERR_DISCONNECTED : SDK_ERR_CONN_FAILED);
        extendedError = (connected? SDK_ERR_NETWORK_DISCONNECTED : SDK_ERR_NETWORK_CONN_FAILED);
    }
}

bool ConnectionManager::canDisconnect() const 
{
    // Note: We don't have to check for mIsReconnectingInProgress here because mConnected remains true while the reconnect occurs.
    return (mConnected || mConnecting);
}

void ConnectionManager::connectComplete()
{
    mConnecting = false;
    mConnected = true;
    mBlazeConnection.setAutoReconnect(mAutoReconnectEnabled);

    if (mBlazeConnection.isMigrationInProgress())
    {
        mBlazeConnection.migrationComplete();
    }
    else
    {
        // Only trigger onConnected callback, when preAuth completes not as part of auto-connection migration
        mStateDispatcher.dispatch("onConnected", &ConnectionManagerStateListener::onConnected);
    }
}

bool ConnectionManager::disconnect(BlazeError error)
{
    if (canDisconnect())
    {
        doDisconnect(error, error);
        return true;
    }
    return false;
}

void ConnectionManager::doDisconnect(BlazeError error, BlazeError extendedError)
{
    mIsWaitingForNetConn = false;
    mIsReconnectingInProgress = false;

    if (mBlazeConnection.isActive())
        mBlazeConnection.disconnect();

    //Cancel our ping scheduled function
    mBlazeHub.getScheduler()->removeByAssociatedObject(this);

    setConnectionStatus(error, extendedError);

    //disconnect doesn't do our callback, so do it directly here.
    dispatchDisconnect(error);

    //send up fresh upnp metrics when the user connects again
    resetRetriveUpnpFlag();
}

void ConnectionManager::onBlazeReconnectBegin()
{
    if (!mIsReconnectingInProgress)
       mDispatcher.dispatch("onReconnectionStart", &ConnectionManagerListener::onReconnectionStart);

    mIsReconnectingInProgress = true;
    ++mConnectionStatus.mNumReconnections;
}

void ConnectionManager::onBlazeReconnectEnd()
{
    mIsReconnectingInProgress = false;

    mDispatcher.dispatch("onReconnectionFinish", &ConnectionManagerListener::onReconnectionFinish, true);
}

/*! ***********************************************************************************************/
/*! \name Helper methods
***************************************************************************************************/

void ConnectionManager::setConnectionStatus(BlazeError error, BlazeError extendedError)
{
    mConnectionStatus.mError = extendedError;
    if (mConnectionAttempted && !canDisconnect())
    {
        mConnectionStatus.mConnectTime.setMillis((int64_t)mTimeSpentWaitingForConnection);
    }

    //  protoSsl errors set externally as these values are set only on specific error conditions.
    //  set the current Network connection status as the status also includes errors returned from dirtysock.
    mConnectionStatus.mNetConnStatus = NetConnStatus('conn', 0, nullptr, 0);

}

/*! ***********************************************************************************************/
/*! \name returns the Blaze Server time UTC (in seconds)
***************************************************************************************************/
uint32_t ConnectionManager::getServerTimeUtc() const
{
    if (mLastPingServerTimeS == 0)
    {
        BLAZE_SDK_DEBUGF("ConnectionManager: Error: Attempting to get server time before first ping server time has been recieved. Returning 0.\n");
        return 0;
    }
    int32_t tickDiff = NetTickDiff(mBlazeHub.getCurrentTime(), mLastPingClientTimeMS) / 1000;
    return mLastPingServerTimeS + tickDiff;
}

/*! ***********************************************************************************************/
/*! \name Config methods
***************************************************************************************************/
const Util::FetchConfigResponse::ConfigMap& ConnectionManager::getServerConfigs() const
{
    return mClientConfig.getConfig();
}

bool ConnectionManager::getServerConfigString(const char8_t* key, const char8_t** value) const
{
    Util::FetchConfigResponse::ConfigMap::const_iterator find
        = mClientConfig.getConfig().find(key);
    if (find == mClientConfig.getConfig().end())
        return false;
    *value = find->second.c_str();

    return true;
}

bool ConnectionManager::getServerConfigInt(const char8_t* key, int32_t* value) const
{
    const char8_t* valueStr;
    if (!getServerConfigString(key, &valueStr))
        return false;
    *value = EA::StdC::AtoI32(valueStr);
    return true;
}

bool ConnectionManager::getServerConfigTimeValue(const char8_t* key, TimeValue& value) const
{
    const char8_t* valueStr;
    if (!getServerConfigString(key, &valueStr))
        return false;
    value.parseTimeInterval(valueStr);
    return true;
}

/*! ***********************************************************************************************/
/*! \name Fire2Connection callbacks
***************************************************************************************************/

void ConnectionManager::onBlazeConnect(BlazeError result, int32_t sslError, int32_t sockError)
{
    ConnectionCbJob* connectionJob = (ConnectionCbJob*)mBlazeHub.getScheduler()->getJob(mConnectionCbJobId);
    if (connectionJob != nullptr && connectionJob->mConnectCb.isValid() && !mBlazeConnection.getDisplayMessages().empty())
    {
        connectionJob->mConnectCb(result, &mBlazeConnection.getDisplayMessages());
    }

    if (result == ERR_OK)
    {
        sendUtilPing();
        if (!mUpnpInfoDiscovered)
            mUpnpJobId = mBlazeHub.getScheduler()->scheduleMethod("retrieveUpnpStatus", this, &ConnectionManager::retrieveUpnpStatus, this);
    }
    else
    {
        onBlazeDisconnect(result, sslError, sockError);
    }
}

void ConnectionManager::onBlazeDisconnect(BlazeError result, int32_t sslError, int32_t sockError)
{
    if (mUpnpJobId != INVALID_JOB_ID)
    {
        mBlazeHub.getScheduler()->cancelJob(mUpnpJobId);
        mUpnpJobId = INVALID_JOB_ID;
    }

    mConnectionStatus.mProtoSslError = sslError;
    mConnectionStatus.mSocketError = sockError;

    //  re-map new error to the legacy error code
    BlazeError legacyResult = result;
    if (result == SDK_ERR_BLAZE_CONN_FAILED)
    {
        legacyResult = SDK_ERR_CONN_FAILED;
    }

    setConnectionStatus(legacyResult, result);

    dispatchDisconnect(legacyResult);
}

const char8_t* ConnectionManager::getUniqueDeviceId() const
{
     static char8_t uniqueDeviceId[64] = "";
     NetConnStatus('hwid', 0, uniqueDeviceId, sizeof(uniqueDeviceId));

     return uniqueDeviceId;
}

void ConnectionManager::sendPreAuth()
{
    Util::PreAuthRequest request;

    // used in conjunction with DirtySDK
    request.setLocalAddress(SocketHtonl(NetConnStatus('addr', 0, nullptr, 0)));

    // set locale and client type
    request.getClientData().setLocale(mBlazeHub.getLocale());
    request.getClientData().setCountry(mBlazeHub.getCountry());
    request.getClientData().setClientType(mBlazeHub.getInitParams().Client);
    request.getClientData().setIgnoreInactivityTimeout(mBlazeHub.getInitParams().IgnoreInactivityTimeout);
    request.getClientData().setServiceName(mBlazeConnection.getServerConnInfo().getServiceName());
    // fetch client config
    request.getFetchClientConfig().setConfigSection(BLAZESDK_CONFIG_SECTION);

    const BlazeHub::InitParameters& initParams = mBlazeHub.getInitParams();
    switch (initParams.Environment)
    {
        case ENVIRONMENT_SDEV:
            request.getClientInfo().setEnvironment(Redirector::ENVIRONMENT_SDEV);
            break;
        case ENVIRONMENT_STEST:
            request.getClientInfo().setEnvironment(Redirector::ENVIRONMENT_STEST);
            break;
        case ENVIRONMENT_SCERT:
            request.getClientInfo().setEnvironment(Redirector::ENVIRONMENT_SCERT);
            break;
        case ENVIRONMENT_PROD:
            request.getClientInfo().setEnvironment(Redirector::ENVIRONMENT_PROD);
            break;
    }
    request.getClientInfo().setPlatform(getClientPlatformType());
    request.getClientInfo().setClientName(initParams.ClientName);
    request.getClientInfo().setClientVersion(initParams.ClientVersion);
    request.getClientInfo().setClientSkuId(initParams.ClientSkuId);
    request.getClientInfo().setClientLocale(initParams.Locale);
    request.getClientInfo().setClientCountry(initParams.Country);
    request.getClientInfo().setBlazeSDKVersion(getBlazeSdkVersionString());

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    char8_t strTitleId[SCE_NP_TITLE_ID_LEN + 1];
    NetConnStatus('titl', 0, strTitleId, sizeof(strTitleId));
    request.getClientInfo().setTitleId(strTitleId);
#elif defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
    request.getClientInfo().setTitleId("mockBsdkTitleId");
#endif

    char8_t buildDate[64];
    blaze_snzprintf(buildDate, sizeof(buildDate), "%s %s", __DATE__, __TIME__);
    request.getClientInfo().setBlazeSDKBuildDate(buildDate);

    char8_t dsVers[32];
    blaze_snzprintf(dsVers, sizeof(dsVers), "%d.%d.%d.%d.%d",
        DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);

    request.getClientInfo().setDirtySDKVersion(dsVers);
    request.getClientInfo().setProtoTunnelVersion(Blaze::BlazeNetworkAdapter::ConnApiAdapter::getTunnelVersion());

    if (mBlazeConnection.isMigrationInProgress())
        mBlazeConnection.useResumeBuffer(); // one-shot resume buffer usage here, only used for sending a single command

    mBlazeHub.getComponentManager()->getUtilComponent()->preAuth(request,
            Util::UtilComponent::PreAuthCb(this, &ConnectionManager::onPreAuth));
}

void ConnectionManager::onPreAuth(const Util::PreAuthResponse* response, BlazeError error, const JobId id)
{
    if (error != Blaze::ERR_OK)
    {
        BLAZE_SDK_DEBUGF("ConnectionManager::onPreAuth: Error fetching client config from server: "
            "error code %s(0x%x); disconnecting\n", (mBlazeHub.getComponentManager() != nullptr ? mBlazeHub.getComponentManager()->getErrorName(error) : "unknown"), error);
        if (mBlazeConnection.isActive())
            doDisconnect(error, error);
        return;
    }

    // Save the config map
    response->getConfig().copyInto(mClientConfig);

    // Save the list of configured server componentIds
    response->getComponentIds().copyInto(mComponentIds);

    // Save the server version
    blaze_strnzcpy(mServerVersion, response->getServerVersion(), sizeof(mServerVersion));

    // Save the server platform
    blaze_strnzcpy(mPlatform, response->getPlatform(), sizeof(mPlatform));

    // Save the server name
    blaze_strnzcpy(mServiceName, response->getServiceName(), sizeof(mServiceName));
    blaze_strnzcpy(mClientId, response->getClientId(), sizeof(mClientId));
    blaze_strnzcpy(mReleaseType, response->getReleaseType(), sizeof(mReleaseType));

    // Save the persona namespace
    blaze_strnzcpy(mPersonaNamespace, response->getPersonaNamespace(), sizeof(mPersonaNamespace));

    BLAZE_SDK_DEBUGF("ConnectionManager::onPreAuth: BlazeSDK version: %s connected to server version: %s, serviceName: %s, platform: %s, personaNamespace: %s",
        getBlazeSdkVersionString(), getServerVersion(), getServiceName(), getPlatform(), getPersonaNamespace());

    if (isOlderBlazeServerVersion(getBlazeSdkVersionString(), getServerVersion()))
    {
        BLAZE_SDK_DEBUGF("ConnectionManager::onPreAuth: Warning: BlazeSDK is newer than the server.");
    }

    // Figure out how frequently the server wants to be pinged.
    TimeValue pingPeriod;
    if (!getServerConfigTimeValue(BLAZESDK_CONFIG_KEY_PING_PERIOD, pingPeriod))
    {
        mBlazeConnection.setPingPeriod(DEFAULT_PING_PERIOD_IN_MS);
    }
    else
    {
        if (pingPeriod.getMillis() < MIN_PING_PERIOD_IN_MS)
            mBlazeConnection.setPingPeriod(DEFAULT_PING_PERIOD_IN_MS);
        else
            mBlazeConnection.setPingPeriod(static_cast<uint32_t>(pingPeriod.getMillis()));
    }

    //See how long our default request timeout is
    TimeValue defaultRequestTimeout;
    if (getServerConfigTimeValue(BLAZESDK_CONFIG_KEY_DEFAULT_REQUEST_TIMEOUT, defaultRequestTimeout))
    {
        mBlazeConnection.setDefaultRequestTimeout((uint32_t)defaultRequestTimeout.getMillis());
    }

    // Fill out the conn idle timeout
    TimeValue connIdleTimeout;
    if (getServerConfigTimeValue(BLAZESDK_CONFIG_KEY_DEFAULT_CONN_IDLE_TIMEOUT, connIdleTimeout))
    {
        mBlazeConnection.setInactivityTimeout((uint32_t)connIdleTimeout.getMillis());
        if (mBlazeConnection.getInactivityTimeout() < Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS)
        {
            BLAZE_SDK_DEBUGF("ConnectionManager::onPreAuth: Warning: configured timeout value conn idle = %ums is less than the recommended minimum of %" PRIi64 "ms\n", mBlazeConnection.getInactivityTimeout(), Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS);
        }
    }

    // set protossl config override
    ProtoSSLRefT *pNullRef = nullptr;
    Blaze::Util::UtilAPI::createAPI(mBlazeHub);
    mBlazeHub.getUtilAPI()->OverrideConfigs(pNullRef);

    BLAZE_SDK_DEBUGF("ConnectionManager::onPreAuth: received timeout values from server: ping period = %ums, conn idle = %ums, request timeout = %ums\n", mBlazeConnection.getPingPeriod(), mBlazeConnection.getInactivityTimeout(), mBlazeConnection.getDefaultRequestTimeout());

    // Enable/disable the auto reconnect feature
    int32_t autoReconnectEnabled;
    if (getServerConfigInt(BLAZESDK_CONFIG_KEY_AUTO_RECONNECT_ENABLED, &autoReconnectEnabled))
    {
        setAutoReconnectEnabled(autoReconnectEnabled != 0);

        BLAZE_SDK_DEBUGF("ConnectionManager::onPreAuth: received auto reconnect setting from the server: %s\n", (getAutoReconnectEnabled() ? "enabled" : "disabled"));
    }

    int32_t maxReconnectAttempts;
    if (getServerConfigInt(BLAZESDK_CONFIG_KEY_MAX_RECONNECT_ATTEMPTS, &maxReconnectAttempts))
    {
        mBlazeConnection.setMaxReconnectAttempts((uint32_t)maxReconnectAttempts);
    }

    // unique identification for this machine
    NetConnSetMachineId(response->getMachineId());

    // Start the QOS process
    mQosManager.initialize(&response->getQosSettings(), QosManager::QosPingSiteLatencyRequestedCb(this, &ConnectionManager::internalQosPingSiteLatencyRequestedCb), 
                                                        QosManager::QosPingSiteLatencyRetrievedCb(this, &ConnectionManager::internalQosPingSiteLatencyRetrievedCb));
    
    connectComplete();
}  /*lint !e1746 Avoid lint to check whether parameter 'id' could be made const reference*/

void ConnectionManager::sendUtilPing()
{
    if (mBlazeConnection.isMigrationInProgress())
    {
        // Migration in process, let's ping the instance to find out where should we migrate to. 
        mBlazeConnection.useResumeBuffer(); // one-shot resume buffer usage here, only used for sending a single command
    }

    JobId pingJobId = mBlazeHub.getComponentManager()->getUtilComponent()->ping(Util::UtilComponent::PingCb(this, &ConnectionManager::onUtilPingResponse));
    Job *pingJob = mBlazeHub.getScheduler()->getJob(pingJobId);
    pingJob->setAssociatedObject(this);
}

void ConnectionManager::onUtilPingResponse(const Util::PingResponse* response, BlazeError error, JobId id)
{
    if (error != Blaze::ERR_OK)
    {
        // When migration happens, later util Pings are used to fetch ERR_MOVED and moveToAddr from Fire2Metadata. Disable disconnect functionality when it happens.
        if (error != ERR_MOVED)
            doDisconnect(SDK_ERR_SERVER_DISCONNECT, error);
        return;
    }

    mLastPingServerTimeS = response->getServerTime();
    mLastPingClientTimeMS = mBlazeHub.getCurrentTime();

    uint32_t userIndex = mBlazeHub.getComponentManager()->getUserIndex();
    const Blaze::UserManager::LocalUser* localUser = mBlazeHub.getUserManager()->getLocalUser(userIndex);
    if (localUser && localUser->hasUserSessionKey())
    {
        // This code path is executed by the client in the case of successful connection migration *after* 
        // the user session key has been cached by the BlazeSDK following login/authentication with the server.
        // BlazeSDK has an existing session key: we are done, server reattached preAuth() info from the existing user session; therefore, we skip preAuth() for compatibility and efficiency.
        BLAZE_SDK_DEBUGF("ConnectionManager::onUtilPingResponse: userIndex(%u), session key exists, complete connection\n", userIndex);
        connectComplete();
    }
    else
    {
        // This code path is executed by the client in the cases of successful:
        // A) initial connection to the blaze server
        // B) connection migration due to unexpected disconnect when no user session yet/already exists
        // BlazeSDK has no session key: proceed with the preAuth() to associate correct client info for this connection.
        BLAZE_SDK_DEBUGF("ConnectionManager::onUtilPingResponse: userIndex(%u), session key is empty, trigger preAuth()\n", userIndex);
        sendPreAuth();
    }
}   /*lint !e1746 Avoid lint to check whether parameter 'id' could be made const reference*/

/*! ************************************************************************************************/
/*! \brief reset the upnp discover flag when user logs out in case the connection still exists
***************************************************************************************************/
void ConnectionManager::resetRetriveUpnpFlag()
{
    mUpnpInfoDiscovered = false;
}

/*! ************************************************************************************************/
/*! \brief check the upnp status, we only do the check if the status is not discovered, the
    ProtoUpnpControl(...) is triggered during NetConnConnect(
***************************************************************************************************/
void ConnectionManager::retrieveUpnpStatus()
{
    // wait until we are AUTHENTICATED; we can't report metrics until we are
    UserManager::UserManager *userManager = mBlazeHub.getUserManager();
    if ((userManager == nullptr) || (userManager->getPrimaryLocalUser() == nullptr))
    {
        mUpnpJobId = mBlazeHub.getScheduler()->scheduleMethod("retrieveUpnpStatus", this, &ConnectionManager::retrieveUpnpStatus, this, 1000);
        return;
    }

    // create an instance of the upnp module to access upnp information
    if (mProtoUpnp == nullptr)
    {
        DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_FRAMEWORK));
        mProtoUpnp = ProtoUpnpCreate();
        DirtyMemGroupLeave();
    }

    // get upnp status flags
    uint16_t upnpStatusFlags = (uint16_t)ProtoUpnpStatus(mProtoUpnp, 'stat', nullptr, 0);

    // make sure we've discovered a router and that the port map process is complete (not necessarily successful)
    if ((upnpStatusFlags != 0) && ProtoUpnpStatus(mProtoUpnp, 'done', nullptr, 0))
    {
        char8_t deviceName[MAX_UPNPDEVICEINFO_LEN] = "";
        ClientMetrics request;
        uint32_t wanIpAddr;
        uint16_t wanPort, qosPort;
        UpnpStatus upnpStatus;

        // get device name info
        ProtoUpnpStatus(mProtoUpnp, 'dnam', deviceName, sizeof(deviceName));
        // get WAN IP address from UPnP
        wanIpAddr = (uint32_t)ProtoUpnpStatus(mProtoUpnp, 'extn', nullptr, 0);
        // get external port from UPnP
        wanPort = (uint16_t)ProtoUpnpStatus(mProtoUpnp, 'extp', nullptr, 0);
        // get external port from QoS
        qosPort = mQosManager.mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().getPort();
        // determine whether UPnP is available or not (meaning; we successfully added a port mapping)
        #if !defined(EA_PLATFORM_XBOX_GDK)
        upnpStatus = (upnpStatusFlags & PROTOUPNP_STATUS_ADDPORTMAP) ? UPNP_ENABLED : UPNP_FOUND;
        #else // for gemini we don't add our own mapping, we just look and see if microsoft did
        upnpStatus = (upnpStatusFlags & PROTOUPNP_STATUS_FNDPORTMAP) ? UPNP_ENABLED : UPNP_FOUND;
        #endif

        // check UPnP external address vs QoS external address to determine if we are double-NATted
        if ((upnpStatusFlags & PROTOUPNP_STATUS_GOTEXTADDR) && (getExternalAddress()->getIp() != wanIpAddr))
        {
            request.getBlazeFlags().setDoubleNat();
        }

        // if UPnP is enabled (the add port mapping operation was successful) and this is the only NAT we are dealing with
        if ((upnpStatus == UPNP_ENABLED) && (request.getBlazeFlags().getDoubleNat() == false))
        {
            Blaze::Util::NatType natType = getNetworkQosData()->getNatType();

            // if UPnP succeeded, QoS classified NAT as moderate, we promote the NAT type to open
            if (natType == Blaze::Util::NAT_TYPE_MODERATE)
            {
                BLAZE_SDK_DEBUGF("ConnectionManager: promoting NAT type from MODERATE to OPEN due to successful UPnP port mapping\n");
                request.getBlazeFlags().setNatPromoted();
                natType = Blaze::Util::NAT_TYPE_OPEN;
            }

            /* In the event UPnP finishes successfully after *QoS* (this is believed to be unlikely due to the fact that UPnP typically
               is a faster process than QoS and starts earlier), it is possible that QoS will detect a non-symmetric (translated) external
               port that will not subsequently be valid.  If we are not double-NATted we can detect this situation by noting the mismatch
               between external and internal ports coupled with the success of UPnP port mapping, and override the QoS port with the UPnP
               port to correct the issue.  In addition, we note this operation with a bit flag, so we can determine if this event is
               actually occurring in the production environment.

               (It is also possible we might see this flag if the NAT tells us that UPnP succeeded, but it did not.  This seems unlikely,
               although possible if the UPnP implementation in a given NAT is broken).

               $$TODO$$ - re-evaluate this process once we have data from production to indicate whether this event is occurring
               or not. */
            if (wanPort != qosPort)
            {
                BLAZE_SDK_DEBUGF("ConnectionManager: overriding QoS external port (%d) with UPnP external port (%d)\n", qosPort, wanPort);
                request.getBlazeFlags().setPortOverride();
            }

            // update nat type and external port, as specified
            mQosManager.updateNatInfoFromUpnp(natType, wanPort);
        }

        // send upnp metrics update
        request.setDeviceInfo(deviceName);
        request.setLastRsltCode(ProtoUpnpStatus(mProtoUpnp, 'lerr', nullptr, 0));
        request.setNatType((uint16_t)getNetworkQosData()->getNatType());
        request.setStatus(upnpStatus);
        request.setFlags(upnpStatusFlags);
        mBlazeHub.getComponentManager()->getUtilComponent()->setClientMetrics(request, Util::UtilComponent::SetClientMetricsCb());

        // clean up upnp module
        ProtoUpnpDestroy(mProtoUpnp);
        mProtoUpnp = nullptr;
        mUpnpInfoDiscovered = true;
    }
    else
        mUpnpJobId = mBlazeHub.getScheduler()->scheduleMethod("retrieveUpnpStatus", this, &ConnectionManager::retrieveUpnpStatus, this, 1000);
}

bool ConnectionManager::initialQosPingSiteLatencyRetrieved() const
{
    return mQosManager.initialPingSiteLatencyRetrieved();
}

void ConnectionManager::internalQosPingSiteLatencyRequestedCb()
{
    mTimeOfLastQosRefresh = EA::TDF::TimeValue::getTimeOfDay();
    mDispatcher.dispatch("onQosPingSiteLatencyRequested", &ConnectionManagerListener::onQosPingSiteLatencyRequested);
}

void ConnectionManager::internalQosPingSiteLatencyRetrievedCb(bool success)
{
    if (!success)
        mTimeOfLastQosRefreshFailure = EA::TDF::TimeValue::getTimeOfDay();
    else 
        mTimeOfLastQosRefreshFailure = 0;    // 0 - indicates that no further refresh is needed

    mDispatcher.dispatch("onQosPingSiteLatencyRetrieved", &ConnectionManagerListener::onQosPingSiteLatencyRetrieved, success);
}


bool ConnectionManager::refreshQosManager()
{
    // Check if the users are all in valid states: 
    if (mBlazeHub.areUsersInActiveSession())
    {
        return false;
    }

    // If the state is okay, then we can reset the QoS manager and it'll work out:
    mQosManager.initialize(nullptr, QosManager::QosPingSiteLatencyRequestedCb(this, &ConnectionManager::internalQosPingSiteLatencyRequestedCb),
                                 QosManager::QosPingSiteLatencyRetrievedCb(this, &ConnectionManager::internalQosPingSiteLatencyRetrievedCb));
    return true;
}

bool ConnectionManager::refreshQosPingSiteLatency(bool bLatencyOnly)
{ 
    return mQosManager.refreshQosPingSiteLatency(QosManager::QosPingSiteLatencyRequestedCb(this, &ConnectionManager::internalQosPingSiteLatencyRequestedCb),
                                                 QosManager::QosPingSiteLatencyRetrievedCb(this, &ConnectionManager::internalQosPingSiteLatencyRetrievedCb), bLatencyOnly);
}

void ConnectionManager::resetQosManager(const QosConfigInfo& qosConfigInfo)
{
    mQosManager.initialize(&qosConfigInfo, QosManager::QosPingSiteLatencyRequestedCb(this, &ConnectionManager::internalQosPingSiteLatencyRequestedCb),
                                           QosManager::QosPingSiteLatencyRetrievedCb(this, &ConnectionManager::internalQosPingSiteLatencyRetrievedCb), true);
}

void ConnectionManager::enableQosReinitialization()
{
    mQosManager.enableReinitialization(QosManager::QosPingSiteLatencyRequestedCb(this, &ConnectionManager::internalQosPingSiteLatencyRequestedCb),
                                       QosManager::QosPingSiteLatencyRetrievedCb(this, &ConnectionManager::internalQosPingSiteLatencyRetrievedCb));
}

void ConnectionManager::getValidVersionList(const char8_t* versionStr, uint32_t* version, uint32_t length) const
{
    const char8_t* origin = versionStr;
    const char8_t* nextPos;
    for (uint32_t i = 0; i < length; ++i)
    {
        nextPos = blaze_str2int(origin, &version[i]);

        // server version string started with non-numeric or '.'
        while(origin == nextPos)
        {
            origin++;
            nextPos = blaze_str2int(origin, &version[i]);
        }

        if (nextPos != origin)
        {
            origin = nextPos;
        }
    }
    return;
}

bool ConnectionManager::isOlderBlazeServerVersion(const char8_t* blazeSdkVersion, const char8_t* serverVersion) const
{
    uint32_t blazeSdk[4];
    uint32_t server[4];
    //"3.7.0.0 (mainline)"
    getValidVersionList(blazeSdkVersion, blazeSdk, 4);
    //"Blaze 3.06.00.00-development-HEAD (CL# unknown)"
    getValidVersionList(serverVersion, server, 4);

    for (uint32_t i = 0; i < 4; ++i)
    {
        if (blazeSdk[i] > server[i])
            return true;
        else if (blazeSdk[i] == server[i])
        {
            continue;
        }
        else
            return false;
    }

    //the same as
    return false;
}

} // Connection
} // Blaze
