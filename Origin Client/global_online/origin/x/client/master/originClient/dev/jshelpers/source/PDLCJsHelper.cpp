//////////////////////////////////////////////////////////////////////////////
// PDLCJsHelper.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "PDLCJsHelper.h"

#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/igo/IGOController.h"

#include "engine/downloader/ContentInstallFlowState.h"
#include "PDLCView.h"
#include "ClientFlow.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include <QDomDocument>
#include <QtWebKitWidgets/QWebView>
#include "TelemetryAPIDLL.h"
#include "services/session/SessionService.h"
#include "services/qt/QtUtil.h"
#include "Service/SDKService/SDK_eCommerce_ServiceArea.h"
#include "services/log/LogService.h"
#include "originwindow.h"

Q_DECLARE_METATYPE(lsx::GetLockboxUrlResponseT);

namespace Origin
{
namespace Client
{

PDLCJsHelper::PDLCJsHelper(QObject* parent)
    : MyGameJsHelper(parent)
{
    mParent = dynamic_cast<PDLCView*>(this->parent());
    ORIGIN_ASSERT_MESSAGE(mParent, "PDLCJsHelper expected the parent to be a PDLCView*");

    // Initialize the lockbox url
    QMetaObject::invokeMethod(SDK::SDK_ECommerceServiceArea::instance(), "selectStore", 
        Q_ARG(QString, Services::readSetting(Services::SETTING_EbisuLockboxURL)));
}

PDLCJsHelper::~PDLCJsHelper()
{

}

void PDLCJsHelper::purchase(const QString& productId)
{
    QString userId = Origin::Services::Session::SessionService::nucleusUser(
        Origin::Services::Session::SessionService::instance()->currentSession());
    lsx::GetLockboxUrlResponseT res;
    QMetaObject::invokeMethod(SDK::SDK_ECommerceServiceArea::instance(), "getLockboxUrl", Qt::DirectConnection,
        Q_RETURN_ARG(lsx::GetLockboxUrlResponseT, res), Q_ARG(QString, mCommerceProfile), 
        Q_ARG(QString, userId), Q_ARG(QString, productId),
        Q_ARG(QString, mContext), Q_ARG(QString, mSku), 
        Q_ARG(bool, false));

    QString lockBoxUrl = res.Url;
    mParent->setUrl(lockBoxUrl);

    UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(mParent->topLevelWidget());
    if(window)
    {
        window->showWindow();
    }
}

void PDLCJsHelper::purchaseSucceeded(const QString& productId)
{
    // Remove this function once LB2 is deprecated.
    Q_UNUSED(productId);

    ORIGIN_LOG_DEBUG << "in purchaseSucceeded " << productId;

    refreshEntitlements();

    Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
    if (contentController && contentController->autoPatchingEnabled() == false)
    {   // we need to force a DLC download because auto patching isn't on a simple refresh will not start the DLC downloading (EBIBUGS-28701)
        ORIGIN_LOG_EVENT << "in purchaseSucceeded forcing download " << productId;
        contentController->forceDownloadPurchasedDLCOnNextRefresh(productId);
    }
}

void PDLCJsHelper::refreshEntitlements()
{
    // Notify the SDK that the purchase succeeded. Not waiting for any response.
    QMetaObject::invokeMethod(SDK::SDK_ECommerceServiceArea::instance(), "purchaseCompleted");
    
    // This will refresh the entitlements, and will automatically start downloading PDLC if encountered.
    Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
    if (contentController)
        contentController->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
}

void PDLCJsHelper::startDownload(const QString& productId)
{
    Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
    if (contentController)
    {
        contentController->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
    }

    MyGameJsHelper::startDownload(productId);
}

void PDLCJsHelper::launchExternalBrowser(QString urlstring)
{
    QUrl url(QUrl::fromEncoded(urlstring.toUtf8()));

    if (Engine::IGOController::instance()->isGameInForeground())
    {
		emit Engine::IGOController::instance()->showBrowser(urlstring, true);
    }
    else
    {
    	Origin::Services::PlatformService::asyncOpenUrl(url);
    }
    GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
}

void PDLCJsHelper::startDownloadEx(const QString& entitlementTags, const QString& groupNames)
{
    Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
    if (!contentController)
        return;

    // This is called from Lockbox and assumes the entitlements are already on the client.
    QList<Origin::Engine::Content::EntitlementRef> entitlements = contentController->entitlements();

    QStringList tags = entitlementTags.split(',');
    QStringList groups = groupNames.split(',');

    QList<Origin::Engine::Content::EntitlementRef>::const_iterator it;
    for(it = entitlements.constBegin(); it != entitlements.constEnd(); ++it)
    {
        Origin::Engine::Content::EntitlementRef e = *it;
        Origin::Engine::Content::ContentConfigurationRef l = e->contentConfiguration();
        if ( tags.contains(l->entitlementTag()) && groups.contains(l->groupName()) )
        {
            Origin::Downloader::IContentInstallFlow * flow = e->localContent()->installFlow();
            if ( flow->canBegin() )
            {
                flow->begin();
            }
        }
    }
}

void PDLCJsHelper::close()
{
    emit closeStore();
}

void PDLCJsHelper::continueShopping(const QString& url)
{
    if(url.isEmpty() || url.compare("home", Qt::CaseInsensitive) == 0)
    {
        // Recalculate "home" URL.  This will pick the correct 
        // addon store and use the correct sizing.
        mParent->navigate();
    }
    else
    {
        mParent->setUrl(url);
    }
}

void PDLCJsHelper::print()
{
    emit printRequested();
}

}
}
