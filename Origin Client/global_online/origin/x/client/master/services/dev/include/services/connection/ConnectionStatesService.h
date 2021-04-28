///////////////////////////////////////////////////////////////////////////////
// \brief Service::ConnectionStatesService.h
//
// central location used to query the current connection state, both global and session
// also emits signals when connection state changes
// does not contain any functions to set/change the connection state; those should be done directly 
// via those owning the state variables (GlobalConnectionStateMonitor, SessionService)
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CONNECTION_STATES_SERVICE_H
#define CONNECTION_STATES_SERVICE_H

#include "services/connection/ConnectionStates.h"
#include "services/session/SessionService.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Connection
        {
            ///
            /// \brief A singleton class that is used as a central authority to query the current connection state
            /// Additionally, this class listens for the changes in the global state and session state and emits signals
            ///
            class ORIGIN_PLUGIN_API ConnectionStatesService : public QObject
            {
                Q_OBJECT

            public:

                ///
                /// \brief Create the monitor service instance
                ///
                static void init();

                ///
                /// \brief Delete the monitor service instance
                ///
                static void release();

                ///
                /// \brief Return the monitor service instance
                ///
                static ConnectionStatesService* instance();

                ///
                /// \brief returns true/false whether user is allowed to attempt to go back "online"
                ///
                static bool canGoOnline (Session::SessionRef session);

                ///
                /// \brief returns true/false as to whether the user that owns session is "online"
                /// should be used to query when "user" is active
                /// 
                /// \param session
                static bool isUserOnline(Session::SessionRef session);

                ///
                /// \brief returns true/false when global state is "online"
                /// should be used to query when there is no active user
                ///
                static bool isGlobalStateOnline();

                ///
                /// \brief Returns the current authentication of user associated with session
                ///
                static Session::AuthenticationState authenticationState(Session::SessionRef session) { return Session::SessionService::instance()->authenticationState (session); }

                ///
                /// \brief returns true if user's authentication state is SESSIONAUTHENTICATION_AUTH
                ///
                static bool isAuthenticated (Session::SessionRef session) { return authenticationState(session) == Session::SESSIONAUTHENTICATION_AUTH; }

                ///
                /// \brief returns true if updatepending is set
                ///
                static bool isRequiredUpdatePending () { return (connectionField (GLOB_ADM_IS_REQUIRED_UPDATE_PENDING) == CS_TRUE); }

                ///
                /// \brief returns true if single login check complete is TRUE
                ///
                static bool isSingleLoginCheckCompleted () { return (connectionField (USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE) == CS_TRUE); }

                ///
                /// \brief returns true if simulatedInternetReachable is set to true
                ///
                static bool isSimulateIsInternetReachableSet () { return (connectionField (GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE)); }

                ///
                /// \brief Return true if user's ManualOffline variable is set
                ///
                static bool isUserManualOfflineFlagSet (Session::SessionRef session) { return (connectionField (USR_ADM_IS_USER_MANUAL_OFFLINE, session) == CS_TRUE); }

                ///
                /// \brief Return the value of the specified connection state field for the session (or empty if requesting global state)
                ///
                static Connection::ConnectionStatus connectionField(ConnectionStateField field, Session::SessionRef session = Session::SessionRef());

            public slots:

            signals:

                ///
                /// \brief Signal emitted when the userconnection state changes
                ///
                /// \param isOnline = true/false (online/offline) for associated session (user)
                /// \param session = session (user)
                void userConnectionStateChanged(bool isOnline, Origin::Services::Session::SessionRef session);

                ///
                /// \brief Signal emitted when the global connection state changes
                /// Only those who are interested purely in the global state (not associated with user) should connect to this signal, 
                /// in most cases, userConnectionStateChanged should be used
                ///
                /// \param isOnline = true/false (online/offline)
                /// 
                void globalConnectionStateChanged (bool isOnline);

                ///
                /// \brief same as globalConnectionStateChanged(true)
                ///
                void nowOnlineGlobal();
                
                ///
                /// \brief same as globalConnectionStateChanged(false)
                ///
                void nowOfflineGlobal();

                ///
                /// \brief signal emitted when we've lost authentication
                ///
                void authenticationRequired ();

                ///
                /// \brief signal emitted when token renewal authentication succeeds
                ///
                void reAuthenticated ();

                ///
                /// \brief same as UserConnectionStateChanged(true, current user session)
                /// 
                void nowOnlineUser();

                ///
                /// \brief signal emitted when user is placed offline
                ///
                void nowOfflineUser();

                ///
                /// \brief signal emitted when user is placed offline
                ///
                void nowRecovering();


            private slots:

                ///
                /// \brief slot for setting up the connect to listen for sessionRef state changes
                ///
                void onBeginSessionComplete(Origin::Services::Session::SessionError error, Origin::Services::Session::SessionRef session);

                ///
                /// \brief slot for disconnecting to the session that's logging out, so we don't listen for any authenticationChanges
                ///
                void onEndSessionComplete(Origin::Services::Session::SessionError error, Origin::Services::Session::SessionRef session);

                ///
                /// \brief connected to the session that emits authenticationStateChanged
                ///
                void onAuthenticationStateChanged (Origin::Services::Session::AuthenticationState newAuthState, Origin::Services::Session::SessionRef session);

                ///
                /// \brief connected to GlobalStateMonitor and is passed in the field that changed
                ///
                void onGlobalConnectionStateChanged (Origin::Services::Connection::ConnectionStateField field);

                ///
                /// \brief connected to session signal userOnlineAllChecksCompleted after session goes thru authentication and single login
                ///
                void onUserOnlineAllChecksCompleted (bool isOnline, Origin::Services::Session::SessionRef session);

                ///
                /// \brief connected to recovery timer to clean up after recovery period
                ///
                void onRecoveryPeriodEnded();
            private:


                static const QString SERVICE_DATA_KEY;

                ConnectionStatesService();

                bool connectionStateActuallyChanged (Origin::Services::Session::SessionRef session, bool newState);

                bool isRecoveryInProgress;
                const unsigned int RecoveryIntervalMilliseconds;
            };

        }   //namespace Connection

    } //namespace Services

} //namespace Origin

#endif // CONNECTION_STATES_SERVICE_H
