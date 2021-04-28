/////////////////////////////////////////////////////////////////////////////
// ClientViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "FriendSearchViewController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "originwindow.h"
#include "originwebview.h"
#include "OriginWebController.h"
#include "SocialJSHelper.h"
#include "engine/igo/IGOController.h"
#include <QWebView>
#include "SearchView.h"
#include "qdebug.h"

namespace Origin
{
    namespace Client
    {

        FriendSearchViewController::FriendSearchViewController(QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            ,mSearchView(NULL)
            ,mSearchViewOIG(NULL)
        {

        }

        FriendSearchViewController::~FriendSearchViewController()
        {
            if(mWindow)
            {
                delete mWindow;
                mWindow = NULL;
            }

            if(mIGOWindow)
            {
                delete mIGOWindow;
                mIGOWindow = NULL;
            }
        }

        UIToolkit::OriginWindow* FriendSearchViewController::createWindow(const UIScope& scope, SearchView* const wv)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }
            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::WebContent,
                QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, windowContext);
            myWindow->webview()->setHasSpinner(true);
            myWindow->webview()->setIsSelectable(false);
            myWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            myWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
            myWindow->webview()->setChangeLoadSizeAfterInitLoad(true);
            myWindow->webview()->setShowSpinnerAfterInitLoad(false);
            myWindow->webview()->setWebview(wv);
            myWindow->setTitleBarText(tr("ebisu_client_add_friend_title"));

            return myWindow;
        }

        namespace
        {
            QString addFriendsUrl()
            {
                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
                QString accessToken = Origin::Services::Session::SessionService::accessToken(session);
                QString dlgFriendSearchURL = Origin::Services::readSetting(Origin::Services::SETTING_DlgFriendSearchURL, session);
                QString s = dlgFriendSearchURL + "?authToken=" + accessToken;
                qDebug() << s;
                return dlgFriendSearchURL + "?authToken=" + accessToken;
            }
        }

        void FriendSearchViewController::show(const UIScope& scope, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (scope == ClientScope)
            {
                if(mWindow == NULL)
                {
                    mSearchView = new SearchView(scope);
                    mWindow = createWindow(scope, mSearchView);
                    mSearchView->loadTrustedUrl(addFriendsUrl());
                    mWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
                    mWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
                    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                }
                else
                {
                    mWindow->activateWindow();
                }
                mWindow->showWindow();

                //normally we don't want to call activateWindow because it would steal focus if this window all of a sudden popped up while the user was on another application.
                //however since these windows are only triggered from menu or origin:// its ok. If we don't activate the window, when using origin:// the window will show up minimized
                mWindow->activateWindow();
            }
            else if (scope == IGOScope && Engine::IGOController::instance() != NULL)
            {
                if (mIGOWindow == NULL)
                {
                    mSearchViewOIG = new SearchView(scope);
                    mIGOWindow = createWindow(scope, mSearchViewOIG);
                    mSearchViewOIG->loadTrustedUrl(addFriendsUrl());
                    mIGOWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
                    mIGOWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);


                    Engine::IIGOWindowManager::WindowProperties properties;
                    properties.setCallOrigin(callOrigin);
                    Engine::IIGOWindowManager::WindowBehavior behavior;

                    Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOWindow, properties, behavior);
                    ORIGIN_VERIFY_CONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                }
            }
        }

        void FriendSearchViewController::closeIGOWindow()
        {
            if (NULL!=mIGOWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void FriendSearchViewController::closeWindow()
        {
            if(NULL!=mWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->close();
                mWindow->deleteLater();
                mWindow = NULL;
                emit windowClosed();
            }
        }
    }
}
