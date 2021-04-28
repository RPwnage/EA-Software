#include "MucRoom.h"

#include <QUuid>
#include <QMutexLocker>

#include "JingleClient.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "Connection.h"
#include "ConnectedUser.h"
#include "OutgoingMucRoomInvite.h"
#include "RemoteUser.h"
#include "XMPPImplEventAdapter.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Chat
{
    MucRoom::MucRoom(Connection *connection, const Chat::JabberID &roomJabberId, QObject *parent) :
        Conversable(connection, "groupchat", parent),
        mJabberId(roomJabberId),
        mDeactivated(false),
        mPeakParticipants(0)
    {
        ORIGIN_VERIFY_CONNECT(connection, SIGNAL(connected()), this, SLOT(reconnectRoom()));
        mJoinedTime.start();
    }
        
    MucRoom::~MucRoom()
    {
        // Dump some statistics
        GetTelemetryInterface()->Metric_GC_MUC_EXIT(participationMilliseconds() / 1000, messagesSent(), peakParticipants());

        leaveRoom();
    }

    void MucRoom::leaveRoom()
    {
        if( !isDeactivated() )
        {
            if (connection() && connection()->jingle())
            {
                connection()->jingle()->MUCLeaveRoom(mJabberId.toJingle());
            }
        }

        mInvitedBy = QString("");

        emit exited(); // removes this mucRoom from 'Connection::mMucRooms'
    }

    void MucRoom::setDeactivated(DeactivationType type, const QString &reason)
    {
        if( !mDeactivated )
        {
            mDeactivated = true;
            emit deactivated(type, reason);
        }
    }

    JabberID MucRoom::jabberId() const
    {
        return mJabberId;
    }

    QString MucRoom::id() const
    {
        return mJabberId.node();
    }

    QSet<ConversableParticipant> MucRoom::participants() const
    {
        QMutexLocker locker(&mStateLock);
        return mParticipants;
    }

    void MucRoom::setNickname(const QString& nickname)
    {
        mNickname = nickname;
    }
        
    QString MucRoom::nickname() const
    {
        return mNickname;
    }

    void MucRoom::setRole(const QString& role)
    {
        mRole = role;
    }

    QString MucRoom::role() const
    {
        return mRole;
    }
        
    QString MucRoom::generateUniqueMucRoomId()
    {
        QString randomString = QUuid::createUuid().toString();

        // Remove some of punctuation so it looks nicer
        return randomString.replace('{', "").replace('}', "").replace('-', "");
    }

    void MucRoom::addParticipant(XmppChatroomMemberProxy const& member)
    {
        const JabberID bareJid = JabberID::fromJingle(member.full_jid).asBare();
        RemoteUser* user = connection()->remoteUserForJabberId(bareJid);

        if (!user)
        {
            return;
        }

        ConversableParticipant participant(QString::fromUtf8(member.name.c_str()), user);
        participant.setRoomPresence(member.presence);

        {
            QMutexLocker locker(&mStateLock);

            mParticipants.insert(participant);
            mPeakParticipants = qMax(mPeakParticipants, mParticipants.size());

            if (member.role.compare("moderator") == 0)
            {
                mModerators.insert(participant);
            }
        }

        emit participantAdded(participant);
    }

    void MucRoom::removeParticipant(XmppChatroomMemberProxy const& member)
    {
        const JabberID bareJid = JabberID::fromJingle(member.full_jid).asBare();
        RemoteUser* user = connection()->remoteUserForJabberId(bareJid);

        if (!user)
        {
            return;
        }

        ConversableParticipant participant(QString::fromUtf8(member.name.c_str()), user);

        {
            QMutexLocker locker(&mStateLock);

            mParticipants.remove(participant);

            if (member.role.compare("moderator") == 0)
            {
                mModerators.remove(participant);
            }
        }

        emit participantRemoved(participant);
    }

    void MucRoom::changeParticipant(XmppChatroomMemberProxy const& member)
    {
        const JabberID bareJid = JabberID::fromJingle(member.full_jid).asBare();
        RemoteUser* user = connection()->remoteUserForJabberId(bareJid);

        if (!user)
        {
            return;
        }

        ConversableParticipant participant(QString::fromUtf8(member.name.c_str()), user);
        participant.setRoomPresence(member.presence);

        {
            QMutexLocker locker(&mStateLock);

            if (mModerators.contains(participant) && member.role.compare("moderator") != 0)
            {
                mModerators.remove(participant);
            }
            else if (member.role.compare("moderator") == 0)
            {
                mModerators.insert(participant);
            }

            // Check if this is us being demoted/promoted
            QString memberName = QString::fromUtf8(member.name.c_str());
            if (mNickname.compare(memberName) == 0)
            {
                mRole = QString::fromUtf8(member.role.c_str());
            }
        }

        emit participantChanged(participant);
    }

    bool MucRoom::isParticipant(RemoteUser* user) const
    {
        ConversableParticipant participant(user->originId(), user);
        return mParticipants.contains(participant);
    }

    bool MucRoom::isModerator(RemoteUser* user) const
    {
        ConversableParticipant participant(user->originId(), user);
        return mModerators.contains(participant);
    }

    void MucRoom::clearRoom()
    {
        mDeactivated = false;
        mPeakParticipants = 0;
        mParticipants.clear();
        mModerators.clear();
    }

    void MucRoom::invite(const OutgoingMucRoomInvite &invite)
    {
        buzz::Jid invitee = invite.invitee().toJingle();
        buzz::Jid inviter = connection()->currentUser()->jabberId().toJingle();

        if (connection() && connection()->jingle())
        {
            connection()->jingle()->MUCInvite(jabberId().toJingle(), invitee, invite.reason(), invite.threadId());  
        }

        emit inviteSent(invite);
    }

    void MucRoom::setJabberId(const Origin::Chat::JabberID &jid)
    {
        mJabberId = jid;
    }
        
    void MucRoom::receivedSystemMessage(const Message &message)
    {
        emit systemMessageReceived(message);
    }
        
    JabberID MucRoom::jabberIdForNickname(const QString &nickname) const
    {
        QMutexLocker locker(&mStateLock);

        if (nickname == mNickname)
        {
            // This is us
            return connection()->currentUser()->jabberId();
        }

        for(QSet<ConversableParticipant>::ConstIterator it = mParticipants.constBegin();
            it != mParticipants.constEnd();
            it++)
        {
            if (it->nickname() == nickname)
            {
                return it->remoteUser()->jabberId();
            }
        }

        return JabberID();
    }
        
    void MucRoom::reconnectRoom()
    {
        if (connection() && connection()->jingle())
        {
            // For now we just want to reconnect to a default room config with no password
            // TODO: GROIP - Need to figure out how to reconnect
            //connection()->jingle()->MUCEnterRoom(mNickname, mJabberId.toJingle(), "", true);
        }
    }

    int MucRoom::peakParticipants() const
    {
        QMutexLocker locker(&mStateLock);
        return mPeakParticipants;
    }

    qint64 MucRoom::participationMilliseconds() const
    {
        QMutexLocker locker(&mStateLock);
        return mJoinedTime.elapsed();
    }
}
}
