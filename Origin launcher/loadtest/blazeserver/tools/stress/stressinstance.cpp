/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_map.h"
#include "framework/util/random.h"
#include "framework/util/locales.h"
#include "framework/protocol/shared/heat2decoder.h"

#include "stressinstance.h"
#include "stressmodule.h"
#include "loginmanager.h"
#include "stress.h"

#include <math.h>

namespace Blaze
{
namespace Stress
{

const uint32_t MAX_REDIRECTOR_RETRY_ATTEMPT = 3;

StressInstance::StressInstance(StressModule *owner, StressConnection* connection, Login* login, int32_t logCategory, bool warnDisconnect)
    : 
#ifdef TARGET_util
      mUtilSlave(BLAZE_NEW Blaze::Util::UtilSlave(*connection)),
#endif
	  mOwner(owner),
      mConnection(connection),
      mLogin(login),
      mLogCategory(logCategory),
      mShutdown(false),
      mLoggedIn(false),
      mWarnDisconnect(warnDisconnect),
      mLastRun(0),
      mTrialCounter(0),
      mExecCounter(0),
      mPingTimer(INVALID_TIMER_ID),
      mNextSleepDelay(owner->getDelay()),
      mWaitForResumeSession(false),
      mPingOutstanding(false),
      mUserSessionsSlave(NULL),
      mUpdateNetworkInfoTimer(INVALID_TIMER_ID)
{ 
    mConnection->setConnectionHandler(this);
    mConnection->setClientType(owner->getClientType());
    mConnection->setResumeSession(owner->getResumeSession());
    mConnection->setResumeSessionDelay(owner->getResumeSessionDelay());

    if (getOwner()->getUseServerQosSettings())
        mConnection->addBaseAsyncHandler(UserSessionsSlave::COMPONENT_ID, UserSessionsSlave::NOTIFY_NOTIFYQOSSETTINGSUPDATED, this);
}

StressInstance::~StressInstance()
{
    if (getOwner()->getUseServerQosSettings())
        mConnection->removeBaseAsyncHandler(UserSessionsSlave::COMPONENT_ID, UserSessionsSlave::NOTIFY_NOTIFYQOSSETTINGSUPDATED, this);

    mConnection->setConnectionHandler(NULL);
#ifdef TARGET_util
    delete mUtilSlave;
#endif
    if (mUserSessionsSlave != NULL)
        delete mUserSessionsSlave;
}

void StressInstance::onAsyncBase(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (component == UserSessionsSlave::COMPONENT_ID && type == UserSessionsSlave::NOTIFY_NOTIFYQOSSETTINGSUPDATED)
    {
        Heat2Decoder decoder;
        QosConfigInfo qosConfig;
        if (decoder.decode(*payload, qosConfig) == Blaze::ERR_OK)
            handleQosSettingsUpdate(qosConfig);
        else
            BLAZE_ERR_LOG(Log::SYSTEM, "[" << getIdent() << "] Failed to decode NotifyQosSettingsUpdated notification!");
    }
}

UserSessionsSlave* StressInstance::getUserSessionsSlave()
{
    if (mUserSessionsSlave == NULL)
        mUserSessionsSlave = BLAZE_NEW UserSessionsSlave(*mConnection);
    return mUserSessionsSlave;
}

void StressInstance::handleQosSettingsUpdate(QosConfigInfo& qosConfig)
{
    mPingSites.clear();
    for (PingSiteInfoByAliasMap::const_iterator itr = qosConfig.getPingSiteInfoByAliasMap().begin(); itr != qosConfig.getPingSiteInfoByAliasMap().end(); ++itr)
        mPingSites.push_back(itr->first.c_str());

    onQosSettingsUpdated();
    if (isLoggedIn())
        updateNetworkInfo();
}

void StressInstance::start()
{
	mShutdown = false;	
    Fiber::CreateParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &StressInstance::setupForDoTask, getOwner()->getModuleName(), params);
}

void StressInstance::shutdown()
{
    if (mPingTimer != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mPingTimer);
        mPingTimer = INVALID_TIMER_ID;
    }

    if (mUpdateNetworkInfoTimer != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mUpdateNetworkInfoTimer);
        mUpdateNetworkInfoTimer = INVALID_TIMER_ID;
    }

    // If we are still connected and/or waiting for the session to be resumed, 
    // call disconnect() to close the connection and stop attempting to resume the session
    if (mWaitForResumeSession || mConnection->connected())
    {
        mConnection->disconnect();
    }
    mShutdown = true;
}

void StressInstance::wakeFromSleep()
{
    if (mSleepEvent.isValid())
        Fiber::signal(mSleepEvent, ERR_OK);
}

void StressInstance::preAuth()
{
#ifdef TARGET_util
    Util::PreAuthRequest request;
    Util::PreAuthResponse response;
    // set locale
    request.getClientData().setLocale(mOwner->getLocale());
    request.getClientData().setServiceName(getLogin()->getOwner()->getServiceName());
    request.getClientData().setClientType(mOwner->getClientType());
    request.getFetchClientConfig().setConfigSection(getLogin()->getOwner()->getPreAuthClientConfig());
    BlazeRpcError rc = mUtilSlave->preAuth(request, response);
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::SYSTEM,
            "[" << getIdent() << "] [StressInstance].preAuth: failed to preAuthorize, err: "
            << ErrorHelp::getErrorName(rc));
    }
    else if (getOwner()->getUseServerQosSettings())
    {
        handleQosSettingsUpdate(response.getQosSettings());
    }
#else
    BLAZE_WARN_LOG(Log::SYSTEM, "[" << getIdent() << "] " << "[StressInstance]: Cannot perform preAuth with util excluded.");
#endif
}

BlazeRpcError StressInstance::doHeartBeat()
{
	Util::PingResponse pingResp;
	return(mUtilSlave->ping(pingResp));
}

void StressInstance::sleep(uint32_t timeInMs)
{
    BlazeRpcError err = gSelector->sleep(timeInMs * 1000, (TimerId*)NULL, &mSleepEvent);
    mSleepEvent.reset();
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[" << getIdent() << "] " << "[StressInstance]: failed to sleep.");
    }
}

void StressInstance::heartbeatPing()
{
#ifdef TARGET_util
    BlazeRpcError rc;
    TimeValue now = TimeValue::getTimeOfDay();
    
    if (getOwner()->getInactivityTimeout() > 0)
    {
        TimeValue lastActivity = eastl::max(TimeValue(mConnection->getLastReadTimestamp()), TimeValue(mConnection->getLastConnectionAttemptTimestamp()));
        TimeValue timeSinceLastActivity= (now.getMicroSeconds() - lastActivity.getMicroSeconds());
        TimeValue inactivityTimeout = getOwner()->getInactivityTimeout()*1000;

        if (timeSinceLastActivity >= inactivityTimeout)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[" << getIdent() << "] " << "[StressInstance]: Inactivity timeout expired; disconnecting (resumable error).");
            mConnection->disconnect(getOwner()->getResumeSession());
            return;
        }
    }

    TimeValue pingPeriod = getOwner()->getPingPeriod()*1000;
    TimeValue afterPingPeriod = now + pingPeriod;
    TimeValue nextPingTime = afterPingPeriod;
    TimeValue timeSinceLastRpc = (now.getMicroSeconds() - mConnection->getLastRpcTimestamp());
    bool doPing = false;
    if (timeSinceLastRpc >= pingPeriod)
    {
        doPing = true;
    }
    else
    {
        // set next ping to occur pingPeriod after the last issued RPC
        nextPingTime = (mConnection->getLastRpcTimestamp() + pingPeriod.getMicroSeconds());
    }

    // Schedule the next ping before sending the current one. This ensures that if we fail to send
    // the current ping, select() will exit when the next ping is due (at latest)
    // even if no network event has occurred.
    mPingTimer = gSelector->scheduleFiberTimerCall(nextPingTime, this, &StressInstance::heartbeatPing, 
        "StressInstance::heartbeatPing", Fiber::CreateParams(Fiber::STACK_LARGE));

    // Ping the server, but only if we've completed the last ping
    if (doPing && !mPingOutstanding)
    {
        Util::PingResponse pingResp;
        mPingOutstanding = true;
        rc = mUtilSlave->ping(pingResp);
        mPingOutstanding = false;

        if (rc != ERR_OK)
        {
            if(!mConnection->connected())
            {
                BLAZE_INFO_LOG(Log::SYSTEM, 
                    "[" << getIdent() << "] [StressInstance].heartbeatPing: Server ping heartbeat stopped.");
                return;
            }
            else
            {
                BLAZE_WARN_LOG(Log::SYSTEM,
                    "[" << getIdent() << "] [StressInstance].heartbeatPing: Server ping heartbeat returned err: "
                    << ErrorHelp::getErrorName(rc));
            }
        }
        now = TimeValue::getTimeOfDay();
        if (afterPingPeriod < now)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[" << getIdent() << "] [StressInstance].heartbeatPing: Ping heartbeat was delayed by " 
                << (now - afterPingPeriod).getMicroSeconds() << "us.");
        }
    }

#else
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getIdent() << "] [StressInstance].heartbeatPing: Cannot perform heartbeatPing with util excluded.");
#endif
}

void StressInstance::waitForResumeSession()
{
    mWaitForResumeSession = true;
    mOwner->decActiveInstanceCount();
    mOwner->incInstancesResumingSession();

    if (mPingTimer != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mPingTimer);
        mPingTimer = INVALID_TIMER_ID;
    }
}

void StressInstance::setupForDoTask()
{
    char8_t addrStr[256];

    getOwner()->incPendingConnections();

    uint32_t curRetryCount = 0;
    bool reconnect = getOwner()->getReconnect();
    while (!mConnection->connect(reconnect) && curRetryCount < MAX_REDIRECTOR_RETRY_ATTEMPT)
    {
        curRetryCount++;
        if (!reconnect)
        {
            getOwner()->incFailedConnections();
            BLAZE_ERR_LOG(Log::SYSTEM, "[" << getIdent() << "] " << "[StressInstance:" << this << "]: failed to connect to " << 
                mConnection->getAddressString(addrStr, sizeof(addrStr)) << ".");
            return;
        }
        
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << getIdent() << "] " << "[StressInstance:" << this << "]: retry connect to " << 
            mConnection->getAddressString(addrStr, sizeof(addrStr)) << " in 10s.");
            
        sleep(10000);
    }

    BLAZE_TRACE_LOG(Log::SYSTEM, "[" << getIdent() << "] " << "[StressInstance:" << this << "]: connected to " << 
        mConnection->getAddressString(addrStr, sizeof(addrStr)));

    getOwner()->incSucceededConnections();

    // schedule pings
    if (getOwner()->getPingPeriod() > 0)
    {
        Fiber::CreateParams params(Fiber::STACK_SMALL);
        gSelector->scheduleFiberCall(this, &StressInstance::heartbeatPing, "StressInstance::heartbeatPing", params);
    }

    if (mOwner->shouldLogin())
        preAuth();

    mLoggedIn = false;

    //  instance is active.
    mOwner->incActiveInstanceCount();
    mTrialCounter = mOwner->getNumOfTrials();
    mExecCounter = mOwner->getNumOfExecsPerTrial();

    Fiber::CreateParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &StressInstance::doTrialBegin, getOwner()->getModuleName(), params);
}

void StressInstance::doTrialBegin()
{
    if (mWaitForResumeSession)
        return;

    if(mTrialCounter != 0 && !mShutdown)
    {
        mExecCounter = mOwner->getNumOfExecsPerTrial();
    
        if (mOwner->shouldLogin() && !mLoggedIn)
        {
            login();
        }

        if (mOwner->getShouldWaitActive())
            mOwner->waitAllActive();

        //  delay trial until initial delay has passed from start of owner module.
        uint32_t initialDelayTime = mOwner->getInitialDelay();
        if (initialDelayTime > 0)
        {
            sleep(initialDelayTime);
        }

        doExec();
    }
    else
    {
        mOwner->decActiveInstanceCount();
        shutdown();
		reconnectHandler();
    }

}

void StressInstance::doExec()
{
    if (mWaitForResumeSession)
        return;

    if (mExecCounter != 0)
    {
        TimeValue startTime = TimeValue::getTimeOfDay();
        BlazeRpcError err = execute();
        TimeValue endTime = TimeValue::getTimeOfDay();
        mOwner->addTransactionTime(endTime - startTime);
        mLastRun = startTime; 
        
        if (err != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(mLogCategory, "[" << getIdent() << "] Action: Err:" << (ErrorHelp::getErrorName(err)));
            mOwner->incErrCount();
        }
    
        if(mOwner->getDelay() > 0)
        {
            // delay the instance with average delay as specified in config file
            // this should keep the actions' starts distributed evenly 
            // within twice the delay period 
            uint32_t execTimeMs = static_cast<uint32_t>((endTime - startTime).getMillis());
            uint32_t allowedDelay = mOwner->getDelay() - execTimeMs;
            if (mNextSleepDelay < allowedDelay)
                allowedDelay = mNextSleepDelay;
            mNextSleepDelay = mOwner->getDelay();
            if (allowedDelay > 0)
            {
                // if time execution was less than what was predicted to be average time
                // between two executions then wait 
                //int instDelay = Random::getRandomNumber(allowedDelay);
                sleep(allowedDelay);
            }
        }

        if(mExecCounter > 0){
            --mExecCounter;
        }

        if (mExecCounter != 0)
        {
            Fiber::CreateParams params(Fiber::STACK_SMALL);
            gSelector->scheduleFiberCall(this, &StressInstance::doExec, getOwner()->getModuleName(), params);
            return;
        }
    }

    bool delay = false;
    if (mOwner->shouldLogout() && Random::getRandomNumber(100) < mOwner->getLogoutChance())
    {
        logout();
        delay = true;
    }
    if (mOwner->getDisconnectChance() > 0 && Random::getRandomNumber(100) < mOwner->getDisconnectChance())
    {
        mConnection->disconnect();
        delay = true;
    }

    if (delay && mOwner->getDelayPerTrial() > 0)
    {
        sleep(mOwner->getDelayPerTrial());
    }

    if(mTrialCounter > 0){
        --mTrialCounter;
    }
    Fiber::CreateParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &StressInstance::doTrialBegin, getOwner()->getModuleName(), params);
}

void StressInstance::onDisconnected(StressConnection* conn, bool resumeSession)
{
    if (mWarnDisconnect)
    {
        BLAZE_WARN_LOG(mLogCategory, "[" << getIdent() << " Disconnected");
    }
    else
    {
        BLAZE_TRACE_LOG(mLogCategory, "[" << getIdent() << " Disconnected");
    }
    getOwner()->incDisconnectConnections();

    if (resumeSession)
    {
        waitForResumeSession();
    }
    else
    {
        mWaitForResumeSession = false;
    onDisconnected();
}
}

void StressInstance::onSessionResumed(StressConnection* conn, bool success)
{
    mWaitForResumeSession = false;
    if (success)
        mOwner->incSucceededResumeSessionAttempts();
    else
        mOwner->incFailedResumeSessionAttempts();

    mOwner->incActiveInstanceCount();
    mOwner->decInstancesResumingSession();

    Fiber::CreateParams params(Fiber::STACK_SMALL);
    // No need to check if pings are enabled; they must be if resumeSession functionality is in use
    gSelector->scheduleFiberCall(this, &StressInstance::heartbeatPing, "StressInstance::heartbeatPing", params);
    gSelector->scheduleFiberCall(this, &StressInstance::doTrialBegin, getOwner()->getModuleName(), params);
}

void StressInstance::onUserSessionMigrated(StressConnection* conn)
{
    if (mPingTimer != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mPingTimer);
        mPingTimer = INVALID_TIMER_ID;
    }

    Util::PingResponse pingResp;
    RpcCallOptions opts;
    opts.followMovedTo = false;
    BlazeRpcError rc = mUtilSlave->ping(pingResp, opts);
    if (rc != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM,
            "[" << getIdent() << "] [StressInstance].onUserSessionMigrated: Server ping returned err: "
            << ErrorHelp::getErrorName(rc));
    }
    else
    {
        mPingOutstanding = false; // reset flag to always send ping after migration
        BLAZE_INFO_LOG(Log::SYSTEM,
            "[" << getIdent() << "] [StressInstance].onUserSessionMigrated: Sent Ping during migration");
        Fiber::CreateParams params(Fiber::STACK_SMALL);
        gSelector->scheduleFiberCall(this, &StressInstance::heartbeatPing, "StressInstance::heartbeatPing", params);
    }
}

void StressInstance::login()
{
    BlazeRpcError err;
    uint32_t id = getIdent();
    mOwner->incPendingLogins();
    BLAZE_TRACE_LOG(mLogCategory, "[" << id << "]: Pending login, total(" << mOwner->getPendingLogins() << ")");
    // Module configured to require authentication so authenticate the user
    err = getLogin()->login();

    if (err == ERR_OK && getOwner()->getUseServerQosSettings())
        updateNetworkInfo();

    // notify derived object via virtual call
    onLogin(err);
    if(err == Blaze::ERR_OK)
    {
        mLoggedIn = true;
        mOwner->incSucceededLogins();
        BLAZE_TRACE_LOG(mLogCategory, "[" << id << "]: Succeeded login, total(" << mOwner->getSucceededLogins() << ")");
    }
    else
    {
        mOwner->incFailedLogins();
        BLAZE_ERR_LOG(mLogCategory, "[" << getIdent() << "]: Failed to login: " << (ErrorHelp::getErrorDescription(err)) << " (" 
                      << (ErrorHelp::getErrorName(err)) << "), total(" << mOwner->getFailedLogins() << ")");
        return;
    }
    
}

void StressInstance::logout()
{
    BlazeRpcError err;
    err = getLogin()->logout();
    // notify derived object via virtual call
    onLogout(err);
    if(err == Blaze::ERR_OK)
    {
        mOwner->incSucceededLogouts();
        BLAZE_TRACE_LOG(mLogCategory, "[" << getIdent() << "]: Succeeded logout, total(" << mOwner->getSucceededLogouts() << ")");
    }
    else
    {
        mOwner->incFailedLogouts();
        BLAZE_ERR_LOG(mLogCategory, "[" << getIdent() << "]: Failed to logout: " << (ErrorHelp::getErrorDescription(err)) << " (" 
                      << (ErrorHelp::getErrorName(err)) << "), total(" << mOwner->getFailedLogouts() << ")");
        return;
    }
    mLoggedIn = false;
    mConnection->resetSessionKey();
}

const char8_t* StressInstance::getRandomPingSiteAlias() const
{
    if (mPingSites.empty())
        return NULL;
    return mPingSites[Blaze::Random::getRandomNumber(mPingSites.size())].c_str();
}

void StressInstance::updateNetworkInfo()
{
    if (mUpdateNetworkInfoTimer == INVALID_TIMER_ID)
        mUpdateNetworkInfoTimer = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &StressInstance::updateNetworkInfoInternal, "StressInstance::updateNetworkInfo");
}

void StressInstance::updateNetworkInfoInternal()
{
    mUpdateNetworkInfoTimer = INVALID_TIMER_ID;

    const char *bestPingSite = getBestPingSiteAlias();
    if (bestPingSite == NULL)
        bestPingSite = getRandomPingSiteAlias();
    if (bestPingSite == NULL)
    {
        BLAZE_WARN_LOG(mLogCategory, "[" << getIdent() << "]: updateNetworkInfo failed: No ping sites configured");
        return;
    }

    UpdateNetworkInfoRequest request;
    request.getOpts().clearNetworkAddressOnly();
    request.getOpts().clearNatInfoOnly();
    request.getOpts().setUpdateMetrics();

    NetworkInfo& networkInfo = request.getNetworkInfo();
    for (size_t i = 0; i < mPingSites.size(); i++)
        networkInfo.getPingSiteLatencyByAliasMap()[mPingSites[i].c_str()] = 9999;

    networkInfo.getPingSiteLatencyByAliasMap()[bestPingSite] = 1;

    Util::NetworkQosData &qosData = networkInfo.getQosData();
    qosData.setDownstreamBitsPerSecond(16384 * 8);
    qosData.setUpstreamBitsPerSecond(8192 * 8);
    qosData.setNatType(Util::NAT_TYPE_OPEN);

    const BlazeRpcError blazeError = getUserSessionsSlave()->updateNetworkInfo(request);
    if (blazeError != Blaze::ERR_OK)
    {
        BLAZE_WARN_LOG(mLogCategory, "[" << getIdent() << "]: updateNetworkInfo failed. Error(" << (ErrorHelp::getErrorName(blazeError)) << ")");
    }
    else
    {
        BLAZE_INFO_LOG(mLogCategory, "[" << getIdent() << "]: updateNetworkInfo: Best ping site is: " << bestPingSite);
    }
}

StressModule::StressModule() :
    mTestStartTime(0),
    mPeriodStartTime(0),
    mTotalTransactionTime(0),
    mTotalTransactionCount(0),
    mTotalErrCount(0),
    mPeriodTransactionTime(0),
    mPeriodTransactionCount(0),
    mPeriodErrCount(0),
    mPendingConnections(0),
    mSucceededConnections(0),
    mFailedConnections(0),
    mDisconnectConnections(0),
    mPendingLogins(0),
    mSucceededLogins(0),
    mFailedLogins(0),
    mSucceededLogouts(0),
    mFailedLogouts(0),
    mActiveInstances(0),
    mInstancesResumingSession(0),
    mTotalInstances(0),
    mIsStressLogin(false),
    mInitialDelay(0),
    mDelay(0),
    mExpectedTxnsPerPeriod(0),
    mLogin(false),
    mLogout(false),
    mLogoutChance(0),
    mDisconnectChance(0),
    mWaitActive(false),
    mReconnect(false),
    mResumeSession(false),
    mNumOfTrials(-1),
    mNumOfExecsPerTrial(-1),
    mDelayPerTrial(0),
    mTimerId(INVALID_TIMER_ID),
    mDumpStatsPeriod_sec(DEFAULT_DUMP_STATS_INTERVAL_SECONDS),
    mPingPeriod_ms(DEFAULT_PING_PERIOD_IN_MS),
    mInactivityTimeout_ms(DEFAULT_INACTIVITY_TIMEOUT_IN_MS),
    mResumeSessionDelay_ms(DEFAULT_RESUME_SESSION_DELAY_IN_MS),
    mUseServerQosSettings(false)
{
    mLogFile = NULL;
}

StressModule::~StressModule()
{
    gSelector->cancelTimer(mTimerId);

    // Close the logfile
    if (mLogFile != NULL) 
    {
        fclose(mLogFile);
        mLogFile = NULL;
    }
}

bool StressModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    mPingPeriod_ms = config.getUInt32("pingPeriod_ms", DEFAULT_PING_PERIOD_IN_MS);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mPingPeriod_ms << " millisecond ping period.");

    mInactivityTimeout_ms = config.getUInt32("inactivityTimeout_ms", DEFAULT_INACTIVITY_TIMEOUT_IN_MS);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mInactivityTimeout_ms << " millisecond inactivity timeout.");

    mResumeSession = config.getBool("resumeSession", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] resumeSession=" << (mResumeSession ? "true" : "false") << ".");

    if ( mInactivityTimeout_ms > 0 )
    {
        if( (mPingPeriod_ms == 0) || (mPingPeriod_ms > mInactivityTimeout_ms) )
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[" << getModuleName() << "] Invalid configuration: inactivity timeout is enabled, but pings are disabled or ping period is greater than the inactivity timeout.");
            return false;
        }
        #ifndef TARGET_util
            BLAZE_ERR_LOG(Log::SYSTEM, "[" << getModuleName() << "] Invalid configuration: inactivity timeout is enabled, but pings cannot be performed because util is excluded.");
            return false;
        #endif
    }

    mResumeSessionDelay_ms = config.getUInt32("resumeSessionDelay_ms", DEFAULT_RESUME_SESSION_DELAY_IN_MS);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mResumeSessionDelay_ms << " millisecond resume session delay.");

    mUseServerQosSettings = config.getBool("useServerQosSettings", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] useServerQosSettings=" << (mUseServerQosSettings ? "true" : "false") << ".");

    mDelay = config.getUInt32("delay", 0);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mDelay << " millisecond delay.");

    if (mDelay > 0)
    {
        mExpectedTxnsPerPeriod = 
            mTotalConnections * DEFAULT_DUMP_STATS_INTERVAL_SECONDS * 1000 / mDelay;
        BLAZE_INFO_LOG(Log::SYSTEM
            , "[" << getModuleName() << "] Configured to run " << mExpectedTxnsPerPeriod << " transactions per interval."
              " If the stress tool cannot satisfy this criterion * will be displayed"
              " next to number of transactions in last period."
            );
    }
    
    mIsStressLogin = config.getBool("stress_login", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] stress_login:" << (mIsStressLogin ? "true" : "false") << ".");

    mInitialDelay = config.getUInt32("initial-delay", 0);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mInitialDelay << " millisecond initial delay.");

    mLogin = config.getBool("login", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] login=" << (mLogin ? "true" : "false") << ".");

    mWaitActive = config.getBool("waitActive", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] waitActive=" << (mWaitActive ? "true" : "false") << ".");

    // should deprecate "logout" in favor of "logout_chance"
    mLogout = config.getBool("logout", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] logout=" << (mLogout ? "true" : "false") << ".");

    // default is 40 because it was the previous non-configurable hardcode value
    mLogoutChance = config.getInt32("logout_chance", 40);
    if (mLogout)
    {
        int32_t origLogoutChance = mLogoutChance;
        if (mLogoutChance < 1)
        {
            mLogoutChance = 1;
        }
        else if (mLogoutChance > 100)
        {
            mLogoutChance = 100;
        }
        if (origLogoutChance == mLogoutChance)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] logout_chance=" << mLogoutChance << "%.");
        }
        else
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] logout_chance=" << mLogoutChance << "%. (cropped from " << origLogoutChance << " to fit percent range)");
        }
    }
    else
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] logout_chance=" << mLogoutChance << "%. (unmodified and ignored due to logout=false)");
    }

    mDisconnectChance = config.getInt32("disconnect_chance", 0);
    int32_t origDisconnectChance = mDisconnectChance;
    if (mDisconnectChance < 0)
    {
        mDisconnectChance = 0;
    }
    else if (mDisconnectChance > 100)
    {
        mDisconnectChance = 100;
    }
    if (origDisconnectChance == mDisconnectChance)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] disconnect_chance=" << mDisconnectChance << "%.");
    }
    else
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] disconnect_chance=" << mDisconnectChance << "%. (cropped from " << origDisconnectChance << " to fit percent range)");
    }

    mReconnect = config.getBool("reconnect", false);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] reconnnect=" << (mReconnect ? "true" : "false") << ".");

    mNumOfTrials = config.getInt32("num_trials", -1);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] number of trials=" << mNumOfTrials << ".");

    mNumOfExecsPerTrial = config.getInt32("num_execs_per_trial", -1);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mNumOfExecsPerTrial << " number of executions per trial.");

    mDelayPerTrial = config.getUInt32("delay_per_trial", 0);
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] " << mDelayPerTrial << " millisecond delay per trial.");

    mTestStartTime = TimeValue::getTimeOfDay().getMicroSeconds();
    mPeriodStartTime = TimeValue::getTimeOfDay().getMicroSeconds();

    mDumpStatsPeriod_sec = config.getUInt32("dumpStatsPeriod_sec", DEFAULT_DUMP_STATS_INTERVAL_SECONDS);

    if (mDumpStatsPeriod_sec > 0) // value of 0 shuts off the dump stats
    {
        mTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay()
            + (mDumpStatsPeriod_sec * 1000 * 1000), this, &StressModule::dumpStats, true, "StressModule::dumpStats");
    }

    mLocale = LocaleTokenCreateFromString(config.getString("locale", "enUS"));
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] account locale = " << mLocale);
    
    const char8_t* clientTypeStr = config.getString("clientType", ClientTypeToString(CLIENT_TYPE_GAMEPLAY_USER));
    if (ParseClientType(clientTypeStr, mClientType))
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] client type = " << clientTypeStr);
    }
    else
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[" << getModuleName() << "] invalid client type = " 
            << clientTypeStr << " defaulting to = " << ClientTypeToString(CLIENT_TYPE_GAMEPLAY_USER));
        mClientType = CLIENT_TYPE_GAMEPLAY_USER;
    }

    if (mDumpStatsPeriod_sec > 0) // value of 0 shuts off the dump stats
    {
    uint32_t tyear, tmonth, tday, thour, tmin, tsec;
    TimeValue tnow = TimeValue::getTimeOfDay(); 
    tnow.getGmTimeComponents(tnow,&tyear,&tmonth,&tday,&thour,&tmin,&tsec);
    char8_t fName[256];
    blaze_snzprintf(fName, sizeof(fName), "%s/stats_%s_%02d%02d%02d%02d%02d%02d.csv",(Logger::getLogDir()[0] != '\0')?Logger::getLogDir():".",getModuleName(), tyear, tmonth, tday, thour, tmin, tsec );
    
    mLogFile = fopen(fName, "wt");
    if (mLogFile == NULL)
    {
        BLAZE_ERR_LOG(Log::SYSTEM,"[" << getModuleName() << "] Failed to create the stats log file("<<fName<<") in folder("<<Logger::getLogDir()<<")");
    }
    else
    {
        // Print the title
        fprintf(mLogFile, "LogTime, ModuleName, Succeeded Login, Active Instances, Total Instances, Total Txns, Txn/s, us/txn, Total Errors, Period txns, Period txn/s, Period us/txn, Period errors\n");
        fflush(mLogFile);
    }
	}

    return true;
}

void StressModule::dumpStats(bool reschedule)
{
    // may be overridden in <component>module to provide own printout.
    saveStats();

    uint64_t totalUsecPerTxn = mTotalTransactionCount ? mTotalTransactionTime / mTotalTransactionCount : 0ULL;
    uint64_t periodUsecPerTxn = mPeriodTransactionCount ? mPeriodTransactionTime / mPeriodTransactionCount : 0ULL;

    double totalTimeSec = getTotalTestTime();
    double periodTimeSec = (TimeValue::getTimeOfDay().getMicroSeconds() - mPeriodStartTime) * 1E-6;

    double txnPerSec = mTotalTransactionCount/totalTimeSec;
    double txnPerSecPeriod = mPeriodTransactionCount/periodTimeSec;


    BLAZE_INFO_LOG(Log::SYSTEM,
        "[" << getModuleName() << "] Connections [succeeded: " << mSucceededConnections << ", pending: " << mPendingConnections << ", failed: " 
        << mFailedConnections << ", disconnects: " << mDisconnectConnections << "]");

    if (mResumeSession)
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << getModuleName() << "] Resume Session Attempts [succeeded: " << mSucceededResumeSessionAttempts << ", pending: " << mInstancesResumingSession
            << ", failed: " << mFailedResumeSessionAttempts << "]");

    BLAZE_INFO_LOG(Log::SYSTEM,
        "[" << getModuleName() << "] Instances [active: " << mActiveInstances << ", total: " << mTotalInstances << "]");

    if (shouldLogin())
    {
        BLAZE_INFO_LOG(Log::SYSTEM,
            "[" << getModuleName() << "] Logins [succeeded: " << mSucceededLogins << ", failed: " << mFailedLogins << "]");
    }

    if (shouldLogout())
    {
        BLAZE_INFO_LOG(Log::SYSTEM,
            "[" << getModuleName() << "] Logouts [succeeded: " << mSucceededLogouts << ", failed: " << mFailedLogouts << "]");
    }

    BLAZE_INFO_LOG(Log::SYSTEM,
        "[" << getModuleName() << "] Period (" << mPeriodTransactionCount << (mPeriodTransactionCount < mExpectedTxnsPerPeriod ? "* " : " ") << "txns, "
        << mPeriodTransactionCount/periodTimeSec << "1f txns/s, " << periodUsecPerTxn << " us/txn, " << mPeriodErrCount << " errs)");

    BLAZE_INFO_LOG(Log::SYSTEM,
        "[" << getModuleName() << "] Total (" << mTotalTransactionCount << " txns, " << mTotalTransactionCount/totalTimeSec << "1f txns/s, " << totalUsecPerTxn 
        << " us/txn, " << mTotalErrCount << " errs)");

    TimeValue time = TimeValue::getTimeOfDay();
    char8_t timeStr[64];
    time.toString(timeStr, sizeof(timeStr));
    
    if (mLogFile) 
    {
    fprintf(mLogFile, 
        "%s, %s, "
        "%8d, %8d, %8d, "
        "%" PRIu64 ", %.2f, %" PRIu64 ", %" PRIu64 ", "
        "%" PRIu64 ", %.2f, %" PRIu64 ", %" PRIu64 "\n", 
        timeStr, getModuleName(),
        mSucceededLogins, mActiveInstances, mTotalInstances,
        mTotalTransactionCount, txnPerSec, totalUsecPerTxn, mTotalErrCount,
        mPeriodTransactionCount, txnPerSecPeriod, periodUsecPerTxn, mPeriodErrCount);

        fflush(mLogFile);
    }

        mPeriodTransactionTime = mPeriodTransactionCount = mPeriodErrCount = 0;
        mPeriodStartTime = TimeValue::getTimeOfDay().getMicroSeconds();
        
        if (reschedule)
        {
            mTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay()
                + (getDumpStatsPeriod() * 1000 * 1000), this, &StressModule::dumpStats, true, "StressModule::dumpStats");
        }
}

void StressModule::addTransactionTime(const TimeValue& ms)
{
    mTotalTransactionTime += ms.getMicroSeconds();
    mTotalTransactionCount++;
    mPeriodTransactionTime += ms.getMicroSeconds();
    mPeriodTransactionCount++;
}

void StressModule::waitAllActive()
{
    while (mTotalInstances > mActiveInstances)
    {
        BlazeRpcError err = gSelector->sleep(100);
        if (err != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[" << getModuleName() << "][StressModule:" << this << "].waitAllActive(): Got sleep error " << (ErrorHelp::getErrorName(err)) << "!");
        }
    }
}

} // Stress
} // Blaze

