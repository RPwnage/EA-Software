///////////////////////////////////////////////////////////////////////////////
// GroupServiceClient.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>
#include <QJsonDocument>
#include "services/log/LogService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace
{
    QNetworkRequest addGOSHeaders(QNetworkRequest request, Origin::Services::Session::SessionRef session)
    {
        request.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(session).toLatin1());
        request.setRawHeader("X-Api-Version", "2");
        request.setRawHeader("X-Application-Key", "Origin");

        return request;
    }

    const int GROUP_MEMBER_LIST_RESPONSE_PAGESIZE = 100;
}

namespace Origin
{
    namespace Services
    {
        GroupServiceClient::GroupServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString groupsURL = Origin::Services::readSetting(Origin::Services::SETTING_GroupsURL).toString();
                setBaseUrl(QUrl(groupsURL));
            }
        }

        GroupCreateResponse* GroupServiceClient::createGroupPriv(Session::SessionRef session, const QString& groupName, const QString& shortName)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            request = addGOSHeaders(request, session);

            QJsonObject data;
            data["name"] = groupName;
            data["shortName"] = shortName;
            data["groupTypeId"] = QString("ORIGIN");
            data["creator"] = nucleusId(session);

            QJsonObject attributes;
            attributes["theme"] = QString("1");
            data["attributes"] = attributes;

            QJsonObject instance;
            instance["inviteURLKey"] = QString("");
            instance["pwd"] = QString("password");
            data["instanceJoinConfig"] = instance;

            QJsonDocument postBodyDoc(data);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService createGroup req: url=" << request.url().toString() << "\n" << postBodyDoc.toJson();

            return new GroupCreateResponse(groupName,postAuth(session, request, postBodyDoc.toJson()));
        }

        GroupChangeNameResponse* GroupServiceClient::changeGroupNamePriv(Session::SessionRef session, const QString& groupGuid, const QString& newGroupName)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/name"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("newName", QUrl::toPercentEncoding(newGroupName));

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService changeGroupName req: url=" << request.url().toString();

            return new GroupChangeNameResponse(groupGuid, newGroupName, putAuth(session, request, QByteArray()));
        }

        GroupDeleteResponse* GroupServiceClient::deleteGroupPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService deleteGroup req: url=" << request.url().toString();

            return new GroupDeleteResponse(groupGuid, deleteResourceAuth(session, request));
        }

        GroupJoinResponse* GroupServiceClient::joinGroupPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/join/" + nucleusId(session)));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService joinGroup req: url=" << request.url().toString();

            return new GroupJoinResponse(groupGuid, postAuth(session, request, QByteArray()));
        }

        GroupLeaveResponse* GroupServiceClient::leaveGroupPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/member/" + nucleusId(session)));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService leaveGroup req: url=" << request.url().toString();

            return new GroupLeaveResponse(groupGuid, deleteResourceAuth(session, request));
        }

        GroupPendingInviteResponse* GroupServiceClient::getPendingGroupInvitesPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/invited"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("userId", nucleusId(session));

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService getPendingGroupInvites req: url=" << request.url().toString();

            return new GroupPendingInviteResponse(getAuth(session, request));
        }

        GroupDeclinePendingInviteResponse* GroupServiceClient::declinePendingGroupInvitePriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/invited/" + nucleusId(session)));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService declinePendingGroupInvite req: url=" << request.url().toString();

            return new GroupDeclinePendingInviteResponse(deleteResourceAuth(session, request));
        }

        GroupConfigResponse* GroupServiceClient::readGroupConfigPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid));
            QUrlQuery serviceUrlQuery(serviceUrl);
            
            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService readGroupConfig req: url=" << request.url().toString();

            return new GroupConfigResponse(getAuth(session, request));
        }

        GroupListResponse* GroupServiceClient::listGroupsPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("userId", nucleusId(session));
            serviceUrlQuery.addQueryItem("pagestart", "0");
            serviceUrlQuery.addQueryItem("pagesize", "100");

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService listGroups req: url=" << request.url().toString();

            return new GroupListResponse(getAuth(session, request));
        }

        GroupMemberListResponse* GroupServiceClient::listGroupMembersPriv(Session::SessionRef session, const QString& groupGuid, int pagestart)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/members"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("userId", nucleusId(session));
            serviceUrlQuery.addQueryItem("pagestart", QString::number(pagestart));
            serviceUrlQuery.addQueryItem("pagesize", QString::number(GROUP_MEMBER_LIST_RESPONSE_PAGESIZE));

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService listGroupMembers req: url=" << request.url().toString();

            return new GroupMemberListResponse(groupGuid, getAuth(session, request), GROUP_MEMBER_LIST_RESPONSE_PAGESIZE);
        }

        GroupRemoveMemberResponse* GroupServiceClient::removeGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/member/" + userId));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService removeGroupMember req: url=" << request.url().toString();

            return new GroupRemoveMemberResponse(groupGuid, deleteResourceAuth(session, request));
        }

        GroupBanMemberResponse* GroupServiceClient::banGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/banned/" + userId));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService banGroupMember req: url=" << request.url().toString();

            return new GroupBanMemberResponse(groupGuid, postAuth(session, request, QByteArray()));
        }

        GroupBannedListResponse* GroupServiceClient::listBannedGroupMembersPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/banned"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("pagestart", "0");
            serviceUrlQuery.addQueryItem("pagesize", "100");

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService listBannedGroupMembers req: url=" << request.url().toString();

            return new GroupBannedListResponse(groupGuid, getAuth(session, request));
        }

        GroupUnbanMemberResponse* GroupServiceClient::unbanGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/banned/" + userId));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService unbanGroupMember req: url=" << request.url().toString();

            return new GroupUnbanMemberResponse(groupGuid, deleteResourceAuth(session, request));
        }

        GroupInviteMemberResponse* GroupServiceClient::inviteGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/invited/" + userId));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService inviteGroupMember req: url=" << request.url().toString();

            return new GroupInviteMemberResponse(groupGuid, postAuth(session, request, QByteArray()));
        }

        GroupUserUpdateRoleResponse* GroupServiceClient::updateGroupUserRolePriv(Session::SessionRef session, const QString& groupGuid, const QString& userId, const QString& role)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/member/" + userId + "/role"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("newRole", role);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService updateGroupUserRole req: url=" << request.url().toString();

            return new GroupUserUpdateRoleResponse(groupGuid, putAuth(session, request, QByteArray()));
        }

        GroupListInvitedMemberResponse* GroupServiceClient::listInviteGroupMemberPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QUrl serviceUrl(urlForServicePath("/group/instance/" + groupGuid + "/invited"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService listInviteGroupMember req: url=" << request.url().toString();

            return new GroupListInvitedMemberResponse(groupGuid, getAuth(session, request));
        }

        GroupRoomPresenceResponse* GroupServiceClient::listGroupRoomPresencePriv(Session::SessionRef session, const QString& jid)
        {
            QUrl serviceUrl(urlForServicePath("/group/presence/gac"));
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("roomjid", jid);
            serviceUrlQuery.addQueryItem("pagestart", "0");
            serviceUrlQuery.addQueryItem("pagesize", "100");

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService listGroupRoomPresence req: url=" << request.url().toString();

            return new GroupRoomPresenceResponse(getAuth(session, request));
        }

        GroupMucListResponse* GroupServiceClient::listMucsInGroupPriv(Session::SessionRef session, const QString& groupGuid)
        {
            QString chatURL = Origin::Services::readSetting(Origin::Services::SETTING_ChatURL).toString();

            QUrl serviceUrl(chatURL + "/chat/muc/groupmuc/" + groupGuid);
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request = addGOSHeaders(request, session);

            ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                << "GroupService listMucsInGroup req: url=" << request.url().toString();

            return new GroupMucListResponse(groupGuid, getAuth(session, request));
        }
    }
}