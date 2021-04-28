#ifndef _CHATMODEL_XMPPIMPLPRESENCECONVERSION_H
#define _CHATMODEL_XMPPIMPLPRESENCECONVERSION_H

#include "Presence.h"
#include "XMPPImplTypes.h"

#include "JingleClient.h"

namespace Origin
{
namespace Chat
{
    Presence::Availability jingleToAvailability(XmppPresenceShow);

    buzz::XmppPresenceShow availabilityToJingle(Presence::Availability);
}
}

#endif
