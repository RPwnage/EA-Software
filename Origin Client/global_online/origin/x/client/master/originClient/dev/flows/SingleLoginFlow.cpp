///////////////////////////////////////////////////////////////////////////////
// SingleLoginFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "flows/SingleLoginFlow.h"
#include "flows/ClientFlow.h"
#include "flows/RTPFlow.h"
#include "engine/login/SingleLoginController.h"
#include "widgets/login/source/SingleLoginDefines.h"
#include "widgets/login/source/SingleLoginViewController.h"
#include "services/session/SessionService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"
#include "Qt/originwindow.h"

using namespace Origin::Engine;
using namespace Origin::Services;

namespace Origin
{
    namespace Client
    {
        SingleLoginFlow::SingleLoginFlow()
        {
            ORIGIN_VERIFY_CONNECT(
                Origin::Engine::SingleLoginController::instance(), SIGNAL(firstUserConflict()),
                this, SLOT(onFirstUserConflict()));
            ORIGIN_VERIFY_CONNECT(
                Origin::Engine::SingleLoginController::instance(), SIGNAL(checkUserOnlineFinished(bool)),
                this, SLOT(onCheckUserOnlineFinished(bool)));
            ORIGIN_VERIFY_CONNECT(
                SingleLoginController::instance(), SIGNAL(checkUserInCloudSavesFinished(bool)),
                this, SLOT(onCheckCloudSavesCompleted(bool)));
            ORIGIN_VERIFY_CONNECT(
                this, SIGNAL(accountInUse()),
                Origin::Client::MainFlow::instance(), SIGNAL(singleLoginAccountInUse()));
        }

        void SingleLoginFlow::start()
        {
            SingleLoginController::instance()->postInitialize();
            checkUserOnline();
        }

        void SingleLoginFlow::checkUserOnline()
        {
            bool inForceOnlineMode = MainFlow::instance()->rtpFlow() && MainFlow::instance()->rtpFlow()->isInForceOnlineMode();
            SingleLoginController::instance()->checkUserOnline(
                Session::SessionService::currentSession(),
                OriginApplication::instance().isSingleLoginEnabled() && !inForceOnlineMode);
        }

        void SingleLoginFlow::checkUserInCloudSaves()
        {
            SingleLoginController::instance()->checkUserInCloudSaves(Session::SessionService::currentSession());
            ORIGIN_LOG_EVENT << "checking for concurrent user in the cloud";
        }

        void SingleLoginFlow::onLoginOnline()
        {
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("SingleLogin", "online");
            ORIGIN_LOG_EVENT << "Logging in online";

            // Check cloud saves first
            checkUserInCloudSaves();
        }

        void SingleLoginFlow::onLoginOffline()
        {
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("SingleLogin", "offline");
            ORIGIN_LOG_EVENT << "Logging into offline mode";

            emit(finished(SingleLoginFlowResult(FLOW_SUCCEEDED, true)));
        }

        void SingleLoginFlow::onCancelLogin()
        {
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("SingleLogin", "cancelled");
            ORIGIN_LOG_EVENT << "Log in cancelled";

            emit(finished(SingleLoginFlowResult(FLOW_FAILED, false)));
        }

        void SingleLoginFlow::onCheckUserOnlineFinished(bool isUserOnline)
        {
            if (isUserOnline)
            {
                //if the interval between when went offline and coming back online is small enough, skip the single login conflict check
                //because the XMPP server won't have detected that the user had gone offline and will erroneously report "your account is in use"
                Session::SessionRef currSession = Session::SessionService::currentSession();
                if (!currSession.isNull())
                {
                    const qint64 IGNORE_SINGLE_LOGIN_CONFLICT_INTERVAL_SEC = 45; //seconds

                    QDateTime wentOfflineTS = Session::SessionService::wentOfflineTime(currSession);
                    if (!wentOfflineTS.isNull())
                    {
                        qint64 offlineDurationSec = wentOfflineTS.secsTo(QDateTime::currentDateTimeUtc());
                        if (offlineDurationSec != 0 && offlineDurationSec < IGNORE_SINGLE_LOGIN_CONFLICT_INTERVAL_SEC)
                        {
                            QString errorStr = QString("Account already online:%1").arg(offlineDurationSec);
                            GetTelemetryInterface()->Metric_ERROR_NOTIFY("SingleLogin", errorStr.toUtf8());
                            ORIGIN_LOG_WARNING << "Single login detected this account is already online:" << offlineDurationSec;
                            Session::SessionService::resetWentOfflineTime(currSession);    //reset for next time
                            emit(finished(SingleLoginFlowResult(FLOW_SUCCEEDED, false)));
                            return;
                        }
                    }
                    else
                    {
                        ORIGIN_LOG_DEBUG << "wentOfflineTS = NULL";
                    }
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("SingleLogin", "Account already online:0");
                    ORIGIN_LOG_WARNING << "Single login detected this account is already online:0";
                }

                emit accountInUse();
                mSingleLoginViewController.reset(new SingleLoginViewController);
                ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(loginOnline()), this, SLOT(onLoginOnline()));
                ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(loginOffline()), this, SLOT(onLoginOffline()));
                ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(cancelLogin()), this, SLOT(onCancelLogin()));

                LoginType loginType = ClientFlow::instance() ? SecondaryLogin : PrimaryLogin;
                mSingleLoginViewController->showSecondUserDialog(loginType);
            }
            else
            {
                emit(finished(SingleLoginFlowResult(FLOW_SUCCEEDED, false)));
            }
        }

        void SingleLoginFlow::onCheckCloudSavesCompleted(bool isSomebodyWriting)
        {
            if (isSomebodyWriting)
            {
                if (mSingleLoginViewController && mSingleLoginViewController->isCloudDialogShowing())
                {
                    // If we're already showing it then just update it
                    mSingleLoginViewController->armTimer();
                }
                else
                {
                    mSingleLoginViewController.reset(new SingleLoginViewController);
                    ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(pollCloudSaves()), this, SLOT(checkUserInCloudSaves()));
                    ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(cancelLogin()), this, SLOT(onCancelLogin()));
                    mSingleLoginViewController->showCloudSyncDialog();
                }
            }
            else
            {
                if (mSingleLoginViewController && mSingleLoginViewController->isCloudDialogShowing())
                {
                    mSingleLoginViewController->closeCloudSyncDialog();
                }
                emit(finished(SingleLoginFlowResult(FLOW_SUCCEEDED, false)));
            }
        }

        void SingleLoginFlow::onFirstUserConflict()
        {
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("SingleLogin", "Another user logged in");
            ORIGIN_LOG_WARNING << "Another user is logging into this account!";

            mSingleLoginViewController.reset(new SingleLoginViewController);
            ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(loginOnline()), this, SLOT(onLoginOnline()));
            ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(loginOffline()), this, SLOT(onLoginOffline()));
            ORIGIN_VERIFY_CONNECT(mSingleLoginViewController.data(), SIGNAL(cancelLogin()), this, SLOT(onCancelLogin()));

            // TBD: determine login type to use
            mSingleLoginViewController->showFirstUserDialog();
        }

        void SingleLoginFlow::closeSingleLoginDialogs()
        {
            //TODO: This isn't the best way to do this but the SingleLoginViewController and SingleLoginView need cleaning to close up the dialogs properly
            foreach(QWidget * widget, QApplication::topLevelWidgets())
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                if (window && (window->objectName() == SingleLoginFirstDialogName || window->objectName() == SingleLoginSecondDialogName))
                {
                    window->close();
                }
            }

        }
    }
}