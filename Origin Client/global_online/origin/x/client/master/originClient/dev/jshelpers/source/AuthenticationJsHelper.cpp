/////////////////////////////////////////////////////////////////////////////
// AuthenticationJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "AuthenticationJsHelper.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/settings/SettingsManager.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/config/OriginConfigService.h"

#include "TelemetryAPIDLL.h"

#include <QString>
#include <QUrl>

namespace Origin
{
    namespace Client
    {
        AuthenticationJsHelper::AuthenticationJsHelper(QObject *parent) :
            QObject(parent)
            , mIsFallback(false)
            , mOfflineBannerType (QString())
            , mOfflineBannerText (QString())
            , mFirstName(false)
            , mLastName(false)
            , mAvatarWindow(false)
            , mAvatarGallery(false)
            , mAvatarUpload(false)
            , mShowAgeUpFlow(true)
            , mSupportErrorDescription(true)
        {
            initFromConfigService();
        }

        /// \brief Initializes some member variables from the dynamic configuration.
        void AuthenticationJsHelper::initFromConfigService()
        {
            // If this assert fires, it means that OriginConfigService hasn't been created, 
            // which probably isn't intended!
            Services::OriginConfigService* configService = Services::OriginConfigService::instance();
            assert(configService);

            if (configService)
            {
                mSupportErrorDescription = configService->miscConfig().supportErrorDescription;
            }
        }

        void AuthenticationJsHelper::onLoginSuccess(const QString& successUrl)
        {
            ORIGIN_LOG_EVENT << "Successfully logged in: ONLINE";
            // successUrl arrives with a fragment containg the code and id_token but QUrl
            // won't parse a fragment, so turn it into a query.
            QString temp(successUrl);
            
            QUrl url(temp.replace('#', '?'));
            QUrlQuery urlQuery(url.query());

            QString authCode = urlQuery.queryItemValue("code");
            QString idToken = urlQuery.queryItemValue("id_token");

            emit onlineLoginComplete(authCode, idToken);
        }

        void AuthenticationJsHelper::onOfflineLoginSuccess(const QString& cids)
        {
            QStringList cidList = cids.split(",", QString::SkipEmptyParts);
            QString primaryCid;
            QString backupCid;

            if (!cidList.isEmpty())
            {
                primaryCid = cidList.back();
                if (cidList.size() > 1)
                {
                    backupCid = cidList.front();
                }
            }

            emit offlineLoginComplete(primaryCid, backupCid);
        }

        bool AuthenticationJsHelper::online()
        {
            bool online = Services::Network::GlobalConnectionStateMonitor::instance()->isOnline();
            return online;
        }

        bool AuthenticationJsHelper::isFallback()
        {
            return mIsFallback;
        }

        void AuthenticationJsHelper::setFallback(bool isFallback)
        {
            mIsFallback = isFallback;
        }

        void AuthenticationJsHelper::onChangePasswordUrl(const QString& url)
        {
            emit(changePasswordUrl(url));
        }

        QString AuthenticationJsHelper::currentEnvironment()
        {
            QString environmentName = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
            environmentName = environmentName.toLower();

            return environmentName;
        }

        void AuthenticationJsHelper::onAuthenticationError(const QString& error, const QString& cids)
        {
            emit(authenticationError(error));

            if (!cids.isEmpty())
            {
                onOfflineLoginSuccess(cids);
            }
        }

        /// \brief Controls whether an additional error string is provided when an login error occurs.
        ///
        /// \return True if additional error info should be provided from the login page when reporting
        /// an error, false if only the error code itself should be provided.
        ///
        /// \sa AuthenticationJsHelper::onAuthenticationError
        bool AuthenticationJsHelper::supportErrorDescription()
        {
            return mSupportErrorDescription;
        }

        /// \brief set whether we support additional description of errors
        /// \sa AuthenticationJsHelper::mSupportErrorDescription, 
        /// AuthenticationJsHelper::supportErrorDescription
        void AuthenticationJsHelper::setSupportErrorDescription(const bool supportDescription)
        {
            mSupportErrorDescription = supportDescription;
        }

        void AuthenticationJsHelper::onAgeUpBegin(const QString& step, const QString& executeJSstmt)
        {
            ORIGIN_LOG_ACTION << "Age Up Begin: " << executeJSstmt;
            emit (ageUpBegin(executeJSstmt));
            //should add telemetry call here
        }
        void AuthenticationJsHelper::onAgeUpCancel(const QString& step)
        {
            ORIGIN_LOG_ACTION << "Age Up Cancel: " << step;
            emit (ageUpCancel());
            //should add telemetry call here
        }

        void AuthenticationJsHelper::onAgeUpEnd(const QString& step)
        {
            ORIGIN_LOG_ACTION << "Age Up End: " << step;
            emit (ageUpComplete());
            //should add telemetry call here
        }

        void AuthenticationJsHelper::onNuxBegin(const QString& step)
        {
            ORIGIN_LOG_ACTION << "Nux Begin Step: " << step;
            emit (nuxBegin(step));
            GetTelemetryInterface()->Metric_NUX_START(step.toUtf8().data(), Origin::Services::readSetting(Origin::Services::SETTING_RUNONSYSTEMSTART));
        }
        void AuthenticationJsHelper::onNuxCancel(const QString& step)
        {
            const QString finalStep = "2";
            ORIGIN_LOG_ACTION << "Nux Cancel Step: " << step;
            emit (nuxCancel(step));
            GetTelemetryInterface()->Metric_NUX_CANCEL(step.toUtf8().data());
            if(step == finalStep)
                GetTelemetryInterface()->Metric_NUX_PROFILE();
        }
        void AuthenticationJsHelper::onNuxEnd(const QString& step)
        {
            const QString finalStep = "2";
            ORIGIN_LOG_ACTION << "Nux End Step: " << step;
            emit (nuxComplete(step));
            GetTelemetryInterface()->Metric_NUX_END(step.toUtf8().data());
            if(step == finalStep)
                GetTelemetryInterface()->Metric_NUX_PROFILE(mFirstName,mLastName,mAvatarWindow,mAvatarGallery,mAvatarUpload);
        }
        void AuthenticationJsHelper::onSendTelemetry(const QString& type)
        {
            TelemetryValues telm = static_cast<TelemetryValues>(type.toInt());
            switch(telm)
            {
                case PRIVACY_SETTINGS_START:
                    ORIGIN_LOG_ACTION << "Privacy Settings Page: Started";
                    GetTelemetryInterface()->Metric_PRIVACY_START();
                    break;
                case PRIVACY_SETTINGS_CANCEL:
                    ORIGIN_LOG_ACTION << "Privacy Settings Page: Canceled";
                    GetTelemetryInterface()->Metric_PRIVACY_CANCEL();
                    break;
                case PRIVACY_SETTINGS_SUCCESS:
                    ORIGIN_LOG_ACTION << "Privacy Settings Page: Continue";
                    GetTelemetryInterface()->Metric_PRIVACY_END();
                    break;
                case EMAIL_DUPLICATE:
                    ORIGIN_LOG_ACTION << "NUX: Entered Email Duplicate";
                    GetTelemetryInterface()->Metric_NUX_EMAILDUPLICATE();
                    break;
                case CAPTCHA_START:
                    ORIGIN_LOG_ACTION << "Captcha: Started";
                    GetTelemetryInterface()->Metric_CAPTCHA_START();
                    break;
                case CAPTCHA_FAIL:
                    ORIGIN_LOG_ACTION << "Captcha: Failed";
                    GetTelemetryInterface()->Metric_CAPTCHA_FAIL();
                    break;
                case FIRST_NAME:
                    ORIGIN_LOG_ACTION << "NUX: Entered First Name";
                    mFirstName = true;
                    break;
                case LAST_NAME:
                    ORIGIN_LOG_ACTION << "NUX: Entered Last Name";
                    mLastName = true;
                    break;
                case AVATAR_WINDOW_START:
                    ORIGIN_LOG_ACTION << "NUX: Entered Avatar Window";
                    mAvatarWindow = true;
                    break;
                case AVATAR_GALLERY:
                    ORIGIN_LOG_ACTION << "NUX: Selected Avatar from Gallery";
                    mAvatarGallery = true;
                    // User can only have one "type" of avatar. Grab the most recent event and set the other to false.
                    mAvatarUpload = false;
                    break;
                case AVATAR_UPLOAD:
                    ORIGIN_LOG_ACTION << "NUX: Uploaded Avatar";
                    mAvatarUpload = true;
                    // User can only have one "type" of avatar. Grab the most recent event and set the other to false.
                    mAvatarGallery = false;
                    break;
                default:
                    GetTelemetryInterface()->Metric_LOGIN_UNKNOWN(type.toLatin1());
                    ORIGIN_LOG_ACTION << "Undefined TelemetryValues type: " << type;
                    break;
            }
        }
        void AuthenticationJsHelper::closeWindow()
        {
            emit close();
        }

        QString AuthenticationJsHelper::offlineBannerType()
        {
            return mOfflineBannerType;
        }

        QString AuthenticationJsHelper::offlineBannerText()
        {
            return mOfflineBannerText;
        }

        void AuthenticationJsHelper::setOfflineBannerInfo (const QString& bannerType, const QString& bannerText)
        {
            mOfflineBannerType = bannerType;
            mOfflineBannerText = bannerText;
        }

        bool AuthenticationJsHelper::showAgeUpFlow ()
        {
            return mShowAgeUpFlow;
        }

        void AuthenticationJsHelper::setShowAgeUpFlow (bool show)
        {
            mShowAgeUpFlow = show;
        }
    }
}
