///////////////////////////////////////////////////////////////////////////////
// IdentityPersonaService.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _IDENTITYUSERPROFILESERVICE_H_INCLUDED_
#define _IDENTITYUSERPROFILESERVICE_H_INCLUDED_

#include "services/rest/NotificationServiceBase.h"
#include "services/plugin/PluginAPI.h"

#include <QMap>
namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API IdentityUserProfileService : public NotificationServiceBase
        {

        Q_OBJECT
        friend class OriginClientFactory<IdentityUserProfileService>;

        public:

            /// \brief DTOR
            ~IdentityUserProfileService();

            // UserProfile keys: EAID, EMAIL, PID, PERSONAID
            // email key will only be populated if ByEmail method is executed and eaid will not be populated
            static IdentityUserProfileService* userProfileByEmailorEAID(Session::SessionRef session, const QString& key);
            static IdentityUserProfileService* userProfileByPid(const QString& accessToken, qint64 userPid);
            static IdentityUserProfileService* userProfileByEAID(const QString& accessToken, const QString& eaid);
            static IdentityUserProfileService* userProfileByEmail(const QString& accessToken, const QString& email);


            static IdentityUserProfileService* updateUserName(Session::SessionRef session, const QString& firstName, const QString& lastName);

            restError getRestError() const;
            int httpResponseCode() const;
            QString httpErrorDescription() const;

signals:
            /// \brief emitted when error is detected
            void userProfileError(Origin::Services::restError);

            /// \brief emitted when request has finished without error
            void userProfileFinished();

        private slots:

            void onPidReceived();
            void onPersonaReceived();
            void onNameProfileUrlReceived();
            void onUserNameUpdated();
            void onUserResponseReceived();

        private:

            /// \brief CTOR
            explicit IdentityUserProfileService(QObject *parent = 0);

            /// \brief implementation of service
            IdentityUserProfileService* userProfileByPidPriv(const QString& accessToken, qint64 userPid);
            IdentityUserProfileService* userProfileByEAIDPriv(const QString& accessToken, const QString& eaid);
            IdentityUserProfileService* userProfileByEmailPriv(const QString& accessToken, const QString& email);

            IdentityUserProfileService* updateUserNamePriv(const QString& accessToken, const QString& pid, const QString& firstName, const QString& lastName);

            /// \brief User's token to utilize during the REST request
            QString mAccessToken;
            QMap<QString, QString> mTempStorage;

            restError mRestError;
            int mHttpResponseCode;
            QString mHttpErrorDescription;

        };
    }
}
#endif