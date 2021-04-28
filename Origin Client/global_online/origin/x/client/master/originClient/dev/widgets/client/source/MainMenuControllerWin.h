/////////////////////////////////////////////////////////////////////////////
// MainMenuControllerWin.h
//
// Copyright (c) 2012-2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_MENU_CONTROLLER_WIN_H
#define MAIN_MENU_CONTROLLER_WIN_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QMenuBar>

// LEGACY APP
#include "MainMenuController.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API MainMenuControllerWin : public MainMenuController
        {
            Q_OBJECT

        public:

            MainMenuControllerWin(bool underage, bool online, bool inDebug, bool developerMode, QWidget *parent = 0);
            ~MainMenuControllerWin();

            virtual void setOnline(bool aOnline);

        protected:
            void init();
            void retranslate();
            void updateMenuText();

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
            void menuActionViewProfile();
            void menuActionAbout();
            void menuActionAddNonOriginGame();
            void menuActionOER();

        private:
            QMenu *menuFile;
            QMenu *menuFriends;
            QMenu *menuGames;
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
            QAction *actionOER;
            QAction *actionViewProfile;

        };
    }
}

#endif

