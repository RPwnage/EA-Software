///////////////////////////////////////////////////////////////////////////////
// AchievementServiceClient.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>
#include <QLocale>

#include "services/achievements/AchievementServiceClient.h"
#include "services/achievements/AchievementServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"

void AddHMACHeaders(QNetworkRequest &serverRequest, QByteArray code, QByteArray payload);

namespace Origin
{
    namespace Services
    {
        namespace Achievements
        {
            AchievementServiceClient::AchievementServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
                : OriginAuthServiceClient(nam)
            {
                if (!baseUrl.isEmpty())
                    setBaseUrl(baseUrl);
                else
                {
                    QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_AchievementServiceURL); 
                    setBaseUrl(QUrl(strUrl));
                }
            }


            // references: https://developer.origin.com/documentation/display/social/TDD+-+Achievements

            AchievementSetQueryResponse * AchievementServiceClient::achievementsPriv(const QString &personaId, const QString &achievementSet, const QString &language, bool bMetadata)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                QString myUrl("personas/%1/%2/all");

                myUrl = myUrl.arg(personaId).arg(achievementSet);

                QUrl serviceUrl(urlForServicePath(myUrl));
                QUrlQuery serviceUrlQuery(serviceUrl);

                if(!language.isEmpty())
                {
                    serviceUrlQuery.addQueryItem("lang", language);
                }
                if(bMetadata)
                {
                    serviceUrlQuery.addQueryItem("metadata", "true");
                }
                serviceUrlQuery.addQueryItem("fullset", "true");

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("X-Api-Version", "2");
                request.setRawHeader("X-Application-Key", "Origin");
                request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());


                return new AchievementSetQueryResponse(personaId, achievementSet, getAuth(session, request));
            }	

            AchievementSetsQueryResponse * AchievementServiceClient::achievementSetsPriv(const QString &personaId, const QString &language, bool bMetadata)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                QString myUrl("personas/%1/all");

                myUrl = myUrl.arg(personaId);

                QUrl serviceUrl(urlForServicePath(myUrl));
                QUrlQuery serviceUrlQuery(serviceUrl);
                
                if(!language.isEmpty())
                {
                    serviceUrlQuery.addQueryItem("lang", language);
                }
                if(bMetadata)
                {
                    serviceUrlQuery.addQueryItem("metadata", "true");
                }

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("X-Api-Version", "2");
                request.setRawHeader("X-Application-Key", "Origin");
                request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());

                return new AchievementSetsQueryResponse(personaId, getAuth(session, request));
            }

			AchievementSetReleaseInfoResponse * AchievementServiceClient::achievementSetReleaseInfoPriv(const QString &language)
			{
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

				QString myUrl("products/released");
								
				QUrl serviceUrl(urlForServicePath(myUrl));
				QUrlQuery serviceUrlQuery(serviceUrl);
				
				if(!language.isEmpty())
				{
					serviceUrlQuery.addQueryItem("lang", language);
				}
				
				serviceUrl.setQuery(serviceUrlQuery);
				QNetworkRequest request(serviceUrl);

				request.setRawHeader("X-Api-Version", "2");
				request.setRawHeader("X-Application-Key", "Origin");
				request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());

				return new AchievementSetReleaseInfoResponse(getAuth(session, request));
			}

            AchievementProgressQueryResponse * AchievementServiceClient::pointsPriv(const QString &personaId)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                QString myUrl("personas/%1/progression");

                myUrl = myUrl.arg(personaId);

                QUrl serviceUrl(urlForServicePath(myUrl));

                QNetworkRequest request(serviceUrl);

                request.setRawHeader("X-Api-Version", "2");
                request.setRawHeader("X-Application-Key", "Origin");
                request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());

                return new AchievementProgressQueryResponse(getAuth(session, request));
            }	

            AchievementGrantResponse *AchievementServiceClient::grantPriv(const QString &personaId, const QString &appId, const QString &achievementSetId, const QString &achievementId, const QString &achievementCode, int progress)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                QString url = "/personas/" + personaId + "/" + achievementSetId + "/" + achievementId + "/progress";

                QUrl serviceUrl(urlForServicePath(url));
                QUrlQuery serviceUrlQuery(serviceUrl);
                serviceUrlQuery.addQueryItem("lang", QLocale().name());

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());
                request.setRawHeader("X-Source", "Origin");
                if(!appId.isEmpty())
                {
                    request.setRawHeader("X-Api-Version", "2");
                    request.setRawHeader("X-Application-Key", appId.toLatin1());
                }
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                QString points = "{ \"points\":" + QString::number(progress) + " }";

                AddHMACHeaders(request, achievementCode.toLatin1(), points.toLatin1());

                return new AchievementGrantResponse(postAuth(session, request, points.toLatin1()));
            }

            PostEventResponse * AchievementServiceClient::postEventPriv(const QString &personaId, const QString &appId, const QString &achievementSetId, const QString &events)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                QString url = "/events/persona/" + personaId + "/" + achievementSetId;

                QNetworkRequest request(urlForServicePath(url));

                request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());
                request.setRawHeader("X-Source", "Origin");
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                return new PostEventResponse(postAuth(session, request, events.toUtf8()));
            }
        }
    }
}
