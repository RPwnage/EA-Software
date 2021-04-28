/////////////////////////////////////////////////////////////////////////////
// LogoutExit.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include <QFrame>
#include <QVBoxLayout>

#include "LogoutExitView.h"
#include "originmsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originpushbutton.h"
#include "origintransferbar.h"

#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "services/debug/DebugService.h"
#include "origindropdown.h"

#include "engine/content/ContentConfiguration.h"

namespace Origin
{
    using namespace UIToolkit;
	namespace Client
	{

		LogoutExitView::LogoutExitView(QWidget* parent)
			: QWidget(parent)
		{
		}

		LogoutExitView::~LogoutExitView()
		{

		}

        UIToolkit::OriginWindow * LogoutExitView::createCloudSyncProgressDialog (const QString& gameName, float currProgress)
        {
            CloudSyncProgressBar* cloudSyncContent = new CloudSyncProgressBar(currProgress);
            OriginWindow* cloudSyncProgressWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close), 
                                                                cloudSyncContent, OriginWindow::MsgBox, QDialogButtonBox::Cancel);

            QString title;

            title = tr( "ebisu_client_cloud_sync_in_progress_title" );

            cloudSyncProgressWindow->msgBox()->setup(OriginMsgBox::NoIcon, title, 
                                                "<b>" + gameName + "</b>");
            cloudSyncProgressWindow->manager()->setupButtonFocus();

            return cloudSyncProgressWindow;
        }


        CloudSyncProgressBar::CloudSyncProgressBar(float currProgress, QWidget* pParent)
	        : QWidget(pParent)
	        , mCloudProgressBar(NULL)
	        , mCloudSyncSucceded(false)
        {
            QVBoxLayout* vboxLayout = new QVBoxLayout(this);
		    mCloudProgressBar = new OriginTransferBar();
            mCloudProgressBar->setTitle(tr("ebisu_client_cloud_launch_syncing"));
            mCloudProgressBar->getProgressBar()->setMinimum(0);
            mCloudProgressBar->getProgressBar()->setMaximum(100);
		    mCloudProgressBar->setRate("");
		    mCloudProgressBar->setTimeRemaining("");
		    mCloudProgressBar->setCompleted("");
		    mCloudProgressBar->setState("");
            mCloudProgressBar->setValue (currProgress * 100);
            vboxLayout->addWidget(mCloudProgressBar);
        }

        CloudSyncProgressBar::~CloudSyncProgressBar()
        {
            if(mCloudProgressBar)
            {
                mCloudProgressBar->deleteLater();
                mCloudProgressBar = NULL;
            }
        }

        void CloudSyncProgressBar::onCloudSyncSucceeded()
        {
	        mCloudSyncSucceded = true;
            emit cloudSyncSuccess();
        }

        void CloudSyncProgressBar::onCloudProgressChanged(float progress, qint64 /*completedBytes*/, qint64 /*totalBytes*/)
        {
	        mCloudProgressBar->setValue(progress * 100);
        }

        void CloudSyncProgressBar::showCloudProgressBar (bool show)
        {
            if (show)
                mCloudProgressBar->show();
            else
                mCloudProgressBar->hide();
        }
	}
}
