#include "PageErrorHandler.h"

#include <QtWebKitWidgets/QWebPage>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>
#include <QMetaObject>

#include "AbstractErrorPage.h"
#include "DefaultErrorPage.h"
#include "PageErrorDetector.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"

using Origin::Services::Connection::ConnectionStatesService;
using namespace Origin::Services::Session;

namespace Origin
{
namespace Client
{

PageErrorHandler::PageErrorHandler(QWebPage *page, AbstractErrorPage *errorPage) :
    QObject(page),
    mPage(page),
    mPageErrorDetector(new PageErrorDetector(page)),
    mHandleOfflineAsError(false),
    mTrackCurrentUrl(true),
	mOffline(false)
{
    mErrorPage = errorPage ? errorPage : new DefaultErrorPage();

    // Hook up our error detector signal
    ORIGIN_VERIFY_CONNECT(mPageErrorDetector, SIGNAL(errorDetected(Origin::Client::PageErrorDetector::ErrorType, QNetworkReply *)),
            this, SLOT(pageErrorDetected(Origin::Client::PageErrorDetector::ErrorType, QNetworkReply *)));

    // This class needs to detect changes in online state, which may be caused by changes in the 
    // global connection state, the per user connection state, as well as log in and log out events
    // since these affect whether there is a per user connection state.
    // global state change
    ORIGIN_VERIFY_CONNECT(ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
            this, SLOT(onConnectionStateChanged()));
    // per user state change
    ORIGIN_VERIFY_CONNECT(ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
            this, SLOT(onConnectionStateChanged()));
    // user state change to recovering
    ORIGIN_VERIFY_CONNECT(ConnectionStatesService::instance(), SIGNAL(nowRecovering()),
            this, SLOT(onRecovering()));
    // log in event
    ORIGIN_VERIFY_CONNECT( Origin::Services::Session::SessionService::instance(), SIGNAL(beginSessionComplete(Origin::Services::Session::SessionError,Origin::Services::Session::SessionRef,QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>)), 
        this, SLOT(onConnectionStateChanged()));
    // log out event
    ORIGIN_VERIFY_CONNECT( Origin::Services::Session::SessionService::instance(), SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)), 
        this, SLOT(onConnectionStateChanged()));

    // initialize based on the current connection state
    updateConnectionState();
}

PageErrorHandler::~PageErrorHandler()
{
    delete mErrorPage;
}

void PageErrorHandler::setRequestToRestore(const QNetworkRequest &req, QNetworkAccessManager::Operation op, const QByteArray &body)
{
    //only set it, when we have the track current url flag set to true
    if(mTrackCurrentUrl)
    {
        mRequestToRestore = req;
        mRequestToRestoreOperation = op;
        mRequestToRestoreBody = body;
    }
}

void PageErrorHandler::showOfflineError()
{
    QString errorHtml = mErrorPage->offlineErrorHtml();

    if (!errorHtml.isNull())
    {
        QMetaObject::invokeMethod(this, "deferredShowErrorContent", Qt::QueuedConnection, Q_ARG(QString, errorHtml));
    }
}

void PageErrorHandler::showPageError(PageErrorDetector::ErrorType type, QNetworkReply *reply)
{
    QString errorHtml = mErrorPage->pageErrorHtml(type, reply);

    if (!errorHtml.isNull())
    {
        QMetaObject::invokeMethod(this, "deferredShowErrorContent", Qt::QueuedConnection, Q_ARG(QString, errorHtml));
    }
}

void PageErrorHandler::loadStartedWhileOffline()
{
    if(!mLoadingErrorContent)
    {
        if(mTrackCurrentUrl)
        {
            // Keep track of which URL to show when we return online
            setRequestToRestore(QNetworkRequest(mPage->history()->currentItem().url()));
        }
        mPage->triggerAction(QWebPage::Stop);
        showOfflineError();
    }
}

void PageErrorHandler::setHandleOfflineAsError(bool handleAsError) 
{
    mHandleOfflineAsError = handleAsError;
    updateConnectionState();
}

void PageErrorHandler::onConnectionStateChanged()
{
    updateConnectionState();
}

void PageErrorHandler::onRecovering()
{
    if (mHandleOfflineAsError && mOffline)
    {
        // print the recovering message
        QString errorHtml = mErrorPage->recoveringErrorHtml();
        if (!errorHtml.isNull())
        {
            QMetaObject::invokeMethod(this, "deferredShowErrorContent", Qt::QueuedConnection, Q_ARG(QString, errorHtml));
        }
    }
}

void PageErrorHandler::updateConnectionState()
{
    SessionRef session = SessionService::currentSession();
    bool online = false;
    if (session)
        online = ConnectionStatesService::isUserOnline(session);
    else
        online = ConnectionStatesService::isGlobalStateOnline();

    if (online)
        nowOnline();
    else
        nowOffline();
}

void PageErrorHandler::nowOffline()
{
	mOffline = true;

    if (mHandleOfflineAsError)
    {
        // This is distasteful but necessary unless we want to rearchitect how the NavigationController works
        // Since it just interacts directly with the QWebHistory it has no knowledge of offline mode or even
        // whether to show an error page when offline.   We catch the loads here when offline and redirect to
        // the offline error page if we consider offline mode an error mode.
        ORIGIN_VERIFY_CONNECT(mPage, SIGNAL(loadStarted()), this, SLOT(loadStartedWhileOffline()));

        // Keep track of which URL to show when we return online, only retain valid URLs (EBIBUGS-20018)
        QUrl toRestore = mPage->mainFrame()->url();
        if ( toRestore.isValid() && toRestore.toString() != "about:blank" && mTrackCurrentUrl )
            setRequestToRestore(QNetworkRequest(toRestore));

        showOfflineError();
    }
}

void PageErrorHandler::nowOnline()
{
	mOffline = false;

	if(mHandleOfflineAsError)
    {
        ORIGIN_VERIFY_DISCONNECT(mPage, SIGNAL(loadStarted()), this, SLOT(loadStartedWhileOffline()));
    }

    if (mRequestToRestore.url().isValid())
    {
        mPage->mainFrame()->load(mRequestToRestore, mRequestToRestoreOperation, mRequestToRestoreBody);

        if(mTrackCurrentUrl)
        {
            // We don't want to clear this out if we are re-using the initial request.
            setRequestToRestore(QNetworkRequest());
        }
    }
}

void PageErrorHandler::pageErrorDetected(PageErrorDetector::ErrorType type, QNetworkReply *reply)
{
    if (mHandleOfflineAsError) 
    {
        if (ConnectionStatesService::isGlobalStateOnline() && PageErrorDetector::LoadTimedOut == type)
		{
			// Online, but the page load timed out. Show page error: could be due to heavy traffic or site maintenance.
			showPageError(type, reply);
		}

		// Otherwise assume the error was due to being offline
		// This should be handled in nowOffline()
    }    
}

void PageErrorHandler::setErrorPage(AbstractErrorPage *page)
{
    delete mErrorPage;
    mErrorPage = page;
}

void PageErrorHandler::deferredShowErrorContent(QString content)
{
    mLoadingErrorContent = true;
    // We want to keep track of what our error type is while we are showing an error page.
    // PageErrorDetector::pageLoadStarted clears it out when we load the error page if we don't disconnect the signals.
    mPageErrorDetector->connectDisconnectPageLoadSignals(false);
    mPage->mainFrame()->setHtml(content);
    mPageErrorDetector->connectDisconnectPageLoadSignals(true);
    mLoadingErrorContent = false;
}
}
}
