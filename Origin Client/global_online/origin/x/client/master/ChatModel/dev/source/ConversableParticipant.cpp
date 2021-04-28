#include "Conversable.h"

#include "Presence.h"
#include "RemoteUser.h"
#include "JabberID.h"

namespace Origin
{
namespace Chat
{
    bool ConversableParticipant::operator==(const ConversableParticipant &other) const
    {
        return remoteUser()->jabberId() == other.remoteUser()->jabberId();
    }
    
    
    uint qHash(const Origin::Chat::ConversableParticipant &participant)
    {
        return qHash(participant.remoteUser()->jabberId());
    }

    void ConversableParticipant::setRoomPresence(const XmppPresenceProxy& presence)
    {
        // Build a Presence instance from all of our bits
        UserActivity userActivity(presence.activityCategory, presence.activityInstance, presence.activityBody);
        mPresence = Presence(userActivity);
    }
}
}
