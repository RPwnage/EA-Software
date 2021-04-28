#include "IncomingMucRoomInvite.h"

#include "OutgoingMucRoomInvite.h"

namespace Origin
{
namespace Chat
{
    buzz::Jid OutgoingMucRoomInvite::toJingle() const
    {
        return mInvitee.toJingle();
    }
}
}
