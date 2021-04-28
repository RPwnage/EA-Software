/////////////////////////////////////////////////////////////////////////////
// SingleLoginViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SingleLoginViewController.h"
#include "services/debug/DebugService.h"
#include "views/source/SingleLoginView.h"

#include <QTimer>

namespace Origin
{
    namespace Client
    {
        SingleLoginViewController::SingleLoginViewController(QWidget *parent)
            : QObject(parent)
            , mCloudCheckInterval(1000 * 10) // Check every 10 seconds
            , mCloudCheckIntervalChange(1000 * 60 * 2) // Back off time
            , mCloudCheckTimeElapsed(0)
            , mCloudDialogShowing(false)
        {
        }

        SingleLoginViewController::~SingleLoginViewController()
        {

        }

        void SingleLoginViewController::showFirstUserDialog()
        {
            QScopedPointer<SingleLoginView> mSingleLoginView(new SingleLoginView);
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(loginOffline()), this, SIGNAL(loginOffline()));
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(loginOnline()), this, SIGNAL(loginOnline()));
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(cancelLogin()), this, SIGNAL(cancelLogin()));

            mSingleLoginView->showFirstUserDialog();
            // showFirstUserDialog commits suicide, i.e. it sets the attribute
            // "delete on close" to true at the beginning and then calls close
            // function at the end. Therefore, we need to take the ownership of
            // the scoped pointer not to delete the same object twice which will cause a crash
            mSingleLoginView.take();
        }

        void SingleLoginViewController::showSecondUserDialog(LoginType loginType)
        {
            QScopedPointer<SingleLoginView> mSingleLoginView(new SingleLoginView);
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(loginOffline()), this, SIGNAL(loginOffline()));
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(loginOnline()), this, SIGNAL(loginOnline()));

            //pass up cancel regardless of primary or secondary; let MainFlow determine whether initial login or in-session login
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(cancelLogin()), this, SIGNAL(cancelLogin()));

            mSingleLoginView->showSecondUserDialog(loginType);
            // showSecondtUserDialog commits suicide, i.e. it sets the attribute
            // "delete on close" to true at the beginning and then calls close
            // function at the end. Therefore, we need to take the ownership of
            // the scoped pointer not to delete the same object twice which will cause a crash
            mSingleLoginView.take();
        }

        void SingleLoginViewController::showCloudSyncDialog()
        {
            QScopedPointer<SingleLoginView> mSingleLoginView(new SingleLoginView);
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(cancelLogin()), this, SIGNAL(cancelLogin()));
            ORIGIN_VERIFY_CONNECT(mSingleLoginView.data(), SIGNAL(loginOnline()), this, SIGNAL(loginOnline()));
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(cloudSyncComplete()), mSingleLoginView.data(), SLOT(closeCloudSyncConflictDialog()));

            mCloudDialogShowing = true;
            armTimer();

            mSingleLoginView->showCloudSyncConflictDialog();
        }

        void SingleLoginViewController::closeCloudSyncDialog()
        {
            emit (cloudSyncComplete());
        }

        void SingleLoginViewController::armTimer()
        {
            QTimer::singleShot(mCloudCheckInterval, this, SLOT(checkCloudSave()));
        }

        void SingleLoginViewController::checkCloudSave()
        {
            mCloudCheckTimeElapsed += mCloudCheckInterval;
            if (mCloudCheckTimeElapsed > mCloudCheckIntervalChange)
            {
                // Back off to 30s
                mCloudCheckInterval = 1000 * 30;
            }
            emit (pollCloudSaves());
        }
    }
}
