/////////////////////////////////////////////////////////////////////////////
// MOTDViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "OriginApplication.h"
#include "services/debug/DebugService.h"
#include "MOTDViewController.h"
#include "services/settings/SettingsManager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"
#include "MOTDJsHelper.h"
#include "engine/login/LoginController.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
// Legacy
#include "services/network/NetworkAccessManager.h"
#include "EbisuSystemTray.h"
#include "CommonJsHelper.h"

namespace Origin
{
    namespace Client
    {

        static bool isVisible = false;

        MOTDViewController::MOTDViewController(const QString &motdUrlTemplate, QObject *parent) 
            : QObject(parent)
            , mMOTDBrowser(NULL)
            , mMOTDWindow(NULL)
            , mMotdJsHelper(NULL)
            , mCommonJsHelper(NULL)
            , mPollTimerArmed(false)
        {
            mMotdUrlTemplate = motdUrlTemplate;

            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), this, SLOT(onUserConnectionStateChanged(bool, Origin::Services::Session::SessionRef)));

            // Do an initial poll
            pollMotd();
        }

        void MOTDViewController::pollMotd()
        {
            // Instantiate the URL with the current locale
            // Note we have to do on the fly as the locale can be changed at runtime
            QString locale = OriginApplication::instance().locale();

            // Make a nice version string
            QString version = QCoreApplication::applicationVersion();
            version.remove(version.lastIndexOf("."), version.size());

            QString url = mMotdUrlTemplate.arg(version).arg(locale);

            QUrl motdUrl(url);

            pollMotd(motdUrl);
        }

        void MOTDViewController::pollMotd(const QUrl &url)
        {
            //if there is an active session, check against userOnline
            //otherwise, check against globalOnline (could be sitting on login window)
            bool isOnline = false;

            if (!Origin::Engine::LoginController::currentUser().isNull() && Origin::Engine::LoginController::currentUser()->getSession())   //valid user and session
            {
                isOnline = Origin::Services::Connection::ConnectionStatesService::isUserOnline (Origin::Engine::LoginController::currentUser()->getSession());
            }
            else    //just check against global state
            {
                isOnline = Origin::Services::Connection::ConnectionStatesService::isGlobalStateOnline ();
            }

            if (isOnline)
            {
                mLastMotdCheck = QDateTime::currentDateTime();

                QNetworkRequest req(url);

                // Always go to the network as we manage caching internally
                req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QVariant(QNetworkRequest::AlwaysNetwork));

                if (!mLastMotdValidator.isEmpty())
                {
                    // Don't show a MOTD we've already shown
                    // This turns it in to a conditional request. The server will respond
                    // with 304 if the content is the same as we last requested
                    mLastMotdValidator.addRequestHeaders(req);
                }

                // Tell the server we can only accept HTML in case someone puts something
                // weird at our URL
                req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9");

                QNetworkReply *reply = Origin::Services::NetworkAccessManager::instance()->get(req);
                ORIGIN_VERIFY_CONNECT(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
            }
            else
            {
                armPollTimer();
            }
        }

        void MOTDViewController::requestFinished()
        {
            QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError)
            {
                // Nothing to do
                armPollTimer();
                return;
            }

            unsigned int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (statusCode == 200)
            {
                if (mLastMotdValidator.match(reply) != Origin::Services::HttpCacheValidator::NoMatch)
                {
                    // Our server provides us with validators but doesn't honour them. Akamai
                    // will do this with ETags in some situations. This is inefficient because
                    // the resource is going over the network but we can easily ignore the
                    // 200 here
                    armPollTimer();
                    return;
                }

                using namespace Origin::UIToolkit;
                if(mMOTDWindow == NULL)
                {
                    mMOTDWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close), NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
                    mMOTDWindow->setTitleBarText(tr("ebisu_client_message_of_the_day"));
                    mMOTDWindow->setBannerText(tr("ebisu_client_message_of_the_day_notice"));
                    mMOTDWindow->setBannerIconType(OriginBanner::Info);
                    mMOTDWindow->setBannerVisible(true);
                    ORIGIN_VERIFY_CONNECT(mMOTDWindow, SIGNAL(rejected()), this, SLOT(destroyPage()));
                    ORIGIN_VERIFY_CONNECT(mMOTDWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(destroyPage()));
                    mMOTDWindow->manager()->setupButtonFocus();
                    mMOTDBrowser = new QWebView();
                    mMotdJsHelper = new MOTDJsHelper(mMOTDBrowser);
                    mCommonJsHelper = new CommonJsHelper(mMOTDBrowser);
                    mMOTDBrowser->page()->mainFrame()->addToJavaScriptWindowObject("motdJsHelper", mMotdJsHelper);
                    mMOTDBrowser->page()->mainFrame()->addToJavaScriptWindowObject("commonHelper", mCommonJsHelper);
                }

                // Use the content directly
                mMOTDBrowser->setContent(reply->readAll(), QString(), reply->url());
                mMOTDWindow->msgBox()->setText(mMOTDBrowser->page()->mainFrame()->toHtml());

                // Minimize the window if we are auto starting from windows and have auto login checked.
                StartupState startupState = OriginApplication::instance().startupState();
                if(startupState == StartInTaskbar || startupState == StartInSystemTray)
                    mMOTDWindow->showMinimized();
                else mMOTDWindow->showWindow();
                isVisible = true;

                // Telemetry and logging for showing the MOTD
                GetTelemetryInterface()->Metric_APP_MOTD_SHOW(reply->url().toString().toLatin1().data(), 
                    mMOTDBrowser->page()->mainFrame()->toPlainText().toUtf8().toBase64().data());
                ORIGIN_LOG_EVENT << "Showing MOTD window.  URL = " << reply->url().toString() << ".  Message = " << mMOTDBrowser->page()->mainFrame()->toPlainText();

                // Keep track of identifying information for the URL
                mLastMotdValidator = Origin::Services::HttpCacheValidator(reply);

                if (!mLastMotdValidator.isEmpty())
                {
                    // Start polling again once they close the MOTD
                    ORIGIN_VERIFY_CONNECT(mMOTDWindow, SIGNAL(rejected()), this, SLOT(armPollTimer()));
                }

                // If we don't have a validator stop polling as we can't tell when new MOTDs
                // are put up
            }
            else if ((statusCode == 301) || 
                (statusCode == 302) ||
                (statusCode == 303) ||
                (statusCode == 307))
            {
                QVariant variantTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
                ORIGIN_ASSERT(!variantTarget.isNull());

                // Poll the next URL
                pollMotd(reply->url().resolved(variantTarget.toUrl()));
            }
            else
            {
                // Some sort of error or 304 Not Modified (doesn't matter which)
                // Retry later
                armPollTimer();
            }

        }

        void MOTDViewController::onUserConnectionStateChanged(bool isOnline, Origin::Services::Session::SessionRef session)
        {
            if(isOnline)
            {
                // If we've never checked, or our last check was more than 15 minutes ago, we'll just go
                // ahead and poll it now.  Otherwise we'll arm it like normal
                if(!mLastMotdCheck.isValid() || mLastMotdCheck.secsTo(QDateTime::currentDateTime()) > (15*60))
                {
                    pollMotd();
                }
                else
                {
                    armPollTimer();
                }
            }
        }

        void MOTDViewController::armPollTimer()
        {
            if(!mPollTimerArmed)
            {
                // Check again in 30 minutes +/- 5 minutes
                // If we don't fuzz this we get clients falling in to lockstep for some reason
                const unsigned int nextPollIntervalSecs = (25 * 60) + (rand() % (10 * 60));

                mPollTimerArmed = true;
                QTimer::singleShot(nextPollIntervalSecs * 1000, this, SLOT(pollTimerFired()));
            }
        }

        void MOTDViewController::pollTimerFired()
        {
            mPollTimerArmed = false;
            pollMotd();
        }

        void MOTDViewController::destroyPage()
        {
            if(mCommonJsHelper)
            {
                mCommonJsHelper->deleteLater();
                mCommonJsHelper = NULL;
            }

            if(mMotdJsHelper)
            {
                mMotdJsHelper->deleteLater();
                mMotdJsHelper = NULL;
            }

            if(mMOTDBrowser)
            {
                mMOTDBrowser->deleteLater();
                mMOTDBrowser = NULL;
            }

            if(mMOTDWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mMOTDWindow, SIGNAL(rejected()), this, SLOT(destroyPage()));
                ORIGIN_VERIFY_DISCONNECT(mMOTDWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(destroyPage()));
                mMOTDWindow->close();
                mMOTDWindow = NULL;
            }
            isVisible = false;

            // Telemetry and logging for MOTD window closing
            GetTelemetryInterface()->Metric_APP_MOTD_CLOSE();
            ORIGIN_LOG_EVENT << "Closing MOTD window";
        }

        bool MOTDViewController::isBrowserVisible()
        {
            return isVisible;
        }

        void MOTDViewController::raise()
        {
            if (mMOTDWindow)
                mMOTDWindow->showWindow();
        }
    }
}
