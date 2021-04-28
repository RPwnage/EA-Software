//  Setting.h
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.

#include "services/session/SessionService.h"
#include "services/session/LoginRegistrationSession.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/rest/AtomServiceClient.h"
#include "services/rest/AtomServiceResponses.h"

namespace Origin
{
    namespace Services
    {
        namespace Session
        {
            QPointer<SessionService> sInstance;

            inline bool isInitialized()
            {
                return !sInstance.isNull();
            }

            void SessionService::init()
            {
                // check to see whether we're already initialized
                if (sInstance.data())
                    return;

                sInstance = new SessionService;
            }

            void SessionService::release()
            {
                if (sInstance.isNull())
                    return;

                delete sInstance.data();
                sInstance = NULL;
            }

            SessionService* SessionService::instance()
            {
                ORIGIN_ASSERT(isInitialized());

                return sInstance.data();
            }

            void SessionService::beginSessionAsync2(SessionFactory factory, AbstractSessionConfiguration const& configuration)
            {
                ORIGIN_ASSERT(isInitialized());

                SessionRef newSession = createSession(factory);
                if (isValidSession(newSession))
                {
                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(beginFinished(SessionError, SessionRef)),
                        instance(), SLOT(onBeginFinished(SessionError, SessionRef)));

                    newSession->beginAsync(configuration);
                }
            }

            void SessionService::beginSessionAsync(SessionFactory factory, AbstractSessionConfiguration const& configuration)
            {
                ORIGIN_ASSERT(isInitialized());

                SessionRef newSession = createSession(factory);
                if (isValidSession(newSession))
                {
                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(beginFinished(SessionError, SessionRef)),
                        instance(), SLOT(onBeginFinished(SessionError, SessionRef)));

                    // hook up session renewal notifications
                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(tokensChanged(Origin::Services::Session::AccessToken, Origin::Services::Session::RefreshToken, Origin::Services::Session::SessionError)),
                        instance(), SLOT(onSessionTokensChanged(Origin::Services::Session::AccessToken, Origin::Services::Session::RefreshToken, Origin::Services::Session::SessionError)));

                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(authenticationStateChanged(Origin::Services::Session::AuthenticationState, Origin::Services::Session::SessionRef)),
                        instance(), SIGNAL(authenticationStateChanged(Origin::Services::Session::AuthenticationState, Origin::Services::Session::SessionRef)));

                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(userOnlineAllChecksCompleted (bool, Origin::Services::Session::SessionRef)),
                        instance(), SIGNAL(userOnlineAllChecksCompleted(bool, Origin::Services::Session::SessionRef)));

                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(relogin (Origin::Services::Session::SessionRef)), 
                        instance(), SIGNAL( relogin (Origin::Services::Session::SessionRef)));

                    ORIGIN_VERIFY_CONNECT(
                        newSession.data(), SIGNAL(sidRenewalRequestCompleted(qint32, qint32)),
                        instance(), SIGNAL(sidRenewalResponse(qint32, qint32)));

                    newSession->beginAsync(configuration);
                }
            }

            void SessionService::updateSessionAsync(SessionRef session, AbstractSessionConfiguration const& configuration)
            {
                ORIGIN_ASSERT(isInitialized());

                if (isValidSession(session))
                {
                    LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                    ORIGIN_ASSERT(lrSession);
                    if (lrSession)
                    {
                        lrSession->updateAsync(configuration);
                    }
                    else
                    {
                        ORIGIN_LOG_EVENT << "Calling updateSessionAsync with empty session";
                    }
                }
            }

            void SessionService::renewAccessTokenAsync(SessionRef session)
            {
                if (isInitialized() && isValidSession(session))
                    session->renewAccessToken();
            }

            void SessionService::endSessionAsync(SessionRef session)
            {
                ORIGIN_ASSERT(isInitialized());

                ORIGIN_VERIFY_CONNECT(session.data(), SIGNAL(endFinished(SessionError, SessionRef)), instance(), SLOT(onEndFinished(SessionError, SessionRef)));
                session->endAsync();
            }

            void SessionService::refreshOriginIdFromServerAsync(SessionRef session)
            {
                UserResponse* resp = AtomServiceClient::user(session);
                ORIGIN_ASSERT(resp);
                if (NULL != resp)
                {
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), instance(), SLOT(onRefreshOriginIdReply()));
                }
            }

            void SessionService::onRefreshOriginIdReply()
            {
                Origin::Services::UserResponse* resp = dynamic_cast<Origin::Services::UserResponse*>(sender());
                ORIGIN_ASSERT(resp);
                if(resp && resp->error() == Origin::Services::restErrorSuccess)
                {
                    LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(mCurrentSession.data());
                    ORIGIN_ASSERT(lrSession);
                    if (lrSession)
                    {
                        lrSession->refreshOriginId(resp);
                        emit userOriginIdUpdated(resp->user().originId, mCurrentSession);
                    }
                    else
                    {
                        ORIGIN_LOG_EVENT << "response received after session has been cleared";
                    }
                }
                resp->deleteLater();
            }

            bool SessionService::isValidSession(SessionRef session)
            {
                ORIGIN_ASSERT(isInitialized());

                return !session.isNull() && instance() && (session == instance()->mCurrentSession);
            }

            Origin::Services::Session::AuthenticationState SessionService::authenticationState(SessionRef session)
            {
                ORIGIN_ASSERT(isInitialized());

                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                return (lrSession != NULL) ? lrSession->authenticationState() : SESSIONAUTHENTICATION_NONE;
            }

            bool SessionService::hasValidSession()
            {
                ORIGIN_ASSERT(isInitialized());

                return instance() && instance()->mCurrentSession;
            }

            SessionRef SessionService::currentSession()
            {
                ORIGIN_ASSERT(isInitialized());

                return instance()->mCurrentSession;
            }

            void SessionService::updateCurrentUserEmail(SessionRef session, const QString& email)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession != NULL)
                {
                    lrSession->mAuthentication2Info.user.email = email;
                    emit userEmailUpdated(email, session);
                }
            }

            void SessionService::updateCurrentUserOriginId(SessionRef session, const QString& originId)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession != NULL)
                {
                    lrSession->mAuthentication2Info.user.originId = originId;
                    emit userOriginIdUpdated(originId, session);
                }
            }

            AccessToken SessionService::accessToken(SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession == NULL)
                    // return an empty token
                    return AccessToken();
                return AccessToken(lrSession->accessToken());
            }

            RefreshToken SessionService::refreshToken(SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession == NULL)
                    // return an empty token
                    return RefreshToken();
                return RefreshToken(lrSession->refreshToken());
            }

            void SessionService::renewSid(SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession) 
                {
                    lrSession->renewSidCookie();
                }
            }

            QString SessionService::nucleusUser(SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession == NULL)
                    // return an empty string
                    return QString();
                return lrSession->nucleusUser();
            }

            QString SessionService::nucleusPersona(SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession == NULL)
                {
                    return QString();
                }
                return lrSession->nucleusPersona();
            }

            QDateTime SessionService::loginTime(SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if(lrSession == NULL)
                {
                    return QDateTime();
                }
                return lrSession->loginTime();
            }

            void SessionService::setConnectionField (SessionRef session, Origin::Services::Connection::ConnectionStateField field, Origin::Services::Connection::ConnectionStatus status)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    lrSession->setConnectionField (field, status);
                }
            }

            Origin::Services::Connection::ConnectionStatus SessionService::connectionField (SessionRef session, Origin::Services::Connection::ConnectionStateField field)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    return (lrSession->connectionField (field));
                }
                else
                {
                    return Origin::Services::Connection::CS_UNDEFINED;
                }
            }

            bool SessionService::isOnline (SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    return (lrSession->isOnline());
                }
                return false;
            }

            void SessionService::setPreviouslyOnline (SessionRef session, bool online)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    lrSession->setPreviouslyOnline (online);
                }
            }

            int SessionService::previouslyOnline (SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    return lrSession->previouslyOnline();
                }
                return false; //default to false
            }

            void SessionService::resetWentOfflineTime (SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    lrSession->setWentOfflineTime (QDateTime());
                }
            }

            QDateTime SessionService::wentOfflineTime (SessionRef session)
            {
                LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(session.data());
                ORIGIN_ASSERT(lrSession);
                if (lrSession != NULL)
                {
                    return lrSession->wentOfflineTime ();
                }
                return QDateTime();
            }

            void SessionService::onBeginFinished(SessionError error, SessionRef /*session*/)
            {
                AbstractSession* session = dynamic_cast<AbstractSession*>(sender());
                ORIGIN_ASSERT_MESSAGE(session, "Received unexpected signal.");

                LoginRegistrationConfiguration2 const* lrConfig = dynamic_cast<LoginRegistrationConfiguration2 const*>(&session->configuration());
                ORIGIN_ASSERT(lrConfig);
				
                if (!lrConfig)
                {
                    ORIGIN_LOG_ERROR << "session configuration is empty";
                }

                if (error == SESSION_NO_ERROR)
                {
                    LoginRegistrationSession* lrSession = dynamic_cast<LoginRegistrationSession*>(mCurrentSession.data());
                    ORIGIN_ASSERT(lrSession);

                    if (lrSession)
                    {
                        // stash away some temporary, per-user settings
                        // If the atom /appSettings call returns anything relevant in the future, add parsing here.
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "mCurrentSession is empty";
                    }
                }
                else
                {
                    //NOTE: if we restore in-client re-login, we need to make sure to prevent clearing of the session in that case, otherwise a new session will be allocated
                    //since the login attemp was failure, we need to destroy the session so it doesn't get leaked
                    mCurrentSession.clear();
                }

				// Null check for static code analysis.
				if(lrConfig != NULL)
				{
					QSharedPointer<AbstractSessionConfiguration> config(new LoginRegistrationConfiguration2(*lrConfig));
					emit beginSessionComplete(error, mCurrentSession, config);
				}
            }

            void SessionService::onEndFinished(SessionError error, SessionRef session)
            {
                mCurrentSession.clear();

                emit endSessionComplete(error, session);
            }

            void SessionService::onSessionTokensChanged(AccessToken accessToken, RefreshToken refreshToken, Session::SessionError error)
            {
                emit sessionTokensChanged(mCurrentSession, accessToken, refreshToken, error);
            }

            SessionService::SessionService()
            {
            }

            SessionRef SessionService::createSession(SessionFactory factory)
            {
                ORIGIN_ASSERT(instance()->mCurrentSession.isNull());

                SessionRef newSession = factory();
                if (!newSession.isNull())
                {
                    instance()->mCurrentSession = SessionRef(newSession);
                }
                return instance()->mCurrentSession;
            }
        } // namespace Session
    } // namespace Services
} // namespace Origin
