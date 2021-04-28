/////////////////////////////////////////////////////////////////////////////
// MultiContentProgressIntegrator.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MultiContentProgressIntegrator.h"
#include "LocalizedContentString.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/LocalContent.h"
#include "services/debug/DebugService.h"
#include "Qt/origintransferbar.h"

namespace Origin
{

namespace Client
{

MultiContentProgressIntegrator::MultiContentProgressIntegrator(QList<Engine::Content::EntitlementRef> entitlementList, const QString& id, QObject* parent, const bool& inDebug)
: QObject(parent)
, mEntitlementsInProgress(entitlementList)
, mId(id)
, mInDebug(inDebug)
{
    if(mInDebug && entitlementList.count())
    {
        QTimer* timer = new QTimer(this);
        timer->setInterval(2000);
        ORIGIN_VERIFY_CONNECT(timer, SIGNAL(timeout()), this, SLOT(onDebugTimeout()));
        timer->start();
        return;
    }

    Engine::Content::EntitlementRef displayEnt;
    foreach(Engine::Content::EntitlementRef ent, mEntitlementsInProgress)
    {
        Downloader::IContentInstallFlow* installFlow = ent->localContent()->installFlow();
        if(installFlow)
        {
            ORIGIN_VERIFY_CONNECT(
                installFlow, SIGNAL(progressUpdate(Origin::Downloader::ContentInstallFlowState, Origin::Downloader::InstallProgressState)),
                this, SLOT(onStateChanged(const Origin::Downloader::ContentInstallFlowState&)));
            ORIGIN_VERIFY_CONNECT(
                installFlow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)),
                this, SLOT(onStateChanged(const Origin::Downloader::ContentInstallFlowState&)));
            // (For DLC) At this point it doesn't really matter which entitlement we show. It could change at any time with queue head changing
            displayEnt = ent;
        }
    }

    // update the gui based on the gathered progress state information
    if(displayEnt.isNull() == false && displayEnt->localContent() && displayEnt->localContent()->installFlow())
    {
        onStateChanged(displayEnt->localContent()->installFlow()->getFlowState());
    }
}


void MultiContentProgressIntegrator::onStateChanged(const Origin::Downloader::ContentInstallFlowState& state) 
{
    QJsonObject updateContent;
    switch(state.getValue())
    {
    case Downloader::ContentInstallFlowState::kReadyToStart:
    case Downloader::ContentInstallFlowState::kCanceling:
    case Downloader::ContentInstallFlowState::kCompleted:
        {
            Downloader::IContentInstallFlow* installerFlow = dynamic_cast<Downloader::IContentInstallFlow*>(sender());
            if(installerFlow && installerFlow->getContent())
            {
                ORIGIN_VERIFY_DISCONNECT(
                    installerFlow, SIGNAL(progressUpdate(Origin::Downloader::ContentInstallFlowState, Origin::Downloader::InstallProgressState)),
                    this, SLOT(onStateChanged(const Origin::Downloader::ContentInstallFlowState&)));
                ORIGIN_VERIFY_DISCONNECT(
                    installerFlow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)),
                    this, SLOT(onStateChanged(const Origin::Downloader::ContentInstallFlowState&)));
                mEntitlementsInProgress.removeOne(installerFlow->getContent());
            }
        }
        break;
    default:
        break;
    }

    if (mEntitlementsInProgress.isEmpty())
    {
        emit PDLCDownloadsComplete(mId);
    }
    else
    {
        // relying on constructors to zero out all fields here
        Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
        Engine::Content::EntitlementRef currentEnt;
        Downloader::InstallProgressState progressState;
        for (QList<Engine::Content::EntitlementRef>::iterator ent = mEntitlementsInProgress.begin(); ent != mEntitlementsInProgress.end(); ++ent )
        {
            currentEnt = (*ent);
            if(currentEnt && currentEnt->localContent() && currentEnt->localContent()->installFlow())
            {
                progressState = currentEnt->localContent()->installFlow()->progressDetail();
            }
            if(queueController->head() == currentEnt)
            {
                break;
            }
        }

        if(currentEnt.isNull())
        {
            return;
        }

        float displayProgress = progressState.mProgress * 100.0f;
        displayProgress = (displayProgress > 100.0f) ? 100.0f : ((displayProgress < 0.0f) ? 0.0f : displayProgress);
        Downloader::ContentInstallFlowState phase = possibleMaskInstallState(currentEnt);
        QString operationStatus;

        switch(progressState.progressState(phase.getValue()))
        {
        case Downloader::InstallProgressState::kProgressState_Paused:
            operationStatus = "paused";
            break;
        case Downloader::InstallProgressState::kProgressState_Indeterminate:
            operationStatus = "complete";
            break;
        case Downloader::InstallProgressState::kProgressState_Active:
            operationStatus = "active";
            break;
        default:
            break;
        }

        LocalizedContentString localizedContent;
        QJsonObject obj = progressObj(currentEnt->contentConfiguration()->displayName(), localizedContent.operationDisplay(currentEnt->localContent()->installFlow()->operationType()),
                                      operationStatus, tr("ebisu_client_percent_notation").arg(static_cast<int>(displayProgress)), localizedContent.makeProgressRateDisplayString(progressState.mBytesPerSecond),
                                      localizedContent.makeTimeRemainingString(progressState.mEstimatedSecondsRemaining), localizedContent.operationPhaseDiplay(currentEnt->localContent()->installFlow()->operationType(), phase.getValue()),
                                      localizedContent.makeProgressDisplayString(progressState.mBytesDownloaded, progressState.mTotalBytes));

        emit updateValues(mId, obj);
    }
}


QJsonObject MultiContentProgressIntegrator::progressObj(const QString& title, const QString& operation, const QString& operationStatus, const QString& percent, 
                        const QString& rate, const QString& timeRemaining, const QString& phase, const QString& completedPercent)
{
    QJsonObject updateContent;
    updateContent["title"] = title;
    updateContent["operation"] = operation;
    updateContent["operationstatus"] = operationStatus;
    updateContent["percent"] = percent;
    updateContent["rate"] = rate;
    updateContent["timeremaining"] = timeRemaining;
    updateContent["phase"] = phase;
    updateContent["completed"] = completedPercent;
    return updateContent;
}


// !!! This should be identical to EntitlementInstallFlowProxy::possibleMaskInstallState !!!
Downloader::ContentInstallFlowState MultiContentProgressIntegrator::possibleMaskInstallState(Engine::Content::EntitlementRef entitlement)
{
    Downloader::ContentInstallFlowState flowState = Downloader::ContentInstallFlowState::kInvalid;
    if (entitlement->localContent()->installFlow())
    {
        Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
        flowState = entitlement->localContent()->installFlow()->getFlowState();
        if(queueController->indexInQueue(entitlement) == 0 && flowState == Downloader::ContentInstallFlowState::kEnqueued)
        {
            if(entitlement->localContent()->installFlow()->isEnqueuedAndPaused())
            {
                flowState = Downloader::ContentInstallFlowState::kPaused;
            }
            else
            {
                flowState = Downloader::ContentInstallFlowState::kInitializing;
            }
        }
    }
    return flowState;
}


void MultiContentProgressIntegrator::onDebugTimeout()
{
    static int progress = 0;
    if(progress == 100)
    {
        progress = 0;
    }
    progress += 1;
    LocalizedContentString localizedContent;
    Downloader::ContentOperationType operationType = Downloader::kOperation_Download;
    Downloader::ContentInstallFlowState phase(Downloader::ContentInstallFlowState::kTransferring);
    QJsonObject obj = progressObj(mEntitlementsInProgress.last()->contentConfiguration()->displayName(), localizedContent.operationDisplay(operationType),
        "active", tr("ebisu_client_percent_notation").arg(progress), localizedContent.makeProgressRateDisplayString(10000),
        localizedContent.makeTimeRemainingString(20000), localizedContent.operationPhaseDiplay(operationType, phase.getValue()),
        localizedContent.makeProgressDisplayString(20000, 8000000));

    emit updateValues(mId, obj);
}

} // namespace Origin
} // namespace Client