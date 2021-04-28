/////////////////////////////////////////////////////////////////////////////
// MainMenuController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MainMenuController.h"

#include <QAction>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "engine/content/ContentController.h"
#include "MenuFlowWindowViewController.h"
#include "MenuNotificationViewController.h"
#include "engine/subscription/SubscriptionManager.h"

namespace Origin
{
    namespace Client
    {
        MainMenuController::MainMenuController(bool underage, bool online, bool inDebug, bool developerMode, QWidget *parent)
            : QObject(parent)
            , mMenuBar(NULL)
            , actionGo_Offline(NULL)
            , mOnline(online)
            , mUnderage(underage)
            , mInDebug(inDebug)
            , mODTEnabled(developerMode)
            , mHomeTabEnabled(false)
            , menuDebug(NULL)
        {
            init();
        }

        MainMenuController::~MainMenuController()
        {
#ifndef ORIGIN_MAC
            // If attached to a parent the parent will delete the menubar object
            if (mMenuBar->parent() == NULL)
            {
                mMenuBar->deleteLater();
            }
#endif
        }

        void MainMenuController::init()
        {
#ifdef ORIGIN_MAC
            mMenuBar = GetAppMenuBar();
#else
            mMenuBar = new QMenuBar(NULL);
#endif
            mMenuBar->setObjectName("mainMenuBar");
            mMenuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            mMenuBar->setContextMenuPolicy(Qt::PreventContextMenu);

            //
            // Create debug menu
            //

            if(mInDebug || mODTEnabled)
            {
                menuDebug = new QMenu(mMenuBar);
                menuDebug->setObjectName("mainDebugMenu");
                menuFlowWindows = new MenuFlowWindowViewController(menuDebug);
                menuToasts = new MenuNotificationViewController(menuDebug);
            }

            //
            // Create debug menu actions
            //

            if(mInDebug)
            {
                action_Debug_Check_for_Origin_Updates = new QAction(NULL);
                action_Debug_Test_DirtyBits = new QAction(NULL);
                action_Debug_Test_DirtyBitsFakeMessages = new QAction(NULL);
                action_Debug_Test_DirtyBitsSocketVerbosity = new QAction(NULL);
                action_Debug_Test_DirtyBitsConnectHandlers = new QAction(NULL);
                action_Debug_Test_DirtyBitsFakeMessageFromFile = new QAction(NULL);
                action_Debug_Show_DirtyBitsViewer = new QAction(NULL);
                action_Debug_Cdn_Override = new QAction(NULL);
                action_Debug_Incremental_Refresh = new QAction(NULL);
                action_Debug_Crash = new QAction(NULL);
                action_Debug_Crash_Recursive = new QAction(NULL);
                action_Debug_Disable_Embargo_Mode = new QAction(NULL);
				action_Debug_OtH_Setting_Clear = new QAction(NULL);
                action_Debug_IGO_Extra_Logging = new QAction(NULL);
                action_Debug_IGO_Extra_Logging->setCheckable(true);
                action_Debug_Join_Pending_Groups = new QAction(NULL);
            }

            if (mInDebug || mODTEnabled)
            {
                action_Debug_Revert_to_Default = new QAction(NULL);
            }

            if(mODTEnabled)
            {
                action_Debug_Origin_Developer_Tool = new QAction(NULL);
                action_Debug_IGO_Web_Inspectors = new QAction(NULL);
                action_Debug_IGO_Web_Inspectors->setCheckable(true);
            }

            //
            // Add debug menu actions to the debug menu
            //

            if(mInDebug)
            {
                menuDebug->addAction(action_Debug_Incremental_Refresh);
                menuDebug->addAction(action_Debug_Check_for_Origin_Updates);
                menuDebug->addSeparator();

                menuDebug->addAction(action_Debug_Test_DirtyBits);
                menuDebug->addAction(action_Debug_Test_DirtyBitsFakeMessages);
                menuDebug->addAction(action_Debug_Test_DirtyBitsSocketVerbosity);

                menuDebug->addAction(action_Debug_Test_DirtyBitsConnectHandlers);
                menuDebug->addAction(action_Debug_Test_DirtyBitsFakeMessageFromFile);
                menuDebug->addAction(action_Debug_Show_DirtyBitsViewer);
                menuDebug->addSeparator();
                
                menuDebug->addAction(action_Debug_Disable_Embargo_Mode);
                menuDebug->addSeparator();
            }

            if(mODTEnabled)
            {
                menuDebug->addAction(action_Debug_Origin_Developer_Tool);
            }

            if(mInDebug || mODTEnabled)
            {
                menuDebug->addAction(action_Debug_Revert_to_Default);
                menuDebug->addSeparator();
            }

            if(mInDebug)
            {
                menuDebug->addAction(action_Debug_Cdn_Override);
                menuDebug->addSeparator();
                menuDebug->addAction(action_Debug_Crash);
                menuDebug->addAction(action_Debug_Crash_Recursive);
                menuDebug->addSeparator();
                menuDebug->addMenu(menuFlowWindows->menu());
                menuDebug->addMenu(menuToasts->menu());
				menuDebug->addSeparator();
				menuDebug->addAction(action_Debug_OtH_Setting_Clear);
                menuDebug->addSeparator();
                menuDebug->addAction(action_Debug_Join_Pending_Groups);
                menuDebug->addAction(action_Debug_IGO_Extra_Logging);
            }
            
            if (mODTEnabled)
            {
                menuDebug->addAction(action_Debug_IGO_Web_Inspectors);
            }
            
            if (mInDebug)
            {
                menuDebug->addSeparator();
            }


#ifdef DEBUG
            if(mInDebug && Origin::Engine::Subscription::SubscriptionManager::instance()->enabled())
            {
                subscriptionMenu = new QMenu("Subscription", menuDebug);

                menuDebug->addMenu(subscriptionMenu);

                QAction *action = new QAction("Redeem: Battlefield 4 Premium Edition", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(redeemBF4()));

                action = new QAction("Redeem: Command and Conquer The Ultimate Edition", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(redeemCnC()));

                action = new QAction("Redeem: FIFA 15 Standard Edition", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(redeemFifa15()));

                action = new QAction("Redeem: The Sims 4", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(redeemSims4()));

                action = new QAction("Redeem: Mass Effect 3", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(redeemME3()));

                action = new QAction("Revoke: Battlefield 4 Premium Edition", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(revokeBF4()));

                action = new QAction("Revoke: Command and Conquer The Ultimate Edition", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(revokeCnC()));

                action = new QAction("Revoke: FIFA 15 Standard Edition", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(revokeFifa15()));

                action = new QAction("Revoke: The Sims 4", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(revokeSims4()));

                action = new QAction("Revoke: Mass Effect 3", NULL);
                subscriptionMenu->addAction(action);
                ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(revokeME3()));
            }
#endif

            //
            // Connect debug menu signals
            //

            if(mInDebug)
            {
                ORIGIN_VERIFY_CONNECT(action_Debug_Check_for_Origin_Updates, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGCheckforUpdates()));

                ORIGIN_VERIFY_CONNECT(action_Debug_Test_DirtyBits, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGtestDirtyBits()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Test_DirtyBitsFakeMessages, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGtestDirtyBitsFakeMessages()));

                ORIGIN_VERIFY_CONNECT(action_Debug_Test_DirtyBitsSocketVerbosity, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGsetWebSocketVerbosity()));
                

                ORIGIN_VERIFY_CONNECT(action_Debug_Test_DirtyBitsConnectHandlers, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGtestDirtyBitsConnectHandlers()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Test_DirtyBitsFakeMessageFromFile, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGtestDirtyBitsFakeMessageFromFile()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Show_DirtyBitsViewer, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGshowDirtyBitsTrafficDebugViewer()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Cdn_Override, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGCdnOverride()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Incremental_Refresh, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGIncrementalRefresh()));
				ORIGIN_VERIFY_CONNECT(action_Debug_OtH_Setting_Clear, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGOtHSettingClear()));
                
                ORIGIN_VERIFY_CONNECT(action_Debug_Crash, SIGNAL(triggered()), this, SLOT(crash()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Crash_Recursive, SIGNAL(triggered()), this, SLOT(crashRecursive()));
                ORIGIN_VERIFY_CONNECT(action_Debug_Disable_Embargo_Mode, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGDisableEmbargoMode()));
                
                ORIGIN_VERIFY_CONNECT(action_Debug_IGO_Extra_Logging, SIGNAL(toggled(bool)), this, SIGNAL(menuActionDEBUGIGOExtraLogging(bool)));

                ORIGIN_VERIFY_CONNECT(action_Debug_Join_Pending_Groups, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGJoinPendingGroups()));
            }

            if(mInDebug || mODTEnabled)
            {
                ORIGIN_VERIFY_CONNECT(action_Debug_Revert_to_Default, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGRevertToDefault()));
            }

            if(mODTEnabled)
            {
                ORIGIN_VERIFY_CONNECT(action_Debug_Origin_Developer_Tool, SIGNAL(triggered()), this, SIGNAL(menuActionDEBUGOriginDeveloperTool()));
                ORIGIN_VERIFY_CONNECT(action_Debug_IGO_Web_Inspectors, SIGNAL(toggled(bool)), this, SIGNAL(menuActionDEBUGIGOWebInspectors(bool)));
            }

            updateMenuText();

            ORIGIN_VERIFY_CONNECT(Engine::Content::ContentController::currentUserContentController(), SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)), this, SLOT(onGamePlayStarted(Origin::Engine::Content::EntitlementRef)));
            ORIGIN_VERIFY_CONNECT(Engine::Content::ContentController::currentUserContentController(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(onGamePlayFinshed(Origin::Engine::Content::EntitlementRef)));
        }

        void MainMenuController::redeemBF4() { Origin::Engine::Subscription::SubscriptionManager::instance()->upgrade("76889"); }
        void MainMenuController::redeemCnC() { Origin::Engine::Subscription::SubscriptionManager::instance()->upgrade("185519"); }
        void MainMenuController::redeemFifa15() { Origin::Engine::Subscription::SubscriptionManager::instance()->upgrade("181837"); }
        void MainMenuController::redeemSims4() { Origin::Engine::Subscription::SubscriptionManager::instance()->upgrade("55482"); }
        void MainMenuController::redeemME3() { Origin::Engine::Subscription::SubscriptionManager::instance()->upgrade("69317"); }
        void MainMenuController::revokeBF4() { Origin::Engine::Subscription::SubscriptionManager::instance()->downgrade("76889"); }
        void MainMenuController::revokeCnC() { Origin::Engine::Subscription::SubscriptionManager::instance()->downgrade("185519"); }
        void MainMenuController::revokeFifa15() { Origin::Engine::Subscription::SubscriptionManager::instance()->downgrade("181837"); }
        void MainMenuController::revokeSims4() { Origin::Engine::Subscription::SubscriptionManager::instance()->downgrade("55482"); }
        void MainMenuController::revokeME3() { Origin::Engine::Subscription::SubscriptionManager::instance()->downgrade("69317"); }

        void MainMenuController::insertDebugMenu()
        {
            if (mInDebug || mODTEnabled)
            {
                // Add the debug menu item to the menubar
                mMenuBar->addAction(menuDebug->menuAction());
            }
        }

        void MainMenuController::setActionShortcut(QAction* action, const QKeySequence& shortcut)
        {
            action->setShortcutContext(Qt::WindowShortcut);
            action->setShortcut(shortcut);
        }

        void MainMenuController::retranslate()
        {
            updateMenuText();
        }

        void MainMenuController::updateMenuText()
        {
            if(mInDebug)
            {
                action_Debug_Incremental_Refresh->setText(tr("ebisu_client_notranslate_incremental_refresh"));
                action_Debug_Check_for_Origin_Updates->setText(tr("ebisu_client_notranslate_check_for_updates").arg(QCoreApplication::applicationName()));
                action_Debug_Test_DirtyBits->setText(tr("ebisu_client_notranslate_log_dirty_bits").arg(QCoreApplication::applicationName()));
                action_Debug_Test_DirtyBitsFakeMessages->setText(tr("ebisu_client_notranslate_fake_dirty_bits").arg(QCoreApplication::applicationName()));
                action_Debug_Test_DirtyBitsSocketVerbosity->setText("Web Socket: Toggle Verbosity");
                action_Debug_Test_DirtyBitsConnectHandlers->setText(tr("ebisu_client_notranslate_dirty_bits_connect").arg(QCoreApplication::applicationName()));
                action_Debug_Test_DirtyBitsFakeMessageFromFile->setText(tr("ebisu_client_notranslate_dirty_bits_fake_message").arg(QCoreApplication::applicationName()));
                action_Debug_Show_DirtyBitsViewer->setText(QString("%1 %2").arg(tr("ebisu_client_notranslate_open")).arg(tr("ebisu_client_notranslate_dirty_bits_viewer")));
                action_Debug_Disable_Embargo_Mode->setText(tr("ebisu_client_notranslate_disable_embargomode").arg(QCoreApplication::applicationName()));
                action_Debug_Cdn_Override->setText(tr("ebisu_client_notranslate_override_cdn"));
                action_Debug_Crash->setText(tr("ebisu_client_notranslate_crash_now"));
                action_Debug_Crash_Recursive->setText(tr("ebisu_client_notranslate_crash_now_recursive"));
				action_Debug_OtH_Setting_Clear->setText(tr("ebisu_client_notranslate_oth_setting_clear"));   
                action_Debug_Join_Pending_Groups->setText("Join all pending chat groups");
            }

            if(mODTEnabled)
            {
                action_Debug_Origin_Developer_Tool->setText(tr("ebisu_client_notranslate_developer_tool").arg(QCoreApplication::applicationName()));
            }

            if(mODTEnabled || mInDebug)
            {
                action_Debug_Revert_to_Default->setText(tr("ebisu_client_notranslate_revert_to_default"));
            }

            if(mInDebug)
            {
                action_Debug_Cdn_Override->setText(tr("ebisu_client_notranslate_override_cdn"));
                action_Debug_Crash->setText(tr("ebisu_client_notranslate_crash_now"));
                action_Debug_Crash_Recursive->setText(tr("ebisu_client_notranslate_crash_now_recursive"));
				action_Debug_OtH_Setting_Clear->setText(tr("ebisu_client_notranslate_oth_setting_clear"));
                action_Debug_IGO_Extra_Logging->setText(tr("ebisu_client_igo_extra_logging"));
                action_Debug_Revert_to_Default->setText(tr("ebisu_client_notranslate_revert_to_default"));
            }

            if (mInDebug || mODTEnabled)  menuDebug->setTitle(tr("ebisu_client_notranslate_debug"));

            // Enable / Disable based on connection status
            if(mInDebug)
            {
                action_Debug_Check_for_Origin_Updates->setEnabled(mOnline);
            }

            if(mODTEnabled)
            {
                action_Debug_Origin_Developer_Tool->setEnabled(mOnline);
                action_Debug_IGO_Web_Inspectors->setText(tr("ebisu_client_igo_web_inspectors"));
            }
        }

        void MainMenuController::setOnline(bool aOnline)
        {
            mOnline = aOnline;
            updateMenuText();
        }

        void MainMenuController::onGamePlayStarted(Origin::Engine::Content::EntitlementRef entitlement)
        {
            if (entitlement.isNull() || entitlement->contentConfiguration() == NULL)
            {
                return;
            }

            // ORSUBS-1562: Do not let users to go offline while they are playing a timed trial.
            if (entitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access
                || entitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                actionGo_Offline->setEnabled(false);
        }

        void MainMenuController::onGamePlayFinshed(Origin::Engine::Content::EntitlementRef entitlement)
        {
            if (entitlement.isNull() || entitlement->contentConfiguration() == NULL)
            {
                return;
            }

            // ORSUBS-1562: Do not let users to go offline while they are playing a timed trial.
            if (entitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access
                || entitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                actionGo_Offline->setEnabled(true);
        }

        bool MainMenuController::isInGame()
        {
            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            if (c && c->isPlaying())
            {
                return true;
            }
            return false;
        }           

        void MainMenuController::crash()
        {
            ORIGIN_LOG_ACTION << "Crashing per user request";
            QString* nullStrPtr = 0;
            nullStrPtr->clear();
        }
        
        void MainMenuController::crashRecursive()
        {
            static bool logged = false;
            
            if(!logged)
            {
                logged = true;
                ORIGIN_LOG_ACTION << "Crashing per user request (recursive)";
            }

            crashRecursive();
        }
        
#ifdef ORIGIN_MAC
        QMenuBar* MainMenuController::GetAppMenuBar()
        {
            static QMenuBar* appMenuBar = NULL;
            if (!appMenuBar)
                appMenuBar = new QMenuBar(NULL);
            
            appMenuBar->clear();
            return appMenuBar;
        }
#endif
    }
}
