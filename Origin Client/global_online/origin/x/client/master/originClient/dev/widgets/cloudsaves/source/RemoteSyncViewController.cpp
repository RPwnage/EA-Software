#include "RemoteSyncViewController.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "engine/content/ContentConfiguration.h"
#include "utilities/LocalizedDateFormatter.h"
#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"
#include "Qt/originwindowmanager.h"
#include "TelemetryAPIDLL.h"
#include "ui_CloudDialog.h"

namespace Origin
{
namespace Client
{
    using Engine::CloudSaves::RemoteSync;

    RemoteSyncViewController::RemoteSyncViewController()
        : mIsCancelled(false)
    {
    }

    void RemoteSyncViewController::untrustedRemoteFilesDetected()
    {
        //using namespace Origin::UIToolkit;

        //OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        //    NULL, OriginWindow::MsgBox, QDialogButtonBox::Cancel);
        //// For Automation
        //window->setObjectName("CLOUD_DIALOG_NEW_SAVE_DATA_FOUND");

        //window->manager()->setupButtonFocus();

        //window->msgBox()->setup(OriginMsgBox::Notice,
        //    tr("ebisu_client_cloud_error_additional_cloud_title"),
        //    tr("ebisu_client_cloud_error_additional_cloud_text")
        //    .arg(tr("application_name"))
        //    .arg("<b>" + remoteSync->entitlement()->contentConfiguration()->displayName() + "</b>"));

        //// Window content
        //{
        //    Ui::CloudDialog ui;
        //    QWidget* commandlinkContent = new QWidget();
        //    ui.setupUi(commandlinkContent);

        //    ui.link1->setText(tr("ebisu_client_cloud_error_additional_cloud_delete_title"));
        //    ui.link1->setDescription(tr("ebisu_client_cloud_error_additional_cloud_delete_text"));
        //    ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SLOT(useRemoteState()));
        //    ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), window, SLOT(accept()));

        //    ui.link2->setText(tr("ebisu_client_cloud_error_additional_cloud_keep_title"));
        //    ui.link2->setDescription(tr("ebisu_client_cloud_error_additional_cloud_keep_text"));
        //    ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SLOT(useLocalState()));
        //    ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), window, SLOT(accept()));

        //    window->setContent(commandlinkContent);
        //}

        //window->setAttribute(Qt::WA_DeleteOnClose, true);

        //// also connecting to the reject signal because some of the calling dialogs are looking for that
        //ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));
        //ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SLOT(cancelSync()));
        //ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), window, SLOT(close()));
        //ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(cancelSync()));

        //window->adjustSize();
        //window->showWindow();
    }

    void RemoteSyncViewController::syncConflictDetected(const QDateTime &localLastModified, const QDateTime &remoteLastModified)
    {
   //     if (mIsCancelled)
   //     {
   //         ORIGIN_LOG_EVENT << "Remote synch CANCELLED";
			//mIsCancelled = false; // paranoid
   //         emit reponse(CancelSync);
   //     }

   //     using namespace Origin::UIToolkit;
   //     OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
   //         NULL, OriginWindow::MsgBox, QDialogButtonBox::Cancel);
   //     // For Automation
   //     window->setObjectName("CLOUD_DIALOG_SYNC_CONFLICT");
   //     window->msgBox()->setup(OriginMsgBox::Notice,
   //         tr("ebisu_client_cloud_error_sync_conflict_title"),
   //         tr("ebisu_client_cloud_error_sync_conflict_text").arg("<b>"+remoteSync->entitlement()->contentConfiguration()->displayName()+"</b>"));

   //     // Window content
   //     {
   //         // TELEMETRY
   //         GetTelemetryInterface()->Metric_CLOUD_WARNING_SYNC_CONFLICT(remoteSync->offerId().toUtf8().data());
   //         Ui::CloudDialog ui;
   //         QWidget* commandlinkContent = new QWidget();
   //         ui.setupUi(commandlinkContent);

   //         ui.link1->setText(tr("ebisu_client_cloud_error_sync_conflict_use_cloud_button"));
   //         QString msg1 = QString("%0<p>").arg(tr("ebisu_client_cloud_error_sync_conflict_cloud_text_new"));
   //         if (remoteLastModified.isNull())
   //         {
   //             msg1 += QString(tr("ebisu_client_cloud_no_sync_files"));
   //         }
   //         else
   //         {
   //             using namespace Origin::Client;
   //             msg1 += QString("<b>%0<\b>")
   //                 .arg(tr("ebisu_client_cloud_error_sync_conflict_last_modified")
   //                 .arg(LocalizedDateFormatter().format(remoteLastModified, LocalizedDateFormatter::LongFormat)));
   //         }
   //         ui.link1->setDescription(msg1);
   //         ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SLOT(useRemoteState()));
   //         ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), window, SLOT(accept()));


   //         ui.link2->setText(tr("ebisu_client_cloud_error_sync_conflict_use_local_button"));
   //         QString msg2 = QString("%0<p>").arg(tr("ebisu_client_cloud_error_sync_conflict_local_text_new"));
   //         if (localLastModified.isNull())
   //         {
   //             msg2 += QString(tr("ebisu_client_cloud_no_sync_files"));
   //         }
   //         else
   //         {
   //             using namespace Origin::Client;
   //             msg2 += QString("<b>%0<\b>")
   //                 .arg(tr("ebisu_client_cloud_error_sync_conflict_last_modified")
   //                 .arg(LocalizedDateFormatter().format(localLastModified, LocalizedDateFormatter::LongFormat)));
   //         }
   //         ui.link2->setDescription(msg2);
   //         ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SLOT(useLocalState()));
   //         ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), window, SLOT(accept()));
   //         window->setContent(commandlinkContent);
   //     }

   //     window->setAttribute(Qt::WA_DeleteOnClose, true);

   //     // also connecting to the reject signal because some of the calling dialogs are looking for that
   //     ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));
   //     ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SLOT(cancelSync()));
   //     ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), window, SLOT(close()));
   //     ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(cancelSync()));

   //     window->adjustSize();
   //     window->manager()->setupButtonFocus();
   //     window->showWindow();
    }

    void RemoteSyncViewController::useRemoteState()
    {
        //emit reponse(UseRemoteState);
    }

    void RemoteSyncViewController::useLocalState()
    {
        //emit reponse(UseLocalState);
    }
    
    void RemoteSyncViewController::cancelSync()
    {
        //emit reponse(CancelSync);
    }
        
    void RemoteSyncViewController::aboutToDeleteSync()
    {
        //deleteLater();
    }

    void RemoteSyncViewController::cancelOperation()
    {
        mIsCancelled = true;
    }

}
}
