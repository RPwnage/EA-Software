///////////////////////////////////////////////////////////////////////////////
// EntitleFreeGameFlow.cpp
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EntitleFreeGameFlow.h"
#include "StoreUrlBuilder.h"
#include "MainFlow.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"
#include "engine/content/ContentController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/rest/HttpStatusCodes.h"

#include <QNetworkReply>

namespace Origin
{
namespace Client
{

EntitleFreeGameFlow::EntitleFreeGameFlow(const Engine::Content::EntitlementRef& entitlement)
    : AbstractFlow()
    , mEntitlement(entitlement)
    , mReply(NULL)
    , mRetryCount(0)
{
}

void EntitleFreeGameFlow::start()
{
    if (mEntitlement.isNull())
    {
        ORIGIN_LOG_ERROR << "Failed to entitle free game: null entitlement";
        EntitleFreeGameFlowResult result;
        result.result = FLOW_FAILED;
        emit finished(result);
        return;
    }

    if (mEntitlement->contentConfiguration()->productId().isEmpty())
    {
        ORIGIN_LOG_ERROR << "Failed to entitle free game: no productId";
        EntitleFreeGameFlowResult result;
        result.result = FLOW_FAILED;
        emit finished(result);
        return;
    }

    if (mReply)
    {
        ORIGIN_LOG_ERROR << "Failed to entitle free game [" << mEntitlement->contentConfiguration()->productId() << "]: request already pending";
        EntitleFreeGameFlowResult result;
        result.result = FLOW_FAILED;
        result.productId = mEntitlement->contentConfiguration()->productId();
        emit finished(result);
        return;
    }

    const QUrl& url = StoreUrlBuilder().entitleFreeGamesUrl(mEntitlement->contentConfiguration()->productId());
    makeRequest(url);
}

void EntitleFreeGameFlow::makeRequest(const QUrl& url)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    mReply = Services::NetworkAccessManager::threadDefaultInstance()->post(req, "");

    ORIGIN_VERIFY_CONNECT(mReply, &QNetworkReply::finished, this, &EntitleFreeGameFlow::onResponse);
}

void EntitleFreeGameFlow::onResponse()
{
    if (!mReply)
    {
        ORIGIN_LOG_ERROR << "Failed to entitle free game [" << mEntitlement->contentConfiguration()->productId() << "]: null reply";
        EntitleFreeGameFlowResult result;
        result.result = FLOW_FAILED;
        result.productId = mEntitlement->contentConfiguration()->productId();
        emit finished(result);
        return;
    }

    const int httpCode = mReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString& location = mReply->header(QNetworkRequest::LocationHeader).toString();

    ORIGIN_VERIFY_DISCONNECT(mReply, &QNetworkReply::finished, this, &EntitleFreeGameFlow::onResponse);
    mReply->deleteLater();
    mReply = NULL;

    if (httpCode == Services::Http301MovedPermanently || httpCode == Services::Http302Found)
    {
        makeRequest(location);
    }
    else if (httpCode >= Services::Http400ClientErrorBadRequest)
    {
        // 4XX and 5XX are errors.  Retry.
        if (mRetryCount < MAX_RETRIES)
        {
            ++mRetryCount;
            start();
        }
        else
        {
            ORIGIN_LOG_ERROR << "Failed to entitle free game [" << mEntitlement->contentConfiguration()->productId() << "]: request failed [http=" << httpCode << "]";
            EntitleFreeGameFlowResult result;
            result.result = FLOW_FAILED;
            result.productId = mEntitlement->contentConfiguration()->productId();
            emit finished(result);
        }
    }
    else
    {
        // 2XX is success!
        ORIGIN_LOG_DEBUG << "Entitle free game [" << mEntitlement->contentConfiguration()->productId() << "]: success";
        EntitleFreeGameFlowResult result;
        result.result = FLOW_SUCCEEDED;
        result.productId = mEntitlement->contentConfiguration()->productId();
        emit finished(result);

        Engine::Content::ContentController *cc = Engine::Content::ContentController::currentUserContentController();
        if (cc)
        {
            // Refresh the game library and download the new game
            cc->forceDownloadPurchasedDLCOnNextRefresh(mEntitlement->contentConfiguration()->productId());
            cc->refreshUserGameLibrary(Engine::Content::ContentUpdates);

            // If this is DLC and the base game is not installed, prompt for it to be installed.
            const Engine::Content::EntitlementRef baseGame = mEntitlement->parent();
            if (!baseGame.isNull() && !baseGame->localContent()->installed(true))
            {
                MainFlow::instance()->contentFlowController()->localContentViewController()->showParentNotInstalledPrompt(mEntitlement);
            }
        }
    }
}

}
}
