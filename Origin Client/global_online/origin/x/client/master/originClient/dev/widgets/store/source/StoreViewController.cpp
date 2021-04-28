/////////////////////////////////////////////////////////////////////////////
// StoreViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "StoreViewController.h"
#include "StoreView.h"
#include "StoreUrlBuilder.h"

#include "services/qt/QtUtil.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "TelemetryAPIDLL.h"
#include "NavigationController.h"
#include "services/log/LogService.h"
#include "originwebview.h"

#include <QtNetwork>
#include <QNetworkReply>
#include <QtWebKitWidgets/QWebView>

namespace Origin
{
    namespace Client
    {

        using Origin::Services::Session::SessionService;

        StoreViewController::StoreViewController(QWidget *parent, bool isMainStoreTab)
            : QObject(parent)
            , mWebViewContainer(NULL)
            , mIsMainStoreTab(isMainStoreTab)
        {
            mWebViewContainer = new UIToolkit::OriginWebView(parent);
            mStoreView = new StoreView(mWebViewContainer);
            // Give the web view a gray background by default
            mWebViewContainer->setPalette(QPalette(QColor(238,238,238)));
            mWebViewContainer->setAutoFillBackground(true);
            mWebViewContainer->setIsContentSize(false);
            mWebViewContainer->setShowSpinnerAfterInitLoad(false);
            mWebViewContainer->setShowAfterInitialLayout(true);
            mWebViewContainer->setHasSpinner(true);
#ifdef ORIGIN_MAC
            // OriginWebView is not fully supported with graphics web view yet. OriginWebView::setGraphicsWebview was
            // created for this instance - to show the loading spinner while the web page was loading.
            mWebViewContainer->setGraphicsWebview(mStoreView, mStoreView->page());
#else
            mWebViewContainer->setWebview(mStoreView);
#endif

            mStoreUrlBuilder = new StoreUrlBuilder();

            loadPersistentCookie("hl", Services::SETTING_StoreLocale);

            if(mIsMainStoreTab)
            {
                ORIGIN_VERIFY_CONNECT(mStoreView->page()->currentFrame(), SIGNAL(urlChanged(const QUrl &)), this, SLOT(storeUrlChanged(const QUrl &)));
                ORIGIN_VERIFY_CONNECT(mStoreView->page()->currentFrame(), SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
                NavigationController::instance()->addWebHistory(STORE_TAB, mStoreView->page()->history());
            }

            QStringList hosts;
            hosts << ".origin.com";
            hosts << ".ea.com";
            mAuthenticationMonitor.setRelevantHosts(hosts);
            mAuthenticationMonitor.setWebPage(mStoreView->page());
            ORIGIN_VERIFY_CONNECT(&mAuthenticationMonitor, SIGNAL(lostAuthentication()), this, SLOT(onLostAuthentication()));
            mAuthenticationMonitor.start();
        }

        void StoreViewController::onLoadFinished(bool isOk)
        {
            NavigationController::instance()->setEnabled(isOk, STORE_TAB);

            // EBIBUGS-28394: Qt WebKit seems to be buggy with respect to fragments, so we must manually handle it ourselves.
            if(mStoreView)
            {
                const QString& fragment = mStoreView->page()->mainFrame()->requestedUrl().fragment();
                if(!fragment.isEmpty())
                {
                    mStoreView->page()->mainFrame()->scrollToAnchor(fragment);
                }
            }
        }

        StoreViewController::~StoreViewController()
        {
            persistCookie("hl", Services::SETTING_StoreLocale);
            mAuthenticationMonitor.shutdown();
        }

        void StoreViewController::loadEntryUrlAndExpose(const QUrl &url)
        {
            ORIGIN_LOG_DEBUG << "[information] Store URL expose: " << url.toString();
            mStoreView->loadEntryUrlAndExpose(url);
        }
        
        void StoreViewController::loadStoreUrl(const QUrl &url)
        {
            ORIGIN_LOG_DEBUG << "[information] Store URL: " << url.toString();
            mStoreView->loadStoreUrl(url);
        }

        void StoreViewController::storeUrlChanged(const QUrl& url)
        {
            ORIGIN_LOG_DEBUG << "[information] Store URL changed: " << url.toString();

            GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
            NavigationController::instance()->recordNavigation(STORE_TAB, url.toString());
        }

        void StoreViewController::onLostAuthentication()
        {
            ORIGIN_LOG_ERROR << "Store lost authentication";
            mStoreView->page()->triggerAction(QWebPage::Stop);
            emit lostAuthentication();
        }

        QWidget *StoreViewController::view() const
        {
            return mWebViewContainer;
        }

        StoreView *StoreViewController::storeView() const
        {
            return mStoreView;
        }

        void StoreViewController::loadPersistentCookie(const QByteArray& cookieName, const Services::Setting& settingKey)
        {
            QString settingValue = Services::readSetting(settingKey).toString();
            if (!settingValue.isEmpty())
            {
                QNetworkCookie cookie(cookieName, settingValue.toLatin1());
                
                QList<QNetworkCookie> cookieList;
                cookieList.push_back(cookie);

                QUrl url;
                url.setHost(mStoreUrlBuilder->homeUrl().host());
                Services::NetworkAccessManagerFactory::instance()->sharedCookieJar()->setCookiesFromUrl(cookieList, url);
            }
        }

        void StoreViewController::persistCookie(const QByteArray& name, const Services::Setting& settingKey)
        {
            QNetworkCookie c;
            if (Services::NetworkAccessManagerFactory::instance()->sharedCookieJar()->findFirstCookieByName(name, c))
            {
                Services::writeSetting(settingKey, QString(c.value()));
            }
            else
            {
                // Clear the persisted value since no 'hl' cookie exists.
                Services::writeSetting(settingKey, "");
            }
        }
    }
}
