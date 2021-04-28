/////////////////////////////////////////////////////////////////////////////
// MenuFlowWindowViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MenuFlowWindowViewController.h"
#include "services/escalation/IEscalationService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/voice/VoiceService.h"
#include <QMenu>
#include <QAction>
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "widgets/cloudsaves/source/CloudSyncViewController.h"
#include "engine/cloudsaves/RemoteSync.h"
#include <QDateTime>
#include "ClientFlow.h"
#include <QLabel>
#include <QTimer>
#include "EmailVerificationMessageAreaViewController.h"

// Play Flow windows

#include "RTPViewController.h"
#include "PlayViewController.h"
#include "InstallerFlow.h"
#include "InstallerViewController.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"
#include "MainFlow.h"
#include "LocalizedDateFormatter.h"
#include "ITOViewController.h"
#include "NetPromoterViewController.h"
#include "NetPromoterViewControllerGame.h"
#include "FirstLaunchFlow.h"
#include "SaveBackupRestoreViewController.h"
#include "BroadcastViewController.h"

// Groip Error dialogs
#include "GroupInviteViewController.h"
#include "CreateGroupViewController.h"
#include "DeleteGroupViewController.h"
#include "EditGroupViewController.h"
#include "RemoveGroupUserViewController.h"
#include "AcceptGroupInviteViewController.h"
#include "SocialViewController.h"
#include "engine/social/SocialController.h"
#include "chat/OriginConnection.h"
#include "chat/ChatGroup.h"
#include "engine/login/User.h"
#include "engine/social/ConversationManager.h"
#include "chat/RemoteUser.h"
#include "chat/XMPPImplTypes.h"

#ifdef ORIGIN_MAC
#include <unistd.h>
#endif
namespace Origin
{
namespace Client
{

MenuFlowWindowViewController::MenuFlowWindowViewController(QObject* parent)
: QObject(parent)
, mMenu(NULL)
{
    mMenu = new QMenu(dynamic_cast<QMenu*>(parent));
    mMenu->setObjectName("mainDebugFlowWindowMenu");
    const QString offerId = Services::readSetting(Services::SETTING_DebugMenuEntitlementID).toString();

    Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
    if(contentController)
    {
        if(contentController->initialFullEntitlementRefreshed())
        {
            setEntitlement();
        }
        else
        {
            ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), 
                this, SLOT(setEntitlement()));
        }
    }
    QMenu* flowsMenu = mMenu->addMenu(tr("Start Flow"));
    QMenu* netPromoterMenu = mMenu->addMenu(tr("Promoters"));

    addAction(new QAction(mMenu), tr("Show Cloud Sync Flow windows"), SLOT(showCloudSyncFlowWindows()));
    addAction(new QAction(mMenu), tr("Show Play Flow windows"), SLOT(showPlayFlowWindows()));
    addAction(new QAction(mMenu), tr("Show Download Flow windows"), SLOT(showDownloadFlowWindows()));
    addAction(new QAction(mMenu), tr("Show Download Flow error windows"), SLOT(showDownloadFlowErrorWindows()));
    addAction(new QAction(mMenu), tr("Show entitlements' windows"), SLOT(showEntitlementWindows()));
    addAction(new QAction(mMenu), tr("Show RTP Flow windows"), SLOT(showRTPWindows()));
    addAction(new QAction(mMenu), tr("Show ITO Flow windows"), SLOT(showITOWindows()));
    addAction(new QAction(mMenu), tr("Show First Launch windows"), SLOT(showFirstLaunchWindows()));
    addAction(new QAction(mMenu), tr("Show Message Area windows"), SLOT(showMessageAreaWindows()));
    addAction(new QAction(mMenu), tr("Show Save Backup Restore windows"), SLOT(showSaveBackupRestore()));
    addAction(new QAction(mMenu), tr("Show Crash window"), SLOT(showCrashWindow()));
    addAction(new QAction(mMenu), tr("Show Broadcast windows"), SLOT(showBroadcastWindows()));

    // Flow menu options
    addAction(flowsMenu, new QAction(flowsMenu), tr("Download"), SLOT(startDownloadFlow()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Download All DLC"), SLOT(startDLCDownloadFlow()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Trigger Download Error"), SLOT(triggerDownloadError()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Trigger Download Error Retry"), SLOT(triggerDownloadErrorRetry()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Update"), SLOT(startUpdate()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Repair"), SLOT(startRepair()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Play"), SLOT(startPlayFlow()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Cloud"), SLOT(startCloudSync()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Broadcast (have game running)"), SLOT(startBroadcastFlow()));
    addAction(flowsMenu, new QAction(flowsMenu), tr("Clear complete download queue"), SLOT(clearCompleteQueue()));

    // Net promoter menu options
    addAction(netPromoterMenu, new QAction(netPromoterMenu), tr("Game Net Promoter"), SLOT(showGameNetPromoter()));
    addAction(netPromoterMenu, new QAction(netPromoterMenu), tr("Origin Net Promoter"), SLOT(showOriginNetPromoter()));
    addAction(netPromoterMenu, new QAction(netPromoterMenu), tr("ebisu_client_notranslate_force_promo_start"), SLOT(showPromoOriginStarted()));
    addAction(netPromoterMenu, new QAction(netPromoterMenu), tr("ebisu_client_notranslate_force_promo_end"), SLOT(showPromoGameplayFinished()));
    addAction(netPromoterMenu, new QAction(netPromoterMenu), tr("ebisu_client_notranslate_force_promo_download"), SLOT(showPromoDownloadStarted()));

    // Display Groip generic error dialogs
    QMenu* groipMenu = mMenu->addMenu(tr("Groip Error Dialogs"));
    addAction(groipMenu, new QAction(groipMenu), tr("Show Groip Windows"), SLOT(showGroipWindows()));
    addAction(groipMenu, new QAction(groipMenu), tr("Show Groip Chat Error"), SLOT(showGroipChatError()));
#ifdef ORIGIN_MAC
    QList<QAction*> actions = groipMenu->actions();
    actions[1]->setEnabled(false);
#endif
}


MenuFlowWindowViewController::~MenuFlowWindowViewController()
{

}

void MenuFlowWindowViewController::addAction(QMenu* menuParent, QAction* newAction, const QString& title, const char* slot)
{
    if(newAction)
    {
        newAction->setText(title);
        ORIGIN_VERIFY_CONNECT(newAction, SIGNAL(triggered()), this, slot);
        menuParent->addAction(newAction);
    }

}

void MenuFlowWindowViewController::addAction(QAction* newAction, const QString& title, const char* slot)
{
    addAction(mMenu, newAction, title, slot);
}


void MenuFlowWindowViewController::setEntitlement(Engine::Content::EntitlementRef ent)
{
    Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
    const QString offerId = Services::readSetting(Services::SETTING_DebugMenuEntitlementID).toString();
    if(offerId.count())
    {
        ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId);
    }
    if(ent.isNull() && contentController && contentController->entitlements().count())
    {
        mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlements().first();
    }
    else
    {
        mEntitlement = ent;
    }

    if(mEntitlement)
    {
        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), 
            this, SLOT(setEntitlement()));
        mMenu->setEnabled(true);
        mMenu->setTitle(QString("%0 (%1) Flow Windows").arg(mEntitlement->contentConfiguration()->displayName()).arg(mEntitlement->contentConfiguration()->productId()));
    }
    else
    {
        mMenu->setEnabled(false);
        mMenu->setTitle("Client Windows: couldn't find entitlement");
    }
}


void MenuFlowWindowViewController::startPlayFlow()
{
    MainFlow::instance()->contentFlowController()->startPlayFlow(mEntitlement->contentConfiguration()->productId(), false);
}


void MenuFlowWindowViewController::startCloudSync()
{
    MainFlow::instance()->contentFlowController()->startCloudSyncFlow(mEntitlement);
}

void MenuFlowWindowViewController::startBroadcastFlow()
{
#ifdef ORIGIN_PC
    new BroadcastViewController(this, ClientScope);
    Engine::IGOController::instance()->twitch()->attemptDisplayWindow(Engine::IIGOCommandController::CallOrigin_CLIENT);
#endif
}

void MenuFlowWindowViewController::startDownloadFlow()
{
    mEntitlement->localContent()->download("");
    if(mEntitlement->localContent()->installFlow()->canResume())
        mEntitlement->localContent()->installFlow()->resume();
}


void MenuFlowWindowViewController::startDLCDownloadFlow()
{
    bool f = false;
    QSet<QString> set;
    mEntitlement->localContent()->downloadAllPDLC(set, f, f);
}


void MenuFlowWindowViewController::triggerDownloadError()
{
    emit mEntitlement->localContent()->installFlow()->IContentInstallFlow_error(Downloader::DownloadError::HttpError, Services::Http403ClientErrorForbidden, "DEBUG", mEntitlement->contentConfiguration()->productId());
}


void MenuFlowWindowViewController::triggerDownloadErrorRetry()
{
    mEntitlement->localContent()->installFlow()->fakeDownloadErrorRetry(Downloader::DownloadError::HttpError, Services::Http403ClientErrorForbidden, "DEBUG", mEntitlement->contentConfiguration()->productId());
}


void MenuFlowWindowViewController::clearCompleteQueue()
{
    Origin::Engine::Content::ContentOperationQueueController *cQController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
    if (cQController)
    {
        cQController->clearCompleteList();
    }
}


void MenuFlowWindowViewController::startUpdate()
{
    mEntitlement->localContent()->update();
    if(mEntitlement->localContent()->installFlow()->canResume())
        mEntitlement->localContent()->installFlow()->resume();
}

void MenuFlowWindowViewController::startRepair()
{
    mEntitlement->localContent()->repair();
    if(mEntitlement->localContent()->installFlow()->canResume())
        mEntitlement->localContent()->installFlow()->resume();
}


void MenuFlowWindowViewController::showGameNetPromoter()
{
    ORIGIN_ASSERT(mEntitlement);
    Origin::Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductID);
    ORIGIN_ASSERT(ent);
    const bool initSetting = Services::readSetting(Services::SETTING_ShowGameNetPromoter);
    Services::writeSetting(Services::SETTING_ShowGameNetPromoter, true, Services::Session::SessionService::currentSession());
    NetPromoterViewControllerGame* np = new NetPromoterViewControllerGame(mEntitlement->contentConfiguration()->productId());
    np->show();
    Services::writeSetting(Services::SETTING_ShowGameNetPromoter, initSetting, Services::Session::SessionService::currentSession());
}


void MenuFlowWindowViewController::showOriginNetPromoter()
{
    const bool initSetting = Services::readSetting(Services::SETTING_ShowNetPromoter);
    Services::writeSetting(Services::SETTING_ShowNetPromoter, true, Services::Session::SessionService::currentSession());
    NetPromoterViewController* np = new NetPromoterViewController();
    np->show();
    Services::writeSetting(Services::SETTING_ShowNetPromoter, initSetting, Services::Session::SessionService::currentSession());
}


void MenuFlowWindowViewController::showPromoOriginStarted()
{
    ORIGIN_ASSERT(mEntitlement);
    ClientFlow::instance()->view()->showPromoDialog(PromoContext::OriginStarted, mEntitlement, true);
}


void MenuFlowWindowViewController::showPromoGameplayFinished()
{
    ORIGIN_ASSERT(mEntitlement);
    ClientFlow::instance()->view()->showPromoDialog(PromoContext::GameFinished, mEntitlement, true);
}


void MenuFlowWindowViewController::showPromoDownloadStarted()
{
    ORIGIN_ASSERT(mEntitlement);
    ClientFlow::instance()->view()->showPromoDialog(PromoContext::DownloadUnderway, mEntitlement, true);
}


void MenuFlowWindowViewController::showCloudSyncFlowWindows()
{
    ORIGIN_ASSERT(mEntitlement);
    // I know this leaks. This is for DEBUG ONLY. There is an understand with QA that after they use
    // the debug menu; they shouldn't continue to use origin in a normal way. We have to call new here so that
    // JS reponder will not crash for having an invalid callback.
    CloudSyncViewController* csvc = new CloudSyncViewController(mEntitlement, true);
    csvc->showCloudSync();
    csvc->showErrorPlay();
    csvc->showErrorRetry();
    csvc->showExternalSavesWarning();
    csvc->showManifestLockedFailure();
    csvc->showNoSpaceFailure(Engine::CloudSaves::SyncFailure(Engine::CloudSaves::NoSpaceFailure, 10000, 10000));
    csvc->showQuotaExceededFailure(Engine::CloudSaves::SyncFailure(Engine::CloudSaves::QuotaExceededFailure, 1000, 1000000));
    csvc->showSyncConflictDetected(QDateTime(), QDateTime());
    csvc->showSyncConflictDetected(QDateTime::currentDateTime(), QDateTime::currentDateTime());
    csvc->showUntrustedRemoteFilesDetected();
    csvc->showEmptyCloudWarning();
}


void MenuFlowWindowViewController::showPlayFlowWindows()
{
    ORIGIN_ASSERT(mEntitlement);
    PlayViewController* pvc = new PlayViewController(mEntitlement->contentConfiguration()->productId(), true);
    pvc->showStartingTimedTrialDialog();
    pvc->showUpdateExistsDialog();
    pvc->showPDLCDownloadsComplete();
    pvc->showDefaultLaunchDialog();
    pvc->showInstallFlowResuming();
    pvc->showOnlineRequiredForTrialDialog();
    pvc->showStartingTimedTrialDialog();
    pvc->showUpdateRequiredDialog();
    pvc->showUpdateCompleteDialog();
    pvc->showMultipleLaunchDialog();
    pvc->showOIGSettings();
    pvc->show3PDDPlayDialog();
    pvc->showGameUpdatingWarning();
    pvc->showPDLCDownloadingWarning();
    pvc->showPluginIncompatible();
    pvc->showSystemRequirementsNotMet("PLAY FLOW RESULT");
    pvc->showUpgradeSaveGameWarning();
    pvc->showFirstLaunchMessageDialog();
}


void MenuFlowWindowViewController::showDownloadFlowWindows()
{
    ORIGIN_ASSERT(mEntitlement);
    if(mEntitlement.isNull() == false)
    {
        // I know this leaks. This is for DEBUG ONLY. There is an understand with QA that after they use
        // the debug menu; they shouldn't continue to use origin in a normal way. We have to call new here so that
        // JS reponder will not crash for having an invalid callback.

        // Because we want to see the reaction to the 'not enough disc space' dialog, create the install flow.
        InstallerFlow* installerFlow = new InstallerFlow(mEntitlement);
        installerFlow->mVC->setInDebug(true);
        Origin::Downloader::InstallArgumentRequest installArgReq;
        installArgReq.contentDisplayName = mEntitlement->contentConfiguration()->displayName();
        installArgReq.installedSize = mEntitlement->contentConfiguration()->downloadSize();
        installArgReq.installPath = mEntitlement->contentConfiguration()->installationDirectory();
        installArgReq.isDip = true;
        installArgReq.isITO = false;
        installArgReq.productId = mEntitlement->contentConfiguration()->productId();
        installArgReq.isLocalSource = true;

        installerFlow->mVC->showPatchErrorDialog();
        installerFlow->mVC->showDLCOperationFinished();
        installerFlow->mVC->showFolderAccessErrorWindow(installArgReq);
        installerFlow->mVC->showInstallThroughSteam(InstallerViewController::ShowNothing);
        installerFlow->mVC->showInstallThroughSteam(InstallerViewController::ShowMonitored);
        installerFlow->mVC->showInstallThroughWindowsLive(InstallerViewController::ShowNothing);
        installerFlow->mVC->showInstallThroughWindowsLive(InstallerViewController::ShowMonitored);
        installerFlow->mVC->showInstallThrougOther(InstallerViewController::ShowNothing);
        installerFlow->mVC->showInstallThrougOther(InstallerViewController::ShowMonitored);

        Origin::Downloader::CancelDialogRequest cancelReq;
        cancelReq.contentDisplayName = mEntitlement->contentConfiguration()->displayName();
        cancelReq.isIGO = false;
        cancelReq.productId = mEntitlement->contentConfiguration()->productId();
        cancelReq.state = Downloader::kOperation_Download;
        installerFlow->mVC->showCancelDialog(cancelReq);
        cancelReq.state = Downloader::kOperation_Install;
        installerFlow->mVC->showCancelDialog(cancelReq);
        cancelReq.state = Downloader::kOperation_ITO;
        installerFlow->mVC->showCancelDialog(cancelReq);
        cancelReq.state = Downloader::kOperation_Update;
        installerFlow->mVC->showCancelDialog(cancelReq);
        cancelReq.state = Downloader::kOperation_Repair;
        installerFlow->mVC->showCancelDialog(cancelReq);
        cancelReq.state = Downloader::kOperation_Preload;
        installerFlow->mVC->showCancelDialog(cancelReq);

        cancelReq.state = Downloader::kOperation_Update;
        QStringList chilren;
        chilren << "Mr. Banana" << "Muffin" << "Claire" << "Some other DLC name";
        installerFlow->mVC->showCancelWithDLCDependentsDialog(cancelReq, chilren);

        const QString eulaOverride = Services::readSetting(Services::SETTING_DebugMenuEulaPathOverride).toString();
        if(eulaOverride.count())
        {
            Origin::Downloader::EulaStateRequest request;
            request.eulas.gameTitle = mEntitlement->contentConfiguration()->displayName();
            Downloader::Eula eula("Eula 1", eulaOverride, false, false);
            request.eulas.listOfEulas.push_front(eula);
            eula = Downloader::Eula("Eula 2", eulaOverride, false, false);
            request.eulas.listOfEulas.push_front(eula);
            installerFlow->mVC->showEulaSummaryDialog(request);
            request.isUpdate = true;
            installerFlow->mVC->showEulaSummaryDialog(request);
            request.isLocalSource = true;
            installerFlow->mVC->showEulaSummaryDialog(request);
            request.isPreload = true;
            installerFlow->mVC->showEulaSummaryDialog(request);
        }

        installerFlow->mVC->showInstallArguments(installArgReq);
        installerFlow->mVC->showInsufficientFreeSpaceWindow(installArgReq);
        installArgReq.isDip = false;
        installerFlow->mVC->showInsufficientFreeSpaceWindow(installArgReq);

        Origin::Downloader::InstallLanguageRequest installLangReq;
        QStringList lang;
        lang << "da_dk" << "nl_nl" << "en_gb" << "en_us" << "fr_fr" << "de_de" << "it_it" << "ja_jp" << "ko_kr" << "pl_pl" << "ru_ru" <<"sv_se" << "th_th" << "fi_fi" << "el_gr" << "hu_hu" << "no_no" << "pt_br" << "pt_pt" << "es_mx" << "es_es" << "cs_cz" << "zh_cn" << "zh_tw" << "tr_tr";
        installLangReq.additionalDownloadRequiredLanguages = lang;
        installLangReq.availableLanguages = lang;
        installLangReq.productId = mEntitlement->contentConfiguration()->productId();
        installerFlow->mVC->showLanguageSelection(installLangReq);
    }
}


void MenuFlowWindowViewController::showDownloadFlowErrorWindows()
{
    ORIGIN_ASSERT(mEntitlement);
    if(mEntitlement.isNull() == false)
    {
        InstallerViewController* ivc = new InstallerViewController(mEntitlement, true);
        ContentFlowController* cfc = MainFlow::instance()->contentFlowController();
        LocalContentViewController* lcvc = cfc->localContentViewController();
        const QString key = QString("%1:%2:%3");
        const QString source = "downloader";
        const QString offerId = mEntitlement->contentConfiguration()->productId();

        QPair<QString, QString> message = ivc->installErrorText(Downloader::DownloadError::FileOpen, Downloader::ContentInstallFlowState::kUnpacking, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::DownloadError::FileOpen)).arg(QString::number(Downloader::ContentInstallFlowState::kUnpacking)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::DownloadError::FileRead, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::DownloadError::FileRead)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::DownloadError::FileWrite, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::DownloadError::FileWrite)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::FlowError::OSRequirementNotMet, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::FlowError::OSRequirementNotMet)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::FlowError::ClientVersionRequirementNotMet, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::FlowError::ClientVersionRequirementNotMet)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::ProtocolError::EscalationFailureAdminRequired, Escalation::kCommandErrorNotElevatedUser, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::ProtocolError::EscalationFailureAdminRequired)).arg(QString::number(Escalation::kCommandErrorNotElevatedUser)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::ProtocolError::EscalationFailureUnknown, Escalation::kCommandErrorProcessExecutionFailure, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::ProtocolError::EscalationFailureUnknown)).arg(QString::number(Escalation::kCommandErrorProcessExecutionFailure)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::ProtocolError::ContentInvalid, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::ProtocolError::ContentInvalid)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::FlowError::JitUrlRequestFailed, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::FlowError::JitUrlRequestFailed)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::DownloadError::HttpError, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::DownloadError::HttpError)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::UnpackError::CRCFailed, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::UnpackError::CRCFailed)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::FlowError::DecryptionFailed, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::FlowError::DecryptionFailed)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::InstallError::Escalation, Escalation::kCommandErrorNotElevatedUser, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::InstallError::Escalation)).arg(QString::number(Escalation::kCommandErrorNotElevatedUser)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::InstallError::Escalation, Escalation::kCommandErrorUACRequestAlreadyPending, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::InstallError::Escalation)).arg(QString::number(Escalation::kCommandErrorUACRequestAlreadyPending)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::InstallError::Escalation, Escalation::kCommandErrorProcessExecutionFailure, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::InstallError::Escalation)).arg(QString::number(Escalation::kCommandErrorProcessExecutionFailure)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::FlowError::LanguageSelectionEmpty, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::FlowError::LanguageSelectionEmpty)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::ProtocolError::DownloadSessionFailure, 0, "");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::ProtocolError::DownloadSessionFailure)).arg(QString::number(0)).arg(offerId), source);

        message = ivc->installErrorText(Downloader::UnpackError::IOCreateOpenFailed, 0, "C:\\Path\\To\\Some\\File.dat");
        lcvc->showFloatingErrorWindow(message.first, message.second, key.arg(QString::number(Downloader::UnpackError::IOCreateOpenFailed)).arg(QString::number(0)).arg(offerId), source);
    }
}


void MenuFlowWindowViewController::showEntitlementWindows()
{
    ORIGIN_ASSERT(mEntitlement);
    if(mEntitlement.isNull() == false)
    {
        ContentFlowController* cfc = MainFlow::instance()->contentFlowController();
        LocalContentViewController* lcvc = cfc->localContentViewController();
        cfc->customizeBoxart(mEntitlement);
        lcvc->showTimedTrialEnding(9000);
        lcvc->showTimedTrialOffline(mEntitlement);
        lcvc->showSubscriptionRemoveError(mEntitlement->contentConfiguration()->masterTitleId(), Engine::Subscription::RedemptionErrorUnknown);
        lcvc->showSubscriptionDowngradeError(mEntitlement->contentConfiguration()->masterTitleId(), Engine::Subscription::RedemptionErrorUnknown);
        lcvc->showUpgradeWarning(mEntitlement);
        lcvc->showDLCUpgradeStandard(mEntitlement, "DLC NAME");
        lcvc->showDLCUpgradeWarning(mEntitlement, "FAKE_DLC_OFFER_ID", "DLC NAME");
        lcvc->showSubscriptionEntitleError(mEntitlement->contentConfiguration()->productId(), Engine::Subscription::RedemptionErrorUnknown);
        lcvc->showSubscriptionEntitleError(mEntitlement->contentConfiguration()->productId(), Engine::Subscription::RedemptionErrorAgeRestricted);
        lcvc->showSubscriptionUpgradeError(mEntitlement->contentConfiguration()->masterTitleId(), Engine::Subscription::RedemptionErrorUnknown);
        lcvc->showSubscriptionUpgradeError(mEntitlement->contentConfiguration()->masterTitleId(), Engine::Subscription::RedemptionErrorAgeRestricted);
        lcvc->showUACFailed();
        lcvc->showDownloadDebugInfo(mEntitlement);
        lcvc->showUpdateDebugInfo(mEntitlement);
        lcvc->showCanNotRestoreFromTrash(mEntitlement);
        lcvc->showEntitleFreeGameError(mEntitlement);
        lcvc->showGameNotInstalled(mEntitlement);
        lcvc->showGameProperties(mEntitlement);
        lcvc->showParentNotInstalledPrompt(mEntitlement);
        lcvc->showRemoveEntitlementPrompt(mEntitlement, true);
        lcvc->showRemoveEntitlementPrompt(mEntitlement, false);
        lcvc->showSignatureVerificationFailed();
        lcvc->showTrialEnding(9000);
        lcvc->showUninstallConfirmationForDLC(mEntitlement);
        lcvc->showUninstallConfirmation(mEntitlement);
        lcvc->showServerSettingsUnsavable();
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorTimedTrialAccount);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorTimedTrialSystem);
        lcvc->showSaveBackupRestoreWarning(mEntitlement);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorFolderInUse);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorGameTimeAccount);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorGameTimeSystem);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorCOPPADownload);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorCOPPAPlay);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorNothingUpdated);
        lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorForbidden);
        if (mEntitlement->parent())
        {
            lcvc->showError(mEntitlement, Engine::Content::LocalContent::ErrorParentNeedsUpdate);
        }
    }
}


void MenuFlowWindowViewController::showRTPWindows()
{
    ORIGIN_ASSERT(mEntitlement);
    if(mEntitlement.isNull() == false)
    {
        // I know this leaks. This is for DEBUG ONLY. There is an understand with QA that after they use
        // the debug menu; they shouldn't continue to use origin in a normal way. We have to call new here so that
        // JS reponder will not crash for having an invalid callback.
        RTPViewController* rtp = new RTPViewController(this);
        rtp->showEarlyTrialDisabledByAdminDialog(mEntitlement);
        rtp->showFreeTrialExpiredDialog(mEntitlement);
        rtp->showEarlyTrialExpiredDialog(mEntitlement);
        rtp->showGameShouldBeInstalledErrorDialog(mEntitlement->contentConfiguration()->displayName());
        rtp->showGameUnreleasedErroDialog(mEntitlement->contentConfiguration()->displayName(), LocalizedDateFormatter().format(QDateTime::currentDateTime().date(), LocalizedDateFormatter::LongFormat));
        rtp->showGameUnreleasedErroDialog(mEntitlement->contentConfiguration()->displayName(), "");
        rtp->showOfflineErrorDialog();
        rtp->showNotEntitledDialog(Origin::Engine::LoginController::currentUser()->eaid(), mEntitlement->contentConfiguration()->displayName(), RTPViewController::ShowOfAgeContent, " blah");
        rtp->showNotEntitledDialog(Origin::Engine::LoginController::currentUser()->eaid(), mEntitlement->contentConfiguration()->displayName(), RTPViewController::ShowNothing, " blah");
        rtp->showWrongCodeDialog(mEntitlement->contentConfiguration()->displayName());
        rtp->showBusyDialog();
        rtp->showOnlineRequiredForTrialDialog();
        rtp->showOnlineRequiredForEarlyTrialDialog(mEntitlement->contentConfiguration()->displayName());
        rtp->showSubscriptionMembershipExpired(mEntitlement, false);
        rtp->showSubscriptionMembershipExpired(mEntitlement, true);
        rtp->showSubscriptionOfflinePlayExpired();
        rtp->showEntitlementRetiredFromSubscription(mEntitlement, false);
        rtp->showEntitlementRetiredFromSubscription(mEntitlement, true);
        rtp->showSubscriptionNotAvailableOnPlatform(Engine::LoginController::currentUser()->eaid(), mEntitlement->contentConfiguration()->displayName(), false, "");
        rtp->showSubscriptionNotAvailableOnPlatform(Engine::LoginController::currentUser()->eaid(), mEntitlement->contentConfiguration()->displayName(), false, mEntitlement->contentConfiguration()->masterTitleId());

        rtp->showIncorrectEnvironment(mEntitlement->contentConfiguration()->displayName(), "FC.QA", RTPViewController::ShowDeveloperMode);
        rtp->showIncorrectEnvironment(mEntitlement->contentConfiguration()->displayName(), "FC.QA", RTPViewController::ShowNothing);
    }
}


void MenuFlowWindowViewController::showITOWindows()
{
    if(mEntitlement.isNull() == false)
    {
        ITOViewController ito(mEntitlement->contentConfiguration()->displayName(), this, true);
        Downloader::InstallArgumentRequest installArgReq;
        installArgReq.contentDisplayName = mEntitlement->contentConfiguration()->displayName();
        installArgReq.installedSize = mEntitlement->contentConfiguration()->downloadSize();
        installArgReq.installPath = mEntitlement->contentConfiguration()->installationDirectory();
        installArgReq.isDip = true;
        installArgReq.isITO = false;
        installArgReq.productId = mEntitlement->contentConfiguration()->productId();
        installArgReq.isLocalSource = true;
        ito.showPrepare(installArgReq);
        ito.showInstallFailed();
        ito.showFailedNoGameName();
        ito.showFreeToPlayFailNoEntitlement();
        ito.showRedeem();
        ito.showWrongCode();
        ito.showDiscCheck();
        ito.showNotReadyToDownload();
        ito.showInsertDisk(2, 3);
        ito.showWrongDisk(2);
        ito.showReadDiscError();
        ito.showDownloadPreparing();
        ito.showCancelConfirmation();
    }
}


void MenuFlowWindowViewController::showFirstLaunchWindows()
{
    FirstLaunchFlow flf;
    flf.showFirstTimeSettingsDialog();
}


void MenuFlowWindowViewController::showMessageAreaWindows()
{
    EmailVerificationMessageAreaViewController evma;
    evma.showMessageSentWindow();
    evma.showMessageErrorWindow();
}


void MenuFlowWindowViewController::showGroipWindows()
{
    // Get Current UIScope
    UIScope scope = (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive()) ? IGOScope : ClientScope;

    // Find a User's Group
    QString groupGuid = "";
    QString groupName = "";
    Origin::Engine::UserRef user = Origin::Engine::LoginController::currentUser();
    Chat::OriginConnection *connection = user->socialControllerInstance()->chatConnection();
    const Chat::ConnectedUser *connectedUser = connection->currentUser();
    const Chat::ChatGroups* groups = connectedUser->chatGroups();
    if (groups)
    {
        const QSet<Origin::Chat::ChatGroup*> set = groups->getGroupList();

        for (QSet<Chat::ChatGroup*>::const_iterator i = set.cbegin(); i != set.cend(); ++i)
        {
            // grab a group form the list
            Chat::ChatGroup* group = *i;

            if (group)
            {
                // cache the group info
                groupGuid = group->getGroupGuid();
                groupName = group->getName();
                break;
            }
        }
        ORIGIN_ASSERT(groupGuid != "");
        ORIGIN_ASSERT(groupName != "");
    }

    ClientViewController* cvc = ClientFlow::instance()->view();
    if (cvc)
    {
        using namespace Chat;
        cvc->showYouNeedFriendsDialog(scope);

        // TODO:
        // Remove all of this RemotUser code and create a GroipViewController to handle
        // all the dialogs currently contained in Client View Controler
        // This new controller should have access to all the dialogs for the debug menu
        Connection* connection = Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection();
        JabberID* jidAH = new JabberID("1111111111", "origin.dm.com");
        JabberID* jidSR = new JabberID("2222222222", "origin.dm.com");
        JabberID* jidDJ = new JabberID("3333333333", "origin.dm.com");
        JabberID* jidKS = new JabberID("4444444444", "origin.dm.com");
        JabberID* jidJM = new JabberID("5555555555", "origin.dm.com");
        JabberID* jidDV = new JabberID("6666666666", "origin.dm.com");
        JabberID* jidHS = new JabberID("7777777777", "origin.dm.com");

        RemoteUser *AlonzoHarris = connection->remoteUserForJabberId(jidAH->asBare());
        RemoteUser *ShortRound = connection->remoteUserForJabberId(jidSR->asBare());
        RemoteUser *DrJones = connection->remoteUserForJabberId(jidDJ->asBare());
        RemoteUser *KevinSmith = connection->remoteUserForJabberId(jidKS->asBare());
        RemoteUser *JasonMewes = connection->remoteUserForJabberId(jidJM->asBare());
        RemoteUser *DarthVader = connection->remoteUserForJabberId(jidDV->asBare());
        RemoteUser *HanSolo = connection->remoteUserForJabberId(jidHS->asBare());
        
        qint32 caps = 0;

        caps |= 0x01; // CAPS
        caps |= 0x02; //CHATSTATES
        caps |= 0x04; //DISCOINFO

        // Done want this group to be MUC capable
        ShortRound->debugSetupRemoteUser("Short Round", caps);
        DrJones->debugSetupRemoteUser("Dr. Jones", caps);
        KevinSmith->debugSetupRemoteUser("Kevin Smith", caps);
        JasonMewes->debugSetupRemoteUser("Jason Mewes", caps);


        caps |= 0x08; //MUC
        caps |= 0x10; //VOIP


        AlonzoHarris->debugSetupRemoteUser("Alonzo Harris", caps);
        DarthVader->debugSetupRemoteUser("Darth Vader", caps);
        HanSolo->debugSetupRemoteUser("Han Solo", caps);

        //show Group Invite Sent Dialogs
        {
            // Invite one success
            QList<Origin::Chat::RemoteUser*> users;
            users.append(AlonzoHarris);
            cvc->showGroupInviteSentDialog(users, scope);
        }
        {
            // Invite multipe success
            QList<Origin::Chat::RemoteUser*> users;
            users.append(AlonzoHarris);
            users.append(DarthVader);
            users.append(HanSolo);
            cvc->showGroupInviteSentDialog(users, scope);
        }

        {
            // Invite all fail
            QList<Origin::Chat::RemoteUser*> users;
            users.append(DrJones);
            users.append(ShortRound);
            cvc->showGroupInviteSentDialog(users, scope);
        }
        {
            // Invite some fail
            QList<Origin::Chat::RemoteUser*> users;
            users.append(KevinSmith);
            users.append(JasonMewes);
            users.append(AlonzoHarris);
            users.append(DarthVader);
            users.append(HanSolo);
            cvc->showGroupInviteSentDialog(users, scope);
        }

        
        cvc->showTransferOwnershipSuccessDialog("Death Star Command", DarthVader->originId(), scope);

        cvc->showTransferOwnershipFailureDialog(HanSolo->originId(), scope);

        // We currently are not using this dialog
        //cvc->showAllFriendsInGroupDialog(scope);

        // TODO: figure out how to call the actual AllFriendsInGroupDialog if needed        
    }

    CreateGroupViewController* cgvc = new CreateGroupViewController();
    if (cgvc)
    {
        cgvc->debugCreateGroupFail();
    }

    EditGroupViewController* egvc = new EditGroupViewController(groupName, groupGuid);
    if (egvc)
    {
        egvc->debugEditGroupFail();
    }

    DeleteGroupViewController* dgvc = new DeleteGroupViewController("", "");
    if (dgvc)
    {
        dgvc->debugDeleteGroupFail();
    }

    RemoveGroupUserViewController* rguvc = new RemoveGroupUserViewController("", "", "");
    if (rguvc)
    {
        rguvc->debugRemoveUserFail();
    }

    AcceptGroupInviteViewController* agivc = new AcceptGroupInviteViewController("", "", "");
     if (agivc)
     {
         agivc->debugAcceptInviteFail();
     }

    SocialViewController* svc = ClientFlow::instance()->socialViewController();
    if (svc)
    {
        svc->acceptInviteFailed();
        svc->rejectInviteFailed();
        svc->onFailedToEnterRoom();
        svc->onMessageGroupRoomInviteFailed();
    }
}

void MenuFlowWindowViewController::showGroipChatError()
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    // Get Current UIScope
    UIScope scope = (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive()) ? IGOScope : ClientScope;

    Engine::Social::ConversationManager* conversationManager = Engine::LoginController::currentUser()->socialControllerInstance()->conversations();
    QList<QSharedPointer<Engine::Social::Conversation>> allConversations = conversationManager->allConversations();

    if (allConversations.isEmpty())
    {
        QString heading = tr("NO ACTIVE CONVERSATIONS FOUND");
        QString description = tr("You must start at least one conversation to use this debug menu option.");
        UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;

        UIToolkit::OriginWindow* noConversationsWindow = 
            UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

        noConversationsWindow->setTitleBarText(tr("DEBUG_ERROR WINDOW"));

        if (scope == IGOScope)
        {
            noConversationsWindow->configForOIG(true);
            Engine::IGOController::instance()->igowm()->addPopupWindow(noConversationsWindow, Engine::IIGOWindowManager::WindowProperties());
        }
        else
        {
            noConversationsWindow->showWindow();
        }
        return;
    }


    QList<QSharedPointer<Engine::Social::Conversation>>::iterator i;
    for (i = allConversations.begin(); i != allConversations.end(); ++i)
    {
        (*i)->debugEvents();
    }
#endif
}

void MenuFlowWindowViewController::showSaveBackupRestore()
{
    SaveBackupRestoreViewController* sb = new SaveBackupRestoreViewController(mEntitlement, NULL, true);
    sb->showSaveBackupRestoreFailed();
    sb->showSaveBackupRestoreSuccessful();
}

void MenuFlowWindowViewController::showCrashWindow()
{
    QString program = "OriginCrashReporter.exe";
    QStringList arguments;
    arguments << "-fake";
    QProcess::startDetached(program, arguments, ".");
}

void MenuFlowWindowViewController::showBroadcastWindows()
{
    // I know this leaks. This is for DEBUG ONLY. There is an understand with QA that after they use
    // the debug menu; they shouldn't continue to use origin in a normal way.
#ifdef ORIGIN_PC
    BroadcastViewController* bvc = new BroadcastViewController(NULL, ClientScope, true);
    bvc->showIntro(Engine::IIGOCommandController::CallOrigin_CLIENT);
    bvc->showNetworkError(Engine::IIGOCommandController::CallOrigin_CLIENT);
    bvc->showNotSupportedError(Engine::IIGOCommandController::CallOrigin_CLIENT);
    bvc->showTwitchLogin(Engine::IIGOCommandController::CallOrigin_CLIENT);
    bvc->showSettings(Engine::IIGOCommandController::CallOrigin_CLIENT);
    bvc->showBroadcastStoppedError();
#endif
}

}
}
