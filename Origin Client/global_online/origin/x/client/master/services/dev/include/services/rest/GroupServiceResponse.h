///////////////////////////////////////////////////////////////////////////////
// GroupServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _GROUP_SERVICE_RESPONSE_H
#define _GROUP_SERVICE_RESPONSE_H

#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"

#include <QMap>

namespace Origin
{
    namespace Services
    {
        class GroupResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
            Q_ENUMS(GroupResponseError)
        public:
            enum GroupResponseError {
                NoError = 0,

                // GroupCreateResponse errors (1-99):
                TooManyGroupCreatePerDayError = 1,
                TooManyGroupCreatePerMinuteError,
                UnknownGroupCreateError = 99,

                // GroupUserUpdateRoleResponse (101-199):
                GroupUserAlreadyAdminError = 101,
                UnknownGroupUserUpdateError = 199,
            };

            explicit GroupResponse(const QString& groupGuid, AuthNetworkRequest);

            const QString& getGroupGuid() const
            {
                return mGroupGuid;
            }

            GroupResponseError getGroupResponseError() { return mGroupResponseError; }

        protected:
            virtual void processReply() = 0;

            QString mGroupGuid;
            GroupResponseError mGroupResponseError;
        };

        class GroupDeleteResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupDeleteResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupCreateResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupCreateResponse(const QString& groupGuid, AuthNetworkRequest);

            const QString& getGroupGuid()
            {
                return mGroupGuid;
            }

            const QString& getGroupName()
            {
                return mGroupName;
            }

            const QString& getGroupShortName()
            {
                return mGroupShortName;
            }

        signals:
            void createGroupFailed();

        protected:
            void processReply();

            QString mGroupGuid;
            QString mGroupName;
            QString mGroupShortName;
        };

        class GroupJoinResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupJoinResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupLeaveResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupLeaveResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupChangeNameResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupChangeNameResponse(const QString& groupGuid, const QString& groupName, AuthNetworkRequest);

            QString getGroupName() const
            {
                return mGroupName;
            }

        protected:
            void processReply();

            QString mGroupName;
        };

        class GroupPendingInviteResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GroupPendingInviteResponse(AuthNetworkRequest);

            struct PendingGroupInfo
            {
                QString groupGuid;
                QString groupName;
                qint64 invitedBy;
            };

            const QVector<PendingGroupInfo> getPendingGroups() const
            {
                return mPendingGroups;
            }

        protected:
            void processReply();

            QVector<PendingGroupInfo> mPendingGroups;
        };

        class GroupDeclinePendingInviteResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GroupDeclinePendingInviteResponse(AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupConfigResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GroupConfigResponse(AuthNetworkRequest);

            const QString& getGroupGuid()
            {
                return mGroupGuid;
            }

            const QString& getGroupName()
            {
                return mGroupName;
            }

            const QString& getGroupShortName()
            {
                return mGroupShortName;
            }

        protected:
            void processReply();

            QString mGroupGuid;
            QString mGroupName;
            QString mGroupShortName;
        };

        class GroupListResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GroupListResponse(AuthNetworkRequest);

            struct GroupInfo
            {
                QString groupGuid;
                QString groupName;
                QString userRole;
            };

            const QVector<GroupInfo>& getGroups() const
            {
                return mGroups;
            }

        protected:
            void processReply();

            QVector<GroupInfo> mGroups;
        };

        class GroupMemberListResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupMemberListResponse(const QString& groupGuid, AuthNetworkRequest, const int pagesize);

            struct MemberInfo
            {
                qint64 userId;
                QString userRole;
                qint64 since;
                qint64 timestamp;
            };

            const QVector<MemberInfo>& getMembers() const
            {
                return mMembers;
            }

            const int getPageSize() const { return mPageSize; }

        protected:
            void processReply();

            QVector<MemberInfo> mMembers;
            const int mPageSize;
        };
        typedef QVector<Origin::Services::GroupMemberListResponse::MemberInfo> MemberInfoVector;

        class GroupRemoveMemberResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupRemoveMemberResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupBanMemberResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupBanMemberResponse(const QString& groupGuid, AuthNetworkRequest);

       protected:
            void processReply();
        };

        class GroupBannedListResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupBannedListResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupUnbanMemberResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupUnbanMemberResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupInviteMemberResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupInviteMemberResponse(const QString& groupGuid, AuthNetworkRequest);

        signals:
			void failure();

        protected:
            void processReply();
        };

        class GroupListInvitedMemberResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupListInvitedMemberResponse(const QString& groupGuid, AuthNetworkRequest);

        protected:
            void processReply();
        };

        class GroupUserUpdateRoleResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupUserUpdateRoleResponse(const QString& groupGuid, AuthNetworkRequest);
            bool failed() { return mFailed; }

		protected:
            void processReply();

        private:
            bool mFailed;
        };

        class GroupRoomPresenceResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GroupRoomPresenceResponse(AuthNetworkRequest);

            struct UserPresenceInfo
            {
                QString jid;
                QString type;
                QString title;
                QString productId;
                QString mode;
                QString gamePresence;
                QString multiplayerId;
                qint64 time;
            };

            const QVector<UserPresenceInfo>& getUserPresence() const
            {
                return mUserPresence;
            }

        protected:
            void processReply();

            QVector<UserPresenceInfo> mUserPresence;
        };

        class GroupMucListResponse : public GroupResponse
        {
            Q_OBJECT
        public:
            explicit GroupMucListResponse(const QString& groupGuid, AuthNetworkRequest);

            struct ChannelInfo
            {
                QString roomJid;
                QString roomName;
                bool passwordProtected;
            };

            const QVector<ChannelInfo>& getChannels() const
            {
                return mChannels;
            }

        protected:
            void processReply();

            QVector<ChannelInfo> mChannels;
        };
    }
}

#endif //_GROUP_SERVICE_RESPONSE_H


