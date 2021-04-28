/////////////////////////////////////////////////////////////////////////////
// SaveBackupRestoreViewController.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "SaveBackupRestoreViewController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/Entitlement.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "originwindow.h"
#include "originbanner.h"
#include "originlabel.h"
#include "originscrollablemsgbox.h"
#include <QLabel>
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{
SaveBackupRestoreViewController::SaveBackupRestoreViewController(Engine::Content::EntitlementRef entitlement, QObject* parent, const bool& inDebug)
: QObject(parent)
, mEntitlement(entitlement)
, mWindow(NULL)
, mInDebug(inDebug)
{
}


SaveBackupRestoreViewController::~SaveBackupRestoreViewController()
{
    closeWindow();
}


void SaveBackupRestoreViewController::showSaveBackupRestoreSuccessful()
{
    closeWindow();
    mWindow = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::NoIcon, "", "", tr("ebisu_client_close"));
    mWindow->msgBox()->setContentsMargins(0, 0, 0, 40);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    mWindow->setTitleBarText(tr("ebisu_client_cloud_restore_title"));
    UIToolkit::OriginBanner* banner = new UIToolkit::OriginBanner();
    banner->setBannerType(UIToolkit::OriginBanner::Message);
    banner->setText(tr("ebisu_client_cloud_restore_success_description"));
    banner->setIconType(UIToolkit::OriginBanner::Success);
    mWindow->setContent(banner);
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SIGNAL(saveBackupRestoreFlowSuccess()));
    mWindow->showWindow();
}


void SaveBackupRestoreViewController::showSaveBackupRestoreFailed()
{
    closeWindow();
    mWindow = UIToolkit::OriginWindow::alertNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Error,
        tr("ebisu_client_cloud_restore_fail_header"), tr("ebisu_client_cloud_restore_fail_description"),
        tr("ebisu_client_close"));
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    QLabel* lblText = mWindow->scrollableMsgBox()->getTextLabel();
    lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
    lblText->setOpenExternalLinks(false);
    ORIGIN_VERIFY_CONNECT(lblText, SIGNAL(linkActivated(const QString&)), this, SLOT(onLinkActivated(const QString&)));
    mWindow->setTitleBarText(tr("ebisu_client_cloud_restore_title"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SIGNAL(saveBackupRestoreFlowFailed()));
    mWindow->showWindow();
}


void SaveBackupRestoreViewController::onLinkActivated(const QString& link)
{
    if (sender() == NULL)
        return;

    if (link.contains("openAccessFAQ", Qt::CaseInsensitive))
    {
        QString url = Services::readSetting(Services::SETTING_SubscriptionFaqURL, Services::Session::SessionService::currentSession()).toString();
        OriginApplication::instance().configCEURL(url);
        Services::PlatformService::asyncOpenUrl(QUrl::fromEncoded(url.toUtf8()));
        GetTelemetryInterface()->Metric_SUBSCRIPTION_FAQ_PAGE_REQUEST(sender()->objectName().toStdString().c_str());
    }
}


void SaveBackupRestoreViewController::closeWindow()
{
    if (mWindow && !mInDebug)
    {
        delete mWindow;
        mWindow = NULL;
    }
}

}
}