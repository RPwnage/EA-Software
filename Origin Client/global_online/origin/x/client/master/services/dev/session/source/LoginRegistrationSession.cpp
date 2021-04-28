//  LoginRegistrationSession.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "services/session/LoginRegistrationSession.h"

#include "services/rest/IdentityPortalServiceClient.h"
#include "services/rest/IdentityPortalServiceResponses.h"
#include "services/rest/AuthPortalServiceClient.h"
#include "services/rest/AuthPortalServiceResponses.h"

#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/connection/ConnectionStates.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/crypto/CryptoService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/rest/AtomServiceResponses.h"
#include "services/rest/AtomServiceClient.h"
#include "services/rest/AtomServiceResponses.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network//NetworkCookieJar.h"

#include "TelemetryAPIDLL.h"
#include "NodeDocument.h"
#include "ReaderCommon.h"

#include <QDesktopServices>
#include <QtWidgets>
#include <stdio.h>
#include <time.h>

#if defined(_DEBUG)
#define ENABLE_SESSION_LOGGING 1
#define ENABLE_EARLY_TOKEN_TIMEOUT 0
#else
// ensure that the following remain at 0 so that we never log them in release builds
#define ENABLE_SESSION_LOGGING 0
#define ENABLE_EARLY_TOKEN_TIMEOUT 0
#endif

namespace
{
    qint64 timerInterval(qint64 authTokenExpiryInSeconds)
    {
#if ENABLE_EARLY_TOKEN_TIMEOUT
        qint64 durationInMsec = 30 * 1000; // start renewal in 30 seconds (for testing)
#else
        // start renewal 1/6 of the expiration time, i.e. 10 min for PROD which is currently set to ~1 hour (59 minutes, 58 seconds)
        // for integration it is currently set to 20 min, so it would be ~ 3 min20 seconds
        qint64 startInterval = (authTokenExpiryInSeconds / 6);
        qint64 durationInMsec = (authTokenExpiryInSeconds - startInterval) * 1000;
#endif
        if (durationInMsec < 0)
            durationInMsec = authTokenExpiryInSeconds * 1000;
        return durationInMsec;
    }
}

namespace Origin
{
    namespace Services
    {
        namespace Session
        {
            const int NETWORK_TIMEOUT_MSECS = 30000;

            // The SID cookie refresh interval is tied to a certain number of token refreshes. In production,
            // this interval will represent a time interval of hours. In integration, the time interval is
            // 20 * SID_REFRESH_INTERVAL minutes.
            const int SID_REFRESH_INTERVAL = 10;

            QString offlineLoginPath(QString const& key, bool makeDir = false);   //forward declaration

            LoginRegistrationConfiguration2::LoginRegistrationConfiguration2()
            {

            }

            void LoginRegistrationConfiguration2::setIdToken(const QString& token)
            {
                mIdToken = token;
            }

            void LoginRegistrationConfiguration2::setAuthorizationCode(const QString& code)
            {
                mAuthorizationCode = code;
            }

            void LoginRegistrationConfiguration2::setOfflineKey(const QString& key)
            {
                mOfflineKey = key;
            }

            void LoginRegistrationConfiguration2::setBackupOfflineKey(const QString& key)
            {
                mBackupOfflineKey = key;
            }

            const QString& LoginRegistrationConfiguration2::idToken() const
            {
                return mIdToken;
            }

            void LoginRegistrationConfiguration2::setRefreshToken(const RefreshToken& refreshToken)
            {
                mRefreshToken = refreshToken;
            }

            const QString& LoginRegistrationConfiguration2::authorizationCode() const
            {
                return mAuthorizationCode;
            }

            const QString& LoginRegistrationConfiguration2::offlineKey() const
            {
                return mOfflineKey;
            }

            const QString& LoginRegistrationConfiguration2::backupOfflineKey() const
            {
                return mBackupOfflineKey;
            }

            bool LoginRegistrationConfiguration2::hasIdToken() const
            {
                return !mIdToken.isEmpty();
            }

            const RefreshToken& LoginRegistrationConfiguration2::refreshToken() const
            {
                return mRefreshToken;
            }

            bool LoginRegistrationConfiguration2::hasAuthorizationCode() const
            {
                return !mAuthorizationCode.isEmpty();
            }

            bool LoginRegistrationConfiguration2::hasOfflineKey() const
            {
                return !mOfflineKey.isEmpty();
            }

            bool LoginRegistrationConfiguration2::isOfflineLogin() const
            {
                return !hasAuthorizationCode() && hasOfflineKey();
            }

            static bool retrieveOfflineLoginInfo(
                QString const& user, 
                QByteArray const& offlineCacheKey, 
                Authentication2& authInfo);
            static bool storeOfflineLoginInfo( 
                QByteArray const& offlineCacheKey, 
                Authentication2 const& authInfo);
            static void removeOfflineLoginSettings(const QString& eaId);
            static void addOfflineLoginSettings(const QString& eaId, QByteArray const& offlineCacheKey);
            static CryptoService::Cipher sOfflineCipher = CryptoService::BLOWFISH;
            static CryptoService::CipherMode sOfflineCipherMode = CryptoService::CIPHER_BLOCK_CHAINING;

            SessionRef LoginRegistrationSession::create()
            {
                QSharedPointer<LoginRegistrationSession> ret(new LoginRegistrationSession());
                ret->setSelf(ret);
                return ret;
            }

            Origin::Services::Session::AuthenticationState LoginRegistrationSession::authenticationState() const
            {
                if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                    return Origin::Services::Session::SESSIONAUTHENTICATION_AUTH;
                else
                    return Origin::Services::Session::SESSIONAUTHENTICATION_NONE;
            }

            void LoginRegistrationSession::beginAsync(AbstractSessionConfiguration const& configuration)
            {
                LoginRegistrationConfiguration2 const* loginConfig = dynamic_cast<LoginRegistrationConfiguration2 const*>(&configuration);

                if (!loginConfig)
                {
                    ORIGIN_ASSERT_MESSAGE(false, "LoginRegstrationSession: bad configuration");
                    return;
                }

                mConfiguration2 = *loginConfig;
                mIdToken = loginConfig->idToken();
                mOfflineKey = loginConfig->offlineKey();
                mBackupOfflineKey = loginConfig->backupOfflineKey();
                
                //if Origin update is pending, that means the user chose to log directly into offline mode
                if (Origin::Services::Connection::ConnectionStatesService::isRequiredUpdatePending())
                {
                    SessionError error;

                    //check for existence of credentials first since we may have gotten credentials after forcing a fail from auto-login
                    if (loginConfig->isOfflineLogin())
                    {
                        error = attemptLoginViaOfflineCache();
                    }
                    //if it's auto-login or sso, we need to request credentials
                    else if (loginConfig->hasAuthorizationCode())
                    {
                        //go back out and ask for credentials
                        //we only want to set the session error since this is not a REST error
                        //mRestError will remain restErrorSuccess
                        error.overrideSessionErrorCode (SESSION_UPDATE_PENDING); 
                    }

                    // TODO: does this still flow correctly into the appropriate spot or has that been broken. Test, basically.
                    emit beginFinished(error, self());
                    return;
                }

                if (loginConfig->isOfflineLogin())
                {
                    SessionError error = attemptLoginViaOfflineCache();
                    emit beginFinished(error, self());
                }
                else
                {
                    if (loginConfig->hasAuthorizationCode())
                    {
                        ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "initiating token fetch";
                        mAuthorizationCode = loginConfig->authorizationCode();
                        extractCid();
                    }
                    else
                    {
                        ORIGIN_LOG_EVENT << "session: bad login configuration";
                        ORIGIN_ASSERT_MESSAGE(false, "bad login configuration");

                        emit beginFinished(restErrorAuthenticationNucleus, self());
                    }
                }
            }

            void LoginRegistrationSession::endAsync()
            {
                //this is just a stop-gap to ensure that the timer doesn't fire after  user has logged out
                //currently others are holding onto the sessionRef so this isn't getting deleted on logout
                //thereby preventing the timer from getting deleted
                stopTimers ();

                //disconnect so we don't get a signal after session is no longer valid
                disconnectSetCookie();

                //disconnect so we don't get a signal after the session is no longer valid
                disconnectAuthenticationError ();

                //clear the authentication states, etc. in case anyone tries to access the values
                mIsUserManualOffline = Connection::CS_FALSE;
                mIsSingleLoginCheckCompleted = Connection::CS_FALSE;
                mAuthentication2Info.reset();
                mAuthentication2Info.token.mRefreshToken.clear();
                emit endFinished(restErrorSuccess, self());
            }

            /// used for when we need to re-login to obtain user's login info from the server, assumes session already exists
            void LoginRegistrationSession::updateAsync (AbstractSessionConfiguration const& configuration)
            {
                LoginRegistrationConfiguration2 const* loginConfig = dynamic_cast<LoginRegistrationConfiguration2 const*>(&configuration);

                //we should allow for empty refresh token to make the request anyways
                //because this could be the case where the user was initially logged into offline mode, so the session had no valid refresh token
                //and now that they are trying to go back online AND they don't have remember me set, then there is no saved off refresh token
                //so we need the response to come back as INVALID_TOKEN so that the user will be logged out

                if (!loginConfig)
                {
                    ORIGIN_ASSERT_MESSAGE(false, "LoginRegstrationSession::UpdateSync : bad configuration");
                    return;
                }

                renewAccessToken();
            }

            Authentication2 const& LoginRegistrationSession::authentication2Info() const
            {
                return mAuthentication2Info;
            }
          
            void LoginRegistrationSession::setConnectionField (Origin::Services::Connection::ConnectionStateField field, Origin::Services::Connection::ConnectionStatus status)
            {
                switch (field)
                {
                    case Connection::USR_ADM_IS_USER_MANUAL_OFFLINE:
                        {
                            //if we've done single login check AND we're authenticated
                            //if we've previously set manualOffline = true and gone offline, authenticationState() would have been set to partial auth, SESSIONAUTHENTICATION_ENCRYPTED
                            if (mIsSingleLoginCheckCompleted == Connection::CS_TRUE && authenticationState() == SESSIONAUTHENTICATION_AUTH)
                            {
                                mIsUserManualOffline = status;
                                //if state == false, then we're all clear 
                                if (status == Connection::CS_FALSE)
                                {
                                    emit userOnlineAllChecksCompleted (true, self());
                                }
                                else
                                {
                                    //we're going manually offline either thru user selecting off main menu/client window OR user choosing
                                    //to go offline after single login check
                                    //it will clear mIsUserSingleLoginCheckCompleted to force check when they want to go back online
                                    onClientConnectionStateChanged (Connection::USR_ADM_IS_USER_MANUAL_OFFLINE);
                                }
                            }
                            //user is attempting to go back online, but only allow if actual state change so we don't initiate refresh again
                            //changing state or trying to go back online after we failed to renew token
                            else if ((mIsUserManualOffline != status) ||
                                     (mIsUserManualOffline == false && 
                                      mAuthentication2Info.token.mAccessToken.isEmpty() && //we cleared this after refresh failed
                                      mExtendTokenServiceResponse == NULL /* refresh not already pending */))
                            {
                                mIsUserManualOffline = status;
                                onClientConnectionStateChanged (Connection::USR_ADM_IS_USER_MANUAL_OFFLINE);
                            }
                        }
                        break;

                    case Connection::USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE:
                        //just set the flag, don't initiate any state changes, global state or manual offline will actually cause the state change
                        mIsSingleLoginCheckCompleted = status;
                        break;

                    default:
                        {
                            ORIGIN_ASSERT_MESSAGE (0, "trying to set unkown session state");
                            ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "trying to set unknown session state: " << field << " state= " << status;
                        }
                        break;
                }
            }

            Connection::ConnectionStatus LoginRegistrationSession::connectionField (Origin::Services::Connection::ConnectionStateField field)
            {
                switch (field)
                {
                    case Connection::USR_ADM_IS_USER_MANUAL_OFFLINE:
                        return mIsUserManualOffline;

                    case Connection::USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE:
                        return mIsSingleLoginCheckCompleted;
                        break;

                    default:
                        {
                            ORIGIN_ASSERT_MESSAGE (0, "trying to get unkown session state");
                            ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "trying to get unknown session state: " << field;
                        }
                        break;
                }
                return Connection::CS_UNDEFINED;
            }

            bool LoginRegistrationSession::isOnline ()
            {
                return (authenticationState() == SESSIONAUTHENTICATION_AUTH &&
                        mIsSingleLoginCheckCompleted == Connection::CS_TRUE &&
                        mIsUserManualOffline == Connection::CS_FALSE);
            }

            void logTokens(const TokensNucleus2& tokensNucleus2)
            {
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "<LoginRegistrationSession Nuke 2 Token info>";
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "access_token: " << tokensNucleus2.mAccessToken ;
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "refresh_token: " << tokensNucleus2.mRefreshToken ;
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "token_type: " << tokensNucleus2.mTokenType;
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "expires_in: " << tokensNucleus2.mExpiresIn;
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "id_token: " << tokensNucleus2.mIdToken;
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "</LoginRegistrationSession Nuke 2 Token info>";
            }

            
            void LoginRegistrationSession::attemptOfflineLogin(Origin::Services::Session::SessionError error)
            {
                SessionError cacheError;

                ORIGIN_LOG_ERROR << "Attempting Offline login after REST request failed: " << error.restErrorCode() << ":" << error.errorCode();

                mRenewalTimer.stop();   //in case it was started

                //make sure we don't have partial data
                mAuthentication2Info.reset();
                mAuthentication2Info.token.mRefreshToken.clear();

                cacheError = attemptLoginViaOfflineCache();

				emit beginFinished(cacheError, self());
            }


            void LoginRegistrationSession::onTokensRetrieved()
            {              
                GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::Authentication);
                
                AuthPortalTokensResponse* response = dynamic_cast<AuthPortalTokensResponse*>(sender());
                ORIGIN_ASSERT(response);
                if ( !response )
                {
                    //we should log the user into offline mode if possible
                    attemptOfflineLogin(restErrorIdentityGetTokens);
                    return;
                }
                else
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onTokensRetrieved()));
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(error(Origin::Services::restError)), this, SLOT(onTokensRetrievedError(Origin::Services::restError)));
                }
                
                if (restErrorSuccess == response->error())
                {
                    //in this case, for initial login, we want to initialize mAuthentication2Info.token.accessToken so that
                    //it will be able to distinguish from the reauthentication case
                    mAuthentication2Info.token = response->tokens();
                    if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                    {
                        doTokensChanged(response->tokens());
                        startRenewalTimer();
                        retrieveUserPid();
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "retrieve tokens: received empty access token";
                        attemptOfflineLogin(restErrorIdentityGetTokens);
                        return;
                    }
                }
                else
                {
                    //  TELEMETRY
                    QString errorString = QString("onTokensRetrieved: request failed=%1").arg(response->error());
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());

                    ORIGIN_ASSERT(self().data() == this);

                    QString errorText(response->reply()->readAll());
                    ORIGIN_LOG_ERROR << "retrieveTokens request failed: " << errorText;
                    attemptOfflineLogin(response->error());
                    return;
                }
            }

            void LoginRegistrationSession::onTokensRetrievedError(Origin::Services::restError rError)
            {
                //  TELEMETRY
                QString errorString = QString("onTokensRetrievedError: request failed=%1").arg(rError);
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());

                ORIGIN_LOG_ERROR << " Tokens Retrieved Error: " << rError;
                AuthPortalTokensResponse* response = dynamic_cast<AuthPortalTokensResponse*>(sender());
                ORIGIN_ASSERT(response);
                if (NULL!=response)
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onTokensRetrieved()));
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(error(Origin::Services::restError)), this, SLOT(onTokensRetrievedError(Origin::Services::restError)));
                    ORIGIN_LOG_ERROR << " Tokens Retrieved HTTP status code: " << response->httpStatus();
                }

                ORIGIN_ASSERT(self().data() == this);
                attemptOfflineLogin(rError);
            }

            void LoginRegistrationSession::retrieveUserPid()
            {
                IdentityPortalUserResponse* response = NULL;
                if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                {
                    response = IdentityPortalServiceClient::retrieveUserInfoByToken(mAuthentication2Info.token.mAccessToken);
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "initiating user pid fetch";
                    if (response)
                    {
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onUserPidRetrieved()));
                        QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                    }
                }
                else
                {
                    //  TELEMETRY
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", "retrieveUserPid: bad id token");
                    ORIGIN_LOG_EVENT << "retrieveUserPid: bad access token";
                    ORIGIN_ASSERT_MESSAGE(false, "retrieveUserPid: bad access token");
                    attemptOfflineLogin(restErrorIdentityGetTokens);//pass in token error as opposed to retrieveUserPid since it's the token that's bad
                    return;
                }

                if (response)
                {
                    response->setDeleteOnFinish(true);
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Unable to create retriveUserPid request";
                    attemptOfflineLogin(restErrorIdentityGetUserPid);
                }
            }

            void LoginRegistrationSession::retrieveUserPersona()
            {
                IdentityPortalExpandedPersonasResponse* response = NULL;
                if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                {
                    response = IdentityPortalServiceClient::expandedPersonas(mAuthentication2Info.token.mAccessToken, mAuthentication2Info.user.userId);
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "initiating user persona fetch";
                    if (response)
                    {
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onPersonasRetrieved()));
                        QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                    }
                }
                else
                {
                    //  TELEMETRY
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", "retrieveUserPersona: bad id token");
                    ORIGIN_LOG_EVENT << "retrieveUserPersona: bad access token";
                    ORIGIN_ASSERT_MESSAGE(false, "retrieveUserPersona: bad access token");
                    attemptOfflineLogin(restErrorIdentityGetTokens);//pass in token error as opposed to retrieveUserPid since it's the token that's bad
                    return;
                }

                if (response)
                {
                    response->setDeleteOnFinish(true);
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Unable to create retriveUserPersona request";
                    attemptOfflineLogin(restErrorIdentityGetUserPersona);
                }
            }

            void LoginRegistrationSession::onPersonasRetrieved()
            {
                SessionError error;
                IdentityPortalExpandedPersonasResponse* response = dynamic_cast<IdentityPortalExpandedPersonasResponse*>(sender());
                ORIGIN_ASSERT(response);
                if ( !response )
                {
                    attemptOfflineLogin (restErrorIdentityGetUserPersona);
                    return;
                }
                else
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onPersonasRetrieved()));
                    error = response->error();
                }

                if (error.restErrorCode() == restErrorSuccess)
                {
                    if (response->personaList().size() > 1)
                    {
                        ORIGIN_LOG_WARNING << "retrieveUserPersona returned multiple personas";
                    }

                    mAuthentication2Info.user.personaId = response->personaId();
                    mAuthentication2Info.user.originId = response->originId();

                    if (mAuthentication2Info.user.originId.isEmpty())
                        ORIGIN_LOG_ERROR << "empty id";
                    retrieveUserName();
                }
				else
				{
                    //  TELEMETRY
                    QString errorString = QString("onPersonasRetrieved: request failed=%1").arg(error.restErrorCode());
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());

                    ORIGIN_ASSERT(self().data() == this);

                    QString errorText(response->reply()->readAll());
                    ORIGIN_LOG_ERROR << "retrieveUserPersona request failed: " << error.restErrorCode() << ":" << errorText;

                    attemptOfflineLogin (error);
                    return;
				}
            }

            void LoginRegistrationSession::onUserPidRetrieved()
            {
                SessionError error;
                IdentityPortalUserResponse* response = dynamic_cast<IdentityPortalUserResponse*>(sender());
                ORIGIN_ASSERT(response);
                if ( !response )
                {
                    attemptOfflineLogin (restErrorIdentityGetUserPid);
                    return;
                }
                else
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onUserPidRetrieved()));
                    error = response->error();
                }

                if (error.restErrorCode() == restErrorSuccess)
                {
                    mAuthentication2Info.user.userId = response->pidId().toLongLong(); // nucleusId
                    mAuthentication2Info.user.email = response->email();
                    mAuthentication2Info.user.country = response->country();
                    mAuthentication2Info.user.dob = response->dob();

                    //anonymous is set at account creating and doesn't change
                    //underage is set based on current date - DOB
                    //per EADP's recommendation, use OR of the two
                    bool anonymous = response->anonymousPid().compare("true", Qt::CaseInsensitive) == 0;
                    bool underAge = response->underagePid().compare("true", Qt::CaseInsensitive) == 0;
                    mAuthentication2Info.user.isUnderage = (anonymous || underAge);
                   
                    QString locale = response->locale();

                    retrieveUserPersona();
                }
				else
				{
                    //  TELEMETRY
                    QString errorString = QString("onUserPidRetrieved: request failed=%1").arg(error.restErrorCode());
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());

                    ORIGIN_ASSERT(self().data() == this);

                    QString errorText(response->reply()->readAll());
                    ORIGIN_LOG_ERROR << "retrieveUserPid request failed: " << error.restErrorCode() << ":" << errorText;

                    attemptOfflineLogin (error);
                    return;
				}
            }

            void LoginRegistrationSession::extractCid()
            {
                if (mIdToken.isEmpty())
                {
                    ORIGIN_LOG_EVENT << "extractCid: idtoken is empty";
                    ORIGIN_ASSERT_MESSAGE(false, "extractCid: bad id token");

                    //set restError to something other than default success, what we're really interested in is the SessionErrorCode
                    SessionError error (restErrorUnknown);
                    error.overrideSessionErrorCode (SESSION_FAILED_EXTRACT_CID);
                    emit beginFinished(error, self());
                }
                else
                {
                    QStringList idTokenParts = mIdToken.split (".");
                    if (idTokenParts.size () > 1)
                    {
                        QByteArray idTokenArray (idTokenParts [1].toUtf8());
                        QByteArray idTokenDecodeArray = QByteArray::fromBase64 (idTokenArray);

                        QScopedPointer<INodeDocument> json(CreateJsonDocument());
                        if (json && json->Parse(idTokenDecodeArray.data()))
                        {
                            if (json->FirstAttribute())
                            {
                                do
                                {
                                    QString key = json->GetAttributeName();
                                    QString value = json->GetAttributeValue();
                                    if (key == "cid")
                                    {
                                        QStringList cids = value.split(",", QString::SkipEmptyParts);
                                        if (!cids.isEmpty())
                                        {
                                            mOfflineKey = cids.back();
                                            if (cids.size() > 1)
                                            {
                                                mBackupOfflineKey = cids.front();
                                            }
                                        }
                                    }
                                    else if (key == "login_as_invisible")
                                    {
                                        mAuthentication2Info.mIsLoginInvisible = value;
                                    }
                                }
                                while (json->NextAttribute());
                            }
                        }
                        mIdToken.clear();
                        ORIGIN_LOG_EVENT << "Logging in as invisible? = " << mAuthentication2Info.mIsLoginInvisible;
                        ORIGIN_LOG_DEBUG << "Offline Key = " << mOfflineKey;
                        logTokens(mAuthentication2Info.token);
                        retrieveTokens();
                    }
                    else
                    {
                        mIdToken.clear();
                        ORIGIN_LOG_ERROR << "Failed to extract CIDs, idToken incorrect format";
                        ORIGIN_ASSERT(false);

                        //set restError to something other than default success, what we're really interested in is the SessionErrorCode
                        SessionError error (restErrorUnknown);
                        error.overrideSessionErrorCode (SESSION_FAILED_EXTRACT_CID);
                        emit beginFinished(error, self());
                    }
                }
            }

            void LoginRegistrationSession::retrieveUserName()
            {
                                    
                IdentityPortalExpandedNameProfilesResponse* response = NULL;
                if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                {
                    response = IdentityPortalServiceClient::retrieveExpandedNameProfiles(mAuthentication2Info.token.mAccessToken, mAuthentication2Info.user.userId);
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "initiating user name fetch";
                    if (response)
                    {
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onUserNameRetrieved()));
                        QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                    }
                }
                else
                {
                    //  TELEMETRY
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", "retrieveUserName: bad access token");
                    ORIGIN_LOG_ERROR << "retrieveUserName: bad access token";
                    ORIGIN_ASSERT_MESSAGE(false, "retrieveUserName: bad access token");
                    attemptOfflineLogin (restErrorIdentityGetTokens);   //set token as the error since it's an invalid token
                    return;
                }

                if (response)
                {
                    response->setDeleteOnFinish(true);
                }
                //not fatal if response is NULL, since user profile name is optional
                else
                {
                    ORIGIN_LOG_ERROR << "retrieveUserName: Unable to create request";
                    ORIGIN_ASSERT_MESSAGE (false, "retrieveUserName: Unable to create request");
                }
            }

            void LoginRegistrationSession::retrieveAppSettings()
            {
                 AppSettingsResponse* response = NULL;
                 if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                 {
                     response = AtomServiceClient::appSettings(Session::SessionService::currentSession());
                     ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "initiating app settings fetch";
                     if (response)
                     {
                         ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onAppSettingsRetrieved()));
                         QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                     }
                 }
                 else
                 {
                     //  TELEMETRY
                     GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", "retrieveAppSettings: bad access token");

                     ORIGIN_LOG_EVENT << "session: bad access token";
                     ORIGIN_ASSERT_MESSAGE(false, "bad access token");
                 }
 
                 if (response)
                 {
                     response->setDeleteOnFinish(true);
                 }
                 else
                 {
                     ORIGIN_ASSERT(false);
                     emit beginFinished(restErrorAtomGetAppSettings, self());
                 }
            }

            void LoginRegistrationSession::onUserNameRetrieved()
            {
                SessionError error;
                IdentityPortalExpandedNameProfilesResponse* response = dynamic_cast<IdentityPortalExpandedNameProfilesResponse*>(sender());
                ORIGIN_ASSERT(response);

                if (!response)
                {
                    //not fatal since user name is optional
                    ORIGIN_LOG_EVENT << "onUserNameRetrieved - response is NULL but not FATAL";
                }
                else 
                {
                    error = response->error();
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onUserNameRetrieved()));
                }

                if (error.restErrorCode() != restErrorSuccess)
                {
                    //  TELEMETRY
                    QString errorString = QString("onUserNameRetrieved: request failed=%1").arg(error.restErrorCode());
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());

                    QString errorText(response->reply()->readAll());
                    ORIGIN_LOG_ERROR << "retrieveUserName request failed: " << errorText;
                    //not fatal since user name is optional, proceed to retrieve AppSettings
                }
                else if (response)
                {
                    mAuthentication2Info.user.firstName = response->firstName();
                    mAuthentication2Info.user.lastName = response->lastName();
                }
                retrieveAppSettings();
            }

            void LoginRegistrationSession::onAppSettingsRetrieved()
            {
                AppSettingsResponse* response = dynamic_cast<AppSettingsResponse*>(sender());
                ORIGIN_ASSERT(response);
 
                if (!response)
                {
                    attemptOfflineLogin (restErrorAtomGetAppSettings);
                    return;
                }
                else
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onAppSettingsRetrieved()));
                }

                SessionError error = response->error();
                if (error.restErrorCode() != restErrorSuccess)
                {
                    //  TELEMETRY
                    QString errorString = QString("onAppSettingsRetrieved: request failed=%1").arg(error.restErrorCode());
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());

                    QString errorText(response->reply()->readAll());
                    ORIGIN_LOG_ERROR << "retrieveAppSettings request failed: " << errorText;

                    //just set it to defaults and allow online login to proceed
                    mAuthentication2Info.appSettings.reset();
                }
                else
                {
                    mAuthentication2Info.appSettings = response->appSettings();
                }


                //manually call this here to write out to update the offline cache and saving off the user settings
                //rather than connecting to beginFinished signal because emitting beginFinished can trigger a relogin (in error cases) and loginComplete
                //could get called after the session has already been cleared (in error cases)
                loginComplete (restErrorSuccess);

                //logged in successfully, so hook up the signal to listen for authentication error from any REST calls
                connectAuthenticationError ();

                //also hook up connection to listen for cookie updates (remid, ttdid)
                connectSetCookie ();

                emit beginFinished(restErrorSuccess, self());
            }

            void LoginRegistrationSession::loginComplete(SessionError error)
            {
                if (error.restErrorCode() == restErrorSuccess &&
                    error.errorCode () == SESSION_NO_ERROR)
                {
                    mLoginTime = QDateTime::currentDateTime();
                    if (!mConfiguration2.isOfflineLogin())
                    {
                        if (!storeOfflineLoginInfo(mOfflineKey.toUtf8(), mAuthentication2Info))
                        {
                            //  TELEMETRY
                            GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogRef", "Unable to write to olc file");

                            ORIGIN_LOG_ERROR << "Unable to write to olc file.";
                        }
                    }
                }
            }

            void LoginRegistrationSession::onBeginFinished()
            {

            }

            void LoginRegistrationSession::onEndFinished()
            {
                emit endFinished(restErrorSuccess, self());
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "session ended, err = " << 0;
            }

            /// this SLOT should only be called when actual state change happens so we shouldn't have to check against previous value
            void LoginRegistrationSession::onClientConnectionStateChanged (Origin::Services::Connection::ConnectionStateField field)
            {
                //what is the current state
                bool isAwake = (Connection::ConnectionStatesService::connectionField (Connection::GLOB_OPR_IS_COMPUTER_AWAKE) == Connection::CS_TRUE);
                bool isConnected = (Connection::ConnectionStatesService::connectionField (Connection::GLOB_OPR_IS_INTERNET_REACHABLE) == Connection::CS_TRUE);
                bool isUpdatePending = (Connection::ConnectionStatesService::connectionField (Connection::GLOB_ADM_IS_REQUIRED_UPDATE_PENDING) == Connection::CS_TRUE);
                bool isManualOffline = (mIsUserManualOffline == Connection::CS_TRUE);

                //checking for state changes
                bool userWentOnline = ((field == Connection::USR_ADM_IS_USER_MANUAL_OFFLINE) && !mIsUserManualOffline);
                bool userWentOffline = ((field == Connection::USR_ADM_IS_USER_MANUAL_OFFLINE) && mIsUserManualOffline);
                bool computerAwoke = ((field == Connection::GLOB_OPR_IS_COMPUTER_AWAKE) && isAwake);
                bool computerGoingToSleep = ((field == Connection::GLOB_OPR_IS_COMPUTER_AWAKE) && !isAwake);
                bool internetDiscovered = ((field == Connection::GLOB_OPR_IS_INTERNET_REACHABLE) && isConnected);
                bool internetLost = ((field == Connection::GLOB_OPR_IS_INTERNET_REACHABLE) && !isConnected);
                bool updateNowPending = ((field == Connection::GLOB_ADM_IS_REQUIRED_UPDATE_PENDING) && isUpdatePending);


                if (userWentOnline)
                {
                    // user went online
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** user went online";
                    //check and make sure we can actually initiate a refresh; e.g. updatepending=false, etc.
                    if (canRefreshToken())  
                    {
                        emit relogin (self());
                    }
                }
                else if (userWentOffline)
                {
                    // user went offline
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** user went offline";
                    mIsSingleLoginCheckCompleted = Connection::CS_FALSE;        //so that they will be forced to go thru single login flow once they try to go back online
                    stopTimers();
                    mAuthentication2Info.token.mAccessToken.clear();

                    //for going offline manually, we don't need to set the TS because we'll be disconnecting to social
                    //so it shouldn't find the user already online, so normal flow thru single login should work

                    //to force a refresh of the sessionID when we go back online
                    mCurrentSidRefreshInterval = 1;

                    //check and see if we're already offline due to global concerns, if so, don't emit another offline signal
                    if (isAwake && isConnected && !isUpdatePending)
//                    if (mPreviouslyOnline == 1)      //if we were previously onlie
                    {
                        emit tokensChanged(AccessToken(mAuthentication2Info.token.mAccessToken), RefreshToken(mAuthentication2Info.token.mRefreshToken), SessionError());
                        emit authenticationStateChanged(SESSIONAUTHENTICATION_ENCRYPTED, self());
                    }
                }
                else if (computerAwoke)
                {
                    // user computer woke up
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** computer awoke";
                    //check to make sure we have internet connection before starting refreshToken, otherwise, just let it go and when internet comes back online, then it will initiate refreshToken
                    //check and make sure we can actually initiate a refresh; e.g. updatepending=false, etc.
                    if (canRefreshToken())  
                    {
                        emit relogin (self());
                    }
                }
                else if (computerGoingToSleep)
                {
                    // user computer going to sleep
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** computer going asleep";
                    mIsSingleLoginCheckCompleted = Connection::CS_FALSE;        //so that they will be forced to go thru single login flow once they try to go back online
                    stopTimers ();
                    mAuthentication2Info.token.mAccessToken.clear();

                    //keep track of when we went offline
                    setWentOfflineTime (QDateTime::currentDateTimeUtc());

                    //to force a refresh of the sessionID when we go back online
                    mCurrentSidRefreshInterval = 1;

                    //check and see if we're already offline, if so, don't emit another offline signal
                    if (isConnected && !isUpdatePending && !isManualOffline)
//                    if (mPreviouslyOnline == 1)
                    {
                        emit tokensChanged(AccessToken(mAuthentication2Info.token.mAccessToken), RefreshToken(mAuthentication2Info.token.mRefreshToken), SessionError());
                        emit authenticationStateChanged(SESSIONAUTHENTICATION_ENCRYPTED, self());
                    }
                }
                else if (internetDiscovered)
                {
                    // internet became reachable
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** internet became reachable";
                    //check and make sure we can actually initiate a refresh; e.g. updatepending=false, etc.
                    if (canRefreshToken())  
                    {
                        emit relogin (self());
                    }
                }
                else if (internetLost)
                {
                    // internet became unreachable
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** internet became unreachable";
                    mIsSingleLoginCheckCompleted = Connection::CS_FALSE;        //so that they will be forced to go thru single login flow once they try to go back online
                    stopTimers ();
                    mAuthentication2Info.token.mAccessToken.clear();

                    //keep track of when we went offline
                    setWentOfflineTime (QDateTime::currentDateTimeUtc());

                    //to force a refresh of the sessionID when we go back online
                    mCurrentSidRefreshInterval = 1;

                    emit tokensChanged(AccessToken(mAuthentication2Info.token.mAccessToken), RefreshToken(mAuthentication2Info.token.mRefreshToken), SessionError());

                    //check and see if we're already offline, if so, don't emit another offline signal
                    if (isAwake && !isUpdatePending && !isManualOffline)
                    {
                        emit authenticationStateChanged(SESSIONAUTHENTICATION_ENCRYPTED, self());
                    }
                }
                else if (updateNowPending)
                {
                    ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** update now pending";
                    mIsSingleLoginCheckCompleted = Connection::CS_FALSE;        //so that they will be forced to go thru single login flow once they try to go back online
                    stopTimers ();
                    mAuthentication2Info.token.mAccessToken.clear();

                    //keep track of when we went offline
                    setWentOfflineTime (QDateTime::currentDateTimeUtc());

                    //check and see if we're already offline, if so, don't emit another offline signal
                    if (mPreviouslyOnline == 1) //isConnected && isAwake && !isManualOffline)
                    {
                        emit tokensChanged(AccessToken(mAuthentication2Info.token.mAccessToken), RefreshToken(mAuthentication2Info.token.mRefreshToken), SessionError());
                        emit authenticationStateChanged(SESSIONAUTHENTICATION_ENCRYPTED, self());
                    }
                }
            }

            void LoginRegistrationSession::onRestAuthenticationFailed(Origin::Services::restError error)
            {
                ORIGIN_LOG_EVENT << "** received REST authentication failure, error = " << error;

                if (Session::SessionService::hasValidSession())
                {
                    // a REST service had an authentication error so try token extension
                    // lower our authentication level to reflect that we may be partially unauthenticated
                    if (/*mAuthenticationInfo.auth.isValid()*/ mAuthentication2Info.token.isValid())     //wasn't already cleared
                    {
                        mAuthentication2Info.token.reset();
                        emit authenticationStateChanged(SESSIONAUTHENTICATION_ENCRYPTED, self());
                    }
                    // restart token refresh
                    mRenewalTimer.stop();
                    renewAccessToken();
                }
                else
                    ORIGIN_LOG_EVENT << "session not valid, not calling refreshToken";
            }

            void LoginRegistrationSession::onRenewalTimerTimeout()
            {
                ORIGIN_LOG_EVENT << "Renewal timer timeout";
                renewAccessToken();
            }

            void LoginRegistrationSession::onExtendSidCookieResponse()
            {
                // Init our emitted status values to "unknown" for now.
                // HttpSuccess == 0 which is the closest (?) that we can get
                // to an "unknown" HTTP status code.
                Services::restError         restError   = Services::restErrorUnknown;
                Services::HttpStatusCode    httpStatus  = Services::HttpSuccess;

                // This response may have come in after user has exited or logged out, kill it here.
                if (!Session::SessionService::hasValidSession())
                {
                    ORIGIN_LOG_WARNING << "No valid session found when extendSidCookie response came in, ignoring response.";
                }
                else
                {
                    // If the response is a failure, we want to attempt another SID refresh after the next token refresh.
                    mCurrentSidRefreshInterval = 1;

                    AuthPortalSidCookieRefreshResponse* response = dynamic_cast<AuthPortalSidCookieRefreshResponse*>(sender());
                    if (response)
                    {
                        if (response->error() != restErrorSuccess)
                        {
                            ORIGIN_LOG_WARNING << "Sid cookie refresh request failed. Error = " << response->error();

                            QString errorString = QString("onExtendSidCookie: request failed=%1").arg(response->error());
                            GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());
                        }
                        else
                        {
                            mCurrentSidRefreshInterval = SID_REFRESH_INTERVAL;
                        }

                        restError   = response->error();
                        httpStatus  = response->httpStatus();
                    }
                    else
                    {
                        ORIGIN_LOG_WARNING << "The reply for sid cookie refresh was null";
                    }
                }

                // :WARN: This function must always emit this signal or else
                // we may cause undefined behavior in the JS-SDK.
                emit sidRenewalRequestCompleted(
                    static_cast<qint32>(restError),
                    static_cast<qint32>(httpStatus));
            }

            void LoginRegistrationSession::onExtendTokensResponse()
            {
                mRenewRequestTimeoutTimer.stop();
                if (mExtendTokenServiceResponse)
                {
                    ORIGIN_VERIFY_DISCONNECT(&mRenewRequestTimeoutTimer, SIGNAL (timeout()), mExtendTokenServiceResponse, SLOT(abort())); 
                }

                // This response may have come in after user has exited or logged out, kill it here.
                if (!Session::SessionService::hasValidSession())
                {
                    ORIGIN_LOG_WARNING << "No valid session found when extendtoken response came in, ignoring response.";
                }
                else
                {
                    restError error = restErrorUnknown;
                    if (NULL!=mExtendTokenServiceResponse)
                    {
                        error = mExtendTokenServiceResponse->error();

                        //  TELEMETRY:  Send error notification that refreshing token failed
                        if (error != restErrorSuccess || mExtendTokenServiceResponse->httpStatus() != Http200Ok)
                        {
                            QString errorString = QString("onExtendTokensResponse:REST=%1,HTTP=%2").arg(error).arg(mExtendTokenServiceResponse->httpStatus());
                            GetTelemetryInterface()->Metric_ERROR_NOTIFY("LogReg", errorString.toLocal8Bit().constData());
                        }
                        if (error == restErrorUnknown && mExtendTokenServiceResponse->httpStatus() == Http400ClientErrorBadRequest)
                        {
							ORIGIN_ASSERT_MESSAGE (0, "unhandled error in http400 response for refreshing token, setting error to restErrorNucleus2InvalidToken");
							ORIGIN_LOG_ERROR << "unhandled error in http400 response for refreshing token, setting error to restErrorNucleus2InvalidToken";
							//set it to generic one
                            error =  restErrorNucleus2InvalidToken;
                        }
                        mExtendTokenServiceResponse->deleteLater();
                    }
                    else
                    {
                        ORIGIN_LOG_EVENT << "onExtendTokenResponse: response is NULL, setting error to restErrorUnknown";
                        ORIGIN_ASSERT(mExtendTokenServiceResponse);
						error =  restErrorUnknown;
                    }

                    ORIGIN_LOG_EVENT << "extendToken response restError = " << error;
                    handleRefreshReloginResponse(error);
                }
                mExtendTokenServiceResponse = NULL;
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "** setting mExtendTokenServiceResponse to NULL";
            }

            void LoginRegistrationSession::handleRefreshReloginResponse(const restError& error)
            {
                ORIGIN_LOG_EVENT << "handleRefreshReloginResponse error: " << error;
                
                switch ( error )
                {
                    case restErrorSuccess:
                        {
                            doTokensChanged(mExtendTokenServiceResponse->tokens());
                            startRenewalTimer();

                            if (mAuthentication2Info.token.mAccessToken.isEmpty())
                            {
                                ORIGIN_LOG_ERROR << "Access token from relogin/renew response is EMPTY";
                            }

                            if (mAuthentication2Info.token.mRefreshToken.isEmpty())
                            {
                                ORIGIN_LOG_ERROR << "Refresh token from relogin/renew response is EMPTY";
                            }

                            // Refresh of the SID cookie is tied to a successful token refresh so that the 2 refreshes will not
                            // occur simultaneously.
                            --mCurrentSidRefreshInterval;
                            if (!mCurrentSidRefreshInterval)
                            {
                                renewSidCookie();
                            }
                        }
                        break;

                    case restErrorIdentityRefreshTokens:
					//Nucleus 2.0 authentication errors, these are considered fatal so there's no point in trying refresh, just log the user out
					case restErrorNucleus2InvalidRequest:
					case restErrorNucleus2InvalidClient:
					case restErrorNucleus2InvalidGrant: // error returned when refreshing an access token with an empty refresh token
					case restErrorNucleus2InvalidScope:
					case restErrorNucleus2UnauthorizedClient:
					case restErrorNucleus2UnsupportedGrantType:
                        {
                            stopTimers();
                            // Only clear the tokens and not the user info in case the user is unable to log out (playing a game,
                            // etc.). Keep the user info until the user is actually logged out (endAsync) so we can hold on to
                            // important user info, such as underage (EBIBUGS-27121).
                            mAuthentication2Info.token.reset();
                            emit tokensChanged(AccessToken(), RefreshToken(), SessionError(error));
                            emit authenticationStateChanged(SESSIONAUTHENTICATION_NONE, self());
					        break;
                        }
                    default:
                        // don't make us be offline until the auth token has actually expired and continue to retry
                        if ( mAccessTokenExpiryTime && time(NULL) > mAccessTokenExpiryTime )
                        {
                            mIsSingleLoginCheckCompleted = Connection::CS_FALSE;    //to force single login check once we get authenticated
                            mAccessTokenExpiryTime = 0;
							mAuthentication2Info.token.mAccessToken.clear();	//to force an emit to occur once we actually get the token
                            emit tokensChanged(AccessToken(), RefreshToken(mAuthentication2Info.token.mRefreshToken), SessionError(error));
                            emit authenticationStateChanged(SESSIONAUTHENTICATION_ENCRYPTED, self());
                        }

                        startFallbackRenewalTimer();
                        
                        break;
                }
            }

            void LoginRegistrationSession::onSetCookie (QString cookieName)
            {
                if (cookieName == Session::LOGIN_COOKIE_NAME_REMEMBERME || cookieName == LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE)
                {
                    QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());

                    QNetworkCookie c;
                    Origin::Services::NetworkCookieJar* cookieJar = Origin::Services::NetworkAccessManagerFactory::instance()->sharedCookieJar();
                    if (cookieJar->findFirstCookieByName(cookieName, c))
                    {
                        if (cookieName == Session::LOGIN_COOKIE_NAME_REMEMBERME)
                        {
                			const Origin::Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;
                            Origin::Services::saveCookieToSetting (rememberMeSetting, c);
                        }
                        else
                        {
                            const Origin::Services::Setting &tfaIdCookieSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_TFAID_PROD : Services::SETTING_TFAID_INT;
                            Origin::Services::saveCookieToSetting (tfaIdCookieSetting, c);
                        }
                    }
                }
            }


            LoginRegistrationSession::LoginRegistrationSession()
                : mOfflineKey("")
                , mAccessTokenExpiryTime(0)
                , mRetryCount(0)
                , mCurrentSidRefreshInterval(SID_REFRESH_INTERVAL)
                ,mExtendTokenServiceResponse(NULL)
                ,mIsUserManualOffline (Connection::CS_UNDEFINED)
                ,mIsSingleLoginCheckCompleted (Connection::CS_UNDEFINED)
                ,mPreviouslyOnline (-1)
                ,mClearOnFailure(true)
                ,mWentOfflineTime (QDateTime())
            {
                ORIGIN_VERIFY_CONNECT(
                    Network::GlobalConnectionStateMonitor::instance(), 
                    SIGNAL(connectionStateChanged (Origin::Services::Connection::ConnectionStateField)), 
                    this, 
                    SLOT(onClientConnectionStateChanged(Origin::Services::Connection::ConnectionStateField))
                    );

                mRenewalTimer.setSingleShot(true);
                ORIGIN_VERIFY_CONNECT(&mRenewalTimer, SIGNAL(timeout()), this, SLOT(onRenewalTimerTimeout()));
            }

            QDateTime LoginRegistrationSession::loginTime()
            {
                return mLoginTime;
            }

            bool LoginRegistrationSession::canRefreshToken()
            {
                //this function should be called prior to any calls to refreshAccessToken() to see if we should be attempting a refresh
                //We should NOT attempt a refresh if:
                //1.  update is pending
                //2.  no internet connection (tho this is up for discussion)
                //3.  computer is asleep
                //the three above conditions currently are being checked by isGlobalOnline() so we can check against that
                //if at some later point, we want to allow refresh attempt when internet is down, then we can check for each of the individual states
                if (!Origin::Services::Connection::ConnectionStatesService::isGlobalStateOnline())
                    return false;

                //4.  is manualofflinemode -- if the user is in manual offline mode, then we shouldn't be attempting refresh either
                if (mIsUserManualOffline == Connection::CS_TRUE)
                    return false;

                return true;
            }

            void LoginRegistrationSession::renewSidCookie()
            {
                if (!mAuthentication2Info.token.mAccessToken.isEmpty())
                {
                    AuthPortalSidCookieRefreshResponse* response = AuthPortalServiceClient::refreshSidCookie(mAuthentication2Info.token.mAccessToken);
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onExtendSidCookieResponse()));
                    QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                    response->setDeleteOnFinish(true);
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Access token is empty";
                }
            }

            void LoginRegistrationSession::renewAccessToken()
            {

                if (NULL!=mExtendTokenServiceResponse)
                {
                    ORIGIN_LOG_EVENT << "token request already in progress";
                    return;
                }

                ORIGIN_LOG_EVENT << "refreshing access token";
                if (mAuthentication2Info.token.mRefreshToken.isEmpty())
                {
                    ORIGIN_LOG_ERROR << "refresh token is EMPTY!"; 
                }
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "renewing with:[" << mAuthentication2Info.token.mRefreshToken << "]";

                mExtendTokenServiceResponse = AuthPortalServiceClient::extendTokens(mAuthentication2Info.token.mRefreshToken);

                ORIGIN_ASSERT(mExtendTokenServiceResponse);
                ORIGIN_VERIFY_CONNECT(mExtendTokenServiceResponse, SIGNAL(finished()), this, SLOT(onExtendTokensResponse()));
                ORIGIN_VERIFY_CONNECT(&mRenewRequestTimeoutTimer, SIGNAL (timeout()), mExtendTokenServiceResponse, SLOT(abort())); 

                //don't want to use QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                //because we need to be able to stop the timer when logging out
                mRenewRequestTimeoutTimer.setSingleShot(true);
                mRenewRequestTimeoutTimer.setInterval (NETWORK_TIMEOUT_MSECS);
                mRenewRequestTimeoutTimer.start();

                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "token renewal requested";
            }

            void LoginRegistrationSession::startRenewalTimer()
            {
                mRetryCount = 0;
                time_t now = time(NULL);
                quint64 expirySec = mAuthentication2Info.token.expiryTimeSecs();
                mAccessTokenExpiryTime = now + expirySec;
                const qint64 durationInMsec  = timerInterval(expirySec);
                mRenewalTimer.start(durationInMsec);
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "token renewal in " << durationInMsec << " milliseconds.";
            }

            namespace
            {
                const qint32 RETRY_THRESHOLD = 10;
                const qint32 BASE_RETRY_DURATION_IN_MSEC = 1000;
                /// bump this if the content of the offline login cache changes
                /// and old versions can no longer be parsed
                const qint32 LATEST_OFFLINE_LOGIN_CACHE_VERSION = 1;
            }

            void LoginRegistrationSession::startFallbackRenewalTimer()
            {
                // clamp retry count at the threshold of five
                qint32 maxedRetries = (mRetryCount < RETRY_THRESHOLD) ? mRetryCount : RETRY_THRESHOLD; // min
                // exponential fall-back of 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 1024, 1024 seconds....
                qint32 durationInMsec = BASE_RETRY_DURATION_IN_MSEC << maxedRetries;
                // add a random jitter on the order of 5% to keep clients from syncronising
                durationInMsec += (durationInMsec * (rand() % 500)) / 10000;
                mRenewalTimer.start(durationInMsec);
                ORIGIN_LOG_EVENT_IF(ENABLE_SESSION_LOGGING) << "token renewal retry in " << durationInMsec << " milliseconds.";
                ++mRetryCount;
            }

            void LoginRegistrationSession::stopTimers ()
            {
                mRenewalTimer.stop();
                mRenewRequestTimeoutTimer.stop();
            }

            // this function is called in response to an server request for the current profile
            void LoginRegistrationSession::refreshOriginId(Origin::Services::UserResponse* resp)
            {
                ORIGIN_ASSERT(resp);
                if ( resp )
                {
                    QString eaid = resp->user().originId;

                    ORIGIN_LOG_DEBUG << "refreshing user EAID:\"" << eaid << "\"";

                    if ( !eaid.isEmpty() )
                    {
                        mAuthentication2Info.user.originId = eaid;
                        // update setting used to initialize login screen, if the setting is currently 
                        // the eaid (not email address), update it with the new eaid
                        QString id = Services::readSetting(Origin::Services::SETTING_LOGINEMAIL);
                        if (!id.contains('@'))
                            Services::writeSetting(Origin::Services::SETTING_LOGINEMAIL, eaid);
                    }
                }
            }

            SessionError LoginRegistrationSession::attemptLoginViaOfflineCache()
            {
                QString saveOfflineKey (mOfflineKey);
                SessionError error = restErrorSuccess;

                QString hashedOfflineKey = QCryptographicHash::hash(mOfflineKey.toUtf8(), QCryptographicHash::Md5).toHex();
                Setting eaidMapping(
                    hashedOfflineKey,
                    Variant::String,
                    Variant(""),
                    Setting::LocalAllUsers,
                    Setting::ReadWrite,
                    Setting::Encrypted);
                QString mappedEaId(Origin::Services::readSetting(eaidMapping));
                mappedEaId = mappedEaId.toLower();

                if (mappedEaId.isEmpty())
                {
                    ORIGIN_LOG_WARNING << "Primary offline key was empty.";
                    hashedOfflineKey = QCryptographicHash::hash(mBackupOfflineKey.toUtf8(), QCryptographicHash::Md5).toHex();
                    Setting eaidMapping(
                        hashedOfflineKey,
                        Variant::String,
                        Variant(""),
                        Setting::LocalAllUsers,
                        Setting::ReadWrite,
                        Setting::Encrypted);
                    mappedEaId = Origin::Services::readSetting(eaidMapping).toString();
                    mappedEaId = mappedEaId.toLower();

                    if (mBackupOfflineKey.isEmpty())
                        ORIGIN_LOG_ERROR << "Backup offline key was empty";

                    if (mappedEaId.isEmpty())
                        ORIGIN_LOG_ERROR << "empty id";

                    mOfflineKey = mBackupOfflineKey;
                }

                mAuthentication2Info.reset();
                bool wasRetrieved = retrieveOfflineLoginInfo(mappedEaId, mOfflineKey.toUtf8(), mAuthentication2Info);
                if (!wasRetrieved)
                {
                    error = restErrorSessionOfflineAuthFailed;
                }
                else
                {
                    //we need to restore the original mOfflineKey since that will now become the backup key in the web cache
                    mOfflineKey = saveOfflineKey;
                    //if it was successful, we should reencrypt the offline cache with the current offlinekey to keep it in sync with the web cache
                    if (!storeOfflineLoginInfo(mOfflineKey.toUtf8(), mAuthentication2Info))
                    {
                        ORIGIN_ASSERT_MESSAGE (0, "Offline Login: Unable to re-encrypt offline cache");
                        ORIGIN_LOG_ERROR << "Offline Login: Unable to re-encrypt offline cache";
                    }
                }

                return error;
            }

            void LoginRegistrationSession::retrieveTokens()
            {
                ORIGIN_LOG_EVENT << "Initial token retrieval";
                GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::Authentication);
                
                AuthPortalTokensResponse* response = AuthPortalServiceClient::retrieveTokens(mAuthorizationCode);
                if (response)
                {
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onTokensRetrieved()));
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::restError)), this, SLOT(onTokensRetrievedError(Origin::Services::restError)));
                    QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                    response->setDeleteOnFinish(true);
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Unable to create retrieveTokens request";
                    attemptOfflineLogin(restErrorIdentityGetTokens);
                }
            }

            void LoginRegistrationSession::doTokensChanged(Origin::Services::TokensNucleus2 newToken)
            { 
                logTokens (newToken);

                //save off what was previously in mAuthenticationInfo
                Origin::Services::TokensNucleus2 prevToken = mAuthentication2Info.token;

                // update with the new tokens
                mAuthentication2Info.token = newToken;

                if (newToken.mRefreshToken.isEmpty())
                {
                    ORIGIN_LOG_ERROR << "doTokensChanged: empty refreshToken passed in";
                }

                //we can't do this check(for diff between prev & new),
                //because for initial login, we need to set mAccessToken to initial value or the code will think reauth has occurred (see emit authenticaonStateChanged below)
                //but in setting mAccessToken to initial value, if we leave the check (diff between prev & new), it won't emit tokensChanged
                //if ((prevToken.mAccessToken != mAuthentication2Info.token.mAccessToken)
                {
                    emit tokensChanged(
                        AccessToken(mAuthentication2Info.token.mAccessToken), 
                        RefreshToken(mAuthentication2Info.token.mRefreshToken),
                        SessionError());
                }

                //if this was just a timed refresh, then prevAuthToken won't be empty so it won't emit that the authentication state has changed
                //this will only get emitted if we've cleared the previous token (e.g. when we lose connection or go manually online or the retry attempts have timed out)
                if (prevToken.mAccessToken.isEmpty() && mAuthentication2Info.token.isValid())
                    emit authenticationStateChanged(SESSIONAUTHENTICATION_AUTH, self());
            }

            QString offlineLoginPath(QString const& key, bool makeDir)
            {
                // Windows 7 and XP return a valid path here, that includes the domain
                // (i.e. ea.com). The path is valid, and W7 has no problem with it.
                // XP however, can not store the file in a path with a domain embedded in it.
                QString sOfflineLoginPathRoot(PlatformService::commonAppDataPath());
                //if environment is not production, append the environment as the folder name
                QString envname = Origin::Services::readSetting(SETTING_EnvironmentName);
                if (envname.toLower() != SETTING_ENV_production)
                {
                    sOfflineLoginPathRoot += envname.toLower() + QDir::separator();
                }
                //if makeDir=true and the path does not exist, create it
                if (makeDir && !QDir(sOfflineLoginPathRoot).exists())
                {
                    QDir().mkpath(sOfflineLoginPathRoot);
                }
                QString hashedKey(key);
                QString offlineLoginPath(QString("%1") + QString("%2.olc"));
                return offlineLoginPath.arg(sOfflineLoginPathRoot).arg(hashedKey);
            }

            bool retrieveOfflineLoginInfo(
                QString const& user, 
                QByteArray const& offlineCacheKey,
                Authentication2& authInfo)
            {
                if (offlineCacheKey.isEmpty())
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("Empty offline cache key for read", "");
                    ORIGIN_LOG_ERROR << "Invalid offline cache key.";
                    return false;
                }

                // convert user ("my_origin_id" or it could be e-mail to "0123456789abcdef.olc"
                QString canonicalUserId(user.toLower());

                if (canonicalUserId.isEmpty())
                    ORIGIN_LOG_ERROR << "canonical user empty";

                QString hashedCanonicalUserId(QCryptographicHash::hash(canonicalUserId.toUtf8(), QCryptographicHash::Md5).toHex());
                QString path(offlineLoginPath(hashedCanonicalUserId));
                if (!QFile::exists(path))
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("OLC file not exist", path.toLocal8Bit().constData());
                    ORIGIN_LOG_EVENT << "retrieveOfflineLoginInfo" << " : " << QString("DOES NOT EXIST: ") + path;
                    ORIGIN_ASSERT(false);
                    return false;
                }

                QFile file(path);

                if (!file.open(QIODevice::ReadOnly))
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("OLC could not open", path.toLocal8Bit().constData());
                    ORIGIN_LOG_EVENT << "retrieveOfflineLoginInfo" << " : " << QString("COULD NOT OPEN: ") + path;
                    ORIGIN_ASSERT(false);
                    return false;
                }

                QByteArray contents;
                QByteArray encryptedContents = file.readAll();
                bool wasDecrypted = CryptoService::decryptSymmetric(contents, encryptedContents, offlineCacheKey, sOfflineCipher, sOfflineCipherMode);

                ORIGIN_ASSERT(wasDecrypted);
                Q_UNUSED(wasDecrypted);

                QDataStream stream(contents);
                stream.setVersion(QDataStream::Qt_4_8);

                int offlineLoginCacheVersion;
                stream >> offlineLoginCacheVersion;
                if (offlineLoginCacheVersion != LATEST_OFFLINE_LOGIN_CACHE_VERSION)
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("OLC mismatch version", path.toLocal8Bit().constData());
                    return false;
                }

                stream >> authInfo.user.userId >> authInfo.user.originId >> authInfo.user.country;
                stream >> authInfo.user.isUnderage;
                stream >> authInfo.user.personaId;
                stream >> authInfo.user.dob;

                file.close();

                return true;
            }

            bool storeOfflineLoginInfo(
                QByteArray const& offlineCacheKey,
                Authentication2 const& authInfo)
            {
                if (offlineCacheKey.isEmpty())
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("Empty offline cache key for write", "");
                    ORIGIN_LOG_ERROR << "Invalid offline cache key.";
                    return false;
                }

                // convert user ("my_origin_id") to "0123456789abcdef.olc"
                //to handle the case when the userid is the same in both PROD and non-PROD environments, we need to differentiate the setting value
                //leave prod as it was, with no extension to preserve live user settings
                //append "Int" in non-prod; use that also as a key to the .olc
                QString originId = authInfo.user.originId;
                QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());

                if (envname != Services::SETTING_ENV_production)
                    originId += "Int";

                QString canonicalUserId(originId.toLower());
                QString hashedCanonicalUserId(QCryptographicHash::hash(canonicalUserId.toUtf8(), QCryptographicHash::Md5).toHex());

                QString path(offlineLoginPath(hashedCanonicalUserId, true /*makeDir*/));

                if (authInfo.user.originId.isEmpty())
                    ORIGIN_LOG_ERROR << "empty id";
                if (canonicalUserId.isEmpty())
                    ORIGIN_LOG_ERROR << "empty canonical id";

                QFile file(path);

                if (!file.open(QIODevice::WriteOnly))
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("OLC not open for write", path.toLocal8Bit().constData());
                    ORIGIN_LOG_EVENT << "storeOfflineLoginInfo" << " : " << QString("COULD NOT OPEN: "); // + path;
                    ORIGIN_ASSERT(false);
                    return false;
                }
                else
                {
                    ORIGIN_LOG_EVENT << "storeOfflineLoginInfo" << " : " << QString ("Opened successfully: "); // + path;
                }

                // login information to store for offline login
                QByteArray contents;
                QDataStream stream(&contents, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Qt_4_8);
                stream << LATEST_OFFLINE_LOGIN_CACHE_VERSION;
                stream << authInfo.user.userId << authInfo.user.originId << authInfo.user.country;
                stream << authInfo.user.isUnderage;
                stream << authInfo.user.personaId;
                stream << authInfo.user.dob;

                QByteArray encryptedContents;
                bool wasEncrypted = CryptoService::encryptSymmetric(encryptedContents, contents, offlineCacheKey, sOfflineCipher, sOfflineCipherMode);
                ORIGIN_ASSERT(wasEncrypted);
                Q_UNUSED(wasEncrypted);

                quint64 numBytesWritten = file.write(encryptedContents);
                ORIGIN_ASSERT(numBytesWritten == static_cast<quint64>(encryptedContents.count()));
                Q_UNUSED(numBytesWritten);

                file.close();

                removeOfflineLoginSettings(originId);
                addOfflineLoginSettings(originId, offlineCacheKey);

                return true;
            }

            void removeOfflineLoginSettings(const QString& eaid)
            {
                QString hashedId = QCryptographicHash::hash(eaid.toLower().toUtf8(), QCryptographicHash::Md5).toHex();
                Setting offlineKeyMapping(
                    hashedId,
                    Variant::String,
                    Variant(""),
                    Setting::LocalAllUsers,
                    Setting::ReadWrite,
                    Setting::Encrypted);
                QString mappedOfflineKey(Origin::Services::readSetting(offlineKeyMapping));

                QString hashedOfflineKey = QCryptographicHash::hash(mappedOfflineKey.toUtf8(), QCryptographicHash::Md5).toHex();
                Setting eaidMapping(
					hashedOfflineKey,
					Variant::String,
					Variant(""),
					Setting::LocalAllUsers,
					Setting::ReadWrite,
					Setting::Encrypted);
				Origin::Services::writeSetting(eaidMapping,"");
            }

            void addOfflineLoginSettings(const QString& eaid, QByteArray const& offlineCacheKey)
            {
                QString hashedOfflineKey = QCryptographicHash::hash(offlineCacheKey, QCryptographicHash::Md5).toHex();
				Setting eaidMapping(
					hashedOfflineKey,
					Variant::String,
					Variant(""),
					Setting::LocalAllUsers,
					Setting::ReadWrite,
					Setting::Encrypted);
				Origin::Services::writeSetting(eaidMapping, eaid.toLower());

                if (eaid.toLower().isEmpty())
                    ORIGIN_LOG_ERROR << "empty id";

                QString hashedId = QCryptographicHash::hash(eaid.toLower().toUtf8(), QCryptographicHash::Md5).toHex();
                Setting offlineKeyMapping(
                    hashedId,
                    Variant::String,
                    Variant(""),
                    Setting::LocalAllUsers,
                    Setting::ReadWrite,
                    Setting::Encrypted);
                Origin::Services::writeSetting(offlineKeyMapping, QString(offlineCacheKey));
            }

            void LoginRegistrationSession::connectAuthenticationError ()
            {
                ORIGIN_VERIFY_CONNECT(
                    RestAuthenticationFailureNotifier::instance(),
                    SIGNAL(authenticationError(Origin::Services::restError)),
                    this,
                    SLOT(onRestAuthenticationFailed(Origin::Services::restError)));

            }

            void LoginRegistrationSession::disconnectAuthenticationError ()
            {
                ORIGIN_VERIFY_DISCONNECT(
                    RestAuthenticationFailureNotifier::instance(),
                    SIGNAL(authenticationError(Origin::Services::restError)),
                    this,
                    SLOT(onRestAuthenticationFailed(Origin::Services::restError)));
            }

            void LoginRegistrationSession::connectSetCookie ()
            {
                ORIGIN_VERIFY_CONNECT(
                    Origin::Services::NetworkAccessManagerFactory::instance()->sharedCookieJar(),
                    SIGNAL(setCookie(QString)),
                    this,
                    SLOT (onSetCookie(QString)));
            }

            void LoginRegistrationSession::disconnectSetCookie ()
            {
                ORIGIN_VERIFY_DISCONNECT(
                    Origin::Services::NetworkAccessManagerFactory::instance()->sharedCookieJar(),
                    SIGNAL(setCookie(QString)),
                    this,
                    SLOT (onSetCookie(QString)));
            }

            void LoginRegistrationSession::setWentOfflineTime (QDateTime offlineTime)
            {
                mWentOfflineTime = offlineTime;
            }
        } // namespace Session
    } // namespace Services
} // namespace Origin
