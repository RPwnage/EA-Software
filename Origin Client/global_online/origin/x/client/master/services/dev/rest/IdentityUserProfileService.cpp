///////////////////////////////////////////////////////////////////////////////
// IdentityUserProfileService.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/log/LogService.h"
#include "services/rest/IdentityUserProfileService.h"
#include "services/rest/AtomServiceClient.h"
#include "services/rest/AtomServiceResponses.h"
#include "services/rest/IdentityPortalServiceClient.h"
#include "services/rest/IdentityPortalServiceResponses.h"
#include "services/debug/DebugService.h"
#include "services/session/AbstractSession.h"


#if defined(_DEBUG)
#define ENABLE_PERSONA_LOGGING 0
#else
// ensure that the following remain at 0 so that we never log them in release builds
#define ENABLE_PERSONA_LOGGING 0
#endif


namespace Origin
{
    namespace Services
    {
        IdentityUserProfileService::IdentityUserProfileService(QObject *parent /*= 0*/ )
            : NotificationServiceBase(parent)
            , mRestError(restErrorUnknown)
            , mHttpResponseCode(503)
            , mHttpErrorDescription("IdentityUserProfileService unavailable")
        {

        }

        IdentityUserProfileService::~IdentityUserProfileService()
        {

        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByEmailorEAID(Session::SessionRef session, const QString& key)
        {
            if (key.indexOf('@') > 0)
            {
                return OriginClientFactory<IdentityUserProfileService>::instance()->userProfileByEmailPriv(
                    Session::SessionService::accessToken(session), key);
            }
            else
            {
                return OriginClientFactory<IdentityUserProfileService>::instance()->userProfileByEAIDPriv(
                    Session::SessionService::accessToken(session), key);
            }
        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByPid(const QString& accessToken, qint64 userPid)
        {
            return OriginClientFactory<IdentityUserProfileService>::instance()->userProfileByPidPriv(accessToken, userPid);
        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByEAID(const QString& accessToken, const QString& eaid)
        {
            return OriginClientFactory<IdentityUserProfileService>::instance()->userProfileByEAIDPriv(accessToken, eaid);
        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByEmail(const QString& accessToken, const QString& email)
        {
           return OriginClientFactory<IdentityUserProfileService>::instance()->userProfileByEmailPriv(accessToken, email);
        }

        IdentityUserProfileService* IdentityUserProfileService::updateUserName(Session::SessionRef session, const QString& firstName, const QString& lastName)
        {
            return OriginClientFactory<IdentityUserProfileService>::instance()->updateUserNamePriv(Session::SessionService::accessToken(session),
                Session::SessionService::nucleusUser(session), firstName, lastName);
        }

        restError IdentityUserProfileService::getRestError() const
        {
            return mRestError;
        }

        int IdentityUserProfileService::httpResponseCode() const
        {
            return mHttpResponseCode;
        }

        QString IdentityUserProfileService::httpErrorDescription() const
        {
            return mHttpErrorDescription;
        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByPidPriv(const QString& accessToken, qint64 userPid)
        {
            QList<quint64> userIds;
            userIds.push_back(userPid);

            QList<UserResponse*> responses = AtomServiceClient::user(Session::SessionService::currentSession(), userIds);
            if(responses.size()>0)
            {
                // We are only interested in the first response.
                ORIGIN_VERIFY_CONNECT(responses[0], SIGNAL(finished()), this, SLOT(onUserResponseReceived()));
            }
            return this;
        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByEAIDPriv(const QString& accessToken, const QString& eaid)
        {
            IdentityPortalPersonasResponse* response = IdentityPortalServiceClient::personasByEAID(accessToken, eaid);
            if (response)
            {
                mUserInfo.insert("EAID", eaid);
                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onPersonaReceived()));
            }

            return this;
        }

        IdentityUserProfileService* IdentityUserProfileService::userProfileByEmailPriv(const QString& accessToken, const QString& email)
        {
            mAccessToken = accessToken;

            IdentityPortalUserIdResponse* response = IdentityPortalServiceClient::pidByEmail(accessToken, email);
            if (response)
            {
                mUserInfo.insert("EMAIL", email);
                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onPidReceived()));
            }

            return this;
        }

        IdentityUserProfileService* IdentityUserProfileService::updateUserNamePriv(const QString& accessToken, const QString& pid, const QString& firstName, const QString& lastName)
        {
            mAccessToken = accessToken;
            mTempStorage.insert("firstname", firstName);
            mTempStorage.insert("lastname", lastName);

            IdentityPortalUserProfileUrlsResponse* response = IdentityPortalServiceClient::retrieveUserProfileUrls(accessToken, pid.toLongLong(),
                IdentityPortalUserProfileUrlsResponse::Name);
            if (response)
            {
                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onNameProfileUrlReceived()));
            }

            return this;
        }

        void IdentityUserProfileService::onUserResponseReceived()
        {
            UserResponse* response = dynamic_cast<UserResponse*>(sender());
            if (!response)
            {
                mRestError = restErrorIdentityGetUserProfile;
                mHttpResponseCode = 503;
                mHttpErrorDescription = "IdentityUserProfileService unavailable";

                emit userProfileError(mRestError);
                return;
            }

            if(response->users().size()>0)
            {
                // Return the first user.
                const Origin::Services::User & user = response->users()[0];

                mUserInfo.insert("USERPID", QString::number(user.userId));
                mUserInfo.insert("EAID", user.originId);
                mUserInfo.insert("PERSONAID", QString::number(user.personaId));
                mUserInfo.insert("FIRSTNAME", user.firstName);
                mUserInfo.insert("LASTNAME", user.lastName);
            }
            else
            {
                mUserInfo.insert("USERPID", QString::number(response->user().userId));
                mUserInfo.insert("EAID", response->user().originId);
                mUserInfo.insert("PERSONAID", QString::number(response->user().personaId));
                mUserInfo.insert("FIRSTNAME", response->user().firstName);
                mUserInfo.insert("LASTNAME", response->user().lastName);
            }

            emit userProfileFinished();

            response->setDeleteOnFinish(true);
        }

        void IdentityUserProfileService::onPidReceived()
        {
            IdentityPortalUserIdResponse* response = dynamic_cast<IdentityPortalUserIdResponse*>(sender());
            if (!response)
            {
                mRestError = restErrorIdentityGetUserProfile;
                mHttpResponseCode = 503;
                mHttpErrorDescription = "IdentityUserProfileService unavailable";

                emit userProfileError(mRestError);
                return;
            }

            mRestError = response->error();
            mHttpResponseCode = response->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            mHttpErrorDescription = response->reply()->errorString();

            if (response->error() != restErrorSuccess)
            {
                emit userProfileError(response->error());
            }
            else
            {
                userProfileByPidPriv(mAccessToken, response->userId());
            }

            response->setDeleteOnFinish(true);
        }

        void IdentityUserProfileService::onPersonaReceived()
        {
            IdentityPortalPersonasResponse* response = dynamic_cast<IdentityPortalPersonasResponse*>(sender());
            if (!response)
            {
                mRestError = restErrorIdentityGetUserProfile;
                mHttpResponseCode = 503;
                mHttpErrorDescription = "IdentityUserProfileService unavailable";

                emit userProfileError(mRestError);
                return;
            }

            mRestError = response->error();
            mHttpResponseCode = response->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            mHttpErrorDescription = response->reply()->errorString();

            if (response->error() != restErrorSuccess)
            {
                emit userProfileError(response->error());
            }
            else
            {
                QString uri(response->personaUri()); // format = /pids/{userid}/personas/{personaid}
                uri.replace("\\", "/");

                QStringList tokens = uri.split("/", QString::SkipEmptyParts);
                if (tokens.size() >= 4)
                {
                    mUserInfo.insert("USERPID", tokens.at(1));
                    mUserInfo.insert("PERSONAID", tokens.at(3));
                    emit userProfileFinished();
                }
                else if (tokens.size() < 2)
                {
                    mRestError = restErrorIdentityGetUserProfile;
                    mHttpResponseCode = 503;
                    mHttpErrorDescription = "IdentityUserProfileService unavailable";

                    ORIGIN_LOG_ERROR << "Persona uri did not contain a userId or personaId: " << uri;
                    emit userProfileError(mRestError);
                }
                else if (tokens.size() < 4)
                {
                    mRestError = restErrorIdentityGetUserProfile;
                    mHttpResponseCode = 503;
                    mHttpErrorDescription = "IdentityUserProfileService unavailable";

                    ORIGIN_LOG_ERROR << "Persona uri did not contain a personaId: " << uri;
                    emit userProfileError(mRestError);
                }

            }

            response->setDeleteOnFinish(true);
        }

        void IdentityUserProfileService::onNameProfileUrlReceived()
        {
            IdentityPortalUserProfileUrlsResponse* response = dynamic_cast<IdentityPortalUserProfileUrlsResponse*>(sender());

            if (!response)
            {
                mRestError = restErrorIdentitySetUserProfile;
                mHttpResponseCode = 503;
                mHttpErrorDescription = "IdentityUserProfileService unavailable";

                emit userProfileError(mRestError);
                return;
            }

            mRestError = response->error();
            mHttpResponseCode = response->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            mHttpErrorDescription = response->reply()->errorString();

            if (response->error() != restErrorSuccess)
            {
                emit userProfileError(response->error());
            }
            else
            {
                QString servicePath;
                if (!response->profileInfo()[IdentityPortalUserProfileUrlsResponse::Name].isEmpty())
                {
                    servicePath = response->profileInfo()[IdentityPortalUserProfileUrlsResponse::Name].first();
                }

                QString firstName = mTempStorage["firstname"];
                QString lastName = mTempStorage["lastname"];

                IdentityPortalUserProfileUrlsResponse* response2 = IdentityPortalServiceClient::updateUserName(servicePath, mAccessToken,
                    firstName, lastName);
                if (response2)
                {
                    ORIGIN_VERIFY_CONNECT(response2, SIGNAL(finished()), this, SLOT(onUserNameUpdated()));
                }
                else
                {
                    emit userProfileError(restErrorIdentitySetUserProfile);
                }
            }

            response->setDeleteOnFinish(true);
        }

        void IdentityUserProfileService::onUserNameUpdated()
        {
            IdentityPortalUserProfileUrlsResponse* response = dynamic_cast<IdentityPortalUserProfileUrlsResponse*>(sender());
            if (!response)
            {
                mRestError = restErrorIdentitySetUserProfile;
                mHttpResponseCode = 503;
                mHttpErrorDescription = "IdentityUserProfileService unavailable";

                emit userProfileError(restErrorUnknown); //TODO create restErrorIdentityPortal
                return;
            }

            mRestError = response->error();
            mHttpResponseCode = response->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            mHttpErrorDescription = response->reply()->errorString();

            if (mRestError == restErrorSuccess)
            {
                emit userProfileFinished();
            }
            else
            {
                emit userProfileError(mRestError);
            }

            response->setDeleteOnFinish(true);
        }

    }
}


