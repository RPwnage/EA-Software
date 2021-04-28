#include <QHash>
#include <services/debug/DebugService.h>

#include "XMPPImplAvailabilityConversion.h"

namespace
{
    using namespace Origin::Chat;

    const unsigned int JingleAvailabilityCount = 5;

    struct JingleToAvailabilityEntry
    {
        XmppPresenceShow jingle;
        Presence::Availability presence;
    };

    JingleToAvailabilityEntry JingleToAvailabilityMap[JingleAvailabilityCount] = {
        {XMPP_PRESENCE_CHAT,          Presence::Chat},
        {XMPP_PRESENCE_DEFAULT,       Presence::Online},
        {XMPP_PRESENCE_AWAY,          Presence::Away},
        {XMPP_PRESENCE_XA,            Presence::Xa},
        {XMPP_PRESENCE_DND,           Presence::Dnd}
    };
}

namespace Origin
{
namespace Chat
{
    Presence::Availability jingleToAvailability(XmppPresenceShow jingle)
    {
        for(unsigned int i = 0; i < JingleAvailabilityCount; i++)
        {
            if (JingleToAvailabilityMap[i].jingle == jingle)
            {
                return JingleToAvailabilityMap[i].presence;
            }
        }

        ORIGIN_ASSERT(0);
        return Presence::Unavailable;
    }

    buzz::XmppPresenceShow availabilityToJingle(Presence::Availability presence)
    {
        for(unsigned int i = 0; i < JingleAvailabilityCount; i++)
        {
            if (JingleToAvailabilityMap[i].presence == presence)
            {
                return (buzz::XmppPresenceShow)JingleToAvailabilityMap[i].jingle;
            }
        }

        ORIGIN_ASSERT(0);
        return buzz::XMPP_PRESENCE_DEFAULT;
    }

}
}
