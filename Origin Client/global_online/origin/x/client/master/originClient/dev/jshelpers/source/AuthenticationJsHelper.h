/////////////////////////////////////////////////////////////////////////////
// AuthenticationJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef AUTHENTICATIONJSHELPER_H
#define AUTHENTICATIONJSHELPER_H

#include "services/rest/OriginServiceValues.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QString>

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API AuthenticationJsHelper : public QObject
        {
            Q_OBJECT

        public:

            enum TelemetryValues
            {
                PRIVACY_SETTINGS_START,
                PRIVACY_SETTINGS_CANCEL,
                PRIVACY_SETTINGS_SUCCESS,
                EMAIL_DUPLICATE,
                CAPTCHA_START,
                CAPTCHA_FAIL,
                FIRST_NAME,
                LAST_NAME,
                AVATAR_WINDOW_START,
                AVATAR_GALLERY,
                AVATAR_UPLOAD
            };

            /// \brief CTOR
            AuthenticationJsHelper(QObject *parent = NULL);

            /// \brief set the bannerType and bannerText
            void setOfflineBannerInfo (const QString& bannerType, const QString& bannerText);

            /// \brief set the var to enable/disable going thru Age Up Flow during login
            void setShowAgeUpFlow (bool show); 

            void setSupportErrorDescription(const bool supportDescription);

            Q_INVOKABLE void onLoginSuccess(const QString& successUrl);
            Q_INVOKABLE void onOfflineLoginSuccess(const QString& cids);
            Q_INVOKABLE bool online();
            Q_INVOKABLE void setFallback(bool isFallback);
            Q_INVOKABLE bool isFallback();
            Q_INVOKABLE QString currentEnvironment();
            Q_INVOKABLE void onAuthenticationError(const QString& error, const QString& cids);
            Q_INVOKABLE bool supportErrorDescription();
            Q_INVOKABLE void onAgeUpBegin(const QString& step, const QString& executeJSstmt);
            Q_INVOKABLE void onAgeUpCancel(const QString& step);
            Q_INVOKABLE void onAgeUpEnd(const QString& step);
            Q_INVOKABLE void onNuxBegin(const QString& step);
            Q_INVOKABLE void onNuxCancel(const QString& step);
            Q_INVOKABLE void onNuxEnd(const QString& step);
            Q_INVOKABLE void onSendTelemetry(const QString& type);
            Q_INVOKABLE void closeWindow();
            Q_INVOKABLE bool showAgeUpFlow();

            Q_INVOKABLE QString offlineBannerType();
            Q_INVOKABLE QString offlineBannerText();

            /// \fn onChangePasswordUrl
            /// \brief called from the JS in the forgot_password.html page
            Q_INVOKABLE void onChangePasswordUrl(const QString& url);

        signals:

            void onlineLoginComplete(const QString& authorizationCode, const QString& idToken);
            void offlineLoginComplete(const QString& primaryCid, const QString& backupCid);

            /// \brief emitted when change password url is received.
            void changePasswordUrl (const QString& url);

            /// \brief emitted when there was an authentication error
            void authenticationError(QString error);

            /// \brief emitted when age up flow is shown during login process
            void ageUpBegin (const QString& skipFn);
            void ageUpCancel ();
            void ageUpComplete ();

            /// \brief emitted when account registration page is about to be loaded
            void nuxBegin(const QString& step);
            void nuxCancel(const QString& step);
            void nuxComplete(const QString& step);

            /// \brief Emitted when closeWindow is invoked.
            void close();

        private:

            void initFromConfigService();

            bool mIsFallback;
            QString mOfflineBannerType;
            QString mOfflineBannerText;
            bool mFirstName;
            bool mLastName;
            bool mAvatarWindow;
            bool mAvatarGallery;
            bool mAvatarUpload;
            bool mShowAgeUpFlow;
            bool mSupportErrorDescription;
        };
    }
}

#endif
