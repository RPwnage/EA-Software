/////////////////////////////////////////////////////////////////////////////
// MainMenuControllerOSX.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_MENU_CONTROLLER_OSX_H
#define MAIN_MENU_CONTROLLER_OSX_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QMenuBar>

#include "MainMenuController.h"

#include "chat/Presence.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
        class OriginMsgBox;
    }
    
    namespace Client
    {
        class ORIGIN_PLUGIN_API MainMenuControllerOSX : public MainMenuController
        {
            Q_OBJECT

        public:

            MainMenuControllerOSX(bool underage, bool online, bool inDebug, bool developerMode, QWidget *parent = 0);
            ~MainMenuControllerOSX();

            virtual void setOnline(bool aOnline);
            virtual void setEnabledHomeTab(bool aShow);
            void setFullscreenSupported(bool aSupported);
            
            static QMenuBar* getBasicMenu(QWidget* parent);

        protected:
            void init();
            void retranslate();
            void updateMenuText();
            void clearWindowMenu();
            void updateWindowMenu();

        signals:
            void menuActionExit();
            void menuActionRedeemProductCode();
            void menuActionReloadSPA();
            void menuActionReloadMyGames();
            void menuActionApplicationSettings();
            void menuActionAccount();
            void menuActionToggleOfflineOnline();
            void menuActionLogOut();
            void menuActionAddFriend();
            void menuActionOriginHelp();
            void menuActionOriginForum();
            void menuActionOrderHistory();
            void menuActionAbout();
            void menuActionAddNonOriginGame();
            void menuActionToggleFullScreen();
            void menuActionDiscover();
            void menuActionMyGames();
            void menuActionStore();
            void menuActionViewProfile();

        protected slots:
            void onAboutToShowViewMenu();
            void onAboutToShowWindowMenu();
            void onActivateWindow(QAction*);

        // Making these public slots in case we ever want to call these from the right-click icon menu or something
        public slots:
            void onMenuActionClose();
            void onMenuActionZoom();
            void onMenuActionMinimize();
            void onMenuActionBringAllToFront();
            
            void onMainWindowFullscreenChanged(bool isFullscreen);
            
        private:
            QMenu *menuAccount;
            QMenu *menuView;
            QMenu *menuGames;
            QMenu *menuFriends;
            QMenu *menuWindow;
            QMenu *menuHelp;

            QAction *actionExit;
            QAction *actionRedeem_Product_Code;
            QAction *actionReload_SPA;
            QAction *actionReload_My_Games;
            QAction *actionApplication_Settings;
            QAction *actionAccount;
            QAction *actionLog_Out;
            QAction *actionAdd_a_Friend;
            QAction *actionOrigin_Help;
            QAction *actionOrigin_Forum;
            QAction *actionOrder_History;
            QAction *actionAbout;
            QAction *actionAddNonOriginGame;

            QAction *action_View_Discover;
            QAction *action_View_MyGames;
            QAction *action_View_Store;
            QAction *action_View_ToggleFullScreen;
            QAction *action_View_Profile;

            QAction *action_Window_Close;
            QAction *action_Window_Minimize;
            QAction *action_Window_Zoom;
            QAction *action_Window_BringAllToFront;
            
            QList<QAction*> windowMenuWindowActions;
        };
    }
}

#endif

