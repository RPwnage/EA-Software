/////////////////////////////////////////////////////////////////////////////
// PDLCViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PDLCViewController.h"
#include "PDLCView.h"

#include "Qt/originwindow.h"
#include "flows/ClientFlow.h"
#include <engine/content/ContentController.h>
#include <engine/content/ContentConfiguration.h>
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "StoreUrlBuilder.h"

#include <QtWebKitWidgets/QWebView>

namespace Origin
{
namespace Client
{
PDLCViewController::PDLCViewController(QWidget* parent)
: QObject(parent)
, mPdlcWindow(NULL)
, mPdlcView(NULL)
{
}

QUrl PDLCViewController::productUrl(const QString& productId, const QString& masterTitleId)
{
    StoreUrlBuilder builder;
    return builder.productUrl(productId, masterTitleId);
}
    
void PDLCViewController::show(const Engine::Content::EntitlementRef ref)
{
    if (!ref.isNull())
    {
        const bool inIgo = Engine::IGOController::instance()->isActive();
        bool reload = false;

        if(mPdlcWindow == NULL || mPdlcView == NULL)
        {
            createPdlcView(inIgo);
            reload = true;
        }
        else if(mPdlcWindow->isHidden())
        {
            reload = true;
        }
        else if(mPdlcView->urlMasterTitleId() != ref->contentConfiguration()->masterTitleId())
        {
            reload = true;
        }
        
        if(reload)
        {
            mPdlcView->setEntitlement(ref);
            mPdlcView->setUrlOfferIds("");
            mPdlcView->navigate();
        }

        mPdlcWindow->showWindow();
    }
}

void PDLCViewController::purchase(const Engine::Content::EntitlementRef ref)
{
    if (!ref.isNull())
    {
        // Fix to satisfy legal requirements...
        // When purchasing FullGamePlusExpansions, we need to show the PDP page with all the legal text.
        // Show the productId PDP page in the store.
        if (ref->contentConfiguration()->isBaseGame())
        {
            StoreUrlBuilder builder;
            ClientFlow::instance()->showUrlInStore(builder.productUrl(ref->contentConfiguration()->productId()));
            return;
        }

        const bool inIgo = Engine::IGOController::instance()->isActive();
        bool reload = false;

	    if(mPdlcWindow == NULL || mPdlcView == NULL)
	    {
            createPdlcView(inIgo);
            reload = true;
	    }
	    else if(mPdlcWindow->isHidden() || mPdlcView->productId() != ref->contentConfiguration()->productId())
	    {
            reload = true;
	    }

        if(reload)
        {
            mPdlcView->setEntitlement(ref->parent());
		    mPdlcView->setUrlCategoryIds("");
		    mPdlcView->setUrlOfferIds("");
            mPdlcView->setUrlMasterTitleId("");
            mPdlcView->purchase(ref->contentConfiguration()->productId());
        }

        mPdlcWindow->showWindow();
    }
}

void PDLCViewController::showInIGOByCategoryIds(const QString& contentId, const QString& categoryIds, const QString& offerIds)
{
    mPdlcView->setEntitlement(Origin::Engine::Content::ContentController::currentUserContentController()->entitlementById(contentId));
    mPdlcView->setUrlCategoryIds(categoryIds);
    mPdlcView->setUrlOfferIds(offerIds);
    mPdlcView->setUrlMasterTitleId("");
    mPdlcView->navigate();
}

void PDLCViewController::showInIGOByMasterTitleId(const QString& contentId, const QString& masterTitleId, const QString& offerIds)
{
    mPdlcView->setEntitlement(Origin::Engine::Content::ContentController::currentUserContentController()->entitlementById(contentId));
	mPdlcView->setUrlCategoryIds("");
	mPdlcView->setUrlOfferIds(offerIds);
	mPdlcView->setUrlMasterTitleId(masterTitleId);
    mPdlcView->navigate();
}

void PDLCViewController::showInIGO(const QString& url)
{
    mPdlcView->setUrl(url);
}

void PDLCViewController::createPdlcView(bool inIgo)
{
    using namespace Origin::UIToolkit;
    mPdlcView = new PDLCView(inIgo);
    
    OriginWindow::WindowContext context = inIgo ? OriginWindow::OIG : OriginWindow::Normal;
    mPdlcWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, mPdlcView, OriginWindow::Custom, 
                                    QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, context);
    mPdlcWindow->setMaximumHeight(660);
    mPdlcWindow->setTitleBarText(tr("ebisu_client_commerce_window_title"));
    
    if(inIgo)
    {
        // Ensure we are on the OIG screen on the initial load
        ORIGIN_VERIFY_CONNECT(mPdlcView->webview(), SIGNAL(loadFinished(bool)), this, SLOT(onOIGInitialLoadFinish()));
        ORIGIN_VERIFY_CONNECT(mPdlcView, SIGNAL(cursorChanged(int)), this, SLOT(setCursor(int)));
        ORIGIN_VERIFY_CONNECT(mPdlcView, SIGNAL(closeStore()), this, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(mPdlcWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(mPdlcView, &PDLCView::sizeChanged, this, &PDLCViewController::onViewSizeChanged);
        
        if (Origin::Engine::IGOController::instance()->showWebInspectors())
            Origin::Engine::IGOController::instance()->showWebInspector(mPdlcView->webview()->page());
    }
    
    else
    {
        ORIGIN_VERIFY_CONNECT(mPdlcWindow, SIGNAL(rejected()), this, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(mPdlcView, SIGNAL(closeStore()), this, SLOT(close()));
    }
}

PDLCViewController::~PDLCViewController()
{
    close();
}

void PDLCViewController::setUrl(const QString& url)
{
    if(mPdlcView) mPdlcView->setUrl(QUrl(url));
}

void PDLCViewController::close()
{
    if(mPdlcWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mPdlcWindow, SIGNAL(rejected()), this, SLOT(close()));
        mPdlcWindow->close();
        mPdlcWindow->deleteLater();
        mPdlcWindow = NULL;
    }

    if(mPdlcView)
    {
        ORIGIN_VERIFY_DISCONNECT(mPdlcView, SIGNAL(closeStore()), this, SLOT(close()));
        ORIGIN_VERIFY_DISCONNECT(mPdlcView->webview(), SIGNAL(loadFinished(bool)), this, SLOT(onOIGInitialLoadFinish()));

        mPdlcView->close();
        mPdlcView->deleteLater();
        mPdlcView = NULL;
    }
}

void PDLCViewController::setCursor(int cursorShape)
{
    Origin::Engine::IGOController::instance()->igowm()->setCursor(cursorShape);
}

void PDLCViewController::onOIGInitialLoadFinish()
{
    using namespace Origin::UIToolkit;
    // We want to make sure the window doesn't overflow in the OIG.
    // Ensure we are on the OIG screen on the initial load
    if (mPdlcView && mPdlcView->webview())
    {
        ORIGIN_VERIFY_DISCONNECT(mPdlcView->webview(), SIGNAL(loadFinished(bool)), this, SLOT(onOIGInitialLoadFinish()));
    }

    if (Engine::IGOController::instance())
    {
        if(mPdlcWindow && (mPdlcWindow->windowContext() == OriginWindow::OIG || mPdlcWindow->manager()))
            Engine::IGOController::instance()->igowm()->ensureOnScreen(mPdlcWindow);
    }
}

void PDLCViewController::onViewSizeChanged(const QSize& size)
{
    // Re-center the dialog in IGO if its size has changed
    Origin::Engine::IGOController::instance()->igowm()->setWindowPosition(ivcView(), defaultPosition(size.width(), size.height()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
    
// IGO specific
bool PDLCViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    if(mPdlcWindow == NULL || mPdlcView == NULL)
        createPdlcView(true);

    behavior->closeOnIGOClose = true;
    behavior->closeIGOOnClose = true;
    behavior->connectionSpecific = true;
    behavior->screenListener = this;

    return true;
}

void PDLCViewController::ivcPostSetup()
{
    Origin::Engine::IIGOWindowManager::WindowProperties properties;
    properties.setPosition(defaultPosition());

    Origin::Engine::IGOController::instance()->igowm()->setWindowProperties(ivcView(), properties);
}
    
void PDLCViewController::onSizeChanged(uint32_t width, uint32_t height)
{
    Origin::Engine::IGOController::instance()->igowm()->setWindowPosition(ivcView(), defaultPosition());
}

QPoint PDLCViewController::defaultPosition(uint32_t width, uint32_t height)
{
    QSize viewSize(ivcView()->size());
    if (width > 0 && height > 0)
    {
        // Use passed size over view size.
        viewSize = QSize(width, height);
    }

    const int PDLC_STORE_Y_OFFSET = -20;
    QPoint position = Origin::Engine::IGOController::instance()->igowm()->centeredPosition(viewSize);
    position.ry() += PDLC_STORE_Y_OFFSET;
    
    return position;
}

}
}
