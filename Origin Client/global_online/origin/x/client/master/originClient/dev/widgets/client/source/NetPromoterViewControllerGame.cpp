/////////////////////////////////////////////////////////////////////////////
// NetPromoterViewControllerGame.cpp
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "NetPromoterViewControllerGame.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/NetPromoterService.h"
#include "services/settings/SettingsManager.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "TelemetryAPIDLL.h"
#include "PendingActionFlow.h"

#include "originwindow.h"
#include "originwebview.h"
#include "NativeBehaviorManager.h"
#include "OfflineCdnNetworkAccessManager.h"
#include "WebWidget/NativeInterface.h"
#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidget/WidgetLink.h"
#include "WebWidgetController.h"
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>

namespace Origin
{
namespace Client
{
const int NETPROMOTER_DAYS_SUPPRESS = 1;
const int NET_PROMOTER_TIMEOUT_MSECS = 4000;
const QString NetPromoterLinkRole("http://widgets.dm.origin.com/linkroles/netpromoter");

static bool sIsVisible = false;

NetPromoterViewControllerGame::NetPromoterViewControllerGame(const QString& productId, QObject* parent)
: QObject(parent)
, mWindow(NULL)
, mResponseTimer(NULL)
, mProductId(productId)
{
    ORIGIN_LOG_ACTION << "Creating";
}


NetPromoterViewControllerGame::~NetPromoterViewControllerGame()
{
    closeWindow();

    if(mResponseTimer)
    {
        delete mResponseTimer;
        mResponseTimer = NULL;
    }
    ORIGIN_LOG_ACTION << "Deleting";
}


void NetPromoterViewControllerGame::onReponseError()
{
    logResponseError();
    ORIGIN_VERIFY_DISCONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    mResponseTimer->stop();
    emit finish(false);
}

void NetPromoterViewControllerGame::logResponseError()
{
    Services::NetPromoterServiceResponse* response = dynamic_cast<Services::NetPromoterServiceResponse*>(sender());
    if(response && response->reply())
    {
        QString errorStr = response->reply()->errorString();
        errorStr = errorStr.replace(Services::nucleusId(Engine::LoginController::currentUser()->getSession()), "XXX");
        ORIGIN_LOG_ACTION << "Not showing - error | " << errorStr;
    }
}


void NetPromoterViewControllerGame::onTimeout()
{
    logTimeOut();
    emit finish(false);
}

void NetPromoterViewControllerGame::logTimeOut()
{
    ORIGIN_LOG_ACTION << "Not showing - timed out";
}

void NetPromoterViewControllerGame::show()
{
    ORIGIN_LOG_ACTION << "Trying to show";
    if(Services::readSetting(Services::SETTING_ShowGameNetPromoter))
    {
        // note that when forcing the NetPromoter, the Promo popup will not appear.
        onShow();
        return;
    }

    if(!okToShow())
    {
        // Can't call the slot because it'll want to clean up a timer we haven't created.
        emit finish(false);
        return;
    }

    int testrange_min = Services::readSetting(Services::SETTING_MinNetPromoterTestRange);
    int testrange_max = Services::readSetting(Services::SETTING_MaxNetPromoterTestRange);

    if(testrange_min < 0 || testrange_max < 0)	// no override range set?
    {
        Engine::UserRef user = Engine::LoginController::currentUser();
        Services::NetPromoterServiceResponse *response = Services::NetPromoterService::displayCheck(user->getSession(), Services::NetPromoterService::Game_PlayEnd, mProductId);

        if(response)
        {
            ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Services::HttpStatusCode)), this, SLOT(onReponseError()));
            ORIGIN_VERIFY_CONNECT(response, SIGNAL(success()), this, SLOT(onShow()));

            mResponseTimer = new QTimer(this);
            ORIGIN_VERIFY_CONNECT(mResponseTimer, SIGNAL(timeout()), response, SLOT(abort()));
            ORIGIN_VERIFY_CONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
            mResponseTimer->start(NET_PROMOTER_TIMEOUT_MSECS);
            response->setDeleteOnFinish(true);
        }
    }
    else
    {
        // loop through override range
        Engine::UserRef user = Engine::LoginController::currentUser();

        for(int i = testrange_min; i <= testrange_max; i++)
        {
            Services::NetPromoterServiceResponse *response = Services::NetPromoterService::displayCheck(user->getSession(), Services::NetPromoterService::Game_PlayEnd, mProductId, i);

            if(response)
            {
                if(testrange_min == testrange_max)	// if the numbers are the same, just act like normal 
                {
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onReponseError()));
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


bool NetPromoterViewControllerGame::okToShow()
{
    // User underage
    Engine::UserRef user = Engine::LoginController::currentUser();
    if(!user || user->isUnderAge())
    {
        ORIGIN_LOG_ACTION << "Not showing - User is underage";
        return false;
    }

    PendingActionFlow *actionFlow = PendingActionFlow::instance();
    if (actionFlow && actionFlow->suppressPromoState())
    {
        ORIGIN_LOG_ACTION << "Not showing - suppressed from localhost/origin2:// call";
        return false;
    }

    // Verify that the game NPS is not asked on the same day with Origin NPS
    if(isTimeToShow(Services::readSetting(Services::SETTING_NETPROMOTER_LASTSHOWN, Services::Session::SessionService::currentSession()), NETPROMOTER_DAYS_SUPPRESS) == false)
    {
        ORIGIN_LOG_ACTION << "Not showing - Origin Promoter showed recently";
        return false;
    }

    // Verify that game NPS is asked only for the 1st game user plays on the assigned day.
    // - Verify that user is not asked for NPS for more than one game per day.
    if(isTimeToShow(Services::readSetting(Services::SETTING_NETPROMOTER_GAME_LASTSHOWN, Services::Session::SessionService::currentSession()), NETPROMOTER_DAYS_SUPPRESS) == false)
    {
        ORIGIN_LOG_ACTION << "Not showing - Game Promoter showed recently";
        return false;
    }

    Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductId);
    if(ent->contentConfiguration()->isNonOriginGame())
    {
        ORIGIN_LOG_ACTION << "Not showing - not Origin entitlement";
        return false;
    }

    if(ent->contentConfiguration()->netPromoterScoreBlacklist())
    {
        ORIGIN_LOG_ACTION << "Not showing - entitlement blacklisted";
        return false;
    }

    // Verify that user is not asked for NPS if his/her Nucleus ID does not match the server assigned number (or related rule)
    // In below picture - if user whose Nucleus ID ends with "00" does not use Origin client on the day when server assigned number is 00, then user should not be asked for Origin NPS in 99 days

    return true;
}


void NetPromoterViewControllerGame::onShow()
{
    if(mResponseTimer)
    {
        ORIGIN_VERIFY_DISCONNECT(mResponseTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
        mResponseTimer->stop();
    }

    if(mWindow == NULL)
    {
        //gets deleted by OriginWebView
        WebWidget::WidgetView* webView = new WebWidget::WidgetView();
        // Act and look more native
        NativeBehaviorManager::applyNativeBehavior(webView);
        ORIGIN_VERIFY_CONNECT(webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));


        using namespace UIToolkit;
        mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close | OriginWindow::Minimize,
            NULL, OriginWindow::WebContent);

        OriginWebView* webContainer = mWindow->webview();
        webContainer->setIsContentSize(true);
        webContainer->setHasSpinner(true);
        QPalette palette = webContainer->webview()->palette();
        palette.setBrush(QPalette::Base, Qt::transparent);
        webView->setPalette(palette);
        webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
        webContainer->webview()->page()->setPalette(palette);
        webContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        webContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
        OfflineCdnNetworkAccessManager* offlineNam = new OfflineCdnNetworkAccessManager(webView->page());
        // Listen for our requests finishing up so we can check for errors
        ORIGIN_VERIFY_CONNECT(offlineNam, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkRequestFinished(QNetworkReply*)));
        webView->widgetPage()->setExternalNetworkAccessManager(offlineNam);
        webContainer->setWebview(webView);
        QMap<QString, QString> variables;
        variables["offerId"] = mProductId;
        WebWidgetController::instance()->loadUserSpecificWidget(webView->widgetPage(), "netpromoter", Engine::LoginController::instance()->currentUser(), WebWidget::WidgetLink(NetPromoterLinkRole, variables));

        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onReject()));
        mWindow->showWindow();
        Services::writeSetting(Services::SETTING_NETPROMOTER_GAME_LASTSHOWN, QDate::currentDate().toJulianDay(), Services::Session::SessionService::currentSession());
        sIsVisible = true;
        ORIGIN_LOG_ACTION << "Showing " << mProductId;
    }
}


void NetPromoterViewControllerGame::populatePageJavaScriptWindowObject()
{
    QWebFrame* frame = dynamic_cast<QWebFrame*>(sender());
    if(frame)
    {
        frame->addToJavaScriptWindowObject("helper", this);
    }
}


void NetPromoterViewControllerGame::onNetworkRequestFinished(QNetworkReply* networkReply)
{
    // Report Queue window network errors to telemetry, such as when 
    // boxart fails to load
    if (networkReply->error() != QNetworkReply::NoError)
    {
        ORIGIN_LOG_ACTION << "Problem loading boxart - error: " << networkReply->errorString();
        QNetworkReply::NetworkError qnetError = networkReply->error();
        int status = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString urlString(networkReply->url().toString());

        // Cache control indicates if this was an offline/cached load
        int cacheControl = networkReply->attribute(QNetworkRequest::CacheLoadControlAttribute).toInt();

        GetTelemetryInterface()->Metric_QUEUE_GUI_HTTP_ERROR(qnetError, status, cacheControl, urlString.toUtf8().constData());
    }
}


void NetPromoterViewControllerGame::onReject()
{
    //  TELEMETRY:  report to telemetry that the user didn't answer
    GetTelemetryInterface()->Metric_NET_PROMOTER_GAME_SCORE(mProductId.toLatin1().data(), "-1", "", "", false);
    closeWindow();
}

void NetPromoterViewControllerGame::closeWindow()
{
    if(mWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onReject()));
        mWindow->close();
        emit finish(true);
        ORIGIN_LOG_ACTION << "Closing window";
    }
    mWindow = NULL;
    sIsVisible = false;
}


bool NetPromoterViewControllerGame::isSurveyVisible()
{
    return sIsVisible;
}


bool NetPromoterViewControllerGame::isTimeToShow(const int& lastJulianDay, const int& daysToWait)
{
    QDate lastShowedPromo;
    if(lastJulianDay)
    {
        lastShowedPromo = QDate::fromJulianDay(lastJulianDay); 
    }

    return (!lastShowedPromo.isValid() || lastShowedPromo.daysTo(QDate::currentDate()) >= daysToWait);
}

}
}