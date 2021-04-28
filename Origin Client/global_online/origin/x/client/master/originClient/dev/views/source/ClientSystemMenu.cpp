/////////////////////////////////////////////////////////////////////////////
// ClientSystemMenu.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "ClientSystemMenu.h"
#include "EbisuSystemTray.h"
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformJumplist.h"
#include "AvailabilityMenuViewController.h"
#include "ContentFlowController.h"
#include "MainFlow.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{
// We are using ':' in our product id string, so we cannot use it as a separator
const char* PRODUCTID_SEPARATOR = ";";	// not allowed in windows or mac filenames

ClientSystemMenu::ClientSystemMenu(QWidget* parent)
: CustomQMenu(ClientScope, parent)
, mMostRecent(NULL)
, mOpenSeparator(NULL)
{
    ORIGIN_VERIFY_CONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)),
        this, SLOT(onPlayingGameTimeUpdated(Origin::Engine::Content::EntitlementRef)));

    mRecentGames.clear();
    // We don't want to show the Open Origin option on Mac
    addSystemTrayOption(tr("ebisu_client_open_app").arg(tr("application_name")), Action_Open);
    addSeparator();

    // Create AvailabilityMenuViewController
    Engine::Social::SocialController* socialController = Engine::LoginController::currentUser()->socialControllerInstance();
    AvailabilityMenuViewController* presenceViewController = new AvailabilityMenuViewController(socialController, this);
    addMenu(presenceViewController->menu());
    
    addSeparator();
    addSystemTrayOption(tr("ebisu_client_redeem_game_code"), Action_Redeem);

    addSeparator();
    addSystemTrayOption(tr("ebisu_mainmenuitem_logout"), Action_Logout);

#if defined(ORIGIN_PC)
    // We don't want to show "Exit" because Mac has a default "Quit" option
    addSystemTrayOption(tr("ebisu_mainmenuitem_quit_app").arg(tr("application_name")), Action_Quit);
#endif
}


ClientSystemMenu::~ClientSystemMenu()
{
}


QAction* ClientSystemMenu::addSystemTrayOption(const QString& text, const Action& actionType, const bool& enabled)
{
    QAction* action = new QAction(this);
    action->setObjectName(QString::number(actionType));
    action->setEnabled(enabled);
    action->setText(text);
    addAction(action);
    ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(onActionTriggered()));
    return action;
}


void ClientSystemMenu::onActionTriggered()
{
    if (sender() == NULL || sender()->objectName().isEmpty())
        return;

    switch (sender()->objectName().toInt())
    {
    case Action_Open:
        GetTelemetryInterface()->Metric_JUMPLIST_ACTION("open", "SystemTray", "");
        emit openTriggered();
        break;
    case Action_Quit:
        ORIGIN_LOG_EVENT << "User requested application exit via system tray, kicking off exit flow.";
        GetTelemetryInterface()->Metric_JUMPLIST_ACTION("quit", "SystemTray", "");
        emit quitTriggered();
        break;
    case Action_Redeem:
        GetTelemetryInterface()->Metric_JUMPLIST_ACTION("redeem", "SystemTray", "");
        emit redeemTriggered();
        break;
    case Action_Logout:
        GetTelemetryInterface()->Metric_JUMPLIST_ACTION("logout", "SystemTray", "");
        emit logoutTriggered();
        break;
    default:
        break;
    }
}


void ClientSystemMenu::addRecentGameToMenu(QString title, QString product_id)
{
    // first check to see if already in menu
    for (QList<OriginSysTrayMenuItem* >::ConstIterator it = mRecentGames.constBegin(); it != mRecentGames.constEnd(); it++)
    {
        OriginSysTrayMenuItem* item = (*it);
        if (item && item->mProductID == product_id)
        {
            // already here - push to the front
            if (mMostRecent != item->mMenuItem)
            {
                this->removeAction(item->mMenuItem);
                this->insertAction(mMostRecent, item->mMenuItem);
                mMostRecent = item->mMenuItem;
            }

            return;
        }
    }

    OriginSysTrayMenuItem* menu_item = new OriginSysTrayMenuItem(title, product_id);
    menu_item->mMenuItem = new QAction(this);

    menu_item->mMenuItem->setText(title);

    // If we don't have the open separator - add it to the list
    if (mOpenSeparator == NULL)
    {
        mOpenSeparator = this->addSeparator();
        this->insertAction(this->actions().first(), mOpenSeparator);
    }

    if (mMostRecent == NULL)
        this->insertAction(mOpenSeparator, menu_item->mMenuItem);
    else
        this->insertAction(mMostRecent, menu_item->mMenuItem);

    mMostRecent = menu_item->mMenuItem;

    ORIGIN_VERIFY_CONNECT(menu_item->mMenuItem, SIGNAL(triggered()), menu_item, SLOT(onPlay()));

    mRecentGames.push_back(menu_item);

    while (mRecentGames.count() > (int)Services::PlatformJumplist::get_recent_list_size())
    {
        OriginSysTrayMenuItem* item = mRecentGames.takeFirst();
        this->removeAction(item->mMenuItem);

        ORIGIN_VERIFY_DISCONNECT(item->mMenuItem, SIGNAL(triggered()), item, SLOT(onPlay()));

        delete item->mMenuItem;
        delete item;
    }
}


struct RECENTLY_PLAYED
{
    QString display;
    QString product_id;
    QString app_path;
    int     item_sub_type;
};


void ClientSystemMenu::removeAllRecent()
{
    for (QList<OriginSysTrayMenuItem* >::ConstIterator it = mRecentGames.constBegin(); it != mRecentGames.constEnd(); it++)
    {
        OriginSysTrayMenuItem* item = (*it);
        this->removeAction(item->mMenuItem);

        ORIGIN_VERIFY_DISCONNECT(item->mMenuItem, SIGNAL(triggered()), item, SLOT(onPlay()));

        delete item->mMenuItem;
        item->mMenuItem = NULL;
    }

    mRecentGames.clear();

    mMostRecent = NULL;

    Services::PlatformJumplist::clear_recently_played();
}


void ClientSystemMenu::updateRecentPlayedGames()
{
    QList<struct RECENTLY_PLAYED> rplist;

    Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
    if (contentController == NULL)
        return;

    QList<Engine::Content::EntitlementRef> entitlements = contentController->entitlements();

    QString liststr = Services::readSetting(Services::SETTING_JUMPLIST_RECENTLIST, Engine::LoginController::currentUser()->getSession());
    if (liststr.isEmpty())
    {
        removeAllRecent();
        return;
    }

    bool done = true;
    bool changed = false;

    // make two identical lists so we can remove items from one while traversing the entire list
    QStringList list = liststr.split(PRODUCTID_SEPARATOR);
    QStringList list_src = liststr.split(PRODUCTID_SEPARATOR);

    QStringList::const_iterator it, end;
    for (it = list_src.begin(); it != list_src.end(); it++)
    {
        Engine::Content::EntitlementRef ent = contentController->entitlementByProductId((*it));

        if ((ent == NULL) || !ent->contentConfiguration() || !ent->localContent() || !ent->contentConfiguration()->isBaseGame() || !ent->localContent()->installed())
        {
            list.removeOne(*it);
            changed = true;
        }
        else
        {
            done = false;	// we have at least one
        }
    }

    if (changed)
    {
        liststr = list.join(PRODUCTID_SEPARATOR);

        Services::writeSetting(Services::SETTING_JUMPLIST_RECENTLIST, liststr, Engine::LoginController::currentUser()->getSession());
    }

    if (!done)
    {	// find the last item in the list
        for (it = list.begin(); it != list.end();)
        {
            end = it;
            it++;
        }
    }

    while (!done)
    {
        Engine::Content::EntitlementRef ent = contentController->entitlementByProductId((*end));

        //  Ensure that entitlement exist because removed NOGS sometimes do not have them.
        if (ent.isNull() == false)
        {
            struct RECENTLY_PLAYED rp;
            rp.display = ent->contentConfiguration()->displayName();
            rp.product_id = ent->contentConfiguration()->productId();
            rp.app_path = ent->localContent()->executablePath();
            rp.item_sub_type = (int)ent->contentConfiguration()->itemSubType();
            rplist.push_back(rp);
        }

        if (end == list.begin())
            done = true;
        end--;
    }

    removeAllRecent();

    // add back most recent
    int count = Services::PlatformJumplist::get_recent_list_size();
    int skip = rplist.size() - count;

    for (QList<struct RECENTLY_PLAYED>::const_iterator it = rplist.begin(); (it != rplist.end()); it++, skip--)
    {
        // because we add from the top and this list is sorted last used to most recently used, for the system tray
        // we only want to include the top x items where x is the size of the recent jumplist set by the user.
        if (skip <= 0)
        {
            // One ampersand char is understood as a "fast navigation" character (underlined letter in a menu)
            // Two ampersand chars are understood as '&'. Therefore, replace '&' with '&&' for a proper display
            QString display = (*it).display;
            addRecentGameToMenu(display.replace("&", "&&"), (*it).product_id);
        }

        // however, we want to pass all items into the jumplist in case the user removes an item, it will back fill with the less recently used games
        Services::PlatformJumplist::add_launched_game((*it).display, (*it).product_id, (*it).app_path, (*it).item_sub_type);
    }
}


void ClientSystemMenu::onPlayingGameTimeUpdated(Origin::Engine::Content::EntitlementRef entRef)
{
    // only update the latest
    Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
    if (contentController == NULL)
        return;

    QList<Engine::Content::EntitlementRef> entitlements = contentController->entitlements();
    for (QList<Engine::Content::EntitlementRef>::const_iterator it = entitlements.begin(); it != entitlements.end(); it++)
    {
        Engine::Content::EntitlementRef ent = (*it);

        if (!ent || !ent->contentConfiguration() || !ent->localContent())
            continue;

        if (!ent->contentConfiguration()->isBaseGame())
            continue;

        if (!ent->localContent()->installed())
            continue;

        if (ent->contentConfiguration()->productId() == entRef->contentConfiguration()->productId())
        {
            QString displayname = ent->contentConfiguration()->displayName();
            // One ampersand char is understood as a "fast navigation" character (underlined letter in a menu)
            // Two ampersand chars are understood as '&'. Therefore, replace '&' with '&&' for a proper display
            if (displayname.contains("&"))
                displayname.replace("&", "&&");
            QString product_id = ent->contentConfiguration()->productId();
            QString app_path = ent->localContent()->executablePath();

            addRecentGameToMenu(displayname, product_id);
            // Don't use the display name that has changed the & to &&. && is a Qt exclusive thing. The platform jumplist would should it as &&.
            Services::PlatformJumplist::add_launched_game(ent->contentConfiguration()->displayName(), product_id, app_path, (int)ent->contentConfiguration()->itemSubType());

            QList<Engine::Content::EntitlementRef> entitlements = contentController->entitlements();

            QString liststr = Services::readSetting(Services::SETTING_JUMPLIST_RECENTLIST, Engine::LoginController::currentUser()->getSession());
            QStringList list;
            if (!liststr.isEmpty())
                list = liststr.split(PRODUCTID_SEPARATOR);

            list.prepend(product_id);

            list.removeDuplicates();

            liststr = list.join(PRODUCTID_SEPARATOR);

            Services::writeSetting(Services::SETTING_JUMPLIST_RECENTLIST, liststr, Engine::LoginController::currentUser()->getSession());

            break;
        }
    }
}

//////
//////

OriginSysTrayMenuItem::OriginSysTrayMenuItem(QString title, QString product_id)
{
    mTitle = title;
    mProductID = product_id;
    mMenuItem = NULL;
}


OriginSysTrayMenuItem::~OriginSysTrayMenuItem()
{
}


void OriginSysTrayMenuItem::onPlay()
{
    // do a forced install check so that recently uninstalled games are checked properly
    Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
    if (contentController == NULL)
        return;

    Engine::Content::EntitlementRef entitlement = contentController->entitlementByProductId(mProductID);

    if (!entitlement || !entitlement->localContent())
        return;

    entitlement->localContent()->installed(true);	// force check

    Client::MainFlow::instance()->contentFlowController()->startPlayFlow(mProductID, false);
    GetTelemetryInterface()->Metric_JUMPLIST_ACTION("launchgame", "SystemTray", "");
}

}
}