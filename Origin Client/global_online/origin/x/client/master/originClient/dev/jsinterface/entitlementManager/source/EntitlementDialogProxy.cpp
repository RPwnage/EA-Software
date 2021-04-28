#include "EntitlementDialogProxy.h"
#include "engine/content/ContentController.h"
#include "MainFlow.h"
#include "ClientFlow.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"
#include "originmsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include <QLabel>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

void EntitlementDialogProxy::showProperties(const QString& offerId)
{
    Client::MainFlow::instance()->contentFlowController()->modifyGameProperties(Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId));
}

void EntitlementDialogProxy::customizeBoxart(const QString& offerId)
{
    Client::MainFlow::instance()->contentFlowController()->customizeBoxart(Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId));
}

void EntitlementDialogProxy::promptForParentInstallation(const QString& offerId)
{
    Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showParentNotInstalledPrompt(Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId));
}

void EntitlementDialogProxy::showDownloadDebug(const QString& offerId)
{
    Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showDownloadDebugInfo(Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId));
}

void EntitlementDialogProxy::showRemoveEntitlementPrompt(const QString& offerId) 
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId);
    Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showRemoveEntitlementPrompt(entitlement, entitlement->localContent()->unInstallable());
}

}
}
}
