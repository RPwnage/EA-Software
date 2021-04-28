#include <QTimer>

#include <services/debug/DebugService.h>

#include <engine/content/ContentController.h>
#include <engine/social/SocialController.h>
#include <engine/igo/IGOController.h>

#include <chat/OriginConnection.h>
#include <chat/ConnectedUser.h>

#include "ConnectedUserPresenceStack.h"
#include "InGameStatusWatcher.h"

namespace
{
    using namespace Origin::Engine::Content;

    class DelayedInGameMessage : public QTimer
    {
    public:
        DelayedInGameMessage(EntitlementRef ref) :
            mEntitlement(ref)
        {
            setInterval(3000);
            setSingleShot(true);
            start();
        }

        EntitlementRef entitlement() const 
        {
            return mEntitlement.toStrongRef();
        }

    private:
        EntitlementWRef mEntitlement;
    };
}


namespace Origin
{
namespace Engine
{
namespace Social
{
    InGameStatusWatcher::InGameStatusWatcher(ConnectedUserPresenceStack *presenceStack, UserRef user, QObject *parent) :
        QObject(parent),
        mPresenceStack(presenceStack),
        mUser(user)
    {
        Content::ContentController *contentController = user->contentControllerInstance();

        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)),
                this, SLOT(onPlayStarted(Origin::Engine::Content::EntitlementRef)));
        
        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)),
                this, SLOT(onPlayFinished(Origin::Engine::Content::EntitlementRef)));
    
        // Defer this by a tick
        // We are created as part of user construction but IGO ends up calling GlobalProgressBar which doesn't have
        // a user object passed in. Because currentUserContentController returns null until the user is fully built
        // this causes it to fail to content a signal.
        QMetaObject::invokeMethod(this, "connectBroadcastSignals", Qt::QueuedConnection);
    }
    
    void InGameStatusWatcher::onPlayStarted(Content::EntitlementRef entRef)
    {
        // Wait 3000ms to make sure the game is still being played
        // This is for games such as BF3 that have a short-lived web launcher
        DelayedInGameMessage *delayMessage = new DelayedInGameMessage(entRef);

        ORIGIN_VERIFY_CONNECT(delayMessage, SIGNAL(timeout()), 
                this, SLOT(delayedPlayStarted()));
    }

    void InGameStatusWatcher::delayedPlayStarted()
    {
        using namespace Content;
        using namespace Chat;

        DelayedInGameMessage *message = dynamic_cast<DelayedInGameMessage*>(sender());

        // Get our content controller
        UserRef user(mUser.toStrongRef());

        if (!user)
        {
            return;
        }

        Content::EntitlementRef entitlement;
        if (message)
        {
        // Make sure the entitlement still exists and is playing
            entitlement = message->entitlement();
            message->deleteLater();
        }

        if (!entitlement || !entitlement->localContent()->playing())
        {
            return;
        }

        // Build our playing status
        QString displayName(entitlement->contentConfiguration()->displayName());

        // Stash our product ID away - it might get blown away below
        mPresenceProductId = entitlement->contentConfiguration()->productId();

        if (entitlement->localContent()->shouldHide())  // scramble the display name
            displayName = tr("ebisu_client_notranslate_embargomode_title");

        const OriginGameActivity playingStatus(
                true,
                mPresenceProductId,
                displayName,
                false,
                false,
                QString(),
                entitlement->contentConfiguration()->multiplayerId());
        
        // Create a presence stack entry
        const Chat::Presence inGamePresence(Presence::Online, displayName, playingStatus);

        // Set it
        mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::InGamePriority, inGamePresence);
    }

    void InGameStatusWatcher::onPlayFinished(Content::EntitlementRef entRef)
    {
        using namespace Chat;

        UserRef user(mUser.toStrongRef());

        if (!user)
        {
            return;
        }

        // Are we asserting playing presence for this game?
        if (mPresenceProductId != entRef->contentConfiguration()->productId())
        {
            return;
        }

        // Clear it
        mPresenceStack->clearPresenceForPriority(ConnectedUserPresenceStack::InGamePriority);

        mPresenceProductId = QString();
    }

    void InGameStatusWatcher::onBroadcastStarted(const QString& broadcastUrl)
    {
        const Chat::OriginGameActivity newActivity = mPresenceStack->presenceForPriority(ConnectedUserPresenceStack::InGamePriority).gameActivity().withBroadcastUrl(broadcastUrl);
        const Chat::Presence inGamePresence(Chat::Presence::Online, newActivity.gameTitle(), newActivity);
        mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::InGamePriority, inGamePresence);
    }

    void InGameStatusWatcher::onBroadcastStopped()
    {
        const Chat::OriginGameActivity newActivity = mPresenceStack->presenceForPriority(ConnectedUserPresenceStack::InGamePriority).gameActivity().withBroadcastUrl("");
        const Chat::Presence inGamePresence(Chat::Presence::Online, newActivity.gameTitle(), newActivity);
        mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::InGamePriority, inGamePresence);
    }
    
    void InGameStatusWatcher::connectBroadcastSignals()
    {
        IGOController* igoController = IGOController::instance();
        ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(broadcastStarted(const QString&)), this, SLOT(onBroadcastStarted(const QString&)));
        ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(broadcastStopped()), this, SLOT(onBroadcastStopped()));
    }
}
}
}
