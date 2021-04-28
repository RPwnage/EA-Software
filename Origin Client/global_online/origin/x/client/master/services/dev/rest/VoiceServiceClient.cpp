///////////////////////////////////////////////////////////////////////////////
// VoiceServiceClient.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/log/LogService.h"
#include "services/rest/VoiceServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace
{
    QNetworkRequest addHeaders(QNetworkRequest request, Origin::Services::Session::SessionRef session)
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("authToken", Origin::Services::Session::SessionService::accessToken(session).toUtf8());
        request.setRawHeader("Accept", "application/json");

        return request;
    }

    QNetworkRequest addUserLocationHeader(QNetworkRequest request, QString continent)
    {
        request.setRawHeader("X-Akamai-Edgescape", (QString("continent=") + continent).toUtf8());
        return request;
    }
}

namespace Origin
{
    namespace Services
    {
        VoiceServiceClient::VoiceServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString voiceURL = Origin::Services::readSetting(Origin::Services::SETTING_VoiceURL).toString();
                setBaseUrl(QUrl(voiceURL));
            }
        }

        ChannelExistsResponse* VoiceServiceClient::channelExistsPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId)
        {
            QUrl serviceUrl(urlForServicePath("sonar-master/operators/" + operatorId + "/channels/" + channelId + "/exists"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addHeaders(request, session);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService ChannelExists req: url=" << request.url().toString();

            return new ChannelExistsResponse(getAuth(session, request));
        }

        CreateChannelResponse* VoiceServiceClient::createChannelPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId, const QString& channelName)
        {
            QString serviceUrl = urlStringForServicePath("sonar-master/operators/" + operatorId + "/channels/" + channelId + "/name/" + QUrl::toPercentEncoding(channelName));
            QUrl servicePath(serviceUrl, QUrl::StrictMode);

            // Sonar Testing
            QString continent = Origin::Services::readSetting(Origin::Services::SETTING_SonarTestingUserLocation).toString();
            if (!continent.isEmpty())
            {
                // bypass Akamai
                QString host = servicePath.host();
                host.prepend("ak.");
                servicePath.setHost(host);
            }

            if (!servicePath.isValid())
            {
                ORIGIN_LOG_WARNING << "servicePath not valid: " << serviceUrl;
            }
            QUrlQuery serviceUrlQuery(servicePath);

            servicePath.setQuery(serviceUrlQuery);
            QNetworkRequest request(servicePath);

            QJsonObject data;
            data["creatorId"] = nucleusId(session);
            QJsonDocument postBodyDoc(data);

            request = addHeaders(request, session);

            // Sonar Testing
            if (!continent.isEmpty())
            {
                request = addUserLocationHeader(request, continent);
            }

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService CreateChannel req: url=" << request.url().toString() << "\n" << postBodyDoc.toJson();

            return new CreateChannelResponse(postAuth(session, request, postBodyDoc.toJson()));
        }

        AddChannelUserResponse* VoiceServiceClient::addUserToChannelPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId)
        {
            QUrl serviceUrl(urlForServicePath("sonar-master/operators/" + operatorId + "/channels/" + channelId + "/users"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            QJsonObject data;
            data["userId"] = nucleusId(session);
            QJsonDocument postBodyDoc(data);

            request = addHeaders(request, session);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService AddUserToChannel req: url=" << request.url().toString() << "\n" << postBodyDoc.toJson();

            return new AddChannelUserResponse(postAuth(session, request, postBodyDoc.toJson()));
        }

        GetChannelTokenResponse* VoiceServiceClient::getChannelTokenPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId)
        {
            QUrl serviceUrl(urlForServicePath("sonar-master/operators/" + operatorId + "/channels/" + channelId + "/users/" + nucleusId(session) + "/token"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addHeaders(request, session);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService GetChannelToken req: url=" << request.url().toString();

            return new GetChannelTokenResponse(getAuth(session, request));
        }
    }
}
