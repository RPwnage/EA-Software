#include <QDateTime>
#include <QFuture>

#include "CloudRestoreFlow.h"
#include "engine/cloudsaves/ChangeSet.h"
#include "utilities/LocalizedDateFormatter.h"
#include "engine/cloudsaves/LastSyncStore.h"
#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/LocalStateCalculator.h"
#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"
#include "Qt/originspinner.h"
#include "Qt/originwindowmanager.h"
#include "services/debug/DebugService.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{
    using namespace Origin::Engine::CloudSaves;

	CloudRestoreFlow::CloudRestoreFlow(const LocalStateBackup &backup) :
		mLocalStateCalculator(NULL),
		mBackup(backup),
        mCurrentWindow(NULL)
	{
	}

	CloudRestoreFlow::~CloudRestoreFlow()
	{
		if (mLocalStateCalculator)
		{
			cleanLocalStateCalculator();
		}

        closeCurrentWindow();
	}

    void CloudRestoreFlow::closeCurrentWindow()
    {
        if(mCurrentWindow)
        {
            mCurrentWindow->deleteLater();
            mCurrentWindow = NULL;
        }
    }

	void CloudRestoreFlow::start()
	{
		// Do we even have a backup?
		if (!mBackup.isValid())
		{
            if(mCurrentWindow == NULL)
            {
                using namespace Origin::UIToolkit;
                mCurrentWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close), NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
                mCurrentWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_cloud_restore_msg_title"), tr("ebisu_client_cloud_restore_msg_nosaves"));
                mCurrentWindow->manager()->setupButtonFocus();
                ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(flowStop()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(flowStop()));
                mCurrentWindow->showWindow();
            }
		}
        else
        {
            // TELEMETRY
            const QString offerId(mBackup.entitlement()->contentConfiguration()->productId());
            GetTelemetryInterface()->Metric_CLOUD_MANUAL_RECOVERY(offerId.toUtf8().data());

            // Calculate our local state using the backup as a cache to skip MD5s where possible
            mLocalStateCalculator = new LocalStateCalculator(mBackup.entitlement(), *mBackup.stateSnapshot());
            ORIGIN_VERIFY_CONNECT(mLocalStateCalculator, SIGNAL(finished(Origin::Engine::CloudSaves::LocalStateSnapshot*)),
                    this, SLOT(localStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot*)));

            // Let the calculator run and finish
            mLocalStateCalculator->start();
        }
    }

	void CloudRestoreFlow::cleanLocalStateCalculator()
	{
		mLocalStateCalculator->wait();
		delete mLocalStateCalculator;
		mLocalStateCalculator = NULL;
	}

	void CloudRestoreFlow::localStateCalculated(LocalStateSnapshot* currentState)
	{
		cleanLocalStateCalculator();

        if(mCurrentWindow == NULL)
        {
            using namespace Origin::UIToolkit;
            mCurrentWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close), NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
            ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(closeCurrentWindow()));
            if (ChangeSet(*mBackup.stateSnapshot(), *currentState).isEmpty())
            {
                mCurrentWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_cloud_restore_msg_title"), tr("ebisu_client_cloud_restore_msg_match"));
                mCurrentWindow->manager()->setupButtonFocus();
                ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(flowStop()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(flowStop()));
            }
            else
            {
                // Confirm the restore
                mCurrentWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_cloud_restore_msg_title"), tr("ebisu_client_cloud_restore_msg_confirm"));
                mCurrentWindow->addButton(QDialogButtonBox::Cancel);
                mCurrentWindow->manager()->setupButtonFocus();
                mCurrentWindow->setDefaultButton(QDialogButtonBox::Cancel);
                ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(flowStop()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(flowStop()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(restoreConfirmed()));
            }

            mCurrentWindow->showWindow();
        }
	}

	void CloudRestoreFlow::restoreConfirmed()
	{
        if(mCurrentWindow == NULL)
        {
            using namespace Origin::UIToolkit;
            mCurrentWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close), NULL, OriginWindow::MsgBox);
            mCurrentWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_cloud_restore_restoring_title"), tr("ebisu_client_cloud_restore_restoring_msg"));

            ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(closeCurrentWindow()));
            ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(flowStop()));
            ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), &mRestoreWatcher, SLOT(cancel()));

            // Dialog content
            OriginSpinner* spinner = new OriginSpinner();
            mCurrentWindow->setContent(spinner);
            spinner->startSpinner();
            mCurrentWindow->showWindow();
        }

		// Do the low level restore
		QFuture<bool> restoreFuture = mBackup.restore();
		
		// Watch the restore
        // Window has to be closed and set to NULL before we go into restoreComplete()
        ORIGIN_VERIFY_CONNECT(&mRestoreWatcher, SIGNAL(finished()), this, SLOT(closeCurrentWindow()));
		ORIGIN_VERIFY_CONNECT(&mRestoreWatcher, SIGNAL(finished()), this, SLOT(restoreComplete()));
		mRestoreWatcher.setFuture(restoreFuture);
	}

	void CloudRestoreFlow::restoreComplete()
	{
		if (mRestoreWatcher.result())
		{
            if(mCurrentWindow == NULL)
            {
                // Confirm the restore
                using namespace Origin::UIToolkit;
                mCurrentWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close), NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
                // last modified date time string
                const QString lastModified = LocalizedDateFormatter().format(mBackup.created(), LocalizedDateFormatter::LongFormat);

                mCurrentWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_cloud_restore_restored_title"), tr("ebisu_client_cloud_restore_restored_msg").arg(lastModified));
                mCurrentWindow->manager()->setupButtonFocus();
                ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow, SIGNAL(rejected()), this, SLOT(flowComplete()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(closeCurrentWindow()));
                ORIGIN_VERIFY_CONNECT(mCurrentWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(flowComplete()));
                mCurrentWindow->showWindow();
            }
		}
		else
		{
			flowStop();
		}
	}

    void CloudRestoreFlow::flowStop()
    {
        emit finished(false);
    }

    void CloudRestoreFlow::flowComplete()
    {
        emit finished(true);
    }
}
}

