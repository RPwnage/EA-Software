/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef STRESSINSTANCE_H
#define STRESSINSTANCE_H

/*** Include files *******************************************************************************/

#include "stressconnection.h"
#include "framework/rpc/usersessionsslave.h"
#ifdef TARGET_util
#include "util/rpc/utilslave.h"
#include "util/tdf/utiltypes.h"
#endif
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stress
{

#define DEFAULT_DUMP_STATS_INTERVAL_SECONDS 0 // by default stats dump is disabled it's mostly spam
#define DEFAULT_PING_PERIOD_IN_MS 10000
#define DEFAULT_INACTIVITY_TIMEOUT_IN_MS 0 // by default the inactivity timeout is disabled
#define DEFAULT_CPU_BUDGET_CHECK_INTERVAL_SECONDS 10

class StressModule;
class StressConnection;
class Login;

class StressInstance : protected StressConnection::ConnectionHandler, virtual public AsyncHandlerBase
{
    NON_COPYABLE(StressInstance);

public:
    ~StressInstance() override;

    // Get the unique id used to identify this instance
    int32_t getIdent() const { return mConnection->getIdent(); }

    BlazeId getBlazeId() const { return mConnection->getBlazeId(); }

    // Get a handle to the associated connection object
    StressConnection* getConnection() const { return mConnection; }

    // Get a handle to the associated UserSessionsSlave object.
    // Creates the UserSessionsSlave if it does not already exist.
    UserSessionsSlave* getUserSessionsSlave();

    // Get a handle to the associated login module (for doing authentication)
    Login* getLogin() const { return mLogin; }

    // This is the entry point for a stress instance to start running
    virtual void start();
    
    // Shut down the instance
    void shutdown();

    // return logged-in status
    bool isLoggedIn() const {return mConnection->isLoggedIn(); }

    // return shutdown status
    bool isShutdown() const {return mShutdown;}

    void login();
    void logout();

    void setCrossplayOptIn(bool enable);

    void sleep(uint32_t timInMs);

    StressModule* getOwner() const { return mOwner; }

    eastl::vector<eastl::string>& getPingSites() { return mPingSites; }
    const char8_t* getRandomPingSiteAlias() const;

protected:
    StressInstance(StressModule *owner, StressConnection* connection, Login* login, int32_t logCategory, bool warnDisconnect = true);

    void onAsyncBase(uint16_t component, uint16_t type, RawBuffer* payload) override;

    // Override these methods to track connection/disconnection events
    virtual void onDisconnected() { }
    virtual void onLogin(BlazeRpcError result) { }
    virtual void onLogout(BlazeRpcError result) { }

    // Override these methods to customize the best ping site alias sent in network info updates
    // and respond to ping site updates on the blazeserver. Called only if UseServerQosSettings is enabled.
    virtual const char8_t* getBestPingSiteAlias() { return nullptr; }
    virtual void onQosSettingsUpdated() { }

    void doTrialBegin();
    void doExec();
    void setupForDoTask();
    void preAuth();
    void wakeFromSleep();
    void setNextSleepDelay(uint32_t sleepMs) { mNextSleepDelay = (uint32_t) sleepMs; }

    void updateNetworkInfo();

    //Override this method to do your task
    virtual BlazeRpcError execute() = 0;

    virtual const char8_t *getName() const = 0;
private:
    StressModule *mOwner;
    StressConnection* mConnection;
#ifdef TARGET_util
    Util::UtilSlave *mUtilSlave;
#endif
    Login* mLogin;
    int32_t mLogCategory;
    bool mShutdown;
    bool mWarnDisconnect;
    TimeValue mLastRun;
    int32_t mTrialCounter;
    int32_t mExecCounter;
    TimerId mPingTimer;
    Fiber::EventHandle mSleepEvent;
    uint32_t mNextSleepDelay;

    bool mPingOutstanding;

    eastl::vector<eastl::string> mPingSites;
    UserSessionsSlave *mUserSessionsSlave;
    TimerId mUpdateNetworkInfoTimer;

    // StressConnection::ConnectionHandler
    void onDisconnected(StressConnection* conn) override;
    void onBeginUserSessionMigration() override;
    bool onUserSessionMigrated(StressConnection* conn) override;

    // Callback to ping the server and keep connected, and to
    // check whether the inactivity timeout has expired
    void heartbeatPing();
    void cancelHeartbeatPing();
    void scheduleHeartbeatPing(const char8_t* caller, EA::TDF::TimeValue nextPingTime = 0);

    void handleQosSettingsUpdate(QosConfigInfo& qosConfig);
    void updateNetworkInfoInternal();
};

struct StressLogContextOverride
{
public:
    StressLogContextOverride(int32_t ident, BlazeId blazeId)
    {
        if (gLogContext != nullptr)
        {
            mPreviousLogContext = *gLogContext;
            gLogContext->set(ident, blazeId);
        }
    }

    ~StressLogContextOverride()
    {
        if (gLogContext != nullptr)
            *gLogContext = mPreviousLogContext;
    }

private:
    LogContextWrapper mPreviousLogContext;
};

} // Stress
} // Blaze

#endif // STRESSINSTANCE_H

