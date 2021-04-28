///////////////////////////////////////////////////////////////////////////////
// \brief Service::GlobalConnectionStateMonitor.cpp
//
// owns and manages the global connection state variables (internet connection, sleep, update pending)
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
#include "SettingsManager.h"

namespace
{
    QString const SERVICE_DATA_KEY = "GlobalConnectionStateMonitor";
    QReadWriteLock readWriteLock;
    QPointer<Origin::Services::Network::GlobalConnectionStateMonitor> sInstance;
    QThread* networkDetectionThread = NULL;
    QTimer* networkDetectorTimer = NULL;
};

namespace Origin
{

namespace Services
{

namespace Network
{

void GlobalConnectionStateMonitor::init()
{
    ConnectionDetection::init();

    if (!sInstance.isNull())
        return;

    sInstance = new GlobalConnectionStateMonitor;
    
#if defined(ORIGIN_MAC)
    // On Mac the call to pollInternetReachabilityPriv() can block under certain situations
    // for example if the users ISP goes down it can take a while to return no connection
    // moving this to it's own thread will fix EBIBUGS-20146 and keep the main thread from blocking on
    // this function call
    networkDetectionThread = new QThread();
    networkDetectionThread->setObjectName("GlobalConnectionStateMonitor::NetworkDetectorTimer");
    ORIGIN_VERIFY_CONNECT(networkDetectionThread, SIGNAL(finished()), networkDetectionThread, SLOT(deleteLater()));
    sInstance->moveToThread( networkDetectionThread );
    networkDetectionThread->start();

    QMetaObject::invokeMethod( sInstance, "pollInternetReachabilityPriv", Qt::QueuedConnection );
#else
//  For now we are leaving the PC side alone, but this should probably also be moved to it's own thread.
    sInstance->pollInternetReachabilityPriv();
#endif
}

void GlobalConnectionStateMonitor::postSettingsInit()
{
    ORIGIN_ASSERT(Origin::Services::SettingsManager::instance());

    // Add the heartbeat URL as a fallback for checking online connectivity
    ConnectionDetection::instance()->AddURL(
        Origin::Services::readSetting(Origin::Services::SETTING_HeartbeatURL).toString().toUtf8().constData());
}

void GlobalConnectionStateMonitor::release()
{
    if (networkDetectionThread != NULL)
    {
        // this should only execute on the Mac since the network detection thread only executes there
        networkDetectionThread->exit();
        networkDetectionThread->wait();
    }
    
    delete sInstance.data();
    sInstance = NULL;
    ConnectionDetection::release();
}

GlobalConnectionStateMonitor* GlobalConnectionStateMonitor::instance()
{
    ORIGIN_ASSERT(sInstance.data());
    return sInstance.data();
}

GlobalConnectionStateMonitor::GlobalConnectionStateMonitor()
    :mIsRequiredUpdatePending (Connection::CS_FALSE)
    ,mIsComputerAwake(Connection::CS_TRUE)
    ,mIsInternetReachable (Connection::CS_TRUE)
    ,mSimulateIsInternetReachable (Connection::CS_TRUE)
{
    QSharedPointer<ConnectionDetection> networkDetector = ConnectionDetection::instance();
    ORIGIN_ASSERT(!networkDetector.isNull());
    networkDetector->SetListener(this);
    networkDetector->Init();

    /// the mNetworkDetector may or may not REQUIRE polling. However, as of 8.x 
    /// polling has always been enabled, and we stay with that choice, since
    /// it takes one variable out of the picture and the cost of polling is
    /// not great.
}

GlobalConnectionStateMonitor::~GlobalConnectionStateMonitor()
{
    cleanupNetworkDetectionTimer();
}

/// callback from the internet polling class
void GlobalConnectionStateMonitor::ConnectionChange(const ConnectionStatus& ics)
{
    setConnectionField(Connection::GLOB_OPR_IS_INTERNET_REACHABLE, ics.mbConnectedToInternet ? Connection::CS_TRUE : Connection::CS_FALSE);
}

Connection::ConnectionStatus GlobalConnectionStateMonitor::connectionFieldPriv(Connection::ConnectionStateField field)
{
    QReadLocker lock(&readWriteLock);
    switch ( field )
    {
    case Connection::GLOB_ADM_IS_REQUIRED_UPDATE_PENDING:
        return mIsRequiredUpdatePending;

    case Connection::GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE:
        return mSimulateIsInternetReachable;

    case Connection::GLOB_OPR_IS_INTERNET_REACHABLE:
        return mIsInternetReachable;

    case Connection::GLOB_OPR_IS_COMPUTER_AWAKE:
        return mIsComputerAwake;
        
    default:
        break;
    }
    return Connection::CS_FALSE;
}

void GlobalConnectionStateMonitor::setConnectionFieldPriv(Connection::ConnectionStateField field, Connection::ConnectionStatus status)
{

    bool changed = false;
    bool wasOnline = isOnline();
    QString fieldName;
    {
        QWriteLocker lock(&readWriteLock);
        switch ( field )
        {
        case Connection::GLOB_ADM_IS_REQUIRED_UPDATE_PENDING:
            fieldName = "GLOB_ADM_IS_REQUIRED_UPDATE_PENDING";
            if (mIsRequiredUpdatePending != status)
            {
                mIsRequiredUpdatePending = status;
                changed = true;
            }
            break;
        case Connection::GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE:
            fieldName = "GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE";
            if (mSimulateIsInternetReachable != status)
            {
                mSimulateIsInternetReachable = status;
                changed = true;
            }
            break;
        case Connection::GLOB_OPR_IS_INTERNET_REACHABLE:
            fieldName = "GLOB_OPR_IS_INTERNET_REACHABLE";
            if ( mIsInternetReachable != status)
            {
                mIsInternetReachable = status;
                changed = true;
            }
            break;
        case Connection::GLOB_OPR_IS_COMPUTER_AWAKE:
            fieldName = "GLOB_OPR_IS_COMPUTER_AWAKE";
            if (mIsComputerAwake != status)
            {
                mIsComputerAwake = status;
                changed = true;

                if (status == Connection::CS_TRUE)
                {
                    // assume that the network is down when we wake up, if this is inaccurate, 
                    // the heartbeat monitor will set it within 5 seconds, EBIBUGS-18589
                    mIsInternetReachable = Connection::CS_FALSE;
                }
            }
            break;
        default:
            break;
        }
    }
    if ( changed )
    {
        //  TELEMETRY:  Send lost network connection hook
        if (wasOnline && !isOnline())
        {
            GetTelemetryInterface()->Metric_APP_CONNECTION_LOST();
        }
        ORIGIN_LOG_EVENT << fieldName << " set to " << (status == Connection::CS_TRUE ? "true" : "false")
            << ", now " << (isOnline() ? "online" : "offline")
            << ", new state: " << stateString();
        emit connectionStateChanged(field);
    }
}

void GlobalConnectionStateMonitor::pollInternetReachabilityPriv()
{
    QSharedPointer<ConnectionDetection> networkDetector = ConnectionDetection::instance();
    ORIGIN_ASSERT(!networkDetector.isNull());

    if (!networkDetector.isNull())
    {
        networkDetector->Poll();
        // saw a problem where the changed flag on the state was false when it should have
        // been true, so we missed a state change, make the call here to make sure
        ConnectionChange(networkDetector->GetLastConnectionStatus());
    }

    if (networkDetectorTimer == NULL)
    {
        networkDetectorTimer = new QTimer();
        Q_ASSERT(networkDetectorTimer);
        if (networkDetectorTimer)
        {
            ORIGIN_VERIFY_CONNECT(networkDetectorTimer, SIGNAL(timeout()), sInstance, SLOT(pollInternetReachabilityPriv()));
            if (networkDetectionThread)
                ORIGIN_VERIFY_CONNECT(networkDetectionThread, SIGNAL(finished()), this, SLOT(cleanupNetworkDetectionTimer()));
            networkDetectorTimer->start(10*1000);   // 5 seconds seems to be too fast causing https://developer.origin.com/support/browse/EBIBUGS-24281
        }
    }
}

void GlobalConnectionStateMonitor::cleanupNetworkDetectionTimer()
{
    delete networkDetectorTimer;
    networkDetectorTimer = NULL;
}

QString GlobalConnectionStateMonitor::stateString()
{
    return 
        QString("RequiredUpdatePending=") + ((mIsRequiredUpdatePending == Connection::CS_TRUE) ? "true" : "false" ) +
        ",InternetReachable=" + ((mIsInternetReachable == Connection::CS_TRUE) ? "true" : "false" ) +
        ",SimulateInternetReachable=" + ( (mSimulateIsInternetReachable == Connection::CS_TRUE) ? "true" : "false" ) +
        ",ComputerAwake=" + ( (mIsComputerAwake == Connection::CS_TRUE) ? "true" : "false" );
}

}
}
}
