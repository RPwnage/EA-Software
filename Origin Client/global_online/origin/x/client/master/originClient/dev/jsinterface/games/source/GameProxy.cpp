#include "GameProxy.h"

#include "engine/content/UpdateCheckFlow.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/GamesController.h"
#include "EntitlementInstallFlowProxy.h"
#include "EntitlementCloudSaveProxy.h"
#include "EntitlementCloudSaveBackupProxy.h"
#include "EntitlementRemoteSyncProxy.h"
#include "NonOriginGameProxy.h"
#include "BoxartProxy.h"
#include "ContentFlowController.h"
#include "MainFlow.h"
#include "LocalContentViewController.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "engine/social/SocialController.h"
#include "utilities/LocalizedContentString.h"
#include "LocalizedDateFormatter.h"

//MY TODO: remove once QA has verified
#include "services/log/LogService.h"

namespace Origin
{
namespace Client
{

namespace JsInterface
{
    static const QString ErrorFlowNotActive("ERROR_FLOW_NOT_ACTIVE");
    static const QString ErrorSuccess("ERROR_SUCCESS");

    GameProxy::GameProxy(Origin::Engine::Content::EntitlementRef entitlement, QObject *parent) :
        QObject(parent),
        mEntitlement(entitlement),
        mCloudSaves(NULL),
        mNonOriginGame(NULL),
        mBoxart(NULL),
        mInstallFlowProxy(new EntitlementInstallFlowProxy(this, mEntitlement))
    {
        mBoxart = new BoxartProxy(this, mEntitlement);

        if (mEntitlement->localContent()->cloudContent()->hasCloudSaveSupport())
        {
            mCloudSaves = new EntitlementCloudSaveProxy(this, mEntitlement);

            // emit changed() whenever our cloud save properties change
            ORIGIN_VERIFY_CONNECT(mCloudSaves, SIGNAL(changed()), this, SLOT(onStateChanged()));
        }

        if (mEntitlement->contentConfiguration()->isNonOriginGame())
        {
            if (!mNonOriginGame)
            {
                mNonOriginGame = new NonOriginGameProxy(this, mEntitlement);
            }
        }

        ORIGIN_VERIFY_CONNECT(entitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onStateChanged()));
        ORIGIN_VERIFY_CONNECT(entitlement->localContent(), SIGNAL(progressChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onProgressChanged()));
        Origin::Downloader::IContentInstallFlow* installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow)
        {
            ORIGIN_VERIFY_CONNECT_EX(installFlow, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), this, SLOT(onOperationError(qint32, qint32, QString, QString)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(installFlow, SIGNAL(IContentInstallFlow_errorRetry(qint32, qint32, QString, QString)), this, SLOT(onOperationError(qint32, qint32, QString, QString)), Qt::QueuedConnection);
        }

        ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(changed(QJsonObject)), parent, SLOT(onGameProxyChanged(QJsonObject)), Qt::DirectConnection);
        ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(progressChanged(QJsonObject)), parent, SLOT(onGameProxyProgressChanged(QJsonObject)), Qt::DirectConnection);
        ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(operationError(QJsonObject)), parent, SLOT(onGameProxyOperationFailed(QJsonObject)), Qt::DirectConnection);
    }

    QVariantMap GameProxy::getCustomBoxArtInfo(QVariantMap args)
    {  
        if (mBoxart)
        {
            return mBoxart->getCustomBoxArtInfo();
        }

        return QVariantMap();
    }

    QVariantMap GameProxy::getCustomBoxArtSize(QVariantMap args)
    {
        QJsonObject obj = QJsonObject::fromVariantMap(args);
        QVariantMap retObj;
        int height = obj["tileHeight"].toInt();
        int width = obj["tileWidth"].toInt();
        if (mBoxart) {
            retObj["imageTop"] = mBoxart->imageTop(height);
            retObj["imageLeft"] = mBoxart->imageLeft(width);
            retObj["imageScale"] = mBoxart->imageScale(width);
            return retObj;
        }

        return QVariantMap();
    }

    QVariantMap GameProxy::startDownload(QVariantMap args)
    {
        Origin::Engine::Content::ContentController* cc = mEntitlement->contentController();
        if (!cc)
        {
            // TBD: telemetry here?
            return QVariantMap();
        }

        Origin::Engine::Content::ContentController::DownloadPreflightingResult result;
        cc->doDownloadPreflighting(*mEntitlement->localContent(), result);
        if (result >= Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD)
            mEntitlement->localContent()->download("");

        return QVariantMap();
    }

    QVariantMap GameProxy::pauseDownload(QVariantMap args)
    {
        pauseCurrentOperation(downloadOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::resumeDownload(QVariantMap args)
    {
        resumeCurrentOperation(downloadOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::cancelDownload(QVariantMap args)
    {
        cancelCurrentOperation(downloadOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::moveToTopOfQueue(QVariantMap args)
    {
        Origin::Engine::Content::ContentOperationQueueController *cQController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
        cQController->pushToTop(mEntitlement);
        return QVariantMap();
    }


    QVariantMap GameProxy::checkForUpdateAndInstall(QVariantMap args)
    {
        Origin::Engine::Content::UpdateCheckFlow* updateCheck = new Origin::Engine::Content::UpdateCheckFlow(mEntitlement);
        ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(updateAvailable()), this, SLOT(onUpdateCheckCompletedForInstall()));
        ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(noUpdateAvailable()), this, SLOT(onUpdateCheckCompletedForInstall()));
        updateCheck->begin();

        return QVariantMap();
    }

    QVariantMap GameProxy::installUpdate(QVariantMap args)
    {
        mEntitlement->localContent()->update();
        return QVariantMap();
    }

    QVariantMap GameProxy::pauseUpdate(QVariantMap args)
    {
        pauseCurrentOperation(updateOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::resumeUpdate(QVariantMap args)
    {
        resumeCurrentOperation(updateOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::cancelUpdate(QVariantMap args)
    {
        cancelCurrentOperation(updateOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::play(QVariantMap args)
    {
        MainFlow::instance()->contentFlowController()->startPlayFlow(mEntitlement->contentConfiguration()->productId(), false);
        return QVariantMap();
    }

    QVariantMap GameProxy::customizeBoxArt(QVariantMap args)
    {
        Client::MainFlow::instance()->contentFlowController()->customizeBoxart(mEntitlement);
        return QVariantMap();
    }

    QVariantMap GameProxy::installParent(QVariantMap args)
    {
        Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showParentNotInstalledPrompt(mEntitlement);
        return QVariantMap();
    }

    QVariantMap GameProxy::install(QVariantMap args)
    {
        mEntitlement->localContent()->install();
        return QVariantMap();
    }

    QVariantMap GameProxy::pauseInstall(QVariantMap args)
    {
        pauseCurrentOperation(installOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::resumeInstall(QVariantMap args)
    {
        resumeCurrentOperation(installOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::cancelInstall(QVariantMap args)
    {
        cancelCurrentOperation(installOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::uninstall(QVariantMap args) 
    {
#ifdef ORIGIN_MAC
        Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showUninstallConfirmation(mEntitlement);
#else
#ifdef BCTODO
        GetTelemetryInterface()->Metric_GAME_UNINSTALL_START(mEntitlement->contentConfiguration()->productId().toUtf8().data());
#endif
        mEntitlement->localContent()->unInstall();
#endif
        return QVariantMap();
    }

    QVariantMap GameProxy::cancelCloudSync(QVariantMap args)
    {
        if(mCloudSaves && mCloudSaves->syncOperation())
        {
            mCloudSaves->syncOperation()->cancel();
        }
        return QVariantMap();
    }

    QVariantMap GameProxy::pollCurrentCloudUsage(QVariantMap args)
    {
        if (mCloudSaves)
        {
            Engine::CloudSaves::RemoteUsageMonitor::instance()->checkCurrentUsage(mEntitlement);
        }
        return QVariantMap();
    }

    QVariantMap GameProxy::restoreLastCloudBackup(QVariantMap args)
    {
        if(mCloudSaves && mCloudSaves->lastBackupObj())
        {
            mCloudSaves->lastBackupObj()->restore();
        }
        return QVariantMap();
    }

    QVariantMap GameProxy::removeFromLibrary(QVariantMap args)
    {
        if(mNonOriginGame)
        {
            mNonOriginGame->removeFromLibrary();
        }
        return QVariantMap();
    }

    QString GameProxy::releaseStatus()
    {
        if (mEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Released)
        {
            return "AVAILABLE";
        }

        if (mEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Unreleased)
        {
            return "UNRELEASED";
        }

        if (mEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload)
        {
            return "PRELOAD";
        }

        if (mEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Expired)
        {
            return "EXPIRED";
        }

        // we should never get here
        return "RELEASED";
    }

    EntitlementInstallFlowProxy * GameProxy::downloadOperation()
    {
        Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow && (installFlow->operationType() == Downloader::kOperation_Download || installFlow->operationType() == Downloader::kOperation_Preload))
        {
            return mInstallFlowProxy;
        }
        return NULL;
    }

    EntitlementInstallFlowProxy * GameProxy::updateOperation()
    {
        Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow && installFlow->operationType() == Downloader::kOperation_Update)
        {
            return mInstallFlowProxy;
        }
        return NULL;
    }

    EntitlementInstallFlowProxy * GameProxy::unpackOperation()
    {
        Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow && installFlow->operationType() == Downloader::kOperation_Unpack)
        {
            return mInstallFlowProxy;
        }
        return NULL;
    }

    EntitlementInstallFlowProxy * GameProxy::repairOperation()
    {
        Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow && installFlow->operationType() == Downloader::kOperation_Repair)
        {
            return mInstallFlowProxy;
        }
        return NULL;
    }

    EntitlementInstallFlowProxy * GameProxy::installOperation()
    {
        Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow && ((installFlow->operationType() == Downloader::kOperation_Install) || (installFlow->operationType() == Downloader::kOperation_ITO)))
        {
            return mInstallFlowProxy;
        }
        return NULL;
    }

    EntitlementInstallFlowProxy *GameProxy::currentOperation()
    {
        Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow && installFlow->operationType() != Downloader::kOperation_None)
        {
            return mInstallFlowProxy;
        }
        return NULL;
    }

    bool GameProxy::repairSupported()
    {
        return mEntitlement->localContent()->repairable();
    }

    bool GameProxy::updateSupported()
    {
        return mEntitlement->localContent()->updatable() && !mEntitlement->localContent()->repairing();
    }

    bool GameProxy::isITOActive()
    {
        bool itoActive = false;
        if (mEntitlement->localContent() && mEntitlement->localContent()->installFlow())
            itoActive = mEntitlement->localContent()->installFlow()->isITO();
        return itoActive;
    }

    QVariantMap GameProxy::repair(QVariantMap args)
    {
        // we run through the update check flow just to make sure we are repairing to the most recent version
        // technically it's not necessary otherwise.
        Origin::Engine::Content::UpdateCheckFlow* updateCheck = new Origin::Engine::Content::UpdateCheckFlow(mEntitlement);
        ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(updateAvailable()), this, SLOT(onUpdateCheckCompletedForRepair()));
        ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(noUpdateAvailable()), this, SLOT(onUpdateCheckCompletedForRepair()));
        updateCheck->begin();

        return QVariantMap();
    }

    void GameProxy::onUpdateCheckCompletedForRepair()
    {
        Origin::Engine::Content::UpdateCheckFlow* updateCheck = dynamic_cast<Origin::Engine::Content::UpdateCheckFlow*>(sender());

        if(updateCheck != NULL)
        {
            updateCheck->deleteLater();
        }

        // We don't care if the update is available or not, we just ran it to ensure we are repairing to the most recent version...
        mEntitlement->localContent()->repair();
    }

    QVariantMap GameProxy::pauseRepair(QVariantMap args)
    {
        pauseCurrentOperation(repairOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::resumeRepair(QVariantMap args)
    {
        resumeCurrentOperation(repairOperation());
        return QVariantMap();
    }

    QVariantMap GameProxy::cancelRepair(QVariantMap args)
    {
        cancelCurrentOperation(repairOperation());
        return QVariantMap();
    }

    void GameProxy::onUpdateCheckCompletedForInstall()
    {
        Origin::Engine::Content::UpdateCheckFlow* updateCheck = dynamic_cast<Origin::Engine::Content::UpdateCheckFlow*>(sender());

        if(updateCheck != NULL)
        {
            updateCheck->deleteLater();
        }

        // we run through the update check flow just to make sure we are updating to the most recent version
        // Whether an update actually exists or not is handled in the normal install flow, and the user feedback
        // flows out of the install flow state changes, so we can't skip it here even if no update exists.
        installUpdate(QVariantMap());
    }

    const QJsonObject GameProxy::createStateObj()
    {
        QJsonObject obj, progressObj, dialogObj;
        Origin::Engine::Content::ContentOperationQueueController *cQController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();

        dialogObj["showCancel"] = false;
        dialogObj["showEula"] = false;
        dialogObj["showDownloadInfo"] = false;
        obj["dialogInfo"] = dialogObj;

        progressObj = createProgressStateObj();
        obj["progressInfo"] = progressObj["progressInfo"];
        obj["hidden"] = Origin::Engine::Content::GamesController::currentUserGamesController()->gameIsHidden(mEntitlement);
        obj["favorite"] = Origin::Engine::Content::GamesController::currentUserGamesController()->gameIsFavorite(mEntitlement);
        obj["productId"] = mEntitlement->contentConfiguration()->productId();
        obj["playable"] = mEntitlement->localContent()->playable();
        obj["downloadable"] = mEntitlement->localContent()->downloadable();
        obj["pausable"] = mInstallFlowProxy->pausable();
        obj["resumable"] = mInstallFlowProxy->resumable();
        obj["cancellable"] = mInstallFlowProxy->cancellable();
        obj["installable"] = mEntitlement->localContent()->installable();
        obj["uninstallable"] = mEntitlement->localContent()->unInstallable();
        obj["installing"] = installOperation() != NULL;
        obj["updating"] = updateOperation() != NULL;
        obj["downloading"] = downloadOperation() != NULL;
        obj["repairing"] = repairOperation() != NULL;
        obj["playing"] = mEntitlement->localContent()->playing();
        obj["uninstalling"] = mEntitlement->localContent()->unInstalling();
        obj["updateAvailable"] = mEntitlement->localContent()->updateAvailable();

        obj["repairSupported"] = repairSupported();
        obj["updateSupported"] = updateSupported();

        obj["queueIndex"] = cQController->indexInQueue(mEntitlement);
        obj["completedIndex"] = cQController->indexInCompleted(mEntitlement);
        obj["queueSkippingEnabled"] = cQController->queueSkippingEnabled(mEntitlement);
        obj["queueSize"] = cQController->entitlementsQueued().length();
        obj["installed"] = mEntitlement->localContent()->installState(true) == Engine::Content::LocalContent::ContentInstalled;
        obj["ito"] = isITOActive();
        obj["releaseStatus"] = releaseStatus();

        obj["availableUpdateVersion"] = mEntitlement->contentConfiguration()->version();
        obj["updateIsMandatory"] = mEntitlement->localContent()->treatUpdatesAsMandatory();
       
		//if we've set a time stamp lets send over the date time as a string (if we haven't initialInstalledTimestamp will be 0)
        if(mEntitlement->localContent()->initialInstalledTimestamp()) 
        {
            obj["initialInstalledTimestamp"] = QDateTime::fromTime_t(mEntitlement->localContent()->initialInstalledTimestamp()).toString();
        }
        else
        {   //this will show up as null on the javascript side
            obj["initialInstalledTimestamp"] = QJsonValue();
        }

        if (mCloudSaves)
        {
            qint64 currentUsage = Engine::CloudSaves::RemoteUsageMonitor::instance()->usageForEntitlement(mEntitlement);
            qint64 maximumUsage = mEntitlement->localContent()->cloudContent()->maximumCloudSaveLocalSize();

            QJsonObject cloudObj;
            cloudObj["currentUsage"] = currentUsage;
            cloudObj["maximumUsage"] = maximumUsage;
            cloudObj["usageDisplay"] = LocalizedContentString().makeProgressDisplayString(currentUsage, maximumUsage);
            cloudObj["lastBackupValid"] = mCloudSaves->lastBackupObj() != NULL;
            obj["cloudSaves"] = cloudObj;
        }

       return obj;
    }

    const QJsonObject GameProxy::createProgressStateObj()
    {
        QJsonObject obj, progressObj;

        obj["productId"] = mEntitlement->contentConfiguration()->productId();
        progressObj["active"] = currentOperation() != NULL;
        progressObj["progress"] = mInstallFlowProxy->progress().toFloat();
        progressObj["phase"] = mInstallFlowProxy->phase();
        progressObj["phaseDisplay"] = mInstallFlowProxy->phaseDisplay();
        progressObj["progressState"] = mInstallFlowProxy->progressState();
        progressObj["secondsRemaining"] = mInstallFlowProxy->secondsRemaining().toLongLong();
        progressObj["playableAt"] = mInstallFlowProxy->playableAt();
        progressObj["shouldLightFlag"] = mInstallFlowProxy->shouldLightFlag();
        Downloader::IContentInstallFlow * installFlow = mEntitlement->localContent()->installFlow();
        if (installFlow)
        {
            progressObj["errorRetryCount"] = installFlow->getErrorRetryCount();
            progressObj["timeLeft"] = LocalizedDateFormatter().formatShortInterval_hs(installFlow->progressDetail().mEstimatedSecondsRemaining);
        }
        obj["progressInfo"] = progressObj;

        return obj;
    }

    const QJsonObject GameProxy::createDownloadQueueObj()
    {
        QJsonObject obj;

        Origin::Engine::Content::ContentOperationQueueController *cQController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();

        obj["productId"] = mEntitlement->contentConfiguration()->productId();
        if (cQController)
        {
            obj["queueIndex"] = cQController->indexInQueue(mEntitlement);
            obj["completedIndex"] = cQController->indexInCompleted(mEntitlement);
            obj["queueSkippingEnabled"] = cQController->queueSkippingEnabled(mEntitlement);
            obj["queueSize"] = cQController->entitlementsQueued().length();
        }

        return obj;
    }

    void GameProxy::onStateChanged()
    {
        emit changed(createStateObj());
    }

    void GameProxy::onProgressChanged()
    {
        emit progressChanged(createProgressStateObj());
    }

    void GameProxy::onOperationError(qint32 type, qint32 code, QString context, QString productId)
    {
        // TODO: Maybe pass something more useful for operation error?
        emit operationError(createProgressStateObj());
    }

    QString GameProxy::pauseCurrentOperation(EntitlementInstallFlowProxy *flow)
    {
        QString result = ErrorFlowNotActive;
        if(flow)
        {
            flow->pause();
            result = ErrorSuccess;
        }

        return result;
    }

    QString GameProxy::resumeCurrentOperation(EntitlementInstallFlowProxy *flow)
    {
        QString result = ErrorFlowNotActive;
        if(flow)
        {
            flow->resume();
            result = ErrorSuccess;
        }

        return result;
    }

    QString GameProxy::cancelCurrentOperation(EntitlementInstallFlowProxy *flow)
    {
        QString result = ErrorFlowNotActive;
        if(flow)
        {
            flow->cancel();
            result = ErrorSuccess;
        }

        return result;
    }
}
}
}