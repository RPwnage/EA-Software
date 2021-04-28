/////////////////////////////////////////////////////////////////////////////
// LogoutExitViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "LogoutExitViewController.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "originmsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originpushbutton.h"
#include "origincheckbutton.h"

#include <QApplication>

#include "LogoutExitView.h"
#include "engine/cloudsaves/RemoteSyncManager.h"


namespace Origin
{
    using namespace Engine::CloudSaves;
    using namespace UIToolkit;
	namespace Client
	{

		LogoutExitViewController::LogoutExitViewController()
            :mLogoutExitView (NULL)
            ,mCloudSyncProgressDialog (NULL)
		{
            init ();
		}

		LogoutExitViewController::~LogoutExitViewController()
		{
			ORIGIN_LOG_EVENT << "LogoutExitViewController deleted";

            if (mLogoutExitView)
            {
                delete mLogoutExitView;
                mLogoutExitView = NULL;
            }
		}

        void LogoutExitViewController::init ()
        {
			assert(!mLogoutExitView);
            mLogoutExitView = new LogoutExitView();
        }

        bool LogoutExitViewController::promptLogoutConfirmation()
        {
            return OriginWindow::prompt(OriginMsgBox::NoIcon,
                tr("ebisu_client_logout_uppercase"), tr("ebisu_client_logout_confirmation_message"),
                tr("ebisu_client_yes"), tr("ebisu_client_no")) == QDialog::Accepted ? true : false;
        }

        bool LogoutExitViewController::promptExitConfirmation()
        {
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::NoIcon, 
                tr("ebisu_client_close_confirm_title").arg(tr("application_name_uppercase")),
                tr("ebisu_client_close_confirmation_message").arg( tr("application_name")));
            OriginPushButton *pYesBtn = windowChrome->addButton(QDialogButtonBox::Yes);
            OriginPushButton *pNoBtn =  windowChrome->addButton(QDialogButtonBox::No);
            windowChrome->setDefaultButton(pNoBtn);
            windowChrome->manager()->setupButtonFocus();
            windowChrome->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pNoBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pYesBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));

            OriginCheckButton* doNotShow = new OriginCheckButton();
            doNotShow->setText(tr("ebisu_client_dont_show_dialog"));
            windowChrome->setCornerContent(doNotShow);

            windowChrome->manager()->setupButtonFocus();
            windowChrome->exec();

            bool isConfirmed = windowChrome->getClickedButton() == pYesBtn;
            bool isChecked = (isConfirmed) ? doNotShow->isChecked() : 0;

            windowChrome->deleteLater();

            // Save the "Don't ask again" checkbox value into settings
            Origin::Services::writeSetting(Origin::Services::SETTING_HIDEEXITCONFIRMATION, isChecked);
    
            return isConfirmed;
        }

        bool LogoutExitViewController::promptCancelCloudSavesonExit()
        {
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::NoIcon, 
                tr("ebisu_client_cloud_sync_in_progress_title"),
		        tr("ebisu_client_cloud_sync_in_progress_text"));
            OriginPushButton *pYesBtn = windowChrome->addButton(QDialogButtonBox::Yes);
            OriginPushButton *pNoBtn =  windowChrome->addButton(QDialogButtonBox::No);
            windowChrome->setDefaultButton(pNoBtn);
            windowChrome->manager()->setupButtonFocus();
            windowChrome->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pNoBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pYesBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));

            //technically it wouldn't be lastSyncFinished but when the pull finished, but it would be rare that both are going on at the same time
            ORIGIN_VERIFY_CONNECT (RemoteSyncManager::instance(), 
                           SIGNAL(lastSyncFinished()),   
                           windowChrome,
                           SLOT(close()));

            windowChrome->manager()->setupButtonFocus();
            windowChrome->exec();

            bool isConfirmed = windowChrome->getClickedButton() == pYesBtn;

            windowChrome->deleteLater();
    
            return isConfirmed;
        }

        /// dialog shows the cloud push progress but has a button allowing the user to cancel
        bool LogoutExitViewController::showCancelCloudSaves()
        {
            bool synching = false;
            if (RemoteSyncManager::instance()->syncBeingPerformed())
            {
                // get the list of cloud pushes
                RemoteSyncManager::RemoteSyncList *syncList = RemoteSyncManager::instance()->remoteSyncInProgressList ();
                if (!syncList->isEmpty ())
                {
                    //take the first one in the list
                    RemoteSyncManager::RemoteSyncList::const_iterator it = syncList->constBegin();
                    RemoteSync* syncContent = it.value();
                    if (syncContent)
                    {
                        Origin::Engine::Content::EntitlementRef entRef = syncContent->entitlement();
                        if (!entRef.isNull())
                        {
                            mCloudSyncProgressDialog = mLogoutExitView->createCloudSyncProgressDialog (entRef->contentConfiguration()->displayName(), syncContent->currentProgres());

                            //finished(int) is emitted when accept() occurs
                            ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog, SIGNAL( finished (int) ), mCloudSyncProgressDialog, SLOT (close()));
                            ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog, SIGNAL( rejected() ), mCloudSyncProgressDialog, SLOT (close()));

                            ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog->button(QDialogButtonBox::Cancel), SIGNAL( clicked() ), mLogoutExitView, SIGNAL( cloudSyncCancelled() ) );

                            // remotePush/Pull emits succeeded -> progressBar::onCloudSyncSucceeded emits cloudSyncSuccess -> ProgressDialog::accept -> this::onCloudSyncComplete
                            ORIGIN_VERIFY_CONNECT(syncContent, SIGNAL(succeeded()), mCloudSyncProgressDialog->content(), SLOT(onCloudSyncSucceeded()));
                			ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog->content(), SIGNAL(cloudSyncSuccess()), mCloudSyncProgressDialog, SLOT(accept()));
                            ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog, SIGNAL(accepted()), this, SLOT(onCloudSyncComplete()));

                            // Remote sync push failed goes through installerview to show the dialog to cancel or retry
                            // Remote sync pull failed will be handled by playview but we don't have to worry because playing game check will have failed so we won't get here

                            //hook up the progress
                            ORIGIN_VERIFY_CONNECT(syncContent, SIGNAL(progress(float, qint64, qint64)), mCloudSyncProgressDialog->content(), SLOT(onCloudProgressChanged(float, qint64, qint64)));

                            //hook up cancel
                            ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog, SIGNAL(rejected()), this, SIGNAL(cancelLogout()));
                            ORIGIN_VERIFY_CONNECT(mLogoutExitView, SIGNAL(cloudSyncCancelled()), this, SLOT(onCloudSyncCancelled()));

                            mCloudSyncProgressDialog->showWindow();
                            synching = true;
                        }
                    }
                }
            }
            return synching;
        }

        void LogoutExitViewController::disconnectSignals ()
        {
			ORIGIN_LOG_EVENT << "disconnectSignals";

            //disconnect the previous signals
            if (mCloudSyncProgressDialog)
            {
                ORIGIN_VERIFY_DISCONNECT(mCloudSyncProgressDialog, SIGNAL(rejected()), this, SIGNAL(cancelLogout()));
                ORIGIN_VERIFY_DISCONNECT(mCloudSyncProgressDialog, SIGNAL(accepted()), this, SLOT(onCloudSyncComplete()));
                ORIGIN_VERIFY_CONNECT(mCloudSyncProgressDialog->button(QDialogButtonBox::Cancel), SIGNAL( clicked() ), mLogoutExitView, SIGNAL( cloudSyncCancelled() ) );
            }
            ORIGIN_VERIFY_DISCONNECT(mLogoutExitView, SIGNAL(cloudSyncCancelled()), this, SIGNAL(cloudSyncCancelled()));
        }

        void LogoutExitViewController::onCloudSyncComplete ()
        {
            disconnectSignals();

            //need to check and see if there are more and show another dialog
            if (RemoteSyncManager::instance()->syncBeingPerformed())
            {
                showCancelCloudSaves ();
            }
            //we shouldn't need to do anything in this SLOT if there are no more since we're listening in a different SLOT for lastSyncFinished
        }

        void LogoutExitViewController::onAllCloudSyncsComplete ()
		{
			ORIGIN_LOG_EVENT << "onAllCloudSyncsComplete";

			// disconnect to prevent another signal from coming in after the controller is gone 
            ORIGIN_VERIFY_DISCONNECT(RemoteSyncManager::instance(), SIGNAL(lastSyncFinished()), 
                                    this, SLOT(onAllCloudSyncsComplete()));

            onCloudSyncComplete(); //to disconnect all the signals
            mCloudSyncProgressDialog->close();
            mCloudSyncProgressDialog = NULL;
            emit lastSyncFinished();
        }

        void LogoutExitViewController::onCloudSyncCancelled()
        {
            //we need to disconnect BEFORE we close the dialog, otherwise close will send the rejected() signal
            disconnectSignals();
            emit cloudSyncCancelled();
        }

        void LogoutExitViewController::promptCancelInstallationOnLogout(const QString& aGamename)
        {
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::Notice, 
                tr("ebisu_client_can_not_logout_title").arg(tr("application_name_uppercase")),
		        tr("ebisu_client_can_not_logout_while_installing").arg(tr("application_name")).arg(aGamename));
            OriginPushButton *pOkBtn = windowChrome->addButton(QDialogButtonBox::Ok);
            windowChrome->setDefaultButton(pOkBtn);
            windowChrome->manager()->setupButtonFocus();
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pOkBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            windowChrome->manager()->setupButtonFocus();
            windowChrome->exec();
        }

        void LogoutExitViewController::promptCancelInstallationOnExit(const QString& aGamename)
        {
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::Notice, 
                tr("ebisu_client_can_not_close_title").arg(tr("application_name_uppercase")),
		        tr("ebisu_client_can_not_close_while_installing").arg(tr("application_name")).arg(aGamename));
            OriginPushButton *pOkBtn = windowChrome->addButton(QDialogButtonBox::Ok);
            windowChrome->setDefaultButton(pOkBtn);
            windowChrome->manager()->setupButtonFocus();
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pOkBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            windowChrome->manager()->setupButtonFocus();
            windowChrome->exec();
        }

		void LogoutExitViewController::promptCannotCloseSDKGamePlaying(const QString& aGamename)
		{
			OriginWindow* windowChrome = new OriginWindow(
				(OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
				NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
			windowChrome->msgBox()->setup(OriginMsgBox::Notice, 
				tr("ebisu_client_can_not_close_title").arg(tr("application_name_uppercase")),
				tr("ebisu_client_can_not_close_while_playing").arg(tr("application_name")).arg(aGamename));
			OriginPushButton *pOkBtn = windowChrome->addButton(QDialogButtonBox::Ok);
			windowChrome->setDefaultButton(pOkBtn);
			windowChrome->manager()->setupButtonFocus();
			ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
			ORIGIN_VERIFY_CONNECT(pOkBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
			windowChrome->manager()->setupButtonFocus();
			windowChrome->exec();
		}

        bool LogoutExitViewController::promptPlayingGameOnExit(const QString& aGamename)
        {
            // *** On exit while playing the game, the user has the option of exiting anyway
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::Notice, 
                tr("ebisu_client_close_confirm_title").arg(tr("application_name_uppercase")),
		        tr("ebisu_client_exit_client_game_running").arg(aGamename).arg(tr("application_name")));
            OriginPushButton *pYesBtn = windowChrome->addButton(QDialogButtonBox::Yes);
            OriginPushButton *pNoBtn =  windowChrome->addButton(QDialogButtonBox::No);
            windowChrome->setDefaultButton(pNoBtn);
            windowChrome->manager()->setupButtonFocus();
            windowChrome->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pNoBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pYesBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));

            windowChrome->manager()->setupButtonFocus();
            windowChrome->exec();

            bool isConfirmed = windowChrome->getClickedButton() == pYesBtn;

            windowChrome->deleteLater();
    
            return isConfirmed;
        }

        void LogoutExitViewController::promptPlayingGameOnLogout(const QString& aGamename)
        {
            // *** On logout while playing the game, the user has no choice but to quit the game first
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::Notice, 
                tr("ebisu_client_can_not_logout_title").arg(tr("application_name_uppercase")),
		        tr("ebisu_client_can_not_logout_while_playing").arg(tr("application_name")).arg(aGamename));
            OriginPushButton *pOkBtn = windowChrome->addButton(QDialogButtonBox::Ok);
            windowChrome->setDefaultButton(pOkBtn);
            windowChrome->manager()->setupButtonFocus();
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pOkBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            windowChrome->manager()->setupButtonFocus();
            windowChrome->exec();
        }

        void LogoutExitViewController::promptPlayingGameOnLogoutBadCredentials ()
        {
            //user want to go Online but has no authenticated credentials so he has to be logged out
            //but can't log out if playing a game so inform the user: Please close all running games and retry to go online.            
            foreach(QWidget* widget, QApplication::topLevelWidgets())
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                if(window && window->objectName() == "promptPlayingGameOnLogoutBadCredentials")
                {
                    //If the dialog is already created we don't need to create it again
                    return;
                }
            }
            OriginWindow* windowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            windowChrome->msgBox()->setup(OriginMsgBox::Notice, 
                tr("ebisu_client_can_not_go_online_title"),
		        tr("ebisu_client_can_not_go_online_while_playing"));
            OriginPushButton *pOkBtn = windowChrome->addButton(QDialogButtonBox::Ok);
            windowChrome->setDefaultButton(pOkBtn);
            windowChrome->manager()->setupButtonFocus();
            ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(pOkBtn, SIGNAL(clicked()), windowChrome, SLOT(close()));
            windowChrome->manager()->setupButtonFocus();
            windowChrome->setObjectName("promptPlayingGameOnLogoutBadCredentials");
            windowChrome->showWindow();
        }

	}
}
