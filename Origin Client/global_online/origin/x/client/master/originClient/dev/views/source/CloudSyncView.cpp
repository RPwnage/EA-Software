/////////////////////////////////////////////////////////////////////////////
// PlayView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CloudSyncView.h"
#include "services/debug/DebugService.h"
#include "utilities/LocalizedDateFormatter.h"
#include "ui_CloudDialog.h"
#include "origintransferbar.h"

namespace Origin
{
namespace Client
{

QuotaExceededFailureView::QuotaExceededFailureView(const QString& displayName, const QString& MBToClear, QWidget* parent)
: QWidget(parent)
, mUI(new Ui::CloudDialog)
{
    mUI->setupUi(this);
    mUI->link1->setText(tr("ebisu_client_cloud_error_storage_limit_free_up_title"));
    mUI->link1->setDescription(tr("ebisu_client_cloud_error_storage_limit_free_up_text").arg("<b>"+displayName+"</b>").arg(MBToClear));
    ORIGIN_VERIFY_CONNECT(mUI->link1, SIGNAL(clicked()), this, SIGNAL(freeSpace()));

    mUI->link2->setText(tr("ebisu_client_cloud_error_storage_limit_disable_title"));
    mUI->link2->setDescription(tr("ebisu_client_cloud_error_storage_limit_disable_text").arg(tr("application_name")));
    ORIGIN_VERIFY_CONNECT(mUI->link2, SIGNAL(clicked()), this, SIGNAL(disableCloud()));
}


SyncConflictDetectedView::SyncConflictDetectedView(const QDateTime& localLastModified, const QDateTime& remoteLastModified, QWidget* parent)
: QWidget(parent)
, mUI(new Ui::CloudDialog)
{
    mUI->setupUi(this);
    QSignalMapper* signalMapper = new QSignalMapper(this);

    mUI->link1->setText(tr("ebisu_client_cloud_error_sync_conflict_use_cloud_button"));
    QString msg1 = tr("ebisu_client_cloud_error_sync_conflict_cloud_text_new") + "<p>";
    if (remoteLastModified.isNull())
    {
        msg1 += QString(tr("ebisu_client_cloud_no_sync_files"));
    }
    else
    {
        using namespace Origin::Client;
        msg1 += QString("<b>%0<\b>")
            .arg(tr("ebisu_client_cloud_error_sync_conflict_last_modified")
            .arg(LocalizedDateFormatter().format(remoteLastModified, LocalizedDateFormatter::LongFormat)));
    }
    mUI->link1->setDescription(msg1);

    signalMapper->setMapping(mUI->link1, Engine::CloudSaves::UseRemoteState);
    ORIGIN_VERIFY_CONNECT(mUI->link1, SIGNAL(clicked()), signalMapper, SLOT(map()));

    mUI->link2->setText(tr("ebisu_client_cloud_error_sync_conflict_use_local_button"));
    QString msg2 = tr("ebisu_client_cloud_error_sync_conflict_local_text_new") + "<p>";
    if (localLastModified.isNull())
    {
        msg2 += QString(tr("ebisu_client_cloud_no_sync_files"));
    }
    else
    {
        using namespace Origin::Client;
        msg2 += QString("<b>%0<\b>")
            .arg(tr("ebisu_client_cloud_error_sync_conflict_last_modified")
            .arg(LocalizedDateFormatter().format(localLastModified, LocalizedDateFormatter::LongFormat)));
    }
    mUI->link2->setDescription(msg2);
    signalMapper->setMapping(mUI->link2, Engine::CloudSaves::UseLocalState);
    ORIGIN_VERIFY_CONNECT(mUI->link2, SIGNAL(clicked()), signalMapper, SLOT(map()));

    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const Origin::Engine::CloudSaves::DelegateResponse&)),
        this, SIGNAL(stateChosen(const Origin::Engine::CloudSaves::DelegateResponse&)));
}


CloudSyncLaunchView::CloudSyncLaunchView(QWidget* parent)
: QWidget(parent)
, mCloudProgressBar(NULL)
{
    QVBoxLayout* vboxLayout = new QVBoxLayout(this);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    mCloudProgressBar = new UIToolkit::OriginTransferBar(this);
    mCloudProgressBar->setTitle(tr("ebisu_client_cloud_launch_syncing"));
    mCloudProgressBar->getProgressBar()->setMinimum(0);
    mCloudProgressBar->getProgressBar()->setMaximum(100);
    mCloudProgressBar->setRate("");
    mCloudProgressBar->setTimeRemaining("");
    mCloudProgressBar->setCompleted("");
    mCloudProgressBar->setState("");

    vboxLayout->addWidget(mCloudProgressBar);
}

void CloudSyncLaunchView::onCloudProgressChanged(float progress)
{
    // Should be >= 10000 because our percent display shows two decimal points
    mCloudProgressBar->setValue(progress * 100);
}

}
}