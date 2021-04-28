///////////////////////////////////////////////////////////////////////////////
// RedeemBrowser.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "RedeemBrowser.h"
#include "OriginApplication.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "jshelpers/source/RedeemJsHelper.h"
#include "jshelpers/source/CommonJsHelper.h"
#include "TelemetryAPIDLL.h"
#include "engine/ito/InstallFlowManager.h"
#include <flows/MainFlow.h>
#include <flows/RTPFlow.h>
#include <QtWebKitWidgets/QWebView>
#include "StoreUrlBuilder.h"

#include "services/log/LogService.h"
#include "OriginWebController.h"

#include <QNetworkReply>
#include <QWebFrame>

RedeemBrowser::RedeemBrowser(QWidget*, SourceType sourceType, RequestorID requestorID, QFrame*, const QString &code)
    : OriginWebView()
    , mSourceType( sourceType )
    , mRequestorID( requestorID )
    , mController(NULL)
    , mRedemptionCode(code)
{
    using Origin::Client::OriginWebController;
    using Origin::Client::PageErrorDetector;

    setHasSpinner(true);
    setScaleSizeOnContentsSizeChanged(true);
    // The page will do a post-loading which will change the loading size. 
    // Let's just set the initial loading size so we won't see any weird window sizing.
    // Hopefully the size of this web page won't change. If it does - we need to update these values.
    setChangeLoadSizeAfterInitLoad(true);
    setWindowLoadingSize(QSize(696,525));
    setShowSpinnerAfterInitLoad(false);

    mController = new OriginWebController(this->webview());
    GetTelemetryInterface()->Metric_ACTIVATION_REDEEM_PAGE_REQUEST();

    Origin::Client::RedeemJsHelper* redeemHelper = NULL;

    switch (mSourceType)
    {
    case ITE:
        redeemHelper = new Origin::Client::RedeemJsHelper(Origin::Client::RedeemJsHelper::ITE, this);
        mController->jsBridge()->addHelper("redeemHelper", redeemHelper);
        Origin::Engine::InstallFlowManager::GetInstance().SetVariable("CodeRedeemed",false);
        break;
    case RTP:
        redeemHelper = new Origin::Client::RedeemJsHelper(Origin::Client::RedeemJsHelper::RTP, this);
        mController->jsBridge()->addHelper("redeemHelper", redeemHelper);
        break;
    case Origin:
        redeemHelper = new Origin::Client::RedeemJsHelper(Origin::Client::RedeemJsHelper::Origin, this);
        mController->jsBridge()->addHelper("redeemHelper", redeemHelper);
        break;
    default:
        ORIGIN_ASSERT_MESSAGE(0, "RedeemBrowser::RedeemBrowser: undefined SourceType");
    }

    if(redeemHelper)
    {
        ORIGIN_VERIFY_CONNECT(redeemHelper, SIGNAL(windowCloseRequested()), this, SIGNAL(closeWindow()));
        ORIGIN_VERIFY_CONNECT(redeemHelper, SIGNAL(windowCloseRequested()), this, SLOT(onCloseWindow()));
    }

    mController->jsBridge()->addHelper( "commonHelper", new Origin::Client::CommonJsHelper(this));

    mController->loadTrustedUrl(activationUrl());

    setVScrollPolicy(Qt::ScrollBarAlwaysOff);
    setHScrollPolicy(Qt::ScrollBarAlwaysOff);

    ORIGIN_VERIFY_CONNECT(mController->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
        this, SLOT(requestFinished(QNetworkReply*)));
}

RedeemBrowser::~RedeemBrowser()
{
}

QUrl RedeemBrowser::activationUrl() const
{
    // Use this as this is the "Access" locale which uses no_NO instead of nb_NO
    OriginApplication& app = OriginApplication::instance();
    QUrl url = QUrl(Origin::Services::readSetting(Origin::Services::SETTING_ActivationURL).toString());

    QUrlQuery activationUrlQuery;
    activationUrlQuery.addQueryItem("gameId", Origin::Client::StoreUrlBuilder().storeEnvironment());
    activationUrlQuery.addQueryItem("locale", app.locale());
    activationUrlQuery.addQueryItem("sourceType", srcType());
    if(!mRedemptionCode.isEmpty())
    {
        activationUrlQuery.addQueryItem("redeemCode", mRedemptionCode);
    }    
    QString retUrl = QUrl::toPercentEncoding(returnUrl());
    activationUrlQuery.addQueryItem("rx", retUrl.toLatin1());
    
    if( mSourceType == ITE )
    {
        // localized game name
        QString gameName = Origin::Engine::InstallFlowManager::GetInstance().GetVariable("GameName").toString();
        activationUrlQuery.addQueryItem("gameName", gameName.toUtf8());
        // first content ID on disk
        activationUrlQuery.addQueryItem("contentID", Origin::Engine::InstallFlowManager::GetInstance().GetVariable("ContentId").toString());
    }
    else if( mSourceType == RTP )
    {
        QString gameName = Origin::Client::MainFlow::instance()->rtpFlow()->getLaunchGameTitle();
        activationUrlQuery.addQueryItem("gameName", gameName.toUtf8());
        // first content id in case of rtp with multiple content ids
        QStringList launchGameIdList = Origin::Client::MainFlow::instance()->rtpFlow()->getLaunchGameIdList();
        if(!launchGameIdList.isEmpty())
        {
            activationUrlQuery.addQueryItem("contentID", launchGameIdList[0]);
        }
    }

    url.setQuery(activationUrlQuery);
    return url.toString();
}


void RedeemBrowser::changeEvent( QEvent* event )
{
    if ( event->type() == QEvent::LanguageChange )
    {
        loadRedeemURL();
    } 
    QWidget::changeEvent( event );
}


QString RedeemBrowser::srcType() const
{
    QString sourceType;
    switch( mSourceType )
    {
    case Origin:
        sourceType = "Origin";
        break;
    case ITE:
        sourceType = "ITO";
        break;
    case RTP:
        sourceType = "RTP";
        break;
    default:
        ORIGIN_ASSERT_MESSAGE(0, "RedeemBrowser::activationUrl: undefined SourceType");
    }
    return sourceType;
}

QString RedeemBrowser::returnUrl() const
{
    QString returnUrl = Origin::Services::readSetting(Origin::Services::SETTING_RedeemCodeReturnURL);
    return returnUrl.arg(srcType()).arg(OriginApplication::instance().locale());
}

void RedeemBrowser::loadRedeemURL()
{
    mController->loadTrustedUrl(activationUrl());
}

void RedeemBrowser::requestFinished(QNetworkReply *reply)
{
    // Only care about the result if it's the page we actually requested.
    if (reply && reply->url() == webview()->page()->mainFrame()->requestedUrl())
    {
        if(reply->error() == QNetworkReply::NoError)
        {
            GetTelemetryInterface()->Metric_ACTIVATION_REDEEM_PAGE_SUCCESS();
    }
        else
    {
            int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QNetworkReply::NetworkError error = reply->error();

            ORIGIN_LOG_ERROR << "Code redemption page load failed.  Http code = " << httpCode << ", error code = " << error;
            GetTelemetryInterface()->Metric_ACTIVATION_REDEEM_PAGE_ERROR(error, httpCode);
        }
    }
}

void RedeemBrowser::onCloseWindow()
{
    //  This function is called when code redemption is successful and JavaScript
    //  on page tells us to close the redeem browser.

    GetTelemetryInterface()->Metric_ACTIVATION_WINDOW_CLOSE("success");
}
