#ifndef _CHATMODEL_CHATGROUP_H
#define _CHATMODEL_CHATGROUP_H

#include <QObject>
#include <QVector>
#include <QSet>
#include "Connection.h"
#include "MucRoom.h"
#include <services/rest/GroupServiceResponse.h>

namespace Origin
{
    namespace Chat
    {
        class ChatChannel;

        class ChatGroup : public QObject
        {
            Q_OBJECT 
        public:
            ChatGroup(Connection *connection, const QString& name, const QString& groupGuid, const qint64& invitedBy, const QString& role, bool groupInvite, QObject* parent = NULL);
            virtual ~ChatGroup();

            ChatChannel* addChannel(const QString& channelName, bool isPasswordProtected, const QString& channelJid = "");

            ///
            /// Destroys the given channel
            /// \param channelId    The id for the channel to destroy
            /// \param nickName     The nick name of user destroying the channel
            /// \param nucleusId    The nick nucleus id of user destroying the channel
            /// \param password     The password for entering the channel
            ///
            void destroyChannel(const QString& channelId, const QString& nickName, const QString& nucleusId, const QString& password = "");

            ///
            /// Joins the given channel with the specified password
            /// \param channelId    The id of the channel to join
            /// \param password     The password for this channel
            ///
            void joinChannel(const QString& channelId, const QString& password, bool failSilently = false);

            ///
            /// Prepare for deletion
            ///
            void prepareForDeletion();

            ///
            /// Gets the information for all channels in the group
            ///
            void getGroupChannelInformation();

            ///
            /// Queries all of the members for this group
            ///
            void queryGroupMembers();

            ///
            /// Removes a user from the given channel
            /// \param channelId    The identifier for this channel
            /// \param userId       The user to remove from the channel
            /// \param by           The Origin Id of the user who is doing the removal
            /// \return             Returns if this operation succeeded or not
            ///
            bool removeUserFromChannel(const QString& channelId, const QString& userId, const QString& by);

            ///
            /// Sets the name of this group
            ///
            void setGroupName(const QString& groupName)
            {
                mGroupName = groupName;
            }

            ///
            /// Returns the name of this group
            ///
            const QString& getName() const
            {
                return mGroupName;
            }

            ///
            /// Returns the guid for this group
            ///
            const QString& getGroupGuid() const
            {
                return mGroupGuid;
            }

            ///
            /// Returns the current users role in the group
            ///
            const QString& getGroupRole() const
            {
                return mGroupRole;
            }

            ///
            /// Returns if this group is for an invite
            ///
            bool isInviteGroup() const
            {
                return mGroupInvite;
            }

            ///
            /// Returns the inviter, if known
            ///
            qint64 getInvitedBy() const
            {
                return mInvitedBy;
            }

            void deactivateRooms(MucRoom::DeactivationType type, const QString& reason);

            void editMembersRole(quint64 userId, const QString& role);

            void editCurrentUserRole(const quint64 userId, const QString& role);

            void inviteUsersToGroup(QList<Origin::Chat::RemoteUser*> users);

            ///
            /// Handle client side updates after receiving DirtyBits messages
            ///
            void handleRoomDestroyed(const QString& channelId);

            ///
            /// Accepts an invite to this chat group
            ///
            void acceptGroupInvite();

            ///
            /// Rejects an invite to this chat group
            ///
            void rejectGroupInvite();

            ///
            /// Returns the current members list
            ///
            QList<qint64> getMembersList() const
            {
                return mMembers;
            };

            ///
            /// Returns the admins list
            ///
            QList<qint64> getAdminsList() const
            {
                return mAdmins;
            };

            ///
            /// Returns the owners list
            ///
            QList<qint64> getOwnersList() const
            {
                return mOwners;
            };

            ///
            /// Returns true if the user is in this group
            ///
            bool isUserInGroup(quint64 id);

            ///
            /// Returns the group role, or blank if the user is not in the group
            ///
            QString getUserRole(qint64 id);

            void removeMember(quint64 userId);

            ChatChannel* channelForId(QString channelId);

            ChatChannel* channelForChannelId(QString channelId);
            
            const QList<ChatChannel*> channels() { return mChatChannels; }

            ///
            /// Returns the group default channel
            ///
            ChatChannel* getDefaultChannel();

            ///
            /// Returns the count of group channels. 
            ///
            int getChannelCount() { return mChatChannels.length(); }
            
        signals:
            // updates the UI
            void channelAdded(Origin::Chat::ChatChannel* channel);
            void channelRemoved(Origin::Chat::ChatChannel* channel);
            void channelChanged(Origin::Chat::ChatChannel* channel);
            void lastChannelRemoved();
            void usersInvitedToGroup(QList<Origin::Chat::RemoteUser*>);

            void groupMembersQueryStart();
            void groupMembersLoadFinished();
            void groupMemberRemoved(Origin::Chat::JabberID);
            void groupMembersRoleChange(quint64 userId, const QString& role);

            void groupInviteAccepted();
            void groupInviteDeclined();

            void rejectInviteFailed();
            void acceptInviteFailed();

            void enterMucRoomFailed(int);
            void rejoinMucRoomSuccess();

            void groupChannelInformationReceived();
            void groupChannelPresenceReceived();
            void groupChannelGetInfoFailed();

        public slots:
            void onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus);

        private slots:
            void onListChannels();
            void onListChannelsError(Origin::Services::HttpStatusCode);
            void onGetGroupMembers();
            void onChannelUpdated();
            void onRoomDestroyed(const QString& channelId);

            void onAcceptInviteSuccess();
            void onAcceptInviteFailed();
            void onRejectInviteSuccess();
            void onRejectInviteFailed();
            void onRetryGroupChannelQuery();

        private:
            Connection* mConnection;

            QString mGroupName;
            QString mGroupGuid;
            qint64 mInvitedBy;
            QString mGroupRole;

            QList<ChatChannel*> mChatChannels;
            
            Services::MemberInfoVector mGroupMembersVector;

            QList<qint64> mMembers;
            QList<qint64> mAdmins;
            QList<qint64> mOwners; 

            bool mQueriedChannels;
            int mChannelQueryRetryAttempts;
            bool mGroupInvite;

            void addUserToMembersList(qint64 , const QString&);
            void removeUserFromMembersList(qint64);
        };

        class ChatGroups : public QObject
        {
            Q_OBJECT
        public:
            ChatGroups(Connection* connection, QObject* parent = NULL);
            virtual ~ChatGroups();

            ///
            /// Initializes all of the groups
            ///
            void initializeGroups();

            ///
            /// Adds a group for us to accept our invite from
            /// \param groupGuid            The guid for this group
            /// \param groupName            The name for this group
            /// \param invitedBy            The inviter for this group
            ///
            void addGroupInvite(const QString& groupGuid, const QString& groupName, const qint64& invitedBy);

            ///
            /// Creates a new chat group
            /// \param groupGuid            The guid for this group
            /// \param groupName            The name for this group
            ///
            void createGroup(const QString& groupGuid, const QString& groupName);

            ///
            /// Joins a chat group
            /// \param groupGuid            The guid for this group
            /// \param groupName            The name for this group
            ///
            void joinGroup(const QString& groupGuid, const QString& groupName);

            ///
            /// Removes the chat group from the UI
            /// \param groupGuid            The guid for this group
            ///
            void removeGroupUI(const QString& groupGuid);

            ///
            /// Leaves a chat group
            /// \param groupGuid            The guid for this group
            ///
            void leaveGroup(const QString& groupGuid);

            ///
            /// Prepare for a group to be deleted
            /// \param groupGuid            The guid for the groud to be deleted
            ///
            void prepareForGroupDeletion(const QString& groupGuid);

            ///
            /// Edits the name of a chat group
            /// \param groupGuid            The guid for this group
            /// \param groupName            The new name for this group
            ///
            void editGroupName(const QString& groupGuid, const QString& groupName);

            void editGroupMemberRole(const QString& groupGuid, quint64 userId, const QString& role);

            void editCurrentUserGroupRole(const QString& groupGuid, quint64 userId, const QString& role);

            void inviteUserToGroup(const QString& groupGuid, Origin::Chat::RemoteUser* user);
            void addUsersToGroup(const QString& groupGuid, QList<Origin::Chat::RemoteUser*> users);

            ///
            /// Adds a chat channel to a chat group
            /// \param groupGuid            The guid for this group
            /// \param channelName          The name for this chat channel
            /// \param locked               Whether or not this chat channel is locked
            /// \param channelJid           The prespecified JID for thsi channel
            /// \return ChatChannel         The new chat channel
            ///
            ChatChannel* addChannelToGroup(const QString& groupGuid, const QString& channelName, bool locked, const QString& channelJid = "");
            void removeChannelFromGroup(const QString& groupGuid, const QString& channelId, const QString& nickName, const QString& nucleusId, const QString& password = "");
            void enterChannelInGroup(const QString& groupGuid, const QString& channelId, const QString& password);
            void removeUserFromChannel(const QString& groupGuid, const QString& channelId, const QString& userId, const QString& by);

            ///
            /// Handle client side updates after receiving DirtyBits messages
            ///
            void handleRoomDestroyed(const QString& groupGuid, const QString& channelId);

            ///
            /// Returns the group for the given id
            /// \param groupGuid            The guid we want a group for
            /// \return ChatGroup           The group the matches the given id
            ///
            ChatGroup* chatGroupForGroupGuid(const QString& groupGuid);

            void removeMemberFromGroup(const QString& groupGuid, quint64 userId);


            ///
            /// Returns list of groups
            const QSet<ChatGroup*> getGroupList() const {return mChatGroups;}
            
            /// Returns the number of chat groups
            unsigned int count() const { return mChatGroups.size(); }
            
            // Delete the group
            // NOTE: Does NOT remove the UI. Use removeGroupUI() for that.
            void deleteGroup(const QString& groupGuid);

        protected slots:
            void onListGroupsSuccess();
            void onListGroupsFailed();
            void onListPendingGroupsSuccess();
            void onLastChannelRemoved();

            void onGroupInviteAccepted();
            void onGroupInviteDeclined();

        signals:
            void groupAdded(Origin::Chat::ChatGroup* group);
            void groupRemoved(Origin::Chat::ChatGroup* group);
            void groupChanged(Origin::Chat::ChatGroup* group);
            void userInvitedToGroup(const QString& groupGuid);

            void listGroupsSuccess();
            void listGroupsFailed();
            void acceptInviteFailed();
            void rejectInviteFailed();

        private:
            QSet<ChatGroup*> mChatGroups;
            QSet<ChatGroup*> mInviteGroups;
            
            Connection* mConnection;
        };
    }
}

#endif
