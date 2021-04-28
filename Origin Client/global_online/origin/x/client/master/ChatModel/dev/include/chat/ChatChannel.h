#ifndef _CHATMODEL_CHATCHANNEL_H
#define _CHATMODEL_CHATCHANNEL_H

#include <QObject>
#include <QVector>
#include <QStringList>

#include "MucRoom.h"

namespace Origin
{
    namespace Chat
    {
        class ChatGroup;
        class ChatChannel : public MucRoom
        {
            friend class MucRoom;

            Q_OBJECT 
        public:
            ChatChannel(Connection *connection, ChatGroup* chatGroup, const QString& jid, const QString& name, bool isLocked, QObject* parent = NULL);
            virtual ~ChatChannel() {};

            ///
            /// Joins a channel
            /// \param password The password for entering this channel
            ///
            void joinChannel(const QString& password = "", bool failSilently = false);

            ///
            /// Rejoins a channel
            ///
            void rejoinChannel();

            ///
            /// Creates a channel
            /// \param password The password for this channel to be created with
            ///
            void createChannel(const QString& password = "");

            ///
            /// Creates the default channel silently
            ///
            void createDefaultChannel();

            ///
            /// Destroys the channel
            /// \param nickName The nick name of the user destroying the channel
            /// \param nucleusId The nucleus id of the user destroying the channel
            /// \param password The password for entering this channel
            ///
            void destroyChannel(const QString& nickName, const QString& nucleusId, const QString& password = "");

            ///
            /// Gets the presence for users in the channel
            ///
            void getPresence();

            ///
            /// Removes a user from the channel
            /// \param userNickname     The user to remove from the chat channel
            /// \param by               The Origin Id of the user doing the removal
            ///
            void removeUser(const QString& userNickname, const QString& by);

            ///
            ///
            ///
            void inviteUser(const Chat::JabberID& toJid, const QString& roomId, const QString& channelId);

            ///
            /// Gets the chat group for this channel
            ///
            ChatGroup* getChatGroup() const
            {
                return mChatGroup;
            }

            ///
            /// Gets the name of the channel
            ///
            const QString& getChannelName() const
            {
                return mChannelName;
            }

            ///
            /// Gets the id for the channel
            ///
            const QString& getChannelId() const
            {
                return mChannelId;
            }

            QString getGroupGuid() const
            {
                // ChannelID contains our group GUID, parse that
                // Form is gac_<guid>_<identifier>
                QStringList str = mChannelId.split('_');
                return str[1];
            }

            ///
            /// Gets the status for the channel
            ///
            const QString& getStatus() const
            {
                return mStatus;
            }

            ///
            /// Gets if the channel is locked
            ///
            bool isLocked() const
            {
                return mIsLocked;
            }

            ///
            /// Unlocks the channel
            ///
            void unlockChannel()
            {
                mIsLocked = false;
                emit (channelUpdated());
            }

            ///
            /// Returns the jids of all users in the channel
            ///
            QVector<JabberID> getChannelUsers() const
            {
                return mChannelUsers;
            }

            ///
            /// Removes user's JabberID from list and updates channel
            ///
            void leaveRoom();

            ///
            /// Adds new member's JabberID to list and updates channel
            ///
            void addParticipant(XmppChatroomMemberProxy const& member);

            ///
            /// Removes member's JabberID from list and updates channel
            ///
            void removeParticipant(XmppChatroomMemberProxy const& member);

            void deactivateRoom(MucRoom::DeactivationType type, const QString& reason);

            QString getParticipantRole(const JabberID& userJabberId) const;

            bool isUserInChannel() const {return mInChannel;};
        signals:
            void channelUpdated();

            // emitted after the room has been destroyed on the server
            void roomDestroyed(const QString& channelId);

            void leftRoom();

        private slots:
            void onGetPresence();
            void onDisconnect();

        private:
        
            /// \brief Returns true if the channel has a password
            bool isPasswordProtected() const { return mIsPasswordProtected; }
            
            /// \brief Wrapper function to clear participants before joining a MucRoom
            void joinMucRoom(const QString& roomId, const QString& password = "", const QString& roomName = "", const bool leaveRoomOnJoin = false, const bool failSilently = false);

            ChatGroup* mChatGroup;
            QString mChannelName;
            QString mChannelId;
            QString mStatus;
            bool mInChannel;

            QVector<JabberID> mChannelUsers;
            QVector<JabberID> mChannelModerators;

            bool mIsLocked;
            bool mIsPasswordProtected;

            JabberID mChannelJid;
        };
    }
}

#endif
