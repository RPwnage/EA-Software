#ifndef _USERAVAILABILITYCONTROLLER_H
#define _USERAVAILABILITYCONTROLLER_H

#include <QSet>
#include <QObject>

#include <chat/Presence.h>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    class ConnectedUserPresenceStack;

    ///
    /// \brief  Handles user-level availability transitions
    ///
    /// Not all availability transitions are allowed (eg going away while in-game) and some additional logic is needed
    /// for some transitions (re-enabling auto-away when going from manual Away => Online). This class handles
    /// enumerating allowed transitions and the logic for performing those transitions 
    ///
    class ORIGIN_PLUGIN_API UserAvailabilityController : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// \brief Creates a UserLevelPresenceController for the given presence stack
        ///
        UserAvailabilityController(ConnectedUserPresenceStack *presenceStack);

        ///
        /// \brief Returns the presences that can be transitioned to
        ///
        /// This includes the current presence. OriginOffline should be treated as invisible
        ///
        QSet<Chat::Presence::Availability> allowedTransitions() const;
       
        ///
        /// \brief Transitions to the passed presence
        ///
        void transitionTo(Chat::Presence::Availability);

    signals:
        ///
        /// \brief Emitted whenever the set of allowed transitions changes
        ///
        void allowedTransitionsChanged();
    
    private:
        ConnectedUserPresenceStack *mPresenceStack;
    };
}
}
}

#endif

