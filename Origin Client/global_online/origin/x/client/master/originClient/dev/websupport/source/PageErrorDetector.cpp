#include <QTimer>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QNetworkReply>

#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"

#include "PageErrorDetector.h"

using Origin::Services::NetworkAccessManager;

namespace Origin
{
namespace Client
{

PageErrorDetector::PageErrorDetector(QWebPage *page) :
    QObject(page),
    mPage(page),
    mTimeoutTimer(NULL),
    mCurrentErrorType(NoError),
    mPageTimeoutMilliseconds(30000)
{
    const int timeoutOverride = Services::readSetting(Services::SETTING_OverrideLoadTimeoutError, Services::Session::SessionService::currentSession());
    if(timeoutOverride != -1)
    {
        mPageTimeoutMilliseconds = timeoutOverride;
    }
    // Listen to our load lifetime signals
    connectDisconnectPageLoadSignals(true);
}

void PageErrorDetector::disarmTimeoutTimer()
{
    delete mTimeoutTimer;
    mTimeoutTimer = NULL;
}

void PageErrorDetector::handleDetectedError(ErrorType type, QNetworkReply *reply)
{
    if (mCurrentErrorType == NoError)
    {
        emit errorDetected(type, reply);

        disarmTimeoutTimer();
        mCurrentErrorType = type;
    }
}

bool PageErrorDetector::errorCheckEnabled(ErrorType type) const
{
    return mEnabledErrorChecks & type;
}

void PageErrorDetector::setErrorCheckEnabled(ErrorType type, bool enable)
{
    if (enable)
    {
        enableErrorCheck(type);
    }
    else
    {
        disableErrorCheck(type);
    }
}

void PageErrorDetector::enableErrorCheck(ErrorType type)
{
    mEnabledErrorChecks |= type;
}

void PageErrorDetector::disableErrorCheck(ErrorType type)
{
    mEnabledErrorChecks &= ~type;
}

void PageErrorDetector::pageLoadStarted()
{
    // Clear our error state
    mCurrentErrorType = NoError;

    // Snoop on network requests
    ORIGIN_VERIFY_CONNECT(mPage->networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
        this, SLOT(networkRequestFinished(QNetworkReply *)));

    if (errorCheckEnabled(LoadTimedOut))
    {
        if (!mTimeoutTimer)
        {
            mTimeoutTimer = new QTimer(this);
            mTimeoutTimer->setSingleShot(true);
            mTimeoutTimer->setInterval(mPageTimeoutMilliseconds);
        }

        mTimeoutTimer->start();

        ORIGIN_VERIFY_CONNECT(mTimeoutTimer, SIGNAL(timeout()),
                this, SLOT(loadTimedOut()));
    }
}

void PageErrorDetector::connectDisconnectPageLoadSignals(const bool& shouldConnect) const
{
    // Disconnect no matter what. If we get into the situation where we are connecting and 
    // are about to connect again, we don't want to have duplicate connections. Disconnecting signals
    // that weren't connected isn't a problem.
    ORIGIN_VERIFY_DISCONNECT(mPage, SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
    ORIGIN_VERIFY_DISCONNECT(mPage, SIGNAL(loadProgress(int)), this, SLOT(pageLoadProgress()));
    ORIGIN_VERIFY_DISCONNECT(mPage, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished(bool)));
    if(shouldConnect)
    {
        ORIGIN_VERIFY_CONNECT(mPage, SIGNAL(loadStarted()), this, SLOT(pageLoadStarted()));
        ORIGIN_VERIFY_CONNECT(mPage, SIGNAL(loadProgress(int)), this, SLOT(pageLoadProgress()));
        ORIGIN_VERIFY_CONNECT(mPage, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished(bool)));
    }
}

void PageErrorDetector::pageLoadFinished(bool success)
{

    // Stop snooping on network requests
    ORIGIN_VERIFY_DISCONNECT(mPage->networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
        this, SLOT(networkRequestFinished(QNetworkReply *)));

    if (!success && errorCheckEnabled(FinishedWithFailure))
    {
        handleDetectedError(FinishedWithFailure);
    }

    disarmTimeoutTimer();
}

void PageErrorDetector::pageLoadProgress()
{
    if (mTimeoutTimer)
    {
        // Restart the timeout clock
        mTimeoutTimer->start(mPageTimeoutMilliseconds);
    }
}
    
void PageErrorDetector::loadTimedOut()
{
    handleDetectedError(LoadTimedOut);
}

void PageErrorDetector::networkRequestFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError error = reply->error();

    // OperationCanceledError shows up for 304 Not Modified which we definitely
    // should not error on
    // There's a bug in Qt 4.7.3 that causes aborted transactions to return
    // ProtocolUnknownError if they're created and then aborted without
    // reentering the event loop
    // Actual protocol unknown errors should be highly marginal so just
    // ignore them entirely
    if (error != QNetworkReply::NoError  &&
        error != QNetworkReply::OperationCanceledError &&
        error != QNetworkReply::ProtocolUnknownError)
    {
        //This gets triggered from javascript and links from server generated web pages, so don't
        //log the full URL outside of debug as there may be private data within the query data
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
#ifdef _DEBUG
        ORIGIN_LOG_EVENT << "Load Error " << error << " (HTTP status " << status << ") for URL: " << reply->url().toString();
#else
        ORIGIN_LOG_EVENT << "Load Error " << error << " (HTTP status " << status << ") for host: " << reply->url().host();
#endif

        // Have we enabled this error check and is this request for us?
        if (errorCheckEnabled(MainFrameRequestFailed) && 
            (reply->url() == mPage->mainFrame()->url()) && 
            !reply->url().isEmpty())
        {
            handleDetectedError(MainFrameRequestFailed, reply);
        }
    }
}
    

}
}
