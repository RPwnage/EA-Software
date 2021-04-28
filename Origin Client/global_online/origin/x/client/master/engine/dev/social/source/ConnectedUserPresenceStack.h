#ifndef _CONNECTEDUSERPRESENCESTACK_H
#define _CONNECTEDUSERPRESENCESTACK_H

#include <QVector>
#include <QString>
#include <QMutex>
#include <QObject>

#include "chat/Presence.h"

namespace Origin
{

namespace Chat
{
    class ConnectedUser;
}

namespace Engine
{
namespace Social
{
    ///
    /// \brief Manages a priority stack of presences for the connected user
    ///
    /// The effective presence of the user is set to the one with the highest priority.
    ///
    /// This is a low level class intended for users such as InGameStatusWatcher. For setting user-visible availability
    /// see UserAvailabilityController
    ///
    class ConnectedUserPresenceStack : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// \brief Creates a new ConnectedUserPresenceStack
        ///
        /// By default it will only contain the DefaultOnlinePriority entry and will not call applyToConnectedUser()
        ///
        ConnectedUserPresenceStack(Chat::ConnectedUser *connectedUser);

        /// \brief Represents the priority a presence stack entry can have
        enum PresencePriority
        {
            /// \brief  Presence set from a game via the Origin SDK
            SdkGamePriority = 4,
            /// \brief  Automatically set in-game or joinable presence
            InGamePriority = 3,
            /// \brief  User has set themselves explicitly away
            ExplicitAwayPriority = 2,
            /// \brief  Automatically detected away
            AutoAwayPriority = 1,
            /// \brief  Fallback online presence
            DefaultOnlinePriority = 0,
            
            MaximumPriority = SdkGamePriority
        };

        ///
        /// \brief Sets the presence entry for the given priority
        ///
        /// If an exisiting presence entry exists at that priority it will be removed.
        /// This implicitly calls applyToConnectedUser() if required.
        ///
        /// This will emit changed() if it changes the state of the stack
        ///
        void setPresenceForPriority(PresencePriority priority, const Chat::Presence &entry);

        ///
        /// \brief Returns the presence entry for the given priority
        ///
        /// If nothing is set for the priority a null Presence will be returned
        ///
        Chat::Presence presenceForPriority(PresencePriority priority) const;

        ///
        /// \brief Removes the presence entry for the given priority if it exists
        ///
        /// This implicitly calls applyToConnectedUser() if required
        ///
        /// This will emit changed() if it changes the state of the stack
        ///
        void clearPresenceForPriority(PresencePriority);
        
        ///
        /// \brief Applies the presence entry at the top of the stack to the connected user
        ///
        /// This is usually called implicitly when required. Two exceptions are after initial construction and
        /// when the connected user's presence has changed by other means
        ///
        void applyToConnectedUser();

    signals:
        ///
        /// \brief Emitted whenever the state of the stack changes
        ///
        /// \sa setPresenceForPriority()
        /// \sa clearPresenceForPriority()
        ///
        void changed();

    private:
        Chat::Presence highestPriorityPresence() const;

        Chat::ConnectedUser *mConnectedUser;

        mutable QMutex mStateLock;
        QVector<Chat::Presence> mPresenceStack;
    };
}
}
}

#endif

