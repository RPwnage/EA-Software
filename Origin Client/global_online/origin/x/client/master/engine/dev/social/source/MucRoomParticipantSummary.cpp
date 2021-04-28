#include "engine/social/MucRoomParticipantSummary.h"
#include "engine/social/UserNamesCache.h"

#include "chat/Conversable.h"
#include "chat/RemoteUser.h"

namespace
{
    const unsigned int MaxEncodedNicknames = 10;
}

namespace Origin
{
namespace Engine
{
namespace Social
{
        
    MucRoomParticipantSummary::MucRoomParticipantSummary() :
        mNull(true),
        mTotalParticipants(0)
    {
    }

    MucRoomParticipantSummary::MucRoomParticipantSummary(const QStringList &nicknameSubset, unsigned int totalParticipants) :
        mNull(false),
        mNicknameSubset(nicknameSubset),
        mTotalParticipants(totalParticipants)
    {
    }

    MucRoomParticipantSummary MucRoomParticipantSummary::fromParticipants(const QSet<Chat::ConversableParticipant> &participants, const UserNamesCache *userNames)
    {
        QStringList allNicknames;

        for(QSet<Chat::ConversableParticipant>::ConstIterator it = participants.constBegin();
            it != participants.constEnd();
            it++)
        {
            const QString originId = userNames->namesForNucleusId(it->remoteUser()->nucleusId()).originId();

            if (!originId.isEmpty())
            {
                allNicknames << originId;
            }
        }

        // Only use the first MaxEncodedNicknames nicknames in alphabetical order
        // This is case sensitive. Lets see if anyone notices
        allNicknames.sort();
        return MucRoomParticipantSummary(allNicknames.mid(0, MaxEncodedNicknames), participants.count());
    }

    MucRoomParticipantSummary MucRoomParticipantSummary::fromEncoded(const QString &encoded)
    {
        const QRegExp encodedReasonRegExp("(\\d+):([\\S;]+)");

        if (encodedReasonRegExp.exactMatch(encoded))
        {
            const int totalParticipants = encodedReasonRegExp.cap(1).toInt();
            QStringList nicknameSubset = encodedReasonRegExp.cap(2).split(";");
            
            return MucRoomParticipantSummary(nicknameSubset, totalParticipants);
        }

        return MucRoomParticipantSummary();
    }
        
    QString MucRoomParticipantSummary::toEncoded() const
    {
        QString ret;

        ret  = QString::number(mTotalParticipants) + ":";
        ret += mNicknameSubset.join(";");

        return ret;
    }
}
}
}
