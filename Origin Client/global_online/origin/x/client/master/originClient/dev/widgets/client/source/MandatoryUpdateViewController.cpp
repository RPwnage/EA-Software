/////////////////////////////////////////////////////////////////////////////
// MandatoryUpdateViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MandatoryUpdateViewController.h"
#include "flows/MainFlow.h"
#include "engine/updater/UpdateController.h"
#include "services/debug/DebugService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "OriginApplication.h"
#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originpushbutton.h"
#include <QTimer>
#include <QApplication>

namespace Origin
{
    namespace Client
    {
        MandatoryUpdateViewController::MandatoryUpdateViewController(QWidget *parent)
            : QWidget(parent)
            , mUpdatePendingMessage(NULL)
        {
            init();
        }

        MandatoryUpdateViewController::~MandatoryUpdateViewController()
        {
            if (mUpdatePendingMessage)
            {
                delete mUpdatePendingMessage;
                mUpdatePendingMessage = NULL;
            }
        }

        void MandatoryUpdateViewController::init()
        {
            if (!Engine::UpdateController::instance())
                Engine::UpdateController::init();

            // Hook up the update component's signals
            ORIGIN_VERIFY_CONNECT(Engine::UpdateController::instance(), SIGNAL(updateAvailable(const Services::AvailableUpdate &)), 
                this, SLOT(updateAvailable(const Services::AvailableUpdate &)));

            ORIGIN_VERIFY_CONNECT(Engine::UpdateController::instance(), SIGNAL(updatePending(const Services::AvailableUpdate &)),
                this, SLOT(updatePending(const Services::AvailableUpdate &)));
        }

        void MandatoryUpdateViewController::updateAvailable(const Services::AvailableUpdate &update)
        {
            if (!(update.updateRule == Services::UpdateRule_Mandatory))
            {
                return;
            }

            // Place client in Forced Offline Mode
            Origin::Services::Network::GlobalConnectionStateMonitor::setUpdatePending(true);

            // Kill any old notifications
            if (mUpdatePendingMessage)
            {
                delete mUpdatePendingMessage;
                mUpdatePendingMessage = NULL;
            }

            using namespace Origin::UIToolkit;
            OriginWindow* mWindowChrome = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
            mWindowChrome->msgBox()->setup(OriginMsgBox::NoIcon, 
                tr("ebisu_client_required_update_title"),
                tr("ebisu_client_update_requires_restart_description").arg(tr("application_name")));
            OriginPushButton *restartNowButton  = 
                mWindowChrome->addButton(QDialogButtonBox::Yes);
            restartNowButton->setText(tr("ebisu_client_restart_now"));
            OriginPushButton *restartLaterButton  = mWindowChrome->addButton(QDialogButtonBox::No);
            restartLaterButton->setText(tr("ebisu_client_restart_later"));
            mWindowChrome->setDefaultButton(restartLaterButton);
            mWindowChrome->manager()->setupButtonFocus();
            mWindowChrome->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(mWindowChrome, SIGNAL(rejected()), mWindowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(restartNowButton, SIGNAL(clicked()), mWindowChrome, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(restartLaterButton, SIGNAL(clicked()), mWindowChrome, SLOT(close()));

            mWindowChrome->exec();

            bool isRestart = (mWindowChrome->getClickedButton() == restartNowButton);

            // setting the delete to happen before the restart goes in
            mWindowChrome->deleteLater();

            if (isRestart)
            {
                // Restart
                MainFlow::instance()->restart();
            }
        }

        void MandatoryUpdateViewController::updatePending(const Services::AvailableUpdate &update)
        {
            if (!(update.updateRule == Services::UpdateRule_Mandatory))
            {
                return;
            }

            int secondsTo = QDateTime::currentDateTimeUtc().secsTo(update.activationDate);
            QString intervalPhrase = intervalToString(secondsTo);

            // Kill any old notifications
            if (mUpdatePendingMessage)
            {
                delete mUpdatePendingMessage;
                mUpdatePendingMessage = NULL;
            }

            // Show a fresh one
            using namespace Origin::UIToolkit;
            mUpdatePendingMessage = new OriginWindow(
                (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
            mUpdatePendingMessage->msgBox()->setup(OriginMsgBox::Info,
                tr("ebisu_client_critical_update_required_title"),
                tr("ebisu_client_required_update_description").arg(QApplication::applicationName()).arg(intervalPhrase)
                );
            ORIGIN_VERIFY_CONNECT(mUpdatePendingMessage, SIGNAL(rejected()), mUpdatePendingMessage, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(mUpdatePendingMessage->button(QDialogButtonBox::Ok), SIGNAL(clicked()), mUpdatePendingMessage, SLOT(reject()));
            mUpdatePendingMessage->setModal(true);
            mUpdatePendingMessage->manager()->setupButtonFocus();
            mUpdatePendingMessage->show();

            QTimer::singleShot(5 * 60 * 1000, this, SLOT(checkForUpdate()));
        }

        void MandatoryUpdateViewController::checkForUpdate()
        {
            emit (requestUpdateCheck());
        }

        QString MandatoryUpdateViewController::intervalToString(float seconds)
        {
            // Show hours after 55 minutes
            if (seconds > (55 * 60))
            {
                int hours = floor((seconds / (60.f * 60.f)) + 0.5f);

                // XXX: This is assuming English-like plural rules. Lets see if anyone
                // notices
                if (hours == 1)
                {
                    return tr("ebisu_client_hour_count_singular").arg(hours);
                }
                else
                {
                    return tr("ebisu_client_hour_count_plural").arg(hours);
                }
            }
            else
            {
                int minutes = floor((seconds / 60.f) + 0.5f);

                if (minutes == 1)
                {
                    return tr("ebisu_client_minute_count_singular").arg(minutes);
                }
                else
                {
                    return tr("ebisu_client_minute_count_plural").arg(minutes);
                }
            }
        }
    }
}