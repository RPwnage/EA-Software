/////////////////////////////////////////////////////////////////////////////
// ChangeAvatarViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ChangeAvatarViewController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "originwindow.h"
#include "originwebview.h"
#include "OriginWebController.h"
#include "CommonJsHelper.h"
#include "ChooseAvatarJsHelper.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"

#include <QWebFrame>

namespace Origin
{
namespace Client
{

ChangeAvatarViewController::ChangeAvatarViewController(QObject* parent)
: QObject(parent)
, mAvatarWindow(NULL)
, mIGOAvatarWindow(NULL)
{

}

ChangeAvatarViewController::~ChangeAvatarViewController()
{
    closeAvatarWindow();
}

QUrl avatarUrl(const UIScope& scope)
{
    QUrl url(Origin::Services::readSetting(Origin::Services::SETTING_NewUserExpURL));
    QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE).toString();
    QUrlQuery query;
    query.addQueryItem("gameId", "ebisu"); 
    query.addQueryItem("locale", locale); 
    query.addQueryItem("sourceType", scope == ClientScope ? "client" : "oig");
    query.addQueryItem("onlyAvatar", "true");
    url.setQuery(query);
    ORIGIN_LOG_EVENT << "Avatar URL: " << url.toString();
    return url;
}

void ChangeAvatarViewController::show(UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin)
{
    if (scope == ClientScope)
    {
        if(mAvatarWindow == NULL)
        {
            using namespace UIToolkit;
            mAvatarWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, NULL, OriginWindow::WebContent);
            mAvatarWindow->webview()->setHasSpinner(true);
            mAvatarWindow->setTitleBarText(tr("ebisu_client_change_avatar"));
            ORIGIN_VERIFY_CONNECT(mAvatarWindow, SIGNAL(rejected()), this, SLOT(closeAvatarWindow()));

            OriginWebController* webController = new OriginWebController(mAvatarWindow->webview()->webview());
            CommonJsHelper* commonJsHelper = new CommonJsHelper(mAvatarWindow->webview());
            webController->jsBridge()->addHelper("commonHelper", commonJsHelper);
            ORIGIN_VERIFY_CONNECT(commonJsHelper, SIGNAL(closeWebWindow()), this, SLOT(closeAvatarWindow()));

            ChooseAvatarJsHelper* chooseAvatarHelper = new ChooseAvatarJsHelper(mAvatarWindow->webview());
            ORIGIN_VERIFY_CONNECT(chooseAvatarHelper, SIGNAL(updateAvatarRequested(QString)), this, SLOT(closeAvatarWindow()));
            ORIGIN_VERIFY_CONNECT(chooseAvatarHelper, SIGNAL(cancelled()), this, SLOT(closeAvatarWindow()));
            webController->jsBridge()->addHelper("chooseAvatarHelper", chooseAvatarHelper);

            QUrl url = avatarUrl(scope);
             QNetworkRequest request(url);
            webController->loadTrustedRequest(request);

            mAvatarWindow->webview()->setIsSelectable(false);
            mAvatarWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            mAvatarWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        }
        else
        {
            mAvatarWindow->activateWindow();
        }
        mAvatarWindow->showWindow();
    }
    else if (scope == IGOScope && Engine::IGOController::instance() != NULL)
    {
        if(mIGOAvatarWindow == NULL)
        {
            using namespace UIToolkit;
            mIGOAvatarWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::WebContent);
            mIGOAvatarWindow->webview()->setHasSpinner(true);
            mIGOAvatarWindow->setTitleBarText(tr("ebisu_client_change_avatar"));
            ORIGIN_VERIFY_CONNECT(mIGOAvatarWindow, SIGNAL(rejected()), this, SLOT(closeIGOAvatarWindow()));

            OriginWebController* webController = new OriginWebController(mIGOAvatarWindow->webview()->webview());
            CommonJsHelper* commonJsHelper = new CommonJsHelper(mIGOAvatarWindow->webview());
            webController->jsBridge()->addHelper("commonHelper", commonJsHelper);
            ORIGIN_VERIFY_CONNECT(commonJsHelper, SIGNAL(closeWebWindow()), this, SLOT(closeIGOAvatarWindow()));

            ChooseAvatarJsHelper* chooseAvatarHelper = new ChooseAvatarJsHelper(mIGOAvatarWindow->webview());
            ORIGIN_VERIFY_CONNECT(chooseAvatarHelper, SIGNAL(updateAvatarRequested(QString)), this, SLOT(closeIGOAvatarWindow()));
            ORIGIN_VERIFY_CONNECT(chooseAvatarHelper, SIGNAL(cancelled()), this, SLOT(closeIGOAvatarWindow()));
            webController->jsBridge()->addHelper("chooseAvatarHelper", chooseAvatarHelper);
            
            QUrl url = avatarUrl(scope);

            // Add our auth token
//             QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
//             url.addQueryItem("token", accessToken);

            QNetworkRequest request(url);
            QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE);
            request.setRawHeader("localeinfo", locale.toLocal8Bit());

            // For now, we need to disable the "Browse" button from the javascript (coming from EADP) which would open a native browser window
            // (ie outside OIG) - we need EADP to fix this/add support for a file browser in OIG
            ORIGIN_VERIFY_CONNECT(webController->page()->mainFrame(), SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinishedInIGO(bool)));
            webController->loadTrustedRequest(request);

            mIGOAvatarWindow->webview()->setIsSelectable(false);
            mIGOAvatarWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            mIGOAvatarWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);

            Engine::IIGOWindowManager::WindowProperties properties;
            properties.setCallOrigin(callOrigin);

            Engine::IIGOWindowManager::WindowBehavior behavior;

            Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOAvatarWindow, properties, behavior);
        }
    }
}

void ChangeAvatarViewController::onLoadFinishedInIGO(bool ok)
{
    // Look up for the "Browse" button on the Edit Avatar page and disable it until the EADP script is updated to use a client proxy and
    // open an OIG file browser (which doesn't exist yet!)
    QWebFrame* frame = dynamic_cast<QWebFrame*>(sender());
    if (frame)
    {
        // Right now, the user case that this temporary fix applies to would directly open the 'Edit Avatar' page. However if at some point we
        // make this available from another page (like the preferences), the 'Edit Avatar' page would be an iframe -> we would need to update
        // the script appropriately.
        QString code = "document.getElementById('fakeFileupload').disabled=true";
        frame->evaluateJavaScript(code);
    }
}
    
void ChangeAvatarViewController::closeIGOAvatarWindow()
{
    if (mIGOAvatarWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mIGOAvatarWindow, SIGNAL(rejected()), this, SLOT(closeIGOAvatarWindow()));
        mIGOAvatarWindow->close();
        mIGOAvatarWindow = NULL;
    }
}

void ChangeAvatarViewController::closeAvatarWindow()
{
    if(mAvatarWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mAvatarWindow, SIGNAL(rejected()), this, SLOT(closeAvatarWindow()));
        mAvatarWindow->close();
        mAvatarWindow->deleteLater();
        mAvatarWindow = NULL;
        emit windowClosed();
    }
}
}
}
