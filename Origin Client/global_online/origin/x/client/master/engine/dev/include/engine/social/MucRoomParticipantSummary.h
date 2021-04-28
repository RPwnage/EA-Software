#ifndef _MUCROOMPARTICIPANTINFO_H
#define _MUCROOMPARTICIPANTINFO_H

#include <QString>
#include <QStringList>
#include <QSet>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Chat
{
    class ConversableParticipant;
}

namespace Engine
{
namespace Social
{
    class UserNamesCache;

    ///
    /// \brief Represents a summary of the participants in a multi-user chat room 
    ///
    /// This consists of a subset of the room's nicknames and the total participant count. This can be encoded in to
    /// a string that can be passed along with multi-user chat room invites
    ///
    class ORIGIN_PLUGIN_API MucRoomParticipantSummary
    {
    public:
        ///
        /// \brief Creates a MucRoomParticipantSummary from a list of room participants
        ///
        /// A subset of the passed participants will be used to create the nickname subset
        ///
        /// \param  participants  List of participants
        /// \param  userNames     UserNamesCache instance to use to look up Origin IDs
        ///
        static MucRoomParticipantSummary fromParticipants(const QSet<Chat::ConversableParticipant> &participants, const UserNamesCache *userNames);

        ///
        /// \brief Creates a MucRoomParticipantSummary from an encoded string
        ///
        /// \param encoded  Encoded string produced by MucRoomParticipantSummary::toEncoded()
        ///
        /// \return MucRoomParticipantSummary instance. The instance will be null if the string isn't in the expected
        ///         format 
        ///
        static MucRoomParticipantSummary fromEncoded(const QString &encoded);

        /// \brief Returns an encoded string representing this MucRoomParticipantSummary
        QString toEncoded() const;

        /// \brief Returns if this represents a null instance
        bool isNull() const
        {
            return mNull;
        }

        /// \brief Returns an arbitrary subset of the nicknames in the room
        QStringList nicknameSubset() const
        {
            return mNicknameSubset;
        }

        /// \brief Returns the total number of participants in the room
        unsigned int totalParticipants() const
        {
            return mTotalParticipants;
        }

    private:
        MucRoomParticipantSummary();
        MucRoomParticipantSummary(const QStringList &nicknameSubset, unsigned int totalParticipants); 

        bool mNull;
        QStringList mNicknameSubset;
        unsigned int mTotalParticipants;
    };
}
}
}

#endif

