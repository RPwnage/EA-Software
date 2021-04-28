/////////////////////////////////////////////////////////////////////////////
// PDLCView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PDLCView.h"
#include "PDLCJsHelper.h"
#include "CommonJsHelper.h"
#include "engine/content/ContentController.h"
#include <engine/content/ContentConfiguration.h>
#include "services/qt/QtUtil.h"
#include "services/session/SessionService.h"
#include "store/source/StoreUrlBuilder.h"
#include "TelemetryAPIDLL.h"

#include "OriginWebController.h"
#include "WebWidget/FeatureRequest.h"
#include "StoreWebPage.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebHistory>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QtNetwork>
#include <QLocale>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWebKitWidgets/Qwebpage>
#include <qaction.h>

namespace
{
    static const QSize WINDOW_LOADING_SIZE = QSize(500,590);
    static const QSize WINDOW_REAL_CURRENCY_SIZE = QSize(1000,590);
    static const QSize WINDOW_MARGIN = QSize(16,45);
    static const QByteArray CURRENCY_TYPE_HEADER = "X-Currency-Type";
    static const QString CURRENCY_TYPE_REAL = "REAL";
}

namespace Origin
{
    namespace Client
    {
        PDLCView::PDLCView(bool inIGO, QWidget* parent)
            : OriginWebView(parent)
            , mWebController(NULL)
            , mHelper(NULL)
        {
            // Set a page to allow popups
            StoreWebPage *page = new StoreWebPage(this);
            page->setUseWhiteList(true);
            this->webview()->setPage(page);

            // set browser settings
            this->webview()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
            this->webview()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
            this->webview()->settings()->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
            this->webview()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
            this->webview()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
            setChangeLoadSizeAfterInitLoad(true);
            setShowSpinnerAfterInitLoad(false);
            setIsContentSize(true);
            setHasSpinner(true);
            setScaleSizeOnContentsSizeChanged(true);
            setPreferredContentsSize(WINDOW_LOADING_SIZE);
            setWindowLoadingSize(WINDOW_LOADING_SIZE);
            mWebController = new OriginWebController(webview(), webview()->page(), DefaultErrorPage::centerAlignedPage());
            mHelper = new PDLCJsHelper(this);
            mHelper->setContext(inIGO ? "IGO" : "APP");
            mWebController->jsBridge()->addHelper("igoStoreHelper", mHelper);    // Remove this line when the store is updated.
            mWebController->jsBridge()->addHelper("storeHelper", mHelper);
            mWebController->jsBridge()->addHelper("clientNavigation", mHelper);    // Used for links in add-on descriptions (named clientNavigation so that the links will work in the game details page too).
            mWebController->jsBridge()->addHelper("commonHelper", new CommonJsHelper(this));

            WebWidget::FeatureRequest clientNavigation("http://widgets.dm.origin.com/features/interfaces/clientNavigation");
            mWebController->jsBridge()->addFeatureRequest(clientNavigation);

            WebWidget::FeatureRequest entitlementManager("http://widgets.dm.origin.com/features/interfaces/entitlementManager");
            mWebController->jsBridge()->addFeatureRequest(entitlementManager);

            webview()->history()->clear();

            webview()->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
            webview()->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

            // Allow the java script to close the whole widget.
            ORIGIN_VERIFY_CONNECT_EX(mHelper, SIGNAL(closeStore()), this, SIGNAL(closeStore()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT(this->webview(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));
            ORIGIN_VERIFY_CONNECT(this->webview()->page()->mainFrame(), &QWebFrame::contentsSizeChanged, this, &PDLCView::onContentsSizeChanged);
            ORIGIN_VERIFY_CONNECT(this->webview()->page()->networkAccessManager(), &QNetworkAccessManager::finished, this, &PDLCView::onRequestFinished);
            ORIGIN_VERIFY_CONNECT(Services::Session::SessionService::instance(), SIGNAL(sessionTokensChanged(Origin::Services::Session::SessionRef, Origin::Services::Session::AccessToken, Origin::Services::Session::RefreshToken, Origin::Services::Session::SessionError)), this, SLOT(updateCemLoginCookie()));
            
            // IGO doesn't support modal dialogs
            if(!inIGO)
            {
                ORIGIN_VERIFY_CONNECT_EX(mHelper, SIGNAL(printRequested()), this, SLOT(onPrintRequested()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(webview()->page(), SIGNAL(printRequested(QWebFrame*)), this, SLOT(onPrintRequested()), Qt::QueuedConnection);
            }

            updateCemLoginCookie();
        }

        PDLCView::~PDLCView()
        {
            if (mWebController)
            {
                QObject::disconnect(mWebController);
                mWebController = NULL;
            }
        }

        void PDLCView::navigate()
        {
            using Services::Session::SessionService;
            setUrl(SessionService::nucleusUser(SessionService::currentSession()), mMasterTitleId, mCategoryIds, mOfferIds);
        }

        void PDLCView::purchase(QString productId)
        {
            mProductId = productId;
            if( mHelper )
            {
                mHelper->purchase(productId);
            }
        }

        void PDLCView::setUrl(const QString& gamerId, const QString& masterTitleId, const QString& categoryIds, const QString &offerIds)
        {
            // Check categoryIds.isEmpty instead of !masterTitleId.isEmpty because masterTitleId is non-empty when viewing add-on store from the GDP in EC1.
            // CategoryIds are always populated in EC1 and always empty in EC2, except when viewing ME3 IGO PDLC store where we'd want V1 anyway.
            if(categoryIds.isEmpty())
            {
                QString strUrl = Services::readSetting(Services::SETTING_PdlcStoreURL);

                // TODO: move this logic to a StoreUrlBuilder::pdlcStoreUrl function once EC2/new store support virtual currency.
                strUrl.replace("{masterTitleId}", masterTitleId);

                QUrl url(strUrl);
                QUrlQuery urlQuery(url);
                
                if(!mHelper->commerceProfile().isEmpty())
                {
                    urlQuery.addQueryItem("profile", mHelper->commerceProfile());
                    urlQuery.addQueryItem("intcmp", QString("ORIGIN-IGO-") + mHelper->commerceProfile().toUpper());
                }

                if(!offerIds.isEmpty())
                    urlQuery.addQueryItem("productIds", offerIds);

                // HACK: we're now relying on Lockbox to provide the scrollbar and sizing, but the real
                // currency add-on store doesn't
                setIsContentSize(false);
                setVScrollPolicy(Qt::ScrollBarAsNeeded);
                setMinimumSize(WINDOW_REAL_CURRENCY_SIZE);
                if (topLevelWidget())
                    topLevelWidget()->setFixedSize(WINDOW_REAL_CURRENCY_SIZE + WINDOW_MARGIN);

                url.setQuery(urlQuery);
                setUrl(url);
            }
            else
            {
                QUrl url(Services::readSetting(Services::SETTING_PdlcStoreLegacyURL));
                QUrlQuery urlQuery(url);
                
                urlQuery.addQueryItem("userId", gamerId);

                setVScrollPolicy(Qt::ScrollBarAsNeeded);

                if(!categoryIds.isEmpty())
                    urlQuery.addQueryItem("categoryId", categoryIds);
                if(!offerIds.isEmpty())
                    urlQuery.addQueryItem("productIds", offerIds);
                if(!masterTitleId.isEmpty()) 
                    urlQuery.addQueryItem("masterTitleId", masterTitleId);

                url.setQuery(urlQuery);
                setUrl(url);
            }
        }

        void PDLCView::setUrl(const QUrl& url)
        {
            QNetworkRequest req(url);
            req.setRawHeader("AuthToken", Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession()).toUtf8());

            mWebController->loadTrustedRequest(req);

            GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
        }

        void PDLCView::setEntitlement(const Engine::Content::EntitlementRef entitlement)
        {
            if(mHelper)
            {
                if(!entitlement.isNull())
                {
                    mHelper->setCommerceProfile(!entitlement->contentConfiguration()->commerceProfile().isEmpty() ? entitlement->contentConfiguration()->commerceProfile() : "oig-real"); 
                    mHelper->setSku(!entitlement->contentConfiguration()->contentId().isEmpty() ? entitlement->contentConfiguration()->contentId() : "unknown");
                    setUrlMasterTitleId(!entitlement->contentConfiguration()->masterTitleId().isEmpty() ? entitlement->contentConfiguration()->masterTitleId() : "unknown");
                }
                else
                {
                    mHelper->setCommerceProfile("oig-real");
                    mHelper->setSku("unknown");
                    setUrlMasterTitleId("unknown");
                }
            }
        }

        void PDLCView::onUrlChanged(const QUrl& url)
        {
            // This is an 8.5.1 hotfix hack to allow scrollbars to appear in paypal.com pages during checkout flow
            // See https://developer.origin.com/support/browse/EBIBUGS-15397
            if(url.toString().contains("paypal.com") && isContentSize() &&
                (this->webview()->page()->mainFrame()->scrollBarPolicy(Qt::Vertical) != Qt::ScrollBarAsNeeded ||
                this->webview()->page()->mainFrame()->scrollBarPolicy(Qt::Horizontal) != Qt::ScrollBarAsNeeded))
            {
                //we set the error checking to false here because the stop call below will trigger the error handling, prevent the loading of paypal page
                //however we need the stop and the scrollbar setting so that the create account page in paypal has scroll bars
                mWebController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);

                this->webview()->stop();
                this->webview()->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAsNeeded);
                this->webview()->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAsNeeded);
                this->webview()->load(url);
            }

            // Hack to explicitly hide scroll bar on LB2 checkout pages.
            if (url.toString().contains("checkout") && url.toString().contains("lockbox-ui") &&
                vScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
            {
                setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            }

            GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
        }

        void PDLCView::onContentsSizeChanged(const QSize& size)
        {
            // This code is needed so that lightboxes can grow and shrink with the dialog.
            setPreferredContentsSize(size);
            setWindowLoadingSize(size);
        }

        void PDLCView::onRequestFinished(QNetworkReply *reply)
        {
            if (!reply || !reply->hasRawHeader(CURRENCY_TYPE_HEADER))
            {
                return;
            }

            // Re-center the dialog since it's going to grow.
            const QString& currencyType = reply->rawHeader(CURRENCY_TYPE_HEADER);
            if (currencyType.compare(CURRENCY_TYPE_REAL, Qt::CaseInsensitive) == 0)
            {
                setIsContentSize(true);
                setVScrollPolicy(Qt::ScrollBarAlwaysOff);

                // TODO: Figure out how to get this from the page before it is done loading
                resize(WINDOW_REAL_CURRENCY_SIZE);
                setMaximumSize(WINDOW_REAL_CURRENCY_SIZE);
                if (topLevelWidget())
                    topLevelWidget()->setFixedSize(WINDOW_REAL_CURRENCY_SIZE + WINDOW_MARGIN);

                emit sizeChanged(WINDOW_REAL_CURRENCY_SIZE + WINDOW_MARGIN);
            }
        }

        void PDLCView::updateCemLoginCookie()
        {
            QString token = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
            // Update cookie
            QUrl cookieDomain;
            // Do not need to set this cookie for new store due to oauth2 magic.
            QString storeHost = QUrl(Services::readSetting(Services::SETTING_PdlcStoreLegacyURL)).host();
            QNetworkCookie loginCookie;
            QList<QNetworkCookie> cookies;

            // Emulate setting this from the store's domain
            cookieDomain.setScheme("http");
            cookieDomain.setHost(storeHost);
            cookieDomain.setPath("/");

            // This emulates basically what ea.com does
            loginCookie.setDomain(storeHost.prepend("."));
            loginCookie.setHttpOnly(true);
            loginCookie.setSecure(true);
            loginCookie.setName("CEM-login");
            loginCookie.setValue(token.toLatin1());
            loginCookie.setPath("/");
            cookies.append(loginCookie);

            webview()->page()->networkAccessManager()->cookieJar()->setCookiesFromUrl(cookies, cookieDomain);
        }

        void PDLCView::onPrintRequested()
        {
            QPrinter printer;
            printer.setOutputFormat(QPrinter::NativeFormat);
            QPrintDialog printDialog(&printer, this);
            if (printDialog.exec() == QDialog::Accepted)
            {
                webview()->print(printDialog.printer());
            }
        }

    }
}
