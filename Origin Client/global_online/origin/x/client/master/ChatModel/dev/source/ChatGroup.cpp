#include "ChatGroup.h"
#include "ChatChannel.h"
#include "RemoteUser.h"
#include "BlockList.h"
#include "ConnectedUser.h"
#include "services/session/SessionService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include <services/rest/GroupServiceClient.h>
#include <QTimer>
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Chat
    {
        ChatGroup::ChatGroup(Connection *connection, const QString& groupName, const QString& groupGuid, const qint64& invitedBy,
            const QString& groupRole, bool groupInvite, QObject* parent /* = NULL */)
            : QObject(parent)
            , mConnection(connection)
            , mGroupName(groupName)
            , mGroupGuid(groupGuid)
            , mInvitedBy(invitedBy)
            , mGroupRole(groupRole)
            , mQueriedChannels(false)
            , mChannelQueryRetryAttempts(0)
            , mGroupInvite(groupInvite)
        {
            // Set our Guid as a property so when we delete the group
            // the QObject will still know about the Guid
            this->setProperty("groupGuid", mGroupGuid);
        }

        ChatGroup::~ChatGroup()
        {
            for (int i = 0; i < mChatChannels.length(); i++)
            {
                ChatChannel* channel = mChatChannels.takeAt(0);
                delete channel;
            }
        }

        ChatChannel* ChatGroup::addChannel(const QString& channelName, bool isPasswordProtected, const QString& channelJid /* = "" */)
        {
            // Make sure we don't already have this channel here
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                QString channelId = channelJid.split('@')[0];
                if (channel && channel->getChannelId().compare(channelId) == 0)
                {
                    return channel;
                }
            }

            ChatChannel* channel = new ChatChannel(mConnection, this, channelJid, channelName, isPasswordProtected);
            mChatChannels.push_back(channel);

            emit channelAdded(channel);

            ORIGIN_VERIFY_CONNECT(channel, SIGNAL(channelUpdated()), 
                this, SLOT(onChannelUpdated()));

            return channel;
        }

        void ChatGroup::destroyChannel(const QString& channelId, const QString& nickName, const QString& nucleusId, const QString& password)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                if (channel && channel->getChannelId().compare(channelId) == 0)
                {
                    ORIGIN_VERIFY_CONNECT(channel, SIGNAL(roomDestroyed(const QString&)),
                        this, SLOT(onRoomDestroyed(const QString&)));

                    channel->destroyChannel(nickName, nucleusId, password); // destroy channel on server
                    break;
                }
            }
        }

        void ChatGroup::joinChannel(const QString& channelId, const QString& password, bool failSilently)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                if (channel && channel->getChannelId().compare(channelId) == 0)
                {
                    channel->joinChannel(password, failSilently);
                    break;
                }
            }
        }

        void ChatGroup::onRetryGroupChannelQuery() 
        {
            ORIGIN_LOG_WARNING << "Retrying for Group Channel information " << 5 - mChannelQueryRetryAttempts << " more times";
            this->getGroupChannelInformation();
        }

        void ChatGroup::getGroupChannelInformation()
        {
            if (!mQueriedChannels)
            {                
                mQueriedChannels = true;

                // Need to find out what channels are associated with this group
                Origin::Services::GroupMucListResponse* resp = Services::GroupServiceClient::listMucsInGroup(
                    Services::Session::SessionService::currentSession(), mGroupGuid);
                resp->setDeleteOnFinish(true);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onListChannels()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onListChannelsError(Origin::Services::HttpStatusCode)));
            }
            else
            {
                // If we already know our channels then just grab the info about the channels themselves
                for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
                {
                    ChatChannel* channel = *i;
                    channel->getPresence();
                }

                emit groupChannelPresenceReceived();
            }
        }

        void ChatGroup::onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus status)
        {
            emit enterMucRoomFailed(status);
        }

        void ChatGroup::onListChannelsError(Origin::Services::HttpStatusCode error)
        {
            mQueriedChannels = false;
            if (!mGroupInvite) 
            {
                ORIGIN_LOG_WARNING<<"Failed to find Channel list for Group: " << mGroupGuid;
                if (mChannelQueryRetryAttempts < 5)
                {
                    mChannelQueryRetryAttempts++;
                    QTimer::singleShot(1000, this, SLOT(onRetryGroupChannelQuery()));
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Failed to find any Group Channel Information for Group: " << mGroupGuid;;
                    ORIGIN_LOG_ERROR << "Http error code: " << error;
                    emit groupChannelGetInfoFailed();
                }
            }
            else
            {
                // If this is an invite no need to retry
                ORIGIN_LOG_WARNING<<"Failed to find Channel list for Invite: " << mGroupGuid;
            }
        }

        void ChatGroup::onListChannels()
        {
            Origin::Services::GroupMucListResponse* resp = dynamic_cast<Origin::Services::GroupMucListResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                mChannelQueryRetryAttempts = 0;

                QVector<Origin::Services::GroupMucListResponse::ChannelInfo> channels = resp->getChannels();                

                for (QVector<Origin::Services::GroupMucListResponse::ChannelInfo>::const_iterator i = channels.cbegin(); i != channels.cend(); ++i)
                {
                    ChatChannel* channel = addChannel((*i).roomName, (*i).passwordProtected, (*i).roomJid);
                    //if ((*i).passwordProtected)
                    //{
                    //    // If this channel is protected by a password, check to see if we have a cached
                    //    // password on disk
                    //    Services::Setting roomPasswordSetting(channel->getChannelId(), Services::Variant::String, "", Services::Setting::LocalPerAccount, Services::Setting::ReadWrite, Services::Setting::Encrypted);
                    //    QString password = Services::readSetting(roomPasswordSetting);

                    //    if (!password.isEmpty())
                    //    {
                    //        channel->unlockChannel();
                    //    }
                    //}

                    channel->getPresence();
                }
                if (channels.size()==0) 
                {
                    mQueriedChannels = false;
                }
            }

			emit groupChannelInformationReceived();
        }

        ChatChannel* ChatGroup::getDefaultChannel()
        {
            ChatChannel* channel = NULL;
            if (mChatChannels.length() > 0) 
            {
                channel = mChatChannels.at(0);
            }
            return channel;
        }

        void ChatGroup::queryGroupMembers()
        {
            Origin::Services::GroupMemberListResponse* resp = Services::GroupServiceClient::listGroupMembers(
                Services::Session::SessionService::currentSession(), mGroupGuid, 0);
            resp->setDeleteOnFinish(true);

            mGroupMembersVector.clear();
            emit groupMembersQueryStart();

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onGetGroupMembers()));
        }

        bool ChatGroup::removeUserFromChannel(const QString& channelId, const QString& userId, const QString& by)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                if (channel && channel->getChannelId().compare(channelId) == 0)
                {
                    channel->removeUser(userId, by);
                    return true;
                }
            }

            return false;
        }

        void ChatGroup::deactivateRooms(MucRoom::DeactivationType type, const QString& reason)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                channel->deactivateRoom(type, reason);
            }
        }

        void ChatGroup::removeMember(quint64 userId)
        {
            removeUserFromMembersList(userId);

            JabberID jid(QString::number(userId), mConnection->host());
            emit groupMemberRemoved(jid);            
        }

        ChatChannel* ChatGroup::channelForId(QString channelId)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                if (channel->id().compare(channelId) == 0)
                {
                    return channel;
                }
            }
            return NULL;
        }

        ChatChannel* ChatGroup::channelForChannelId(QString channelId)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                if (channel->getChannelId().compare(channelId) == 0)
                {
                    return channel;
                }
            }
            return NULL;
        }

        void ChatGroup::editMembersRole(quint64 userId, const QString& role)
        {
            removeUserFromMembersList(userId);
            addUserToMembersList(userId, role);

            emit groupMembersRoleChange(userId, role);
        }

        void ChatGroup::editCurrentUserRole(const quint64 userId, const QString& role)
        {
            mGroupRole = role;
            editMembersRole(userId, role);
        }

        void ChatGroup::onGetGroupMembers()
        {
            using namespace Services;
            GroupMemberListResponse* resp = dynamic_cast<GroupMemberListResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                MemberInfoVector memberInfo = resp->getMembers();    
                mGroupMembersVector << memberInfo;

                if (resp->getPageSize() == memberInfo.size()) 
                {
                    GroupMemberListResponse* next = GroupServiceClient::listGroupMembers(
                        Session::SessionService::currentSession(), mGroupGuid, mGroupMembersVector.size()+1);
                    next->setDeleteOnFinish(true);

                    ORIGIN_VERIFY_CONNECT(next, SIGNAL(finished()), this, SLOT(onGetGroupMembers()));

                } else {
                    mMembers.clear();
                    mAdmins.clear();
                    mOwners.clear();

                    for( MemberInfoVector::iterator i = mGroupMembersVector.begin(); i!= mGroupMembersVector.end(); ++i)
                    {
                        GroupMemberListResponse::MemberInfo info = *i;
                        addUserToMembersList(info.userId, info.userRole);
                    }
                    emit groupMembersLoadFinished();
                }
            }
        }

        bool ChatGroup::isUserInGroup(quint64 id)
        {
            return (mOwners.contains(id) || mAdmins.contains(id) || mMembers.contains(id));            
        }

        QString ChatGroup::getUserRole(qint64 id)
        {            
            if (mOwners.contains(id)) 
            {
                return "superuser";
            } 
            else if (mAdmins.contains(id))
            {
                return "admin";
            } 
            else if (mMembers.contains(id))
            {
                return "member";
            }
            return "";
        }

        void ChatGroup::addUserToMembersList(qint64 userId, const QString& role)
        {
            if (role.compare("superuser")==0)
            {
                mOwners.append(userId);

            } 
            else if (role.compare("admin")==0)
            {
                mAdmins.append(userId);
            } 
            else
            {
                mMembers.append(userId);
            }
        }

        void ChatGroup::removeUserFromMembersList(qint64 userId)
        {
            if (mOwners.contains(userId)) {
                mOwners.removeAll(userId);
            }

            if (mAdmins.contains(userId)) {
                mAdmins.removeAll(userId);
            }

            if (mMembers.contains(userId)) {
                mMembers.removeAll(userId);
            }
        }

        void ChatGroup::onChannelUpdated()
        {
            ChatChannel* channel = dynamic_cast<ChatChannel*>(sender());
            if (!channel)
                return;

            emit channelChanged(channel);
        }

        void ChatGroup::inviteUsersToGroup( QList<Origin::Chat::RemoteUser*> users)
        {
            ConnectedUser* currentUser = mConnection->currentUser();
            ORIGIN_ASSERT(currentUser);
            // Grab the list of blocked users, they should never get invites
            Origin::Chat::BlockList* blockList = currentUser->blockList();
            ORIGIN_ASSERT(blockList);
            QSet<JabberID> blockedListIds = blockList->jabberIDs();
            //QList<Origin::Chat::RemoteUser*> blockedUsers;
            for (int i = 0; i < users.size(); ++i)
            {
                // If this is a blocked user remove it from our original list
                // and continue to the next so this we don't send an invite
                RemoteUser* user = users[i];
                if (blockedListIds.contains(user->jabberId()))
                {
                    //blockedUsers.append(user);
                    users.removeAt(i);
                    continue;
                }
                QString nucleusId = QString::number(user->nucleusId());
                Origin::Services::GroupInviteMemberResponse* resp = Services::GroupServiceClient::inviteGroupMember(
                    Services::Session::SessionService::currentSession(), this->getGroupGuid(), nucleusId);
                resp->setDeleteOnFinish(true);
                GetTelemetryInterface()->Metric_CHATROOM_INVITE_SENT(mGroupGuid.toLatin1(), nucleusId.toLatin1());
            }
            //TODO: Waiting on design/production decision about changing the messaging so the user is informed of the blocked users who will not get invites
            emit(usersInvitedToGroup(users));
        }

        void ChatGroup::handleRoomDestroyed(const QString& channelId)
        {
            onRoomDestroyed(channelId);
        }

        void ChatGroup::acceptGroupInvite()
        {
            Services::GroupJoinResponse* resp = Services::GroupServiceClient::joinGroup(
                Services::Session::SessionService::currentSession(), mGroupGuid);
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onAcceptInviteSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onAcceptInviteFailed()));
        }

        void ChatGroup::onAcceptInviteSuccess()
        {
            Origin::Services::GroupJoinResponse* resp = dynamic_cast<Origin::Services::GroupJoinResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                mGroupInvite = false;
                emit (groupInviteAccepted());

                ConnectedUser* currentUser = mConnection->currentUser();
                ORIGIN_ASSERT(currentUser);
                unsigned int groupCount = (currentUser && currentUser->chatGroups()) ? currentUser->chatGroups()->count() : 0;
                GetTelemetryInterface()->Metric_CHATROOM_INVITE_ACCEPTED(mGroupGuid.toLatin1(), groupCount);
            }
        }

        void ChatGroup::onAcceptInviteFailed()
        {
            emit acceptInviteFailed();
        }

        void ChatGroup::rejectGroupInvite()
        {
            Services::GroupDeclinePendingInviteResponse* resp = Services::GroupServiceClient::declinePendingGroupInvite(
                Services::Session::SessionService::currentSession(), mGroupGuid);
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onRejectInviteSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onRejectInviteFailed()));
        }

        void ChatGroup::onRejectInviteSuccess()
        {
            Origin::Services::GroupDeclinePendingInviteResponse* resp = dynamic_cast<Origin::Services::GroupDeclinePendingInviteResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                emit (groupInviteDeclined());
                GetTelemetryInterface()->Metric_CHATROOM_INVITE_DECLINED(mGroupGuid.toLatin1());
            }
        }

        void ChatGroup::onRejectInviteFailed()
        {
            emit rejectInviteFailed();
        }

        void ChatGroup::onRoomDestroyed(const QString& channelId)
        {
            for (QList<ChatChannel*>::iterator i = mChatChannels.begin(); i != mChatChannels.end(); ++i)
            {
                ChatChannel* channel = *i;
                if (channel && channel->getChannelId().compare(channelId) == 0)
                {
                    ORIGIN_VERIFY_DISCONNECT(channel, SIGNAL(roomDestroyed(const QString&)),
                        this, SLOT(onRoomDestroyed(const QString&)));

                    mChatChannels.erase(i);
                    emit (channelRemoved(channel)); // remove channel from Chat Groups Tab in Friends List
                    delete channel;
                    break;
                }
            }

            // signal that the last channel was removed
            if (mChatChannels.size() == 0)
            {
                ORIGIN_VERIFY_DISCONNECT(mConnection, SIGNAL(roomDestroyed(const QString&)), this, SLOT(onRoomDestroyed(const QString&)));
                emit lastChannelRemoved();
            }
        }

        void ChatGroup::prepareForDeletion()
        {
            ORIGIN_VERIFY_CONNECT(mConnection, SIGNAL(roomDestroyed(const QString&)), this, SLOT(onRoomDestroyed(const QString&)));
        }

        ChatGroups::ChatGroups(Connection* connection, QObject* parent /* = NULL */)
            : QObject(parent)            
            , mConnection(connection)
        {

        }

        ChatGroups::~ChatGroups()
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                delete group;
            }
            mChatGroups.clear();

            for (QSet<ChatGroup*>::iterator j = mInviteGroups.begin(); j != mInviteGroups.end(); ++j)
            {
                ChatGroup* group = *j;
                delete group;
            }
            mInviteGroups.clear();

        }

        void ChatGroups::initializeGroups()
        {
            Origin::Services::GroupListResponse* groupResponse = Services::GroupServiceClient::listGroups(
                Services::Session::SessionService::currentSession());
            groupResponse->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(groupResponse, SIGNAL(success()), this, SLOT(onListGroupsSuccess()));
            ORIGIN_VERIFY_CONNECT(groupResponse, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onListGroupsFailed()));

            Services::GroupPendingInviteResponse* groupPendingResponse = Services::GroupServiceClient::getPendingGroupInvites(
                Services::Session::SessionService::currentSession());

            ORIGIN_VERIFY_CONNECT(groupPendingResponse, SIGNAL(success()), this, SLOT(onListPendingGroupsSuccess()));
        }

        void ChatGroups::onListGroupsSuccess()
        {
            Origin::Services::GroupListResponse* resp = dynamic_cast<Origin::Services::GroupListResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                QVector<Origin::Services::GroupListResponse::GroupInfo> groups = resp->getGroups();

                GetTelemetryInterface()->Metric_CHATROOM_GROUP_COUNT(groups.size());

                QSet<ChatGroup*> seenGroups;
                for (QVector<Origin::Services::GroupListResponse::GroupInfo>::const_iterator i = groups.cbegin(); i != groups.cend(); ++i)
                {
                    ChatGroup* group;
                    if ((group = chatGroupForGroupGuid((*i).groupGuid)) == NULL)
                    {
                        const QString userRole = (*i).userRole;
                        group = new ChatGroup(mConnection, (*i).groupName, (*i).groupGuid, 0, userRole, false);
                        if (userRole == "superuser")
                        {
                            ORIGIN_VERIFY_CONNECT(group, SIGNAL(lastChannelRemoved()), this, SLOT(onLastChannelRemoved()));
                        }
                    }
                    seenGroups << group;
                }

                QSet<ChatGroup*> removedGroups;
                QSet<ChatGroup*> addedGroups;

                removedGroups = mChatGroups - seenGroups;
                addedGroups = seenGroups - mChatGroups;

                mChatGroups = seenGroups;

                foreach(ChatGroup *removed, removedGroups)
                {
                    emit groupRemoved(removed);
                }

                foreach(ChatGroup *added, addedGroups)
                {
                    emit groupAdded(added);
                }

                emit listGroupsSuccess();
            }
        }

        void ChatGroups::onListGroupsFailed()
        {
            emit listGroupsFailed();
        }

        void ChatGroups::onListPendingGroupsSuccess()
        {
            Services::GroupPendingInviteResponse* resp = dynamic_cast<Services::GroupPendingInviteResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                QVector<Origin::Services::GroupPendingInviteResponse::PendingGroupInfo> pendingGroups = resp->getPendingGroups();                

                for (QVector<Origin::Services::GroupPendingInviteResponse::PendingGroupInfo>::const_iterator i = pendingGroups.cbegin();
                    i != pendingGroups.cend(); ++i)
                {
                    addGroupInvite((*i).groupGuid, (*i).groupName, (*i).invitedBy);
                }
                resp->deleteLater();
            }
        }

        void ChatGroups::addGroupInvite(const QString& groupGuid, const QString& groupName, const qint64& invitedBy)
        {
            ORIGIN_ASSERT(!groupGuid.isEmpty());
            ORIGIN_ASSERT(!groupName.isEmpty());

            // Make sure we aren't already invited to this group
            for (QSet<ChatGroup*>::ConstIterator i = mInviteGroups.cbegin(); i != mInviteGroups.cend(); i++)
            {
                ChatGroup* invite = *i;
                if (invite->getGroupGuid() == groupGuid)
                    return; // Don't need to tell the user about this invite they already know
            }

            ChatGroup* group = new ChatGroup(mConnection, groupName, groupGuid, invitedBy, "member", true);
            ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteAccepted()), this, SLOT(onGroupInviteAccepted()));
            ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteDeclined()), this, SLOT(onGroupInviteDeclined()));
            ORIGIN_VERIFY_CONNECT(group, SIGNAL(rejectInviteFailed()), this, SIGNAL(rejectInviteFailed()));
            ORIGIN_VERIFY_CONNECT(group, SIGNAL(acceptInviteFailed()), this, SIGNAL(acceptInviteFailed()));
            mInviteGroups.insert(group);

            emit (groupAdded(group));
            GetTelemetryInterface()->Metric_CHATROOM_INVITE_RECEIVED(groupGuid.toLatin1());
        }

        void ChatGroups::onGroupInviteAccepted()
        {
            ChatGroup* group = dynamic_cast<ChatGroup*>(sender());

            if (group != NULL)
            {
                mInviteGroups.remove(group);
                mChatGroups.insert(group);
                emit (groupChanged(group));
            }            
        }

        void ChatGroups::onGroupInviteDeclined()
        {
            ChatGroup* group = dynamic_cast<ChatGroup*>(sender());

            if (group != NULL && group->isInviteGroup())
            {
                mInviteGroups.remove(group);
                emit (groupRemoved(group));
            }  
        }

        void ChatGroups::createGroup(const QString& groupGuid, const QString& groupName)
        {
            ORIGIN_ASSERT(!groupGuid.isEmpty());
            ORIGIN_ASSERT(!groupName.isEmpty());

            ChatGroup* group = new ChatGroup(mConnection, groupName, groupGuid, 0, "superuser", false);
            ORIGIN_VERIFY_CONNECT(group, SIGNAL(lastChannelRemoved()), this, SLOT(onLastChannelRemoved()));
            mChatGroups.insert(group);

            emit (groupAdded(group));
            GetTelemetryInterface()->Metric_CHATROOM_CREATE_GROUP(groupGuid.toLocal8Bit(), mChatGroups.size());

            ChatChannel* channel = group->addChannel(tr("ebisu_client_lobby"), false);
            channel->createDefaultChannel();
        }

        void ChatGroups::joinGroup(const QString& groupGuid, const QString& groupName)
        {
            ChatGroup* group = new ChatGroup(mConnection, groupName, groupGuid, 0, "member", false);
            mChatGroups.insert(group);

            emit (groupAdded(group));
        }

        void ChatGroups::deleteGroup(const QString& groupGuid)
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    mChatGroups.erase(i);
                    delete group;
                    break;
                }                
            }
        }

        void ChatGroups::removeGroupUI(const QString& groupGuid)
        {
            // Check our existing groups
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    mChatGroups.erase(i);
                    emit (groupRemoved(group)); // remove group from Chat Groups UI
                    delete group;
                    return;
                }
            }
            // Check our pending groups
            for (QSet<ChatGroup*>::iterator i = mInviteGroups.begin(); i != mInviteGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    emit (groupRemoved(group)); // remove group from Chat Groups UI
                    return;
                }
            }
        }

        void ChatGroups::leaveGroup(const QString& groupGuid)
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->deactivateRooms(MucRoom::GroupLeftType, "");
                    mChatGroups.erase(i);
                    GetTelemetryInterface()->Metric_CHATROOM_LEAVE_GROUP(group->getGroupGuid().toLatin1(), count());
                    emit (groupRemoved(group));
                    delete group;
                    break;
                }                
            }
        }

        void ChatGroups::prepareForGroupDeletion(const QString& groupGuid)
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->prepareForDeletion();
                    break;
                }                
            }
        }

        void ChatGroups::editGroupName(const QString& groupGuid, const QString& groupName)
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->setGroupName(groupName);
                    emit (groupChanged(group));
                    break;
                }                
            }
        }

        void ChatGroups::editGroupMemberRole(const QString& groupGuid, quint64 userId, const QString& role)
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0 && group->isUserInGroup(userId))
                {
                    group->editMembersRole(userId, role);
                    break;
                }                
            }
        }

        void ChatGroups::editCurrentUserGroupRole(const QString& groupGuid, quint64 userId, const QString& role)
        {
            for (QSet<ChatGroup*>::iterator i = mChatGroups.begin(); i != mChatGroups.end(); ++i)
            {
                ChatGroup* group = *i;
                if (group && group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->editCurrentUserRole(userId, role);
                    break;
                }                
            }
        }

        ChatChannel* ChatGroups::addChannelToGroup(const QString& groupGuid, const QString& channelName, bool locked, const QString& channelJid /* = "" */)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    ChatChannel* channel = group->addChannel(channelName, locked, channelJid);
                    return channel;
                }                
            }
            
            return NULL;
        }

        void ChatGroups::removeChannelFromGroup(const QString& groupGuid, const QString& channelId, const QString& nickName, const QString& nucleusId, const QString& password)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->destroyChannel(channelId, nickName, nucleusId, password);
                    break;
                }                
            }
        }

        void ChatGroups::inviteUserToGroup(const QString& groupGuid, Origin::Chat::RemoteUser* user)
        {
            Origin::Services::GroupInviteMemberResponse* resp = Services::GroupServiceClient::inviteGroupMember(
                    Services::Session::SessionService::currentSession(), groupGuid, QString::number(user->nucleusId()));
            resp->setDeleteOnFinish(true);
            emit(userInvitedToGroup(groupGuid));
        }

        void ChatGroups::addUsersToGroup(const QString& groupGuid, QList<RemoteUser*> users)
        {
            for (QList<RemoteUser*>::const_iterator i = users.cbegin(); i != users.cend(); ++i)
            {
                RemoteUser* user = *i;
             
                Origin::Services::GroupInviteMemberResponse* resp = Services::GroupServiceClient::inviteGroupMember(
                    Services::Session::SessionService::currentSession(), groupGuid, QString::number(user->nucleusId()));
                resp->setDeleteOnFinish(true);
            }
        }

        void ChatGroups::enterChannelInGroup(const QString& groupGuid, const QString& channelId, const QString& password)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->joinChannel(channelId, password);
                    break;
                }                
            }
        }

        void ChatGroups::removeUserFromChannel(const QString& groupGuid, const QString& channelId, const QString& userId, const QString& by)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->removeUserFromChannel(channelId, userId, by);
                    break;
                }                
            }
        }

        void ChatGroups::handleRoomDestroyed(const QString& groupGuid, const QString& channelId)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->handleRoomDestroyed(channelId);
                    break;
                }                
            }
        }

        void ChatGroups::removeMemberFromGroup(const QString& groupGuid, quint64 userId)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    group->removeMember(userId);
                    break;
                }                
            }

        }

        void ChatGroups::onLastChannelRemoved()
        {
            ChatGroup* group = dynamic_cast<Origin::Chat::ChatGroup*>(sender());
            ORIGIN_ASSERT(group);
            if (group)
            {
                ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(lastChannelRemoved()), this, SLOT(onLastChannelRemoved()));
                deleteGroup(group->getGroupGuid()); // remove group from mChatGroups
            }
        }

        ChatGroup* ChatGroups::chatGroupForGroupGuid(const QString& groupGuid)
        {
            for (QSet<ChatGroup*>::const_iterator i = mChatGroups.cbegin(); i != mChatGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    return group;
                }                
            }

            // Check invite groups as well
            for (QSet<ChatGroup*>::const_iterator i = mInviteGroups.cbegin(); i != mInviteGroups.cend(); ++i)
            {
                ChatGroup* group = *i;
                if (group->getGroupGuid().compare(groupGuid) == 0)
                {
                    return group;
                }                
            }

            return NULL;
        }
    }
}
