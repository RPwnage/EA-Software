 ///////////////////////////////////////////////////////////////////////////////
// AtomServiceClient.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/AtomServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "OriginServiceResponse.h"
#include "AtomServiceResponses.h"
#include "services/common/XmlUtil.h"
#include <QDomDocument>

namespace 
{
    using namespace Origin::Services;

    QString reportReasonToString(AtomServiceClient::ReportReason reason)
    {
        switch(reason)
        {
        case AtomServiceClient::ChildSolicitationReason:
            return "Child Solicitation";
        case AtomServiceClient::TerroristThreatReason:
            return "Terrorist Threat";
        case AtomServiceClient::ClientHackReason:
            return "Client Hack";
        case AtomServiceClient::HarassmentReason:
            return "Harassment";
        case AtomServiceClient::SpamReason:
            return "SPAM";
        case AtomServiceClient::CheatingReason:
            return "Cheating";
        case AtomServiceClient::OffensiveContentReason:
            return "Offensive Content";
        case AtomServiceClient::SuicideThreatReason:
            return "Suicide Threat";
        }

        ORIGIN_ASSERT(0);
        return QString();
    }
    
    QString reportContentTypeToString(AtomServiceClient::ReportContentType contentType)
    {
        switch(contentType)
        {
        case AtomServiceClient::NameContent:
            return "NAME";
        case AtomServiceClient::AvatarContent:
            return "Avatar";
        case AtomServiceClient::CustomGameNameContent:
            return "Custom Game Name";
        case AtomServiceClient::RoomNameContent:
            return "Room Name";
        case AtomServiceClient::InGame:
            return "In Game";
        }

        ORIGIN_ASSERT(0);
        return QString();
    }
}

namespace Origin
{
    namespace Services
    {
        AtomServiceClient::AtomServiceClient(const QUrl &baseUrl,NetworkAccessManager *nam /*= NULL*/ )
            : RequestClientBase(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                 QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_AtomURL); 
                 if (!strUrl.endsWith("/atom"))
                 {
                     strUrl += QString("/atom");
                 } 
                 setBaseUrl(QUrl(strUrl));
            }
        }

        AppSettingsResponse* AtomServiceClient::appSettings(Session::SessionRef session)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->appSettingsPriv(session);
        }

        UserGamesResponse* AtomServiceClient::userGames(Session::SessionRef session, quint64 nucleusId)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->userRequest<UserGamesResponse>(session, nucleusId, "games");
        }

        UserFriendsResponse* AtomServiceClient::userFriends(Session::SessionRef session, quint64 nucleusId)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->userRequest<UserFriendsResponse>(session, nucleusId, "friends");
        }

        ShareAchievementsResponse* AtomServiceClient::shareAchievements(Session::SessionRef session, quint64 nucleusId)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->userRequest<ShareAchievementsResponse>(session, nucleusId, "shareAchievements");
        }

        UserResponse* AtomServiceClient::user(Session::SessionRef session)
        {
            ORIGIN_ASSERT(session);
            QString servicePath = QString("/users/%1").arg(Session::SessionService::nucleusUser(session));
            return OriginClientFactory<AtomServiceClient>::instance()->getRequest<UserResponse>(session, servicePath);
        }

        UserResponse* AtomServiceClient::user(Session::SessionRef session, quint64 nucleusId )
        {
            QString servicePath = QString("/users/%1").arg(nucleusId);
            return OriginClientFactory<AtomServiceClient>::instance()->getRequest<UserResponse>(session, servicePath);
        }

        QList<UserResponse*> AtomServiceClient::user(Session::SessionRef session, const QList<quint64> &nucleusIdList)
        {
            QList<UserResponse*> resps;
            QString nucleusIdsString;

            for (int index = 0; index < nucleusIdList.size(); ++index)
            {
                nucleusIdsString += QString::number(nucleusIdList.at(index)) + ",";

                // Atom can handle a comma delimited list of up to 5 nucleus Id's.
                // If we have added 5 Id's to our nucleusIdsString, or if we are at the end of our nucleusIdList,
                // then send the request.
                if ((index + 1) % 5 == 0 || index == nucleusIdList.size() - 1)
                {
                    // chop off the trailing comma
                    nucleusIdsString.chop(1);

                    QMap<QString, QString> query;
                    query.insert("userIds", nucleusIdsString);
                    nucleusIdsString = "";

                    resps << OriginClientFactory<AtomServiceClient>::instance()->getRequest<UserResponse>(session, "/users", query);
                }
            }
            return resps;
        }

        SearchOptionsResponse* AtomServiceClient::searchOptions(Session::SessionRef session)
        {
            ORIGIN_ASSERT(session);
            QString servicePath = QString("/users/%1/searchOptions").arg(Session::SessionService::nucleusUser(session));
            return OriginClientFactory<AtomServiceClient>::instance()->getRequest<SearchOptionsResponse>(session, servicePath);
        }

        SetSearchOptionsResponse* AtomServiceClient::setSearchOptions(Session::SessionRef session, QVector<QString>& mySearchOptions )
        {
            ORIGIN_ASSERT(session);
            QString servicePath = QString("/users/%1/searchOptions").arg(Session::SessionService::nucleusUser(session));
            return OriginClientFactory<AtomServiceClient>::instance()->putRequest<SetSearchOptionsResponse>(session, servicePath, mySearchOptions);
        }

        OriginAuthServiceResponse* AtomServiceClient::reportUser(Session::SessionRef session, quint64 nucleusId, ReportContentType contentType, ReportReason reason, QString masterTitle, QString comments)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->reportUserPriv(session, nucleusId, contentType, reason, masterTitle, comments);
        }

        //////////////////////////////////////////////////////////////////////////
        // app settings

        AppSettingsResponse* AtomServiceClient::appSettingsPriv(Session::SessionRef session)
        {
            ORIGIN_ASSERT(session);
            QString servicePath = QString("/users/%1/appSettings").arg(Session::SessionService::nucleusUser(session));
            QNetworkRequest request = buildRequest(session, servicePath);
            
            return new AppSettingsResponse(getNonAuth(request));
        }

        //////////////////////////////////////////////////////////////////////////
        // Privacy

        PrivacyGetSettingResponse* AtomServiceClient::privacySetting(Session::SessionRef session, privacySettingCategory privacysetting)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->privacySettingPriv(session, privacysetting);
        }

        OriginServiceResponse* AtomServiceClient::setPrivacySetting(Session::SessionRef session, privacySettingCategory myPrivacySetting, const QString& payload)
        {
            return OriginClientFactory<AtomServiceClient>::instance()->setPrivacySettingPriv(session, myPrivacySetting, payload);
        }

        PrivacyGetSettingResponse* AtomServiceClient::privacySettingPriv(Session::SessionRef session, privacySettingCategory myPrivacySetting)
        {
            return new PrivacyGetSettingResponse(getAuth(session,privacyRequest(session, myPrivacySetting)));
        }

        OriginServiceResponse* AtomServiceClient::setPrivacySettingPriv(Session::SessionRef session, privacySettingCategory myPrivacySetting, const QString& payload)
        {
            QNetworkRequest req = privacyRequest(session, myPrivacySetting);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain"); 
            return new OriginAuthServiceResponse(postAuth(session,req, payload.toUtf8()));
        }

        OriginAuthServiceResponse* AtomServiceClient::reportUserPriv(Session::SessionRef session, quint64 nucleusId, ReportContentType contentType, ReportReason reason, QString masterTitle, QString comments)
        {
            ORIGIN_ASSERT(session);

            QString servicePath = QString("/users/%1/reportUser/%2").arg(Session::SessionService::nucleusUser(session)).arg(nucleusId);

            QNetworkRequest req = buildRequest(session, servicePath);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");

            QDomDocument postBodyDoc;
            QDomElement reportUserEl = postBodyDoc.createElement("reportUser");
            postBodyDoc.appendChild(reportUserEl);

            QDomElement contentTypeEl = postBodyDoc.createElement("contentType");
            contentTypeEl.appendChild(postBodyDoc.createTextNode(reportContentTypeToString(contentType)));
            reportUserEl.appendChild(contentTypeEl);
                
            QDomElement reportReasonEl = postBodyDoc.createElement("reportReason");
            reportReasonEl.appendChild(postBodyDoc.createTextNode(reportReasonToString(reason)));
            reportUserEl.appendChild(reportReasonEl);

            if (!masterTitle.isEmpty())
            {
                QDomElement reportMasterTitleEl = postBodyDoc.createElement("location");
                reportMasterTitleEl.appendChild(postBodyDoc.createTextNode(masterTitle));
                reportUserEl.appendChild(reportMasterTitleEl);
            }

            if (!comments.isEmpty())
            {
                QDomElement reportCommentsEl = postBodyDoc.createElement("comments");
                reportCommentsEl.appendChild(postBodyDoc.createTextNode(comments));
                reportUserEl.appendChild(reportCommentsEl);
            }

            return new OriginAuthServiceResponse(postAuth(session,req, postBodyDoc.toByteArray()));
        }

        QNetworkRequest AtomServiceClient::privacyRequest(Session::SessionRef session, privacySettingCategory myPrivacySetting)
        {
            QString path = urlForServicePath("users/" + Session::SessionService::nucleusUser(session) + "/privacySettings/" + privSettingsMap().name(myPrivacySetting)).toString();
            if (path.endsWith('/'))
            {
                path.chop(1);
            }
            QUrl serviceUrl(path);
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", Session::SessionService::accessToken(session).toUtf8());
            return req;
        }

    }
}
