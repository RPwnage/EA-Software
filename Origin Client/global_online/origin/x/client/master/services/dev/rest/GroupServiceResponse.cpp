///////////////////////////////////////////////////////////////////////////////
// VoiceServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QJsonDocument>
#include "services/rest/GroupServiceResponse.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        GroupResponse::GroupResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
            , mGroupGuid(groupGuid)
            , mGroupResponseError(GroupResponse::NoError)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        GroupDeleteResponse::GroupDeleteResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupDeleteResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupCreateResponse::GroupCreateResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupCreateResponse::processReply()
        {
            QNetworkReply::NetworkError netError = reply()->error();

            if (netError == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService createGroup resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();

                    mGroupGuid = obj["_id"].toString();
                    mGroupName = obj["name"].toString();
                    mGroupShortName = obj["shortName"].toString();

                    emit success();
                }
                else
                {
                    emit error(Http400ClientErrorBadRequest);
                }
            }
            else
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService createGroup resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();
                    QJsonArray errArray = obj["errors"].toArray() ;
                    if (errArray.size() > 0)
                    {
                        QString error = errArray.at(0).toString();
                        if (error.length()) 
                        {
                            mGroupResponseError = GroupResponse::UnknownGroupCreateError;
                            if (error == "Too many groups created in a day")
                            {
                                mGroupResponseError = GroupResponse::TooManyGroupCreatePerDayError;
                            } else if (error == "Too many groups created in a minute")
                            {
                                mGroupResponseError = GroupResponse::TooManyGroupCreatePerMinuteError;
                            } 
                            emit createGroupFailed();
                            return;
                        }
                    }
                }
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupJoinResponse::GroupJoinResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupJoinResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupLeaveResponse::GroupLeaveResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupLeaveResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupChangeNameResponse::GroupChangeNameResponse(const QString& groupGuid, const QString& groupName, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
            , mGroupName(groupName)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupChangeNameResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupPendingInviteResponse::GroupPendingInviteResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupPendingInviteResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService pendingInvite resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonArray arr = jsonDoc.array();

                    foreach (const QJsonValue & value, arr)
                    {
                        QJsonObject obj = value.toObject();
                        PendingGroupInfo pendingGroupInfo;
                        pendingGroupInfo.groupGuid = obj["_id"].toString();
                        pendingGroupInfo.groupName = obj["name"].toString();
                        pendingGroupInfo.invitedBy = obj["by"].toDouble();

                        mPendingGroups.push_back(pendingGroupInfo);
                    }                
                }

                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupDeclinePendingInviteResponse::GroupDeclinePendingInviteResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupDeclinePendingInviteResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupConfigResponse::GroupConfigResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupConfigResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService config resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();

                    mGroupGuid = obj["_id"].toString();
                    mGroupName = obj["name"].toString();
                    mGroupShortName = obj["shortName"].toString();                   
                }

                 emit success();
            }
        }

        GroupListResponse::GroupListResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupListResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService listGroups resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonArray arr = jsonDoc.array();

                    foreach (const QJsonValue & value, arr)
                    {
                        QJsonObject obj = value.toObject();
                        GroupInfo groupInfo;
                        groupInfo.groupGuid = obj["_id"].toString();
                        groupInfo.groupName = obj["name"].toString();
                        groupInfo.userRole = obj["r"].toString();

                        mGroups.push_back(groupInfo);
                    }
                }

                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupMemberListResponse::GroupMemberListResponse(const QString& groupGuid, AuthNetworkRequest reply, const int pagesize)
            : GroupResponse(groupGuid, reply)
            , mPageSize(pagesize)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupMemberListResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService listGroupMembers resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonArray arr = jsonDoc.array();

                    foreach (const QJsonValue & value, arr)
                    {
                        QJsonObject obj = value.toObject();
                        MemberInfo memberInfo;
                        memberInfo.userId = obj["id"].toDouble();
                        memberInfo.userRole = obj["r"].toString();
                        memberInfo.since = obj["since"].toDouble();
                        memberInfo.timestamp = obj["ts"].toDouble();

                        mMembers.push_back(memberInfo);
                    }
                }

                emit success();
            }
        }

        GroupRemoveMemberResponse::GroupRemoveMemberResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupRemoveMemberResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error(Http400ClientErrorBadRequest);
            }
        }

        GroupBanMemberResponse::GroupBanMemberResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupBanMemberResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
        }

        GroupBannedListResponse::GroupBannedListResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupBannedListResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
        }

        GroupUnbanMemberResponse::GroupUnbanMemberResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupUnbanMemberResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
        }

        GroupInviteMemberResponse::GroupInviteMemberResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupInviteMemberResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
			else
			{
				emit failure();
			}
        }

        GroupListInvitedMemberResponse::GroupListInvitedMemberResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupListInvitedMemberResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                emit success();
            }
        }

        GroupUserUpdateRoleResponse::GroupUserUpdateRoleResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            mFailed = true;
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupUserUpdateRoleResponse::processReply()
        {
            mFailed = ( reply()->error() != QNetworkReply::NoError );
            if (mFailed)
            {
                QJsonParseError jsonResult;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(reply()->readAll(), &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();
                    QJsonArray errArray = obj["errors"].toArray() ;
                    if (errArray.size() > 0)
                    {
                        mGroupResponseError = GroupResponse::UnknownGroupUserUpdateError;

                        QString error = errArray.at(0).toString();
                        if (error.length()) 
                        {
                            if (error == "User is already in the role admin")
                            {
                                mGroupResponseError = GroupResponse::GroupUserAlreadyAdminError;
                            }
                        }

                    }
                }
            }

        }

        GroupRoomPresenceResponse::GroupRoomPresenceResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupRoomPresenceResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService listGroupRoomPresence resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonArray arr = jsonDoc.array();

                    foreach (const QJsonValue & value, arr)
                    {
                        QJsonObject obj = value.toObject();
                        UserPresenceInfo userPresenceInfo;
                        userPresenceInfo.jid = obj["jid"].toString();
                        userPresenceInfo.type = obj["type"].toString();
                        userPresenceInfo.title = obj["title"].toString();
                        userPresenceInfo.productId = obj["prorudctId"].toString();
                        userPresenceInfo.mode = obj["mode"].toString();
                        userPresenceInfo.gamePresence = obj["gamePresence"].toString();
                        userPresenceInfo.multiplayerId = obj["multiplayerId"].toString();
                        userPresenceInfo.time = obj["time"].toDouble();

                        mUserPresence.push_back(userPresenceInfo);
                    }
                }

                emit success();
            }
        }

        GroupMucListResponse::GroupMucListResponse(const QString& groupGuid, AuthNetworkRequest reply)
            : GroupResponse(groupGuid, reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GroupMucListResponse::processReply()
        {
            if (reply()->error() == QNetworkReply::NoError)
            {
                QJsonParseError jsonResult;
                QByteArray response(reply()->readAll());

                ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogGroupServiceREST))
                    << "GroupService listMucsInGroup resp: json=" << response;

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonArray arr = jsonDoc.array();

                    foreach (const QJsonValue & value, arr)
                    {
                        QJsonObject obj = value.toObject();
                        ChannelInfo channelInfo;
                        channelInfo.roomJid = obj["roomJid"].toString();
                        channelInfo.roomName = obj["roomName"].toString();
                        channelInfo.passwordProtected = obj["passwordProtected"].toBool();
                        mChannels.push_back(channelInfo);
                    }                
                }

                emit success();
            }
            else
            {
                emit error( Http400ClientErrorBadRequest );
            }

        }
    }
}
