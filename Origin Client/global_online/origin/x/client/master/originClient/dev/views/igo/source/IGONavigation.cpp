///////////////////////////////////////////////////////////////////////////////
// IGONavigation.cpp
// 
// Created by Mike Dorval
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGONavigation.h"
#include "WebWidgetController.h"
#include "OfflineCdnNetworkAccessManager.h"
#include "webWidget/WidgetPage.h"
#include "webWidget/WidgetView.h"
#include <QPainter>
#include <qstyleoption.h>
#include <QVBoxLayout>
#include <QWebFrame>

namespace Origin
{
namespace Client
{
const QString IGONavigationURL("http://widgets.dm.origin.com/linkroles/navbarview");

IGONavigation::IGONavigation(QWidget* parent)
: Origin::Engine::IGOQWidget(parent)
{
    mWebView = new WebWidget::WidgetView();
    mWebView->widgetPage()->setExternalNetworkAccessManager(new Client::OfflineCdnNetworkAccessManager(mWebView->page()));

    setLayout(new QVBoxLayout());
    QWidget::layout()->setMargin(0);
    QWidget::layout()->setSpacing(0);
    QWidget::layout()->addWidget(mWebView);

    QPalette palette = mWebView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    //Set transparency on each widget up the chain
    //Web Page
    mWebView->page()->setPalette(palette);
    //Web View
    mWebView->setPalette(palette);
    //IGOQWidget
    setPalette(palette);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    mWebView->setAutoFillBackground(false);

    // This class is created before WebWidgetController::userLoggedIn is called. Wait for the ok from WebWidgetController.
    ORIGIN_VERIFY_CONNECT(WebWidgetController::instance(), SIGNAL(allProxiesLoaded()), this, SLOT(onProxiesLoaded()));
    // For dynamic sizing (taken from OriginWebView):
    const QSize initalSize = QSize(100, 100);
    resize(initalSize);
    mWebView->page()->setPreferredContentsSize(initalSize);
    ORIGIN_VERIFY_CONNECT_EX(
        mWebView->page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
        this, SLOT(updateSize()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(
        Origin::Engine::IGOController::instance()->gameTracker(), SIGNAL(gameFocused(uint32_t)),
        this, SLOT(updateSize()), Qt::QueuedConnection);
}


IGONavigation::~IGONavigation()
{
    mWebView->deleteLater();
    mWebView = NULL;
}

void IGONavigation::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void IGONavigation::onProxiesLoaded()
{
    WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "oig", Engine::LoginController::instance()->currentUser(), IGONavigationURL);
}


void IGONavigation::updateSize()
{
    // Set the preferred size so that qt will ignore it and finally give us the 
    // actual size of the content
    const QSize contentSize = mWebView->page()->mainFrame()->contentsSize();
    if (contentSize == QSize(0, 0))
        return;
    mWebView->page()->setPreferredContentsSize(contentSize);
    resize(contentSize);
    
    emit igoResize(contentSize);
}

} // Client
} // Origin