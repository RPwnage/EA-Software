///////////////////////////////////////////////////////////////////////////////
//
// brief\ Service::GlobalConnectionStateMonitor.h
//
// owns and manages the global connection state variables (internet connection, sleep, update pending)
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef GLOBALCONNECTIONSTATEMONITOR_H
#define GLOBALCONNECTIONSTATEMONITOR_H

#include "services/network/InternetConnectionDetection.h"
#include "services/session/AbstractSession.h"
#include "services/connection/ConnectionStates.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

namespace Network
{
///
/// \brief class GlobalConnectionStateMonitor
///
/// This class monitors whether the global connection states:
/// 1) are we connected to the internet?
/// 2) is the computer awake/asleep?
/// 3) is Origin update pending
///
/// When accessing REST services through the NetworkAccessManager, this class is consulted first
/// to determine whether the network request should proceed
///
class ORIGIN_PLUGIN_API GlobalConnectionStateMonitor : public QObject, public IConnectionListener
{
    Q_OBJECT
public:

    ///
    /// \brief Create the monitor instance
    ///
    static void init();

    ///
    /// \brief Performs additional initialization once SettingsManager has been initialized.
    ///
    static void postSettingsInit();

    ///
    /// \brief Delete the monitor instance
    ///
    static void release();

    ///
    /// \brief Return the monitor instance
    ///
    static GlobalConnectionStateMonitor* instance();

    ///
    ///  \brief Return true if we have internet access, regardless of whether
    /// there is an authorized user or not.
    ///
    
    static bool isOnline()
    {
        // if we have all adapters disabled, instance will be NULL!
        if (!instance())
            return false;

        return (instance()->mIsInternetReachable == Connection::CS_TRUE && 
                instance()->mSimulateIsInternetReachable == Connection::CS_TRUE && 
                instance()->mIsComputerAwake == Connection::CS_TRUE &&
                instance()->mIsRequiredUpdatePending == Connection::CS_FALSE);
    }

    ///
    ///  \brief Return the value of the specified global connection state field. This 
    /// function cannot be used to query per-user field since no user (i.e. session)
    /// is given. An assert will fail if any such field is specified. Depending on 
    /// the field in question, a returned value of true may mean on-line or offline,
    /// this should be clear from the names of the different fields, see the 
    /// ConnectionStateField enum.
    ///
    static Connection::ConnectionStatus connectionField(Origin::Services::Connection::ConnectionStateField field)
    {
        // if we have all adapters disabled, instance will be NULL!
        if (!instance())
            return Connection::CS_FALSE;

        return (instance()->connectionFieldPriv(field));
    }

    ///
    ///  \brief sets global state computer awake/asleep
    /// needs to be public because this function is accessed from the client layer (OriginApplication listens for windows message that computer is awake/asleep)
    ///
    static void setComputerAwake (bool awake) { setConnectionField (Connection::GLOB_OPR_IS_COMPUTER_AWAKE, awake ? Connection::CS_TRUE : Connection::CS_FALSE); }

    ///
    ///  \brief sets global state simulateInternetReachable
    /// needs to be public because this function is accessed from the client layer (OriginApplication sets value based on automation settings passed in on launch)
    ///
    static void setSimulateIsInternetReachable (bool reachable) { setConnectionField (Connection::GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE, reachable ? Connection::CS_TRUE : Connection::CS_FALSE); }

    ///
    ///  \brief sets global state updatePending
    /// needs to be public because this function is accessed from the client layer (MOTDWatcher checks for update and sets this flag if update found)
    ///
    static void setUpdatePending (bool pending) { setConnectionField (Connection::GLOB_ADM_IS_REQUIRED_UPDATE_PENDING, pending ? Connection::CS_TRUE : Connection::CS_FALSE); }

    ///
    ///  \brief Trigger a poll to the internet reachability check, which accesses an EA server
    /// and updates the state of the GLOB_OPR_IS_INTERNET_REACHABLE flag.
    ///
    static
    void pollInternetReachability()
    {
        if (instance())
            instance()->pollInternetReachabilityPriv();
    }

    ///
    ///  \brief Callback from the internet polling class
    ///
    virtual void ConnectionChange(const ConnectionStatus& ics);

signals:

    ///
    ///  \brief Signal emitted when the global connection state changes
    ///
    /// \param field that changed
    void connectionStateChanged(Origin::Services::Connection::ConnectionStateField field);

private slots:
    
    void pollInternetReachabilityPriv();
    void cleanupNetworkDetectionTimer();
    
private:

    ///
    ///  \brief set the value of the specified global state field. An assert will
    /// fail if called with a per-session field.
    ///
    static 
    void setConnectionField (Origin::Services::Connection::ConnectionStateField field, Connection::ConnectionStatus status)
    {
        if (instance())
            instance()->setConnectionFieldPriv(field, status);
    }


    QString stateString();

    Connection::ConnectionStatus mIsRequiredUpdatePending;      // disconnected when true
    Connection::ConnectionStatus mIsComputerAwake;
    Connection::ConnectionStatus mIsInternetReachable;          // disconnected when false
    Connection::ConnectionStatus mSimulateIsInternetReachable;  // disconnected when false, for debugging

    GlobalConnectionStateMonitor();
    GlobalConnectionStateMonitor(GlobalConnectionStateMonitor const&);
    GlobalConnectionStateMonitor& operator=(GlobalConnectionStateMonitor const&);
    ~GlobalConnectionStateMonitor();
    
    Connection::ConnectionStatus connectionFieldPriv(Origin::Services::Connection::ConnectionStateField field);
    void setConnectionFieldPriv(Origin::Services::Connection::ConnectionStateField field, Connection::ConnectionStatus status);
};

}

}

}

#endif // GLOBALCONNECTIONSTATEMONITOR_H
