///////////////////////////////////////////////////////////////////////////////
// VoiceServiceClient.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _GROUP_SERVICE_CLIENT_H
#define _GROUP_SERVICE_CLIENT_H

#include "OriginAuthServiceClient.h"
#include "GroupServiceResponse.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// \brief HTTP service client for the Group server
        ///
        class GroupServiceClient : public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<GroupServiceClient>;

            ///
            /// \brief Creates a group
            ///
            static GroupCreateResponse* createGroup(Session::SessionRef session, const QString& groupName, const QString& shortName)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->createGroupPriv(session, groupName, shortName);
            }

            static GroupDeleteResponse* deleteGroup(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->deleteGroupPriv(session, groupGuid);
            }

            static GroupJoinResponse* joinGroup(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->joinGroupPriv(session, groupGuid);
            }

            static GroupLeaveResponse* leaveGroup(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->leaveGroupPriv(session, groupGuid);
            }

            static GroupPendingInviteResponse* getPendingGroupInvites(Session::SessionRef session)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->getPendingGroupInvitesPriv(session);
            }

            static GroupDeclinePendingInviteResponse* declinePendingGroupInvite(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->declinePendingGroupInvitePriv(session, groupGuid);
            }

            static GroupChangeNameResponse* changeGroupName(Session::SessionRef session, const QString& groupGuid, const QString& newGroupName)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->changeGroupNamePriv(session, groupGuid, newGroupName);
            }

            static GroupConfigResponse* readGroupConfig(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->readGroupConfigPriv(session, groupGuid);
            }

            static GroupListResponse* listGroups(Session::SessionRef session)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->listGroupsPriv(session);
            }

            static GroupMemberListResponse* listGroupMembers(Session::SessionRef session, const QString& groupGuid, int pagestart)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->listGroupMembersPriv(session, groupGuid, pagestart);
            }

            static GroupRemoveMemberResponse* removeGroupMember(Session::SessionRef session, const QString& groupGuid, const QString& userId)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->removeGroupMemberPriv(session, groupGuid, userId);
            }

            static GroupBanMemberResponse* banGroupMember(Session::SessionRef session, const QString& groupGuid, const QString& userId)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->banGroupMemberPriv(session, groupGuid, userId);
            }

            static GroupBannedListResponse* listBannedGroupMembers(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->listBannedGroupMembersPriv(session, groupGuid);
            }

            static GroupUnbanMemberResponse* unbanGroupMember(Session::SessionRef session, const QString& groupGuid, const QString& userId)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->unbanGroupMemberPriv(session, groupGuid, userId);
            }

            static GroupInviteMemberResponse* inviteGroupMember(Session::SessionRef session, const QString& groupGuid, const QString& userId)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->inviteGroupMemberPriv(session, groupGuid, userId);
            }

            static GroupUserUpdateRoleResponse* updateGroupUserRole(Session::SessionRef session, const QString& groupGuid, const QString& userId, const QString& role)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->updateGroupUserRolePriv(session, groupGuid, userId, role);
            }

            static GroupListInvitedMemberResponse* listInviteGroupMember(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->listInviteGroupMemberPriv(session, groupGuid);
            }

            static GroupRoomPresenceResponse* listGroupRoomPresence(Session::SessionRef session, const QString& jid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->listGroupRoomPresencePriv(session, jid);
            }

            static GroupMucListResponse* listMucsInGroup(Session::SessionRef session, const QString& groupGuid)
            {
                return OriginClientFactory<GroupServiceClient>::instance()->listMucsInGroupPriv(session, groupGuid);
            }

        private:
            /// 
            /// \brief Creates a new group service client
            ///
            /// \param baseUrl       Base URL for the friends service API.
            /// \param nam           QNetworkAccessManager instance to send requests through.
            ///
            explicit GroupServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager* nam = NULL);

            ///
            /// \brief Creates a group
            ///
            GroupCreateResponse* createGroupPriv(Session::SessionRef session, const QString& groupName, const QString& shortName);
        
            ///
            /// \brief Deletes a group
            ///
            GroupDeleteResponse* deleteGroupPriv(Session::SessionRef session, const QString& groupGuid);

            ///
            /// \brief Joins a group
            ///
            GroupJoinResponse* joinGroupPriv(Session::SessionRef session, const QString& groupGuid);

            ///
            /// \brief Leaves a group
            ///
            GroupLeaveResponse* leaveGroupPriv(Session::SessionRef session, const QString& groupGuid);

            ///
            /// \brief Gets all of the groups a user has a pending invite to
            ///
            GroupPendingInviteResponse* getPendingGroupInvitesPriv(Session::SessionRef session);

            GroupDeclinePendingInviteResponse* declinePendingGroupInvitePriv(Session::SessionRef session, const QString& groupGuid);

            ///
            /// \brief Changes a group name
            ///
            GroupChangeNameResponse* changeGroupNamePriv(Session::SessionRef session, const QString& groupGuid, const QString& newGroupName);

            ///
            /// \brief Reads a group config
            ///
            GroupConfigResponse* readGroupConfigPriv(Session::SessionRef session, const QString& groupGuid);

            ///
            /// \brief Lists all groups that a user is in
            ///
            GroupListResponse* listGroupsPriv(Session::SessionRef session);

            ///
            /// \brief Lists all members of a group
            ///
            GroupMemberListResponse* listGroupMembersPriv(Session::SessionRef session, const QString& groupGuid, int pagestart);
        
            ///
            /// \brief Removes a member from a group
            ///
            GroupRemoveMemberResponse* removeGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId);

            ///
            /// \brief Bans a member from a group
            ///
            GroupBanMemberResponse* banGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId);

            ///
            /// \brief Lists all banned members of a group
            ///
            GroupBannedListResponse* listBannedGroupMembersPriv(Session::SessionRef session, const QString& groupGuid);

            ///
            /// \brief Unbans a member from a group
            ///
            GroupUnbanMemberResponse* unbanGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId);

            ///
            /// \brief Invites a member to a group
            ///
            GroupInviteMemberResponse* inviteGroupMemberPriv(Session::SessionRef session, const QString& groupGuid, const QString& userId);
        
            ///
            /// \brief Updates a users role in a group
            ///
            GroupUserUpdateRoleResponse* updateGroupUserRolePriv(Session::SessionRef session, const QString& groupGuid, const QString& userId, const QString& role);

            ///
            /// \brief Lists all members invited to a group
            ///
            GroupListInvitedMemberResponse* listInviteGroupMemberPriv(Session::SessionRef session, const QString& groupGuid);
        
            ///
            /// \brief Lists the presence for all users in a room
            ///
            GroupRoomPresenceResponse* listGroupRoomPresencePriv(Session::SessionRef session, const QString& jid);

            ///
            /// \brief Lists all muc rooms in a group
            ///
            GroupMucListResponse* listMucsInGroupPriv(Session::SessionRef session, const QString& groupGuid);
        };
    }
}

#endif //_GROUP_SERVICE_CLIENT_H