#include "ChatChannel.h"
#include "ChatGroup.h"
#include "Connection.h"
#include "services/rest/GroupServiceClient.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "ConnectedUser.h"

namespace Origin
{
    namespace Chat
    {
        ChatChannel::ChatChannel(Connection *connection, ChatGroup* chatGroup, const QString& jid,
            const QString& name, bool isPasswordProtected, QObject* parent /* = NULL */)
            : MucRoom(connection, JabberID(), parent)
            , mChatGroup(chatGroup)
            , mChannelName(name)
            , mInChannel(false)
            , mIsLocked(isPasswordProtected)
            , mIsPasswordProtected(isPasswordProtected)
        {
            if (jid.isEmpty())
            {
                // New room, need to assign the channelId and create a JID
                QString randomString = QUuid::createUuid().toString();
                randomString.replace('{', "").replace('}', "").replace('-', "");

                mChannelId = "gac_" + chatGroup->getGroupGuid() + "_" + randomString.toLower();

                QString mucHost = "muc.";
                mucHost.append(connection->host());
                mChannelJid = JabberID(mChannelId, mucHost);
            }
            else
            {
                mChannelJid = JabberID::fromEncoded(jid.toLatin1());
                mChannelId = mChannelJid.node();
            }

            ORIGIN_VERIFY_CONNECT(connection, SIGNAL(roomDestroyed(const QString&)),
                this, SIGNAL(roomDestroyed(const QString&)));
            ORIGIN_VERIFY_CONNECT(connection, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)),
                this, SLOT(onDisconnect()));
        }

        void ChatChannel::joinChannel(const QString& password, bool failSilently)
        {
            if (isPasswordProtected() && password.isEmpty())
            {
                // It's possible we have a cached password
                Services::Setting roomPasswordSetting(mChannelId, Services::Variant::String, "", Services::Setting::LocalPerAccount, Services::Setting::ReadWrite, Services::Setting::Encrypted);
                QString storedPassword = Services::readSetting(roomPasswordSetting);
                joinMucRoom(mChannelId, storedPassword, "", false, failSilently);
            }
            else
            {
                joinMucRoom(mChannelId, password, "", false, failSilently);
            }
        }

        void ChatChannel::rejoinChannel()
        {            
            clearRoom();
            mChannelUsers.clear();

            connection()->rejoinMucRoom(this, mChannelId);

            // Lets the proxy know we are in this room
            mInChannel = true;

            // Need to add current user to our list.
            JabberID bareUserJid = connection()->currentUser()->jabberId().asBare();
            if (!mChannelUsers.contains(bareUserJid))
            {
                mChannelUsers.append(bareUserJid);
                mStatus = QString(tr("ebisu_client_in_chat")).arg(mChannelUsers.count());
            }
            emit (channelUpdated());            
        }


        void ChatChannel::createChannel(const QString& password)
        {
            mIsPasswordProtected = (!password.isEmpty());
            if(connection()->currentUser()->visibility() != Chat::ConnectedUser::Invisible)
                joinMucRoom(mChannelId, password, mChannelName);
        }

        void ChatChannel::createDefaultChannel()
        {
            joinMucRoom(mChannelId, "", mChannelName, true);
        }

        void ChatChannel::destroyChannel(const QString& nickName, const QString& nucleusId, const QString& password)
        {
            QString password_ = password;
            if (password_.isEmpty())
            {
                Services::Setting roomPasswordSetting(getChannelId(), Services::Variant::String, "", Services::Setting::LocalPerAccount, Services::Setting::ReadWrite, Services::Setting::Encrypted);
                password_ = Services::readSetting(roomPasswordSetting).toString();
            }

            connection()->destroyMucRoom(this, mChannelId, nickName, nucleusId, password_);
        }

        void ChatChannel::getPresence()
        {
            Origin::Services::GroupRoomPresenceResponse* resp = Services::GroupServiceClient::listGroupRoomPresence(
                Services::Session::SessionService::currentSession(), mChannelJid.toEncoded());
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onGetPresence()));
        }

        void ChatChannel::removeUser(const QString& userNickname, const QString& by)
        {
            connection()->kickMucUser(mChannelId, userNickname, by);
        }

        void ChatChannel::inviteUser(const Chat::JabberID& toJid, const QString& roomId, const QString& channelId)
        {
            connection()->chatGroupRoomInvite(toJid, roomId, channelId);
        }

        void ChatChannel::deactivateRoom(MucRoom::DeactivationType type, const QString& reason)
        {
            this->setDeactivated(type, reason);
        }

        void ChatChannel::onGetPresence()
        {
            Origin::Services::GroupRoomPresenceResponse* resp = dynamic_cast<Origin::Services::GroupRoomPresenceResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                QVector<Origin::Services::GroupRoomPresenceResponse::UserPresenceInfo> userPresence = 
                    resp->getUserPresence();       

                mChannelUsers.clear();

                for (QVector<Origin::Services::GroupRoomPresenceResponse::UserPresenceInfo>::const_iterator i = userPresence.cbegin(); 
                    i != userPresence.cend(); ++i)
                {
                    QByteArray arr = (*i).jid.toLatin1();
                    JabberID jid = JabberID::fromEncoded(arr);
                    // Add these as bare so they match what we get from Connection
                    JabberID bareJid = jid.asBare();
                    mChannelUsers.append(bareJid);
                }
                mStatus = QString(tr("ebisu_client_in_chat")).arg(resp->getUserPresence().count());
            }

            emit (channelUpdated());
        }

        void ChatChannel::onDisconnect()
        {
            mInChannel = false;
        }

        QString ChatChannel::getParticipantRole(const JabberID& userJabberId) const
        {
            RemoteUser* user = connection()->remoteUserForJabberId(userJabberId);
            if (!user)
            {
                return "";
            }

            if (this->isModerator(user))
            {
                return "moderator";
            } else if (this->isParticipant(user))
            {
                return "participant";
            }
            
            return "";


        }

        void ChatChannel::addParticipant(XmppChatroomMemberProxy const& member)
        {
            MucRoom::addParticipant(member);
            const JabberID bareJid = JabberID::fromJingle(member.full_jid).asBare();
            RemoteUser* user = connection()->remoteUserForJabberId(bareJid);
            if (!user)
            {
                return;
            }

            if (!mChannelUsers.contains(bareJid))
            {
                mChannelUsers.append(bareJid);
                mStatus = QString(tr("ebisu_client_in_chat")).arg(mChannelUsers.count());

                emit (channelUpdated());
            }
        }

        void ChatChannel::removeParticipant(XmppChatroomMemberProxy const& member)
        {
            MucRoom::removeParticipant(member);
            const JabberID bareJid = JabberID::fromJingle(member.full_jid).asBare();
            RemoteUser* user = connection()->remoteUserForJabberId(bareJid);
            if (!user)
            {
                return;
            }

            if (mChannelUsers.contains(bareJid))
            {
                int index = mChannelUsers.indexOf(bareJid);
                mChannelUsers.remove(index);
            }
            mStatus = QString(tr("ebisu_client_in_chat")).arg(mChannelUsers.count());

            emit (channelUpdated());
        }

        void ChatChannel::leaveRoom()
        {
            JabberID bareJid = connection()->currentUser()->jabberId().asBare();
            if (mChannelUsers.contains(bareJid))
            {
                int index = mChannelUsers.indexOf(bareJid);
                mChannelUsers.remove(index);
            }
            mStatus = QString(tr("ebisu_client_in_chat")).arg(mChannelUsers.count());

            MucRoom::leaveRoom();

            // Lets the proxy know we are not in this room anymore
            mInChannel = false;
            emit (channelUpdated());

            emit (leftRoom());
        }

        void ChatChannel::joinMucRoom(const QString& roomId, const QString& password, const QString& roomName, const bool leaveRoomOnJoin, const bool failSilently)
        {
            clearRoom();
            mChannelUsers.clear();
            connection()->joinMucRoom(this, roomId, password, roomName, leaveRoomOnJoin, failSilently);

            if (!leaveRoomOnJoin)
            {
                // Lets the proxy know we are in this room
                mInChannel = true;

                // Need to add current user to our list.
                JabberID bareUserJid = connection()->currentUser()->jabberId().asBare();
                if (!mChannelUsers.contains(bareUserJid))
                {
                    mChannelUsers.append(bareUserJid);
                    mStatus = QString(tr("ebisu_client_in_chat")).arg(mChannelUsers.count());
                }
                emit (channelUpdated());
            }
        }
    }
}