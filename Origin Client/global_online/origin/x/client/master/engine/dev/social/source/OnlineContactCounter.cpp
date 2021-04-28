#include "engine/social/OnlineContactCounter.h"

#include <QTimer>

#include <services/debug/DebugService.h>

#include <chat/Roster.h>
#include <chat/RemoteUser.h>

namespace Origin
{
namespace Engine
{
namespace Social
{

namespace
{
    const unsigned int UpdateDelayMs = 250;

    bool contactIsOnline(Chat::RemoteUser *contact)
    {
        const Chat::Presence presence(contact->presence());                                                             
        return !presence.isNull() && (presence.availability() != Chat::Presence::Unavailable);
    }
}
    
OnlineContactCounter::OnlineContactCounter(Chat::Roster *roster) :
    mRoster(roster),
    mCachedOnlineContactCount(0),
    mUpdateQueued(false)
{
    ORIGIN_VERIFY_CONNECT(roster, SIGNAL(contactAdded(Origin::Chat::RemoteUser *)),
            this, SLOT(connectContactSignals(Origin::Chat::RemoteUser *)));
    
    ORIGIN_VERIFY_CONNECT(roster, SIGNAL(contactRemoved(Origin::Chat::RemoteUser *)),
            this, SLOT(disconnectContactSignals(Origin::Chat::RemoteUser *)));

    QSet<Chat::RemoteUser*> contacts = roster->contacts();

    // Connect everyone's signals and get an initial count
    for(auto it = contacts.constBegin(); it != contacts.constEnd(); it++)
    {
        Chat::RemoteUser *contact = *it;

        mCachedOnlineContactCount += contactIsOnline(contact);
        connectContactSignals(contact);
    }
}
    
void OnlineContactCounter::connectContactSignals(Origin::Chat::RemoteUser *user)
{
    ORIGIN_VERIFY_CONNECT(
        user, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
        this, SLOT(asyncUpdate()));

    asyncUpdate();
}

void OnlineContactCounter::disconnectContactSignals(Origin::Chat::RemoteUser *user)
{
    ORIGIN_VERIFY_DISCONNECT(
        user, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
        this, SLOT(asyncUpdate()));
    
    asyncUpdate();
}
    
void OnlineContactCounter::asyncUpdate()
{
    if (!mUpdateQueued)
    {
        mUpdateQueued = true;

        QTimer::singleShot(UpdateDelayMs, this, SLOT(updateNow()));
    }
}

void OnlineContactCounter::updateNow()
{
    unsigned int newOnlineContactCount = 0;
    mUpdateQueued = false;

    QSet<Chat::RemoteUser*> contacts = mRoster->contacts();

    for(auto it = contacts.constBegin(); it != contacts.constEnd(); it++)
    {
        newOnlineContactCount += contactIsOnline(*it);
    }

    if (newOnlineContactCount != mCachedOnlineContactCount)
    {
        // Online contact count changed
        mCachedOnlineContactCount = newOnlineContactCount;
        emit onlineContactCountChanged(newOnlineContactCount);
    }
}

}
}
}
