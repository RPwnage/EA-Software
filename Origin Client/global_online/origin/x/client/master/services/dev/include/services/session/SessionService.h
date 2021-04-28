//  SessionService.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef SESSIONSERVICE_H
#define SESSIONSERVICE_H

#include <limits>
#include <QDateTime>
#include <QtCore>

#include "services/session/AbstractSession.h"
#include "services/session/AbstractSessionCredentials.h"
#include "services/connection/ConnectionStates.h"

namespace Origin
{
    namespace Services
    {
        /// \brief The %Session namespace contains the instructions that provide %Origin %Session management.
        ///
        /// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/TB+-+Session+Management">Session Management</a>
        namespace Session
        {
            
            class ISession;

            enum AuthenticationState
            {
                SESSIONAUTHENTICATION_NONE,
                SESSIONAUTHENTICATION_ENCRYPTED,
                SESSIONAUTHENTICATION_AUTH
            };

            //cookies used for login purposes
            const QString LOGIN_COOKIE_NAME_REMEMBERME = "remid";
            const QString LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE = "ttdid";
            const QString LOGIN_COOKIE_NAME_LASTUSERNAME = "pcun";

            /// \brief Singleton manager that governs sessions and lifetime. 
            /// The SessionService ... 
            class ORIGIN_PLUGIN_API SessionService : public QObject
            {
                Q_OBJECT

            public:

                /// \brief Initializes the session service.
                static void init();

                /// \brief De-initializes the session service.
                static void release();

                /// \brief Returns a pointer to the session services.
                static SessionService* instance();

                /// \brief Initiates a new session using the given token.
                static void beginSessionAsync(SessionFactory factory, AbstractSessionConfiguration const& configuration);
                static void beginSessionAsync2(SessionFactory factory, AbstractSessionConfiguration const& configuration);

                /// \brief Updates session with the given tokens
                static void updateSessionAsync(SessionRef session, AbstractSessionConfiguration const& configuration);

                /// \brief Refresh the tokens in the given session
                void renewAccessTokenAsync(SessionRef session);

                /// \brief Terminates the given session.
                static void endSessionAsync(SessionRef session);

                /// \brief Call the server to get the current origin id and email for this user.
                static void refreshOriginIdFromServerAsync(SessionRef session);

                /// \brief Returns true if the current session is valid.
                static bool isValidSession(SessionRef session);

                /// \brief Returns true if the current session is authenticated against
                /// our services
                static AuthenticationState authenticationState(SessionRef session);

                /// \brief Returns true if there is a currently valid session.
                static bool hasValidSession();

                /// \brief Returns the current session.
                /// NOTE: this function is NOT TO BE USED except under exceptional circumstances.
                static SessionRef currentSession();

                /// \brief Returns the access token associated with the given session.
                static AccessToken accessToken(SessionRef session);

                /// \brief Returns the refresh token associated with the given session.
                static RefreshToken refreshToken(SessionRef session);

                /// \brief Issues a request to renew SID cookie
                static void renewSid(SessionRef session);

                /// \brief Returns the nucleus user associated with the given session.
                static QString nucleusUser(SessionRef session);

                /// \brief Returns the nucleus persona associated with the given session.
                static QString nucleusPersona(SessionRef session);

                /// \brief Returns the time the user of the given session was successfully logged in.
                static QDateTime loginTime(SessionRef session);

                ///
                /// sets the value of manualUserOffline flag
                ///
                static void setManualOfflineFlag (SessionRef session, bool offline) { setConnectionField (session, Connection::USR_ADM_IS_USER_MANUAL_OFFLINE, offline ? Connection::CS_TRUE : Connection::CS_FALSE); } 

                ///
                /// sets the value of the SingleLoginCheckCompleted flag
                ///
                static void setSingleLoginCkCompletedFlag (SessionRef session, bool completed) { setConnectionField (session, Connection::USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE, completed ? Connection::CS_TRUE : Connection::CS_FALSE); } 

                /// \brief return the value of the session's connection state
                static Connection::ConnectionStatus connectionField (SessionRef session, Origin::Services::Connection::ConnectionStateField field);

                /// \brief returns true if the session connection states deem the user to be online (checks just the session connection states, ignores global connection state)
                static bool isOnline (SessionRef session);

                /// \brief caches the previous online state of the session
                static void setPreviouslyOnline (SessionRef session, bool online);

                /// \brief returns the previous online state of the session
                static int previouslyOnline (SessionRef session);

                /// \brief Update the user's email.
                void updateCurrentUserEmail(SessionRef session, const QString& email);

                /// \brief Update the user's Origin ID.
                void updateCurrentUserOriginId(SessionRef session, const QString& originId);

                ///
                /// reset timestamp to QDateTime() to avoid wrap around occuring when difference is calculated between now and the last time the user went offline;
                ///                
                static void resetWentOfflineTime (SessionRef session);

                /// \brief return the time that we went offline
                static QDateTime wentOfflineTime (SessionRef session);

            signals:

                /// \brief Signal emitted when beginSessionAsync has completed.
                void beginSessionComplete(
                    Origin::Services::Session::SessionError,
                    Origin::Services::Session::SessionRef,
                    QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>);

                /// \brief Signal emitted when endSessionAsync has completed.
                void endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef);

                ///
                /// \brief Signal emitted when session's tokens have changed. 
                ///
                /// If token renewal changed (because the user changed their password in a web browser), 
                /// this signal is emitted with empty tokens.
                ///
                void sessionTokensChanged(Origin::Services::Session::SessionRef, Origin::Services::Session::AccessToken, Origin::Services::Session::RefreshToken, Origin::Services::Session::SessionError);

                /// \brief Signal emitted when the session's authentication state changed.
                void authenticationStateChanged(Origin::Services::Session::AuthenticationState, Origin::Services::Session::SessionRef session);

                /// \brief emitted when the user has been fully authenticated -- i.e. authenticated, went thru all the checks and can be considered "online"
                void userOnlineAllChecksCompleted (bool isOnline, Origin::Services::Session::SessionRef session);

                /// \brief emitted when attempting to go back online, and need to relogin for to retrieve user's login info from the server
                void relogin (Origin::Services::Session::SessionRef session);

                /// \brief Emitted as response to an async call to get the current origin id and email
                /// \param eaid The current Origin id of the user
                /// \param session The current session
                void originIdRefreshedFromServer(QString const& eaid, Origin::Services::Session::SessionRef session);

                /// \brief Emitted if a user's origin ID changed
                void userOriginIdUpdated(QString const& eaid, Origin::Services::Session::SessionRef session);

                /// \breif Emitted if a user's email changed
                void userEmailUpdated(QString const& email, Origin::Services::Session::SessionRef session);

                /// \brief Emitted when we receive the SID renewal response
                void sidRenewalResponse(qint32, qint32);

            protected slots:
            
                void onBeginFinished(SessionError error, SessionRef session);
                void onEndFinished(SessionError error, SessionRef session);

                void onSessionTokensChanged(Origin::Services::Session::AccessToken accessToken, Origin::Services::Session::RefreshToken refreshToken, Origin::Services::Session::SessionError error);

                void onRefreshOriginIdReply();

            private:
                /// \brief passes to the session the new value for the session's connection state 
                static void setConnectionField (SessionRef session, Origin::Services::Connection::ConnectionStateField field, Origin::Services::Connection::ConnectionStatus status);

                SessionService();
                SessionService(SessionService const& from);
                SessionService& operator=(SessionService const& from);

                static SessionRef createSession(SessionFactory factory);

                SessionRef mCurrentSession;
            };

        } // namespace Session
    } // namespace Services
} // namespace Origin

#endif // SESSIONSERVICE_H
