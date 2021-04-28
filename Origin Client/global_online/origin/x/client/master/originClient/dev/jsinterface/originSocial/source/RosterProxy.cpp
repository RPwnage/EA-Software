#include "RosterProxy.h"
#include "RemoteUserProxy.h"

#include <chat/Roster.h>
#include <services/debug/DebugService.h>

#include "ClientFlow.h"
#include "widgets/social/source/SocialViewController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    
RosterProxy::RosterProxy(Chat::Roster *roster) :
    mProxied(roster)
{
    ORIGIN_VERIFY_CONNECT(roster, SIGNAL(loaded()), this, SIGNAL(loaded()));
    ORIGIN_VERIFY_CONNECT(roster, SIGNAL(anyUserChange()), this, SIGNAL(anyUserChange()));
}

bool RosterProxy::hasLoaded()
{
    return mProxied->hasLoaded();
}

bool RosterProxy::hasFriends()
{
    return mProxied->hasFullySubscribedContacts();
}

void RosterProxy::removeContact(QObject *obj)
{
    RemoteUserProxy *contactProxy = dynamic_cast<RemoteUserProxy*>(obj);

    if (!contactProxy)
    {
        return;
    }

    ClientFlow::instance()->socialViewController()->removeContactWithConfirmation(contactProxy->proxied());
}

}
}
}
