// *********************************************************************
//  IGOController.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
// *********************************************************************

#include "engine/igo/IGOController.h"

#include "IGOGameTracker.h"

#include "engine/content/ContentController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/process/IProcess.h"
#include "services/escalation/IEscalationClient.h"
#include "services/platform/PlatformService.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "services/settings/InGameHotKey.h"
#include "services/downloader/Common.h"
#include "services/settings/SettingsManager.h"
#include "services/rest/AuthPortalServiceClient.h"
#include "services/rest/AuthPortalServiceResponses.h"

#if defined(ORIGIN_PC)
#include "IGOWinHelpers.h"
#include "services/process/ProcessWin.h"
#include "IGOSharedStructs.h"

#elif defined(ORIGIN_MAC)
#include "MacDllMain.h"
#include "IGOMacHelpers.h"
#include "engine/igo/IGOSetupOSX.h"
#include "services/log/LogService.h"
#include "services/process/ProcessOSX.h"
#include "services/escalation/IEscalationClient.h"
#endif

#include "IGOWindowManager.h"
#include "IGOWindowManagerIPC.h"
#include "IGOQtContainer.h"
#include "IGOQWidget.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOTelemetryIPC.h"

#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "chat/Roster.h"

#include "TelemetryAPIDLL.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This is defined in EbisuError.h in the SDK.
// We define it here to prevent sdk/client cross requirements
#define EBISU_SUCCESS 0
#define EBISU_ERROR_GENERIC -1

// detect concurrencies - assert if our mutex is already locked
#ifdef ORIGIN_PC

#ifdef _DEBUG_MUTEXES
#define DEBUG_MUTEX_TEST(x){\
bool s=x->tryLock();\
if(s)\
x->unlock();\
ORIGIN_ASSERT(s==true);\
}
#else
#define DEBUG_MUTEX_TEST(x) void(0)
#endif

#elif defined (ORIGIN_MAC)

#define DEBUG_MUTEX_TEST(x) void(0)

#endif // !MAC OSX


using namespace Origin;

namespace Origin
{
namespace Engine
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ORIGIN_PC

// Run the IGOProxy/IGOProxy64 exe multiple times to collect offsets from the different graphics API we support so 
// that IGO can directly access COM object
OIGOffsetsLookupWorker* OIGOffsetsLookupWorker::createWorker(QString appPath, QString exePath, QString apiOption)
{
    QThread* thread = new QThread;
    OIGOffsetsLookupWorker* worker = new OIGOffsetsLookupWorker(thread, appPath, exePath, apiOption);
    worker->moveToThread(thread);

    ORIGIN_VERIFY_CONNECT(thread, SIGNAL(started()), worker, SLOT(started()));
    ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished()), thread, SLOT(quit()));
    ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    ORIGIN_VERIFY_CONNECT(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    return worker;
}

void OIGOffsetsLookupWorker::start()
{
    mThread->start();
}

OIGOffsetsLookupWorker::OIGOffsetsLookupWorker(QThread* thread, QString appPath, QString exePath, QString apiOption)
    : mThread(thread), mAppPath(appPath), mExePath(exePath), mApiOption(apiOption), mDone(false)
{
}

OIGOffsetsLookupWorker::~OIGOffsetsLookupWorker()
{
}

void OIGOffsetsLookupWorker::started()
{
    ORIGIN_LOG_EVENT << "Running process " << mExePath << " -L " <<  mApiOption;

    QProcess* process = new QProcess(this);
    QStringList arguments;
    arguments += QStringLiteral("-L");
    arguments += mApiOption;
    ORIGIN_VERIFY_CONNECT(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onProcessFinished(int, QProcess::ExitStatus)));
    ORIGIN_VERIFY_CONNECT(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onProcessError(QProcess::ProcessError)));

    process->start(mExePath, arguments);
    int timeoutInMS = 4000;
    if (!process->waitForFinished(timeoutInMS))
    {
        if (!mDone) // if not error starting process
        {
            // Kill the process - something bad must have happened
            process->kill();
        }
    }
    delete process;

    emit finished();
}

void OIGOffsetsLookupWorker::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    ORIGIN_LOG_DEBUG << "Process " << mExePath << " -L " << mApiOption << " completed normally (" << exitCode << ")";
    mDone = true;
}

void OIGOffsetsLookupWorker::onProcessError(QProcess::ProcessError error)
{
    ORIGIN_LOG_ERROR << "Process " << mExePath << " -L " << mApiOption << " failure - " << error;
    mDone = true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static QString AuthCodeParamBegin("{auth_code[");
static QString AuthCodeParamEnd("]}");
static QString BrowserWindowMiniAppParam("oigminiapp");
static QString BrowserWindowSizeParam("oigwinsize");
static QString BrowserWindowMaxSizeValue("maxsize");
static QString BrowserWindowPosParam("oigwinpos");
static QString BrowserWindowCenteredPosParam("centered");
static QString BrowserWindowNoPinningParam("oignopinning");
static QString BrowserIDParam("oigwinid");

IGOURLUnwinderResponse::IGOURLUnwinderResponse(const QString& url)
{
    // Clean up URL to uncode special characters/ease our pain searching for tokens
    mUrl = QUrl(url).toString(QUrl::DecodeReserved);
}

bool IGOURLUnwinderResponse::resolve()
{
    // We may "fake" a delayed resolve if we need to remove some of the params!
    bool immediateChanges = false;

    // Do we need an auth code?
    int searchStartIdx = 0;
    int authCodeBeginIdx = mUrl.indexOf(AuthCodeParamBegin, searchStartIdx);

    while (authCodeBeginIdx >= 0)
    {
        bool validAuthCodeRequest = false;

        int clientIdBeginIdx = authCodeBeginIdx + AuthCodeParamBegin.size();
        int authCodeEndIdx = mUrl.indexOf(AuthCodeParamEnd, clientIdBeginIdx);
        if (authCodeEndIdx >= 0)
        {
            QString clientId = mUrl.mid(clientIdBeginIdx, authCodeEndIdx - clientIdBeginIdx);
            
            // Extend the authCodeEndIdx to capture the end of the token for when we're going to replace the token with the actual value
            authCodeEndIdx += AuthCodeParamEnd.size();

            // Is this a secure url?
            if (mUrl.startsWith("https:", Qt::CaseInsensitive))
            {
                QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
                Origin::Services::AuthPortalAuthorizationCodeResponse* resp = Origin::Services::AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, clientId);
                if (resp)
                {
                    ORIGIN_LOG_EVENT << "Requesting auth code for: " << clientId;

                    validAuthCodeRequest = true;
                    mPendingRequests.push_back(PendingRequest(resp, authCodeBeginIdx, authCodeEndIdx));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onAuthCodeRetrieved()));
                    resp->setDeleteOnFinish(true);

                    searchStartIdx = authCodeEndIdx;
                }
            }
        }

        else
            ORIGIN_LOG_ERROR << "URL cannot use auth code when not using secure website";

        // Should we simply remove the param?
        if (!validAuthCodeRequest)
        {
            ORIGIN_LOG_ERROR << "Unable to request auth code - removing parameter (" << authCodeBeginIdx << " / " << authCodeEndIdx << ") from URL";
            immediateChanges = true;
            int removeLen = authCodeEndIdx >= 0 ? authCodeEndIdx - authCodeBeginIdx : mUrl.length() - authCodeBeginIdx;
            mUrl.remove(authCodeBeginIdx, removeLen);

            searchStartIdx = authCodeBeginIdx;
        }

        authCodeBeginIdx = mUrl.indexOf(AuthCodeParamBegin, searchStartIdx);
    }

    // If we have "immediate changes", schedule a call to see whether we are done with the resolving
    if (immediateChanges)
        QMetaObject::invokeMethod(this, "checkResolveStatus", Qt::QueuedConnection);

    bool resolving = immediateChanges || mPendingRequests.size() > 0;
    return resolving;
}

void IGOURLUnwinderResponse::checkResolveStatus()
{
    bool completed = true;
    for (PendingRequests::const_iterator iter = mPendingRequests.begin(); iter != mPendingRequests.end(); ++iter)
        completed &= iter->completed;

    if (completed)
    {
        // Re-create the URL with the proper entries - start from the end to be able to use the start/end indexes we previously computed
        for (PendingRequests::const_reverse_iterator iter = mPendingRequests.rbegin(); iter != mPendingRequests.rend(); ++iter)
            mUrl.replace(iter->tokenStartIdx, iter->tokenEndIdx - iter->tokenStartIdx, iter->value);

        ORIGIN_LOG_EVENT << "Dynamic resolve completed: '" << mUrl << "'";

        // Let the work know about it
        emit resolved(mUrl);

        // Our work here is done
        deleteLater();
    }
}

void IGOURLUnwinderResponse::onAuthCodeRetrieved()
{
    Origin::Services::AuthPortalAuthorizationCodeResponse* response = dynamic_cast<Origin::Services::AuthPortalAuthorizationCodeResponse*>(sender());
    if (response)
    {
        // Do we have an entry for it?
        for (PendingRequests::iterator iter = mPendingRequests.begin(); iter != mPendingRequests.end(); ++iter)
        {
            if (iter->key == response)
            {
                iter->completed = true;

                // Ok, we have our entry to update - now did we actually get a code or not?
                bool success = ((response->error() == Origin::Services::restErrorSuccess) && (response->httpStatus() == Origin::Services::Http302Found));
                if (success)
                    iter->value = response->authorizationCode();

                else
                {
                    ORIGIN_LOG_ERROR << "IGO URL unwinder: auth code retrieval failed with: restError = " << response->error() << " httpStatus = " << response->httpStatus();
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("IGOURLUnwinderAuthCodeRetrievalFail", QString("%1").arg(response->httpStatus()).toUtf8().constData());
                }

                // Are we done resolving this URL?
                checkResolveStatus();
                break;
            }
        }
    }
}

QString IGOURLUnwinder::staticResolve(const QString& url, const QString& locale)
{
    if (url.isEmpty())
        return QString();

    ORIGIN_LOG_EVENT << "Static unwinding for: " << url;

    // Perform the basic replacements (see https://confluence.ea.com/display/EBI/OIG+Browser+default+URL+structure+update+PRD)
    static QString gameLocale("en_US");

    // Do we have a valid locale? ie in the form: en_US - which btw is our default
    if (locale.length() == 5 && locale[2] == '_')
        gameLocale = locale;

    else
        ORIGIN_LOG_ERROR << "Invalid game locale:" << locale << " - using default:" << gameLocale;

    QString gameCountry = gameLocale.mid(3, 2);
    QString gameLanguage = gameLocale.mid(0, 2);

    QString resolvedURL = url;
    resolvedURL.replace("{game_locale}", gameLocale, Qt::CaseInsensitive);
    resolvedURL.replace("{game_country}", gameCountry, Qt::CaseInsensitive);
    resolvedURL.replace("{game_language}", gameLanguage, Qt::CaseInsensitive);

    QUrl gameURL(resolvedURL);
    QUrlQuery query(gameURL);

    // If not safe scheme, don't allow the use of auth_code
    if (gameURL.scheme().compare("https", Qt::CaseInsensitive) != 0)
    {
        typedef QList<QPair<QString, QString>> QueryItems;
        QueryItems items = query.queryItems();
        for (QueryItems::const_iterator iter = items.begin(); iter != items.end(); ++iter)
        {
            if (iter->second.contains(AuthCodeParamBegin, Qt::CaseInsensitive))
            {
                ORIGIN_LOG_ERROR << "URL cannot use auth code when not using secure website - removing parameter '" << iter->first << "'";
                query.removeQueryItem(iter->first);
            }
        }
    }

    // Add our old default params
    if (!query.hasQueryItem("gameLocale"))
        query.addQueryItem("gameLocale", locale);

    if (!query.hasQueryItem("originLocale"))
        query.addQueryItem("originLocale", Origin::Services::readSetting(Origin::Services::SETTING_LOCALE));

    gameURL.setQuery(query);
    resolvedURL = gameURL.toString(QUrl::None);

    ORIGIN_LOG_EVENT << "Resolved URL: " << resolvedURL;

    return resolvedURL;
}

IGOURLUnwinderResponse* IGOURLUnwinder::dynamicResolve(const QString& url)
{
    ORIGIN_LOG_EVENT << "Dynamic resolve for '" << url << "'";

    IGOURLUnwinderResponse* response = new IGOURLUnwinderResponse(url);
    if (response->resolve())
        return response;

    delete response;
    return NULL;
}

QString IGOURLUnwinder::requestedBrowserProperties(const QString& url, BrowserProperties* props)
{
    if (!props || url.isEmpty())
        return url;

    QUrl gameURL(url);
    QUrlQuery query(gameURL);

    bool hasProps = false;

    // Remove mini-app identifier
    query.removeAllQueryItems(BrowserWindowMiniAppParam);

    // Search for window dimensions
    QStringList winDims = query.allQueryItemValues(BrowserWindowSizeParam);
    if (!winDims.empty())
    {
        QString winDim = winDims.at(0);

        // Is it the special case of max size?
        if (winDim.compare(BrowserWindowMaxSizeValue, Qt::CaseInsensitive) == 0)
        {
            props->size = IIGOWindowManager::WindowProperties::maximumSize();
            props->noRestore = true;
            hasProps = true;
        }

        else
        {
            QStringList dims = winDim.split('x', QString::SplitBehavior::SkipEmptyParts, Qt::CaseInsensitive);
            if (dims.size() == 2)
            {
                int width = dims[0].toInt();
                int height = dims[1].toInt();

                props->size = QSize(width, height);
                hasProps = true;
            }
        }

        query.removeAllQueryItems(BrowserWindowSizeParam);
    }

    // Search for window positioning
    QStringList winMPos = query.allQueryItemValues(BrowserWindowPosParam);
    if (!winMPos.empty())
    {
        QString winPos = winMPos.at(0);

        // Is it the special case of max size?
        if (winPos.compare(BrowserWindowCenteredPosParam, Qt::CaseInsensitive) == 0)
        {
            props->centered = true;
            props->noRestore = true;
            hasProps = true;
        }

        query.removeAllQueryItems(BrowserWindowPosParam);
    }

    // Search for no pinning option
    QStringList noPinning = query.allQueryItemValues(BrowserWindowNoPinningParam);
    if (!noPinning.empty())
    {
        props->noPinning = true;
        hasProps = true;

        query.removeAllQueryItems(BrowserWindowNoPinningParam);
    }

    // Search for unique ID
    QStringList IDs = query.allQueryItemValues(BrowserIDParam);
    if (!IDs.empty())
    {
        props->id = IDs.at(0);
        hasProps = true;

        query.removeAllQueryItems(BrowserIDParam);
    }

    if (hasProps)
    {
        ORIGIN_LOG_EVENT << "Extracted browser params from URL: id=" << props->id << ", Size=" << props->size.width() << " x " << props->size.height()
                         << ", NoPinning=" << props->noPinning << ", NoRestore=" << props->noRestore << ", centered=" << props->centered;

        gameURL.setQuery(query);
        return gameURL.toString(QUrl::None);
    }

    else
        return url;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Helper class to manage/recycle the set of visible browsers
WebBrowserManager::WebBrowserManager(IGOController* controller, IIGOUIFactory* factory, IIGOWindowManager* windowManager)
    : mController(controller)
    , mUIFactory(factory)
    , mWindowManager(windowManager)
    , mCurrentWebBrowser(NULL)
{
}
    
WebBrowserManager::~WebBrowserManager()
{
}

IWebBrowserViewController* WebBrowserManager::createWebBrowser(const QString& url)
{
    return createWebBrowser(url, IWebBrowserViewController::IGO_BROWSER_SHOW_NAV, IIGOWindowManager::WindowProperties(), IIGOWindowManager::WindowBehavior());
}
        
IWebBrowserViewController* WebBrowserManager::createWebBrowser(const QString& initialUrl, IWebBrowserViewController::BrowserFlags flags, const IIGOWindowManager::WindowProperties& settings, const IIGOWindowManager::WindowBehavior& baseBehavior)
{
    // Is this coming from a js helper (like LaunchExternalBrowser) / is the URL to be open in miniapp mode?
    QUrlQuery query = QUrlQuery(QUrl(initialUrl));
    if (query.hasQueryItem(BrowserWindowMiniAppParam))
    {
        flags &= ~(IWebBrowserViewController::IGO_BROWSER_SHOW_NAV | IWebBrowserViewController::IGO_BROWSER_ADD_TAB);
        flags |= Engine::IWebBrowserViewController::IGO_BROWSER_MINI_APP;
    }

    // Extract any OIG-specific URL parameters used to personalize the browser
    IGOURLUnwinder::BrowserProperties requestedBrowserProps;
    QString url = initialUrl;

    if ((flags & IWebBrowserViewController::IGO_BROWSER_MINI_APP) != 0)
    {
        url = mController->urlUnwinder()->requestedBrowserProperties(initialUrl, &requestedBrowserProps);

        if (!requestedBrowserProps.id.isEmpty())
        {
            // The browser has an ID -> was it created previously? (ie miniapp = single tab)
            for (WebBrowserList::iterator it = mWebBrowserList.begin(); it != mWebBrowserList.end(); ++it)
            {
                IWebBrowserViewController* controller = *it;
                if (controller->id() == requestedBrowserProps.id)
                {
                    // Load the new URL (this should only be used for single tab browsers)
                    controller->setTabInitialUrl(0, url);

                    // We already have that guy! -> don't create a new instance, simply make sure to push this one to the front
                    mWindowManager->setFocusWindow(controller->ivcView());

                    return controller;
                }
            }
        }
    }
        
    // Don't allow tabs for connection-specific browsers since we want to be able to show/hide them depending on the game focus
    bool addTab = (flags & IWebBrowserViewController::IGO_BROWSER_ADD_TAB) != 0 && !baseBehavior.connectionSpecific;
    
    // Do we need to create a new browser or simply add a new tab to the current one?
    if (!addTab || !mCurrentWebBrowser || !mCurrentWebBrowser->isAddTabAllowed())
    {
        mCurrentWebBrowser = mUIFactory->createWebBrowser(url, flags);
        mCurrentWebBrowser->setID(requestedBrowserProps.id);

        // Let the world know we are opening a new browser
        mController->openingWebBrowser();
        
        IIGOWindowManager::WindowProperties properties;
        properties.setOpenAnimProps(mController->defaultOpenAnimation());
        properties.setCloseAnimProps(mController->defaultCloseAnimation());
        properties.setWId(IViewController::WI_BROWSER + mWebBrowserList.size());
        properties.setUniqueID(requestedBrowserProps.id);
        properties.setCallOrigin(settings.callOrigin());

        if (!requestedBrowserProps.size.isEmpty() && requestedBrowserProps.size != IIGOWindowManager::WindowProperties::maximumSize())
        {
            // Don't make it better than the current resolution though!
            QSize windowSize = requestedBrowserProps.size.boundedTo(mWindowManager->maximumWindowSize());
            properties.setSize(windowSize);
        }

        else
        {
// Disable the previous limitation for Mac to accomodate the Sims4 store
#if 0 // def ORIGIN_MAC
            // Because of the browser poor performances in high resolution (think SIMCITY), we limit the initial size of the browser window
            static const int DEFAULT_BROWSER_WIDTH = 1024;
            static const int DEFAULT_BROWSER_HEIGHT = 768;
            properties.setSize(QSize(DEFAULT_BROWSER_WIDTH, DEFAULT_BROWSER_HEIGHT));
#else
            properties.setMaximumSize();
#endif
        }
        
        // Check whether we need to override the regular browser positioning algorithm
        if (requestedBrowserProps.centered)
            properties.setCenteredPosition();

        // We only allow the oldest browser to be pinnable -> look up for any opened browser THAT IS NOT a miniApp!
        bool browserFound = false;
        for (size_t idx = 0; idx < mWebBrowserList.size(); ++idx)
        {
            browserFound = (mWebBrowserList[idx]->creationFlags() & IWebBrowserViewController::IGO_BROWSER_MINI_APP) == 0;
            if (browserFound)
                break;
        }

        if (!requestedBrowserProps.noRestore)
        {
            if ((!browserFound && (flags & IWebBrowserViewController::IGO_BROWSER_MINI_APP) == 0)
                || !properties.uniqueID().isEmpty())
                properties.setRestorable(true);
        }

        IIGOWindowManager::WindowBehavior behavior = baseBehavior;
        behavior.draggingSize = 40;

        // don't allow resizing (or restore) when the user explicitly set the size to max.
        if (requestedBrowserProps.size == IIGOWindowManager::WindowProperties::maximumSize())
        {
            behavior.disableResizing = true;
            behavior.nsBorderSize = 0;
            behavior.ewBorderSize = 0;
            behavior.nsMarginSize = 0;
            behavior.ewMarginSize = 0;
        }

        else
        {
#ifdef ORIGIN_MAC
            behavior.nsBorderSize = 10;
            behavior.ewBorderSize = 10;
            behavior.nsMarginSize = 6;
            behavior.ewMarginSize = 6;
#else
            behavior.nsBorderSize = 10;
            behavior.ewBorderSize = 8;
#endif  
        }

        behavior.doNotDeleteOnClose = true;
        behavior.pinnable = !requestedBrowserProps.noPinning;
        behavior.supportsContextMenu = true;
        behavior.windowListener = this;
        behavior.controller = mCurrentWebBrowser;
        if ((flags & IWebBrowserViewController::IGO_BROWSER_MINI_APP) != 0)
        {
            behavior.closeIGOOnClose = true;
            behavior.connectionSpecific = true;
            behavior.closeOnIGOClose = requestedBrowserProps.noPinning;
        }

        mWindowManager->addWindow(mCurrentWebBrowser->ivcView(), properties, behavior);
        
        // keep track of open browsers
        mWebBrowserList.push_back(mCurrentWebBrowser);

		// Use an event filter here instead of connecting to the destroyed() signal because we want access to the object
		// BEFORE it's destroyed!
        QObject* obj = dynamic_cast<QObject*>(mCurrentWebBrowser);
        if (obj)
        {
            obj->installEventFilter(this);
            ORIGIN_VERIFY_CONNECT(mController, SIGNAL(igoStop()), obj, SLOT(deleteLater()));
        }
    }

    // Make sure to have at least one tab before trying to show the navigation bar! BUT...
    // Do we need to resolve the url? (ie auth_code)
    bool blankPage = false;
    if ((flags & Origin::Engine::IWebBrowserViewController::IGO_BROWSER_RESOLVE_URL) != 0)
    {
        IGOURLUnwinderResponse* resp = mController->urlUnwinder()->dynamicResolve(url);
        if (resp)
        {
            // Show a blank page while we're waiting on the URL expansion
            blankPage = true;
            mCurrentWebBrowser->addTab("", blankPage);

            PendingURL pendingURL;
            pendingURL.key = resp;
            pendingURL.browser = mCurrentWebBrowser;
            pendingURL.tabIdx = mCurrentWebBrowser->tabCount() - 1;
            mPendingURLs.push_back(PendingURL(pendingURL));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(resolved(QString)), this, SLOT(onURLResolved(QString)));
        }

        else
            mCurrentWebBrowser->addTab(url, blankPage);
    }

    else
        mCurrentWebBrowser->addTab(url, blankPage);

    bool showNav = (flags & Origin::Engine::IWebBrowserViewController::IGO_BROWSER_SHOW_NAV) != 0;
    mCurrentWebBrowser->showNavigation(showNav);
    
    mWindowManager->setFocusWindow(mCurrentWebBrowser->ivcView());

    // Telemetry for when the user opens a mini app
    if (flags & IWebBrowserViewController::IGO_BROWSER_MINI_APP)
    {
        IGOGameTracker::GameInfo info = mController->gameTracker()->currentGameInfo();
        GetTelemetryInterface()->Metric_IGO_BROWSER_MAPP(info.mBase.mProductID.toUtf8().data());
    }

    return mCurrentWebBrowser;
}
    
bool WebBrowserManager::eventFilter(QObject* obj, QEvent* evt)
{
	if (obj != this)
	{
		QEvent::Type type = evt->type();
		switch (type)
		{
			case QEvent::Destroy:
			case QEvent::DeferredDelete:
				onWebBrowserDestroyed(obj);
				break;

			default:
				break;
		}
	}

	return QObject::eventFilter(obj, evt);
}

void WebBrowserManager::onWebBrowserDestroyed(QObject* object)
{
    IWebBrowserViewController* wbController = qobject_cast<IWebBrowserViewController*>(object);
	if (wbController)
	{
		// Remove from list
		int iBrowser = 0;
		for (WebBrowserList::iterator it = mWebBrowserList.begin(); it != mWebBrowserList.end(); ++it)
		{
			if (wbController == *it)
			{
                // Let the world know we are closing a browser instance
                mController->closingWebBrowser();

                mWebBrowserList.erase(it);
                break;
			}
			iBrowser++;
		}

        // Don't change the window ordering if we are closing all the browsers at once
        if (mController->isActive())
        {
            bool newFirstBrowserFound = false;
            for (const int numBrowsers = mWebBrowserList.size(); iBrowser < numBrowsers; iBrowser++)
            {
                IIGOWindowManager::WindowProperties settings;

                settings.setWId(IViewController::WI_BROWSER + iBrowser);
                
                // We only allow the oldest browser to be pinnable
                bool isMiniApp = (mWebBrowserList[iBrowser]->creationFlags() & IWebBrowserViewController::IGO_BROWSER_MINI_APP) != 0;
                if (!newFirstBrowserFound && !isMiniApp)
                {
                    newFirstBrowserFound = true;
                    settings.setRestorable(true);
                }

                mWindowManager->setWindowProperties(mWebBrowserList[iBrowser]->ivcView(), settings);
            }
        }
            
        // see if we need to change the last in-focus web browser
        if (wbController == mCurrentWebBrowser)
        {
            if( mWebBrowserList.empty() )
                mCurrentWebBrowser = NULL;
            else
                mCurrentWebBrowser = mWebBrowserList.back();
        }
    }
}

void WebBrowserManager::onFocusChanged(QWidget* window, bool hasFocus)
{
    if (hasFocus)
        mCurrentWebBrowser = static_cast<IWebBrowserViewController*>(mWindowManager->windowController(window));
}

void WebBrowserManager::onURLResolved(QString url)
{
    IGOURLUnwinderResponse* resp = qobject_cast<IGOURLUnwinderResponse*>(sender());
    if (resp)
    {
        for (PendingURLs::iterator urlIter = mPendingURLs.begin(); urlIter != mPendingURLs.end(); ++urlIter)
        {
            if (urlIter->key == resp)
            {
                // Do we still have that browser around?
                WebBrowserList::iterator wbIter = mWebBrowserList.begin();
                for (; wbIter != mWebBrowserList.end(); ++wbIter)
                {
                    if (urlIter->browser == *wbIter)
                    {
                        // Do we still have that tab around?
                        if (urlIter->tabIdx < urlIter->browser->tabCount())
                        {
                            // Is it still on a blank page?
                            if (urlIter->browser->isTabBlank(urlIter->tabIdx))
                                urlIter->browser->setTabInitialUrl(urlIter->tabIdx, url);
                        }

                        break;
                    }
                }

                break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Helper class to use a single window to show different content from the client space (for example Search/Profile views)
SharedViewController::SharedViewController(QObject* parent)
    : QObject(parent), mWindow(NULL)
{
}
    
SharedViewController::~SharedViewController()
{
    // Make sure to detach from current content, which should be deleted from the real controller owner
    if (mWindow)
    {
        mWindow->takeContent();
        mWindow->deleteLater();
    }
    
    for (ManagedControllers::iterator iter = mControllers.begin(); iter != mControllers.end(); ++iter)
    {
        QObject* obj = dynamic_cast<QObject*>(iter->second);
        if (obj)
            obj->deleteLater();
    }
}

void SharedViewController::setWindow(Origin::UIToolkit::OriginWindow* window)
{
    if (!window)
        return;
    
    if (!mWindow)
    {
        mWindow = window;
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
    }
    
    else
        ORIGIN_LOG_DEBUG << "SharedContentViewController already holds a window to show the shared content";
}
    
void SharedViewController::add(int cid, IViewController* controller)
{
    if (!controller)
        return;
    
    for (ManagedControllers::iterator iter = mControllers.begin(); iter != mControllers.end(); ++iter)
    {
        if (iter->first == cid)
        {
            if (iter->second == controller)
                return;
            
            ORIGIN_LOG_DEBUG << "SharedContentViewController already owns a controller of type=" << cid;
            
            // We're still going to add it to the list to keep track of it/delete it properly in time,
            // but it will not be showing up anytime soon!
        }
    }
    
    mControllers.push_back(eastl::make_pair(cid, controller));
}

IViewController* SharedViewController::get(int cid)
{
    for (ManagedControllers::iterator iter = mControllers.begin(); iter != mControllers.end(); ++iter)
    {
        if (iter->first == cid)
            return iter->second;
    }
    
    return NULL;
}

void SharedViewController::show(int cid)
{
    if (mWindow)
    {
        // We always take away the content so that we can actually have an empty window (useful when cleaning up)
        mWindow->takeContent();

        // Switch the actual content of the window WITHOUT deleting it
        for (ManagedControllers::iterator iter = mControllers.begin(); iter != mControllers.end(); ++iter)
        {
            if (iter->first == cid)
            {
                if (mWindow->content() != iter->second->ivcView())
                {
                    mWindow->setContent(iter->second->ivcView());
                }
                
                break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Handles the initial notification that IGO is available (user should have focus on the game + the resolution must follow the
// proper restriction)
IGOAvailableNotifier::IGOAvailableNotifier(IGOWindowManager* igowm, IGOGameTracker* tracker, uint32_t gameId)
    : mWindowManager(igowm), mTracker(tracker), mGameId(gameId)
{
    // Go away when we quit the game (in the event we never showed the notification!)
    ORIGIN_VERIFY_CONNECT(tracker, SIGNAL(gameRemoved(uint32_t)), this, SLOT(onGameRemoved(uint32_t)));
                          
    // Setup to show it whenever the size is appropriate (ie the user is interacting with the window (hopefully!) so he also has focus)
    ORIGIN_VERIFY_CONNECT(igowm, SIGNAL(screenSizeChanged(uint32_t, uint32_t, uint32_t)), this, SLOT(onScreenSizeChanged(uint32_t, uint32_t, uint32_t)));

    // Try immediately though, in case the size was good to start with (assume this was created on the gameFocused event)
    onScreenSizeChanged(0, 0, gameId);
}

IGOAvailableNotifier::~IGOAvailableNotifier()
{
    disconnect();
    mWindowManager = NULL;
    mTracker = NULL;
    mGameId = -1;
}

void IGOAvailableNotifier::onGameRemoved(uint32_t gameId)
{
    if (gameId == mGameId)
        deleteLater();
}
    
void IGOAvailableNotifier::onScreenSizeChanged(uint32_t width, uint32_t height, uint32_t gameId)
{
    if (gameId == mGameId)
    {
        if (mWindowManager->isScreenLargeEnough() && mTracker)
        {
            IGOGameTracker::GameInfo info = mTracker->currentGameInfo();
            if (info.mBase.mTimeRemaining > 0)
            {
                emit IGOController::instance()->igoShowTrialWelcomeToast(info.mBase.mTitle, info.mBase.mTimeRemaining);
            }
            else
            {
                emit IGOController::instance()->igoShowOpenOIGToast();
            }
            deleteLater();
        }
    }
}
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CleanLogFolder()
{
    // Limit the number of log files we keep around
    const int MaxLogFileCount = 256;
    
    QString path = Origin::Services::PlatformService::logDirectory();
    QDir logDir(path, QStringLiteral("IGO*.txt"), QDir::Time, QDir::Files | QDir::NoSymLinks);

    QFileInfoList files = logDir.entryInfoList();
    
    int surplus = (MaxLogFileCount * 10) / 100;
    int cnt = files.size();
    if (cnt > (MaxLogFileCount + surplus))
    {
        ORIGIN_LOG_EVENT << "Cleaning up IGO logs (" << cnt << " / " << MaxLogFileCount << ")";
        
        for (int idx = MaxLogFileCount; idx < cnt; ++idx)
        {
            QFileInfo file = files.at(idx);
            logDir.remove(file.fileName());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static IGOController* sInstance = NULL;
static QMutex sInstanceLock(QMutex::Recursive);
static IIGOUIFactory* sIGOUIFactory = NULL;

void IGOController::init(IIGOUIFactory* factory)
{
    sIGOUIFactory = factory;
}

void IGOController::release()
{
    QMutexLocker lock(&sInstanceLock);
    
    delete sInstance;
    sInstance = NULL;
}

bool IGOController::instantiated()
{
    bool instantiated = true;
    
    if (!sInstance)
    {
        QMutexLocker lock(&sInstanceLock);
        if (!sInstance)
        {
            instantiated = false;
        }
    }
    
    return instantiated;
}
    
IGOController* IGOController::instance()
{
    if (!sInstance)
    {
        QMutexLocker lock(&sInstanceLock);
        if (!sInstance)
        {
            static bool once = false;

            if (once)   // rare case of getting into an infinit instance() loop
            {
                ORIGIN_ASSERT_MESSAGE(false, "Please make sure to instantiate IGOController not recursivly!");
                return sInstance;
            }
            once = true;

#ifdef ORIGIN_MAC
            // Make sure we are setup to use IGO
            static bool helperToolChecked = false;
            if (!helperToolChecked)
            {
                helperToolChecked = true;
                
                // While at it, setup our hooks (to disable key equivalents (ie CMD+Q) for OIG, disable flash plugin fullscreen, etc...)
                SetupOIGHooks();
            }
#else
            // Setup hooks
            HookWin32();
#endif
            sInstance = new IGOController(sIGOUIFactory);
            
            // manually fire the logged in signal, because IGO connected after as user logged in already if the first game was just started!
            if (LoginController::instance()->isUserLoggedIn())
                sInstance->onUserLoggedIn(LoginController::instance()->currentUser());

            once = false;
        }
    }
    
    return sInstance;
}

IGOController::IGOController(IIGOUIFactory* factory)
    : mUIFactory(factory)
    , mGameTracker(NULL)
    , mWindowManager(NULL)
    , mIPCServer(NULL)
#ifdef ORIGIN_PC
    , mTwitch(NULL)
#endif
    , mWebBrowserManager(NULL)
    , mURLUnwinder(NULL)
    , mExceptionSystem(NULL)
    , mClock(NULL)
    , mTitle(NULL)
    , mLogo(NULL)
    , mNavigation(NULL)
    , mGlobalProgress(NULL)
    , mKoreanController(NULL)
#ifdef ORIGIN_PC
    , mTwitchViewController(NULL)
    , mTwitchBroadcastIndicator(NULL)
#endif
    , mDesktopSetupCompleted(false)
    , mUserIsLoggedIn(false)
    , mExtraLogging(false)
    , mShowWebInspectors(false)
#ifdef ORIGIN_PC
    , mOffsetsLookupCount(0)
    , mMaxOffsetsLookupCount(0)
#endif
{

    // main thread because of UI creation?
    ORIGIN_ASSERT_MESSAGE(QThread::currentThread() == QCoreApplication::instance()->thread(), "Please make sure to instanciate IGOController from the main thread!");

    // Make sure to limit the number of log files
    CleanLogFolder();

#ifdef ORIGIN_MAC
    // Initialize the memory region shared with IGO if not already done (for example if somebody called one of the
    // methods in IGOWindowManager that access that data!)
    checkSharedMemorySetup();
#endif
    mWindowManager = new IGOWindowManager();
    
    mIPCServer = new IGOWindowManagerIPC(WindowManager());
    WindowManager()->setIPCServer(mIPCServer);
#ifdef ORIGIN_MAC
    mTelemetryServer = new IGOTelemetryIPC();
#endif
    mGameTracker = new IGOGameTracker(mIPCServer);
    
#ifdef ORIGIN_PC
    // broadcasting
    mTwitch = new IGOTwitch();
    mTwitch->setGameTracker(mGameTracker);
    mTwitch->setIPCServer(mIPCServer);
    mIPCServer->setTwitch(mTwitch);
#endif
    
    mURLUnwinder = new IGOURLUnwinder();

    // Enable IGO info display window depending on EACore.ini EnableIGODebugMenu flag
    setIGOEnableLogging(false);
    setIGODebugMenu(Origin::Services::readSetting(Origin::Services::SETTING_EnableIGODebugMenu));

    // Since we always log the injection/cleanup process, remove "old" log files not to clutter the log folder too much
    removeOldLogs();
    
#ifdef ORIGIN_PC
    // Enable IGO stress test mode for the hooking API
    setIGOStressTest(Origin::Services::readSetting(Origin::Services::SETTING_EnableIGOStressTest));
#endif

    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoCommand(int, Origin::Engine::IIGOCommandController::CallOrigin)), WindowManager(), SLOT(igoCommand(int, Origin::Engine::IIGOCommandController::CallOrigin))); // from IGOFlow
    ORIGIN_VERIFY_CONNECT(WindowManager(), SIGNAL(igoStateChange(bool, bool)), this, SLOT(igoStateChanged(bool, bool)));
    ORIGIN_VERIFY_CONNECT(WindowManager(), SIGNAL(igoStateChange(bool, bool)), Origin::SDK::SDK_ServiceArea::instance(), SLOT(IgoStateChanged(bool, bool)));
    ORIGIN_VERIFY_CONNECT(WindowManager(), SIGNAL(igoStop()), this, SIGNAL(igoStop()));
    ORIGIN_VERIFY_CONNECT(WindowManager(), SIGNAL(igoUserIgoOpenAttemptIgnored()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(IgoUserOpenAttemptIgnored()));
    
    ORIGIN_VERIFY_CONNECT(WindowManager(), SIGNAL(requestGoOnline()), this, SIGNAL(requestGoOnline()));

    // Notify when user logs in/out
    ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(onUserLoggedIn(Origin::Engine::UserRef)));
    ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onUserLoggedOut(Origin::Engine::UserRef)));

    // Game changes notifications
    ORIGIN_VERIFY_CONNECT(mGameTracker, SIGNAL(gameAdded(uint32_t)), this, SLOT(onGameAdded(uint32_t)));
    ORIGIN_VERIFY_CONNECT(mGameTracker, SIGNAL(gameFocused(uint32_t)), this, SLOT(onGameFocused(uint32_t)));
    ORIGIN_VERIFY_CONNECT(mGameTracker, SIGNAL(gameRemoved(uint32_t)), this, SLOT(onGameRemoved(uint32_t)));
    
    // Internal connections
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(showBrowser(const QString&, bool)), this, SLOT(igoShowBrowser(const QString&, bool)));
    
    // broadcasting
#ifdef ORIGIN_PC
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastStarted(const QString&)), this, SIGNAL(broadcastStarted(const QString&)));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastStopped()), this, SIGNAL(broadcastStopped()));

    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastAccountDisconnected()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastAccountDisconnected()));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastStarted(const QString&)), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastStarted()));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastStopped()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastStopped()));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastPermitted(Origin::Engine::IIGOCommandController::CallOrigin)), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastBlocked()));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastStartPending()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastStartPending()));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastStartStopError()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastError()));
    ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastErrorOccurred_forSDK()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastError()));    

	// Run our separate thread to collect graphics API offsdets for IGO
    dispatchOffsetsLookup();
#endif

}

IGOController::~IGOController()
{
    delete mKoreanController;
    delete mURLUnwinder;
    delete mWebBrowserManager;
#ifdef ORIGIN_PC
    delete mTwitchBroadcastIndicator;
    delete mTwitchViewController;
    delete mTwitch;
    for(SDKSignalMap::const_iterator iter = mSDKSignals.begin(); iter != mSDKSignals.end(); ++iter)
        CloseHandle(iter->second);
    mSDKSignals.clear();
#endif
    dynamic_cast<QObject*>(mWindowManager)->deleteLater();
    mGameTracker->deleteLater();
#ifdef ORIGIN_MAC
    delete mTelemetryServer;
#endif
#ifdef ORIGIN_PC
    UnhookWin32();
    UnloadIGODll();
#endif
    
#ifdef ORIGIN_MAC
    // Clean up memory region shared with IGO
    FreeSharedMemory(false);
#endif
}

void IGOController::startProcessWithIGO(Origin::Services::IProcess *process, const QString& gameProductId
                                        , const QString& gameTitle, const QString& gameMasterTitle, const QString& gameFolder
                                        , const QString& gameLocale, const QString& executePath, bool forceKillAtOwnershipExpiry
                                        , bool isUnreleased, const qint64 timeRemaining, QString achievementSetID, const QString& IGOBrowserDefaultURL)
{
#ifdef ORIGIN_PC
	// Make sure we are done with our graphics API offset lookups
	int offsetsLookupCount = mOffsetsLookupCount;
	if (offsetsLookupCount != mMaxOffsetsLookupCount)
	{
		DWORD timeoutInMS = 4000;
		Sleep(timeoutInMS);
		offsetsLookupCount = mOffsetsLookupCount;
		if (!offsetsLookupCount)
			ORIGIN_LOG_ERROR << "Starting game before we are done collecting offsets";
	}
#endif

    // Verify current settings
    bool enableLogging = Services::readSetting(Services::SETTING_ENABLEINGAMELOGGING, Services::Session::SessionService::currentSession());
    setIGOEnableLogging(enableLogging);
                        
    // Register the core info of the new game
    IGOGameTracker::BaseGameLaunchInfo gameInfo;
    
    gameInfo.mProductID = gameProductId;
    gameInfo.mInstallDir = gameFolder;
    gameInfo.mExecutablePath = executePath;

#if defined(ORIGIN_PC)
    Origin::Services::ProcessList::getExeNameFromFullPath(gameInfo.mExecutable, gameInfo.mExecutablePath);
#else
    // no used actually on MAC
    gameInfo.mExecutable = gameInfo.mExecutablePath;
#endif

#ifdef ORIGIN_MAC
    gameInfo.mBundleId = Origin::Services::PlatformService::getBundleIdentifier(gameFolder);
#endif
    gameInfo.mTitle = gameTitle;
    gameInfo.mMasterTitle = gameMasterTitle;
    gameInfo.mAchievementSetID = achievementSetID;
    gameInfo.mLocale = gameLocale;
    gameInfo.mForceKillAtOwnershipExpiry = forceKillAtOwnershipExpiry;
    gameInfo.mIsUnreleased = isUnreleased;
    gameInfo.mTimeRemaining = timeRemaining;

    QString defaultURL = urlUnwinder()->staticResolve(IGOBrowserDefaultURL, gameLocale);
    mGameTracker->addDefinition(gameInfo, defaultURL);
    
#if defined(ORIGIN_PC)
#ifdef DEBUG
    const QString igoName = "igo32d.dll";
#else
    const QString igoName = "igo32.dll";
#endif
    HMODULE hIGO = GetModuleHandle(igoName.utf16());
    if (!hIGO)
        hIGO = LoadLibrary(igoName.utf16());

    if(!isIGODLLValid(igoName) && forceKillAtOwnershipExpiry)
    {
        ORIGIN_LOG_ERROR << "Cannot start free trial due to invalid IGO";
        return;
    }
#pragma message("TODO IGOController: add missing file error handling/logging")

    //if (!hIGO)
    //    CoreErrorManager::GetSingleton()->AddError( QString(), tr("ebisu_error_client_install_corrupt").arg("Origin") );
    
    ORIGIN_VERIFY_CONNECT(process, SIGNAL(finished(uint32_t, int)), WindowManager(), SLOT(onGameProcessFinished(uint32_t, int)));

    Origin::Services::ProcessWin* processWin = static_cast<Origin::Services::ProcessWin*>(process);
    if (processWin)
    {
        if(processWin->isElevated())
        {
            bool succesfullInjectedIGO = true;

            QString escalationReasonStr = "startProcessWithIGO (" + executePath + ")";
            int escalationError = 0;
            QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            if (Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
            {
                // split the process start into two pieces
                processWin->startProcess();
                //do IGO injection as early as possible, this is esp. required for .Net launchers like Sims 3!!!!
                //doing it after "continueForIGO" is too late!!!
                qint32 result = escalationClient->injectIGOIntoProcess(process->pid()->dwProcessId, process->pid()->dwThreadId);
                if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "injectIGOIntoProcess", result, escalationClient->getSystemError()))
                {
                    // error
                    ORIGIN_LOG_ERROR << "Injecting IGO into an elevated process failed - process access: " << result;

                    succesfullInjectedIGO = false;

                    //  TELEMETRY:  Detect IGO injection failure
                    GetTelemetryInterface()->Metric_IGO_INJECTION_FAIL(gameProductId.toLocal8Bit().constData(), processWin->isElevated(), forceKillAtOwnershipExpiry);
                }

                // destroy the process if IGO injection failed and we run a free trial
                if (forceKillAtOwnershipExpiry&& !succesfullInjectedIGO)
                    escalationClient->terminateProcess(process->pid()->dwProcessId);

                processWin->setupProcessMonitoring();   // continue the process start and set the process monitoring code...
            }
        }
        else
        {
            bool succesfullInjectedIGO = false;

            // split the process start into two pieces
            processWin->startProcess();
            
            // in some error cases, we elevate the execution, if that happend, IGO injection needs to be changed to use the elevated path as well!
            if (processWin->isElevated())
            {
                bool succesfullInjectedIGO = true;

                QString escalationReasonStr = "startProcessWithIGO (" + executePath + ")";
                int escalationError = 0;
                QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                if (Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                {
                    qint32 result = escalationClient->injectIGOIntoProcess(process->pid()->dwProcessId, process->pid()->dwThreadId);
                    if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "injectIGOIntoProcess", result, escalationClient->getSystemError()))
                    {
                        // error
                        ORIGIN_LOG_ERROR << "Injecting IGO into an elevated process failed - process access: " << result;

                        succesfullInjectedIGO = false;

                        //  TELEMETRY:  Detect IGO injection failure
                        GetTelemetryInterface()->Metric_IGO_INJECTION_FAIL(gameProductId.toLocal8Bit().constData(), processWin->isElevated(), forceKillAtOwnershipExpiry);
                    }

                    // destroy the process if IGO injection failed and we run a free trial
                    if (forceKillAtOwnershipExpiry&& !succesfullInjectedIGO)
                        escalationClient->terminateProcess(process->pid()->dwProcessId);

                    processWin->setupProcessMonitoring();   // continue the process start and set the process monitoring code...
                }
            }
            else
            {
                typedef bool(*InjectIGOFunc)(HANDLE, HANDLE);
                InjectIGOFunc InjectIGODll = hIGO != NULL ? (InjectIGOFunc)GetProcAddress(hIGO, "InjectIGODll") : NULL;

                if (InjectIGODll != NULL)
                {
                    succesfullInjectedIGO = InjectIGODll(process->pid()->hProcess, process->pid()->hThread);
                }
                else
                {
                    ORIGIN_LOG_ERROR << "IGO binary mismatch";
                }

                //  TELEMETRY:  Detect IGO injection failure
                if (succesfullInjectedIGO == false)
                {
                    ORIGIN_LOG_ERROR << "Injecting IGO into an process failed";
                    GetTelemetryInterface()->Metric_IGO_INJECTION_FAIL(gameProductId.toLocal8Bit().constData(), processWin->isElevated(), forceKillAtOwnershipExpiry);
                }

                // destroy the process if IGO injection failed and we run a free trial
                if (forceKillAtOwnershipExpiry && !succesfullInjectedIGO)
                {
                    if (!TerminateProcess(process->pid()->hProcess, 0))
                    {
                        // fallback, user with UAC will be able to cancel this, so we might just always use the
                        // OriginClientService to inject IGO with the little anoyance of having a gamer to confirm it via UAC
                        // for the first game launch
                        QString escalationReasonStr = "TerminateProcess";
                        int escalationError = 0;
                        QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                        if (Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                        {
                            escalationError = escalationClient->terminateProcess(process->pid()->dwProcessId);
                            escalationReasonStr = QString("TerminateProcess pid=%1").arg(process->pid()->dwProcessId);
                            Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "terminateProcess", escalationError, escalationClient->getSystemError());
                        }
                    }
                }

                processWin->setupProcessMonitoring();   // continue the process start and set the process monitoring code...
            }
        }
    }
#elif defined(ORIGIN_MAC)
    
    // Use dyld to inject OIG (supports 32-bit/64-bit)
    static const QString OIGLoaderFileName = "IGOLoader.dylib";
    QString oigLoaderPath = QDir(QCoreApplication::applicationDirPath()).filePath(OIGLoaderFileName);
    
    process->addEnvironmentVariable("DYLD_INSERT_LIBRARIES", oigLoaderPath);
    if (igoEnableLogging())
        process->addEnvironmentVariable("OIG_LOADER_LOGGING", "1");
    
    process->setExceptionSystem(mExceptionSystem);
    
    process->start();
#endif
}

void IGOController::unloadIGODll()
{
#ifdef ORIGIN_PC
    UnloadIGODll();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QString IGOController::hotKey() const
{
    Origin::Services::InGameHotKey toggleKey = Origin::Services::InGameHotKey::fromULongLong(Origin::Services::readSetting(Origin::Services::SETTING_INGAME_HOTKEY, Origin::Services::Session::SessionService::currentSession()));
    return toggleKey.asNativeString();
}

QString IGOController::pinHotKey() const
{
    Origin::Services::InGameHotKey pinKey = Origin::Services::InGameHotKey::fromULongLong(Origin::Services::readSetting(Origin::Services::SETTING_PIN_HOTKEY, Origin::Services::Session::SessionService::currentSession()));
    return pinKey.asNativeString();
}
    
void IGOController::setOriginKeyboardLanguage(void* language)
{
    mIPCServer->setOriginKeyboardLanguage((HKL)language);
}

bool IGOController::isEnabled()
{
    return Services::readSetting(Services::SETTING_EnableIgo, Services::Session::SessionService::currentSession());
}

bool IGOController::isEnabledForAllGames()
{
    return isEnabled() && Services::readSetting(Services::SETTING_EnableIgoForAllGames, Services::Session::SessionService::currentSession());
}

bool IGOController::isEnabledForSupportedGames()
{
    return isEnabled() && !Services::readSetting(Services::SETTING_EnableIgoForAllGames, Services::Session::SessionService::currentSession());
}

bool IGOController::isActive() const
{
    return isGameInForeground() && WindowManager()->visible();
}

bool IGOController::isVisible() const
{
    return WindowManager()->visible();
}

bool IGOController::isConnected() const
{
    return mIPCServer->isConnected();
}

bool IGOController::isAvailable() const
{
    return igowm()->isScreenLargeEnough() && isConnected();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IGOController::signalIGO(uint32_t processId)
{
#if defined(ORIGIN_PC)

    QString signalName = QString::fromStdWString(OIG_SDK_SIGNAL_NAME);
    signalName += QString::number(processId, 10);

    ORIGIN_LOG_EVENT << "SDK Signal: " << signalName;

    if (mSDKSignals.find(processId) == mSDKSignals.end())
        mSDKSignals[processId] = CreateEventW(NULL, TRUE, FALSE, signalName.toStdWString().c_str());

    if (mSDKSignals[processId] == NULL)
        mSDKSignals.erase(mSDKSignals.find(processId)); // remove broken signal
    else
    {
        BOOL done = ResetEvent(mSDKSignals[processId]);
        if (done)
        {
            BOOL signaled = SetEvent(mSDKSignals[processId]);
            if (signaled)
            {
            }
        }
    }


#elif defined(ORIGIN_MAC)


#endif
}



void IGOController::setIGOEnable(bool enable)
{
#if defined(ORIGIN_PC)
    
    HMODULE hIGO = LoadIGODll();
    if (hIGO)
    {
        typedef void (*EnableIGOFunc)(bool);
        EnableIGOFunc EnableIGO = (EnableIGOFunc)GetProcAddress(hIGO, "EnableIGO");
        if (EnableIGO)
        {
            EnableIGO(enable);
        }
    }
    
#elif defined(ORIGIN_MAC)
    
    if (checkSharedMemorySetup())
        EnableIGO(enable);
    
#endif
}

void IGOController::setIGOStressTest(bool enable)
{
#if defined(ORIGIN_PC)
    
    HMODULE hIGO = LoadIGODll();
    if (hIGO)
    {
        typedef void (*EnableIGOStressTestFunc)(bool);
        EnableIGOStressTestFunc EnableIGOStressTest = (EnableIGOStressTestFunc)GetProcAddress(hIGO, "EnableIGOStressTest");
        if (EnableIGOStressTest)
        {
            EnableIGOStressTest(enable);
        }
    }
    
#elif defined(ORIGIN_MAC)
    
    // not impl. for OSX
    
#endif
}

void IGOController::setIGODebugMenu(bool enable)
{
#if defined(ORIGIN_PC)
    
    HMODULE hIGO = LoadIGODll();
    if (hIGO)
    {
        typedef void (*EnableIGODebugMenuFunc)(bool);
        EnableIGODebugMenuFunc EnableIGODebugMenu = (EnableIGODebugMenuFunc)GetProcAddress(hIGO, "EnableIGODebugMenu");
        if (EnableIGODebugMenu)
        {
            EnableIGODebugMenu(enable);
        }
    }
    
#elif defined(ORIGIN_MAC)
    
    if (checkSharedMemorySetup())
        EnableIGODebugMenu(enable);
    
#endif
}

void IGOController::setIGOEnableLogging(bool enable)
{
    // Is it overridden from EACore.ini?
    bool forceLogging = Services::readSetting(Services::SETTING_ForceFullIGOLogging.name());
    if (forceLogging)
        enable = forceLogging;

#if defined(ORIGIN_PC)
    
    HMODULE hIGO = LoadIGODll();
    if (hIGO)
    {
        typedef void (*EnableIGOFunc)(bool);
        EnableIGOFunc EnableIGOLog = (EnableIGOFunc)GetProcAddress(hIGO, "EnableIGOLog");
        if (EnableIGOLog)
        {
            EnableIGOLog(enable);
        }
    }
    
#elif defined(ORIGIN_MAC)
    
    if (checkSharedMemorySetup())
        EnableIGOLog(enable);
    
#endif

    // Notify the running games of the change
    if (mIPCServer->isStarted())
    {
        eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgLoggingEnabled(enable));
        mIPCServer->send(msg);
    }
}

bool IGOController::igoEnableLogging() const
{
#if defined(ORIGIN_PC)
    
    HMODULE hIGO = LoadIGODll();
    if (hIGO)
    {
        typedef bool (*IsIGOLogEnabledFunc)(void);
        IsIGOLogEnabledFunc IsIGOLogEnabled = (IsIGOLogEnabledFunc)GetProcAddress(hIGO, "IsIGOLogEnabled");
        if (IsIGOLogEnabled)
        {
            return IsIGOLogEnabled();
        }
    }
    
#elif defined(ORIGIN_MAC)
    
    if (checkSharedMemorySetup())
        return IsIGOLogEnabled();
    
#endif    
    
    return false;
}
    
void IGOController::resetHotKeys()
{
    Origin::Services::InGameHotKey toggleKey = Origin::Services::InGameHotKey::fromULongLong(Origin::Services::readSetting(Origin::Services::SETTING_INGAME_HOTKEY, Origin::Services::Session::SessionService::currentSession()));
    Origin::Services::InGameHotKey pinKey = Origin::Services::InGameHotKey::fromULongLong(Origin::Services::readSetting(Origin::Services::SETTING_PIN_HOTKEY, Origin::Services::Session::SessionService::currentSession()));
    
    mIPCServer->resetHotKeys(toggleKey.firstKey(), toggleKey.lastKey(), pinKey.firstKey(), pinKey.lastKey());
    
    emit hotKeyChanged(toggleKey.asNativeString());
}

void IGOController::putGameIntoBackground()
{
    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgGoToBackground());
    mIPCServer->send(msg);
}

bool IGOController::isGameInForeground() const
{
    return IGOWindowManagerIPC::isRunning();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const QString OIGWindowIDUrlParamKey = QStringLiteral("oigwindowid");

QWebPage* IGOController::createJSWindow(const QUrl& url)
{
#ifdef _DEBUG
    ORIGIN_LOG_DEBUG << "Requesting JS window for '" << url.toString() << "'";
#endif
    int32_t wID = 0;
    // Is it a request to open an OIG window?
    if (isIGOWindow(url, wID))
    {
        if (wID && wID != IViewController::WI_INVALID) // dont' use GENERIC/INVALID
        {
            // Look up the controller to notify
            QWidget* window = WindowManager()->window(wID);
            if (window)
            {
                // Make sure this is an ASP-certified controller
                IASPViewController* aspController = dynamic_cast<IASPViewController*>(WindowManager()->windowController(window));
                if (aspController)
                    return aspController->createJSWindow(url);
            }
        }

        else
            ORIGIN_LOG_ERROR << "Skipping request using unknown windowID=" << wID << "(" << url.toString() << ")";
    }

    return NULL;
}

QUrlQuery IGOController::createJSRequestParams(IViewController* controller)
{
    // Create the URL params we need when responding to the createWindow javascript event to associate the URL page
    // to the OIG window
    QUrlQuery query;
    int32_t wID = WindowManager()->windowID(controller);
    if (wID == IViewController::WI_INVALID)
        ORIGIN_LOG_ERROR << "No window associated with controller " << controller;

    query.addQueryItem(OIGWindowIDUrlParamKey, QString::number(wID));

    return query;
}

bool IGOController::isIGOWindow(const QUrl& url, int32_t& wID)
{
    QUrlQuery query(url);
    // Angular and Qt can't agree on fragments and queries
    // It's possible that our query is empty, but still valid
    if (query.isEmpty())
    {
        QString fragment = url.fragment();
        QUrl fragmentUrl(fragment);
        QUrlQuery fragmentQuery(fragmentUrl);
        if (fragmentQuery.hasQueryItem(OIGWindowIDUrlParamKey))
        {
            QString value = fragmentQuery.queryItemValue(OIGWindowIDUrlParamKey);
            wID = value.toInt();
            return true;
        }

    }
    else
    {
        if (query.hasQueryItem(OIGWindowIDUrlParamKey))
        {
            QString value = query.queryItemValue(OIGWindowIDUrlParamKey);
            wID = value.toInt();
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IGOController::setupDesktop(Origin::Services::Exception::ExceptionSystem* exceptionSystem)
{
    if (mDesktopSetupCompleted)
        return;
    
    mExceptionSystem = exceptionSystem;
    
    // main thread because of UI creation?
    ORIGIN_ASSERT_MESSAGE(QThread::currentThread() == QCoreApplication::instance()->thread(), "Please make sure to instanciate IGOController from the main thread!");

    mDesktopSetupCompleted = true;
    // Initialize the desktop
	{
		IIGOWindowManager::WindowProperties settings;
		IIGOWindowManager::WindowBehavior behavior;
		behavior.persistent = true;
        behavior.dontStealFocus = true;
    
        IIGOWindowManager::AnimPropertySet topAnimProps;
        
        // Everything seems to be using a simple ease in ease out curve - also we want all the elements to slide in at the same speed -> use the same offscreen offsets
        const int SLIDE_OFFSCREEN_OFFSET = 200;

        IIGOWindowManager::WindowAnimBlendParams curveProps;
        curveProps.type = IIGOWindowManager::WindowAnimBlendFcn_EASE_IN_OUT;
        curveProps.durationInMS = 300;

        // Fade alpha in/out
        topAnimProps.props.alpha = 0;
        topAnimProps.propCurves.alpha = curveProps;

        // Margins for widgets
        const int MARGIN_DEFAULT = 80;

        mClock = mUIFactory->createClock();
        settings.setPosition(QPoint(MARGIN_DEFAULT + mClock->ivcView()->width(), MARGIN_DEFAULT), Engine::IIGOWindowManager::WindowDockingOrigin_X_RIGHT, true);
        
        // Slide in/out
        topAnimProps.props.posY = -SLIDE_OFFSCREEN_OFFSET;
        topAnimProps.propCurves.posY = curveProps;
        settings.setOpenAnimProps(topAnimProps);
        settings.setCloseAnimProps(topAnimProps);
        
        WindowManager()->addWindow(mClock->ivcView(), settings, behavior);

        //
        mLogo = mUIFactory->createLogo();

        settings.setPosition(QPoint(MARGIN_DEFAULT, MARGIN_DEFAULT), Engine::IIGOWindowManager::WindowDockingOrigin_DEFAULT, true);
        
        // Slide in/out
        settings.setOpenAnimProps(topAnimProps);
        settings.setCloseAnimProps(topAnimProps);
        
        WindowManager()->addWindow(mLogo->ivcView(), settings, behavior);

        //
        
        settings.setHoverOnTransparent(true);

        mTitle = mUIFactory->createTitle();
        settings.setPosition(QPoint(0, MARGIN_DEFAULT), Engine::IIGOWindowManager::WindowDockingOrigin_X_CENTER, true);
        
        // Slide in/out
        settings.setOpenAnimProps(topAnimProps);
        settings.setCloseAnimProps(topAnimProps);
        
        WindowManager()->addWindow(mTitle->ivcView(), settings, behavior);
        
        //
        
        IIGOWindowManager::AnimPropertySet leftAnimProps;
        
        // Fade alpha in/out
        leftAnimProps.props.alpha = 0;
        leftAnimProps.propCurves.alpha = curveProps;
        
        mNavigation = mUIFactory->createNavigation();
        //Margin for nav = Margin Above Logo + Logo + Margin below Logo
        settings.setPosition(QPoint(MARGIN_DEFAULT, MARGIN_DEFAULT + mLogo->ivcView()->height() + MARGIN_DEFAULT), Engine::IIGOWindowManager::WindowDockingOrigin_DEFAULT, true); 

        // Slide in/out
        leftAnimProps.props.posX = -SLIDE_OFFSCREEN_OFFSET;
        leftAnimProps.propCurves.posX = curveProps;
        settings.setOpenAnimProps(leftAnimProps);
        settings.setCloseAnimProps(leftAnimProps);

        WindowManager()->addWindow(mNavigation->ivcView(), settings, behavior);

#ifdef ORIGIN_PC
        mTwitchViewController = mUIFactory->createTwitchBroadcast();
        mTwitchBroadcastIndicator = mUIFactory->createTwitchBroadcastIndicator();

        ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastAccountLinkDialogOpen(Origin::Engine::IIGOCommandController::CallOrigin)), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastAccountLinkDialogOpen()));
        ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastDialogOpen(Origin::Engine::IIGOCommandController::CallOrigin)), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastDialogOpen()));
        ORIGIN_VERIFY_CONNECT(mTwitch, SIGNAL(broadcastDialogClosed()), Origin::SDK::SDK_ServiceArea::instance(), SLOT(BroadcastDialogClosed()));
#endif
	}
}

QRect IGOController::desktopWorkArea()
{
    QSize desktopSize = mWindowManager->screenSize();
    QRect workArea(QPoint(), desktopSize);
    return workArea;
}

// unfortunate helper to distinguish whether Twitch was started from the SDK (to help with closing IGO automatically)
#ifdef ORIGIN_PC
IIGOCommandController::CallOrigin IGOController::twitchStartedFrom() const
{
    if (mTwitchViewController)
        return mTwitchViewController->startedFrom();

    return IIGOCommandController::CallOrigin_CLIENT;
}
#endif

IWebBrowserViewController* IGOController::createWebBrowser(const QString& url)
{
    return WBManager()->createWebBrowser(url);
}

IWebBrowserViewController* IGOController::createWebBrowser(const QString& url, IWebBrowserViewController::BrowserFlags flags, const IIGOWindowManager::WindowProperties& settings, const IIGOWindowManager::WindowBehavior& behavior)
{
    return WBManager()->createWebBrowser(url, flags, settings, behavior);
}

void IGOController::alert(Origin::UIToolkit::OriginWindow* window)
{
    IIGOWindowManager::WindowProperties properties;
    IIGOWindowManager::WindowBehavior behavior;
    behavior.modal = true;
    
    WindowManager()->addPopupWindow(window, properties, behavior);
    WindowManager()->activateWindow(window);
}

void IGOController::showWebInspector(QWebPage* page)
{
    // Delay by one frame in case the call is coming from the creation of the container of the page itself
    QMetaObject::invokeMethod(this, "igoShowWebInspector", Qt::QueuedConnection, Q_ARG(QWebPage*, page));
}

IIGOWindowManager::AnimPropertySet IGOController::defaultOpenAnimation()
{
    // TODO: This is just for QA/test purposes right now
    IIGOWindowManager::AnimPropertySet tbProps;
    uint32_t valueReferences = 0;

    IIGOWindowManager::WindowAnimBlendParams curveProps;
    curveProps.type = IIGOWindowManager::WindowAnimBlendFcn_CUBIC_BEZIER;
    curveProps.durationInMS = 150;
    curveProps.optionalA = 0.03f;
    curveProps.optionalB = 1.00f;
    curveProps.optionalC = 0.20f;
    curveProps.optionalD = 1.00f;

    tbProps.props.posY = 40;
    valueReferences |= IIGOWindowManager::WindowAnimOrigin_POS_Y_BOTTOM;
    tbProps.propCurves.posY = curveProps;

    tbProps.props.alpha = 0;
    tbProps.propCurves.alpha = curveProps;

    tbProps.props.width = 0;
    valueReferences |= IIGOWindowManager::WindowAnimOrigin_WIDTH_CENTER;
    tbProps.propCurves.width = curveProps;

    tbProps.props.height = 0;
    valueReferences |= IIGOWindowManager::WindowAnimOrigin_HEIGHT_CENTER;
    tbProps.propCurves.height = curveProps;

    tbProps.valueReferences = static_cast<IIGOWindowManager::WindowAnimOrigin>(valueReferences);
    return tbProps;
}

IIGOWindowManager::AnimPropertySet IGOController::defaultCloseAnimation()
{
    // TODO: This is just for QA/test purposes right now
    IIGOWindowManager::AnimPropertySet tbProps;
    uint32_t valueReferences = 0;

    IIGOWindowManager::WindowAnimBlendParams curveProps;
    curveProps.type = IIGOWindowManager::WindowAnimBlendFcn_CUBIC_BEZIER;
    curveProps.durationInMS = 600;
    curveProps.optionalA = 0.03f;
    curveProps.optionalB = 1.00f;
    curveProps.optionalC = 0.20f;
    curveProps.optionalD = 1.00f;

    tbProps.props.posY = 40;
    valueReferences |= IIGOWindowManager::WindowAnimOrigin_POS_Y_BOTTOM;
    tbProps.propCurves.posY = curveProps;

    tbProps.props.alpha = 0;
    tbProps.propCurves.alpha = curveProps;

    tbProps.props.width = 0;
    valueReferences |= IIGOWindowManager::WindowAnimOrigin_WIDTH_CENTER;
    tbProps.propCurves.width = curveProps;

    tbProps.props.height = 0;
    valueReferences |= IIGOWindowManager::WindowAnimOrigin_HEIGHT_CENTER;
    tbProps.propCurves.height = curveProps;

    tbProps.valueReferences = static_cast<IIGOWindowManager::WindowAnimOrigin>(valueReferences);

    tbProps.propCurves.posY.durationInMS = 150;
    tbProps.propCurves.alpha.durationInMS = 150;
    tbProps.propCurves.width.durationInMS = 150;
    tbProps.propCurves.height.durationInMS = 150;
    return tbProps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t IGOController::EbisuShowIGO(bool visible)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Switch IGO on if necessary
    if (WindowManager()->setIGOVisible(visible, NULL, CallOrigin_CLIENT))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowProfileUICallback : public IIGOWindowManager::ICallback
{
public:
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->showMyProfile(IIGOCommandController::CallOrigin_SDK); } // ends up calling IGOController::igoShowProfile
};

int32_t IGOController::EbisuShowProfileUI(uint64_t user)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        // Switch IGO on if necessary
        if (WindowManager()->setIGOVisible(true, new EbisuShowProfileUICallback(), CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuShowFriendsProfileUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowFriendsProfileUICallback(uint64_t target) : mTarget(target) {}
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->showFriendsProfile(mTarget, IIGOCommandController::CallOrigin_SDK); } // ends up calling IGOController::igoShowProfile

private:
    uint64_t mTarget;
};

int32_t IGOController::EbisuShowFriendsProfileUI(uint64_t user, uint64_t target)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        // Switch IGO on if necessary
        if (WindowManager()->setIGOVisible(true, new EbisuShowFriendsProfileUICallback(target), CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuShowRecentUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowRecentUICallback(uint64_t user) : mUser(user) {}
    virtual void operator() (IIGOWindowManager* igowm)
    {
        IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(IIGOCommandController::CallOrigin_SDK);

        IIGOWindowManager::WindowBehavior behavior;

        QString url = QString("http://placehold.it/300&text=Show+Recent+UI+%1").arg(QString::number(mUser));
        IViewController* controller = IGOController::instance()->createWebBrowser(url, IWebBrowserViewController::IGO_BROWSER_SHOW_VIEW, properties, behavior);
        igowm->maximizeWindow(controller->ivcView());
    }
    
private:
    uint64_t mUser;
};

int32_t IGOController::EbisuShowRecentUI(uint64_t user)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Switch IGO on if necessary
    if (WindowManager()->setIGOVisible(true, new EbisuShowRecentUICallback(user), CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowFeedbackUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowFeedbackUICallback(uint64_t user, uint64_t target) : mUser(user), mTarget(target) {}
    virtual void operator() (IIGOWindowManager* igowm)
    {
        IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(IIGOCommandController::CallOrigin_SDK);

        IIGOWindowManager::WindowBehavior behavior;
        behavior.closeIGOOnClose = true;

        QString url = QString("http://placehold.it/300&text=Show+Feedback+UI+%1+%2").arg(QString::number(mUser), QString::number(mTarget));
        IViewController* controller = IGOController::instance()->createWebBrowser(url, IWebBrowserViewController::IGO_BROWSER_SHOW_VIEW, properties, behavior);
        igowm->maximizeWindow(controller->ivcView());
    }
    
private:
    uint64_t mUser;
    uint64_t mTarget;
};

int32_t IGOController::EbisuShowFeedbackUI(uint64_t user, uint64_t target)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Switch IGO on if necessary
    if (WindowManager()->setIGOVisible(true, new EbisuShowFeedbackUICallback(user, target), CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowBrowserUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowBrowserUICallback(const QString& url, IWebBrowserViewController::BrowserFlags flags) : mUrl(url), mFlags(flags) {}
    virtual void operator() (IIGOWindowManager* igowm)
    {
        IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(IIGOCommandController::CallOrigin_SDK);

        IIGOWindowManager::WindowBehavior behavior;
        behavior.connectionSpecific = true;
        
        IViewController* controller = IGOController::instance()->createWebBrowser(mUrl, mFlags, properties, behavior);
        Q_UNUSED(controller)
    }
    
private:
    QString mUrl;
    IWebBrowserViewController::BrowserFlags mFlags;
};

int32_t IGOController::EbisuShowBrowserUI(IWebBrowserViewController::BrowserFlags flags, const char* url)
{
    int32_t retVal = EBISU_ERROR_GENERIC;

    // This is only for QA to test mini-app browser behavior until the SDK Tool is updated with a button to toggle the flag
    bool miniAppBrowser = Origin::Services::readSetting(Origin::Services::SETTING_IGOMiniAppBrowser);
    if (miniAppBrowser)
        flags = Origin::Engine::IWebBrowserViewController::IGO_BROWSER_MINI_APP;

    // Always try to expand URLs coming from the SDK (ie the game itself)
    flags |= Origin::Engine::IWebBrowserViewController::IGO_BROWSER_RESOLVE_URL;

    // This is far from perfect!
    QString locale = gameTracker()->currentGameInfo().mBase.mLocale;
    QString resolvedURL = urlUnwinder()->staticResolve(url, locale);

    // Switch IGO on if necessary
    if (WindowManager()->setIGOVisible(true, new EbisuShowBrowserUICallback(resolvedURL, flags), CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowCheckoutUICallback : public IIGOWindowManager::ICallback, public IGOController::ICallback<IPDLCStoreViewController>
{
public:
    EbisuShowCheckoutUICallback(const char* pURL)
    : mURL(pURL)
    {
    }

    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->createPdlcStore(this, IIGOCommandController::CallOrigin_SDK); }
    virtual void operator() (IPDLCStoreViewController* store) { store->showInIGO(mURL); }

private:
    QString mURL;
};

int32_t IGOController::EbisuShowCheckoutUI(const char* pURL)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    EbisuShowCheckoutUICallback* callback = new EbisuShowCheckoutUICallback(pURL);
    if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

class EbisuShowStoreUIByCategoryIdsCallback : public IIGOWindowManager::ICallback, public IGOController::ICallback<IPDLCStoreViewController>
{
public:
    EbisuShowStoreUIByCategoryIdsCallback(const QString& contentId, const QString& categoryIds, const QString& offerIds)
    : mContentId(contentId), mCategoryIds(categoryIds), mOfferIds(offerIds)
    {
    }

    virtual void operator() (IPDLCStoreViewController* store) { store->showInIGOByCategoryIds(mContentId, mCategoryIds, mOfferIds); }
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->createPdlcStore(this, IIGOCommandController::CallOrigin_SDK); }
    
private:
    QString mContentId;
    QString mCategoryIds;
    QString mOfferIds;
};

int32_t IGOController::EbisuShowStoreUIByCategoryIds(const QString& gamerId, const QString& contentId, const QString& categoryIds, const QString& offerIds)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    EbisuShowStoreUIByCategoryIdsCallback* callback = new EbisuShowStoreUIByCategoryIdsCallback(contentId, categoryIds, offerIds);
    if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

class EbisuShowStoreUIByMasterTitleIdCallback : public IIGOWindowManager::ICallback, public IGOController::ICallback<IPDLCStoreViewController>
{
public:
    EbisuShowStoreUIByMasterTitleIdCallback(const QString& contentId, const QString& masterTitleIds, const QString& offerIds)
    : mContentId(contentId), mMasterTitleIds(masterTitleIds), mOfferIds(offerIds)
    {
    }
    
    virtual void operator() (IPDLCStoreViewController* store) { store->showInIGOByMasterTitleId(mContentId, mMasterTitleIds, mOfferIds); }
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->createPdlcStore(this, IIGOCommandController::CallOrigin_SDK); }

private:
    QString mContentId;
    QString mMasterTitleIds;
    QString mOfferIds;
};

int32_t IGOController::EbisuShowStoreUIByMasterTitleId(const QString& gamerId, const QString& contentId, const QString& masterTitleIds, const QString& offerIds)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    EbisuShowStoreUIByMasterTitleIdCallback* callback = new EbisuShowStoreUIByMasterTitleIdCallback(contentId, masterTitleIds, offerIds);
    if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowFriendsUICallback : public IIGOWindowManager::ICallback
{
public:
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->igoShowFriendsList(IIGOCommandController::CallOrigin_SDK); }
};

int32_t IGOController::EbisuShowFriendsUI(uint64_t user)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        EbisuShowFriendsUICallback* callback = new EbisuShowFriendsUICallback();
        if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuShowFindFriendsUICallback : public IIGOWindowManager::ICallback
{
public:
    virtual void operator() (IIGOWindowManager* igowm) { emit IGOController::instance()->showFindFriends(IIGOCommandController::CallOrigin_SDK); }
};

int32_t IGOController::EbisuShowFindFriendsUI(uint64_t user)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        EbisuShowFindFriendsUICallback* callback = new EbisuShowFindFriendsUICallback();
        if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuRequestFriendUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuRequestFriendUICallback(IGOController* controller, uint64_t user, uint64_t target, const char* source)
    : mController(controller), mUser(user), mTarget(target), mSource(QString::fromUtf8(source))
    {
    }
    
    virtual void operator() (IIGOWindowManager* igowm)
    {
        if (mUser != 0 && mTarget != 0)
        {
            // Find our Jabber ID
            Chat::OriginConnection *conn = Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection();
            const Chat::JabberID jid(conn->nucleusIdToJabberId(mTarget));
            
            conn->currentUser()->roster()->requestSubscription(jid, mSource);
        }

        mController->EbisuShowFriendsProfileUI(mUser, mTarget);
    }
    
private:
    IGOController* mController;

    uint64_t mUser;
    uint64_t mTarget;
    QString mSource;
};

int32_t IGOController::EbisuRequestFriendUI(uint64_t user, uint64_t target, const char* source)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        // Delay opening the Chat window until OIG is visible to fix the activation of the Chat window issue
        EbisuRequestFriendUICallback* callback = new EbisuRequestFriendUICallback(this, user, target, source);
        if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuShowComposeChatUICallback : public IIGOWindowManager::ICallback
{
    public:
        EbisuShowComposeChatUICallback(IGOController* controller, uint64_t user, const uint64_t *pUserList, uint32_t iUserCount, const char *pMessage)
        : mController(controller)
        {
            mEbisuChatCallbackSenderFriendID.setNum(user);
            for ( uint32_t i = 0; i < iUserCount; ++i )
            {
                QString currentFriend;
                currentFriend.setNum(pUserList[i]);
                mEbisuChatCallbackToFriendID.push_back(currentFriend);
            }
            
            mEbisuChatCallbackMessage = QString::fromUtf8(pMessage);
        }
    
        virtual void operator() (IIGOWindowManager* igowm)
        {
            mController->showChat(mEbisuChatCallbackSenderFriendID, mEbisuChatCallbackToFriendID, mEbisuChatCallbackMessage, IIGOCommandController::CallOrigin_SDK);
        }

    private:
        IGOController* mController;
    
        QString mEbisuChatCallbackMessage;
        QString mEbisuChatCallbackSenderFriendID;
        QStringList mEbisuChatCallbackToFriendID;
};

int32_t IGOController::EbisuShowComposeChatUI(uint64_t user, const uint64_t *pUserList, uint32_t iUserCount, const char *pMessage)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        if (iUserCount > 0)
        {
            // Delay opening the Chat window until OIG is visible to fix the activation of the Chat window issue
            EbisuShowComposeChatUICallback* callback = new EbisuShowComposeChatUICallback(this, user, pUserList, iUserCount, pMessage);
            if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
                retVal = EBISU_SUCCESS;
        }
    }
    
    return retVal;
}

//

class EbisuShowInviteUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowInviteUICallback(const QString& gameName)
    : mGameName(gameName) {}
    
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->igoShowInviteFriendsToGame(mGameName, IIGOCommandController::CallOrigin_SDK); }
    
private:
    QString mGameName;
};

int32_t IGOController::EbisuShowInviteUI()
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    IGOGameTracker::GameInfo info = mGameTracker->currentGameInfo();
    EbisuShowInviteUICallback* callback = new EbisuShowInviteUICallback(info.mBase.mTitle);
    if (WindowManager()->setIGOVisible(true, callback, CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowAchievementsUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowAchievementsUICallback(uint64_t userId, uint64_t personaId, const QString &achievementSet, IIGOCommandController::CallOrigin callOrigin)
        : mUserId(userId), mPersonaId(personaId), mAchievementSet(achievementSet), mCallOrigin(callOrigin) {}

    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->igoShowAchievements(mUserId, mPersonaId, mAchievementSet, mCallOrigin); }

private:
    uint64_t mUserId;
    uint64_t mPersonaId;
    QString mAchievementSet;
    IIGOCommandController::CallOrigin mCallOrigin;
};

int32_t IGOController::EbisuShowAchievementsUI(uint64_t userId, uint64_t personaId, const QString &achievementSet, bool showIgo, CallOrigin callOrigin)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        // Delay opening the window until OIG is visible (except if we don't want to force a change of the current visibility!)
        bool visible = showIgo ? true : isVisible();
        EbisuShowAchievementsUICallback* callback = new EbisuShowAchievementsUICallback(userId, personaId, achievementSet, callOrigin);
        if (WindowManager()->setIGOVisible(visible, callback, callOrigin))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuShowSelectAvatarUICallback : public IIGOWindowManager::ICallback
{
public:
    virtual void operator() (IIGOWindowManager* igowm) { emit IGOController::instance()->showAvatarSelect(IIGOCommandController::CallOrigin_SDK); }
};

int32_t IGOController::EbisuShowSelectAvatarUI(uint64_t user)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    if (WindowManager()->setIGOVisible(true, new EbisuShowSelectAvatarUICallback(), IIGOCommandController::CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//
    
int32_t IGOController::EbisuShowGameDetailsUI(const QString &productId, const QString& masterTitleId)
{
    QScopedPointer<IPDLCStoreViewController> pdlc(mUIFactory->createPDLCStore());
    QUrl url = pdlc->productUrl(productId, masterTitleId);
    
    return EbisuShowBrowserUI(IWebBrowserViewController::IGO_BROWSER_SHOW_VIEW, url.toString().toLocal8Bit());
}

//

class EbisuShowBroadcastUICallback : public IIGOWindowManager::ICallback
{
public:
    virtual void operator() (IIGOWindowManager* igowm) { igowm->igoCommand(IIGOCommandController::CMD_SHOW_BROADCAST, IIGOCommandController::CallOrigin_SDK); }
};

int32_t IGOController::EbisuShowBroadcastUI()
{
    int32_t retVal = EBISU_ERROR_GENERIC;

    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        // Switch IGO on if necessary
        if (WindowManager()->setIGOVisible(true, new EbisuShowBroadcastUICallback(), IIGOCommandController::CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }

    return retVal;
}

class EbisuShowReportUserUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowReportUserUICallback(uint64_t target, const QString& masterTitle) : mTarget(target), mMasterTitle(masterTitle) {}
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->showReportUser(mTarget, mMasterTitle, IIGOCommandController::CallOrigin_SDK); }

private:
    uint64_t mTarget;
    QString mMasterTitle;
};

int32_t IGOController::EbisuShowReportUserUI(uint64_t user, uint64_t target, const QString& masterTitle)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    // Not available to underage user
    Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::instance()->currentUser();
    if (currentUser && !currentUser->isUnderAge())
    {
        // Switch IGO on if necessary
        if (WindowManager()->setIGOVisible(true, new EbisuShowReportUserUICallback(target, masterTitle), IIGOCommandController::CallOrigin_SDK))
            retVal = EBISU_SUCCESS;
    }
    
    return retVal;
}

//

class EbisuShowCodeRedemptionUICallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowCodeRedemptionUICallback() {}
    
    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->igoShowCodeRedemption(IIGOCommandController::CallOrigin_SDK); }
};

int32_t IGOController::EbisuShowCodeRedemptionUI()
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    IGOGameTracker::GameInfo info = mGameTracker->currentGameInfo();
    EbisuShowCodeRedemptionUICallback* callback = new EbisuShowCodeRedemptionUICallback();
    if (WindowManager()->setIGOVisible(true, callback, IIGOCommandController::CallOrigin_SDK))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

//

class EbisuShowErrorCallback : public IIGOWindowManager::ICallback
{
public:
    EbisuShowErrorCallback(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot, IIGOCommandController::CallOrigin callOrigin)
        : mTitle(title), mText(text), mOkBtnText(okBtnText), mOkBtnSlot(okBtnSlot), mOkBtnObj(okBtnObj), mCallOrigin(callOrigin) {}

    virtual void operator() (IIGOWindowManager* igowm) { IGOController::instance()->igoShowError(mTitle, mText, mOkBtnText, mOkBtnObj, mOkBtnSlot, mCallOrigin); }

private:
    QString mTitle;
    QString mText;
    QString mOkBtnText;
    QString mOkBtnSlot;
    QPointer<QObject> mOkBtnObj;
    IIGOCommandController::CallOrigin mCallOrigin;
};

int32_t IGOController::EbisuShowErrorUI(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot)
{
    int32_t retVal = EBISU_ERROR_GENERIC;

    EbisuShowErrorCallback* callback = new EbisuShowErrorCallback(title, text, okBtnText, okBtnObj, okBtnSlot, CallOrigin_SDK);
    if (WindowManager()->setIGOVisible(isVisible(), callback, CallOrigin_SDK))
        retVal = EBISU_SUCCESS;

    return retVal;
}

int32_t IGOController::showError(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot, IIGOCommandController::CallOrigin callOrigin)
{
    int32_t retVal = EBISU_ERROR_GENERIC;
    
    EbisuShowErrorCallback* callback = new EbisuShowErrorCallback(title, text, okBtnText, okBtnObj, okBtnSlot, callOrigin);
    if (WindowManager()->setIGOVisible(isVisible(), callback, callOrigin))
        retVal = EBISU_SUCCESS;
    
    return retVal;
}

bool IGOController::isBroadcasting()
{
#ifdef ORIGIN_PC
    return mTwitch->isBroadcasting();
#else
    return false;
#endif
}

bool IGOController::isBroadcastingPending()
{
#ifdef ORIGIN_PC
    return mTwitch->isBroadcastingPending();
#else
    return false;
#endif
}

bool IGOController::isBroadcastTokenValid()
{
#ifdef ORIGIN_PC
    return mTwitch->isBroadcastTokenValid();
#else
    return false;
#endif
}

bool IGOController::isBroadcastingBlocked()
{
#ifdef ORIGIN_PC
    return mTwitch->isBroadcastingBlocked();
#else
    return false;
#endif
}

void IGOController::setBroadcastOriginatedFromSDK(bool flag)
{
#ifdef ORIGIN_PC
    mTwitch->setBroadcastOriginatedFromSDK(flag);
#endif
}

bool IGOController::attemptBroadcastStart(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
#ifdef ORIGIN_PC
    return mTwitch->attemptBroadcastStart(callOrigin);
#else
    return false;
#endif
}

bool IGOController::attemptBroadcastStop(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
#ifdef ORIGIN_PC
    return mTwitch->attemptBroadcastStop(callOrigin);
#else
    return false;
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SearchProfileViewController::SearchProfileViewController(QObject* parent)
    : SharedViewController(parent)
{
#ifdef ORIGIN_MAC
    // Need to match the minimum game window size (800x600)
    static const int DefaultXMargin = 20;
    static const int DefaultYMargin = 20;
    
    QSize defaultSize(1024, 640);
    QSize screenSize = IGOController::instance()->igowm()->screenSize();
    
    if (defaultSize.width() > (screenSize.width() - DefaultXMargin* 2))
        defaultSize.setWidth(screenSize.width() - DefaultXMargin * 2);
    
    if (defaultSize.height() > (screenSize.height() - DefaultYMargin * 2))
        defaultSize.setHeight(screenSize.height() - DefaultYMargin * 2);

    QSize minSize = IGOController::instance()->igowm()->minimumScreenSize();
    QSize minimumSize(minSize.width() - DefaultXMargin * 2, minSize.height() - DefaultYMargin * 2);
#else
    QSize defaultSize(1024, 640);
    QSize minimumSize(995, 640);
#endif

    UIToolkit::OriginWindow* window = IGOController::instance()->uiFactory()->createSharedWindow(defaultSize, minimumSize);
    setWindow(window);
}

SearchProfileViewController::~SearchProfileViewController()
{
    // TODO: RIGHT NOW, THE CLIENT SEARCH/PROFILE CONTROLLERS DON'T PROPERLY DESTROY THEIR VIEWS - DOING SO ACTUALLY WILL REQUIRE
    // UPDATING THE NAVIGATION "CONTROLLER" THAT STORES DIRECT POINTERS TO THOSE CONTROLLERS' WEB HISTORY!
    // Make sure to detach from current content, which should be deleted from the real controller owner
    
    // SO IN THE MEANTIME, WE DESTROY THE VIEWS HERE!
    show(ControllerID_NONE);
    ISearchViewController* searchController = search();
    if (searchController)
        searchController->ivcView()->deleteLater();
    
    IProfileViewController* profileController = profile();
    if (profileController)
        profileController->ivcView()->deleteLater();
}

void IGOController::igoShowSearch(const QString& keyword, CallOrigin callOrigin)
{
    // We share the window for the profile and search view
    SearchProfileViewController* mainController = SPViewController(callOrigin);
    if (mainController)
    {
        ISearchViewController* searchController = static_cast<ISearchViewController*>(mainController->search());
        if (!searchController)
        {
            searchController = mUIFactory->createSearch();
            if (searchController)
                mainController->addSearch(searchController);
        }
        
        if (searchController)
        {
            searchController->isLoadSearchPage(keyword);
            
            mainController->showSearch();
            WindowManager()->setFocusWindow(mainController->ivcView());
            
            IProfileViewController* profileController = mainController->profile();
            if (profileController)
                profileController->resetNucleusId();
        }
    }
    
    ORIGIN_LOG_EVENT << "[information] Showing OIG search";
}

void IGOController::igoShowProfile(quint64 nucleusId, CallOrigin callOrigin)
{
    // At some point the SDK code should be updated to call the correct function
    // For now we will just re-direct to the correct function.
    igoShowSharedWindow(QString::number(nucleusId), callOrigin);
}

//

// Handles the relocation of the friends list window when the user resizes the game
void IGOController::igoShowFriendsList(CallOrigin callOrigin)
{
    IIGOWindowManager::WindowProperties properties;
    properties.setCallOrigin(callOrigin);
    properties.setOpenAnimProps(defaultOpenAnimation());
    properties.setCloseAnimProps(defaultCloseAnimation());

    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;
    behavior.nsBorderSize = 6;
    behavior.ewBorderSize = 4;
    behavior.cacheMode = QGraphicsItem::ItemCoordinateCache;

    ControllerFunctorFactoryNoArg<ISocialHubViewController> functor(&IIGOUIFactory::createSocialHub);
    ISocialHubViewController* hub = createController<ISocialHubViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);
    Q_UNUSED(hub);
}

void IGOController::igoShowChatWindow(CallOrigin callOrigin)
{
    IIGOWindowManager::WindowProperties properties;
    properties.setCallOrigin(callOrigin);
    properties.setOpenAnimProps(defaultOpenAnimation());
    properties.setCloseAnimProps(defaultCloseAnimation());

    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;
    behavior.nsBorderSize = 6;
    behavior.ewBorderSize = 4;
    behavior.cacheMode = QGraphicsItem::ItemCoordinateCache;

    ControllerFunctorFactoryNoArg<ISocialConversationViewController> functor(&IIGOUIFactory::createSocialChat);
    ISocialConversationViewController* chat = createController<ISocialConversationViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);
    Q_UNUSED(chat);
}


void IGOController::igoShowSharedWindow(const QString& id, IIGOCommandController::CallOrigin callOrigin)
{
    ORIGIN_LOG_EVENT << "Request to show Shared SPA window";
    IIGOWindowManager::WindowProperties properties;
    properties.setCallOrigin(callOrigin);
    properties.setOpenAnimProps(defaultOpenAnimation());
    properties.setCloseAnimProps(defaultCloseAnimation());

    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;
    behavior.nsBorderSize = 6;
    behavior.ewBorderSize = 4;
    behavior.cacheMode = QGraphicsItem::ItemCoordinateCache;

    ControllerFunctorFactoryOneArg<ISharedSPAViewController, const QString&> functor(&IIGOUIFactory::createSharedSPAWindow, id);
    ISharedSPAViewController* shared = createController<ISharedSPAViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);
    shared->navigate(id);
}

//
    
void IGOController::igoShowSettings(ISettingsViewController::Tab displayTab /*= ISettingsViewController::Tab_GENERAL*/)
{
    IIGOWindowManager::WindowProperties properties;
    properties.setOpenAnimProps(defaultOpenAnimation());
    properties.setCloseAnimProps(defaultCloseAnimation());

    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;
#ifdef ORIGIN_MAC
    behavior.nsBorderSize = 10;
    behavior.ewBorderSize = 10;
    behavior.nsMarginSize = 6;
    behavior.ewMarginSize = 6;
#else
    behavior.nsBorderSize = 20;
    behavior.ewBorderSize = 10;
#endif

    ControllerFunctorFactoryOneArg<ISettingsViewController, ISettingsViewController::Tab> functor(&IIGOUIFactory::createSettings, displayTab);
    ISettingsViewController* settings = createController<ISettingsViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);
    Q_UNUSED(settings);
}

//

void IGOController::igoShowCustomerSupport()
{
    IIGOWindowManager::WindowProperties settings;
    settings.setOpenAnimProps(defaultOpenAnimation());
    settings.setCloseAnimProps(defaultCloseAnimation());
    
    IIGOWindowManager::WindowBehavior behavior;
#ifdef ORIGIN_MAC
    behavior.draggingSize = 40;
    behavior.nsBorderSize = 10;
    behavior.ewBorderSize = 10;
    behavior.nsMarginSize = 6;
    behavior.ewMarginSize = 6;
#else
    behavior.draggingSize = 50;
    behavior.nsBorderSize = 20;
    behavior.ewBorderSize = 10;
#endif
    
    ControllerFunctorFactoryNoArg<ICustomerSupportViewController> functor(&IIGOUIFactory::createCustomerSupport);
    ICustomerSupportViewController* customerSupport = createController<ICustomerSupportViewController>(ControllerWindowType_NORMAL, functor, &settings, &behavior);
    Q_UNUSED(customerSupport);
}

//

void IGOController::igoShowInviteFriendsToGame(const QString& gameName, CallOrigin callOrigin)
{
    Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
    if (user)
    {
        IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(callOrigin);
        properties.setOpenAnimProps(defaultOpenAnimation());
        properties.setCloseAnimProps(defaultCloseAnimation());
        
        IIGOWindowManager::WindowBehavior behavior;
        behavior.cacheMode = QGraphicsItem::ItemCoordinateCache;

        ControllerFunctorFactoryOneArg<IInviteFriendsToGameViewController, const QString&> functor(&IIGOUIFactory::createInviteFriendsToGame, gameName);
        IInviteFriendsToGameViewController* invite = createController<IInviteFriendsToGameViewController>(ControllerWindowType_POPUP, functor, &properties, &behavior);
        Q_UNUSED(invite);
    }
}

//
    
void IGOController::igoShowAchievements(uint64_t userId, uint64_t personaId, const QString &achievementSet, CallOrigin callOrigin)
{
    Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
    if (user)
    {
        IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(callOrigin);
        properties.setOpenAnimProps(defaultOpenAnimation());
        properties.setCloseAnimProps(defaultCloseAnimation());
        
        IIGOWindowManager::WindowBehavior behavior;
        behavior.draggingSize = 40;
        behavior.nsBorderSize = 20;
        behavior.ewBorderSize = 10;
        behavior.pinnable = true;

        ControllerFunctorFactoryNoArg<IAchievementsViewController> functor(&IIGOUIFactory::createAchievements);
        IAchievementsViewController* achievements = createController<IAchievementsViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);
        if (achievements)
            achievements->show(user, userId, achievementSet);
    }
}

//

class showErrorViewCallback : public IGOController::ICallback<IErrorViewController>
{
public:
    showErrorViewCallback(Origin::UIToolkit::OriginWindow* window) : mWindow(window) {}
    virtual void operator()(IErrorViewController* controller) { controller->setView(mWindow); }
    
private:
    Origin::UIToolkit::OriginWindow* mWindow;
};
    
void IGOController::igoShowError(Origin::UIToolkit::OriginWindow* window)
{
    showErrorViewCallback callback(window);
    ErrorController(&callback, CallOrigin_SDK);
}

class showErrorParamsCallback : public IGOController::ICallback<IErrorViewController>
{
public:
    showErrorParamsCallback(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot) : mTitle(title), mText(text), mOkBtnText(okBtnText), mOkBtnSlot(okBtnSlot), mOkBtnObj(okBtnObj) {}
    virtual void operator()(IErrorViewController* controller) { controller->setView(mTitle, mText, mOkBtnText, mOkBtnObj, mOkBtnSlot); }
    virtual ~showErrorParamsCallback() {};
    
private:
    QString mTitle;
    QString mText;
    QString mOkBtnText;
    QString mOkBtnSlot;
    QObject* mOkBtnObj;
};

void IGOController::igoShowError(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot, CallOrigin callOrigin)
{
    showErrorParamsCallback callback(title, text, okBtnText, okBtnObj, okBtnSlot);
    ErrorController(&callback, callOrigin);
}

//
    
void IGOController::igoShowDownloads()
{
    if (mGlobalProgress == NULL) {
        mGlobalProgress = mUIFactory->createGlobalProgress();
    }
    mGlobalProgress->showWindow();
}

//

void IGOController::igoShowBroadcast()
{
#ifdef ORIGIN_PC
    if(mTwitch)
    {
        mTwitch->attemptDisplayWindow(IIGOCommandController::CallOrigin_CLIENT);
    }
#endif
}

//
    
IPDLCStoreViewController* IGOController::createPdlcStore(ICallback<IPDLCStoreViewController>* callback, CallOrigin callOrigin)
{
    IIGOWindowManager::WindowProperties properties;
    properties.setCallOrigin(callOrigin);
    properties.setOpenAnimProps(defaultOpenAnimation());
    properties.setCloseAnimProps(defaultCloseAnimation());
    
    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;

    ControllerFunctorFactoryNoArg<IPDLCStoreViewController> functor(&IIGOUIFactory::createPDLCStore);
    return createController<IPDLCStoreViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior, callback);
}

//
    
void IGOController::igoShowStoreUI(Origin::Engine::Content::EntitlementRef entRef)
{
    IPDLCStoreViewController* store = createPdlcStore(NULL, CallOrigin_CLIENT);
    if (store)
        store->show(entRef);
}

//
    
void IGOController::igoShowCheckoutUI(Origin::Engine::Content::EntitlementRef entRef)
{
    IPDLCStoreViewController* store = createPdlcStore(NULL, CallOrigin_CLIENT);
    if (store)
        store->purchase(entRef);
}

//

void IGOController::igoShowCodeRedemption(CallOrigin callOrigin)
{
    Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
    if (user)
    {
        IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(callOrigin);
        properties.setOpenAnimProps(defaultOpenAnimation());
        properties.setCloseAnimProps(defaultCloseAnimation());

        IIGOWindowManager::WindowBehavior behavior;

        ControllerFunctorFactoryNoArg<ICodeRedemptionViewController> functor(&IIGOUIFactory::createCodeRedemption);
        ICodeRedemptionViewController* codeRedemption = createController<ICodeRedemptionViewController>(ControllerWindowType_POPUP, functor, &properties, &behavior);
        Q_UNUSED(codeRedemption);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IGOController::igoShowBrowser(const QString& url, bool useTabs)
{
    Origin::Engine::IWebBrowserViewController::BrowserFlags flags = Engine::IWebBrowserViewController::IGO_BROWSER_SHOW_NAV;
    if (useTabs)
        flags |= Engine::IWebBrowserViewController::IGO_BROWSER_ADD_TAB;

    Engine::IIGOWindowManager::WindowProperties settings;
    settings.setOpenAnimProps(defaultOpenAnimation());
    settings.setCloseAnimProps(defaultCloseAnimation());
    settings.setMaximumSize();

    createWebBrowser(url, (IWebBrowserViewController::BrowserFlags)flags, settings, IIGOWindowManager::WindowBehavior());
}

void IGOController::igoShowBrowserForGame()
{
    WindowManager()->showBrowserForGame();
}

void IGOController::igoStateChanged(bool visible, bool sdkCall)
{
    // Should we show the welcome dialog?
    uint32_t gameId = mGameTracker->currentGameId();
    IGOGameInstances::iterator gameInfo = mGames.find(gameId);
    
    // Don't show the Welcome dialog if IGO was opened because of a call from the game/SDK
    if (visible && !sdkCall && gameInfo != mGames.end() && !gameInfo->second.welcomedUser)
    {
        gameInfo->second.welcomedUser = true;
        // Keeping SETTING_SHOWIGONUX setting for when we redo the IGO NUX
    }

    emit stateChanged(visible);
}

void IGOController::igoShowWebInspector(QWebPage* page)
{
    static const char* WEB_INSPECTOR_CHILD_NAME = "OIGWI";
    
    if (!page)
        return;
    
    // Did we already create a web inspector for this page?
    QObject* wi = page->findChild<QObject*>(WEB_INSPECTOR_CHILD_NAME);
    if (wi)
    {
        // Yep, simply make it the top window
        IWebInspectorController* wiController = dynamic_cast<IWebInspectorController*>(wi);
        if (wiController)
            WindowManager()->setFocusWindow(wiController->ivcView());
        
        return;
    }
    
    // Let's create a new one then
    IIGOWindowManager::WindowProperties properties;
    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;
    behavior.nsBorderSize = 10;
    behavior.ewBorderSize = 10;

    ControllerFunctorFactoryOneArg<IWebInspectorController, QWebPage*> functor(&IIGOUIFactory::createWebInspector, page);
    IWebInspectorController* wiController = createController<IWebInspectorController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);

    wi = dynamic_cast<QObject*>(wiController);
    if (wi)
        wi->setParent(page);
}
    

void IGOController::onGameAdded(uint32_t gameId)
{
    mGames[gameId] = IGOGameInfo();
    
    // Make sure we have the Korean-specific controller setup - don't wait to get focus on the game, especially
    // in the case of Mac because you won't get an initial focus event until the game is showing in the minmum resolution
    // supported by IGO
    if (!mKoreanController)
        mKoreanController = mUIFactory->createKoreanTooMuchFun();
    
    // Are we trying to re-attach to a running game, in which case we should notify the content controller
    IGOGameTracker::GameInfo info = gameTracker()->gameInfo(gameId);
    if (info.mIsRestart)
    {
        Content::EntitlementRef entitlement = Content::ContentController::currentUserContentController()->entitlementByProductId(info.mBase.mProductID);
        if (entitlement)
        {
            Content::LocalContent* content = entitlement->localContent();
            if (content)
            {
                Origin::Services::IProcess* process = content->playRunningInstance(gameId, info.mBase.mExecutablePath);
                if (process)
                {
                    ORIGIN_VERIFY_CONNECT(process, SIGNAL(finished(uint32_t, int)), WindowManager(), SLOT(onGameProcessFinished(uint32_t, int)));
                }
            }
        }
    }
}

void IGOController::onGameFocused(uint32_t gameId)
{
    // Show notification IGO available
	IGOGameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end() && !iter->second.notifyIGOAvailable)
    {
        iter->second.notifyIGOAvailable = true;
        IGOAvailableNotifier* notifier = new IGOAvailableNotifier(WindowManager(), mGameTracker, gameId);
        Q_UNUSED(notifier);
    }
}

void IGOController::onGameRemoved(uint32_t gameId)
{
    mGames.erase(gameId);
    
    // Stop the Korean Gaming Law timer when we are not playing any games.
    if (mGames.size() == 0)
    {
        delete mKoreanController;
        mKoreanController = NULL;
    }

    // NOTE: WE USED TO RELY ON THIS INSTEAD TO KNOW WHEN TO STOP KOREAN CONTROLLER, KEEPING IT AROUND IN CASE IT'S STILL USEFUL FOR SOME REASON
    // ORIGIN_VERIFY_CONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(onPlayFinished()));
}
    
void IGOController::onUserLoggedIn(Origin::Engine::UserRef user)
{
    mUserIsLoggedIn = true;
    WindowManager()->enableUpdates(true);   
    WindowManager()->closeAllWindows();

    ORIGIN_ASSERT(Content::ContentController::currentUserContentController());

    ORIGIN_VERIFY_CONNECT(Content::ContentController::currentUserContentController(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(onEntitlementsLoaded()));
    ORIGIN_VERIFY_CONNECT(Content::ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SLOT(onEntitlementsLoaded()));
}

void IGOController::onUserLoggedOut(Origin::Engine::UserRef user)
{
    mUserIsLoggedIn = false;
    
    mIPCServer->stop();
    mGameTracker->reset();
    WindowManager()->reset();

    ORIGIN_VERIFY_DISCONNECT(Content::ContentController::currentUserContentController(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(onEntitlementsLoaded()));
    ORIGIN_VERIFY_DISCONNECT(Content::ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SLOT(onEntitlementsLoaded()));
    ORIGIN_VERIFY_DISCONNECT(Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)), this, SLOT(onSettingChanged(const QString&, const Origin::Services::Variant&)));
}

void IGOController::onSettingChanged(const QString& name, const Origin::Services::Variant& value)
{
    if (name == Origin::Services::SETTING_INGAME_HOTKEY.name()
        || name == Origin::Services::SETTING_INGAME_HOTKEYSTRING.name()
        || name == Origin::Services::SETTING_PIN_HOTKEY.name()
        || name == Origin::Services::SETTING_PIN_HOTKEYSTRING.name())
    {
        resetHotKeys();
    }

    if (name == Origin::Services::SETTING_ENABLEINGAMELOGGING.name())
    {
        setIGOEnableLogging(value);
    }
}

void IGOController::onEntitlementsLoaded()
{
    ORIGIN_VERIFY_DISCONNECT(Content::ContentController::currentUserContentController(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(onEntitlementsLoaded()));
    ORIGIN_VERIFY_DISCONNECT(Content::ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SLOT(onEntitlementsLoaded()));

    // The connection server shouldn't have been started until now
    ORIGIN_ASSERT(!mIPCServer->isStarted());
    if (mIPCServer->isStarted())
        ORIGIN_LOG_ERROR << "IGOController::onEntitlementsLoaded - server previously started...This indicates an error, seems like IGOController was started in the wrong place!";

    // Wait until we have loaded the initial set of entitlements before listening to games trying to reconnect to the client
    mIPCServer->restart(true);
    
    // Register our current set of hotkeys
    resetHotKeys();

    ORIGIN_VERIFY_CONNECT(Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)), this, SLOT(onSettingChanged(const QString&, const Origin::Services::Variant&)));
    
    //A non RTP SDK game might already be running...
    //Usually that is handled by the content controller, but if we launch a non RTP game, the SDK is up and running, before a user has been logged in!
    
    //It would be nice, to have some kind of "getSDKConnections().size() to put the next line
    signalIGO(0);
}

#ifdef ORIGIN_PC
void IGOController::onOIGOffsetsComputed()
{
    ++mOffsetsLookupCount;
}
#endif
    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IGOController::removeOldLogs()
{
    // Right now, remove IGO logs that are over 30 days old
    QString logFilesPath = Origin::Services::PlatformService::logDirectory();
    QDir dir(logFilesPath);
    
    QDateTime today = QDateTime::currentDateTime();
    
    QStringList filters;
    filters.append("IGO_Log.*");
    filters.append("IGOProxy*");
    QStringList files = dir.entryList(filters, QDir::Files | QDir::NoSymLinks | QDir::Writable);
    foreach (QString file, files)
    {
        QFileInfo info(dir.absoluteFilePath(file));
        QDateTime creation = info.created();
        QDateTime lastModified = info.lastModified();
        QDateTime checkTime = creation < lastModified ? creation : lastModified;
        
        if (checkTime.daysTo(today) > 15)
            dir.remove(file);
    }
}

#ifdef ORIGIN_PC
bool IGOController::isIGODLLValid(QString igoName)
{
    bool valid = true;
    QString igoPath = QCoreApplication::applicationDirPath() + "/" + igoName;
    QString originPath = QCoreApplication::applicationFilePath();
    
    //we should have a handle already unless the DLL is missing
    HMODULE hIGO = GetModuleHandle(igoName.utf16());
    
    //see if the certificates match
    bool certificatesMatch = Services::PlatformService::compareFileSigning(originPath, igoPath);
#ifdef DEBUG
    certificatesMatch = true;
#endif
    
    //check if we have a valid handle as well
    if(!hIGO || !certificatesMatch)
    {
        ORIGIN_LOG_ERROR << "Valid IGO Not found.";
        valid = false;
    }
    
    return valid;
}

void IGOController::dispatchOffsetsLookup()
{
    QString appPath = QCoreApplication::applicationDirPath();
	appPath.replace("/", "\\");

    static const char* APIS[] = {"DX10", "DX11", "DX12" };
    int apiCount = _countof(APIS);

    mOffsetsLookupCount = 0;
    mMaxOffsetsLookupCount = 0;

	// 32-bit
    {
		QString exePath = appPath;
#ifdef _DEBUG
		exePath.append("\\IGOProxyd.exe");
#else
		exePath.append("\\IGOProxy.exe");
#endif

        for (int idx = 0; idx < apiCount; ++idx)
        {
            OIGOffsetsLookupWorker* worker = OIGOffsetsLookupWorker::createWorker(appPath, exePath, APIS[idx]);
            ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished()), this, SLOT(onOIGOffsetsComputed()));
            worker->start();

            ++mMaxOffsetsLookupCount;
        }
    }

	// 64-bit
	Origin::Services::PlatformService::OriginProcessorArchitecture arch = Origin::Services::PlatformService::supportedArchitecturesOnThisMachine();
	if (arch & Origin::Services::PlatformService::ProcessorArchitecture64bit)
	{
		QString exePath = appPath;
#ifdef _DEBUG
		exePath.append("\\IGOProxy64d.exe");
#else
		exePath.append("\\IGOProxy64.exe");
#endif

        for (int idx = 0; idx < apiCount; ++idx)
        {
            OIGOffsetsLookupWorker* worker = OIGOffsetsLookupWorker::createWorker(appPath, exePath, APIS[idx]);
            ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished()), this, SLOT(onOIGOffsetsComputed()));
            worker->start();

            ++mMaxOffsetsLookupCount;
        }
	}
}
#endif

IGOWindowManager* IGOController::WindowManager() const
{
    return static_cast<IGOWindowManager*>(mWindowManager);
}

WebBrowserManager* IGOController::WBManager()
{
    if (!mWebBrowserManager)
        mWebBrowserManager = new WebBrowserManager(this, mUIFactory, WindowManager());
    
    return mWebBrowserManager;
}
    
SearchProfileViewController* IGOController::SPViewController(CallOrigin callOrigin)
{
    IIGOWindowManager::WindowProperties properties;
    properties.setCallOrigin(callOrigin);
    properties.setRestorable(true);
    
    IIGOWindowManager::WindowBehavior behavior;
    behavior.draggingSize = 40;
#ifdef ORIGIN_MAC
    behavior.nsBorderSize = 10;
    behavior.ewBorderSize = 10;
    behavior.nsMarginSize = 6;
    behavior.ewMarginSize = 6;
#else
    behavior.nsBorderSize = 20;
    behavior.ewBorderSize = 10;
#endif
    behavior.pinnable = true;

    ControllerFunctorNew<SearchProfileViewController, IGOController*> functor(this);
    SearchProfileViewController* controller = createController<SearchProfileViewController>(ControllerWindowType_NORMAL, functor, &properties, &behavior);
    return controller;
}

IErrorViewController* IGOController::ErrorController(ICallback<IErrorViewController>* callback, CallOrigin callOrigin)
{
    IIGOWindowManager::WindowProperties properties;
    properties.setCallOrigin(callOrigin);
    properties.setOpenAnimProps(defaultOpenAnimation());
    properties.setCloseAnimProps(defaultCloseAnimation());

    IIGOWindowManager::WindowBehavior behavior;

    ControllerFunctorFactoryNoArg<IErrorViewController> functor(&IIGOUIFactory::createError);
    IErrorViewController* error = createController<IErrorViewController>(ControllerWindowType_POPUP, functor, &properties, &behavior, callback);
    return error;
}

   
template<class T> T* IGOController::createController(enum ControllerWindowType winType, const IControllerFunctor<T>& functor, IIGOWindowManager::WindowProperties* settings, IIGOWindowManager::WindowBehavior* behavior, ICallback<T>* callback)
{
    if (!settings || !behavior)
        return NULL;
    
    T* controller = NULL;
    QWidget* window = WindowManager()->window(T::windowID());
    if (window)
        controller = static_cast<T*>(WindowManager()->windowController(window));
        
    if (!controller)
    {
        controller = functor();
        if (controller)
        {
            if (controller->ivcPreSetup(settings, behavior))
            {
                // If we have yet a callback to complete the setup, go for it
                if (callback)
                    (*callback)(controller);
                
                if (controller->ivcView())
                {
                    settings->setWId(T::windowID());
                    
                    behavior->controller = controller;
                    IIGOWindowManager::IScreenListener* listener = dynamic_cast<IIGOWindowManager::IScreenListener*>(controller);
                    if (listener)
                        behavior->screenListener = listener;

                    bool success = false;
                    if (winType == ControllerWindowType_NORMAL)
                        success = WindowManager()->addWindow(controller->ivcView(), *settings, *behavior);
                    
                    else
                        success = WindowManager()->addPopupWindow(controller->ivcView(), *settings, *behavior);
                    
                    if (success)
                    {
                        controller->ivcPostSetup();
                    }
                    
                    else
                    {
                        delete controller;
                        controller = NULL;
                    }
                }
                
                else
                {
                    delete controller;
                    controller = NULL;
                }
            }
        }
    }

    if (controller)
    {
        WindowManager()->moveWindowToFront(controller->ivcView());
        WindowManager()->setFocusWindow(controller->ivcView());
    }

    return controller;
}

void IGOController::igoHideUI()
{
    WindowManager()->setIGOVisible(false, NULL, IIGOCommandController::CallOrigin_CLIENT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IViewController::~IViewController() {}
IIGOUIFactory::~IIGOUIFactory() {}
    
} // Engine
} // Origin
