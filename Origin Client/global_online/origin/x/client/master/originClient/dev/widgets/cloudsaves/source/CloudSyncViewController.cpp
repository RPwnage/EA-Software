/////////////////////////////////////////////////////////////////////////////
// CloudSyncViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CloudSyncViewController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"
#include "utilities/LocalizedDateFormatter.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "ui_CloudDialog.h"
#include "TelemetryAPIDLL.h"
#include "CloudSyncView.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "DialogController.h"
#include "PlayView.h"
#include "LocalizedContentString.h"

namespace Origin
{
namespace Client
{

static const QString CLOUD_SYNC_DIALOG_OVERRIDE_ID = "_CloudSync";
CloudSyncViewController::CloudSyncViewController(Engine::Content::EntitlementRef entitlement, const bool& inDebug, QObject* parent)
: QObject(parent)
, mEntitlement(entitlement)
, mInDebug(inDebug)
, mWindowShowing("")
{

}


CloudSyncViewController::~CloudSyncViewController()
{
    DialogController::instance()->closeGroup(rejectGroup());
}


void CloudSyncViewController::emitStopFlow()
{
    mWindowShowing = "";
    emit stopFlow();
}


bool CloudSyncViewController::isWindowShowing() const
{
    return mWindowShowing.count();
}


QString CloudSyncViewController::rejectGroup() const
{
    return mInDebug ? "" : mEntitlement->contentConfiguration()->productId() + "_CLOUDFLOW";
}


void CloudSyncViewController::showCloudSync()
{
    mWindowShowing = mEntitlement->contentConfiguration()->productId() + CLOUD_SYNC_DIALOG_OVERRIDE_ID;
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-progress", 
        tr("ebisu_client_launching"), "",
        tr("ebisu_client_cancel"), QJsonObject(), this, "emitStopFlow");
    mt.overrideId = mWindowShowing;
    mt.rejectGroup = rejectGroup();

    LocalizedContentString localizedContent;
    QJsonObject content;
    content["title"] = mEntitlement->contentConfiguration()->displayName();
    content["operation"] = tr("ebisu_client_cloud_sync");
    content["operationstatus"] = "active";
    content["percent"] = tr("ebisu_client_percent_notation").arg(0);
    content["rate"] = "";
    content["timeremaining"] = "";
    content["phase"] = "";
    content["completed"] = "";
    mt.contentInfo = content;
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::onCloudProgressChanged(const float& progress, const qint64& completedBytes, const qint64& totalBytes)
{
    // Do not need to update a window if it's not showing
    if(mWindowShowing.compare(mEntitlement->contentConfiguration()->productId() + CLOUD_SYNC_DIALOG_OVERRIDE_ID) != 0)
        return;
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-progress", 
        tr("ebisu_client_launching"), "",
        tr("ebisu_client_cancel"), QJsonObject(), this, "emitStopFlow");
    mt.overrideId = mWindowShowing;
    mt.rejectGroup = rejectGroup();

    LocalizedContentString localizedContent;
    QJsonObject content;
    content["title"] = mEntitlement->contentConfiguration()->displayName();
    content["operation"] = tr("ebisu_client_cloud_sync");
    content["operationstatus"] = "active";
    content["percent"] = tr("ebisu_client_percent_notation").arg(static_cast<int>(progress * 100.0f));
    content["rate"] = "";
    content["timeremaining"] = "";
    content["phase"] = "";
    content["completed"] = localizedContent.makeProgressDisplayString(completedBytes, totalBytes);
    mt.contentInfo = content;
    DialogController::instance()->updateMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::onRemoteSyncFinished()
{
    ORIGIN_VERIFY_DISCONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(cloudSaveProgress(float, qint64, qint64)), this, SLOT(onCloudProgressChanged(const float&, const qint64&, const qint64&)));
    ORIGIN_VERIFY_DISCONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(remoteSyncFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(onRemoteSyncFinished()));
    mWindowShowing = "";
    DialogController::instance()->closeMessageDialog(DialogController::SPA, mEntitlement->contentConfiguration()->productId() + CLOUD_SYNC_DIALOG_OVERRIDE_ID);
}


void CloudSyncViewController::showQuotaExceededFailure(const Origin::Engine::CloudSaves::SyncFailure& failureReason)
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showQuotaExceededFailure";
    const unsigned int bytesInMB = 1024 * 1024;
    const int storageLimit = failureReason.limitBytes()/bytesInMB;
    const int memoryExceeded = failureReason.wantedBytes()/bytesInMB;
    const QString displayName = mEntitlement->contentConfiguration()->displayName();
    const QString offerId = mEntitlement->contentConfiguration()->productId();
    GetTelemetryInterface()->Metric_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY(offerId.toUtf8().data(), failureReason.wantedBytes()/bytesInMB);

    QJsonArray commandbuttons;
    commandbuttons.append(PlayView::commandLinkObj("continueFlow", "otkicon-cloud", tr("ebisu_client_cloud_error_storage_limit_free_up_title"), tr("ebisu_client_cloud_error_storage_limit_free_up_text").arg("<b>"+displayName+"</b>").arg(memoryExceeded - storageLimit)));
    commandbuttons.append(PlayView::commandLinkObj("disableCloudStorage", "otkicon-close", tr("ebisu_client_cloud_error_storage_limit_disable_title"), tr("ebisu_client_cloud_error_storage_limit_disable_text").arg(tr("application_name"))));

    QJsonObject content;
    content["commandbuttons"] = commandbuttons;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-commandbuttons", 
        tr("ebisu_client_cloud_error_storage_limit_title"), tr("ebisu_client_cloud_error_storage_limit_text").arg("<b>"+displayName+"</b>").arg(storageLimit).arg(memoryExceeded), 
        tr("ebisu_client_cancel"), content, this, "onCommandbuttonsDone");
    mt.rejectGroup = rejectGroup();

    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::onCommandbuttonsDone(QJsonObject obj)
{
    mWindowShowing = "";
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const QString selectedOption = Services::JsonValueValidator::validate(obj["selectedOption"]).toString();
        if(selectedOption.compare("continueFlow", Qt::CaseInsensitive) == 0)
        {
            emit continueFlow();
        }
        else if(selectedOption.compare("disableCloudStorage", Qt::CaseInsensitive) == 0)
        {
            emit disableCloudStorage();
        }
        else if(selectedOption.compare("UseRemoteState", Qt::CaseInsensitive) == 0)
        {
            emit stateChosen(Engine::CloudSaves::UseRemoteState);
        }
        else if(selectedOption.compare("UseLocalState", Qt::CaseInsensitive) == 0)
        {
            emit stateChosen(Engine::CloudSaves::UseLocalState);
        }
        else
        {
            emit stopFlow();
        }
    }
    else
    {
        emit stopFlow();
    }
}


void CloudSyncViewController::showNoSpaceFailure(const Origin::Engine::CloudSaves::SyncFailure& failureReason)
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showNoSpaceFailure";
    QString failedDrive = failureReason.failedPath();

#ifdef Q_OS_WIN
    if (failedDrive[1] == ':')
    {
        // This is a standard Windows path, not a UNC path
        // Show them just the drive letter
        failedDrive = failedDrive.left(2);
    }
#endif

    const unsigned int bytesInMB = 1024 * 1024;
    qint64 additionalBytesNeeded = failureReason.wantedBytes() - failureReason.limitBytes();

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_cloud_error_full_drive_title"), 
        tr("ebisu_client_cloud_error_full_drive_text").arg(failedDrive).arg(additionalBytesNeeded/bytesInMB),
        tr("ebisu_client_cloud_retry_sync_button"), tr("ebisu_client_cancel"), "no", QJsonObject(), this, "onRetryPullOnAccept");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::showManifestLockedFailure()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showManifestLockedFailure";
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_cloud_error_locked_title"), 
        tr("ebisu_client_cloud_error_locked_text"),
        tr("ebisu_client_cloud_error_sync_error_play_anyway_button"), tr("ebisu_client_cancel"), "no", QJsonObject(), this, "onContinueOnAccept");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::onContinueOnAccept(QJsonObject obj)
{
    mWindowShowing = "";
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        emit continueFlow();
    }
    else
    {
        emit stopFlow();
    }
}


void CloudSyncViewController::showErrorPlay()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    // For some reason the cloud sync engine is emiting to show this several times. Let's protect ourselves a bit.
    if(mWindowShowing.compare(mEntitlement->contentConfiguration()->productId() + "_showErrorPlay") == 0)
        return;
    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showErrorPlay";
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_cloud_error_sync_error_title"), 
        tr("ebisu_client_cloud_error_sync_error_launch_text"),
        tr("ebisu_client_cloud_error_sync_error_play_anyway_button"), tr("ebisu_client_cancel"), "no", QJsonObject(), this, "onContinueOnAccept");
    mt.overrideId = mWindowShowing;
    mt.highPriority = true;
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::showExternalSavesWarning()
{
    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showExternalSavesWarning";
    // Not calling emitStopFlow() here because this dialog is supplementary.  It does not replace mWindow so there is no reason to close mWindow.
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_cloud_warning"), 
        tr("ebisu_client_cloud_settings_rtp_notice").arg(tr("application_name")),
        tr("ebisu_client_ok_uppercase"));
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::showErrorRetry()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showErrorRetry";
    Services::Connection::ConnectionStatesService* css = Services::Connection::ConnectionStatesService::instance();
    ORIGIN_VERIFY_CONNECT(css, SIGNAL(globalConnectionStateChanged(bool)), this, SLOT(onGlobalConnectionStateChanged(bool)));
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_cloud_error_sync_error_title"), 
        tr("ebisu_client_cloud_error_sync_error_launch_text"),
        tr("ebisu_client_cloud_retry_sync_button"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onRetryPushOnAccept");
    mt.acceptEnabled = css->isGlobalStateOnline();
    mt.overrideId = mEntitlement->contentConfiguration()->productId() + "_CloudSyncFlow";
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::onRetryPushOnAccept(QJsonObject obj)
{
    mWindowShowing = "";
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        emit retryPush();
    }
    else
    {
        emit stopFlow();
    }
}


void CloudSyncViewController::onRetryPullOnAccept(QJsonObject obj)
{
    mWindowShowing = "";
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        emit retryPull();
    }
    else
    {
        emit stopFlow();
    }
}


void CloudSyncViewController::onGlobalConnectionStateChanged(const bool& online)
{
    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_onGlobalConnectionStateChanged";
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_cloud_error_sync_error_title"), 
        tr("ebisu_client_cloud_error_sync_error_launch_text"),
        tr("ebisu_client_cloud_retry_sync_button"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onRetryPushOnAccept");
    mt.acceptEnabled = online;
    mt.overrideId = mEntitlement->contentConfiguration()->productId() + "_CloudSyncFlow";
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->updateMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::showUntrustedRemoteFilesDetected()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showUntrustedRemoteFilesDetected";
    QJsonArray commandbuttons;
    commandbuttons.append(PlayView::commandLinkObj("UseRemoteState", "otkicon-cloud", tr("ebisu_client_cloud_error_additional_cloud_delete_title"), tr("ebisu_client_cloud_error_additional_cloud_delete_text")));
    commandbuttons.append(PlayView::commandLinkObj("UseLocalState", "otkicon-save", tr("ebisu_client_cloud_error_additional_cloud_keep_title"), tr("ebisu_client_cloud_error_additional_cloud_keep_text")));

    QJsonObject content;
    content["commandbuttons"] = commandbuttons;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-commandbuttons", 
        tr("ebisu_client_cloud_error_additional_cloud_title"), 
        tr("ebisu_client_cloud_error_additional_cloud_text").arg(tr("application_name")).arg("<b>" + mEntitlement->contentConfiguration()->displayName() + "</b>"), 
        tr("ebisu_client_cancel"), content, this, "onCommandbuttonsDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::showEmptyCloudWarning()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showEmptyCloudWarning";
    QJsonArray commandbuttons;
    commandbuttons.append(PlayView::commandLinkObj("UseRemoteState", "otkicon-cloud", tr("ebisu_client_cloud_error_empty_cloud_delete_title"), tr("ebisu_client_cloud_error_empty_cloud_delete_text")));
    commandbuttons.append(PlayView::commandLinkObj("UseLocalState", "otkicon-save", tr("ebisu_client_cloud_error_empty_cloud_keep_title"), tr("ebisu_client_cloud_error_empty_cloud_keep_text")));

    QJsonObject content;
    content["commandbuttons"] = commandbuttons;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-commandbuttons", 
        tr("ebisu_client_cloud_error_empty_cloud_title"), 
        tr("ebisu_client_cloud_error_empty_cloud_text").arg(tr("application_name")).arg("<b>" + mEntitlement->contentConfiguration()->displayName() + "</b>"),
        tr("ebisu_client_cancel"), content, this, "onCommandbuttonsDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void CloudSyncViewController::showSyncConflictDetected(const QDateTime& localLastModified, const QDateTime& remoteLastModified)
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    mWindowShowing = mEntitlement->contentConfiguration()->productId() + "_showSyncConflictDetected";
    GetTelemetryInterface()->Metric_CLOUD_WARNING_SYNC_CONFLICT(mEntitlement->contentConfiguration()->productId().toUtf8().data());

    const QString bottomTemplate = QString("<span class=\"origin-cldialog-cloudsyncconflict-time-stamp-modified\">%0</span>").arg(tr("ebisu_client_cloud_error_sync_conflict_last_modified").arg(""));
    QString msg1 = tr("ebisu_client_cloud_error_sync_conflict_cloud_text_new");
    QString msgBottom1 = bottomTemplate;
    if (remoteLastModified.isNull())
    {
        msgBottom1 = tr("ebisu_client_cloud_no_sync_files");
    }
    else
    {
        msgBottom1.append(LocalizedDateFormatter().format(remoteLastModified, LocalizedDateFormatter::LongFormat));
    }

    QString msg2 = tr("ebisu_client_cloud_error_sync_conflict_local_text_new");
    QString msgBottom2 = bottomTemplate;
    if (localLastModified.isNull())
    {
        msgBottom2 = tr("ebisu_client_cloud_no_sync_files");
    }
    else
    {
        msgBottom2.append(LocalizedDateFormatter().format(localLastModified, LocalizedDateFormatter::LongFormat));
    }
    QJsonArray commandbuttons;
    QJsonObject cb = PlayView::commandLinkObj("UseRemoteState", "otkicon-cloud", tr("ebisu_client_cloud_error_sync_conflict_use_cloud_button"), msg1, msgBottom1);
    commandbuttons.append(cb);

    cb = PlayView::commandLinkObj("UseLocalState", "otkicon-save", tr("ebisu_client_cloud_error_sync_conflict_use_local_button"), msg2, msgBottom2);
    commandbuttons.append(cb);

    QJsonObject content;
    content["commandbuttons"] = commandbuttons;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-commandbuttons", 
        tr("ebisu_client_cloud_error_sync_conflict_title"), 
        tr("ebisu_client_cloud_error_sync_conflict_text").arg("<b>"+mEntitlement->contentConfiguration()->displayName()+"</b>"),
        tr("ebisu_client_cancel"), content, this, "onCommandbuttonsDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}

}
}
