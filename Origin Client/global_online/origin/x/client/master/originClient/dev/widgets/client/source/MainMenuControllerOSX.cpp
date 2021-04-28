/////////////////////////////////////////////////////////////////////////////
// MainMenuControllerOSX.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MainMenuControllerOSX.h"
#include "OriginApplicationDelegateOSX.h"

#include "services/debug/DebugService.h"
#include "engine/content/ContentController.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/OnlineContactCounter.h"
#include "engine/social/ConversationManager.h"

#include "AvailabilityMenuViewController.h"

#include <QApplication>

#include "originwindow.h"
#include "OriginMsgBox.h"
#include "ChatWindowView.h"
#include "OriginApplicationDelegateOSX.h"

//#define ENABLE_FULLSCREEN

namespace Origin
{
    namespace Client
    {
        MainMenuControllerOSX::MainMenuControllerOSX(bool underage, bool online, bool inDebug, bool developerMode, QWidget *parent)
            : MainMenuController(underage, online, inDebug, developerMode, parent)
        {
            init();
        }

        MainMenuControllerOSX::~MainMenuControllerOSX()
        {
        }

        void MainMenuControllerOSX::init()
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
            actionAbout = new QAction(NULL);
            actionAddNonOriginGame = new QAction(NULL);
            
            action_View_Profile = new QAction(NULL);
            action_View_Discover = new QAction(NULL);
            action_View_MyGames = new QAction(NULL);
            action_View_Store = new QAction(NULL);
#ifdef ENABLE_FULLSCREEN
            action_View_ToggleFullScreen = new QAction(NULL);
#endif
            action_Window_Close = new QAction(NULL);
            action_Window_Minimize = new QAction(NULL);
            action_Window_Zoom = new QAction(NULL);
            action_Window_BringAllToFront = new QAction(NULL);
            
            // Set the menu roles
            actionApplication_Settings->setMenuRole(QAction::PreferencesRole);
            actionAbout->setMenuRole(QAction::AboutRole);
            actionExit->setMenuRole(QAction::QuitRole);

            // Top level menu dropdowns

            // Use this menu to add system-known action (about, preferences, quit...) - Qt will automatically move them
            // to the internally maintained native Application menu
            menuAccount = new QMenu(mMenuBar);
            menuAccount->setObjectName("mainAccountMenu");

            menuView = new QMenu(mMenuBar);
            menuView->setObjectName("mainViewMenu");
            
            menuGames = new QMenu(mMenuBar);
            menuGames->setObjectName("mainGamesMenu");
            
            menuFriends = new QMenu(mMenuBar);
            menuFriends->setObjectName("mainFriendsMenu");
            
            menuWindow = new QMenu(mMenuBar);
            menuWindow->setObjectName("mainWindowMenu");
            
            menuHelp = new QMenu(mMenuBar);
            menuHelp->setObjectName("mainHelpMenu");

            // Social status flyout menu


            // Add the top-level menu dropdowns to the Menubar
            mMenuBar->addAction(menuAccount->menuAction());
            mMenuBar->addAction(menuView->menuAction());
            mMenuBar->addAction(menuGames->menuAction());
            if (!mUnderage) mMenuBar->addAction(menuFriends->menuAction());
            mMenuBar->addAction(menuWindow->menuAction());
            if (!mUnderage) mMenuBar->addAction(menuHelp->menuAction());

            // Debug menu goes here if enabled
            insertDebugMenu();

            // Submenu items "Application" (but attached to "Account" and internally moved to internally-maintained Application menu)
            menuAccount->addAction(actionApplication_Settings);
            setActionShortcut(actionApplication_Settings, QKeySequence(Qt::CTRL + Qt::Key_Comma));
            menuAccount->addAction(actionAbout);
            menuAccount->addAction(actionExit);
            setActionShortcut(actionExit, QKeySequence(Qt::CTRL + Qt::Key_Q));

            // Submenu items "Account"
            if (!mUnderage) menuAccount->addAction(actionAccount);
            menuAccount->addSeparator();
            menuAccount->addAction(actionGo_Offline);
            menuAccount->addAction(actionLog_Out);

            // Submenu items "View"
            menuView->addAction(actionReload_SPA);
            setActionShortcut(actionReload_SPA, QKeySequence(Qt::CTRL + Qt::Key_R));
            menuView->addSeparator();
            if (!mUnderage) menuView->addAction(action_View_Discover);
            if (!mUnderage) menuView->addAction(action_View_Store);
            menuView->addAction(action_View_MyGames);
            if (!mUnderage) menuView->addAction(action_View_Profile);
            if (!mUnderage) menuView->addAction(actionOrder_History);

            
#ifdef ENABLE_FULLSCREEN
            menuView->addSeparator();
            menuView->addAction(action_View_ToggleFullScreen);

            // Need to determine our current fullscreen mode when the menu is clicked
            ORIGIN_VERIFY_CONNECT(menuView, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowViewMenu()));
#endif
            // Submenu items "Games"
            menuGames->addAction(actionRedeem_Product_Code);
            menuGames->addAction(actionAddNonOriginGame);
            menuGames->addAction(actionReload_My_Games);
            
            // Create AvailabilityMenuViewController
            Engine::Social::SocialController *socialController = Engine::LoginController::currentUser()->socialControllerInstance();
            AvailabilityMenuViewController *presenceViewController = new AvailabilityMenuViewController(socialController, mMenuBar);

            // Submenu items "Friends"
            menuFriends->addAction(actionAdd_a_Friend);
            menuFriends->addSeparator();
            menuFriends->addMenu(presenceViewController->menu());

            // Submenu items "Window"
            menuWindow->addAction(action_Window_Close);
            menuWindow->addAction(action_Window_Minimize);
            menuWindow->addAction(action_Window_Zoom);
            menuWindow->addSeparator();
            menuWindow->addAction(action_Window_BringAllToFront);
            menuWindow->addSeparator();
            ORIGIN_VERIFY_CONNECT(menuWindow, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowWindowMenu()));

            // Window items get added dynamically here when the menu is clicked
            connect(menuWindow, SIGNAL(triggered(QAction*)), this, SLOT(onActivateWindow(QAction*)));
            
            // Submenu items "Help"
            menuHelp->addAction(actionOrigin_Help);
            menuHelp->addAction(actionOrigin_Forum);
            menuHelp->addSeparator();
            

            // Set the shortcuts
#ifdef ENABLE_FULLSCREEN
            action_View_ToggleFullScreen->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_F);
#endif
            action_Window_Close->setShortcut(Qt::CTRL+Qt::Key_W);
            action_Window_Minimize->setShortcut(Qt::CTRL+Qt::Key_M);

            // Connect the actions to our signals
            ORIGIN_VERIFY_CONNECT(actionExit, SIGNAL(triggered()), this, SIGNAL(menuActionExit()));
            ORIGIN_VERIFY_CONNECT(actionReload_My_Games, SIGNAL(triggered()), this, SIGNAL(menuActionReloadMyGames()));
            ORIGIN_VERIFY_CONNECT(actionRedeem_Product_Code, SIGNAL(triggered()), this, SIGNAL(menuActionRedeemProductCode()));
            ORIGIN_VERIFY_CONNECT(actionReload_SPA, SIGNAL(triggered()), this, SIGNAL(menuActionReloadSPA()));
            ORIGIN_VERIFY_CONNECT(actionApplication_Settings, SIGNAL(triggered()), this, SIGNAL(menuActionApplicationSettings()));
            ORIGIN_VERIFY_CONNECT(actionAccount, SIGNAL(triggered()), this, SIGNAL(menuActionAccount()));
            ORIGIN_VERIFY_CONNECT(actionGo_Offline, SIGNAL(triggered()), this, SIGNAL(menuActionToggleOfflineOnline()));
            ORIGIN_VERIFY_CONNECT(actionLog_Out, SIGNAL(triggered()), this, SIGNAL(menuActionLogOut()));
            ORIGIN_VERIFY_CONNECT(actionAdd_a_Friend, SIGNAL(triggered()), this, SIGNAL(menuActionAddFriend()));
            ORIGIN_VERIFY_CONNECT(actionOrigin_Help, SIGNAL(triggered()), this, SIGNAL(menuActionOriginHelp()));
            ORIGIN_VERIFY_CONNECT(actionOrigin_Forum, SIGNAL(triggered()), this, SIGNAL(menuActionOriginForum()));
            ORIGIN_VERIFY_CONNECT(actionOrder_History, SIGNAL(triggered()), this, SIGNAL(menuActionOrderHistory()));
            ORIGIN_VERIFY_CONNECT(actionAbout, SIGNAL(triggered()), this, SIGNAL(menuActionAbout()));
            ORIGIN_VERIFY_CONNECT(actionAddNonOriginGame, SIGNAL(triggered()), this, SIGNAL(menuActionAddNonOriginGame()));
            ORIGIN_VERIFY_CONNECT(action_View_Discover, SIGNAL(triggered()), this, SIGNAL(menuActionDiscover()));
            ORIGIN_VERIFY_CONNECT(action_View_MyGames, SIGNAL(triggered()), this, SIGNAL(menuActionMyGames()));
            ORIGIN_VERIFY_CONNECT(action_View_Profile, SIGNAL(triggered()), this, SIGNAL(menuActionViewProfile()));
            
            ORIGIN_VERIFY_CONNECT(action_View_Store, SIGNAL(triggered()), this, SIGNAL(menuActionStore()));
#ifdef ENABLE_FULLSCREEN
            ORIGIN_VERIFY_CONNECT(action_View_ToggleFullScreen, SIGNAL(triggered()), this, SIGNAL(menuActionToggleFullScreen()));
#endif
            // Windowing signals
            ORIGIN_VERIFY_CONNECT(action_Window_Close, SIGNAL(triggered()), this, SLOT(onMenuActionClose()));
            ORIGIN_VERIFY_CONNECT(action_Window_Zoom, SIGNAL(triggered()), this, SLOT(onMenuActionZoom()));
            ORIGIN_VERIFY_CONNECT(action_Window_Minimize, SIGNAL(triggered()), this, SLOT(onMenuActionMinimize()));
            ORIGIN_VERIFY_CONNECT(action_Window_BringAllToFront, SIGNAL(triggered()), this, SLOT(onMenuActionBringAllToFront()));
            
            updateMenuText();
        }

        void MainMenuControllerOSX::retranslate()
        {
            updateMenuText();
        }

        void MainMenuControllerOSX::updateMenuText()
        {
            actionExit->setText(tr("ebisu_mainmenuitem_exit"));
            actionRedeem_Product_Code->setText(tr("ebisu_mainmenuitem_redeem_game_code"));
            actionReload_My_Games->setText(tr("ebisu_mainmenuitem_refresh_my_games"));
            actionReload_SPA->setText(tr("ebisu_client_menuitem_refresh"));
            actionApplication_Settings->setText(tr("ebisu_client_application_settings"));
            action_View_Profile->setText(tr("ebisu_client_action_profile"));
            actionAccount->setText(tr("ebisu_client_account_privacy"));
            actionLog_Out->setText(tr("ebisu_mainmenuitem_logout"));
            actionAdd_a_Friend->setText(tr("ebisu_mainmenuitem_add_a_friend"));
            actionOrigin_Help->setText(QString(tr("ebisu_mainmenuitem_client_help")).arg(tr("application_name")));
            actionOrigin_Forum->setText(QString(tr("ebisu_mainmenuitem_forum")).arg(tr("application_name")));
            actionOrder_History->setText(tr("ebisu_mainmenuitem_order_history"));
            actionAbout->setText(tr("ebisu_mainmenuitem_client_about"));
            actionAddNonOriginGame->setText(tr("ebisu_client_add_non_origin_game"));
            action_View_Discover->setText(tr("ebisu_client_discover"));
            action_View_MyGames->setText(tr("ebisu_mainmenuitem_my_games"));
            action_View_Store->setText(tr("ebisu_mainmenuitem_store"));
#ifdef ENABLE_FULLSCREEN
            action_View_ToggleFullScreen->setText(tr("ebisu_mainmenuitem_enter_full_screen")); // gets updated dynamically when the view menu is popped
#endif
            menuAccount->setTitle(tr("ebisu_mainmenuitem_account"));
            menuView->setTitle(tr("ebisu_mainmenuitem_view"));
            menuGames->setTitle(tr("ebisu_mainmenuitem_games"));
            menuFriends->setTitle(tr("ebisu_mainmenuitem_friends"));
            menuWindow->setTitle(tr("ebisu_mainmenuitem_window"));
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

            action_Window_Close->setText(tr("ebisu_mainmenuitem_close"));
            action_Window_Minimize->setText(tr("ebisu_mainmenuitem_minimize"));
            action_Window_Zoom->setText(tr("ebisu_mainmenuitem_zoom"));
            action_Window_BringAllToFront->setText(tr("ebisu_mainmenuitem_bring_to_front"));

#ifdef ORIGIN_X
            //disable everything but logout, exit, go online/offline
            actionAdd_a_Friend->setEnabled (false);
            action_View_Profile->setEnabled(false);
#endif
            // Enable / Disable based on connection status
            actionRedeem_Product_Code->setEnabled(mOnline);
            actionReload_SPA->setEnabled(mOnline);
            actionReload_My_Games->setEnabled(mOnline);
            actionOrigin_Help->setEnabled(mOnline);
            actionOrigin_Forum->setEnabled(mOnline);
            actionAccount->setEnabled(mOnline);
            action_View_Store->setEnabled (mOnline);
            actionOrder_History->setEnabled(mOnline);
            action_View_Discover->setEnabled (mOnline);
        }
        
        void MainMenuControllerOSX::clearWindowMenu()
        {
            foreach (QAction *action, windowMenuWindowActions)
            {
                menuWindow->removeAction(action);
            }
            
            windowMenuWindowActions.clear();
        }
        
        void MainMenuControllerOSX::updateWindowMenu()
        {
            // Remove previous menu items
            clearWindowMenu();

            QWidget *activeWindow = QApplication::activeWindow();

            // Find windows and add them to the menu
            foreach (QWidget *widget, QApplication::topLevelWidgets())
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                if (window && window->isVisible())
                {
                    QString title = window->decoratedWindowTitle();
                    if (!title.isEmpty())
                    {
                        QAction* action = new QAction(NULL);
                        action->setText(title);
                        action->setCheckable(true);

                        if (window == activeWindow)
                        {
                            action->setChecked(true);
                        }

                        else
                        if (window->isMinimized())
                        {
                            // Show diamond marker for minimized window
                            action->setChecked(true);
                            action->setUseDiamondMarker(true);
                        }
                        
                        menuWindow->addAction(action);
                        windowMenuWindowActions.append(action);
                    }
                }
            }

            // If we have no active window disable all Window options as they have no functionality
            if (activeWindow == 0)
            {
                action_Window_Close->setEnabled(false);
                action_Window_Zoom->setEnabled(false);
                action_Window_Minimize->setEnabled(false);

                // We check for all topLevel Widget to determine if we have any minimized windows
                // This way we give the user an option to open all minimized windows
                bool minimized = false;
                foreach (QWidget* widget, QApplication::topLevelWidgets())
                {
                    UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                    if (window && window->isVisible())
                    {
                        minimized = window->isMinimized();
                    }
                }
                action_Window_BringAllToFront->setEnabled(minimized);
            }
            else
            {
                UIToolkit::OriginWindow* activeOriginWindow = dynamic_cast<UIToolkit::OriginWindow*>(activeWindow);
                // Check to make sure we have a valid pointer
                // This fixes EBIBUGS-20512 but causes a condition where no Origin window appears to have focus
                // until the notification disappears
                if (activeOriginWindow)
                {
                    UIToolkit::OriginWindow::TitlebarItems items = activeOriginWindow->titlebarItems();
                    action_Window_Close->setEnabled(items & UIToolkit::OriginWindow::Close);
                    action_Window_Zoom->setEnabled(items & UIToolkit::OriginWindow::Maximize);
                    action_Window_BringAllToFront->setEnabled(true);
                    action_Window_Minimize->setEnabled(items & UIToolkit::OriginWindow::Minimize);
                }
                // Since we have no activeOriginWindow diable some of the menu options
                else
                {
                    action_Window_Close->setEnabled(false);
                    action_Window_Zoom->setEnabled(false);
                    action_Window_Minimize->setEnabled(false);
                }
            }

#ifdef ENABLE_FULLSCREEN
            // Enable/disable items based on the active window's titlebar options
            UIToolkit::OriginWindow* activeOriginWindow = dynamic_cast<UIToolkit::OriginWindow*>(activeWindow);
            if (activeOriginWindow)
            {
                UIToolkit::OriginWindow::TitlebarItems items = activeOriginWindow->titlebarItems();
                
                if (activeOriginWindow->isFullscreen())
                {
                    // Disable Close, Minimize, and Zoom if the current window is fullscreen
                    action_Window_Close->setEnabled(activeOriginWindow->isFullscreen() ? false : true);
                    action_Window_Minimize->setEnabled(activeOriginWindow->isFullscreen() ? false : true);
                    action_Window_Zoom->setEnabled(activeOriginWindow->isFullscreen() ? false : true);
                }
                else
                {
                    action_Window_Close->setEnabled(items & UIToolkit::OriginWindow::Close);
                    action_Window_Minimize->setEnabled(items & UIToolkit::OriginWindow::Minimize);
                    action_Window_Zoom->setEnabled(items & UIToolkit::OriginWindow::Maximize); // Interpret "Maximize" as "Zoom" here
                }
            }
#endif
        }

        void MainMenuControllerOSX::setOnline(bool aOnline)
        {
            MainMenuController::setOnline(aOnline);
            updateMenuText();
        }

        void MainMenuControllerOSX::setEnabledHomeTab(bool aShow)
        {
            MainMenuController::setEnabledHomeTab(aShow);
        }
        
        void MainMenuControllerOSX::setFullscreenSupported(bool aSupported)
        {
#ifdef ENABLE_FULLSCREEN
            action_View_ToggleFullScreen->setEnabled(aSupported);
#endif
        }

        void MainMenuControllerOSX::onAboutToShowViewMenu()
        {
        }
        
        void MainMenuControllerOSX::onAboutToShowWindowMenu()
        {
            updateWindowMenu();
        }

        void MainMenuControllerOSX::onMenuActionClose()
        {
            QWidget *activeWindow = QApplication::activeWindow();
            if (activeWindow)
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(activeWindow);
                if (window)
                {
                    if (auto *chatWindow = dynamic_cast<ChatWindowView*>(window->content()))
                    {
                        // Command-W is overridden to mean "close tab" for tabbed windows
                        // Currently only the chat window is tabbed
                        chatWindow->closeActiveConversation();
                    }
                    else
                    {
                        window->close();
                    }
                }
            }
        }
        
        void MainMenuControllerOSX::onMenuActionZoom()
        {
            QWidget *activeWindow = QApplication::activeWindow();
            if (activeWindow)
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(activeWindow);
                if (window) window->showZoomed();
            }
        }
        
        void MainMenuControllerOSX::onMenuActionMinimize()
        {
            QWidget *activeWindow = QApplication::activeWindow();
            if (activeWindow)
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(activeWindow);
                if (window) window->showMinimized();
            }
        }
        
        void MainMenuControllerOSX::onMenuActionBringAllToFront()
        {
            UIToolkit::OriginWindow* window;
            
#ifdef ENABLE_FULLSCREEN
            // Apparently we're not supposed to do anything if we're in fullscreen
            // since only our main client window supports fullscreen
            bool isFullscreen = false;
            foreach (QWidget *widget, QApplication::topLevelWidgets())
            {
                window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                if (window && window->isVisible() && window->isFullscreen())
                {
                    isFullscreen = true;
                    break;
                }
            }

            if (!isFullscreen)
            {
                QWidgetList widgets = GetOrderedTopLevelWidgets();
                foreach (QWidget *widget, widgets)
                {
                    window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                    if (window && window->isVisible())
                    {
                        if (window->isMinimized())
                            window->showNormal();
                        else
                            window->raise();
                    }
                }
            }
#endif
#ifndef ENABLE_FULLSCREEN
            // We still need this function to do something even if we are not enableing full screen
            QWidgetList widgets = GetOrderedTopLevelWidgets();
            foreach (QWidget *widget, widgets)
            {
                window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                if (window && window->isVisible())
                {
                    if (window->isMinimized())
                        window->showNormal();
                    
                    else
                        window->raise();
                }
            }
#endif
        }
        
        void MainMenuControllerOSX::onMainWindowFullscreenChanged(bool isFullscreen)
        {
#ifdef ENABLE_FULLSCREEN
            if (isFullscreen)
            {
                action_View_ToggleFullScreen->setText(tr("ebisu_mainmenuitem_exit_full_screen"));
            }
            else
            {
                action_View_ToggleFullScreen->setText(tr("ebisu_mainmenuitem_enter_full_screen"));
            }
#endif
        }
    
        void MainMenuControllerOSX::onActivateWindow(QAction* action)
        {
            if (action)
            {
                // See if this action is in our list of window actions
                int index = windowMenuWindowActions.indexOf(action);
                if (index >= 0)
                {
                    // Search for a window with the correct title
                    foreach (QWidget *widget, QApplication::topLevelWidgets())
                    {
                        UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                        if (window && window->isVisible() && (action->text() == window->decoratedWindowTitle()))
                        {
                            // Activate it
                            window->raise();
                            window->activateWindow();
                            return;
                        }
                    }
                    
                }
            }
        }
        
        // Helper to trigger regular NSApplication quit behavior
        static void quitApplicationHelper()
        {
            triggerAppTermination();
        }
        
        QMenuBar* MainMenuControllerOSX::getBasicMenu(QWidget* parent)
        {
            QMenuBar* menubar = MainMenuController::GetAppMenuBar();
            
            QMenu* menuApplication = new QMenu(menubar);
            menuApplication->setObjectName("mainOriginMenu");
            
            menubar->addAction(menuApplication->menuAction());
            
            QAction* actionApplication_Settings = new QAction(NULL);
            QAction* actionApplication_About = new QAction(NULL);
            
            // We also need to redefine the default Quit behavior because Qt menus for Mac don't clean up correctly - after logging out once,
            // you wouldn't be able to quit from the logging window
            QAction* actionApplication_Exit = new QAction(NULL);
            
            actionApplication_Settings->setMenuRole(QAction::PreferencesRole);
            actionApplication_About->setMenuRole(QAction::AboutRole);
            actionApplication_Exit->setMenuRole(QAction::QuitRole);
            
            menuApplication->addAction(actionApplication_Settings);
            menuApplication->addAction(actionApplication_About);
            menuApplication->addAction(actionApplication_Exit);
            
            actionApplication_Settings->setText(tr("ebisu_client_application_settings"));
            actionApplication_About->setText(tr("ebisu_mainmenuitem_client_about"));
            
            QObject::connect(actionApplication_Exit, &QAction::triggered, quitApplicationHelper);
            
            actionApplication_Settings->setDisabled(true);
            actionApplication_About->setDisabled(true);
            
            return menubar;
        }
    }
}
