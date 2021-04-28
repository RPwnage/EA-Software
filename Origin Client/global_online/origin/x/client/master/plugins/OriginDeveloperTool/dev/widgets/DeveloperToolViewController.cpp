/////////////////////////////////////////////////////////////////////////////
// DeveloperToolViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "DeveloperToolViewController.h"
#include "OriginDeveloperProxy.h"
#include "services/debug/DebugService.h"
#include "OriginApplication.h"
#include "originwindow.h"
#include "originwebview.h"
#include "originpushbutton.h"
#include "originmsgbox.h"
#include "originwindowmanager.h"

#include "WebWidgetController.h"
#include "WebWidget/WidgetPage.h"
#include "flows/ClientFlow.h"
#include "engine/login/LoginController.h"

#include "Services/Settings/SettingsManager.h"
#include "NativeBehaviorManager.h"
#include "OfflineCdnNetworkAccessManager.h"
#include "TelemetryAPIDLL.h"

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebView>

namespace Origin
{

using namespace UIToolkit;

namespace Plugins
{

namespace DeveloperTool
{
    DeveloperToolViewController* DeveloperToolViewController::sInstance = NULL;

    DeveloperToolViewController* DeveloperToolViewController::instance()
    {
        if(!sInstance)
            init();

        return sInstance;
    }

    DeveloperToolViewController::DeveloperToolViewController(QWidget *parent)
    : QObject(parent)
        , mDeveloperToolWindow(0)
        , mWebViewContainer(0)
        , mWebView(0)
    {
        ORIGIN_VERIFY_CONNECT(
            Client::ClientFlow::instance(), SIGNAL(odtCloseWindowRequest()),
            this, SLOT(close()));
    }

    DeveloperToolViewController::~DeveloperToolViewController()
    {
        closeDeveloperTool();
    }

    void DeveloperToolViewController::show()
    {
        if (!mDeveloperToolWindow)
        {
            // Create our web view 
            mWebView = new WebWidget::WidgetView();
            mWebViewContainer = new UIToolkit::OriginWebView();

            mWebViewContainer->setWebview(dynamic_cast<QWebView*>(mWebView));

            Client::NativeBehaviorManager::applyNativeBehavior(mWebView);

            mWebViewContainer->setWindowLoadingSize(QSize(1000, 770));
            mWebViewContainer->setIsContentSize(true);
            mWebViewContainer->setHasSpinner(true);
            mWebViewContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
            mWebViewContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);

            // Create a window containing the web view
            mDeveloperToolWindow = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close),
                mWebViewContainer);

            mDeveloperToolWindow->setContentMargin(0);

            ORIGIN_VERIFY_CONNECT(mDeveloperToolWindow, SIGNAL(rejected()), this, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(mWebViewContainer->webview(), SIGNAL(titleChanged(const QString&)), this, SLOT(pageTitleChanged(const QString&)));
                
            mDeveloperToolWindow->setTitleBarText(tr("ebisu_client_notranslate_developer_tool").arg(QCoreApplication::applicationName()));
                            
            // Allow external network access through our network access manager               
            mWebView->widgetPage()->setExternalNetworkAccessManager(new Origin::Client::OfflineCdnNetworkAccessManager(mWebView->page()));

            WebWidget::NativeInterface originDeveloper("originDeveloper", new JsInterface::OriginDeveloperProxy, true);
            mWebView->widgetPage()->addPageSpecificInterface(originDeveloper);

            Client::WebWidgetController::instance()->loadSharedWidget(mWebView->widgetPage(), "odt");
        }
            
        mDeveloperToolWindow->showWindow();
    }

    void DeveloperToolViewController::pageTitleChanged(const QString& newTitle)
    {
        if (mDeveloperToolWindow)
        {
            mDeveloperToolWindow->setTitleBarText(newTitle);
        }
    }

    bool DeveloperToolViewController::close(const CloseMode mode)
    {
        // if null then we have previously close ODT return true.
        if(!mDeveloperToolWindow)
            return true;
            
        bool retval = false;

        if (CloseWindow_PromptUser == mode)
        {
            // Can log telemtry here if necessary.
            OriginWindow *confirmExitDialog = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Close | OriginWindow::Icon),
                0,
                OriginWindow::MsgBox,
                QDialogButtonBox::Yes | QDialogButtonBox::No);
 
            QString devToolName = tr("ebisu_client_notranslate_developer_tool").arg(QCoreApplication::applicationName());

            // Override localization
            confirmExitDialog->setButtonText(QDialogButtonBox::Yes, tr("ebisu_client_notranslate_yes"));
            confirmExitDialog->setButtonText(QDialogButtonBox::No, tr("ebisu_client_notranslate_no"));

            confirmExitDialog->msgBox()->setup(
                OriginMsgBox::NoIcon,
                tr("ebisu_client_notranslate_close_confirm_title").arg(devToolName.toUpper()),
                tr("ebisu_client_notranslate_unsaved_changes_confirmation").arg(devToolName));
            confirmExitDialog->manager()->setupButtonFocus();
            
            ORIGIN_VERIFY_CONNECT(confirmExitDialog, SIGNAL(accepted()), confirmExitDialog, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(confirmExitDialog, SIGNAL(rejected()), confirmExitDialog, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(confirmExitDialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), confirmExitDialog, SLOT(accept()));
            ORIGIN_VERIFY_CONNECT(confirmExitDialog->button(QDialogButtonBox::No), SIGNAL(clicked()), confirmExitDialog, SLOT(reject()));

            if(confirmExitDialog->execWindow() == QDialog::Accepted)
            {
                closeDeveloperTool();
                retval = true;
            }
        }
        else
        {
            closeDeveloperTool();
            retval = true;
        }

        return retval;
    }

    bool DeveloperToolViewController::close(const int closeMode)
    {
        return close(static_cast<CloseMode>(closeMode));
    }

    void DeveloperToolViewController::closeDeveloperTool()
    {
        if(mDeveloperToolWindow)
        {
            ORIGIN_VERIFY_DISCONNECT(mDeveloperToolWindow, SIGNAL(rejected()), this, SLOT(close()));

            mDeveloperToolWindow->close();
            QCoreApplication::sendPostedEvents(mDeveloperToolWindow);
            delete mDeveloperToolWindow;
            mDeveloperToolWindow = 0;
        }
    }

    void DeveloperToolViewController::restart()
    {
        OriginWindow *restartDialog = new OriginWindow(
            (OriginWindow::TitlebarItems)(OriginWindow::Close | OriginWindow::Icon),
            0,
            OriginWindow::MsgBox,
            QDialogButtonBox::Yes | QDialogButtonBox::No);

        restartDialog->msgBox()->setup(
            OriginMsgBox::NoIcon,
            tr("ebisu_client_notranslate_restart_change_language_title"),
            tr("ebisu_client_notranslate_save_and_restart_text").arg(qApp->applicationName()));
        restartDialog->manager()->setupButtonFocus();
        restartDialog->setButtonText(QDialogButtonBox::Yes, tr("ebisu_client_notranslate_restart_now"));
        restartDialog->setButtonText(QDialogButtonBox::No, tr("ebisu_client_notranslate_not_now"));
        restartDialog->setDefaultButton(QDialogButtonBox::Yes);
            
        ORIGIN_VERIFY_CONNECT(restartDialog, SIGNAL(finished(int)), restartDialog, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(restartDialog, SIGNAL(rejected()), restartDialog, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(restartDialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), restartDialog, SLOT(accept()));
        ORIGIN_VERIFY_CONNECT(restartDialog->button(QDialogButtonBox::No), SIGNAL(clicked()), restartDialog, SLOT(reject()));

        if(restartDialog->execWindow() == QDialog::Accepted)
        {
            GetTelemetryInterface()->Metric_ODT_RESTART_CLIENT();

            // If we're restarting then close our window without prompting
            // the user. Do NOT call close directly since WebView event
            // handling is happening further up the stack for this object.
            QMetaObject::invokeMethod(
                this, 
                "close", 
                Qt::QueuedConnection,
                Q_ARG(int, CloseWindow_Force));
            
            // Note that we can't call MainFlow::restart here directly
            // because the restart request originates from the JS bridge and
            // eventually we destroy our webview during the restart flow,
            // which would cause problems further up the stack (at this point
            // in execution).
            QMetaObject::invokeMethod(
                Client::MainFlow::instance(), 
                "restart", 
                Qt::QueuedConnection);
        }
    }

} // namespace DeveloperTool

} // namespace Plugins

} // namespace Origin