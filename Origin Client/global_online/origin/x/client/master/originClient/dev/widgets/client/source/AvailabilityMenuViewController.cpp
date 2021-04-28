#include "AvailabilityMenuViewController.h"

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QApplication>

#include "services/debug/DebugService.h"
#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserAvailabilityController.h"

#include "flows/ToInvisibleFlow.h"

namespace Origin
{
namespace Client
{
    AvailabilityMenuViewController::AvailabilityMenuViewController(Engine::Social::SocialController *socialController, QWidget *parent) : 
        QObject(parent),
        mSocialController(socialController)
    {
        // Make a top-level menu to contain everything 
        mMenu = new QMenu(parent);
        mMenu->setObjectName("mainSetStatusMenu");
        mMenu->setTitle(tr("ebisu_mainmenuitem_set_presence"));

        // Enable/disable ourselves when our connection state changes
        ORIGIN_VERIFY_CONNECT(mSocialController->chatConnection(), SIGNAL(connected()),
                this, SLOT(onChatConnected()));
        ORIGIN_VERIFY_CONNECT(mSocialController->chatConnection(), SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)),
                this, SLOT(onChatDisconnected()));

        mMenu->setEnabled(mSocialController->chatConnection()->established());

        ORIGIN_VERIFY_CONNECT(mMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
            
        // Make an action group so all the options are exclusive
        mActionGroup = new QActionGroup(mMenu);

        // Populate our actions
        mVisibleActions.insert(addAction(":/platform/green-dot.png", tr("ebisu_mainmenuitem_presence_online")), Chat::Presence::Online);
        mVisibleActions.insert(addAction(":/platform/red-dot.png", tr("ebisu_mainmenuitem_presence_away")), Chat::Presence::Away);

        mInvisibleAction = addAction(":/platform/clear-dot.png", tr("ebisu_mainmenuitem_presence_invisible"));
    }

    void AvailabilityMenuViewController::onAboutToShow()
    {
        using namespace Chat;
        using namespace Engine::Social;

        // Find our current availability
        const ConnectedUser *connectedUser = mSocialController->chatConnection()->currentUser();
        const Chat::Presence presence(connectedUser->requestedPresence());
        const Chat::Presence::Availability requestedAvailability(presence.availability());
        const bool invisible(connectedUser->visibility() == Chat::ConnectedUser::Invisible);

        // Find the availabilities we can transition to
        QSet<Chat::Presence::Availability> allowedTransitions = mSocialController->userAvailability()->allowedTransitions();

        // Update all of our presence stuff just in time
        for(ActionAvailabilityHash::ConstIterator it = mVisibleActions.constBegin();
            it != mVisibleActions.constEnd();
            it++)
        {
            QAction *action = it.key();
            Chat::Presence::Availability availability = it.value();
        
            // Change the name of the Online label depending on if we're in-game
            if (availability == Chat::Presence::Online)
            {
                if (!presence.gameActivity().isNull())
                {
                    action->setText(tr("ebisu_client_presence_ingame"));
                }
                else
                {
                    action->setText(tr("ebisu_mainmenuitem_presence_online"));
                }
            }

            action->setChecked((availability == requestedAvailability) && !invisible);
            action->setVisible(allowedTransitions.contains(availability));
        }
         
        mInvisibleAction->setChecked(invisible);
    }

    void AvailabilityMenuViewController::onTriggered()
    {
        QAction *action = dynamic_cast<QAction*>(sender());
        
        if (!action)
        {
            // Bad cast
            return;
        }

        if (action == mInvisibleAction)
        {
            // Become invisible if we're not
            ToInvisibleFlow *flow = new ToInvisibleFlow(mSocialController);

            ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(bool)), flow, SLOT(deleteLater()));
            flow->start(Chat::ConnectedUser::Invisible);
        }
        else
        {
            // Transition to the triggered availability
            mSocialController->userAvailability()->transitionTo(mVisibleActions[action]);

            // Become visible if we're not
            mSocialController->chatConnection()->currentUser()->requestVisibility(Chat::ConnectedUser::Visible);

            if (!mVisibleActions.contains(action))
            {
                // Couldn't find the action
                return;
            }
        }
    }
        
    QAction* AvailabilityMenuViewController::addAction(const QString& iconFilename, const QString &text)
    {
        // Create the action and add it to the action group and menu
        QAction *action = new QAction(QIcon(iconFilename), text, mMenu);

        if (iconFilename.count())
        {
            action->setProperty("style", "hasIcon");
            mMenu->setStyle(QApplication::style());
        }

        action->setCheckable(true);

        mActionGroup->addAction(action);
        mMenu->addAction(action);

        // Listen for when its triggered
        ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(onTriggered()));

        return action;
    }

    void AvailabilityMenuViewController::onChatConnected()
    {
        mMenu->setEnabled(true);
    }
    
    void AvailabilityMenuViewController::onChatDisconnected()
    {
        mMenu->setEnabled(false);
    }
}
}
