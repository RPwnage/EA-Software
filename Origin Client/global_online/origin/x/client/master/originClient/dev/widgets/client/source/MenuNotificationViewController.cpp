/////////////////////////////////////////////////////////////////////////////
// MenuNotificationViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MenuNotificationViewController.h"
#include "services/escalation/IEscalationService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/voice/VoiceService.h"
#include <QMenu>
#include <QAction>
#include "engine/content/ContentController.h"
#include "widgets/cloudsaves/source/CloudSyncViewController.h"
#include "engine/cloudsaves/RemoteSync.h"
#include <QDateTime>
#include "ClientFlow.h"
#include <QLabel>
#include <QTimer>

#include "MainFlow.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"

// Notifications
#include "OriginToastManager.h"
#include "engine/igo/IGOController.h"
#include "ToastViewController.h"
#include "AchievementToastView.h"
#include "OriginNotificationDialog.h"
#ifdef ORIGIN_MAC
#include <unistd.h>
#endif
namespace Origin
{
namespace Client
{
    const int TOAST_DELAY_TIMER = 3000;

MenuNotificationViewController::MenuNotificationViewController(QObject* parent)
: QObject(parent)
, mProductID(Services::readSetting(Origin::Services::SETTING_DebugMenuEntitlementID))
, mMenu(NULL)
{
    mMenu = new QMenu(dynamic_cast<QMenu*>(parent));
    mMenu->setObjectName("mainToastsMenu");
    if(mProductID == "")
    {
        mMenu->setEnabled(false);
        mMenu->setTitle("Error: Include QA::DebugMenuEntitlementID in .ini");
    }
    else
    {
        mMenu->setTitle("Notification Toasts - " + mProductID);
    }

    // Set up fake OIG toggle
    QAction* fakeOIGToasts = new QAction(mMenu);
    fakeOIGToasts->setText("Fake OIG (Toggle how toasts look)");
    fakeOIGToasts->setCheckable(true);
    ORIGIN_VERIFY_CONNECT(fakeOIGToasts, SIGNAL(toggled(bool)), ClientFlow::instance()->originToastManager(), SLOT(enableFakeOIG(const bool&)));
    mMenu->addAction(fakeOIGToasts);

    // Display various notifications
	QMenu* toastMenu = mMenu->addMenu(tr("Normal Toasts"));
  	addAction(toastMenu, new QAction(toastMenu), tr("Show All Toasts"), SLOT(showToasts()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Finish Download"), SLOT(showFinishDownloadToast()));
    QMenu* installedMenu = toastMenu->addMenu(tr("InstallFinished"));
    	addAction(installedMenu, new QAction(installedMenu), tr("Show Ready to Play"), SLOT(showReadyToPlayToast()));
    	addAction(installedMenu, new QAction(installedMenu), tr("Show Ready to Install Update"), SLOT(showReadyToInstallUpdateToast()));
    	addAction(installedMenu, new QAction(installedMenu), tr("Show PDLC Restart"), SLOT(showPDLCRestartGameToast()));
    	addAction(installedMenu, new QAction(installedMenu), tr("Show PDLC Save"), SLOT(showPDLCReloadSaveToast()));
    	addAction(installedMenu, new QAction(installedMenu), tr("Show PDLC Deploy"), SLOT(showPDLCHotDeployToast()));
        addAction(installedMenu, new QAction(installedMenu), tr("Show Dynamic Content Playable"), SLOT(showDynamicContentPlayableToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Download Error"), SLOT(showDownloadErrorToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Friends Signing In"), SLOT(showFriendsSigningInToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Friends Signing Out"), SLOT(showFriendsSigningOutToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Friends Start Game"), SLOT(showFriendsStartGameToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Friends Quit Game"), SLOT(showFriendsQuitGameToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Friends Start Broadcast"), SLOT(showFriendsStartBroadcastToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Friends Stop Broadcast"), SLOT(showFriendsStopBroadcastToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Incoming Text Chat"), SLOT(showIncomingTextChatToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Long Incoming Text Chat"), SLOT(showLongIncomingTextChatToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show IGO"), SLOT(showIGOToast()));
	addAction(toastMenu, new QAction(toastMenu), tr("Show Achievement"), SLOT(showAchievementToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Broadcast Stopped"), SLOT(showBroadcastStoppedToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Broadcast Not Permitted"), SLOT(showBroadcastNotPermittedToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Incomming Chat Room Message"), SLOT(showIncommingChatRoomMessageToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Invite to Chat Room"), SLOT(showInviteToChatRoomToast()));    
   	addAction(toastMenu, new QAction(toastMenu), tr("Show Kickout of Room"), SLOT(showKickedOutOfGroupToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Game Invite"), SLOT(showGameInviteToast()));
    QMenu* koreanMenu = toastMenu->addMenu(tr("Korean Timer"));
        addAction(koreanMenu, new QAction(koreanMenu), tr("Show Korean Single"), SLOT(showKoreanTimerSingular()));
        addAction(koreanMenu, new QAction(koreanMenu), tr("Show Korean Plural"), SLOT(showKoreanTimerPlural()));
    QMenu* freeTrialMenu = toastMenu->addMenu(tr("GameTime"));
        addAction(freeTrialMenu, new QAction(freeTrialMenu), tr("Show GameTime Single"), SLOT(showGameTimeSingular()));
        addAction(freeTrialMenu, new QAction(freeTrialMenu), tr("Show GameTime Plural"), SLOT(showGameTimePlural()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Unpin Toast"), SLOT(showUnPinToast()));
    addAction(toastMenu, new QAction(toastMenu), tr("Show Repin Toast"), SLOT(showRePinToast()));
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled )
    {
        addAction(toastMenu, new QAction(toastMenu), tr("Show Incomming VoIP Call"), SLOT(showIncommingVoIPCallToast()));
        QMenu* voipMenu = toastMenu->addMenu(tr("InGame VoIP"));
        addAction(voipMenu, new QAction(voipMenu), tr("Show VoIP Toasts (John)"), SLOT(showVoIPToast()));
        addAction(voipMenu, new QAction(voipMenu), tr("Show 3 VoIP Toasts"), SLOT(show3VoIPToast()));
        addAction(voipMenu, new QAction(voipMenu), tr("Show VoIP Toasts (User)"), SLOT(showUserVoIPToast()));
        addAction(voipMenu, new QAction(voipMenu), tr("Show VoIP User Muted"), SLOT(showUserMuted()));
        addAction(voipMenu, new QAction(voipMenu), tr("Remove VoIP Toasts (Jimmy)"), SLOT(removeVoIPToast()));
        addAction(voipMenu, new QAction(voipMenu), tr("Remove 3 VoIP Toasts"), SLOT(remove3VoIPToast()));
        addAction(voipMenu, new QAction(voipMenu), tr("Remove VoIP Toast (User)"), SLOT(removeUserVoIPToast()));
        addAction(voipMenu, new QAction(voipMenu), tr("Remove VoIP User Muted"), SLOT(removeUserMuted()));
    }
#endif

	// Delayed 3 second to help with IGO testing
  	QMenu* delayMenu = mMenu->addMenu(tr("3 Second Delayed Toasts"));
    addAction(delayMenu, new QAction(delayMenu), tr("Show All Toasts"), SLOT(delayedShowToasts()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Finish Download"), SLOT(delayedFinishDownloadToast()));
    QMenu* delayinstalledMenu = delayMenu->addMenu(tr("InstallFinished"));
        addAction(delayinstalledMenu, new QAction(delayinstalledMenu), tr("Show Ready to Play"), SLOT(delayedShowReadyToPlayToast()));
        addAction(delayinstalledMenu, new QAction(delayinstalledMenu), tr("Show Ready to Install Update"), SLOT(delayedShowReadyToInstallUpdateToast()));
        addAction(delayinstalledMenu, new QAction(delayinstalledMenu), tr("Show PDLC Restart"), SLOT(delayedShowPDLCRestartGameToast()));
        addAction(delayinstalledMenu, new QAction(delayinstalledMenu), tr("Show PDLC Save"), SLOT(delayedShowPDLCReloadSaveToast()));
        addAction(delayinstalledMenu, new QAction(delayinstalledMenu), tr("Show PDLC Deploy"), SLOT(delayedShowPDLCHotDeployToast()));
        addAction(delayinstalledMenu, new QAction(delayinstalledMenu), tr("Show Dynamic Content Playable"), SLOT(delayedShowDynamicContentPlayableToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Download Error"), SLOT(delayedDownloadErrorToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Friends Signing In"), SLOT(delayedFriendsSigningInToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Friends Signing Out"), SLOT(delayedFriendsSigningOutToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Friends Start Game"), SLOT(delayedFriendsStartGameToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Friends Quit Game"), SLOT(delayedFriendsQuitGameToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Friends Start Broadcast"), SLOT(delayedFriendsStartBroadcastToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Friends Stop Broadcast"), SLOT(delayedFriendsStopBroadcastToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Incoming Text Chat"), SLOT(delayedIncomingTextChatToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Long Incoming Text Chat"), SLOT(delayedLongIncomingTextChatToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Achievement"), SLOT(delayedAchievementToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Game Invite"), SLOT(delayedShowGameInviteToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Broadcast Stopped"), SLOT(delayedBroadcastStoppedToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show IGO"), SLOT(delayedIGOToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Broadcast Not Permitted"), SLOT(delayedBroadcastNotPermittedToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Incomming Chat Room Message"), SLOT(delayedShowIncommingChatRoomMessageToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Invite To Chat Room"), SLOT(delayedShowInviteToChatRoomToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Kickout of Room"), SLOT(delayedShowKickedOutOfGroupToast()));
    QMenu* delayKoreanMenu = delayMenu->addMenu(tr("Korean Timer"));
        addAction(delayKoreanMenu, new QAction(delayKoreanMenu), tr("Show Korean Single"), SLOT(delayedShowKoreanTimerSingular()));
        addAction(delayKoreanMenu, new QAction(delayKoreanMenu), tr("Show Korean Plural"), SLOT(delayedShowKoreanTimerPlural()));
    QMenu* delayedFreeTrialMenu = delayMenu->addMenu(tr("GameTime"));
        addAction(delayedFreeTrialMenu, new QAction(delayedFreeTrialMenu), tr("Show GameTime Single"), SLOT(delayedShowGameTimeSingular()));
        addAction(delayedFreeTrialMenu, new QAction(delayedFreeTrialMenu), tr("Show GameTime Plural"), SLOT(delayedShowGameTimePlural()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Unpin Toast"), SLOT(delayedShowUnPinToast()));
    addAction(delayMenu, new QAction(delayMenu), tr("Show Repin Toast"), SLOT(delayedShowRePinToast()));
#if ENABLE_VOICE_CHAT
    if( isVoiceEnabled )
    {
        addAction(delayMenu, new QAction(delayMenu), tr("Show Incomming VoIP Call"), SLOT(delayedShowIncommingVoIPCallToast()));
        QMenu* delayVoipMenu = delayMenu->addMenu(tr("InGame VoIP"));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Show VoIP Toasts"), SLOT(delayedShowVoIPToast()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Show 3 VoIP Toasts"), SLOT(delayedShow3VoIPToast()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Show VoIP Toasts (User)"), SLOT(delayedShowUserVoIPToast()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Show VoIP User Muted"), SLOT(showUserMuted()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Remove VoIP Toasts"), SLOT(delayedRemoveVoIPToast()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Remove 3 VoIP Toasts"), SLOT(delayedRemove3VoIPToast()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Remove VoIP Toast (User)"), SLOT(delayedRemoveUserVoIPToast()));
        addAction(delayVoipMenu, new QAction(delayVoipMenu), tr("Remove VoIP User Muted"), SLOT(removeUserMuted()));
    }
#endif
}


MenuNotificationViewController::~MenuNotificationViewController()
{

}

void MenuNotificationViewController::addAction(QMenu* menuParent, QAction* newAction, const QString& title, const char* slot)
{
    if(newAction)
    {
        newAction->setText(title);
        ORIGIN_VERIFY_CONNECT(newAction, SIGNAL(triggered()), this, slot);
        menuParent->addAction(newAction);
    }

}

void MenuNotificationViewController::addAction(QAction* newAction, const QString& title, const char* slot)
{
    addAction(mMenu, newAction, title, slot);
}

void MenuNotificationViewController::showToasts()
{
    const int TOAST_DELAY = 1000;

    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        // Delay each toast for 1 second so that we have time to see each toast
        showFinishDownloadToast();
	    QTimer::singleShot(TOAST_DELAY, this, SLOT(showReadyToPlayToast()));
        QTimer::singleShot(TOAST_DELAY *2,  this, SLOT(showPDLCRestartGameoast()));
        QTimer::singleShot(TOAST_DELAY *3,  this, SLOT(showPDLCReloadSaveToast()));
        QTimer::singleShot(TOAST_DELAY *4,  this, SLOT(showPDLCHotDeployToast()));
        QTimer::singleShot(TOAST_DELAY *5,  this, SLOT(showDynamicContentPlayableToast()));
        QTimer::singleShot(TOAST_DELAY *6,  this, SLOT(showDownloadErrorToast()));
        QTimer::singleShot(TOAST_DELAY *7,  this, SLOT(showFriendsSigningInToast()));
        QTimer::singleShot(TOAST_DELAY *8,  this, SLOT(showFriendsSigningOutToast()));
        QTimer::singleShot(TOAST_DELAY *9,  this, SLOT(showFriendsStartGameToast()));
        QTimer::singleShot(TOAST_DELAY *10, this, SLOT(showFriendsQuitGameToast()));
        QTimer::singleShot(TOAST_DELAY *11, this, SLOT(showFriendsStartBroadcastToast()));
        QTimer::singleShot(TOAST_DELAY *12, this, SLOT(showFriendsStopBroadcastToast()));
        QTimer::singleShot(TOAST_DELAY *13, this, SLOT(showIncomingTextChatToast()));
	    QTimer::singleShot(TOAST_DELAY *14, this, SLOT(showLongIncomingTextChatToast()));
        QTimer::singleShot(TOAST_DELAY *15, this, SLOT(showIGOToast()));
        QTimer::singleShot(TOAST_DELAY *16, this, SLOT(showAchievementToast()));
        QTimer::singleShot(TOAST_DELAY *17, this, SLOT(showBroadcastStoppedToast()));
        QTimer::singleShot(TOAST_DELAY *18, this, SLOT(showBroadcastNotPermittedToast()));
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
	        QTimer::singleShot(TOAST_DELAY *19, this, SLOT(showIncommingVoIPCallToast()));
#endif
        QTimer::singleShot(TOAST_DELAY *20, this, SLOT(showKickedOutOfGroupToast()));
        QTimer::singleShot(TOAST_DELAY *21, this, SLOT(showIncommingChatRoomMessageToast()));
        QTimer::singleShot(TOAST_DELAY *22, this, SLOT(showGameInviteToast()));
        QTimer::singleShot(TOAST_DELAY *23, this, SLOT(showKoreanTimerSingular()));
        QTimer::singleShot(TOAST_DELAY *24, this, SLOT(showKoreanTimerPlural()));
        QTimer::singleShot(TOAST_DELAY *25, this, SLOT(showGameTimeSingular()));
        QTimer::singleShot(TOAST_DELAY *26, this, SLOT(showGameTimePlural()));
        QTimer::singleShot(TOAST_DELAY *27, this, SLOT(showInviteToChatRoomToast()));
    }
}

void MenuNotificationViewController::showFinishDownloadToast()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_FINISHEDDOWNLOAD.name(), QString("<b>%0</b>").arg(displayName), QString(tr("ebisu_client_notifications_is_ready_to_install")));
    if (toast)
    {
        // IGO
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), Origin::Engine::IGOController::instance(), SLOT(igoShowDownloads()), Qt::QueuedConnection);

        // Desktop
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), ClientFlow::instance()->view(), SLOT(showDownloadProgressDialog()), Qt::QueuedConnection);
    }
}

void MenuNotificationViewController::showReadyToPlayToast()
{
	QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast( Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), QString("<b>%0</b>").arg(displayName), (tr("ebisu_client_notifications_is_ready_to_play")));
}

void MenuNotificationViewController::showReadyToInstallUpdateToast()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), "<b>"+tr("ebisu_client_notification_game_ready_to_install")+"</b>", tr("ebisu_client_notification_restart_game_to_update"));
}

void MenuNotificationViewController::showPDLCRestartGameToast()
{
	QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }
    otm->showToast( Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), tr("ebisu_client_notification_additional_content_for_game_downlaoded").arg(displayName), (tr("ebisu_client_notification_potentially_restart_game_to_use_dlc")));
}

void MenuNotificationViewController::showPDLCReloadSaveToast()
{
	QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast( Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), QString("<b>%0</b>").arg(displayName), (tr("ebisu_client_notifications_pdlc_reload_save")));
}

void MenuNotificationViewController::showPDLCHotDeployToast()
{
	QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast( Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), QString("<b>%0</b>").arg(displayName), (tr("ebisu_client_notifications_pdlc_hot_deployed")));
}

void MenuNotificationViewController::showDynamicContentPlayableToast()
{
    QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(),
        "<b>"+tr("ebisu_client_game_ready_to_play").arg(displayName)+"</b>", 
        tr("ebisu_client_game_still_downloading"));
}

void MenuNotificationViewController::showDownloadErrorToast()
{
    QString displayName = QString("fakedisplayname");
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        displayName = ent->contentConfiguration()->displayName();
    }

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_DOWNLOADERROR.name(), QString("<b>%0</b>").arg(displayName), QString(tr("ebisu_client_download_failed")));
    if (toast)
    {
        // IGO
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), Origin::Engine::IGOController::instance(), SLOT(igoShowDownloads()), Qt::QueuedConnection);

        // Desktop
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), ClientFlow::instance()->view(), SLOT(showDownloadProgressDialog()), Qt::QueuedConnection);
    }
}

void MenuNotificationViewController::showFriendsSigningInToast()
{

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_FRIENDSIGNINGIN.name(), tr("ebisu_client_friend_signing_in_message").arg("<b>Mr. Banana</b>"));
}

void MenuNotificationViewController::showFriendsSigningOutToast()
{

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_FRIENDSIGNINGOUT.name(), tr("ebisu_client_friend_signing_out_message").arg("<b>Mr. Banana</b>"));
}

void MenuNotificationViewController::showFriendsStartGameToast()
{
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        const QString displayName = ent->contentConfiguration()->displayName();

        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR<<"OriginToastManager not available";
            return;
        }

        otm->showToast(Services::SETTING_NOTIFY_FRIENDSTARTSGAME.name(), tr("ebisu_client_friend_starts_game_message").arg("<b>Mr. Banana</b>").arg(displayName));
    }
}

void MenuNotificationViewController::showFriendsQuitGameToast()
{
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        const QString displayName = ent->contentConfiguration()->displayName();

        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR<<"OriginToastManager not available";
            return;
        }

        otm->showToast(Services::SETTING_NOTIFY_FRIENDQUITSGAME.name(), tr("ebisu_client_friend_quits_game_message").arg("<b>Mr. Banana</b>").arg(displayName));
    }
}

void MenuNotificationViewController::showFriendsStartBroadcastToast() 
{        
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        const QString displayName = ent->contentConfiguration()->displayName();

        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR<<"OriginToastManager not available";
            return;
        }

        otm->showToast(Services::SETTING_NOTIFY_FRIENDSTARTBROADCAST.name(), tr("ebisu_client_soref_user_started_broadcasting_game").arg("<b>Mr. Banana</b>").arg(displayName));
    }
}

void MenuNotificationViewController::showFriendsStopBroadcastToast() 
{        
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        const QString displayName = ent->contentConfiguration()->displayName();

        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR<<"OriginToastManager not available";
            return;
        }

        otm->showToast(Services::SETTING_NOTIFY_FRIENDSTOPBROADCAST.name(), tr("ebisu_client_soref_user_stopped_broadcasting_game").arg("<b>Mr. Banana</b>").arg(displayName));
    }
}

void MenuNotificationViewController::showIncomingTextChatToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_INCOMINGTEXTCHAT.name(), "<b>Mr. Banana</b>", "How are you today?");
}

void MenuNotificationViewController::showLongIncomingTextChatToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_INCOMINGTEXTCHAT.name(), "<b>Mr. Banana</b>", "blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah ");
}

void MenuNotificationViewController::showIGOToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast("POPUP_OIG_HOTKEY", tr("ebisu_client_open_app").arg(tr("ebisu_client_igo_name")));
}

void MenuNotificationViewController::showAchievementToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    UIToolkit::OriginNotificationDialog* toast = otm->showAchievementToast("POPUP_ACHIEVEMENT", QString("<b>%1</b>").arg(tr("ebisu_client_achievement_unlocked")), 
                                                                            "Very Long Achievement Name - Or Maybe even longer.", 10, QString(""), false);

    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), ClientFlow::instance()->view(), SLOT(showAchievementsHome()), Qt::QueuedConnection);
}

void MenuNotificationViewController::showBroadcastStoppedToast() 
{        
    emit Origin::Engine::IGOController::instance()->igoShowToast("POPUP_BROADCAST_STOPPED", tr("ebisu_client_twitch_stopped_broadcasting"), "");
}

void MenuNotificationViewController::showBroadcastNotPermittedToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast("POPUP_BROADCAST_NOT_PERMITTED", tr("ebisu_client_twitch_permitted_broadcasting"));
}

void MenuNotificationViewController::showIncommingChatRoomMessageToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE.name(), QString("<b>Mr. Banana</b><br/><i>Super Duper Group: Raid Chat</i>"), "Hey what's up?");
}

void MenuNotificationViewController::showInviteToChatRoomToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_INVITETOCHATROOM.name(), QString("<b>Mr. Banana</b><br/><i>Super Duper Group: Raid Chat</i>"), "Hey what's up?");
}


void MenuNotificationViewController::showKickedOutOfGroupToast() 
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_INCOMINGTEXTCHAT.name(), QString("<b>Removed From<i>Super Duper Group</i></b>"), QString(tr("ebisu_client_groip_kicked_from_room")).arg("Mr. Banana"));
}

void MenuNotificationViewController::showGameInviteToast()
{
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    if(ent.isNull() == false)
    {
        const QString displayName = ent->contentConfiguration()->displayName();

        QString bodyStr = QString(tr("ebisu_client_non_friend_invite_body1").arg(QString("<b>%0</b>").arg("Mr. Banana")).arg(displayName));

        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR<<"OriginToastManager not available";
            return;
        }

        otm->showToast(Services::SETTING_NOTIFY_GAMEINVITE.name(), bodyStr);
    }
}

void MenuNotificationViewController::showKoreanTimerSingular()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString title = tr("ebisu_client_take_a_break");
    QString hours = tr("ebisu_client_interval_hour_singular").arg(1);
    QString message = tr("ebisu_client_take_a_break_time").arg(hours);
    otm->showToast("POPUP_OIG_KOREAN_WARNING", title, message);
}

void MenuNotificationViewController::showKoreanTimerPlural()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString title = tr("ebisu_client_take_a_break");
    QString hours = tr("ebisu_client_interval_hour_plural").arg(5);
    QString message = tr("ebisu_client_take_a_break_time").arg(hours);
    otm->showToast("POPUP_OIG_KOREAN_WARNING", title, message);
}

void MenuNotificationViewController::showGameTimeSingular()
{   
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString message = tr("ebisu_client_free_trial_ending_notification_minute").arg(1);
    UIToolkit::OriginNotificationDialog* toast = otm->showToast("POPUP_FREE_TRIAL_EXPIRING", message, "");
    QSignalMapper* signalMapper = new QSignalMapper(toast->content());
    signalMapper->setMapping(toast, 1);
    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(int)), MainFlow::instance()->contentFlowController()->localContentViewController(), SLOT(showTrialEnding(int)));
}

void MenuNotificationViewController::showGameTimePlural()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString message = tr("ebisu_client_free_trial_ending_notification_minutes").arg(30);
    UIToolkit::OriginNotificationDialog* toast = otm->showToast("POPUP_FREE_TRIAL_EXPIRING", message, "");
    QSignalMapper* signalMapper = new QSignalMapper(toast->content());
    signalMapper->setMapping(toast, 30);
    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(int)), MainFlow::instance()->contentFlowController()->localContentViewController(), SLOT(showTrialEnding(int)));
}

void MenuNotificationViewController::showUnPinToast()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    const QString title = tr("ebisu_client_window_pinned");
    otm->showToast("POPUP_IGO_PIN_ON", title, "");
}

void MenuNotificationViewController::showRePinToast()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    const QString title =  tr("ebisu_client_all_windows_have_been_unpinned").arg(tr("ebisu_client_igo_name"));
    otm->showToast("POPUP_IGO_PIN_ON", title, "");
}

#if ENABLE_VOICE_CHAT

void MenuNotificationViewController::showIncommingVoIPCallToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_INCOMINGVOIPCALL.name(), QString(tr("ebisu_client_is_calling")).arg("<b>Mr. Banana</b>"));
       
    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(actionAccepted()), this, SLOT(onAccepted()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(actionRejected()), this, SLOT(onRejected()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(onClicked()), Qt::QueuedConnection);
}

void MenuNotificationViewController::showVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showVoiceChatIdentifier(QString("<b>John</b>"));

}

void MenuNotificationViewController::show3VoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showVoiceChatIdentifier(QString("<b>Fred</b>"));
    otm->showVoiceChatIdentifier(QString("<b>Jimmy</b>"));
    otm->showVoiceChatIdentifier(QString("<b>Alex</b>"));
}

void MenuNotificationViewController::showUserVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    using namespace Origin::Engine;
    UserRef user = LoginController::currentUser();
    QString id = QString::number(user->userId());

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showVoiceChatIdentifier(id, OriginToastManager::UserVoiceType_CurrentUserTalking);
}

void MenuNotificationViewController::showUserMuted()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    using namespace Origin::Engine;
    UserRef user = LoginController::currentUser();
    QString id = QString::number(user->userId());

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showVoiceChatIdentifier(id, OriginToastManager::UserVoiceType_CurrentUserMuted);
}

void MenuNotificationViewController::removeVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->removeVoiceChatIdentifier(QString("<b>Jimmy</b>"));
}

void MenuNotificationViewController::remove3VoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->removeVoiceChatIdentifier(QString("<b>Fred</b>"));
    otm->removeVoiceChatIdentifier(QString("<b>Jimmy</b>"));
    otm->removeVoiceChatIdentifier(QString("<b>Alex</b>"));
}

void MenuNotificationViewController::removeUserVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    using namespace Origin::Engine;
    UserRef user = LoginController::currentUser();
    QString id = QString::number(user->userId());

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->removeVoiceChatIdentifier(id);
}

void MenuNotificationViewController::removeUserMuted()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    using namespace Origin::Engine;
    UserRef user = LoginController::currentUser();
    QString id = QString::number(user->userId());

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->removeVoiceChatIdentifier(id);
}

#endif
/////////////////////////////////////////////////////////////////////////////////////
////////////  Delayed Notifications  ////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void MenuNotificationViewController::delayedShowToasts()
{
	QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showToasts()));
}

void MenuNotificationViewController::delayedFinishDownloadToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFinishDownloadToast()));
}

void MenuNotificationViewController::delayedShowReadyToPlayToast()
{
	QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showReadyToPlayToast()));
}

void MenuNotificationViewController::delayedShowReadyToInstallUpdateToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showReadyToInstallUpdateToast()));
}

void MenuNotificationViewController::delayedShowPDLCRestartGameToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showPDLCRestartGameoast()));
}

void MenuNotificationViewController::delayedShowPDLCReloadSaveToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showPDLCReloadSaveToast()));
}

void MenuNotificationViewController::delayedShowPDLCHotDeployToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showPDLCHotDeployToast()));
}

void MenuNotificationViewController::delayedShowDynamicContentPlayableToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showDynamicContentPlayableToast()));
}

void MenuNotificationViewController::delayedDownloadErrorToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showDownloadErrorToast()));
}

void MenuNotificationViewController::delayedFriendsSigningInToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFriendsSigningInToast()));
}

void MenuNotificationViewController::delayedFriendsSigningOutToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFriendsSigningOutToast()));
}

void MenuNotificationViewController::delayedFriendsStartGameToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFriendsStartGameToast()));
}

void MenuNotificationViewController::delayedFriendsQuitGameToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFriendsQuitGameToast()));
}

void MenuNotificationViewController::delayedFriendsStartBroadcastToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFriendsStartBroadcastToast()));
}

void MenuNotificationViewController::delayedFriendsStopBroadcastToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showFriendsStopBroadcastToast()));
}

void MenuNotificationViewController::delayedIncomingTextChatToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showIncomingTextChatToast()));
}

void MenuNotificationViewController::delayedLongIncomingTextChatToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showLongIncomingTextChatToast()));
}

void MenuNotificationViewController::delayedIGOToast()
{
	QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showIGOToast()));
}

void MenuNotificationViewController::delayedAchievementToast()
{
	QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showAchievementToast()));
}

void MenuNotificationViewController::delayedBroadcastStoppedToast()
{
	QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showBroadcastStoppedToast()));
}

void MenuNotificationViewController::delayedBroadcastNotPermittedToast()
{
	QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showBroadcastNotPermittedToast()));
}

void MenuNotificationViewController::delayedShowIncommingChatRoomMessageToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showIncommingChatRoomMessageToast()));
}

void MenuNotificationViewController::delayedShowInviteToChatRoomToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showInviteToChatRoomToast()));
}

void MenuNotificationViewController::delayedShowKickedOutOfGroupToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showKickedOutOfGroupToast()));
}

void MenuNotificationViewController::delayedShowGameInviteToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showGameInviteToast()));
}

void MenuNotificationViewController::delayedShowKoreanTimerSingular()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showKoreanTimerSingular()));
}

void MenuNotificationViewController::delayedShowKoreanTimerPlural()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showKoreanTimerPlural()));
}

void MenuNotificationViewController::delayedShowGameTimeSingular()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showGameTimeSingular()));
}

void MenuNotificationViewController::delayedShowGameTimePlural()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showGameTimePlural()));
}

void MenuNotificationViewController::delayedShowUnPinToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showUnPinToast()));
}

void MenuNotificationViewController::delayedShowRePinToast()
{
    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showRePinToast()));
}

#if ENABLE_VOICE_CHAT

void MenuNotificationViewController::delayedShowIncommingVoIPCallToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showIncommingVoIPCallToast()));
}

void MenuNotificationViewController::delayedShowVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showVoIPToast()));
}

void MenuNotificationViewController::delayedShow3VoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(show3VoIPToast()));
}

void MenuNotificationViewController::delayedShowUserVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showUserVoIPToast()));
}

void MenuNotificationViewController::delayedShowUserMuted()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(showUserMuted()));
}

void MenuNotificationViewController::delayedRemoveVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(removeVoIPToast()));
}

void MenuNotificationViewController::delayedRemove3VoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(remove3VoIPToast()));
}

void MenuNotificationViewController::delayedRemoveUserVoIPToast()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(removeUserVoIPToast()));
}

void MenuNotificationViewController::delayedRemoveUserMuted()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    QTimer::singleShot(TOAST_DELAY_TIMER, this, SLOT(removeUserMuted()));
}

#endif


// Call back functions for Imcoming call toast testing
void MenuNotificationViewController::onAccepted()
{
    ORIGIN_LOG_EVENT<<"Incomming call was accepted";
}

void MenuNotificationViewController::onRejected()
{
    ORIGIN_LOG_EVENT<<"Incomming call was declined";
}

void MenuNotificationViewController::onClicked()
{
    ORIGIN_LOG_EVENT<<"Incomming call activated through the HOT KEY";
}

}
}
