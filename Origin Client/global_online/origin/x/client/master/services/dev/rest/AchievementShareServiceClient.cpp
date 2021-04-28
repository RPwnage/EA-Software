///////////////////////////////////////////////////////////////////////////////
// AchievementShareServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/rest/AchievementShareServiceClient.h"
#include "services/rest/SearchFriendServiceResponses.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {
        using Origin::Services::Session::SessionService;

        AchievementShareServiceClient::AchievementShareServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {

                QString strUrl = Origin::Services::readSetting(Services::SETTING_AchievementSharingURL);
                setBaseUrl(strUrl);
            }
        }

        namespace
        {
            QByteArray authorizationHeader(Session::SessionRef session)
            {
                QByteArray ah = "Bearer ";

                const QString accessToken(SessionService::accessToken(session));

                ah += accessToken.toLatin1();
                return ah;
            }

            QNetworkRequest request(Session::SessionRef session, const QUrl& serviceUrl)
            {
                QNetworkRequest req(serviceUrl);
                // Set header
                req.setRawHeader("Authorization", authorizationHeader(session));
                req.setRawHeader("X-Source","eadm-origin");
                return req;
            }

            QUrl url(const QUrl& myUrl,quint64 userId)
            {
                QString strUrl = QUrl::fromPercentEncoding(myUrl.toString().toLocal8Bit());
                strUrl = strUrl.arg(userId);
                QUrl serviceUrl(strUrl);
                return serviceUrl;
            }
        }

        AchievementShareServiceResponses* AchievementShareServiceClient::achievementsSharingPriv(Session::SessionRef session, quint64 userId)
        {
            ORIGIN_ASSERT(session);
            return new AchievementShareServiceResponses(getAuth(session, request(session, url(baseUrl(),userId))));
        }

        AchievementShareServiceResponses* AchievementShareServiceClient::setAchievementsSharingPriv(Session::SessionRef session, quint64 userId, QByteArray& data)
        {
            ORIGIN_ASSERT(session);
            ORIGIN_LOG_EVENT << "Payload to be set:\n" << data;
            if (data.contains("pidPrivacySettings"))
            {
                data.replace("{\n  \"pidPrivacySettings\" : ", "");
                data.replace("}\n}", "\n}");
                ORIGIN_LOG_EVENT << "Payload to be set CONTAINED pidPrivacySettings wrapper - REMOVED:\n" << data;
            }

            return new AchievementShareServiceResponses(putAuth(session, request(session, url(baseUrl(), userId)), data));
        }
    }
}

