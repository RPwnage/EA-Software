#include <engine/social/UserAvailabilityController.h>
#include <services/debug/DebugService.h>

#include "ConnectedUserPresenceStack.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    UserAvailabilityController::UserAvailabilityController(ConnectedUserPresenceStack *presenceStack) :
        mPresenceStack(presenceStack)
    {
        // The allowed transitions might not change every time the stack mutates but it's probably easier to make
        // the listeners to allowedTransitionsChanged() burn a few cycles than try to filter it out on our end
        ORIGIN_VERIFY_CONNECT(mPresenceStack, SIGNAL(changed()), this, SIGNAL(allowedTransitionsChanged()));
    }

    QSet<Chat::Presence::Availability> UserAvailabilityController::allowedTransitions() const
    {
        QSet<Chat::Presence::Availability> allowed;

        // inGamePresence is our in-game presence if any
        const bool inGame = !mPresenceStack->presenceForPriority(ConnectedUserPresenceStack::SdkGamePriority).isNull() ||
                            !mPresenceStack->presenceForPriority(ConnectedUserPresenceStack::InGamePriority).isNull();
        
        // We can always go online
        allowed << Chat::Presence::Online;

        if (!inGame)
        {
            // If we're not in-game we can also go away
            allowed << Chat::Presence::Away;
        }

        return allowed;
    }
   
    void UserAvailabilityController::transitionTo(Chat::Presence::Availability presence)
    {
        if (!allowedTransitions().contains(presence))
        {
            // Not an allowed transition
            return;
        }

        // Handle explicit away
        if (presence == Chat::Presence::Away)
        {
            // We're explicitly away now
            const Chat::Presence awayPresence(Chat::Presence::Away);
            mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::ExplicitAwayPriority, awayPresence);
        }
        else
        {
            // We're no longer explicitly away, clear our away entry
            mPresenceStack->clearPresenceForPriority(ConnectedUserPresenceStack::ExplicitAwayPriority);
        }
    }
}
}
}
