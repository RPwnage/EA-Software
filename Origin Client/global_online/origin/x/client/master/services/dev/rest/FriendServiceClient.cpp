///////////////////////////////////////////////////////////////////////////////
// FriendServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/rest/FriendServiceClient.h"
#include "services/rest/OriginServiceResponse.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        FriendServiceClient::FriendServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_FriendsURL);
                setBaseUrl(QUrl(strUrl + "/friends"));
            }
        }

        OriginServiceResponse* FriendServiceClient::inviteFriendPriv(Session::SessionRef session, quint64 friendId, const QString &comment, const QString &emailTemplate, const QString &source)
        {
            QUrl serviceUrl(urlForServicePath("inviteFriend"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("friendId", QString::number(friendId));
            serviceUrl.setQuery(serviceUrlQuery);

            return inviteFriendFromUrlPriv(session, serviceUrl, comment, emailTemplate, source);
        }

        OriginServiceResponse* FriendServiceClient::inviteFriendByEmailPriv(Session::SessionRef session, const QString &email, const QString &comment, const QString &emailTemplate, const QString &source)
        {
            QUrl serviceUrl(urlForServicePath("inviteFriendByEmail"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("email", email);
            serviceUrl.setQuery(serviceUrlQuery);

            return inviteFriendFromUrl(session, serviceUrl, comment, emailTemplate, source);
        }

        OriginAuthServiceResponse* FriendServiceClient::inviteFriendFromUrlPriv(Session::SessionRef session, QUrl url, const QString &comment, const QString &emailTemplate, const QString &source)
        {
            QUrl serviceUrl(url);
            QUrlQuery serviceUrlQuery(serviceUrl);
            
            if (!comment.isNull())
            {
                serviceUrlQuery.addQueryItem("comment", comment.toUtf8());
            }

            if (!emailTemplate.isNull())
            {
                serviceUrlQuery.addQueryItem("emailtemplate", emailTemplate.toUtf8());
            }

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest req(serviceUrl);

            if (!source.isNull())
            {
                req.setRawHeader("Source", source.toUtf8());
            }
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new OriginAuthServiceResponse(getAuth(session,  req));
        }

        UserActionResponse *FriendServiceClient::confirmInvitationPriv(Session::SessionRef session, quint64 friendId, const QString &emailTemplate)
        {
            QUrl serviceUrl(urlForServicePath("confirmInvitation"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("friendId", QString::number(friendId));

            if (!emailTemplate.isNull())
            {
                serviceUrlQuery.addQueryItem("emailtemplate", emailTemplate);
            }
            serviceUrl.setQuery(serviceUrlQuery);

            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new UserActionResponse(friendId, getAuth(session,  req) );
        }

        UserActionResponse *FriendServiceClient::rejectInvitationPriv(Session::SessionRef session, quint64 friendId, const QString &emailTemplate)
        {
            QUrl serviceUrl(urlForServicePath("rejectInvitation"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("friendId", QString::number(friendId));

            if (!emailTemplate.isNull())
            {
                serviceUrlQuery.addQueryItem("emailtemplate", emailTemplate);
            }
            serviceUrl.setQuery(serviceUrlQuery);

            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new UserActionResponse(friendId, getAuth(session,  req) );
        }

        RetrieveInvitationsResponse *FriendServiceClient::retrieveInvitationsPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("retrieveInvitation"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new RetrieveInvitationsResponse(getAuth(session,  req) );
        }

        PendingFriendsResponse *FriendServiceClient::retrievePendingFriendsPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("user/" + escapedNucleusId(session) + "/pendingFriends"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new PendingFriendsResponse(getAuth(session,  req) );
        }

        UserActionResponse *FriendServiceClient::deleteFriendPriv(Session::SessionRef session, quint64 friendId)
        {
            QUrl serviceUrl(urlForServicePath("deleteFriend"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("friendId", QString::number(friendId));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new UserActionResponse(friendId, getAuth(session,  req) );
        }

        UserActionResponse *FriendServiceClient::blockUserPriv(Session::SessionRef session, quint64 userId)
        {
            QUrl serviceUrl(urlForServicePath("blockUser"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("friendId", QString::number(userId));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new UserActionResponse(userId, getAuth(session,  req) );
        }

        UserActionResponse *FriendServiceClient::unblockUserPriv(Session::SessionRef session, quint64 userId)
        {
            QUrl serviceUrl(urlForServicePath("unblockUser"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrlQuery.addQueryItem("friendId", QString::number(userId));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new UserActionResponse(userId, getAuth(session,  req) );
        }

        BlockedUsersResponse *FriendServiceClient::blockedUsersPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("getBlockUserList"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("nucleusId", nucleusId(session));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new BlockedUsersResponse(getAuth(session,  req));
        }
    }
}

