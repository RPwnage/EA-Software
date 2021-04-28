#include "IncomingMucRoomInvite.h"

namespace Origin
{
namespace Chat
{
    IncomingMucRoomInvite IncomingMucRoomInvite::fromJingle(const buzz::Jid& inviter, const buzz::Jid& room, const QString& reason)
    {
        const QString roomId = JabberID::fromJingle(room).node();
        const JabberID inviterJid = JabberID::fromJingle(inviter);

        return IncomingMucRoomInvite(roomId, inviterJid, reason, "");
    }
}
}
