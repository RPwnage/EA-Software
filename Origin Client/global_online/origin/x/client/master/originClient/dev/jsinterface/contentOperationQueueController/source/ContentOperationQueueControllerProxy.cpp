#include "ContentOperationQueueControllerProxy.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

ContentOperationQueueControllerProxy::ContentOperationQueueControllerProxy(QObject* parent)
: QObject(parent)
, mQueueController(Engine::LoginController::currentUser()->contentOperationQueueControllerInstance())
{
	// these need to be Qt::QueuedConnection to avoid deadlocks in ContentOperationQueueController code
    ORIGIN_VERIFY_CONNECT_EX(mQueueController, SIGNAL(enqueued(Origin::Engine::Content::EntitlementRef)), this, SLOT(onEnqueued(Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mQueueController, SIGNAL(removed(const QString&, const bool&, const bool&)), this, SIGNAL(removed(const QString&, const bool&, const bool&)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mQueueController, SIGNAL(addedToComplete(Origin::Engine::Content::EntitlementRef)), this, SLOT(onAddedToComplete(Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mQueueController, SIGNAL(completeListCleared()), this, SIGNAL(completeListCleared()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mQueueController, SIGNAL(headBusy(const bool&)), this, SIGNAL(headBusy(const bool&)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mQueueController, SIGNAL(headChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), this, SLOT(onHeadChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
}


void ContentOperationQueueControllerProxy::remove(const QString& productId)
{
     mQueueController->remove(Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId), true);
}


QStringList ContentOperationQueueControllerProxy::entitlementsQueuedOfferIdList()
{
    QStringList offerIdList;

    if (Engine::LoginController::currentUser())
    {
        foreach(Engine::Content::EntitlementRef ent, mQueueController->entitlementsQueued())
        {
            offerIdList.append(ent->contentConfiguration()->productId());
        }
    }

    return offerIdList;
}


QStringList ContentOperationQueueControllerProxy::entitlementsCompletedOfferIdList()
{
    QStringList offerIdList;

    if (Engine::LoginController::currentUser())
    {
        foreach(Engine::Content::EntitlementRef ent, mQueueController->entitlementsCompleted())
        {
            offerIdList.append(ent->contentConfiguration()->productId());
        }
    }

    return offerIdList;
}

QString ContentOperationQueueControllerProxy::headOfferId() const
{
    if(mQueueController->entitlementsQueued().length())
    {
        return mQueueController->head()->contentConfiguration()->productId();
    }
    return QString();
}


void ContentOperationQueueControllerProxy::onEnqueued(Origin::Engine::Content::EntitlementRef entitlement)
{
    emit enqueued(entitlement->contentConfiguration()->productId());
}

void ContentOperationQueueControllerProxy::onAddedToComplete(Origin::Engine::Content::EntitlementRef entitlement)
{
    emit addedToComplete(entitlement->contentConfiguration()->productId());
}

void ContentOperationQueueControllerProxy::onHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead)
{
    const QString newHeadOfferId = newHead && newHead->contentConfiguration() ? newHead->contentConfiguration()->productId() : "";
    const QString oldHeadOfferId = oldHead && oldHead->contentConfiguration() ? oldHead->contentConfiguration()->productId() : "";
    emit headChanged(newHeadOfferId, oldHeadOfferId);
}

bool ContentOperationQueueControllerProxy::queueSkippingEnabled(const QString& productId)
{
    return mQueueController->queueSkippingEnabled(productId);
}

bool ContentOperationQueueControllerProxy::isHeadBusy()
{
    return mQueueController->isHeadBusy();
}

int ContentOperationQueueControllerProxy::index(const QString& productId)
{
    return mQueueController->indexInQueue(productId);
}

void ContentOperationQueueControllerProxy::pushToTop(const QString& productId)
{
    mQueueController->pushToTop(productId);
}

bool ContentOperationQueueControllerProxy::isParentInQueue(const QString& childProductId)
{
    return parentInQueue_internal(childProductId).isNull() == false;
}

QString ContentOperationQueueControllerProxy::parentOfferIdInQueue(const QString& childProductId)
{
    Engine::Content::EntitlementRef parent = parentInQueue_internal(childProductId);
    if(parent.isNull() == false)
    {
        return parent->contentConfiguration()->productId();
    }
    return QString();
}

bool ContentOperationQueueControllerProxy::isInQueue(const QString& productId)
{
    return index(productId) != -1;
}

bool ContentOperationQueueControllerProxy::isInQueueOrCompleted(const QString& productId)
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
    return (mQueueController->indexInQueue(entitlement) != -1) || (mQueueController->indexInCompleted(entitlement) != -1);
}

void ContentOperationQueueControllerProxy::clearCompleteList()
{
    mQueueController->clearCompleteList();
}

Origin::Engine::Content::EntitlementRef ContentOperationQueueControllerProxy::parentInQueue_internal(const QString& childProductId)
{
    Engine::Content::EntitlementRef dlc = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(childProductId);
    // Is game DLC?
    if(dlc.isNull() == false && dlc->parent().isNull() == false)
    {
        // The problem is.... Even though this is DLC there is a little bit of confusion as to what the "parent" product is. It could be the deluxe edition DLC, but 
        // the game the user is entitled to is the standard... or the user is entitled to both... The queue will get confused.
        QList<Engine::Content::EntitlementRef> relatedEntitlements = Engine::Content::ContentController::currentUserContentController()->entitlementsByMasterTitleId(dlc->contentConfiguration()->masterTitleId());
        foreach(Engine::Content::EntitlementRef ent, relatedEntitlements)
        {
            // If it's a base game and is in the queue
            if(ent->parent().isNull() && mQueueController->indexInQueue(ent) != -1)
            {
                return ent;
            }
        }
    }
    return Engine::Content::EntitlementRef();
}

}
}
}