/////////////////////////////////////////////////////////////////////////////
// MainMenuController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_MENU_CONTROLLER_H
#define MAIN_MENU_CONTROLLER_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QMenuBar>

#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class MenuFlowWindowViewController;
        class MenuNotificationViewController;

        class ORIGIN_PLUGIN_API MainMenuController : public QObject
        {
            Q_OBJECT

        public:

            MainMenuController(bool underage, bool online, bool inDebug, bool developerMode, QWidget *parent = 0);
            ~MainMenuController();

            virtual void setOnline(bool aOnline);
            virtual void setEnabledHomeTab(bool aShow) { mHomeTabEnabled = aShow; }

            QMenuBar* menuBar() { return mMenuBar; }

        protected:
            void init();
            void retranslate();
            void updateMenuText();
            void insertDebugMenu();
            void setActionShortcut(QAction* action, const QKeySequence& shortcut);

        signals:
            // Debug menu signals
            void menuActionDEBUGCheckforUpdates();
            void menuActionDEBUGtestDirtyBits();
            void menuActionDEBUGtestDirtyBitsFakeMessages();
            void menuActionDEBUGsetWebSocketVerbosity();
            void menuActionDEBUGtestDirtyBitsConnectHandlers();
            void menuActionDEBUGtestDirtyBitsFakeMessageFromFile();            
            void menuActionDEBUGshowDirtyBitsTrafficDebugViewer();

            void menuActionDEBUGOriginDeveloperTool();
            void menuActionDEBUGRevertToDefault();
            void menuActionDEBUGCdnOverride();
            void menuActionDEBUGDisableEmbargoMode();
			void menuActionDEBUGOtHSettingClear();
            void menuActionDEBUGIGOExtraLogging(bool checked);
            void menuActionDEBUGIGOWebInspectors(bool checked);
            
            void menuActionDEBUGIncrementalRefresh();

            void menuActionDEBUGJoinPendingGroups();


        protected slots:
            void onGamePlayStarted(Origin::Engine::Content::EntitlementRef);
            void onGamePlayFinshed(Origin::Engine::Content::EntitlementRef);
            void redeemBF4();
            void redeemCnC();
            void redeemFifa15();
            void redeemSims4();
            void redeemME3();
            void revokeBF4();
            void revokeCnC();
            void revokeFifa15();
            void revokeSims4();
            void revokeME3();

        protected:
            static bool isInGame();

        protected:
#ifdef ORIGIN_MAC
            static QMenuBar* GetAppMenuBar();
#endif
            QMenuBar* mMenuBar;
            QAction *actionGo_Offline;

            bool mOnline;
            bool mUnderage;
            const bool mInDebug;
            bool mODTEnabled;
            bool mHomeTabEnabled;

        private:
            // Debug menu
            QMenu *menuDebug;
            MenuFlowWindowViewController* menuFlowWindows;
            MenuNotificationViewController* menuToasts;

            // Debug menu actions
            QAction *action_Debug_Check_for_Origin_Updates;
            QAction *action_Debug_Origin_Developer_Tool;
            QAction *action_Debug_Revert_to_Default;
            QAction *action_Debug_Test_DirtyBits;
            QAction *action_Debug_Test_DirtyBitsConnectHandlers;
            QAction *action_Debug_Test_DirtyBitsFakeMessageFromFile;

            QAction *action_Debug_Show_DirtyBitsViewer;
            QAction *action_Debug_Test_DirtyBitsFakeMessages;
            QAction *action_Debug_Test_DirtyBitsSocketVerbosity;
            QAction *action_Debug_Cdn_Override;
            QAction *action_Debug_Incremental_Refresh;
            QAction *action_Debug_Crash;
            QAction *action_Debug_Crash_Recursive;
            QAction *action_Debug_Disable_Embargo_Mode;
			QAction *action_Debug_OtH_Setting_Clear;
            QAction *action_Debug_IGO_Extra_Logging;
            QAction *action_Debug_IGO_Web_Inspectors;

            QAction *action_Debug_Join_Pending_Groups;

            QMenu *subscriptionMenu;

        private slots:
            void crash();           ///< Force a crash
            void crashRecursive();  ///< Force a stack overflow crash
        };
    }
}

#endif
