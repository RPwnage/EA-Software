/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_STRESSMODULE_H
#define BLAZE_STRESS_STRESSMODULE_H

/*** Include files *******************************************************************************/

#include "framework/blazedefines.h"
#include "framework/callback.h"
#include "framework/tdf/userdefines.h"
#include "EASTL/hash_map.h"
#include "EASTL/string.h"
#include "loginmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigBootUtil;
class ConfigMap;

namespace Stress
{

class StressInstance;
class StressConnection;
//class Blaze::Stress::Login;

class StressModule
{
public:
    StressModule();

    // Perform any necessary configuration prior to starting the stress test
    virtual bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil = NULL);

    // Called by the core system to create stress instances for this module
    virtual StressInstance* createInstance(StressConnection* connection, Login* login) = 0;

    virtual ~StressModule();

    // Called when the stress tester determines how many connections the module will use out of the total pool
    virtual void setConnectionDistribution(uint32_t conns) {mTotalConnections = conns;}
    virtual uint32_t getTotalConnections() const {return mTotalConnections;}
    virtual const char8_t* getModuleName() = 0;

    bool isStressLogin() const { return mIsStressLogin; }
    uint32_t getInitialDelay() const { return mInitialDelay; }
    uint32_t getDelay() const { return mDelay; }
    uint32_t getExpectedTxnsPerPeriod() const { return mExpectedTxnsPerPeriod; }
    uint32_t getPendingLogins() const { return mPendingLogins; }
    uint32_t getSucceededLogins() const { return mSucceededLogins; }
    uint32_t getFailedLogins() const { return mFailedLogins; }
    uint32_t getSucceededLogouts() const { return mSucceededLogouts; }
    uint32_t getFailedLogouts() const { return mFailedLogouts; }
    uint32_t getSucceededResumeSessionAttempts() const { return mSucceededResumeSessionAttempts; }
    uint32_t getFailedResumeSessionAttempts() const { return mFailedResumeSessionAttempts; }
    bool shouldLogin() const { return mLogin; }
    bool shouldLogout() const { return mLogout; }
    int32_t getLogoutChance() const { return mLogoutChance; }
    int32_t getDisconnectChance() const { return mDisconnectChance; }
    int32_t getNumOfTrials() const { return mNumOfTrials; }
    int32_t getNumOfExecsPerTrial() const { return mNumOfExecsPerTrial; }
    uint32_t getDelayPerTrial() const { return mDelayPerTrial; }
    bool getShouldWaitActive() const { return mWaitActive; }
    bool getReconnect() const { return mReconnect; }
    bool getResumeSession() const { return mResumeSession; }
    uint32_t getLocale() const { return mLocale; }
    ClientType getClientType() const { return mClientType; }
    uint32_t getDumpStatsPeriod() const { return mDumpStatsPeriod_sec; }
    uint32_t getPingPeriod() const { return mPingPeriod_ms; }
    uint32_t getInactivityTimeout() const { return mInactivityTimeout_ms; }
    uint32_t getResumeSessionDelay() const { return mResumeSessionDelay_ms; }
    bool getUseServerQosSettings() const { return mUseServerQosSettings; }
    Blaze::ClientPlatformType getPlatform() const { return mPlatform; }

    void addTransactionTime(const EA::TDF::TimeValue& ms);

    void incPendingConnections() { ++mPendingConnections;}
    void incSucceededConnections() { --mPendingConnections; ++mSucceededConnections; }
    void incFailedConnections() { --mPendingConnections; ++mFailedConnections; }
    void incDisconnectConnections() { ++mDisconnectConnections; }
    void incActiveInstanceCount() { ++mActiveInstances; }
    void incInstancesResumingSession() { ++mInstancesResumingSession; }
    void incPendingLogins() { ++mPendingLogins;}
    void incSucceededLogins() { ++mSucceededLogins;}
    void incFailedLogins() { ++mFailedLogins;}
    void incSucceededResumeSessionAttempts() { ++mSucceededResumeSessionAttempts;}
    void incFailedResumeSessionAttempts() { ++mFailedResumeSessionAttempts;}
    void incTotalInstances() { ++mTotalInstances; }
    void incErrCount() { ++mTotalErrCount; ++mPeriodErrCount; }
    void waitAllActive();

    //on logout
    void decActiveInstanceCount() { --mActiveInstances; }
    void decInstancesResumingSession() { --mInstancesResumingSession; }
    void incSucceededLogouts() { ++mSucceededLogouts;}
    void incFailedLogouts() { ++mFailedLogouts;}

    uint32_t getTotalInstances() const { return mTotalInstances; }

    virtual bool saveStats() {return false;}

    int64_t getTotalTestTimeInMillis() const {
        return EA::TDF::TimeValue::getTimeOfDay().getMillis() - mTestStartTime/1000;
    }
    double getTotalTestTime() const {
        return (EA::TDF::TimeValue::getTimeOfDay().getMicroSeconds() - mTestStartTime) * 1E-6;
    }

    void dumpStats(bool reschedule);

    const GeoLocationData* getGeoLocationSample() const;

private:
    bool loadGeoIpSamples();

protected:
    int64_t mTestStartTime;
    int64_t mPeriodStartTime;
    
    uint64_t mTotalTransactionTime;
    uint64_t mTotalTransactionCount;
    uint64_t mTotalErrCount;
    
    uint64_t mPeriodTransactionTime;
    uint64_t mPeriodTransactionCount;
    uint64_t mPeriodErrCount;
    
    uint32_t mPendingConnections;
    uint32_t mSucceededConnections;
    uint32_t mFailedConnections;
    uint32_t mDisconnectConnections;
    
    uint32_t mPendingLogins;
    uint32_t mSucceededLogins;
    uint32_t mFailedLogins;

    uint32_t mSucceededLogouts;
    uint32_t mFailedLogouts;

    uint32_t mSucceededResumeSessionAttempts;
    uint32_t mFailedResumeSessionAttempts;

    uint32_t mActiveInstances;
    uint32_t mInstancesResumingSession;
    uint32_t mTotalInstances;
    uint32_t mTotalConnections;
    bool mIsStressLogin;

    uint32_t mInitialDelay; // Number of millis between start and first action executed.
    uint32_t mDelay; // Number of millis between issuing a commands
    uint32_t mExpectedTxnsPerPeriod; // Calculated when mDelay is defined
    bool mLogin;     // Should the module login first?
    bool mLogout;     // Should the module logout after login?
    int32_t mLogoutChance; // percentage chance for instance to logout after login
    int32_t mDisconnectChance; // percentage chance for instance to disconnect (without attempting to resume the session) after login (may or may not have logged out first)
    bool mWaitActive; // Should the module wait until all of its stress instances are active before starting a trial?
    bool mReconnect;   // Should the module keep trying to connect if the initial attempt fails?
    bool mResumeSession; // Should the module attempt to resume the session after certain kinds of disconnects, mimicking BlazeSDK?
                         // If true, the Fire2 protocol must be configured, pings must be enabled, and mPingPeriod_ms should be reasonably less than mInactivityTimeout_ms
    int32_t mNumOfTrials; // Number of logins
    int32_t mNumOfExecsPerTrial; // Number of executions of commands per login
    uint32_t mDelayPerTrial; // sleep number of millis between logouts
    uint32_t mLocale;
    ClientType mClientType;

    const char* mOverrideUserGeoIpSampleDbFilename;
    eastl::vector<GeoLocationData*> mUserGeoIpSamples;

    TimerId mTimerId;

    // The stats log file
    FILE*   mLogFile;

    uint32_t mDumpStatsPeriod_sec; // Time between logging stat dumps in seconds.
    uint32_t mPingPeriod_ms; // Time between sending a heartbeat ping to the server for each stress client (0 to disable pings)
    uint32_t mInactivityTimeout_ms; // If the stress client has not received anything from the server for this length of time,
                                   // the stress client will disconnect and, if mResumeSession is true, attempt to resume the session.
                                   // If mInactivityTimeout_ms is 0, the inactivity timeout is ignored.
    uint32_t mResumeSessionDelay_ms;  // The minimum delay between attempts to resume a session

    bool mUseServerQosSettings; // If true, the stress client will simulate latencies for the ping sites provided in the blazeserver
                                // qos settings config. The client will send an updateNetworkInfo request on login and when the blazeserver's
                                // qos settings are reconfigured.

    Blaze::ClientPlatformType mPlatform;//for testing platform specific client flow
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_STRESSMODULE_H

