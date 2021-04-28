//
//  IGOInviteFriendsToGameViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 3/10/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOInviteFriendsToGameViewController.h"
#include "originmsgbox.h"
#include "originpushbutton.h"

#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"

#include "engine/login/user.h"
#include "engine/social/SocialController.h"
#include "engine/multiplayer/InviteController.h"
#include "RemoteUserProxy.h"
#include "services/debug/DebugService.h"
#include "InviteFriendsWindowView.h"
#include "originwindow.h"

namespace Origin
{
namespace Client
{

IGOInviteFriendsToGameViewController::IGOInviteFriendsToGameViewController(const QString& gameName)
: mGameName(gameName)
, mWindow(NULL)
, mInviteFriendsWindowView(NULL)
{
}

IGOInviteFriendsToGameViewController::~IGOInviteFriendsToGameViewController()
{
}

bool IGOInviteFriendsToGameViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    if(mWindow)
    {
        closeWindow();
    }
    
    Engine::UserRef user = Engine::LoginController::instance()->currentUser();
    bool isInvisible = false, isUnderAge = false, isOffline = false;
    if(user)
    {
        // TODO: It seems we CAN technically send game invites while we are invisible. I'm going to leave this in just in case.
        // We would also need to add the case to error out in invite-friends-window.js in case they have the window up already.
        // Feel free to take it out in the future when things have settled down a bit.
        //isInvisible = user->socialControllerInstance()->chatConnection()->currentUser()->visibility() == Chat::ConnectedUser::Invisible;
        isOffline = Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()) == false;
        isUnderAge = user->isUnderAge();
    }

    if(isUnderAge || isOffline)
    {
        showUnavailableWarning(isUnderAge, isInvisible, isOffline);
    }
    else
    {
        using namespace UIToolkit;
        mInviteFriendsWindowView = new InviteFriendsWindowView(NULL, IGOScope, "", "", "");
        mInviteFriendsWindowView->setDialogType(InviteFriendsWindowJsInterface::Game);

        OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
        OriginWindow::WindowContext windowContext = OriginWindow::OIG;

        mWindow = new OriginWindow(titlebarItems,
            mInviteFriendsWindowView,
            OriginWindow::Custom, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, 
            windowContext);

        mWindow->setAttribute(Qt::WA_DeleteOnClose, false);

        mWindow->setObjectName("IGO Friend Invite Window");
        mWindow->setIgnoreEscPress(true);
        mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        mWindow->setContentMargin(0);
        mWindow->setSizeGripEnabled(false); 
        mWindow->setTitleBarText(tr("ebisu_client_invite_to_game_title").arg(mGameName));

        ORIGIN_VERIFY_CONNECT(mInviteFriendsWindowView, SIGNAL(inviteUsersToGame(const QObjectList& )), this, SLOT(onInviteUsersToGame(const QObjectList& )));
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));

        // Close our view when the webview window is closed
        ORIGIN_VERIFY_CONNECT(mInviteFriendsWindowView->page(), SIGNAL(windowCloseRequested()),this, SLOT(closeWindow()));
    }
    
    return true;
}

void IGOInviteFriendsToGameViewController::closeWindow()
{
    if(mWindow)
    {
        mWindow->deleteLater();
        mWindow = NULL;
    }
}
    
void IGOInviteFriendsToGameViewController::ivcPostSetup()
{
}

void IGOInviteFriendsToGameViewController::onInviteUsersToGame(const QObjectList& users)
{
    // Invite to multiplayer game
    Engine::Social::SocialController *socialController = Engine::LoginController::currentUser()->socialControllerInstance();
    if (socialController)
    {
        for(QObjectList::const_iterator it = users.begin(); it != users.end(); ++it)
        {
            JsInterface::RemoteUserProxy* userProxy = dynamic_cast<JsInterface::RemoteUserProxy*>(*it);
            if (userProxy && userProxy->proxied()) 
            {
                socialController->multiplayerInvites()->inviteToLocalSession(userProxy->proxied()->jabberId());
            }
        }
    }
    closeWindow();
}

void IGOInviteFriendsToGameViewController::showUnavailableWarning(const bool& underAge, const bool& invisible, const bool& /*offline*/)
{
    if(mWindow)
    {
        closeWindow();
    }

    QString title;
    QString message;
    if(underAge)
    {
        title = tr("ebisu_client_unable_to_invite_friends_uppercase");
        message = tr("ebisu_client_unable_to_invite_friends_text");
    }
    else if(invisible)
    {
        title = tr("ebisu_client_currently_invisible_title");
        message = tr("ebisu_client_offline_friend_invite").arg(tr("ebisu_client_presence_ingame"));
    }
    else
    {
        title = tr("ebisu_client_currently_offline_title");
        message = tr("ebisu_client_cant_invite_friends_to_game_when_offline");
    }

    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModal(OriginMsgBox::NoIcon, title, message, tr("ebisu_client_ok_uppercase"));
    mWindow->configForOIG();
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(closeWindow()));
    
    Origin::Engine::IGOController::instance()->igowm()->addPopupWindow(mWindow, Origin::Engine::IIGOWindowManager::WindowProperties(), Origin::Engine::IIGOWindowManager::WindowBehavior());
    mWindow->showWindow();
}

} // Client
} // Origin