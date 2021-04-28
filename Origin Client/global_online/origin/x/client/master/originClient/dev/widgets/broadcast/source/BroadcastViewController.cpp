/////////////////////////////////////////////////////////////////////////////
// BroadcastViewController.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "BroadcastViewController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "engine/igo/IGOController.h"
#include "engine/igo/IGOTwitch.h"
#include "engine/login/LoginController.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidget/WidgetView.h"
#include "WebWidgetController.h"
#include "ClientFlow.h"
#include "OriginToastManager.h"
#include "originwebview.h"
#include "UIScope.h"
#include "NativeBehaviorManager.h"
#include "OfflineCdnNetworkAccessManager.h"
#include "OriginWebController.h"
#include <QWebFrame>
#include <QNetworkRequest>

namespace Origin
{
namespace Client
{
#ifdef ORIGIN_PC
const QString BroadcastConnectLinkRole("http://widgets.dm.origin.com/linkroles/broadcastconnect");
const QString BroadcastSettingsLinkRole("http://widgets.dm.origin.com/linkroles/broadcastsettings");

BroadcastViewController::BroadcastViewController(QObject* parent, const UIScope& context, const bool& inDebug)
: QObject(parent)
, mContext(context)
, mInDebug(inDebug)
, mWindow(NULL)
, mWebView(NULL)
, mTwitchHelper(new TwitchJsHelper)
, mOIGPrevPosition(QPoint(-1, -1))
, mCurrentWindowOption(BROADCAST_NONE)
{
    ORIGIN_VERIFY_CONNECT(mTwitchHelper, SIGNAL(close(const QString&)), this, SLOT(onLoginWindowDone(const QString&)));
    Engine::IGOTwitch* twitch = Engine::IGOController::instance()->twitch();
    ORIGIN_VERIFY_CONNECT(twitch, SIGNAL(broadcastDialogOpen(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showSettings(Origin::Engine::IIGOCommandController::CallOrigin)));
    ORIGIN_VERIFY_CONNECT(twitch, SIGNAL(broadcastIntroOpen(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showIntro(Origin::Engine::IIGOCommandController::CallOrigin)));
    ORIGIN_VERIFY_CONNECT(twitch, SIGNAL(broadcastAccountLinkDialogOpen(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showTwitchLogin(Origin::Engine::IIGOCommandController::CallOrigin)));
    ORIGIN_VERIFY_CONNECT(twitch, SIGNAL(broadcastPermitted(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showNotSupportedError(Origin::Engine::IIGOCommandController::CallOrigin)));
    ORIGIN_VERIFY_CONNECT(twitch, SIGNAL(broadcastNetworkError(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showNetworkError(Origin::Engine::IIGOCommandController::CallOrigin)));
    ORIGIN_VERIFY_CONNECT(twitch, SIGNAL(broadcastStoppedError()), this, SLOT(showBroadcastStoppedError()));
}


BroadcastViewController::~BroadcastViewController()
{
    if(mTwitchHelper)
    {
        mTwitchHelper->deleteLater();
        mTwitchHelper = NULL;
    }
}


bool BroadcastViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    properties->setOpenAnimProps(Engine::IGOController::instance()->defaultOpenAnimation());
    properties->setCloseAnimProps(Engine::IGOController::instance()->defaultCloseAnimation());
    return true;
}


void BroadcastViewController::showIntro(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
    if(mCurrentWindowOption != BROADCAST_INTRO)
    {
        // What kind of window was open before/do we need to overwrite the calling origin?
        bool closeIGO = resetCloseIGOOnClosePropertyIfStartedFromSDK();
        if (closeIGO)
            callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;

        closeWindow();
    }

    else
    {
        if (startedFrom() == Origin::Engine::IIGOCommandController::CallOrigin_SDK)
            callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;
    }

    showWindow(callOrigin);
    mCurrentWindowOption = BROADCAST_INTRO;
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "oig", Engine::LoginController::instance()->currentUser(), BroadcastConnectLinkRole);
    else
        mWebView->loadLink(BroadcastConnectLinkRole);
}


void BroadcastViewController::showSettings(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
    if(mCurrentWindowOption != BROADCAST_SETTINGS)
    {
        // What kind of window was open before/do we need to overwrite the calling origin?
        bool closeIGO = resetCloseIGOOnClosePropertyIfStartedFromSDK();
        if (closeIGO)
            callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;

        closeWindow();
    }

    else
    {
        if (startedFrom() == Origin::Engine::IIGOCommandController::CallOrigin_SDK)
            callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;
    }

    showWindow(callOrigin);
    mCurrentWindowOption = BROADCAST_SETTINGS;
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "oig", Engine::LoginController::instance()->currentUser(), BroadcastSettingsLinkRole);
    else
        mWebView->loadLink(BroadcastSettingsLinkRole);
}


void BroadcastViewController::showTwitchLogin(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
    // What kind of window was open before/do we need to overwrite the calling origin?
    bool closeIGO = resetCloseIGOOnClosePropertyIfStartedFromSDK();
    if (closeIGO)
        callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;

    closeWindow();

    mCurrentWindowOption = BROADCAST_LOGIN;
    if(mWindow == NULL)
    {
        // show the web browser
        using namespace UIToolkit;
        // no minimize since it is modal
        mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::WebContent, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
        mWindow->setTitleBarText(tr("ebisu_client_twitch_broadcast_gameplay"));
        mWindow->webview()->setHasSpinner(true);
        // since it is a web page, it doesn't matter what size it is
        mWindow->webview()->setIsContentSize(false);
        mWindow->setContentMargin(0);
        mWindow->resize(818, 700);
        mWindow->webview()->setHScrollPolicy(Qt::ScrollBarAsNeeded);
        mWindow->webview()->setVScrollPolicy(Qt::ScrollBarAsNeeded);
        mWindow->webview()->setIsSelectable(false);
        OriginWebController* webController = new OriginWebController(mWindow->webview()->webview());

        Services::Session::SessionRef session = Services::Session::SessionService::currentSession();
        const bool isUserOnline = session ? Services::Connection::ConnectionStatesService::isUserOnline(session) : Services::Connection::ConnectionStatesService::isGlobalStateOnline();
        if (!isUserOnline)
        {
            mWindow->webview()->setIsContentSize(true);
            mWindow->adjustSize();
        }

        // pre-production
        // redirect_uri = http://stage.download.dm.origin.com/twitchTV/index.html
        // client_secret = none, we have not setup one yet!
        // client_id = none, we have not setup one yet!

        // redirect_uri has to match the client_id see application profile for Origin on twitch.com (user: Dutchman66 pwd: hlj987 - owner: van Veenendaal, Hans <hvveenendaal@ea.com> )
        //QUrl url("https://api.twitch.tv/kraken/oauth2/authorize?redirect_uri=http://stage.download.dm.origin.com/twitchTV/index.html&client_id=<none>&response_type=token&scope=user_read+channel_read+channel_editor+channel_commercial+sdk_broadcast+chat_login+metadata_events_edit");

        // production
        // client_id = coi1gbc524xzypbgnq1i9bwpj
        // client_secret = 41tt52p63dpgxwyc87mzjoizv
        // redirect_uri = http://download.dm.origin.com/twitchTV/index.html
        QUrl url("https://api.twitch.tv/kraken/oauth2/authorize?redirect_uri=http://download.dm.origin.com/twitchTV/index.html&client_id=coi1gbc524xzypbgnq1i9bwpj&response_type=token&scope=user_read+channel_read+channel_editor+channel_commercial+sdk_broadcast+chat_login+metadata_events_edit");

        QNetworkRequest request(url);
        webController->loadTrustedRequest(request);
        // webController->jsBridge()->addHelper(...) won't work because twitch is not a "trusted" hos 
        ORIGIN_VERIFY_CONNECT(mWindow->webview()->webview()->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));

        if(mContext == IGOScope)
        {
            // make the window modal because we need the callback to be active for the token registration
            Engine::IIGOWindowManager::WindowProperties properties;
            properties.setCallOrigin(callOrigin);
            properties.setPosition(QPoint(0, 0), static_cast<Engine::IIGOWindowManager::WindowDockingOrigin>(Engine::IIGOWindowManager::WindowDockingOrigin_X_CENTER | Engine::IIGOWindowManager::WindowDockingOrigin_Y_CENTER));
            properties.setOpenAnimProps(Engine::IGOController::instance()->defaultOpenAnimation());
            properties.setCloseAnimProps(Engine::IGOController::instance()->defaultCloseAnimation());

            Engine::IIGOWindowManager::WindowBehavior behavior;

            Engine::IGOController::instance()->igowm()->addPopupWindow(mWindow, properties, behavior);
        }
    }

    if(mContext == ClientScope)
    {
        mWindow->showWindow();
    }
    else
    {
        Engine::IGOController::instance()->igowm()->moveWindowToFront(mWindow);
    }
}


void BroadcastViewController::showNotSupportedError(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
    // What kind of window was open before/do we need to overwrite the calling origin?
    bool closeIGO = resetCloseIGOOnClosePropertyIfStartedFromSDK();
    if (closeIGO)
        callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;

    closeWindow();

    if(mContext == ClientScope)
    {
        mCurrentWindowOption = BROADCAST_ERROR;
        mWindow = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_client_unable_to_broadcast_caps"), tr("ebisu_client_sorry_broadcasting_not_supported"), tr("ebisu_client_ok_uppercase"));
        mWindow->setTitleBarText(tr("ebisu_client_twitch_broadcast_gameplay"));
        mWindow->showWindow();
    }
    else
    {
        Engine::IGOController::instance()->showError(tr("ebisu_client_unable_to_broadcast_caps"), tr("ebisu_client_sorry_broadcasting_not_supported"), "", NULL, "", callOrigin);
    }
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }
    otm->showToast("POPUP_BROADCAST_NOT_PERMITTED", tr("ebisu_client_twitch_permitted_broadcasting"), "");
}


void BroadcastViewController::showNetworkError(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
    // What kind of window was open before/do we need to overwrite the calling origin?
    bool closeIGO = resetCloseIGOOnClosePropertyIfStartedFromSDK();
    if (closeIGO)
        callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;

    closeWindow();

    if(mContext == ClientScope)
    {
        mCurrentWindowOption = BROADCAST_ERROR;
        mWindow = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_client_igo_video_broadcast"), tr("ebisu_client_network_service_unavailable_generic"), tr("ebisu_client_ok_uppercase"));
        mWindow->setTitleBarText(tr("ebisu_client_twitch_broadcast_gameplay"));
        mWindow->showWindow();
    }
    else
    {
        Engine::IGOController::instance()->showError(tr("ebisu_client_igo_video_broadcast"), tr("ebisu_client_network_service_unavailable_generic"), "", NULL, "", callOrigin);
    }
}


void BroadcastViewController::showBroadcastStoppedError()
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR << "ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR << "OriginToastManager not available";
        return;
    }
    otm->showToast("POPUP_BROADCAST_STOPPED", tr("ebisu_client_twitch_stopped_broadcasting"), "");
}


void BroadcastViewController::populatePageJavaScriptWindowObject()
{
    QWebFrame* mainFrame = dynamic_cast<QWebFrame*>(sender());
    if (mainFrame)
    {
        mainFrame->addToJavaScriptWindowObject("twitchHelper", mTwitchHelper);
    }
}


void BroadcastViewController::showWindow(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
{
    if(mWindow == NULL)
    {
        using namespace UIToolkit;
        mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::WebContent,
            QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
        mWindow->setObjectName("BroadcastSettings");
        mWindow->setTitleBarText(tr("ebisu_client_twitch_broadcast_gameplay"));
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));

        //gets deleted by OriginWebView
        mWebView = new WebWidget::WidgetView();
        mWebView->widgetPage()->setExternalNetworkAccessManager(new Client::OfflineCdnNetworkAccessManager(mWebView->page()));
        ORIGIN_VERIFY_CONNECT(mWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));
        // Act and look more native
        NativeBehaviorManager::applyNativeBehavior(mWebView);

        OriginWebView* webContainer = mWindow->webview();
        webContainer->setShowSpinnerAfterInitLoad(false);
        webContainer->setHasSpinner(true);
        webContainer->setChangeLoadSizeAfterInitLoad(false);
        webContainer->setWebview(mWebView);
        webContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        webContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);

        if(mContext == IGOScope)
        {
            Engine::IIGOWindowManager::WindowProperties properties;
            properties.setCallOrigin(callOrigin);
            properties.setOpenAnimProps(Engine::IGOController::instance()->defaultOpenAnimation());
            properties.setCloseAnimProps(Engine::IGOController::instance()->defaultCloseAnimation());
            properties.setRestorable(true);
            if(mOIGPrevPosition == QPoint(-1,-1))
            { 
                properties.setPosition(QPoint(0, 0), static_cast<Engine::IIGOWindowManager::WindowDockingOrigin>(Engine::IIGOWindowManager::WindowDockingOrigin_X_CENTER | Engine::IIGOWindowManager::WindowDockingOrigin_Y_CENTER));
            }
            else
            {
                properties.setPosition(mOIGPrevPosition);
            }

            Engine::IIGOWindowManager::WindowBehavior behavior;
            behavior.pinnable = true;
            behavior.disableResizing = true;

            if (Engine::IGOController::instance()->showWebInspectors())
                Engine::IGOController::instance()->showWebInspector(mWebView->page());

            Engine::IGOController::instance()->igowm()->addWindow(mWindow, properties, behavior);
        }
        else
        {
            mWindow->showWindow();
        }
    }

    else
    {
        // Quite unfortunate, but because of the crazyness of the signal/slot flow
        if (callOrigin == Origin::Engine::IIGOCommandController::CallOrigin_SDK)
        {
            Engine::IIGOWindowManager::WindowProperties properties;
            properties.setCallOrigin(callOrigin);
            Engine::IGOController::instance()->igowm()->setWindowProperties(mWindow, properties);

            Engine::IIGOWindowManager::WindowEditableBehaviors resetBehaviors;
            resetBehaviors.setCloseIGOOnClose(true);
            Engine::IGOController::instance()->igowm()->setWindowEditableBehaviors(mWindow, resetBehaviors);
        }
    }

    if(mContext == ClientScope)
    {
        mWindow->showWindow();
    }
    else
    {
        Engine::IGOController::instance()->igowm()->moveWindowToFront(mWindow);
    }
}

bool BroadcastViewController::resetCloseIGOOnClosePropertyIfStartedFromSDK() const
{
    if (startedFrom() == Origin::Engine::IIGOCommandController::CallOrigin_SDK)
    {
        Engine::IIGOWindowManager::WindowEditableBehaviors resetBehaviors;
        resetBehaviors.setCloseIGOOnClose(false);
        Engine::IGOController::instance()->igowm()->setWindowEditableBehaviors(mWindow, resetBehaviors);

        return true;
    }

    return false;
}

void BroadcastViewController::closeWindow()
{
    if(mWindow && !mInDebug)
    {
        if (mContext == IGOScope)
        {
            Engine::IIGOWindowManager::WindowProperties properties;
            if (Engine::IGOController::instance()->igowm()->windowProperties(mWindow, &properties))
                mOIGPrevPosition = properties.position();
        }

        ORIGIN_VERIFY_DISCONNECT(mWindow->webview()->webview()->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));
        mWindow->deleteLater();
        mWindow = NULL;
        mWebView->deleteLater();
        mWebView = NULL;
        if(Engine::IGOController::instance()->twitch())
        {
            Engine::IGOController::instance()->twitch()->broadcastDialogClosed();
        }
    }
}


void BroadcastViewController::onLoginWindowDone(const QString& token)
{    
    // What kind of window was open before/do we need to overwrite the calling origin?
    Origin::Engine::IIGOCommandController::CallOrigin callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_CLIENT;
    bool closeIGO = resetCloseIGOOnClosePropertyIfStartedFromSDK();
    if (closeIGO)
        callOrigin = Origin::Engine::IIGOCommandController::CallOrigin_SDK;

    closeWindow();

    Engine::IGOController::instance()->twitch()->storeTwitchLogin(token, callOrigin);
}


QPoint BroadcastViewController::defaultPosition(uint32_t width, uint32_t height)
{
    QPoint pos = Engine::IGOController::instance()->igowm()->centeredPosition(mWindow->size());
    if (pos.y() < 10)
        pos.setY(10);

    return pos;
}

Origin::Engine::IIGOCommandController::CallOrigin BroadcastViewController::startedFrom() const
{
    if (mWindow && !mInDebug)
    {
        if (mContext == IGOScope)
        {
            Engine::IIGOWindowManager::WindowProperties properties;
            if (Engine::IGOController::instance()->igowm()->windowProperties(mWindow, &properties))
            {
                if (properties.callOriginDefined())
                    return properties.callOrigin();
            }
        }
    }

    return Origin::Engine::IIGOCommandController::CallOrigin_CLIENT;
}

void BroadcastViewController::onSizeChanged(uint32_t width, uint32_t height)
{
    Engine::IGOController::instance()->igowm()->setWindowPosition(mWindow, defaultPosition(width, height));
}
#endif
}
}