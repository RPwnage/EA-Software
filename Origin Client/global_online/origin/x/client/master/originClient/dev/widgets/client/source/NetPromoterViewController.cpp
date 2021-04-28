/////////////////////////////////////////////////////////////////////////////
// PromoBrowserViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "NetPromoterViewController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "engine/login/LoginController.h"

#include "originwindow.h"
#include "originscrollablemsgbox.h"
#include "originwindowmanager.h"
#include "NetPromoterWidget.h"
#include "TelemetryAPIDLL.h"
#include "services/rest/NetPromoterService.h"

#include "services/settings/SettingsManager.h"
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "MainFlow.h"
#include "PendingActionFlow.h"
#include "RTPFlow.h"

#include <QLocale>

namespace Origin
{
    namespace Client
    {
        using Origin::Services::Session::SessionRef;
        using Origin::Services::NetPromoterService;
        using Origin::Services::NetPromoterServiceResponse;

        const int NET_PROMOTER_TIMEOUT_MSECS = 4000;

        static int sIsVisible = -1;

        NetPromoterViewController::NetPromoterViewController(QWidget *parent)
        : QObject(parent)
        , mDialog(NULL)
        , mNetPromoter(NULL)
        , mResponseTimer(NULL)
        {

        }


        NetPromoterViewController::~NetPromoterViewController()
        {
            closeNetPromoterDialog();

            if (mResponseTimer)
            {
                delete mResponseTimer;
                mResponseTimer = NULL;
            }
            ORIGIN_LOG_ACTION << "Deleting";
        }

        void NetPromoterViewController::onNoShow()
        {
            ORIGIN_VERIFY_DISCONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
            mResponseTimer->stop();
            sIsVisible = 0;
            emit onNoNetPromoter();
            logResponseError();
        }

        void NetPromoterViewController::logResponseError()
        {
            NetPromoterServiceResponse* response = dynamic_cast<NetPromoterServiceResponse*>(sender());
            if(response)
            {
                QString errorStr = response->reply()->errorString();
                errorStr = errorStr.replace(Services::nucleusId(Engine::LoginController::currentUser()->getSession()), "XXX");
                ORIGIN_LOG_ACTION << "Not showing - error | " << errorStr;
            }
        }

        void NetPromoterViewController::onTimeout()
        {
            logTimeOut();
            sIsVisible = 0;
            emit onNoNetPromoter();
        }

        void NetPromoterViewController::logTimeOut()
        {
            ORIGIN_LOG_ACTION << "Not showing - timed out";
        }

        void NetPromoterViewController::show()
        {
            bool forceShow = Origin::Services::readSetting(Origin::Services::SETTING_ShowNetPromoter);
            if ( forceShow )
            {
                // note that when forcing the NetPromoter, the Promo popup will not appear.
                onShow();
                return;
            }

            if ( !okToShow() )
            {
                // Can't call the slot because it'll want to clean up a timer we haven't created.
                sIsVisible = 0;
                emit onNoNetPromoter();
                return;
            }

            int testrange_min = Origin::Services::readSetting(Origin::Services::SETTING_MinNetPromoterTestRange);
            int testrange_max = Origin::Services::readSetting(Origin::Services::SETTING_MaxNetPromoterTestRange);

            if (testrange_min < 0 || testrange_max < 0)	// no override range set?
            {
                Engine::UserRef user = Engine::LoginController::currentUser();
                NetPromoterServiceResponse *response = NetPromoterService::displayCheck(user->getSession(), Services::NetPromoterService::Origin_General);

                if (response)
                {
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onNoShow()));
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(success()), this, SLOT(onShow()));

                    mResponseTimer = new QTimer(this);
                    ORIGIN_VERIFY_CONNECT(mResponseTimer, SIGNAL(timeout()), response, SLOT(abort()));
                    ORIGIN_VERIFY_CONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
                    mResponseTimer->start(NET_PROMOTER_TIMEOUT_MSECS);
                    response->setDeleteOnFinish(true);
                }
            }
            else
            {	// loop through override range
                Engine::UserRef user = Engine::LoginController::currentUser();

                for (int i = testrange_min; i <= testrange_max; i++)
                {
                    NetPromoterServiceResponse *response = NetPromoterService::displayCheck(user->getSession(), Services::NetPromoterService::Origin_General, "", i);

                    if (response)
                    {
                        if (testrange_min == testrange_max)	// if the numbers are the same, just act like normal 
                        {
                            ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onNoShow()));
                            ORIGIN_VERIFY_CONNECT(response, SIGNAL(success()), this, SLOT(onShow()));

                            mResponseTimer = new QTimer(this);
                            ORIGIN_VERIFY_CONNECT(mResponseTimer, SIGNAL(timeout()), response, SLOT(abort()));
                            ORIGIN_VERIFY_CONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
                            mResponseTimer->start(NET_PROMOTER_TIMEOUT_MSECS);
                            response->setDeleteOnFinish(true);
                        }
                        else
                        {	// otherwise only react if the response is success
                            ORIGIN_VERIFY_CONNECT(response, SIGNAL(success()), this, SLOT(onShow()));
                            ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(logResponseError()));

                            // QA requested for there not to be a timer for when we have a range.
                            response->setDeleteOnFinish(true);
                        }
                    }
                }
            }
        }

        bool NetPromoterViewController::okToShow()
        {
            QString firstStartStr = Services::readSetting(Services::SETTING_FIRSTTIMESTARTED).toString();
            // ISO 8601 date format
            QDateTime firstStart = QDateTime::fromString(firstStartStr, "yyyy-MM-ddThh:mm:ssZ");
            QDateTime now = QDateTime::currentDateTime();

            // Don't show the net promoter if they installed the client today.
            if (firstStart.isValid() && firstStart.date() == now.date())
            {
                ORIGIN_LOG_ACTION << "Not showing - Origin installed today";
                return false;
            }

        	QList<Engine::Content::EntitlementRef> entitlements = Engine::Content::ContentController::currentUserContentController()->entitlements();
        	QList<Engine::Content::EntitlementRef>::const_iterator entIter = entitlements.constBegin();

            // Don't show the net promoter if they received an entitlement within 48 hours.
            while (entIter != entitlements.constEnd())
            {
                Engine::Content::EntitlementRef entRef = (*entIter);
                QDateTime grantDate = entRef->contentConfiguration()->entitleDate();

                if (grantDate.isValid() && grantDate.addDays(2) > now)
                {
                    ORIGIN_LOG_ACTION << "Not showing - received entitlement recently";
                    return false;
                }

                entIter++;
            }

            // Don't show the net promoter if we started up to do an RtP launch.
            if (MainFlow::instance()->rtpFlow()->getRtpLaunchActive())
            {
                ORIGIN_LOG_ACTION << "Not showing - RtP active";
                return false;
            }

            Engine::UserRef user = Engine::LoginController::currentUser();
            if ( !user || user->isUnderAge() )
            {
                ORIGIN_LOG_ACTION << "Not showing - underage";
                return false;
            }

            // Not suppressed via localhost/origin2:// command
            PendingActionFlow *actionFlow = PendingActionFlow::instance();
            if (actionFlow && actionFlow->suppressPromoState())
            {
                ORIGIN_LOG_ACTION << "Not showing - suppressed by localhost/origin2:// call";
                return false;
            }

            ORIGIN_LOG_ACTION << "client - ok to show";
            return true;
        }

        void NetPromoterViewController::onShow()
        {
            if (mResponseTimer)
            {
                ORIGIN_VERIFY_DISCONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
                mResponseTimer->stop();
            }

            if (!mDialog)
            {
                using namespace Origin::UIToolkit;
                mDialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Close | OriginWindow::Minimize,
                    NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                
                Engine::UserRef user = Engine::LoginController::currentUser();
                mNetPromoter = new NetPromoterWidget(TWO_QUESTION);

                mDialog->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon,
                    tr("ebisu_client_origin_survey_title").arg(tr("application_name_uppercase")),
                    tr("ebisu_client_origin_one_survey_text").arg(tr("application_name")));
                mDialog->scrollableMsgBox()->setHasDynamicWidth(true);
                mDialog->button(QDialogButtonBox::Ok)->setText(tr("ebisu_client_submit"));
                mDialog->button(QDialogButtonBox::Ok)->setEnabled(false);
                mDialog->setDefaultButton(QDialogButtonBox::Cancel);
                mDialog->manager()->setupButtonFocus();
                connect(mNetPromoter, SIGNAL(surveySelected(bool)), this, SLOT(suveryChanged(bool)));

                ORIGIN_VERIFY_CONNECT(mDialog, SIGNAL(rejected()), this, SLOT(onUserCancelNetPromoterDialog()));
                ORIGIN_VERIFY_CONNECT(mDialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onUserSubmitNetPromoterDialog()));
                ORIGIN_VERIFY_CONNECT(mDialog->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onUserCancelNetPromoterDialog()));
                mNetPromoter->adjustSize();
                mDialog->setContent(mNetPromoter);
                mDialog->showWindow();
                ORIGIN_LOG_ACTION << "showing";
                Services::writeSetting(Services::SETTING_NETPROMOTER_LASTSHOWN, QDate::currentDate().toJulianDay(), Services::Session::SessionService::currentSession());
                sIsVisible = 1;
            }
        }

        void NetPromoterViewController::onUserSubmitNetPromoterDialog()
        {
            if (mNetPromoter)
            {
                QString locale = QLocale::system().name();
                QString score = QString("%1").arg(mNetPromoter->surveyNumber());
                bool canContact = mNetPromoter->canContactUser();
                QString feedback = mNetPromoter->surveyText();
                QByteArray feedbackBytes = feedback.toUtf8().toBase64();
                // adjust length by chopping of characters (in the QString), chopping bytes 
                // (in the QByteArray) might upset the last encoded character.
                const int TEXT_LIMIT = 4000; // telemetry limit is 4k
                while ( feedbackBytes.size() > TEXT_LIMIT )
                {
                    feedback.chop(1);
                    feedbackBytes = feedback.toUtf8().toBase64();
                }
                GetTelemetryInterface()->Metric_NET_PROMOTER_SCORE(score.toLatin1().data(), 
                    feedbackBytes.data(), locale.toLatin1().data(), canContact);

                closeNetPromoterDialog();
                showConfirmationDialog();
                ORIGIN_LOG_ACTION << "finished - true";
            }
        }

        void NetPromoterViewController::onUserCancelNetPromoterDialog()
        {
            //  TELEMETRY:  report to telemetry that the user didn't answer
            ORIGIN_LOG_ACTION << "finished - false";
            GetTelemetryInterface()->Metric_NET_PROMOTER_SCORE("-1", "", "", false);
            closeNetPromoterDialog();
        }

        void NetPromoterViewController::suveryChanged(bool selected)
        {
            if (mDialog)
            {
                Origin::UIToolkit::OriginPushButton* btn = NULL;
                if (selected)
                {
                    mDialog->button(QDialogButtonBox::Ok)->setEnabled(true);
                    btn = mDialog->button(QDialogButtonBox::Ok);
                }
                else
                {
                    btn = mDialog->button(QDialogButtonBox::Cancel);
                }
                if (mDialog->defaultButton() != btn)
                {
                    mDialog->setDefaultButton(btn);
                    mDialog->manager()->resetDefaultButton(btn);
                }
            }
        }

        void NetPromoterViewController::showConfirmationDialog()
        {
            Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::Success,
                tr("ebisu_client_thank_you_for_your_feedback"),
                "",
                tr("ebisu_client_ok"));
        }

        void NetPromoterViewController::closeNetPromoterDialog()
        {
            if (mDialog)
            {
                ORIGIN_VERIFY_DISCONNECT(mDialog, SIGNAL(rejected()), this, SLOT(onUserCancelNetPromoterDialog()));
                ORIGIN_VERIFY_DISCONNECT(mDialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onUserSubmitNetPromoterDialog()));
                ORIGIN_VERIFY_DISCONNECT(mDialog->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onUserCancelNetPromoterDialog()));
                mDialog->close();
                ORIGIN_LOG_ACTION << "Deleting window";
            }
            mDialog = NULL;
            mNetPromoter = NULL;
            sIsVisible = 0;
        }

        int NetPromoterViewController::isSurveyVisible()
        {
            return sIsVisible;
        }
    }
}
