//  LoginRegistrationSession.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef LOGINREGISTRATIONSESSION_H
#define LOGINREGISTRATIONSESSION_H

#include "services/session/AbstractSession.h"
#include "services/session/AbstractSessionCredentials.h"
#include "services/session/SessionService.h"
#include "services/rest/AuthenticationData.h"
#include "services/rest/OriginServiceValues.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/plugin/PluginAPI.h"

#include <QtCore>

namespace Origin
{
    namespace Services
    {
		class AuthPortalTokensResponse;
		class ExtendTokenServiceResponse;
        class UserResponse;

        namespace Session
        {
            class ORIGIN_PLUGIN_API LoginRegistrationConfiguration2 : public AbstractSessionConfiguration
            {
            public:
                LoginRegistrationConfiguration2();

                void setIdToken(const QString& token);
                void setAuthorizationCode(const QString& code);
                void setOfflineKey(const QString& offlineKey);
                void setBackupOfflineKey(const QString& backupKey);
                void setRefreshToken(const RefreshToken& token);

                const QString& idToken() const;
                const QString& authorizationCode() const;
                const QString& offlineKey() const;
                const QString& backupOfflineKey() const;
                const RefreshToken& refreshToken() const;

                bool hasIdToken() const;
                bool hasAuthorizationCode() const;
                bool hasOfflineKey() const;
                bool hasRefreshToken() const;

                bool isOfflineLogin() const;
                bool isAutoLogin() {return false;}

            private:
                QString mIdToken;
                QString mAuthorizationCode;
                QString mOfflineKey;
                QString mBackupOfflineKey;
                RefreshToken mRefreshToken;
            };

            /// \brief Defines the session management when logging in via the LoginRegistration API. 
            class ORIGIN_PLUGIN_API LoginRegistrationSession : public AbstractSession
            {
                friend class SessionService;
                Q_OBJECT

            public:

                /// \brief Factory method for creating LoginRegistrationSession objects.
                static SessionRef create();

                /// \brief Returns the authentication info associated with this session.
                ///
                //Authentication const& authenticationInfo() const;
                Authentication2 const& authentication2Info() const;

                /// \brief renew token
                virtual void renewAccessToken();

                /// \brief renew sid cookie
                void renewSidCookie();

                ///
                ///  \brief This function should be called prior to any calls to refreshToken() to see if we should be attempting a refresh.
                ///  We should NOT attempt a refresh if: <ul>
                ///  <li>An update is pending</li>
                ///  <li>There is no internet connection (although this is up for discussion)</li>
                ///  <li>The computer is asleep</li>
                ///  <li>The user is manually offline</li>
                ///  </ul>
                bool canRefreshToken();

                /// \brief Reports when the user authenticated, if they have.
                /// \return The date/time in the local timezone where the user logged in, which will be an invalid time if they haven't logged in yet.
                QDateTime loginTime();

            signals:
                /// \brief Indicates that the SID renewal request has completed
                /// Check the error status codes to indicate failure or success.
                /// \param restError REST error code
                /// \param httpStatus HTTP status code
                void sidRenewalRequestCompleted(qint32 restError, qint32 httpStatus);

            protected:

                /// \brief TBD.
                virtual void beginAsync(AbstractSessionConfiguration const& configuration);

                void retrieveTokens();

                /// \brief TBD.
                virtual void endAsync();

                /// \brief Returns a reference to the session's configuration.
                virtual AbstractSessionConfiguration const& configuration() const;

                /// \brief TBD.
                AccessToken accessToken() const;

                /// \brief TBD.
                RefreshToken refreshToken() const;

                /// \brief Update origin id and email from the given response
                void refreshOriginId(Origin::Services::UserResponse* resp);

                /// \brief TBD.
                QString nucleusUser() const;

                /// \brief nucleus Persona.
                QString nucleusPersona() const;

                /// \brief Returns the current state of authentication.
                Origin::Services::Session::AuthenticationState authenticationState() const;

                /// \brief Sets the connection field to status
                void setConnectionField (Origin::Services::Connection::ConnectionStateField field, Origin::Services::Connection::ConnectionStatus status);

                /// \brief Returns the current value of connection field.
                Origin::Services::Connection::ConnectionStatus connectionField (Origin::Services::Connection::ConnectionStateField field);

                /// \brief Returns true if authenticated, manualoffline = false && single login check completed = true.
                bool isOnline ();

                void setPreviouslyOnline (bool online) { mPreviouslyOnline = static_cast<int>(online); }

                int previouslyOnline () { return mPreviouslyOnline; }

                /// \brief Called when logging back in to update an existing session.
                /// \param configuration TBD.
                void updateAsync (AbstractSessionConfiguration const& configuration);

                /// \brief called to connect slot to listen to authenticationError from OriginServiceResponses
                void connectAuthenticationError ();

                /// \brief called to disconnect slot to listen to authenticationError from OriginServiceResponses
                void disconnectAuthenticationError ();

                /// \brief return the time that we went offline
                QDateTime wentOfflineTime () { return mWentOfflineTime;}

                /// \brief save off the time that we went offline so we can use it to compare against when we go back online
                void setWentOfflineTime (QDateTime offlineTime);

            signals:

                /// \fn tokensChanged
                /// \brief Signal emitted when the session tokens change.
                void tokensChanged(
                    Origin::Services::Session::AccessToken authToken,
                    Origin::Services::Session::RefreshToken refreshToken,
                    Origin::Services::Session::SessionError error);

                /// \brief Signal emitted when authentication state has changed
                void authenticationStateChanged(Origin::Services::Session::AuthenticationState newAuthenticationState, Origin::Services::Session::SessionRef session);

                /// \brief emitted when the user has been fully authenticated -- i.e. authenticated, went thru all the checks and can be considered "online"
                void userOnlineAllChecksCompleted (bool isOnline, Origin::Services::Session::SessionRef session);

                /// \brief emitted when attempting to go back online, and need to relogin for to retrieve user's login info from the server, but relogin has
                /// to get initiated from the engine layer
                void relogin (Origin::Services::Session::SessionRef session);

            protected slots:

                /// \brief TBD.
                virtual void onBeginFinished();

                void loginComplete(SessionError error);
                void onTokensRetrieved();


                void onUserPidRetrieved();
                void onUserNameRetrieved();

                /// \brief handles initial token retrieval's error
                void onTokensRetrievedError(Origin::Services::restError);

                /// \brief TBD.
                virtual void onEndFinished();

            private slots:

                /// \brief TBD.
                void onClientConnectionStateChanged(Origin::Services::Connection::ConnectionStateField field);

                /// \brief TBD.
                /// connected to signal from REST services when REST response contains a TOKEN authentication failure
                ///
                void onRestAuthenticationFailed(Origin::Services::restError error);

                /// \brief TBD.
                void onRenewalTimerTimeout();

				/// \brief TBD.
				void onExtendTokensResponse();

                /// \brief The callback for when the request to refresh the sid cookie has returned
                void onExtendSidCookieResponse();

                /// \brief The callback for when the request has returned.
                void onPersonasRetrieved();

                /// \brief The callback for when the request has returned.
                void onAppSettingsRetrieved();

                /// \brief The callback when a cookie change in the cookie jar has been detected
                void onSetCookie (QString cookieName);

            private:

                LoginRegistrationSession();
                LoginRegistrationSession(LoginRegistrationSession const& from);
                LoginRegistrationSession& operator=(LoginRegistrationSession const& from);

                /// \brief Starts renewal timer based on nucleus 2 token values.
                void startRenewalTimer();

                /// \brief Starts renewal or retrieval tokens timer.
                void startFallbackRenewalTimer();

                /// \brief An authenticated login has either failed or is blocked (update pending), so try logging in using the offline cache
                SessionError attemptLoginViaOfflineCache();

                /// \brief common function to handle response to token renewal or re-login after being offline
                void handleRefreshReloginResponse(const restError& error);

                /// \brief stop renewal and relogin timers
                void stopTimers ();

                /// \brief retrieves the user's Persona from EADP
                void retrieveUserPersona();

				/// \brief retrieves the userPID (nucleusID) from EADP
                void retrieveUserPid();

				/// \brief extracts the cid from the idToken passed in from login window
                void extractCid();

				/// \brief retrieves the username (last, first)
                void retrieveUserName();

				/// fn retrieveAppSettings
				/// \brief performs request to retrieve user settings
                void retrieveAppSettings();

                /// \brief call to update with the new token
                void doTokensChanged(Origin::Services::TokensNucleus2 newToken);

                /// \brief attempt offline login and emit beginFinished(error, self()), called when any of the REST requests fail (retrievetokens, retriveuserPid, etc)
                void attemptOfflineLogin(Origin::Services::Session::SessionError error);

                /// \brief set up the connection to listen for when new cookies are added so that we can correctly update the login related cookies that are saved to settings
                void connectSetCookie ();

                /// \brief disconnect listening for new cookies so that it doesn't get called after session becomes invalid
                void disconnectSetCookie ();

                LoginRegistrationConfiguration2 mConfiguration2;
                QString mOfflineKey;
                QString mBackupOfflineKey;
                QString mIdToken;
				QDateTime mLoginTime;

                //Authentication mAuthenticationInfo;
                Authentication2 mAuthentication2Info;
                QTimer mRenewalTimer;
                time_t mAccessTokenExpiryTime;
                qint32 mRetryCount;
                qint32 mCurrentSidRefreshInterval;

                // for initial retrieval of tokens
                QString mAuthorizationCode;

				/// \brief pointer to request response. Used to abort request if caught in the middle of password change-triggered tokens refresh
				AuthPortalTokensResponse* mExtendTokenServiceResponse;

                Connection::ConnectionStatus mIsUserManualOffline; //= true when user chooses to go offline from client window OR after single login conflict OR logged into offline mode
                Connection::ConnectionStatus mIsSingleLoginCheckCompleted;  //set to 
                int mPreviouslyOnline;     // used to keep track of actual session online/offline connection change so we'd only emit if actual overall connection state changes
                                           // needs to be int so we can have -1 indicate that it's not been initialized

                bool mClearOnFailure;       //defaults to true, but when the session gets held by the user, it should get set to false so that when RE-login failure occurs
                                            //the session isn't cleared

                /// \brief timer used to timeout of waiting for renewal request of access_token
                QTimer mRenewRequestTimeoutTimer; 

                /// \brief keep track of when last when offline so we can compare against it when going back online
                QDateTime mWentOfflineTime;
            };

            inline AccessToken LoginRegistrationSession::accessToken() const
            {
                return AccessToken(mAuthentication2Info.token.mAccessToken);
            }

            inline RefreshToken LoginRegistrationSession::refreshToken() const
            {
                return RefreshToken(mAuthentication2Info.token.mRefreshToken);
            }

            inline QString LoginRegistrationSession::nucleusUser() const
            {
                return QString::number(mAuthentication2Info.user.userId);
            }

            inline QString LoginRegistrationSession::nucleusPersona() const
            {
                return QString::number(mAuthentication2Info.user.personaId);
            }

            inline AbstractSessionConfiguration const& LoginRegistrationSession::configuration() const
            {
                return mConfiguration2;
            }
        } // namespace Session
    } // namespace Services
} // namespace Origin

#endif // LOGINREGISTRATIONSESSION_H
