///////////////////////////////////////////////////////////////////////////////
// PrivacyServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/PrivacyServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        PrivacyServiceClient::PrivacyServiceClient(const QUrl &baseUrl,NetworkAccessManager *nam) 
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_FriendsURL); 
                setBaseUrl(QUrl(strUrl + "/friends/user"));
            }
        }

        PrivacyFriendsVisibilityResponse* PrivacyServiceClient::friendsProfileVisibilityPriv(Session::SessionRef session, const QList<quint64>& friendsIds )
        {
            QString localFriendsIds;
            foreach (quint64 friendId, friendsIds)
            {
                localFriendsIds += QString::number(friendId) + ";";
            }
            localFriendsIds.resize(localFriendsIds.size()-1); // eliminate last ';'
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session) + "/visibility/friends/" + localFriendsIds));
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyFriendsVisibilityResponse(getAuth(session, req));
        }

        PrivacyFriendVisibilityResponse * PrivacyServiceClient::isFriendProfileVisibilityPriv(Session::SessionRef session, quint64 friendId)
        {
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session) + "/visibility/friend/" + QString::number(friendId)));
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyFriendVisibilityResponse(getAuth(session, req) );
        }

        PrivacyFriendVisibilityResponse * PrivacyServiceClient::friendRichPresencePrivacyPriv(Session::SessionRef session, quint64 friendId)
        {
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session) + "/richPresenceVisibility/friend/" + QString::number(friendId)));
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyFriendVisibilityResponse(getAuth(session, req) );
        }

        PrivacyVisibilityResponse * PrivacyServiceClient::richPresencePrivacyPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session)+ "/richpresenceprivacy"));
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyVisibilityResponse(getAuth(session, req) );
        }

        PrivacyVisibilityResponse *PrivacyServiceClient::profilePrivacyPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session) + "/visibility"));
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyVisibilityResponse(getAuth(session, req) );
        }

        PrivacyVisibilityResponse *PrivacyServiceClient::updateProfilePrivacyPriv(Session::SessionRef session, visibility MyVisibility)
        {
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session) + "/visibility"));

            QNetworkRequest req(serviceUrl);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyVisibilityResponse(postAuth(session, req, visibilitySetting(MyVisibility)));
        }

        PrivacyVisibilityResponse *PrivacyServiceClient::updateRichPresencePriv(Session::SessionRef session, visibility MyVisibility)
        {
            QUrl serviceUrl(urlForServicePath(escapedNucleusId(session) + "/richpresenceprivacy"));
            QNetworkRequest req(serviceUrl);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PrivacyVisibilityResponse(postAuth(session, req, visibilitySetting(MyVisibility)));
        }

        // HELPERS
        const QByteArray PrivacyServiceClient::visibilitySetting( visibility MyVisibility )
        {
            return Visibility().name(MyVisibility).toUtf8().constData();
        }
    }
}


