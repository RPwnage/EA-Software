/////////////////////////////////////////////////////////////////////////////
// SingleLoginView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SingleLoginView.h"
#include "services/debug/DebugService.h"
#include "engine/igo/IGOController.h"
#include "widgets/login/source/SingleLoginDefines.h"

// legacy includes
#include "TelemetryAPIDLL.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"
#include "originspinner.h"
#include "ui_CloudDialog.h"

namespace Origin
{
    namespace Client
    {
        SingleLoginView::SingleLoginView(QWidget *parent)
            : QWidget(parent)
            , mMessageBox(NULL)
            , mOIGMessageBox(NULL)
        {
            init();
        }


        SingleLoginView::~SingleLoginView()
        {

        }

        void SingleLoginView::closeMessageDialog()
        {
            if(mMessageBox)
            {
                ORIGIN_VERIFY_DISCONNECT(mMessageBox, SIGNAL(rejected()), 0, 0);
                mMessageBox->close();
                mMessageBox->deleteLater();
                mMessageBox = NULL;
            }
            if(mOIGMessageBox)
            {
                ORIGIN_VERIFY_DISCONNECT(mOIGMessageBox, SIGNAL(rejected()), 0, 0);
                mOIGMessageBox->close();
                mOIGMessageBox->deleteLater();
                mOIGMessageBox = NULL;
            }
        }

        void SingleLoginView::init()
        {

        }

        void SingleLoginView::showFirstUserDialog()
        {
            // This dialog is informational. At this point, another user has already logged in
            // to the same account. So 'go offline' right away.
            GetTelemetryInterface()->Metric_SINGLE_LOGIN_FIRST_USER_LOGGING_OUT();
            emit (loginOffline());

            setAttribute(Qt::WA_DeleteOnClose);
            using namespace UIToolkit;
            mMessageBox = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                                           NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
            mMessageBox->setObjectName(SingleLoginFirstDialogName);
            mMessageBox->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_single_login_title_forced_offline"), tr("ebisu_client_single_login_error_forced_offline_text"));
            mMessageBox->manager()->setupButtonFocus();

            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), mMessageBox, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), mMessageBox, SLOT(close()));

            // We want this dialog in both IGO and the client
            if (Engine::IGOController::instance()->igowm()->visible())
            {
                Engine::IGOController::instance()->igowm()->addPopupWindow(mMessageBox, Engine::IIGOWindowManager::WindowProperties());
            }

            mMessageBox->showWindow();
        }

        void SingleLoginView::showSecondUserDialog(LoginType loginType)
        {
            setAttribute(Qt::WA_DeleteOnClose);
            if(mMessageBox == NULL)
                mMessageBox = createSecondUserDialog(loginType, 0);
            mMessageBox->showWindow();

            // We want this dialog in both IGO and the client
            if (Engine::IGOController::instantiated() && Engine::IGOController::instance()->isActive() && mOIGMessageBox == NULL)
            {
                mOIGMessageBox = createSecondUserDialog(loginType, 1);
                Engine::IGOController::instance()->igowm()->addPopupWindow(mOIGMessageBox, Engine::IIGOWindowManager::WindowProperties());
            }

            else
            {
                // EBIBUGS-27669: Because this is important we want to make sure we raise this. However, we don't want to steal focus from OIG.
                mMessageBox->raise();
                mMessageBox->activateWindow();
            }
        }

        UIToolkit::OriginWindow* SingleLoginView::createSecondUserDialog(const LoginType& loginType, const int& context)
        {
            using namespace UIToolkit;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            // Not in OIG
            if(context == 0)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            OriginWindow* window = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                                                    QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            window->setAttribute(Qt::WA_DeleteOnClose, false);
            window->setObjectName(SingleLoginSecondDialogName);
            ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCancelClicked()));
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SLOT(onCancelClicked()));

            // Button doesn't make sense if we're in game since they're already offline anyways
            if (loginType == PrimaryLogin)
            {
                window->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_single_login_title"), tr("ebisu_client_single_login_text"));
                Ui::CloudDialog ui;
                QWidget* commandlinkContent = new QWidget();
                ui.setupUi(commandlinkContent);

                ui.link1->setText(tr("ebisu_client_single_login_login_anyway"));
                ui.link1->setDescription(tr("ebisu_client_single_login_kick"));
                ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SLOT(onLogInBtnClicked()));

                ui.link2->setText(tr("ebisu_client_single_login_offline_mode"));
                ui.link2->setDescription(tr("ebisu_client_single_login_send_offline"));
                ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SLOT(onOfflineBtnClicked()));
                window->setContent(commandlinkContent);
            }
            else
            {
                window->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_single_login_title"), tr("ebisu_client_single_login_text_inclient"));
                window->addButton(QDialogButtonBox::Ok);
                window->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_single_login_login_anyway"));
                ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onLogInBtnClicked()));
            }

            if(window->manager())
                window->manager()->setupButtonFocus();
            return window;
        }

        void SingleLoginView::onLogInBtnClicked()
        {
            emit (loginOnline());
            GetTelemetryInterface()->Metric_SINGLE_LOGIN_SECOND_USER_LOGGING_IN("Online");
            closeMessageDialog();
            close();
        }

        void SingleLoginView::onOfflineBtnClicked()
        {
            emit (loginOffline());
            GetTelemetryInterface()->Metric_SINGLE_LOGIN_SECOND_USER_LOGGING_IN("Offline");
            closeMessageDialog();
            close();
        }

        void SingleLoginView::onCancelClicked()
        {
            emit (cancelLogin());
            GetTelemetryInterface()->Metric_SINGLE_LOGIN_SECOND_USER_LOGGING_IN("Cancel");
            closeMessageDialog();
            close();
        }

        void SingleLoginView::onCancelWaitingForCloudClicked()
        {
            emit (cancelLogin());
            closeMessageDialog();
            close();
        }

        void SingleLoginView::showCloudSyncConflictDialog()
        {
            setAttribute(Qt::WA_DeleteOnClose);
            using namespace UIToolkit;
            OriginSpinner* spinner = new OriginSpinner();
            mMessageBox = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
                                           spinner, OriginWindow::MsgBox, QDialogButtonBox::Cancel);
            mMessageBox->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_single_login_error_game_save_title"), tr("ebisu_client_single_login_error_game_save_text"));
            mMessageBox->manager()->setupButtonFocus();

            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCancelWaitingForCloudClicked()));

            spinner->startSpinner();
            mMessageBox->exec();
        }

        void SingleLoginView::closeCloudSyncConflictDialog()
        {
            closeMessageDialog();
            close();
        }
    }
}
