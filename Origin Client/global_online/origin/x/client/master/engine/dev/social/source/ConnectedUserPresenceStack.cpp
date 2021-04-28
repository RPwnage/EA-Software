#include <QMutexLocker>

#include "services/debug/DebugService.h"
#include "ConnectedUserPresenceStack.h"
#include "chat/ConnectedUser.h"

namespace Origin
{
namespace Engine
{
namespace Social
{

ConnectedUserPresenceStack::ConnectedUserPresenceStack(Chat::ConnectedUser *connectedUser) : 
    mConnectedUser(connectedUser),
    mStateLock(QMutex::Recursive)
{
    // Create our slots now
    // All entries will be default constructed as null
    mPresenceStack.resize(MaximumPriority + 1);

    mPresenceStack[DefaultOnlinePriority] = Chat::Presence(Chat::Presence::Online);
}

void ConnectedUserPresenceStack::clearPresenceForPriority(PresencePriority priority)
{
    // Set an invalid entry for the passed priority
    setPresenceForPriority(priority, Chat::Presence());
}

void ConnectedUserPresenceStack::setPresenceForPriority(PresencePriority priority, const Chat::Presence &entry)
{
    bool emitChanged = false;

    {
        QMutexLocker lock(&mStateLock);

        if (entry != mPresenceStack[priority])
        {
            emitChanged = true;
            mPresenceStack[priority] = entry;

            applyToConnectedUser();
        }
    }

    if (emitChanged)
    {
        emit changed();
    }
}

Chat::Presence ConnectedUserPresenceStack::presenceForPriority(PresencePriority priority) const
{
    QMutexLocker lock(&mStateLock);
    if(priority >= mPresenceStack.size())
        return Chat::Presence();
    else
        return mPresenceStack[priority];
}

Chat::Presence ConnectedUserPresenceStack::highestPriorityPresence() const
{
    QMutexLocker lock(&mStateLock);

    // Find the entry with the highest priority
    for(int i = MaximumPriority; i >= DefaultOnlinePriority; i--)
    {
        const Chat::Presence &entry = mPresenceStack[i];

        if (!entry.isNull())
        {
            return entry;
        }
    }

    // Couldn't find anything
    return Chat::Presence();
}

void ConnectedUserPresenceStack::applyToConnectedUser()
{
    using Chat::Presence;
    mConnectedUser->requestPresence(highestPriorityPresence());
}

}
}
}
