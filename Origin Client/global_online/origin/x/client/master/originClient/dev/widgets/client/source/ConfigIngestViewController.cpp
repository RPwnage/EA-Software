/////////////////////////////////////////////////////////////////////////////
// ConfigIngestViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ConfigIngestViewController.h"
#include "services/debug/DebugService.h"
#include "OriginApplication.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originscrollablemsgbox.h"

#include <QtCore/QMath.h>
#ifdef ORIGIN_MAC
#include <QLayout>
#endif

namespace Origin
{
    namespace Client
    {
        ConfigIngestViewController::ConfigIngestViewController(QObject *parent /*= NULL*/) 
            : QObject(parent)
            , mConfigIngestWindow(NULL)
            , mTickCount(0)
        {
            mTickTimer.setSingleShot(false);
            mTimeoutTimer.setSingleShot(true);
        }

        ConfigIngestViewController::~ConfigIngestViewController()
        {
        }

        void ConfigIngestViewController::setTickInterval(int msec)
        {
            mTickTimer.setInterval(msec);
        }

        void ConfigIngestViewController::setTimeoutInterval(int msec)
        {
            mTimeoutTimer.setInterval(msec);
        }
        
        int ConfigIngestViewController::exec()
        {
            using namespace Origin::UIToolkit;
            int retval = QDialog::Rejected;

            if(!mConfigIngestWindow)
            {
                int timeoutSeconds = qCeil(mTimeoutTimer.interval() / 1000);
                QString timeoutString = timeoutSeconds == 1 ? tr("ebisu_client_interval_second_singular").arg(timeoutSeconds) : tr("ebisu_client_interval_second_plural").arg(timeoutSeconds);

                mConfigIngestWindow = new OriginWindow(
                    (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                    NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Yes | QDialogButtonBox::No);

                mConfigIngestWindow->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon,
                    tr("ebisu_client_ingest_config_file_title").toUpper(),
                    tr("ebisu_client_ingest_config_file_message").arg(tr("application_name")).arg(timeoutString));
                mConfigIngestWindow->manager()->setupButtonFocus();
    
                ORIGIN_VERIFY_CONNECT(mConfigIngestWindow, SIGNAL(accepted()), mConfigIngestWindow, SLOT(close()));
                ORIGIN_VERIFY_CONNECT(mConfigIngestWindow, SIGNAL(rejected()), mConfigIngestWindow, SLOT(close()));
                ORIGIN_VERIFY_CONNECT(mConfigIngestWindow->button(QDialogButtonBox::Yes), SIGNAL(clicked()), mConfigIngestWindow, SLOT(accept()));
                ORIGIN_VERIFY_CONNECT(mConfigIngestWindow->button(QDialogButtonBox::No), SIGNAL(clicked()), mConfigIngestWindow, SLOT(reject()));
                
                ORIGIN_VERIFY_CONNECT(&mTickTimer, SIGNAL(timeout()), this, SLOT(onTick()));
                ORIGIN_VERIFY_CONNECT(&mTimeoutTimer, SIGNAL(timeout()), mConfigIngestWindow, SLOT(accept()));
                
                mTickTimer.start();
                mTimeoutTimer.start();

                retval = mConfigIngestWindow->execWindow();

                mTickTimer.stop();
                mTimeoutTimer.stop();
            }

            return retval;
        }
        
        void ConfigIngestViewController::onTick()
        {
            ++mTickCount;

            int timeoutSeconds = qCeil((mTimeoutTimer.interval() - (mTickCount * mTickTimer.interval())) / 1000);
            QString timeoutString = timeoutSeconds == 1 ? tr("ebisu_client_interval_second_singular").arg(timeoutSeconds) : tr("ebisu_client_interval_second_plural").arg(timeoutSeconds);
            
            QString messageText = tr("ebisu_client_ingest_config_file_message").arg(tr("application_name")).arg(timeoutString);
            mConfigIngestWindow->scrollableMsgBox()->setText(messageText);
#ifdef ORIGIN_MAC
            QWidget* parent = mConfigIngestWindow->scrollableMsgBox()->parentWidget();
            if (parent)
                parent->layout()->activate();
#else
            mConfigIngestWindow->update();
#endif
        }
    }
}
