///////////////////////////////////////////////////////////////////////////////
// InstallerFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "flows/InstallerFlow.h"
#include "InstallerViewController.h"
#include "services/debug/DebugService.h"
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/GamesController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/PlayFlow.h"
#include "engine/ito/InstallFlowManager.h"
#include "TelemetryAPIDLL.h"
#include "services/log/LogService.h"
#include "MainFlow.h"
#include "RTPFlow.h"
#include "ITOFlow.h"
#include "ClientFlow.h"
#include "ITEUIManager.h"
#include "ITOViewController.h"
#include "OriginNotificationDialog.h"
#include "OriginToastManager.h"
#include "engine/igo/IGOController.h"
#include "engine/igo/IGOQWidget.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"

namespace Origin
{
namespace Client
{

InstallerFlow::InstallerFlow(Engine::Content::EntitlementRef entitlement)
: mVC(NULL),
  mEntitlement(entitlement)
{
    mVC = new InstallerViewController(mEntitlement);
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(stopFlow()), this, SLOT(onViewStopFlow()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(stopFlowForSelfAndDependents()), this, SLOT(onViewStopFlowForSelfAndDependents()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(continueInstall()), this, SLOT(onContinueInstall()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(clearRTPLaunch()), this, SLOT(onClearRTPLaunch()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(installArgumentsCreated(const QString&, Origin::Downloader::InstallArgumentResponse)), 
        this, SLOT(onInstallArgumentsCreated(const QString&, Origin::Downloader::InstallArgumentResponse)));
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(languageSelected(const QString&, Origin::Downloader::EulaLanguageResponse)), 
        this, SLOT(onLanguageSelected(const QString&, Origin::Downloader::EulaLanguageResponse)));
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(eulaStateDone(const QString&, Origin::Downloader::EulaStateResponse)), 
        this, SLOT(onEulaStateDone(const QString&, Origin::Downloader::EulaStateResponse)));
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(retryDiskSpace(const Origin::Downloader::InstallArgumentRequest&)), 
        this, SLOT(onPresentInstallInfo(const Origin::Downloader::InstallArgumentRequest&)), Qt::QueuedConnection);
}


InstallerFlow::~InstallerFlow()
{
    if(mVC)
    {
        ORIGIN_VERIFY_DISCONNECT(mVC, 0, this, 0);
        delete mVC;
        mVC = NULL;
    }
}


void InstallerFlow::onViewStopFlow()
{
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();

    // I wish we could move this to the backend somewhere... someday..
    bool isHeadAndBusy = false;
	
	// must protect against user logging out which results in a null queueController
	Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
	if (queueController)
		isHeadAndBusy = queueController->indexInQueue(mEntitlement) == 0 && queueController->isHeadBusy();

    if (!flow)
        return;

    if (flow->isDynamicDownload() && isHeadAndBusy)
    {
        if (!flow->isUpdate())
            return;

        if ((flow->getFlowState() != Downloader::ContentInstallFlowState::kPostTransfer) // this prevents cancel from update eula dialog from getting us stuck (EBIBUGS-27906)
         && (flow->getFlowState() != Downloader::ContentInstallFlowState::kPendingEulaLangSelection)) // this prevents the flow from getting stuck when rejecting EULAs *before* an update (ORPLAT-3048)
            return;
    }

    // Cancel() calls canCancel()
    flow->cancel();
}


void InstallerFlow::onViewStopFlowForSelfAndDependents()
{
    QList<Engine::Content::EntitlementRef> dependentChildren = dependentActiveOperationChildren();
    foreach(Engine::Content::EntitlementRef child, dependentChildren)
    {
        if(!child.isNull() && child->localContent() && child->localContent()->installFlow())
        {
            child->localContent()->installFlow()->cancel();
        }
    }
    onViewStopFlow();
}


void InstallerFlow::onFlowCanceled()
{
    InstallerFlowResult result;
    result.result = FLOW_FAILED;
    result.productId = mEntitlement->contentConfiguration()->productId();
    emit finished(result);
}


void InstallerFlow::start()
{
    if(mEntitlement.isNull() == false && mEntitlement->localContent() && mEntitlement->localContent()->installFlow())
    {
        Origin::Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(canceling()), this, SLOT(onFlowCanceled()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(stopped()), this, SLOT(onFlowCanceled()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(pendingInstallArguments(const Origin::Downloader::InstallArgumentRequest&)), this, SLOT(onPresentInstallInfo(const Origin::Downloader::InstallArgumentRequest&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(pendingInstallLanguage(const Origin::Downloader::InstallLanguageRequest&)), this, SLOT(onPresentEulaLanguageSelection(const Origin::Downloader::InstallLanguageRequest&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(pendingEulaState(const Origin::Downloader::EulaStateRequest&)), this, SLOT(onPresentEula(const Origin::Downloader::EulaStateRequest&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(pendingCancelDialog(const Origin::Downloader::CancelDialogRequest&)), this, SLOT(onPresentCancelDialog(const Origin::Downloader::CancelDialogRequest&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), this, SLOT(onInstallError(qint32, qint32, QString, QString)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(pending3PDDDialog(const Origin::Downloader::ThirdPartyDDRequest&)), this, SLOT(onPresent3PDDInstallDialog(const Origin::Downloader::ThirdPartyDDRequest&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(contentPlayable()), this, SLOT(onPlayable()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(pendingContentExit()), this, SLOT(onPendingContentExit()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onStateChanged(Origin::Downloader::ContentInstallFlowState)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(flow, SIGNAL(warn(qint32, QString)), this, SLOT(onInstallWarning(qint32, QString)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->playFlow().data(), SIGNAL(stopped()), this, SLOT(updateView()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->playFlow().data(), SIGNAL(started()), this, SLOT(updateView()), Qt::QueuedConnection);

        flow->onInstallerFlowCreated();
    }
}


void InstallerFlow::updateView()
{
    if(mEntitlement.isNull() || mEntitlement->localContent() == NULL || mEntitlement->localContent()->installFlow() == NULL)
        return;
    Downloader::CancelDialogRequest request;
    request.contentDisplayName = mEntitlement->contentConfiguration()->displayName();
    request.isIGO = (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive());
    request.state = mEntitlement->localContent()->installFlow()->operationType();
    request.productId = mEntitlement->contentConfiguration()->productId();
    onPresentCancelDialog(request, true);
}


void InstallerFlow::onPresentInstallInfo(const Origin::Downloader::InstallArgumentRequest& request)
{
    // Parameters to be returned from the view
    Downloader::InstallArgumentRequest overrideRequest = request;
    qint64 initalSize = overrideRequest.installedSize;

    // In cases where "install size" property is specified, we want to use that for required space size
    if(request.isITO || request.isDip)
    {
        qint64 overrideInstallSize = mEntitlement->contentConfiguration()->downloadSize();
        if(overrideInstallSize > 0)
        {
            overrideRequest.installedSize = overrideInstallSize;
        }
    }
    if(!overrideRequest.isDip)
        overrideRequest.installedSize = overrideRequest.installedSize * 2;

    if(request.isITO)
    {
        bool bypass_dialog = false;
        if (request.useDefaultInstallOptions)   // for Free to Play Optimization
        {
            // need to check first if we have enough disk space.  because if not, we can't bypass
            const int64_t free_disk_space = Services::PlatformService::GetFreeDiskSpace(overrideRequest.installPath);
            if(overrideRequest.installedSize < free_disk_space)
            {
                bypass_dialog = true;

                // then we need to set up the install arguments like the dialog would have
                Origin::Engine::InstallFlowManager::GetInstance().SetStateCommand("Next");
                Origin::Downloader::InstallArgumentResponse args;
                args.optionalComponentsToInstall = 0xffffffff;
                args.installDesktopShortCut = true;
                args.installStartMenuShortCut = true;

                mEntitlement->localContent()->installFlow()->setInstallArguments(args);
            }
        }

        if (!bypass_dialog)
            ITEUIManager::GetInstance()->viewController()->showPrepare(overrideRequest);
        //ITO is still using its old UI/logic so it will return its arguments in the ITEPrepare.cpp
    }
    else
    {
		const int64_t free_disk_space = Services::PlatformService::GetFreeDiskSpace(overrideRequest.installPath);

		if (free_disk_space == -1)	// this means that we don't have access to the install path so report it properly and cancel the install (EBIBUGS-25611)
		{
			ORIGIN_LOG_EVENT << "Folder Access Error detected.";
			Services::PlatformService::PrintDriveInfoToLog();
			mVC->showFolderAccessErrorWindow(overrideRequest);
		}
		else if(overrideRequest.installedSize < free_disk_space)
        {
            // If there was a install size override we want to only show was origin is going to be doing.
            // However, we wanted to compare the size needed with the disk space
            overrideRequest.installedSize = initalSize;
            // PDLC and updates just pass through here to present a "Insufficient Disk Space" error dialog if the 
            // user doesn't have enough disk space to install.  There is no other information that requires user input. 
            // so we just construct an empty response and push it back to the install flow
            if(request.isPdlc || request.isUpdate)
            {
                onInstallArgumentsCreated(request.productId, Origin::Downloader::InstallArgumentResponse());
            }
            else
            {
                mVC->showInstallArguments(overrideRequest);
            }
        }
        else
        {
            ORIGIN_LOG_EVENT << "Insufficient disk space detected.";
            Services::PlatformService::PrintDriveInfoToLog();
            mVC->showInsufficientFreeSpaceWindow(overrideRequest);
            // Tell the VC the original size. It's might pass the install arguments when the user changes
            // their install directory. We need to redo the calculate of what we want to show for the size.
            mVC->setInstallArgSize(initalSize);
        }
    }
}


void InstallerFlow::onInstallArgumentsCreated(const QString& productId, Origin::Downloader::InstallArgumentResponse args)
{
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();
    if (flow)
        flow->setInstallArguments(args);
}


void InstallerFlow::onPresentEulaLanguageSelection(const Origin::Downloader::InstallLanguageRequest& input)
{
    mVC->showLanguageSelection(input);
}


void InstallerFlow::onLanguageSelected(const QString& productId, Origin::Downloader::EulaLanguageResponse response)
{
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();
    if(flow && flow->isActive())
        flow->setEulaLanguage(response);
}


void InstallerFlow::onPresentEula(const Origin::Downloader::EulaStateRequest& request)
{
    if(request.eulas.listOfEulas.count())
    {
        mVC->showEulaSummaryDialog(request);
    }
}


void InstallerFlow::onEulaStateDone(const QString& productId, Origin::Downloader::EulaStateResponse eulaState)
{
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();
    if(flow && flow->isActive())
        flow->setEulaState(eulaState);
}


void InstallerFlow::onPresentCancelDialog(const Origin::Downloader::CancelDialogRequest& request, const bool& updateOnly)
{
    const InstallerViewController::installerUIStates uiState = updateOnly ? InstallerViewController::UpdateOnly : InstallerViewController::ShowRegular;
    if(request.state == Downloader::kOperation_Update)
    {
        const QList<Engine::Content::EntitlementRef> dependentChildren = dependentActiveOperationChildren();
        if(dependentChildren.length())
        {
            QStringList childrenNames;
            foreach(Engine::Content::EntitlementRef ent, dependentChildren)
            {
                childrenNames += ent->contentConfiguration()->displayName();
            }
            mVC->showCancelWithDLCDependentsDialog(request, childrenNames, uiState);
            return;
        }
    }

    mVC->showCancelDialog(request, uiState);
}


QList<Engine::Content::EntitlementRef> InstallerFlow::dependentActiveOperationChildren() const
{
    const QList<Engine::Content::EntitlementRef> children = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController()->children_of(mEntitlement);
    QList<Engine::Content::EntitlementRef> ret;
    foreach(Engine::Content::EntitlementRef ent, children)
    {
        // If DLC can't continue downloading without parent update finishing
        if(ent->contentConfiguration()->softwareBuildMetadata().mbDLCRequiresParentUpToDate)
        {
            ret.append(ent);
        }
    }
    return ret;
}


void InstallerFlow::onClearRTPLaunch()
{
    if(MainFlow::instance()->rtpFlow()->getRtpLaunchActive(mEntitlement->contentConfiguration()->contentId()))
    {
        MainFlow::instance()->rtpFlow()->clearRtpLaunchInfo(mEntitlement->contentConfiguration()->contentId());
    }
}


void InstallerFlow::onContinueInstall()
{
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();
    if(flow && flow->canResume())
        flow->resume();
}


void InstallerFlow::onInstallError(qint32 type, qint32 code, QString context, QString productId)
{
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();

    //the error is coming from ITO flow and it's a read disc type so it will get handled by ITEDiskInstallProgress::onError because it needs the extra dialog
    //to allow the user to retry/cancel
    // or the error is a result of a missing language and we need to handle the order of the error dialogs in the ITO flow itself
    if(flow && flow->isUsingLocalSource() && (ITOFlow::isReadDiscErrorType(type) || (type == Downloader::FlowError::LanguageSelectionEmpty)))
        return;

    //flash the main window if are not it focus
    if(ClientFlow::instance())
    {
        ClientFlow::instance()->alertMainWindow();
    }

    if(type == Downloader::ProtocolError::DestageFailed)	// special case for handling update apply errors
    {
        mVC->showPatchErrorDialog();
    }
    else
    if (flow)
    {
        QString title, text;
#if 0  // def _DEBUG // turn off so that there isn't confusion between the error message the testers report and what the devs see in debug
        title = "INTERNAL DOWNLOADER ERROR";
        text = description;	// for debug, show default internal english text
#else
        QPair<QString, QString> message = mVC->installErrorText(type, code, context);
        title = message.first;
        text = message.second;
#endif

        if(flow->suppressDialogs() == false)
        {
            //type is the actual error code that we want, code returns things like HTTP error, etc. but in most internal errors, it returns 0
            ORIGIN_LOG_ERROR << QString("Download Error: code = %1, description = %2").arg(code).arg(text);
            const QString key = QString("%1&%2&%3").arg(type).arg(code).arg(productId);
            // EBIBUGS-28116: Instead of showing this is on VC we need to move the errors to a higher level. The VC get deleted
            // when the flow ends so the slot isn't picked up to show the OER.
            const QString source = "downloader";
            MainFlow::instance()->contentFlowController()->localContentViewController()->showFloatingErrorWindow(title, text, key, source);
        }
        else //error occurred during auto-update, so we don't show error dialog, just pause
        {
            if(flow->canPause())
                flow->pause(true /*autoresume*/);
            else
            {
                ORIGIN_LOG_ERROR << "Download error occurred for [" << productId << "] but unable to pause. error = " << text;
            }
        }
    }
}


void InstallerFlow::onInstallWarning(qint32 code, QString description)
{
    Downloader::IContentInstallFlow* flow = dynamic_cast<Downloader::IContentInstallFlow*>(sender());
    if(flow)
    {
#ifdef _DEBUG
        const QString productId = flow->getContent()->contentConfiguration()->productId();
        //type is the actual error code that we want, code returns things like HTTP error, etc. but in most internal errors, it returns 0
        ORIGIN_LOG_ERROR << QString("Download Error: code = %1, description = %2").arg(code).arg(description);
        mVC->pushAndShowErrorWindow(tr("ebisu_client_error_uppercase"), description, productId + QString::number(code));
#endif
    }
}


void InstallerFlow::onPresent3PDDInstallDialog(const Origin::Downloader::ThirdPartyDDRequest& /*request*/)
{
    const InstallerViewController::installerUIStates monitored = mEntitlement->contentConfiguration()->monitorInstall() ? InstallerViewController::ShowMonitored : InstallerViewController::ShowNothing;
    switch(mEntitlement->contentConfiguration()->partnerPlatform())
    {
    case Services::Publishing::PartnerPlatformSteam:
        mVC->showInstallThroughSteam(monitored);
        break;
    case Services::Publishing::PartnerPlatformGamesForWindows:
        mVC->showInstallThroughWindowsLive(monitored);
        break;
    case Origin::Services::Publishing::PartnerPlatformOtherExternal:
    default:
        mVC->showInstallThrougOther(monitored);
        break;
    }
}


void InstallerFlow::onPlayable()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR << "ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR << "OriginToastManager not available";
        return;
    }

    const bool isDLC = mEntitlement->parent().isNull() ? false : true;
    const QString displayName = mEntitlement->contentConfiguration()->displayName();
    const int numSiblingsWithActiveOperation = isDLC ? Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController()->children_of(mEntitlement->parent()).length() : 0;


    // Base games show the notification if they installed successfully, which means that they are playable.
    if(mEntitlement->localContent()->installed() == false)
    {
        ORIGIN_LOG_ERROR << displayName << ":" << mEntitlement->contentConfiguration()->productId() << " not installed.";
        return;
    }

    if(mEntitlement->localContent()->playable() == false)
    {
        ORIGIN_LOG_ERROR << displayName << ":" << mEntitlement->contentConfiguration()->productId() << " not playable.";
        return;
    }

    // DLC shows the notification if their base game is running.
    if(isDLC && mEntitlement->parent()->localContent()->playing() == false)
    {
        ORIGIN_LOG_ERROR << displayName << ":" << mEntitlement->contentConfiguration()->productId() << " (DLC) finished, but base game isn't being played.";
        return;
    }

    // DLC shows the notification if their base game is running.
    if(isDLC && numSiblingsWithActiveOperation > 0)
    {
        ORIGIN_LOG_ERROR << displayName << ":" << mEntitlement->contentConfiguration()->productId() << " (DLC) siblings are still in the queue.";
        return;
    }

    // DLC isn't directly playable so it needs a different message based on catalog addonDeploymentStrategy
    if(isDLC)
    {
        switch (mEntitlement->contentConfiguration()->addonDeploymentStrategy())
        {
        case Origin::Services::Publishing::AddonDeploymentStrategyHotDeployable:
            otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), QString("<b>%0</b>").arg(displayName) , tr("ebisu_client_notifications_pdlc_hot_deployed"));
            break;

        case Origin::Services::Publishing::AddonDeploymentStrategyReloadSave:
            otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), QString("<b>%0</b>").arg(displayName), tr("ebisu_client_notifications_pdlc_reload_save"));
            break;

        case Origin::Services::Publishing::AddonDeploymentStrategyRestartGame:
        default:
            otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(),
                tr("ebisu_client_notification_additional_content_for_game_downlaoded").arg(mEntitlement->parent()->contentConfiguration()->displayName()),
                tr("ebisu_client_notification_potentially_restart_game_to_use_dlc"));
            break;
        }
        mVC->showDLCOperationFinished();
    }
    else 
    {
        if(mEntitlement->localContent()->installFlow() && mEntitlement->localContent()->installFlow()->isDynamicDownload())
        {
            otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(),
                "<b>"+tr("ebisu_client_game_ready_to_play").arg(mEntitlement->contentConfiguration()->displayName())+"</b>", 
                tr("ebisu_client_game_still_downloading"));
        }
        else
        {
            otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), QString("<b>%0</b>").arg(displayName), (tr("ebisu_client_notifications_is_ready_to_play")));       	
        }
    }
}


void InstallerFlow::onPendingContentExit()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR << "ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR << "OriginToastManager not available";
        return;
    }
    otm->showToast(Services::SETTING_NOTIFY_FINISHEDINSTALL.name(), "<b>"+tr("ebisu_client_notification_game_ready_to_install")+"</b>", tr("ebisu_client_notification_restart_game_to_update"));
}


void InstallerFlow::onStateChanged(Origin::Downloader::ContentInstallFlowState newState)
{
    updateView();
    Downloader::IContentInstallFlow* flow = mEntitlement->localContent()->installFlow();

    if(flow)
    {
        switch(newState)
        {
        case Downloader::ContentInstallFlowState::kTransferring:
            {
                // Show promo browser when the download begins
                const Engine::Content::EntitlementRef ent = flow->getContent();
                Downloader::ContentInstallFlowState prev = flow->getPreviousState();

                // Only show the download-underway promo once per download. If
                // the download is paused and resumed, including because the
                // client was shut down and restarted, we don't show the promo
                // again.  Avoid showing promos for updates and repairs as well.
                if(ent &&
                   prev != Downloader::ContentInstallFlowState::kPaused && 
                   !flow->isUpdate() &&
                   !flow->isRepairing())
                {
                    ClientFlow::instance()->showPromoDialog(PromoContext::DownloadUnderway, ent);
                }
            }
            break;

        case Downloader::ContentInstallFlowState::kReadyToInstall:
            // If previous state is kInstalling, the installer failed or was exited and we shouldn't show another notification.
            if(flow->getPreviousState() != Origin::Downloader::ContentInstallFlowState::kInstalling)
            {
                QString displayName = "";
                if(mEntitlement.isNull() == false)
                {
                    displayName = mEntitlement->contentConfiguration()->displayName();
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

                UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_FINISHEDDOWNLOAD.name(), QString("<b>%0</b>").arg(displayName),(tr("ebisu_client_notifications_is_ready_to_install")));

                if (toast)
                {
                    // IGO
                    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), Origin::Engine::IGOController::instance(), SLOT(igoShowDownloads()), Qt::QueuedConnection);
                    
                    // Desktop
                    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), ClientFlow::instance()->view(), SLOT(showDownloadProgressDialog()), Qt::QueuedConnection);
                }
            }
            break;

        case Origin::Downloader::ContentInstallFlowState::kCompleted:
            {
                onFlowComplete();
            }
            break;

        case Origin::Downloader::ContentInstallFlowState::kInvalid:
        case Origin::Downloader::ContentInstallFlowState::kError:
            {
                // We don't want to show the Download Failed notification during the ITO. EBIBUGS-18251
                if(flow->isUsingLocalSource() == false)
                {
                    QString displayName = "";
                    if(mEntitlement.isNull() == false)
                    {
                        displayName = mEntitlement->contentConfiguration()->displayName();
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

                    UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_DOWNLOADERROR.name(), QString("<b>%0</b>").arg(displayName), (tr("ebisu_client_download_failed")));
                    if (toast)
                    {
                        // IGO
                        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), Origin::Engine::IGOController::instance(), SLOT(igoShowDownloads()), Qt::QueuedConnection);

                         // Desktop
                        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), ClientFlow::instance()->view(), SLOT(showDownloadProgressDialog()), Qt::QueuedConnection);
                    }
                }
            }
            break;

        case Origin::Downloader::ContentInstallFlowState::kCanceling:
            onFlowComplete();
            break;

        default:
            break;
        }
    }
}

void InstallerFlow::onFlowComplete()
{
    InstallerFlowResult result;
    result.result = FLOW_SUCCEEDED;
    result.productId = mEntitlement->contentConfiguration()->productId();
    emit finished(result);
}

}
}
