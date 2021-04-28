#include "ConnectedUserProxy.h"
#include "ConversionHelpers.h"

#include "chat/ConnectedUser.h"

#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"
#include "engine/social/UserAvailabilityController.h"

#include "services/debug/DebugService.h"

#include "flows/ToInvisibleFlow.h"

namespace
{
    QString visibilityToString(Origin::Chat::ConnectedUser::Visibility visibility)
    {
        using namespace Origin::Chat;

        switch(visibility)
        {
            case ConnectedUser::Visible:
                return "VISIBLE";
            case ConnectedUser::Invisible:
                return "INVISIBLE";
        }

        ORIGIN_ASSERT(0);
        return QString();
    }
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{

using namespace ConversionHelpers;

ConnectedUserProxy::ConnectedUserProxy(OriginSocialProxy *socialProxy, Engine::UserRef user, int id, Chat::ConnectedUser *proxied) : 
    SocialUserProxy(socialProxy, user->socialControllerInstance(), id, proxied),
    mConnectedUser(proxied),
    mUser(user)
{
    ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(visibilityChanged(Origin::Chat::ConnectedUser::Visibility)),
            this, SIGNAL(visibilityChanged()));
    
    ORIGIN_VERIFY_CONNECT(user->socialControllerInstance()->userAvailability(), SIGNAL(allowedTransitionsChanged()),
            this, SIGNAL(allowedAvailabilityTransitionsChanged()));
}

QVariant ConnectedUserProxy::nickname()
{
    return mUser->eaid();
}

QString ConnectedUserProxy::requestedAvailability()
{
    if (mConnectedUser.isNull())
    {
        return "ONLINE";
    }

    return availabilityToString(mConnectedUser->presence().availability());
}


void ConnectedUserProxy::setRequestedAvailability(const QString &str)
{
    if (mConnectedUser.isNull())
    {
        return;
    }

    bool parsedOk;

    // Attempt to parse the passed string
    const Chat::Presence::Availability wantedAvailability(stringToAvailability(str, parsedOk));

    if (parsedOk)
    {
        // Do a high-level presence transition
        mUser->socialControllerInstance()->userAvailability()->transitionTo(wantedAvailability);
    }
}

QStringList ConnectedUserProxy::allowedAvailabilityTransitions()
{
    Engine::Social::UserAvailabilityController *userAvailability = mUser->socialControllerInstance()->userAvailability();

    QSet<Chat::Presence::Availability> allowedAvailabilitys(userAvailability->allowedTransitions());
    QStringList allowedStrings;

    // Convert to JavaScript enum strings
    for(QSet<Chat::Presence::Availability>::ConstIterator it = allowedAvailabilitys.constBegin();
        it != allowedAvailabilitys.constEnd();
        it++)
    {
        allowedStrings << availabilityToString(*it); 
    }

    return allowedStrings;
}

QString ConnectedUserProxy::visibility()
{
    return visibilityToString(mConnectedUser->visibility());
}

QString ConnectedUserProxy::requestedVisibility()
{
    return visibilityToString(mConnectedUser->requestedVisibility());
}

void ConnectedUserProxy::setRequestedVisibility(const QString &str)
{
    Chat::ConnectedUser::Visibility visibility;

    if (str == "VISIBLE")
    {
        visibility = Chat::ConnectedUser::Visible;
    }
    else if (str == "INVISIBLE")
    {
        visibility = Chat::ConnectedUser::Invisible;
    }
    else
    {
        return;
    }

    ToInvisibleFlow *flow = new ToInvisibleFlow(mUser->socialControllerInstance());
    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(bool)), flow, SLOT(deleteLater()));

    flow->start(visibility);
}

void ConnectedUserProxy::setInitial(const QString &str) {
    QMetaObject::invokeMethod(mConnectedUser, "applyConnectionState");
}

quint64 ConnectedUserProxy::nucleusPersonaId()
{
    return mUser->personaId();
}

}
}
}
