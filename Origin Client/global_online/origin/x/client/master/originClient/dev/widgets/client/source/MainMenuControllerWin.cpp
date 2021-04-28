/////////////////////////////////////////////////////////////////////////////
// MainMenuControllerWin.cpp
//
// Copyright (c) 2012-2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MainMenuControllerWin.h"
#include "services/debug/DebugService.h"
#include "engine/content/ContentController.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/OnlineContactCounter.h"
#include "engine/social/ConversationManager.h"

#include "AvailabilityMenuViewController.h"

//#define ORIGIN_X -- defined in the originClient.build, used to disable things not yet implemented

namespace Origin
{
    namespace Client
    {
        MainMenuControllerWin::MainMenuControllerWin(bool underage, bool online, bool inDebug, bool developerMode, QWidget *parent)
            : MainMenuController(underage, online, inDebug, developerMode, parent)
        {
            init();
        }

        MainMenuControllerWin::~MainMenuControllerWin()
        {
        }

        void MainMenuControllerWin::init()
        {
            actionExit = new QAction(NULL);
            actionRedeem_Product_Code = new QAction(NULL);
            actionReload_SPA = new QAction(NULL);
            actionReload_My_Games = new QAction(NULL);
            actionApplication_Settings = new QAction(NULL);
            actionAccount = new QAction(NULL);
            actionGo_Offline = new QAction(NULL);
            actionLog_Out = new QAction(NULL);
            actionAdd_a_Friend = new QAction(NULL);
            actionOrigin_Help = new QAction(NULL);
            actionOrigin_Forum = new QAction(NULL);
            actionOrder_History = new QAction(NULL);
            actionViewProfile = new QAction(NULL);
            actionAbout = new QAction(NULL);
            actionAddNonOriginGame = new QAction(NULL);
            actionOER = new QAction(NULL);

            menuFile = new QMenu(mMenuBar);
            menuFile->setObjectName("mainOriginMenu");
            menuGames = new QMenu(mMenuBar);
            menuGames->setObjectName("mainGamesMenu");
            menuFriends = new QMenu(mMenuBar);
            menuFriends->setObjectName("mainFriendsMenu");
            menuHelp = new QMenu(mMenuBar);
            menuHelp->setObjectName("mainHelpMenu");

            // Top-level menu dropdowns
            mMenuBar->addAction(menuFile->menuAction());
            mMenuBar->addAction(menuGames->menuAction());
            if (!mUnderage) mMenuBar->addAction(menuFriends->menuAction());
            mMenuBar->addAction(menuHelp->menuAction());

            // Debug menu goes here if enabled
            insertDebugMenu();

            // Origin bmenu items
            menuFile->addAction(actionApplication_Settings);
            if (!mUnderage)
            {
                menuFile->addAction(actionAccount);
                menuFile->addAction(actionOrder_History);
                menuFile->addAction(actionViewProfile);
            }
            menuFile->addSeparator();
            menuFile->addAction(actionReload_SPA);
            setActionShortcut(actionReload_SPA, QKeySequence(Qt::CTRL + Qt::Key_R));
            menuFile->addSeparator();
            menuFile->addAction(actionGo_Offline);
            menuFile->addAction(actionLog_Out);
            menuFile->addAction(actionExit);
            
            // Games menu items
            menuGames->addAction(actionRedeem_Product_Code);
            menuGames->addAction(actionAddNonOriginGame);
            menuGames->addAction(actionReload_My_Games);

            // Create AvailabilityMenuViewController
            Engine::Social::SocialController *socialController = Engine::LoginController::currentUser()->socialControllerInstance();
            AvailabilityMenuViewController *presenceViewController = new AvailabilityMenuViewController(socialController, mMenuBar);

            // Friend menu items
            menuFriends->addAction(actionAdd_a_Friend);
            menuFriends->addSeparator();
            menuFriends->addMenu(presenceViewController->menu());

            if (!mUnderage)
            {
                menuHelp->addAction(actionOrigin_Help);
                setActionShortcut(actionOrigin_Help, QKeySequence::HelpContents);
                menuHelp->addAction(actionOrigin_Forum);
            }
            menuHelp->addAction(actionOER);
            menuHelp->addSeparator();
            menuHelp->addAction(actionAbout);

            // Connect the actions to our signals
            ORIGIN_VERIFY_CONNECT(actionExit, SIGNAL(triggered()), this, SIGNAL(menuActionExit()));
            ORIGIN_VERIFY_CONNECT(actionRedeem_Product_Code, SIGNAL(triggered()), this, SIGNAL(menuActionRedeemProductCode()));
            ORIGIN_VERIFY_CONNECT(actionReload_SPA, SIGNAL(triggered()), this, SIGNAL(menuActionReloadSPA()));
            ORIGIN_VERIFY_CONNECT(actionReload_My_Games, SIGNAL(triggered()), this, SIGNAL(menuActionReloadMyGames()));
            ORIGIN_VERIFY_CONNECT(actionApplication_Settings, SIGNAL(triggered()), this, SIGNAL(menuActionApplicationSettings()));
            ORIGIN_VERIFY_CONNECT(actionAccount, SIGNAL(triggered()), this, SIGNAL(menuActionAccount()));
            ORIGIN_VERIFY_CONNECT(actionGo_Offline, SIGNAL(triggered()), this, SIGNAL(menuActionToggleOfflineOnline()));
            ORIGIN_VERIFY_CONNECT(actionLog_Out, SIGNAL(triggered()), this, SIGNAL(menuActionLogOut()));
            ORIGIN_VERIFY_CONNECT(actionAdd_a_Friend, SIGNAL(triggered()), this, SIGNAL(menuActionAddFriend()));
            ORIGIN_VERIFY_CONNECT(actionOrigin_Help, SIGNAL(triggered()), this, SIGNAL(menuActionOriginHelp()));
            ORIGIN_VERIFY_CONNECT(actionOrigin_Forum, SIGNAL(triggered()), this, SIGNAL(menuActionOriginForum()));
            ORIGIN_VERIFY_CONNECT(actionViewProfile, SIGNAL(triggered()), this, SIGNAL(menuActionViewProfile()));
            ORIGIN_VERIFY_CONNECT(actionOrder_History, SIGNAL(triggered()), this, SIGNAL(menuActionOrderHistory()));
            ORIGIN_VERIFY_CONNECT(actionOER, SIGNAL(triggered()), this, SIGNAL(menuActionOER()));
            ORIGIN_VERIFY_CONNECT(actionAbout, SIGNAL(triggered()), this, SIGNAL(menuActionAbout()));
            ORIGIN_VERIFY_CONNECT(actionAddNonOriginGame, SIGNAL(triggered()), this, SIGNAL(menuActionAddNonOriginGame()));

            updateMenuText();
        }

        void MainMenuControllerWin::retranslate()
        {
            updateMenuText();
        }

        void MainMenuControllerWin::updateMenuText()
        {
            actionExit->setText(tr("ebisu_mainmenuitem_exit"));
            actionRedeem_Product_Code->setText(tr("ebisu_mainmenuitem_redeem_game_code"));
            actionReload_My_Games->setText(tr("ebisu_mainmenuitem_refresh_my_games"));
            actionReload_SPA->setText(tr("ebisu_client_menuitem_refresh"));
            actionApplication_Settings->setText(tr("ebisu_client_application_settings"));
            actionAccount->setText(tr("ebisu_client_account_privacy"));
            actionLog_Out->setText(tr("ebisu_mainmenuitem_logout"));
            actionAdd_a_Friend->setText(tr("ebisu_mainmenuitem_add_a_friend"));
            actionOrigin_Help->setText(QString(tr("ebisu_mainmenuitem_client_help")).arg(tr("application_name")));
            actionOrigin_Forum->setText(QString(tr("ebisu_mainmenuitem_forum")).arg(tr("application_name")));
            actionViewProfile->setText(tr("ebisu_client_action_profile"));
            actionOrder_History->setText(tr("ebisu_mainmenuitem_order_history"));
            actionAbout->setText(tr("ebisu_mainmenuitem_client_about"));
            actionAddNonOriginGame->setText(tr("ebisu_client_add_non_origin_game"));
            actionOER->setText(tr("diag_tool_form_title"));
            menuFile->setTitle(tr("application_name"));
            menuFriends->setTitle(tr("ebisu_mainmenuitem_friends"));
            menuGames->setTitle(tr("ebisu_mainmenuitem_games"));
            menuHelp->setTitle(tr("ebisu_mainmenuitem_help"));


            if (mOnline)
            {
                // Online
                actionGo_Offline->setText(tr("ebisu_mainmenuitem_go_offline"));
            }
            else
            {
                // Offline
                actionGo_Offline->setText(tr("ebisu_mainmenuitem_go_online"));
            }

#ifdef ORIGIN_X
            actionAdd_a_Friend->setEnabled (false);
            actionViewProfile->setEnabled(false);
#else

            // Enable / Disable based on connection status
            actionAdd_a_Friend->setEnabled(mOnline);
            actionViewProfile->setEnabled(mOnline);
#endif
            actionRedeem_Product_Code->setEnabled(mOnline);
            actionReload_SPA->setEnabled(mOnline);
            actionReload_My_Games->setEnabled(mOnline);
            actionOrigin_Help->setEnabled(mOnline);
            actionOrigin_Forum->setEnabled(mOnline);
            actionAccount->setEnabled(mOnline);
            actionOrder_History->setEnabled(mOnline);
        }

        void MainMenuControllerWin::setOnline(bool aOnline)
        {
            MainMenuController::setOnline(aOnline);
            updateMenuText();
        }
    }
}
